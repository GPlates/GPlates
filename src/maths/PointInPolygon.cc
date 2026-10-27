/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <algorithm>
#include <cmath>
#include <iterator>
#include <memory>
#include <utility>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <QDebug>

#include "PointInPolygon.h"

#include "AngularDistance.h"
#include "AngularExtent.h"
#include "Centroid.h"
#include "GeometryDistance.h"
#include "GreatCircleArc.h"
#include "MathsUtils.h"
#include "PointOnSphere.h"
#include "SmallCircleBounds.h"
#include "SphericalArea.h"
#include "Vector3D.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "utils/Profile.h"

namespace GPlatesMaths
{
	namespace PointInPolygon
	{
		/**
		 * If dot product of crossing arc and polygon edge greater than this then too closely aligned.
		 *
		 * Note: Choosing a larger epsilon than default to ensure we don't have numerical issues.
		 *       We just want to avoid the more expensive test that is required when the planes align,
		 *       so the actual epsilon we use is not important.
		 */
		const double MAX_DOT_PRODUCT_CROSSING_ARC_AND_POLYGON_EDGE = 1 - 1e-4;
		//! If dot product of crossing arc and polygon edge less than this then too closely aligned.
		const double MIN_DOT_PRODUCT_CROSSING_ARC_AND_POLYGON_EDGE = -MAX_DOT_PRODUCT_CROSSING_ARC_AND_POLYGON_EDGE;


		/**
		 * The point-ON-polygon threshold distance: a test point within this distance of the polygon
		 * outline (of an edge, or a vertex) lies "on" the outline.
		 */
		const AngularExtent &
		get_point_on_polygon_outline_threshold()
		{
			static const AngularExtent THRESHOLD =
					AngularExtent::create_from_cosine(get_point_on_polygon_threshold_cosine());

			return THRESHOLD;
		}


		/**
		 * How far the inner/outer bounding small circles (centred on the outside point) are
		 * contracted/expanded beyond the polygon edges.
		 *
		 * A bounds test that decides a point (clearly inside the inner circle, or clearly outside
		 * the outer circle) skips the on-outline test, so the bounds are widened by more than the
		 * point-ON-polygon threshold. Otherwise a point within the threshold of the outline could
		 * be decided by the bounds as outside, while the O(N) test (no bounds) finds it on the
		 * outline, and the answer would depend on the speed. The extra margin (the zero-length
		 * threshold of about 1e-6 radians, which is the default widening) covers the bounds test
		 * comparing cosines, which resolves angles near zero and pi only to about 1.5e-8 radians.
		 */
		const AngularExtent &
		get_bounds_widening()
		{
			static const AngularExtent WIDENING =
					get_point_on_polygon_outline_threshold() +
					AngularExtent::create_from_cosine(
							GreatCircleArc::get_zero_length_threshold_cosine());

			return WIDENING;
		}


		/**
		 * Returns the normal (rotation axis) of the arc joining the start of the crossings arc (the
		 * polygon's outside point, or its second reference point) and the test point.
		 *
		 * If the arc is zero length or the points are antipodal to each other then
		 * a normal to an arbitrary great circle passing through both points is returned.
		 */
		UnitVector3D
		get_crossings_arc_plane_normal(
				const UnitVector3D &crossing_arc_start_point,
				const UnitVector3D &crossing_arc_end_point)
		{
			const Vector3D arc_normal = cross(crossing_arc_start_point, crossing_arc_end_point);

			if (arc_normal.magSqrd() <= 0 /*note: this is an epsilon test*/)
			{
				// The arc end points are the same or antipodal (within epsilon). Pick any plane
				// that passes through the end point. Its orientation is arbitrary, which is fine
				// only if the plane also contains the start point (any arc joining exactly
				// antipodal points gives the same parity), and that is only true if the end points
				// are *exactly* antipodal. So this is only used for the crossings arc from the
				// outside point to its antipode (its exact negation). A test point this close to
				// either of them is handled separately (see
				// 'is_point_in_polygon_with_degenerate_crossings_arc()').
				return generate_perpendicular(crossing_arc_end_point);
			}

			return arc_normal.get_normalisation();
		}


		/**
		 * Returns true if the intersection of polygon edge with crossing arc *great circle* lies on
		 * the crossing *arc* defined by @a crossing_arc_start_point, @a crossing_arc_end_point and
		 * @a dot_crossing_arc_end_points.
		 *
		 * This only decides crossing parity. Whether the crossing arc end point (the point being
		 * tested) lies "on" the polygon outline is decided separately (see
		 * @a is_point_on_polygon_outline).
		 *
		 * @pre @a intersection_point must lie on the great circle of the crossing arc (within a suitable epsilon).
		 */
		bool
		does_polygon_edge_intersection_with_crossing_gc_lie_on_crossing_gca(
				const UnitVector3D &crossings_arc_start_point,
				const UnitVector3D &crossings_arc_rotation_axis,
				const double &dot_crossings_arc_end_points,
				const UnitVector3D &intersection_point)
		{
			// Is the intersection point closer to the crossings arc start point than the
			// crossings arc end point is...
			return dot(crossings_arc_start_point, intersection_point).dval() >= dot_crossings_arc_end_points &&
				// ...and does the intersection point lie on the half-circle starting at the crossings arc start point...
				dot(cross(crossings_arc_start_point, intersection_point), crossings_arc_rotation_axis).dval() >= 0;
		}


		/**
		 * Returns the intersection of the great circle arc @a gca and the
		 * plane (passing through origin) with normal @a plane_normal.
		 *
		 * @pre the endpoints of the great circle arc @a gca must be on opposite
		 * sides of the plane.
		 * @pre the great circle arc @a gca must not be zero-length.
		 */
		UnitVector3D
		if_plane_divides_gca_get_intersection_of_gca_and_plane(
				const GreatCircleArc &gca,
				const UnitVector3D &plane_normal)
		{
			const UnitVector3D &gca_start_point(gca.start_point().position_vector());
			const UnitVector3D &gca_end_point(gca.end_point().position_vector());

			// Determine the signed distances of the arc endpoints from the plane.
			const double signed_distance_gca_start_to_plane =
					dot(plane_normal, gca_start_point).dval();
			const double signed_distance_gca_end_to_plane =
					dot(plane_normal, gca_end_point).dval();

			// See if the arc endpoints are antipodal.
			const Vector3D gca_midpoint = Vector3D(gca_start_point) + Vector3D(gca_end_point);
			// NOTE: We're avoiding the more expensive square-root calculation here.
			if (gca_midpoint.magSqrd().dval() < 1e-6 /*equivalent to a magnitude of 1e-3*/)
			{
				// The arc endpoints are antipodal (shouldn't be able to get an arc
				// that spans a full half-circle but it can happen within numerical tolerance).
				// Because the points are antipodal the absolute value of their signed
				// distances from any plane (passing through origin) will always be equal
				// regardless of the orientation of the splitting plane.
				// This means the intersection cannot be calculated using signed distance ratios.
				// The solution is to divide the half-circle arc into two arcs of equal length
				// and determine the intersection from those.
				const UnitVector3D gca_midpoint_norm(cross(gca.rotation_axis(), gca_start_point));

#if 0
				// Determine the signed distance of the arc midpoint from the plane.
				const double signed_distance_gca_mid_to_plane =
						dot(plane_normal, gca_midpoint_norm).dval();
#endif

				// Find out which sides of the plane the arc endpoints and midpoint are.
				// Note that we must use the same comparisons as the main function -
				// FIXME: make this more obvious in the code somehow.
				const bool is_arc_start_on_negative_side_of_plane =
						dot(plane_normal, gca_start_point).dval() < 0;
				const bool is_arc_mid_on_negative_side_of_plane =
						dot(plane_normal, gca_midpoint_norm).dval() < 0;
#if 0
				const bool is_arc_end_on_negative_side_of_plane =
						dot(plane_normal, gca_end_point).dval() < 0;
#endif

				// If the plane divides the first arc.
				if (is_arc_start_on_negative_side_of_plane ^
					is_arc_mid_on_negative_side_of_plane)
				{
					const GreatCircleArc first_gca = GreatCircleArc::create(
							PointOnSphere(gca_start_point), PointOnSphere(gca_midpoint_norm));

					return if_plane_divides_gca_get_intersection_of_gca_and_plane(
							first_gca, plane_normal);
				}
				// ...else the plane must divide the second arc (we know this because the
				// precondition to this function is the plane must divide the original arc).

				const GreatCircleArc second_gca = GreatCircleArc::create(
						PointOnSphere(gca_midpoint_norm), PointOnSphere(gca_end_point));

				return if_plane_divides_gca_get_intersection_of_gca_and_plane(
						second_gca, plane_normal);
			}

			// The denominator of the ratios used to interpolate the arc endpoints.
			const real_t denom =
					signed_distance_gca_start_to_plane - signed_distance_gca_end_to_plane;
			if (denom == 0 /*this is an epsilon test*/)
			{
				// The great circle arc is so closely aligned with the plane that we cannot
				// use signed distance ratios to determine the intersection.
				// Arbitrarily choose the arc midpoint as the intersection - this is ok
				// because if the crossings-arc endpoint (the test point for the point-in-poly
				// test) lands near the arc then we are testing a point-in-poly near a polygon
				// edge (the arc) and the result is allowed to be arbitrary - but if the
				// crossings-arc endpoint is not near the arc then the point-in-poly result
				// should *not* be arbitrary (because it should be clear whether the point is
				// inside or outside the polygon) but our arbitrary choice of intersection
				// as the arc midpoint should not affect the point-in-poly test in this case
				// (because the intersection point will be clearly inside or outside the
				// crossings-arc - that test is not done in this function though).

				// Return the arc midpoint.
				// We know the midpoint is not near the origin (within epsilon) since
				// we tested above - so we know we can normalise it.
				return gca_midpoint.get_normalisation();
			}
			const real_t inv_denom = 1.0 / denom;

			// Interpolate the arc endpoints based on the signed distances and then
			// normalise to get a unit vector.
			// It should be safe to normalise the result because we've already determined
			// that the arc end points are *not* antipodal and hence a linear interpolation
			// of them should not be near the origin (within epsilon).
			return
				((signed_distance_gca_start_to_plane * inv_denom) * gca_end_point -
					(signed_distance_gca_end_to_plane * inv_denom) * gca_start_point)
				.get_normalisation();
		}


		/**
		 * Returns the number of polygon edges crossed by the great circle arc joining
		 * @a crossings_arc_start_point and @a crossings_arc_end_point.
		 *
		 * The sequence of edges begins at @a edges_begin and ends
		 * at @a edges_end. Note that the sequence of edges must *not* wrap
		 * past the last edge of the polygon.
		 *
		 * @a crossings_arc_plane_normal is the plane normal of the great circle
		 * passing through the crossing arc points.
		 * If the arc is zero length or the arc endpoints are antipodal then the plane normal
		 * can be any plane that passes through one (and hence both) of the arc points.
		 *
		 * This only decides crossing parity, without a threshold. Whether
		 * @a crossings_arc_end_point lies "on" the polygon outline is decided separately (see
		 * @a is_point_on_polygon_outline). The two used to be decided together, by relaxing
		 * the test of which side of an edge's plane the crossing arc end point is on (counting a
		 * crossing, and the point as on the outline, if within the threshold of the plane). But
		 * then the crossing arc no longer had to reach the edge's plane, so the pairing tests below
		 * no longer implied an intersection: a point near the great circle of an edge, but beyond
		 * its end (up to the threshold divided by the sine of the angle between the edge and the
		 * crossing arc), or near an edge at its antipode, was "on" the outline. And a point at a
		 * vertex, or near an edge almost parallel to the crossing arc, was not.
		 */
		unsigned int
		get_num_polygon_edges_crossed(
				const PolygonOnSphere::ring_const_iterator &edges_begin,
				const PolygonOnSphere::ring_const_iterator &edges_end,
				const UnitVector3D &crossings_arc_start_point,
				const UnitVector3D &crossings_arc_end_point,
				const UnitVector3D &crossings_arc_plane_normal,
				const double &dot_crossings_arc_end_points)
		{
			if (edges_begin == edges_end)
			{
				return 0;
			}

			unsigned int num_edges_crossed = 0;
			PolygonOnSphere::ring_const_iterator edge_iter = edges_begin;

			//
			// If the arc intersects a polygon vertex we need to make sure the
			// number of crossings is counted correctly for the two polygon edges
			// adjacent to the intersected polygon vertex:
			// (1) counted once if crossings arc goes from inside to outside or outside
			//     to inside polygon,
			// (2) counted twice if crossings arc goes from inside to inside or outside
			//     to outside polygon.
			// This is done by determining which side of the arc plane each polygon vertex
			// is on and only incrementing the crossings counter whenever the status of
			// two vertices of a polygon edge differ.
			//

			// Determine which side of arc plane the start vertex of the first edge is on.
			// Note: this is *not* an epsilon test.
			const GreatCircleArc &first_edge = *edge_iter;
			bool is_edge_start_on_negative_side_of_crossings_arc_plane =
					dot(crossings_arc_plane_normal,
						first_edge.start_point().position_vector()).dval() < 0;

			// Iterate over the polygon edges in the sequence.
			do
			{
				const GreatCircleArc &edge = *edge_iter;
				const UnitVector3D &edge_end_point = edge.end_point().position_vector();

				// Determine which side of arc plane the end vertex of the current edge is on.
				// Note: this is *not* an epsilon test.
				// Note: this should be the exact same comparison as with the start vertex.
				const bool is_edge_end_on_negative_side_of_crossings_arc_plane =
						dot(crossings_arc_plane_normal, edge_end_point).dval() < 0;

				// If the start and end of the current edge are on different sides
				// of the arc plane then it's possible we have a crossing.
				//
				// NOTE: This test must return true *only* if the signed distance of
				// one vertex is "< 0" and the other is ">= 0". And this cannot be done
				// by multiplying the signed distances and testing for "<=0" - in other
				// words each signed distance must be tested individually and then the
				// *boolean* results should be compared.
				if (is_edge_start_on_negative_side_of_crossings_arc_plane ^
					is_edge_end_on_negative_side_of_crossings_arc_plane)
				{
					// It's possible that the polygon edge (GCA) is classified as zero
					// length (and hence has no rotation axis) but the edge points could
					// still be separated by a distance less than epsilon and hence end up
					// on different sides of the crossing plane (which effectively means the
					// edge end points lie, within epsilon, on the crossing plane).
					// In this case we can test for a crossing by simply determining if one
					// of the edge points lies on the crossing arc.
					if (edge.is_zero_length())
					{
						// Both polygon edge start and end points are extremely close together (or the same).
						// Arbitrarily choose the end point of the polygon edge as the intersection point
						// with the great circle of the crossings arc.
						if (does_polygon_edge_intersection_with_crossing_gc_lie_on_crossing_gca(
								crossings_arc_start_point,
								crossings_arc_plane_normal,
								dot_crossings_arc_end_points,
								edge_end_point/*intersection_point*/))
						{
							++num_edges_crossed;
						}
					}
					else // The polygon edge (GCA) has a rotation axis (plane normal)...
					{
						const UnitVector3D &edge_plane_normal = edge.rotation_axis();

						// Get a measure of how closely aligned the polygon edge arc
						// is relative to the crossings arc.
						const double crossings_arc_normal_dot_edge_normal =
								dot(crossings_arc_plane_normal, edge_plane_normal).dval();

						// If the polygon edge is *not* too closely aligned with the crossing
						// arc then we can see if the polygon edge plane separates the crossing
						// arc end points.
						// NOTE: choosing a larger epsilon than default to ensure we don't
						// have numerical issues such as both crossing-arc endpoints calculated
						// to be on the same side of polygon edge plane (and hence no crossing
						// detected) when it's quite clear that a crossing has occurred.
						// We just want to avoid the more expensive test that is required when
						// the planes align so the actual epsilon we use is not important
						// (it's just used to funnel the majority of relative plane
						// orientations that occur in practice, ie not aligned, into a
						// less expensive test).
						if (crossings_arc_normal_dot_edge_normal <
								MAX_DOT_PRODUCT_CROSSING_ARC_AND_POLYGON_EDGE &&
							crossings_arc_normal_dot_edge_normal >
								MIN_DOT_PRODUCT_CROSSING_ARC_AND_POLYGON_EDGE)
						{
							// We now know that the two polygon edge endpoints are
							// on opposite sides of the crossings arc plane.
							// So now the two arcs intersect only if:
							// 1) both endpoints of the crossings-arc are on opposite sides of
							//    the polygon edge plane, *and*
							// 2) the start points of each arc are on different sides of
							//    the other arc's plane (eg, if the polygon edge start point
							//    is on the *negative* side of the crossings-arc plane then
							//    the crossings-arc start point must be on the *positive* side
							//    of the polygon edge plane).
							// Condition (2) is required so we don't return an intersection
							// when the arcs are on the opposite sides of the globe (even
							// though the plane of each arc divides the other arc's endpoints).
							if (is_edge_start_on_negative_side_of_crossings_arc_plane)
							{
								// The crossings-arc endpoints must be on opposite sides of
								// the edge plane *and* the crossings-arc start point must
								// be on the *positive* side since the polygon edge start point
								// is on the *negative* side of the crossings-arc plane.
								if (dot(edge_plane_normal, crossings_arc_start_point).dval() >= 0 &&
									dot(edge_plane_normal, crossings_arc_end_point).dval() <= 0)
								{
									++num_edges_crossed;
								}
							}
							else // edge start point is on positive side of crossings-arc plane...
							{
								// The crossings-arc endpoints must be on opposite sides of
								// the edge plane *and* the crossings-arc start point must
								// be on the *negative* side since the polygon edge start point
								// is on the *positive* side of the crossings-arc plane.
								if (dot(edge_plane_normal, crossings_arc_start_point).dval() <= 0 &&
									dot(edge_plane_normal, crossings_arc_end_point).dval() >= 0)
								{
									++num_edges_crossed;
								}
							}
						}
						else // polygon edge is too closely aligned with the crossing arc...
						{
							// Since the polygon edge is too closely aligned with the crossing
							// arc plane we cannot use the polygon edge plane to determine
							// if an intersection occurred.
							// Instead determine the point where the crossing plane intersects
							// the polygon edge by using the calculated signed distances
							// of the crossing arc endpoints from the polygon edge plane and
							// then interpolating between them based on the distance ratio.

							// NOTE: We've already calculated the signed distances above
							// but we calculate them again here because we should only
							// get here rarely and we want to avoid storing and copying
							// the signed distance in each iteration of the main loop
							// (like we do the edge start flags) since that adds an overhead
							// to the non-rare cases.
							const UnitVector3D ring_edge_intersects_crossings_arc_plane =
									if_plane_divides_gca_get_intersection_of_gca_and_plane(
											edge, crossings_arc_plane_normal);

							// If the intersection of polygon edge with great circle of crossings arc lies
							// on the crossings *arc* then increment the number of polygon edges crossed.
							if (does_polygon_edge_intersection_with_crossing_gc_lie_on_crossing_gca(
									crossings_arc_start_point,
									crossings_arc_plane_normal,
									dot_crossings_arc_end_points,
									// Intersection of polygon edge with great circle of crossings arc...
									ring_edge_intersects_crossings_arc_plane/*intersection_point*/))
							{
								++num_edges_crossed;
							}
						}
					}
				}

				// The start point of the next edge is the end point of the current edge.
				is_edge_start_on_negative_side_of_crossings_arc_plane =
						is_edge_end_on_negative_side_of_crossings_arc_plane;
			}
			// The sequence of edges will not wrap past the last polygon edge.
			// If there is a sequence that does wrap it will be divided into two
			// sequences by the caller.
			while (++edge_iter != edges_end);

			return num_edges_crossed;
		}


		/**
		 * Returns true if @a point is within the point-ON-polygon threshold distance of any
		 * edge in the sequence of edges (an edge's closest point can be one of its end points, a
		 * polygon vertex).
		 */
		bool
		is_point_on_polygon_outline(
				const PolygonOnSphere::ring_const_iterator &edges_begin,
				const PolygonOnSphere::ring_const_iterator &edges_end,
				const UnitVector3D &point)
		{
			// A point within the threshold of an edge is also within it of the edge's great circle.
			// That rejects most edges with one dot product (the sine of the distance to the great
			// circle), before the full distance to the edge. The margin is larger than the
			// precision with which the full distance resolves the sine near the threshold: it
			// compares the cosine, 'sqrt(1 - sine^2)', and near the threshold one ULP of that
			// cosine is about 8e-11 of the sine, so a margin any smaller could reject an edge the
			// full distance accepts.
			static const double MAX_SINE_DISTANCE_TO_GREAT_CIRCLE =
					get_point_on_polygon_threshold_sine() + 1e-9;

			for (PolygonOnSphere::ring_const_iterator edge_iter = edges_begin;
				edge_iter != edges_end;
				++edge_iter)
			{
				const GreatCircleArc &edge = *edge_iter;

				if (!edge.is_zero_length() &&
					std::fabs(dot(edge.rotation_axis(), point).dval()) >
						MAX_SINE_DISTANCE_TO_GREAT_CIRCLE)
				{
					continue;
				}

				if (minimum_distance(point, edge, get_point_on_polygon_outline_threshold()))
				{
					return true;
				}
			}

			return false;
		}


		/**
		 * Returns true if @a point is within the point-ON-polygon threshold distance of the
		 * polygon's outline (its exterior ring or any interior ring).
		 */
		bool
		is_point_on_polygon_outline(
				const PolygonOnSphere &polygon,
				const UnitVector3D &point)
		{
			if (is_point_on_polygon_outline(
					polygon.exterior_ring_begin(),
					polygon.exterior_ring_end(),
					point))
			{
				return true;
			}

			for (unsigned int interior_ring_index = 0;
				interior_ring_index < polygon.number_of_interior_rings();
				++interior_ring_index)
			{
				if (is_point_on_polygon_outline(
						polygon.interior_ring_begin(interior_ring_index),
						polygon.interior_ring_end(interior_ring_index),
						point))
				{
					return true;
				}
			}

			return false;
		}


		/**
		 * Returns true if the test point (which is @a crossings_arc_end_point) is inside polygon
		 * (taking into account exterior and interior rings).
		 *
		 * The great circle arc joining @a crossings_arc_start_point and @a crossings_arc_end_point is
		 * tested to see how many polygon edges are crossed (including interior rings).
		 */
		bool
		is_point_in_polygon(
				const PolygonOnSphere &polygon,
				const UnitVector3D &crossings_arc_start_point,
				const UnitVector3D &crossings_arc_end_point,
				bool use_point_on_polygon_threshold)
		{
			const UnitVector3D crossings_arc_plane_normal = get_crossings_arc_plane_normal(
					crossings_arc_start_point, crossings_arc_end_point);

			const double dot_crossings_arc_end_points =
					dot(crossings_arc_start_point, crossings_arc_end_point).dval();

			// The polygon has an exterior ring and zero or more interior rings.
			//
			// Add the number of crossings from all rings (exterior and interiors).

			unsigned int num_polygon_edges_crossed = get_num_polygon_edges_crossed(
					polygon.exterior_ring_begin(), polygon.exterior_ring_end(),
					crossings_arc_start_point, crossings_arc_end_point,
					crossings_arc_plane_normal, dot_crossings_arc_end_points);

			for (unsigned int interior_ring_index = 0;
				interior_ring_index < polygon.number_of_interior_rings();
				++interior_ring_index)
			{
				num_polygon_edges_crossed += get_num_polygon_edges_crossed(
						polygon.interior_ring_begin(interior_ring_index),
						polygon.interior_ring_end(interior_ring_index),
						crossings_arc_start_point, crossings_arc_end_point,
						crossings_arc_plane_normal, dot_crossings_arc_end_points);
			}

			// If the number of edges crossed is odd then test point is inside the polygon.
			if ((num_polygon_edges_crossed & 1) == 1)
			{
				return true;
			}

			// Otherwise, if the test point lies *on* any polygon outline (exterior or interior)
			// then it's considered to be *inside* the polygon. Otherwise the test point is outside
			// the polygon.
			return use_point_on_polygon_threshold &&
					is_point_on_polygon_outline(polygon, crossings_arc_end_point);
		}


		/**
		 * Returns true if the crossings arc from @a crossings_arc_start_point (the outside point)
		 * to @a test_point is too short, or too close to antipodal, for its plane to be defined.
		 *
		 * This is when @a test_point is within about 1e-6 radians (the cross product of the
		 * arc end points is zero within epsilon) of the outside point or of its antipode. The
		 * plane chosen then (see @a get_crossings_arc_plane_normal) contains the test point
		 * but not, in general, the outside point, so the test of which side of each polygon
		 * edge's plane the outside point is on has no meaning, and nor does the crossing
		 * parity.
		 */
		bool
		is_crossings_arc_degenerate(
				const UnitVector3D &crossings_arc_start_point,
				const UnitVector3D &test_point)
		{
			return cross(crossings_arc_start_point, test_point).magSqrd() <=
					0 /*note: this is an epsilon test*/;
		}


		/**
		 * Returns true if @a test_point is inside @a polygon, when the crossings arc from the
		 * polygon's outside point to @a test_point is degenerate (see
		 * @a is_crossings_arc_degenerate).
		 *
		 * The crossings are counted instead from the polygon's second reference point, about 90
		 * degrees from the outside point, whose status is cached. That arc is well defined, so the
		 * answer is as exact as any other test point's, with or without the threshold.
		 *
		 * (Before the second reference point, such a test point took the status of the outside
		 * point, or of its antipode. That was exact with the threshold, since about 1e-6 radians is
		 * less than the threshold, but not without it: an edge could pass between the test point
		 * and the point whose status it took, as at a bow-tie's or a sliver's centroid.)
		 */
		bool
		is_point_in_polygon_with_degenerate_crossings_arc(
				const PolygonOnSphere &polygon,
				const UnitVector3D &test_point,
				bool use_point_on_polygon_threshold)
		{
			const SecondReferencePoint &second_reference_point =
					polygon.get_second_reference_point();

			// The crossing parity from the second reference point, which tells whether the test point
			// has the same status.
			const bool is_crossing_parity_odd = is_point_in_polygon(
					polygon,
					second_reference_point.point,
					test_point,
					false/*use_point_on_polygon_threshold*/);
			if (is_crossing_parity_odd != second_reference_point.is_in_polygon)
			{
				return true;
			}

			return use_point_on_polygon_threshold &&
					is_point_on_polygon_outline(polygon, test_point);
		}


		//! Keeps track of an iteration range of polygon edges.
		class EdgeSequence
		{
		public:
			EdgeSequence(
					const PolygonOnSphere::ring_const_iterator &begin_,
					const PolygonOnSphere::ring_const_iterator &end_) :
				begin(begin_),
				end(end_)
			{  }

			PolygonOnSphere::ring_const_iterator begin;
			PolygonOnSphere::ring_const_iterator end;
		};

		//! Typedef for a list of polygon edge ranges.
		typedef std::vector<EdgeSequence> edge_sequence_list_type;


		/**
		 * Returns true if the test point (which is @a crossings_arc_end_point) is inside polygon.
		 *
		 * The great circle arc joining @a crossings_arc_start_point and @a crossings_arc_end_point is
		 * tested to see how many edges are crossed.
		 *
		 * The polygon edges are defined by a sequence of edge sequences
		 * @a edge_sequences_begin and @a edge_sequences_end.
		 * These must include all polygon edges (exterior and interior) that the crossings arc can
		 * potentially cross, and (if @a use_point_on_polygon_threshold is true) all edges
		 * within the point-ON-polygon threshold of the test point. Any other edges can be excluded.
		 */
		bool
		is_point_in_polygon(
				const edge_sequence_list_type::const_iterator &edge_sequences_begin,
				const edge_sequence_list_type::const_iterator &edge_sequences_end,
				const UnitVector3D &crossings_arc_start_point,
				const UnitVector3D &crossings_arc_end_point,
				bool use_point_on_polygon_threshold)
		{
			const UnitVector3D crossings_arc_plane_normal = get_crossings_arc_plane_normal(
					crossings_arc_start_point, crossings_arc_end_point);

			const double dot_crossings_arc_end_points =
					dot(crossings_arc_start_point, crossings_arc_end_point).dval();

			unsigned int num_polygon_edges_crossed = 0;

			// Iterate over the edge sequences and see how many edges cross the crossings-arc.
			edge_sequence_list_type::const_iterator edge_sequences_iter = edge_sequences_begin;
			for ( ; edge_sequences_iter != edge_sequences_end; ++edge_sequences_iter)
			{
				const EdgeSequence &edge_sequence = *edge_sequences_iter;

				num_polygon_edges_crossed += get_num_polygon_edges_crossed(
						edge_sequence.begin, edge_sequence.end,
						crossings_arc_start_point, crossings_arc_end_point,
						crossings_arc_plane_normal, dot_crossings_arc_end_points);
			}

			// If the number of edges crossed is odd then test point is inside the polygon.
			if ((num_polygon_edges_crossed & 1) == 1)
			{
				return true;
			}

			// Otherwise, if the test point lies *on* any polygon outline (exterior or interior)
			// then it's considered to be *inside* the polygon. Otherwise the test point is outside
			// the polygon.
			if (!use_point_on_polygon_threshold)
			{
				return false;
			}

			for (edge_sequences_iter = edge_sequences_begin;
				edge_sequences_iter != edge_sequences_end;
				++edge_sequences_iter)
			{
				const EdgeSequence &edge_sequence = *edge_sequences_iter;

				if (is_point_on_polygon_outline(
						edge_sequence.begin,
						edge_sequence.end,
						crossings_arc_end_point))
				{
					return true;
				}
			}

			return false;
		}


		/**
		 * A recursive spatial partition of the surface of the sphere into spherical lunes.
		 *
		 * A spherical lune is the surface of the sphere bounded by two half great circles
		 * that intersect along a common axis. The volume bounded by the two semi-planes
		 * (of the half circles) and the spherical lune is a spherical wedge.
		 *
		 * The common axis is the polygon's outside point and its antipode, and the spherical
		 * lunes partition the possible paths that a crossings-arc can take from the outside
		 * point to any test point (point-in-polygon) on the globe.
		 *
		 * The tree starts off as four equal-area partitions of the entire sphere and then
		 * each node in the tree is partitioned into two child nodes of equal area.
		 * Traversing the tree we can quickly find the subset of polygon edges (that intersect
		 * a spherical lune leaf node) that a crossings-arc can potentially cross thus limiting
		 * the number of polygon edges we need to test crossings for. This reduces the cost
		 * from O(N) to O(log(N)) in the number of polygon edges N.
		 */
		class SphericalLuneTree
		{
		public:
			/**
			 * Creates a @a SphericalLuneTree.
			 */
			static
			std::unique_ptr<SphericalLuneTree>
			create(
					const PolygonOnSphere::non_null_ptr_to_const_type &polygon,
					bool build_ologn_hint,
					bool keep_shared_reference_to_polygon);


			bool
			is_point_in_polygon(
					const UnitVector3D &test_point,
					bool use_point_on_polygon_threshold) const;


			/**
			 * The number of calls to @a is_point_in_polygon that tested polygon edges (that
			 * the bounds did not decide).
			 */
			unsigned int
			get_num_queries_testing_edges() const
			{
				return d_num_queries_testing_edges;
			}

		private:
			//! Typedef for an index into a sequence of @a EdgeSequence objects.
			typedef unsigned int edge_sequence_index_type;

			//! Typedef for a node index (can be internal or leaf).
			typedef unsigned int node_index_type;

			//! An internal node of spherical lune tree (has two child nodes).
			class InternalNode
			{
			public:
				InternalNode(
						const UnitVector3D splitting_plane_,
						node_index_type first_child_node_index,
						node_index_type second_child_node_index,
						bool is_first_child_internal,
						bool is_second_child_internal) :
					splitting_plane(splitting_plane_)
				{
					child_node_indices[0] = first_child_node_index;
					child_node_indices[1] = second_child_node_index;
					is_child_node_internal[0] = is_first_child_internal;
					is_child_node_internal[1] = is_second_child_internal;
				}

				//! Normal to the plane that splits the current node into two child nodes.
				UnitVector3D splitting_plane;

				//! The first/second child node is on the positive/negative side of the plane.
				node_index_type child_node_indices[2];

				//! The type of each child node (internal or leaf).
				bool is_child_node_internal[2];
			};
			typedef std::vector<InternalNode> internal_node_seq_type;

			/**
			 * A leaf node of spherical lune tree.
			 *
			 * Contains a list of polygon edge ranges for the polygon edges
			 * that intersect the spherical lune represented by the leaf node.
			 */
			class LeafNode
			{
			public:
				LeafNode(
						edge_sequence_index_type edge_sequences_start_index_,
						unsigned int num_edge_sequences_,
						const InnerOuterBoundingSmallCircle &outside_point_bounds_) :
					edge_sequences_start_index(edge_sequences_start_index_),
					num_edge_sequences(num_edge_sequences_),
					outside_point_bounds(outside_point_bounds_)
				{  }

				/**
				 * Default constructor for the case where no sequences intersect spherical lune of leaf node.
				 *
				 * This happens when the polygon does not reach the spherical lune (a horseshoe may
				 * not, for example).
				 */
				LeafNode() :
					edge_sequences_start_index(0),
					num_edge_sequences(0)
				{  }

				edge_sequence_index_type edge_sequences_start_index;
				unsigned int num_edge_sequences;
				boost::optional<InnerOuterBoundingSmallCircle> outside_point_bounds;
			};
			typedef std::vector<LeafNode> leaf_node_seq_type;

			/**
			 * The data for the spherical lune tree - the code is in class @a SphericalLuneTree -
			 * this is so the @a TreeBuilder can build the tree data and pass it to the
			 * @a SphericalLuneTree.
			 */
			class TreeData
			{
			public:
				//! Default constructor.
				TreeData() :
					d_root_node_index(0)
				{  }

				//! The root node index (into the internal nodes).
				node_index_type d_root_node_index;

				/**
				 * Contains all the internal nodes except the root node. This is done to
				 * keep the nodes together in memory and reduce memory allocations.
				 */
				internal_node_seq_type d_internal_nodes;

				/**
				 * Contains all the leaf nodes. This is done to keep the nodes together
				 * in memory and reduce memory allocations.
				 */
				leaf_node_seq_type d_leaf_nodes;

				/**
				 * Contains all the edge sequences referenced by leaf nodes.
				 */
				edge_sequence_list_type d_edge_sequences;
			};

			/**
			 * Builds the inner and outer bounding small circles used for quickly testing
			 * if a point is inside/outside the polygon.
			 */
			class BoundsDataBuilder
			{
			public:
				//! Constructor.
				BoundsDataBuilder(
						const UnitVector3D &outside_point,
						const PolygonOnSphere &polygon);

				/**
				 * The small circle inner/outer bounds of all polygon edges relative
				 * to the outside point.
				 */
				InnerOuterBoundingSmallCircleBuilder d_outside_point_bounds_builder;

				/**
				 * Whether the antipode of the outside point is inside the polygon.
				 */
				bool d_is_outside_point_antipode_in_polygon;
			};

			/**
			 * The inner and outer bounding small circles used for quickly testing
			 * if a point is inside/outside the polygon.
			 */
			class BoundsData
			{
			public:
				//! Constructor.
				explicit
				BoundsData(
						const BoundsDataBuilder &bounds_data_builder);

				/**
				 * The small circle inner/outer bounds of all polygon edges relative
				 * to the outside point.
				 */
				InnerOuterBoundingSmallCircle d_outside_point_bounds;

				/**
				 * Whether the antipode of the outside point is inside the polygon.
				 */
				bool d_is_outside_point_antipode_in_polygon;
			};

			/**
			 * Use to build the spherical lune tree.
			 */
			class TreeBuilder
			{
			public:
				static const unsigned int AVERAGE_NUM_EDGES_PER_LEAF_NODE = 16;

				TreeBuilder(
						const PolygonOnSphere &polygon,
						const UnitVector3D &outside_point);

				void
				build_tree();

				//! The tree data built by us.
				TreeData d_tree_data;

				/**
				 * Builds bounding data for the entire polygon - byproduct of calculating bounds for
				 * the leaf nodes of the tree.
				 */
				BoundsDataBuilder d_bounds_data_builder;

			private:
				const PolygonOnSphere &d_polygon;
				const UnitVector3D d_outside_point;
				const UnitVector3D d_outside_point_antipode;
				unsigned int d_max_tree_depth;

				static const double SPHERICAL_WEDGE_PLANE_EPSILON;


				/**
				 * Builds a subtree (returned node could be internal or leaf).
				 *
				 * @a first_spherical_wedge_bounding_plane has its normal pointing
				 * *outside* the wedge whereas @a second_spherical_wedge_bounding_plane
				 * points *inside* the wedge. This is so the split plane for the wedge
				 * can be found simply by adding the two normals (and normalising).
				 */
				std::pair<node_index_type, bool/*internal node*/>
				build_sub_tree(
						const edge_sequence_list_type &parent_edge_sequences,
						const UnitVector3D &first_spherical_wedge_bounding_plane,
						const UnitVector3D &second_spherical_wedge_bounding_plane,
						const unsigned int current_tree_depth);

				unsigned int
				get_polygon_edges_that_intersect_spherical_lune(
						edge_sequence_list_type &intersecting_edge_sequences,
						const EdgeSequence &edge_sequence,
						const UnitVector3D &first_spherical_wedge_bounding_plane,
						const UnitVector3D &second_spherical_wedge_bounding_plane);
			};


			/**
			 * A reference to ensure the polygon stays alive because we are storing
			 * iterators into its internal structures.
			 *
			 * This is optional because the polygon itself might be caching *us* in
			 * which case we would have circular shared pointers causing a memory leak.
			 */
			boost::optional<PolygonOnSphere::non_null_ptr_to_const_type> d_polygon_shared_pointer;

			/**
			 * The polygon reference to use when iterating over the polygon's points.
			 */
			const PolygonOnSphere &d_polygon;

			/**
			 * The polygon's outside point (see PolygonOnSphere::get_outside_point), the start of
			 * the crossings arc (the end is the test point of the point-in-polygon test).
			 */
			const UnitVector3D d_outside_point;

			/**
			 * Bounds data for the entire polygon - for early rejection/acceptance testing.
			 */
			BoundsData d_bounds_data;

			/**
			 * The data for the spherical lune tree.
			 *
			 * This is optional because in certain scenarios it might not be worth
			 * generating a spherical lune tree (because of the cost to build).
			 * This can happen when only a few points are being tested against a polygon.
			 */
			boost::optional<TreeData> d_tree_data;

			/**
			 * The number of calls to @a is_point_in_polygon that tested polygon edges.
			 *
			 * Like the polygon's cache (that holds us), this is modified by a 'const' method
			 * without synchronisation.
			 */
			mutable unsigned int d_num_queries_testing_edges;


			SphericalLuneTree(
					const PolygonOnSphere::non_null_ptr_to_const_type &polygon,
					bool keep_shared_reference_to_polygon,
					const UnitVector3D &outside_point,
					const BoundsDataBuilder &bounds_data_builder,
					const boost::optional<TreeData> &tree_data = boost::none);

			bool
			is_point_in_polygon(
					const InternalNode &node,
					const UnitVector3D &test_point,
					bool use_point_on_polygon_threshold) const;
		};


		std::unique_ptr<SphericalLuneTree>
		SphericalLuneTree::create(
				const PolygonOnSphere::non_null_ptr_to_const_type &polygon,
				bool build_ologn_hint,
				bool keep_shared_reference_to_polygon)
		{
			const UnitVector3D &outside_point = polygon->get_outside_point();

			// Only build a tree if we've been asked to *and* there are enough polygon edges
			// to make it worthwhile.
			if (!build_ologn_hint ||
				polygon->number_of_segments() <= TreeBuilder::AVERAGE_NUM_EDGES_PER_LEAF_NODE)
			{
				// Don't build a spherical lune tree.
				// But can still benefit from the bounds data.

				BoundsDataBuilder bounds_data_builder(outside_point, *polygon);
				bounds_data_builder.d_outside_point_bounds_builder.add(*polygon);

				return std::unique_ptr<SphericalLuneTree>(
						new SphericalLuneTree(
								polygon,
								keep_shared_reference_to_polygon,
								outside_point,
								bounds_data_builder));
			}

			// Build the spherical lune tree.
			TreeBuilder tree_builder(*polygon, outside_point);
			tree_builder.build_tree();

			// Copy the built tree structures - copying also has the advantage of removing
			// any excess memory usage caused by std::vector.
			// Also copy the polygon bounds data built by the tree builder.
			return std::unique_ptr<SphericalLuneTree>(
					new SphericalLuneTree(
							polygon,
							keep_shared_reference_to_polygon,
							outside_point,
							tree_builder.d_bounds_data_builder,
							tree_builder.d_tree_data));
		}


		SphericalLuneTree::SphericalLuneTree(
				const PolygonOnSphere::non_null_ptr_to_const_type &polygon,
				bool keep_shared_reference_to_polygon,
				const UnitVector3D &outside_point,
				const BoundsDataBuilder &bounds_data_builder,
				const boost::optional<TreeData> &tree_data) :
			d_polygon(*polygon),
			d_outside_point(outside_point),
			d_bounds_data(bounds_data_builder),
			d_tree_data(tree_data),
			d_num_queries_testing_edges(0)
		{
			if (keep_shared_reference_to_polygon)
			{
				d_polygon_shared_pointer = polygon;
			}
		}


		bool
		SphericalLuneTree::is_point_in_polygon(
				const UnitVector3D &test_point,
				bool use_point_on_polygon_threshold) const
		{
			//
			// First determine if we can get an early result to avoid testing the polygon edges.
			//

			// Test the point against the outer/inner small circle bounds.
			const InnerOuterBoundingSmallCircle::Result bounds_result =
					d_bounds_data.d_outside_point_bounds.test(test_point);
			// See if the test point is clearly outside the polygon.
			// Note that this means inside the inner bounds since the centre of the bounding
			// inner/outer small circles is the outside point.
			if (bounds_result == InnerOuterBoundingSmallCircle::INSIDE_INNER_BOUNDS)
			{
				// The test point is outside the polygon.
				return false;
			}
			// See if the test point is closer to the antipode of the outside point than any
			// polygon edge (no edge lies between them, so the test point has its status, inside
			// or outside).
			if (bounds_result == InnerOuterBoundingSmallCircle::OUTSIDE_OUTER_BOUNDS)
			{
				return d_bounds_data.d_is_outside_point_antipode_in_polygon;
			}

			// The bounds did not decide, so the polygon edges (or some of them) will be tested.
			++d_num_queries_testing_edges;

			// If the test point is so close to the outside point, or its antipode, that the
			// crossings arc is degenerate (which, with the bounds tests above, needs an outline
			// that close to it too).
			if (is_crossings_arc_degenerate(d_outside_point, test_point))
			{
				return is_point_in_polygon_with_degenerate_crossings_arc(
						d_polygon,
						test_point,
						use_point_on_polygon_threshold);
			}

			// If it's more efficient to bypass the spherical lune tree then simply
			// test all the edges of the polygon. We still have the advantage of the bounds.
			if (!d_tree_data)
			{
				const UnitVector3D &crossings_arc_start_point = d_outside_point;
				const UnitVector3D &crossings_arc_end_point = test_point;

				return GPlatesMaths::PointInPolygon::is_point_in_polygon(
							d_polygon,
							crossings_arc_start_point,
							crossings_arc_end_point,
							use_point_on_polygon_threshold);
			}

			// Start by recursing into the root node.
			return is_point_in_polygon(
					d_tree_data->d_internal_nodes[d_tree_data->d_root_node_index],
					test_point,
					use_point_on_polygon_threshold);
		}


		bool
		SphericalLuneTree::is_point_in_polygon(
				const InternalNode &node,
				const UnitVector3D &test_point,
				bool use_point_on_polygon_threshold) const
		{
			// Determine which side of the splitting plane the test point is on.
			// The positive side of the plane maps to child index 0.
			// This determines which child node to recurse into.
			const unsigned int child_node_index_offset =
					(dot(node.splitting_plane, test_point).dval() >= 0) ? 0 : 1;

			const unsigned int child_node_index = node.child_node_indices[child_node_index_offset];
			if (node.is_child_node_internal[child_node_index_offset])
			{
				// The child node is an internal node so recurse into it.
				return is_point_in_polygon(
						d_tree_data->d_internal_nodes[child_node_index],
						test_point,
						use_point_on_polygon_threshold);
			}

			// We've reached a leaf node...
			const LeafNode &leaf_node = d_tree_data->d_leaf_nodes[child_node_index];

			// If there are no intersections in the current spherical lune then no edge lies between
			// the test point and the outside point (which can happen for concave polygons - picture
			// a U-shaped polygon like a horse shoe on the surface of the globe and the spherical lune
			// wedge with axis through the outside point and its antipode, and its two half-planes -
			// the U-shaped polygon does not intersect the wedge).
			// Therefore the test point, being in the spherical lune wedge, is also outside the polygon.
			//
			// Note that the leaf node's edges also include every edge within the point-ON-polygon
			// threshold of the test point (so the leaf's edges are enough for the on-outline test,
			// and a leaf with no edges means the test point is not on the outline). This is because
			// an edge is assigned to a spherical lune if it comes within
			// SPHERICAL_WEDGE_PLANE_EPSILON (1e-4) of it, which is much larger than the threshold
			// (about 1.4e-6): if it enters the lune (the lune expanded by that much), or comes that
			// close to either of its bounding half circles (see
			// 'get_polygon_edges_that_intersect_spherical_lune()').
			if (!leaf_node.outside_point_bounds)
			{
				return false;
			}

			// First determine if we can get an early result to avoid testing the polygon edges.
			// Test the point against the outer/inner small circle bounds.
			const InnerOuterBoundingSmallCircle::Result bounds_result =
					leaf_node.outside_point_bounds.get().test(test_point);
			// See if the test point is clearly outside the polygon.
			// Note that this means inside the inner bounds since the centre of the bounding
			// inner/outer small circles is the outside point.
			if (bounds_result == InnerOuterBoundingSmallCircle::INSIDE_INNER_BOUNDS)
			{
				// The test point is outside the polygon.
				return false;
			}
			// See if the test point is closer to the antipode of the outside point than any
			// polygon edge in the current leaf node (no edge lies between them, so the test point
			// has its status, inside or outside).
			if (bounds_result == InnerOuterBoundingSmallCircle::OUTSIDE_OUTER_BOUNDS)
			{
				return d_bounds_data.d_is_outside_point_antipode_in_polygon;
			}

			// Get the iterator range of edge sequences referenced by the leaf node.
			const edge_sequence_list_type::const_iterator edge_sequence_list_begin =
					d_tree_data->d_edge_sequences.begin() + leaf_node.edge_sequences_start_index;
			const edge_sequence_list_type::const_iterator edge_sequence_list_end =
					edge_sequence_list_begin + leaf_node.num_edge_sequences;

			// Test using the subset of polygon edges associated with the leaf node.
			const UnitVector3D &crossings_arc_start_point = d_outside_point;
			const UnitVector3D &crossings_arc_end_point = test_point;
			return GPlatesMaths::PointInPolygon::is_point_in_polygon(
					edge_sequence_list_begin,
					edge_sequence_list_end,
					crossings_arc_start_point,
					crossings_arc_end_point,
					use_point_on_polygon_threshold);
		}


		SphericalLuneTree::BoundsDataBuilder::BoundsDataBuilder(
				const UnitVector3D &outside_point,
				const PolygonOnSphere &polygon) :
			d_outside_point_bounds_builder(outside_point)
		{
			// Test whether the antipode of the outside point is inside the polygon.
			// Note that the crossings-arc is arbitrary (because the arc start and end points are
			// antipodal to each other), but it doesn't matter because the result should be the
			// same regardless of which arc is chosen.
			const UnitVector3D &crossings_arc_start_point = outside_point;
			const UnitVector3D crossings_arc_end_point = -outside_point;
			d_is_outside_point_antipode_in_polygon =
					GPlatesMaths::PointInPolygon::is_point_in_polygon(
							polygon,
							crossings_arc_start_point,
							crossings_arc_end_point,
							false/*use_point_on_polygon_threshold*/);
		}


		SphericalLuneTree::BoundsData::BoundsData(
				const BoundsDataBuilder &bounds_data_builder) :
			d_outside_point_bounds(
					bounds_data_builder.d_outside_point_bounds_builder
							.get_inner_outer_bounding_small_circle(
									get_bounds_widening(),
									get_bounds_widening())),
			d_is_outside_point_antipode_in_polygon(bounds_data_builder.d_is_outside_point_antipode_in_polygon)
		{  }


		// '1e-4' represents an angular deviation of 0.34 minutes away from the plane.
		// The spherical wedge is expanded slightly to include polygon edges that are
		// near the wedge (to protect against numerical precision issues).
		const double SphericalLuneTree::TreeBuilder::SPHERICAL_WEDGE_PLANE_EPSILON = 1e-4;

		SphericalLuneTree::TreeBuilder::TreeBuilder(
				const PolygonOnSphere &polygon,
				const UnitVector3D &outside_point) :
			d_bounds_data_builder(outside_point, polygon),
			d_polygon(polygon),
			d_outside_point(outside_point),
			d_outside_point_antipode(-outside_point)
		{
			const unsigned int num_polygon_edges = polygon.number_of_segments();
			const double num_leaf_nodes = double(num_polygon_edges) / AVERAGE_NUM_EDGES_PER_LEAF_NODE;
			// The tree depth is less log2(num_leaf_nodes) rounded up.
			// For example if 'num_leaf_nodes' is 65 then 'max_tree_depth' is 7 which supports
			// 128 leaf nodes but since leaf nodes cannot have more than
			// 'AVERAGE_NUM_EDGES_PER_LEAF_NODE' edges the tree should not be full (of nodes).
			d_max_tree_depth = static_cast<unsigned int>(
					std::log(num_leaf_nodes) / std::log(2.0) + 1 - 1e-6);
		}


		void
		SphericalLuneTree::TreeBuilder::build_tree()
		{
			// Create the root node and its two child nodes.
			// The four child nodes (of the two child nodes of the root node) represent
			// four equal area spherical lunes that partition the entire globe surface.
			// The axis of all spherical lunes is through the outside point and its antipode.

			// The splitting plane of the root node can be any plane that passes through the
			// antipode of the outside point (and hence also the outside point).
			const UnitVector3D root_node_splitting_plane =
					generate_perpendicular(d_outside_point_antipode);

			const UnitVector3D child_nodes_splitting_plane(
					cross(d_outside_point_antipode, root_node_splitting_plane));

			// All polygon edges.
			edge_sequence_list_type all_polygon_edges;

			// Add polygon edges in exterior ring.
			all_polygon_edges.push_back(
					EdgeSequence(d_polygon.exterior_ring_begin(), d_polygon.exterior_ring_end()));

			// Add polygon edges in interior rings.
			for (unsigned int interior_ring_index = 0;
				interior_ring_index < d_polygon.number_of_interior_rings();
				++interior_ring_index)
			{
				all_polygon_edges.push_back(
						EdgeSequence(
								d_polygon.interior_ring_begin(interior_ring_index),
								d_polygon.interior_ring_end(interior_ring_index)));
			}

			// Create the four grandchildren nodes that represents the four equal area
			// spherical lunes partitioning the entire sphere.
			const std::pair<node_index_type, bool> grand_child_nodes[2][2] =
			{
				{
					build_sub_tree(
							all_polygon_edges,
							-root_node_splitting_plane, child_nodes_splitting_plane,
							2/*current_tree_depth*/),
					build_sub_tree(
							all_polygon_edges,
							child_nodes_splitting_plane, root_node_splitting_plane,
							2/*current_tree_depth*/)
				},
				{
					build_sub_tree(
							all_polygon_edges,
							root_node_splitting_plane, -child_nodes_splitting_plane,
							2/*current_tree_depth*/),
					build_sub_tree(
							all_polygon_edges,
							-child_nodes_splitting_plane, -root_node_splitting_plane,
							2/*current_tree_depth*/)
				}
			};

			// Create the first child node of the root node.
			const InternalNode first_child_of_root_node(
					child_nodes_splitting_plane,
					grand_child_nodes[0][0].first,
					grand_child_nodes[0][1].first,
					grand_child_nodes[0][0].second,
					grand_child_nodes[0][1].second);
			const node_index_type first_child_of_root_node_index =
					d_tree_data.d_internal_nodes.size();
			d_tree_data.d_internal_nodes.push_back(first_child_of_root_node);

			// Create the second child node of the root node.
			const InternalNode second_child_of_root_node(
					-child_nodes_splitting_plane,
					grand_child_nodes[1][0].first,
					grand_child_nodes[1][1].first,
					grand_child_nodes[1][0].second,
					grand_child_nodes[1][1].second);
			const node_index_type second_child_of_root_node_index =
					d_tree_data.d_internal_nodes.size();
			d_tree_data.d_internal_nodes.push_back(second_child_of_root_node);

			// Create the root node.
			const InternalNode root_node(
					root_node_splitting_plane,
					first_child_of_root_node_index,
					second_child_of_root_node_index,
					true/*is_first_child_internal*/,
					true/*is_second_child_internal*/);
			d_tree_data.d_root_node_index = d_tree_data.d_internal_nodes.size();
			d_tree_data.d_internal_nodes.push_back(root_node);
		}


		std::pair<SphericalLuneTree::node_index_type, bool/*internal node*/>
		SphericalLuneTree::TreeBuilder::build_sub_tree(
				const edge_sequence_list_type &parent_edge_sequences,
				const UnitVector3D &first_spherical_wedge_bounding_plane,
				const UnitVector3D &second_spherical_wedge_bounding_plane,
				const unsigned int current_tree_depth)
		{
			// The edges that intersect the current spherical lune.
			edge_sequence_list_type sub_tree_edge_sequences;
			unsigned int num_intersecting_edges = 0;

			// First find the edges bounded by the current spherical lune (bounded by
			// two spherical wedge semi-planes).
			// We can limit the polygon edges tested to only those bounded by our parent node
			// because the parent node also bounds this child node.
			BOOST_FOREACH(const EdgeSequence &parent_edge_sequence, parent_edge_sequences)
			{
				num_intersecting_edges += get_polygon_edges_that_intersect_spherical_lune(
						sub_tree_edge_sequences,
						parent_edge_sequence,
						first_spherical_wedge_bounding_plane,
						second_spherical_wedge_bounding_plane);
			}

			// Create an internal node unless:
			// 1) the current tree depth exceeds a threshold, or
			// 2) the number of edges intersecting the spherical lune exceeds a threshold.
			if (current_tree_depth < d_max_tree_depth &&
				num_intersecting_edges > AVERAGE_NUM_EDGES_PER_LEAF_NODE)
			{
				const UnitVector3D splitting_plane = (
						Vector3D(first_spherical_wedge_bounding_plane) +
						Vector3D(second_spherical_wedge_bounding_plane)
					).get_normalisation();

				const std::pair<node_index_type, bool> first_child_sub_tree =
						build_sub_tree(
								sub_tree_edge_sequences,
								first_spherical_wedge_bounding_plane,
								splitting_plane,
								current_tree_depth + 1);

				const std::pair<node_index_type, bool> second_child_sub_tree =
						build_sub_tree(
								sub_tree_edge_sequences,
								splitting_plane,
								second_spherical_wedge_bounding_plane,
								current_tree_depth + 1);

				// Create an internal node.
				const InternalNode sub_tree_node(
						splitting_plane,
						first_child_sub_tree.first,
						second_child_sub_tree.first,
						first_child_sub_tree.second,
						second_child_sub_tree.second);
				const node_index_type sub_tree_node_index =
						d_tree_data.d_internal_nodes.size();
				d_tree_data.d_internal_nodes.push_back(sub_tree_node);

				// Return internal node.
				return std::make_pair(sub_tree_node_index, true/*internal node*/);
			}
			// If we get here we have reached a leaf node...

			// Get the range of dot products (from the outside point) that the
			// intersecting polygon edges lie within.
			// This will be used for early inside/outside point-in-polygon tests.

			// Determine the bounds for the current subtree (which is a leaf).
			InnerOuterBoundingSmallCircleBuilder leaf_node_bounds_data_builder(
					d_outside_point);

			// If there are no intersections in the current spherical lune then no edge lies between
			// a point in it and the outside point (which can happen for concave polygons - picture a
			// U-shaped polygon like a horse shoe on the surface of the globe and the spherical lune
			// wedge with axis through the outside point and its antipode, and its two half-planes -
			// the U-shaped polygon does not intersect the wedge).
			if (sub_tree_edge_sequences.empty())
			{
				// Just store a default-constructed LeafNode that contains no sequences of edges
				// and no inner-outer bounding small circles.
				const node_index_type leaf_node_index = d_tree_data.d_leaf_nodes.size();
				d_tree_data.d_leaf_nodes.push_back(LeafNode());

				// Return leaf node.
				return std::make_pair(leaf_node_index, false/*internal node*/);
			}

			// Iterate over the edge sequences and build the bounds.
			BOOST_FOREACH(const EdgeSequence &sub_tree_edge_sequence, sub_tree_edge_sequences)
			{
				// Expand/contract the outer/inner bounds as needed to include the
				// current edge sequence.
				leaf_node_bounds_data_builder.add(
						sub_tree_edge_sequence.begin, sub_tree_edge_sequence.end);
			}

			// Get the leaf node inner/outer bounding small circle.
			const InnerOuterBoundingSmallCircle leaf_node_inner_outer_bounding_small_circle =
					leaf_node_bounds_data_builder.get_inner_outer_bounding_small_circle(
							get_bounds_widening(), get_bounds_widening());

			// Expand/contract the outer/inner bounds of the entire spherical lune tree
			// to include the current leaf node bounds.
			d_bounds_data_builder.d_outside_point_bounds_builder.add(
					leaf_node_inner_outer_bounding_small_circle);

			// Create a leaf node - it references edges that will be stored in the global list.
			const LeafNode leaf_node(
					d_tree_data.d_edge_sequences.size(),
					sub_tree_edge_sequences.size(),
					leaf_node_inner_outer_bounding_small_circle);

			// Insert the edges that intersect the current spherical lune into the global list.
			d_tree_data.d_edge_sequences.insert(d_tree_data.d_edge_sequences.end(),
					sub_tree_edge_sequences.begin(), sub_tree_edge_sequences.end());
			const node_index_type leaf_node_index = d_tree_data.d_leaf_nodes.size();
			d_tree_data.d_leaf_nodes.push_back(leaf_node);

			// Return leaf node.
			return std::make_pair(leaf_node_index, false/*internal node*/);
		}


		unsigned int
		SphericalLuneTree::TreeBuilder::get_polygon_edges_that_intersect_spherical_lune(
				edge_sequence_list_type &intersecting_edge_sequences,
				const EdgeSequence &edge_sequence,
				const UnitVector3D &first_spherical_wedge_bounding_plane,
				const UnitVector3D &second_spherical_wedge_bounding_plane)
		{
			// The lune's two bounding half circles (from the antipode of the outside point to the
			// outside point), each as two quarter circles (a great circle arc must be shorter than a
			// half circle).
			//
			// An edge that does not enter the lune can still come within the point-ON-polygon
			// threshold of a point in it, past one of these half circles: near either of the lune's
			// apexes it can pass round the apex (crossing a bounding great circle on its
			// other half), and elsewhere it can run nearly parallel to a half circle with both end
			// points outside the widened lune. So an edge that comes within the lune's widening of
			// either half circle is included too, which makes a leaf's edges enough for the
			// on-outline test. The lune is on the negative side of the first plane and the positive
			// side of the second.
			const UnitVector3D *const half_circle_planes[2] = {
				&first_spherical_wedge_bounding_plane,
				&second_spherical_wedge_bounding_plane
			};
			// Each half circle's two quarter circles, created only if an edge needs them.
			typedef std::pair<GreatCircleArc, GreatCircleArc> half_circle_type;
			boost::optional<half_circle_type> half_circles[2];
			const auto get_half_circle = [&](
					unsigned int half_circle_index) -> const half_circle_type &
			{
				if (!half_circles[half_circle_index])
				{
					const UnitVector3D &half_circle_plane = *half_circle_planes[half_circle_index];
					const UnitVector3D &other_plane = *half_circle_planes[1 - half_circle_index];
					// The half circle's midpoint, 90 degrees from the apexes, is on the lune's
					// side of the other plane (the positive side for the first half circle,
					// negative for the second).
					const UnitVector3D midpoint =
							cross(half_circle_plane, d_outside_point_antipode).get_normalisation();
					const PointOnSphere half_circle_midpoint(
							((dot(other_plane, midpoint).dval() >= 0) == (half_circle_index == 0))
									? midpoint
									: -midpoint);
					const PointOnSphere outside_point_antipode(d_outside_point_antipode);
					const PointOnSphere outside_point(-d_outside_point_antipode);
					half_circles[half_circle_index] = std::make_pair(
							GreatCircleArc::create(outside_point_antipode, half_circle_midpoint),
							GreatCircleArc::create(half_circle_midpoint, outside_point));
				}

				return half_circles[half_circle_index].get();
			};
			static const AngularExtent LUNE_WIDENING =
					AngularExtent::create_from_angle(std::asin(SPHERICAL_WEDGE_PLANE_EPSILON));
			const auto is_edge_near_half_circle = [&](
					const GreatCircleArc &edge,
					unsigned int half_circle_index)
			{
				// An arc (shorter than a half circle) that does not cross a great circle is nowhere
				// closer to it than at its nearer end point (the sine of the distance along the arc
				// is a sine curve), so if both end points are further than the widening, on one
				// side, then so is the whole arc. That rejects most edges with two dot products.
				const UnitVector3D &half_circle_plane = *half_circle_planes[half_circle_index];
				const double start_dot =
						dot(half_circle_plane, edge.start_point().position_vector()).dval();
				const double end_dot =
						dot(half_circle_plane, edge.end_point().position_vector()).dval();
				if ((start_dot > SPHERICAL_WEDGE_PLANE_EPSILON &&
						end_dot > SPHERICAL_WEDGE_PLANE_EPSILON) ||
					(start_dot < -SPHERICAL_WEDGE_PLANE_EPSILON &&
						end_dot < -SPHERICAL_WEDGE_PLANE_EPSILON))
				{
					return false;
				}

				const half_circle_type &half_circle = get_half_circle(half_circle_index);
				return minimum_distance(edge, half_circle.first, LUNE_WIDENING) ||
						minimum_distance(edge, half_circle.second, LUNE_WIDENING);
			};

			unsigned int num_intersecting_edges = 0;

			// Keep track of the current sequence of edges that intersect the spherical lune.
			PolygonOnSphere::ring_const_iterator current_intersecting_edge_sequence_start =
					edge_sequence.begin;

			// Iterate over the polygon edges to be tested for intersection.
			PolygonOnSphere::ring_const_iterator polygon_edge_iter = edge_sequence.begin;
			for ( ; polygon_edge_iter != edge_sequence.end; ++polygon_edge_iter)
			{
				const GreatCircleArc &edge = *polygon_edge_iter;

				if (edge.is_zero_length())
				{
					// Since the edge is zero length or very close to it (within epsilon)
					// we can simply test if one of the endpoints is inside the spherical wedge.
					// We can do this because our spherical wedge inclusion test also uses
					// an epsilon (and it is quite bigger than the zero edge length epsilon).
					const UnitVector3D &edge_start_point = edge.start_point().position_vector();
					if (dot(first_spherical_wedge_bounding_plane, edge_start_point).dval()
							< SPHERICAL_WEDGE_PLANE_EPSILON &&
						dot(second_spherical_wedge_bounding_plane, edge_start_point).dval()
							> -SPHERICAL_WEDGE_PLANE_EPSILON)
					{
						// Polygon edge intersects spherical lune so continue to next edge.
						++num_intersecting_edges;
						continue;
					}
				}
				else // Polygon edge is not zero length and hence has a rotation axis...
				{
					const UnitVector3D &edge_start_point = edge.start_point().position_vector();
					const UnitVector3D &edge_end_point = edge.end_point().position_vector();

					// If either polygon edge arc endpoint is inside the spherical wedge then
					// the polygon edge must intersect the spherical lune.
					// The spherical wedge is expanded slightly to include polygon edges that are
					// near the wedge (to protect against numerical precision issues).
					if (dot(first_spherical_wedge_bounding_plane, edge_start_point).dval()
							< SPHERICAL_WEDGE_PLANE_EPSILON &&
						dot(second_spherical_wedge_bounding_plane, edge_start_point).dval()
							> -SPHERICAL_WEDGE_PLANE_EPSILON)
					{
						// Polygon edge intersects spherical lune so continue to next edge.
						++num_intersecting_edges;
						continue;
					}

					if (dot(first_spherical_wedge_bounding_plane, edge_end_point).dval()
							< SPHERICAL_WEDGE_PLANE_EPSILON &&
						dot(second_spherical_wedge_bounding_plane, edge_end_point).dval()
							> -SPHERICAL_WEDGE_PLANE_EPSILON)
					{
						// Polygon edge intersects spherical lune so continue to next edge.
						++num_intersecting_edges;
						continue;
					}

					const UnitVector3D &edge_plane_normal = edge.rotation_axis();

					// Test for the case where neither polygon edge arc endpoint is in the
					// spherical lune but the arc still crosses the spherical lune.
					// Since the spherical lune is bounded by the two half-circles we only need to test
					// if the polygon edge arc intersects *one* of the half-circles because if it
					// crosses one half-circle then it must also cross the other half-circle
					// (unless it passes through the lune's apex, which the test below catches).
					// We arbitrarily choose the first spherical wedge half-circle to test against.

					// If the polygon edge arc intersects a half-circle:
					// 1) the plane of the half-circle arc must separate the polygon edge arc endpoints, *and*
					// 2) the start points of each arc are on different sides of
					//    the other arc's plane (eg, if the polygon edge start point
					//    is on the *negative* side of the half-circle plane then
					//    the half-circle-arc start point must be on the *positive* side
					//    of the polygon edge plane).
					// Condition (2) is required so we don't return an intersection
					// when the arcs are on the opposite sides of the globe (even
					// though the plane of each arc divides the other arc's endpoints).
					// Note that for (2) since one of the arcs is a half-circle its endpoints
					// will always be on opposite sides of any plane (of any orientation) simply
					// because its endpoints are antipodal to each other so we only need to test
					// that the polygon edge endpoints are on opposite sides of the spherical
					// wedge plane and not vice versa.
					if (dot(first_spherical_wedge_bounding_plane, edge_start_point).dval() <= 0)
					{
						// The spherical wedge start point must be on the *positive* side of
						// the polygon edge plane since the polygon edge start point is on the
						// *negative* side of the spherical wedge plane.
						// NOTE: the spherical wedge start point is the antipode of the outside point.
						if (dot(first_spherical_wedge_bounding_plane, edge_end_point).dval() >= 0 &&
							dot(edge_plane_normal, d_outside_point_antipode).dval() >= 0)
						{
							// Polygon edge intersects spherical lune so continue to next edge.
							++num_intersecting_edges;
							continue;
						}
					}
					else // edge start point is on positive side of first spherical wedge plane...
					{
						// The spherical wedge start point must be on the *negative* side of
						// the polygon edge plane since the polygon edge start point is on the
						// *positive* side of the spherical wedge plane.
						// NOTE: the spherical wedge start point is the antipode of the outside point.
						if (dot(first_spherical_wedge_bounding_plane, edge_end_point).dval() <= 0 &&
							dot(edge_plane_normal, d_outside_point_antipode).dval() <= 0)
						{
							// Polygon edge intersects spherical lune so continue to next edge.
							++num_intersecting_edges;
							continue;
						}
					}
				}

				// The polygon edge does not enter the spherical lune, but include it if it comes
				// within the lune's widening of either bounding half circle (see above).
				if (is_edge_near_half_circle(edge, 0) ||
					is_edge_near_half_circle(edge, 1))
				{
					++num_intersecting_edges;
					continue;
				}

				// The current polygon edge does not intersect the spherical lune so
				// output the current sequence of intersecting edges if any.
				if (polygon_edge_iter != current_intersecting_edge_sequence_start)
				{
					intersecting_edge_sequences.push_back(
							EdgeSequence(
									current_intersecting_edge_sequence_start, polygon_edge_iter));
				}

				current_intersecting_edge_sequence_start = polygon_edge_iter;
				++current_intersecting_edge_sequence_start;
			}

			// Output the last sequence of intersecting edges if it hasn't already been.
			if (polygon_edge_iter != current_intersecting_edge_sequence_start)
			{
				intersecting_edge_sequences.push_back(
						EdgeSequence(
								current_intersecting_edge_sequence_start, polygon_edge_iter));
			}

			return num_intersecting_edges;
		}
	}
}


namespace GPlatesMaths
{
	namespace PointInPolygon
	{
		namespace
		{
			/**
			 * A candidate for the outside point should be at least this far from the outline (as a
			 * cosine; 0.01 radians), for two reasons:
			 *
			 * - Its status is decided from fan sums of triangles with apex at its antipode, which are
			 *   ill-conditioned for a candidate at a vertex.
			 * - The filled polygons are drawn as fans from that antipode, rendered with a perspective
			 *   projection from the centre of the globe whose near plane is 1e-3 from it
			 *   (GLCubeSubdivision's default 'zNear'). A fan triangle whose edge passes within a
			 *   distance d of the outside point has its plane within about d of the centre, so it
			 *   would be clipped by the near plane if d were less than about 1e-3.
			 *
			 * Only if no candidate that far away is outside does a closer one serve.
			 *
			 * (The second reference point needs no minimum: its status comes from exact crossing
			 * parity, and nothing is drawn from it, so it is simply the candidate furthest from the
			 * outline.)
			 */
			const double MAXIMUM_CANDIDATE_DISTANCE_TO_OUTLINE_COSINE = std::cos(1e-2);

			/**
			 * The closest a candidate for the outside point may ever be to the outline (as a cosine;
			 * 1e-5 radians), when no candidate further than the above is outside.
			 *
			 * Well above the 1e-6 radians within which a candidate takes the boundary centroid's
			 * status (see 'is_candidate_in_polygon_ring'), so no edge can pass between them.
			 */
			const double MAXIMUM_CANDIDATE_DISTANCE_TO_OUTLINE_COSINE_AT_ALL = std::cos(1e-5);


			/**
			 * The candidates for the outside point and the second reference point, besides the
			 * boundary centroid and its antipode: the 12 vertices of an icosahedron and the 20
			 * centres of its faces, so no point on the sphere is more than about 23 degrees from one.
			 */
			const std::vector<UnitVector3D> &
			get_fixed_candidate_directions()
			{
				static const std::vector<UnitVector3D> DIRECTIONS = []()
				{
					const double golden_ratio = (1 + std::sqrt(5.0)) / 2;

					std::vector<UnitVector3D> icosahedron_vertices;
					for (const double first_sign : { -1.0, 1.0 })
					{
						for (const double second_sign : { -1.0, 1.0 })
						{
							icosahedron_vertices.push_back(
									Vector3D(0, first_sign, second_sign * golden_ratio).get_normalisation());
							icosahedron_vertices.push_back(
									Vector3D(first_sign, second_sign * golden_ratio, 0).get_normalisation());
							icosahedron_vertices.push_back(
									Vector3D(second_sign * golden_ratio, 0, first_sign).get_normalisation());
						}
					}

					std::vector<UnitVector3D> directions(icosahedron_vertices);

					// A face is three mutually adjacent vertices (whose dot products are 1/sqrt(5)).
					const double adjacent_vertices_dot = 1 / std::sqrt(5.0);
					const auto are_adjacent = [&](
							const UnitVector3D &vertex1,
							const UnitVector3D &vertex2)
					{
						return std::fabs(dot(vertex1, vertex2).dval() - adjacent_vertices_dot) < 1e-6;
					};
					const unsigned int num_vertices = icosahedron_vertices.size();
					for (unsigned int i = 0; i < num_vertices; ++i)
					{
						for (unsigned int j = i + 1; j < num_vertices; ++j)
						{
							for (unsigned int k = j + 1; k < num_vertices; ++k)
							{
								if (are_adjacent(icosahedron_vertices[i], icosahedron_vertices[j]) &&
									are_adjacent(icosahedron_vertices[j], icosahedron_vertices[k]) &&
									are_adjacent(icosahedron_vertices[i], icosahedron_vertices[k]))
								{
									directions.push_back(
											(Vector3D(icosahedron_vertices[i]) +
												Vector3D(icosahedron_vertices[j]) +
												Vector3D(icosahedron_vertices[k])).get_normalisation());
								}
							}
						}
					}

					return directions;
				}();

				return DIRECTIONS;
			}


			/**
			 * A candidate point with the cosine of its distance to the polygon's outline.
			 */
			struct Candidate
			{
				Candidate(
						const UnitVector3D &point_,
						const double &distance_to_outline_cosine_) :
					point(point_),
					distance_to_outline_cosine(distance_to_outline_cosine_)
				{  }

				UnitVector3D point;
				double distance_to_outline_cosine;
			};


			/**
			 * Returns the candidates (the boundary centroid's antipode, the centroid, then the fixed
			 * directions), each with its distance to the outline.
			 */
			std::vector<Candidate>
			get_candidates(
					const PolygonOnSphere &polygon)
			{
				const UnitVector3D &boundary_centroid = polygon.get_boundary_centroid();

				std::vector<UnitVector3D> candidate_points;
				candidate_points.push_back(-boundary_centroid);
				candidate_points.push_back(boundary_centroid);
				const std::vector<UnitVector3D> &fixed_directions = get_fixed_candidate_directions();
				candidate_points.insert(candidate_points.end(), fixed_directions.begin(), fixed_directions.end());

				std::vector<Candidate> candidates;
				for (const UnitVector3D &candidate_point : candidate_points)
				{
					// The distance to the outline (not solid), using the polygon's bounding tree.
					const AngularDistance distance_to_outline =
							minimum_distance(PointOnSphere(candidate_point), polygon).get();
					candidates.push_back(Candidate(candidate_point, distance_to_outline.get_cosine().dval()));
				}

				return candidates;
			}


			/**
			 * Returns true if every vertex of a ring is in the open hemisphere about @a centre.
			 *
			 * The ring's edges are then in it too (each is shorter than a half circle).
			 */
			bool
			is_ring_in_open_hemisphere(
					const PolygonOnSphere::ring_vertex_const_iterator &ring_vertex_begin,
					const PolygonOnSphere::ring_vertex_const_iterator &ring_vertex_end,
					const UnitVector3D &centre)
			{
				for (PolygonOnSphere::ring_vertex_const_iterator ring_vertex_iter = ring_vertex_begin;
					ring_vertex_iter != ring_vertex_end;
					++ring_vertex_iter)
				{
					if (dot(ring_vertex_iter->position_vector(), centre).dval() <= 0)
					{
						return false;
					}
				}

				return true;
			}


			/**
			 * Returns true if @a candidate_point is in the interior of a ring (see InteriorReference).
			 *
			 * @a candidate_point must not be on, or very near, the ring's outline.
			 */
			bool
			is_candidate_in_polygon_ring(
					const PolygonOnSphere::ring_const_iterator &ring_begin,
					const PolygonOnSphere::ring_const_iterator &ring_end,
					bool is_ring_in_boundary_centroid_hemisphere,
					const UnitVector3D &boundary_centroid,
					const UnitVector3D &candidate_point)
			{
				if (is_ring_in_boundary_centroid_hemisphere)
				{
					// The ring is within the open hemisphere about the centroid C, and its interior is
					// parity from -C. A candidate not in that hemisphere is outside it.
					if (dot(candidate_point, boundary_centroid).dval() <= 0)
					{
						return false;
					}

					// A candidate within about 1e-6 radians of C has the status of C, whose crossings arc
					// from -C is exactly antipodal (so its arbitrary plane contains -C as well). Whether C
					// is inside doesn't matter: the candidate is at least 1e-5 radians from the outline
					// (see MAXIMUM_CANDIDATE_DISTANCE_TO_OUTLINE_COSINE_AT_ALL), so no edge passes between
					// them.
					const UnitVector3D crossings_arc_start_point = -boundary_centroid;
					const UnitVector3D &crossings_arc_end_point =
							is_crossings_arc_degenerate(crossings_arc_start_point, candidate_point)
							? boundary_centroid
							: candidate_point;

					return (get_num_polygon_edges_crossed(
								ring_begin,
								ring_end,
								crossings_arc_start_point,
								crossings_arc_end_point,
								get_crossings_arc_plane_normal(crossings_arc_start_point, crossings_arc_end_point),
								dot(crossings_arc_start_point, crossings_arc_end_point).dval()) & 1) == 1;
				}

				// The fan sum from the candidate's antipode is the integral of the ring's winding number
				// less its winding number at the candidate.
				const real_t ring_fan_signed_area = SphericalArea::calculate_polygon_ring_fan_signed_area(
						PointOnSphere(-candidate_point),
						ring_begin,
						ring_end);

				return (get_ring_winding_offset(ring_fan_signed_area.dval()) & 1) != 0;
			}


			/**
			 * Returns true if @a candidate_point is in the interior of @a polygon (an odd number of
			 * its rings' interiors).
			 */
			bool
			is_candidate_in_polygon(
					const PolygonOnSphere &polygon,
					const std::vector<bool> &is_ring_in_boundary_centroid_hemisphere,
					const UnitVector3D &candidate_point)
			{
				const UnitVector3D &boundary_centroid = polygon.get_boundary_centroid();

				bool is_in_polygon = is_candidate_in_polygon_ring(
						polygon.exterior_ring_begin(),
						polygon.exterior_ring_end(),
						is_ring_in_boundary_centroid_hemisphere[0],
						boundary_centroid,
						candidate_point);

				for (unsigned int interior_ring_index = 0;
					interior_ring_index < polygon.number_of_interior_rings();
					++interior_ring_index)
				{
					if (is_candidate_in_polygon_ring(
							polygon.interior_ring_begin(interior_ring_index),
							polygon.interior_ring_end(interior_ring_index),
							is_ring_in_boundary_centroid_hemisphere[interior_ring_index + 1],
							boundary_centroid,
							candidate_point))
					{
						is_in_polygon = !is_in_polygon;
					}
				}

				return is_in_polygon;
			}
		}
	}
}


GPlatesMaths::PointInPolygon::InteriorReference
GPlatesMaths::PointInPolygon::calculate_interior_reference(
		const PolygonOnSphere &polygon)
{
	const UnitVector3D &boundary_centroid = polygon.get_boundary_centroid();

	// Which rings are within the open hemisphere about the boundary centroid C.
	std::vector<bool> is_ring_in_boundary_centroid_hemisphere;
	is_ring_in_boundary_centroid_hemisphere.push_back(
			is_ring_in_open_hemisphere(
					polygon.exterior_ring_vertex_begin(),
					polygon.exterior_ring_vertex_end(),
					boundary_centroid));
	for (unsigned int interior_ring_index = 0;
		interior_ring_index < polygon.number_of_interior_rings();
		++interior_ring_index)
	{
		is_ring_in_boundary_centroid_hemisphere.push_back(
				is_ring_in_open_hemisphere(
						polygon.interior_ring_vertex_begin(interior_ring_index),
						polygon.interior_ring_vertex_end(interior_ring_index),
						boundary_centroid));
	}

	// If every ring is in that hemisphere then -C is outside every ring (and more than 90 degrees
	// from the outline), as it always was. Almost every polygon is like this, and everything about it
	// is calculated as it was before the interior rule.
	if (std::find(
			is_ring_in_boundary_centroid_hemisphere.begin(),
			is_ring_in_boundary_centroid_hemisphere.end(),
			false) == is_ring_in_boundary_centroid_hemisphere.end())
	{
		return InteriorReference(-boundary_centroid, std::vector<bool>());
	}

	// Otherwise the outside point is the candidate furthest from the outline that is outside.
	std::vector<Candidate> candidates = get_candidates(polygon);
	std::stable_sort(
			candidates.begin(),
			candidates.end(),
			[](const Candidate &lhs, const Candidate &rhs)
			{
				return lhs.distance_to_outline_cosine < rhs.distance_to_outline_cosine;
			});
	//
	// If no candidate far enough from the outline is outside, the furthest that is outside, almost
	// regardless of distance (its fan sums are less accurate, but rounding them to the nearest
	// multiple of 4 pi tolerates large errors).
	for (const Candidate &candidate : candidates)
	{
		if (candidate.distance_to_outline_cosine > MAXIMUM_CANDIDATE_DISTANCE_TO_OUTLINE_COSINE)
		{
			// This candidate, and the rest, are too close to the outline.
			break;
		}

		if (!is_candidate_in_polygon(polygon, is_ring_in_boundary_centroid_hemisphere, candidate.point))
		{
			return InteriorReference(candidate.point, is_ring_in_boundary_centroid_hemisphere);
		}
	}
	//
	// The second pass skips the candidates the first pass found inside. A candidate it takes is
	// closer to the outline than the near plane allows (see
	// MAXIMUM_CANDIDATE_DISTANCE_TO_OUTLINE_COSINE), so the filled polygon may be clipped near it,
	// but point-in-polygon (crossing parity) is still right. That is better than the fallback
	// below, where -C may be inside.
	for (const Candidate &candidate : candidates)
	{
		if (candidate.distance_to_outline_cosine > MAXIMUM_CANDIDATE_DISTANCE_TO_OUTLINE_COSINE &&
			candidate.distance_to_outline_cosine <= MAXIMUM_CANDIDATE_DISTANCE_TO_OUTLINE_COSINE_AT_ALL &&
			!is_candidate_in_polygon(polygon, is_ring_in_boundary_centroid_hemisphere, candidate.point))
		{
			return InteriorReference(candidate.point, is_ring_in_boundary_centroid_hemisphere);
		}
	}

	// No candidate is outside (and at least 1e-5 radians off the outline). That needs a polygon
	// whose interior covers almost the whole sphere (rings that tile it, say). Keep -C, as before
	// the interior rule, which may then be inside, so such a polygon's point-in-polygon answers (and
	// fills) are inverted.
	return InteriorReference(-boundary_centroid, is_ring_in_boundary_centroid_hemisphere);
}


GPlatesMaths::PointInPolygon::SecondReferencePoint
GPlatesMaths::PointInPolygon::calculate_second_reference_point(
		const PolygonOnSphere &polygon)
{
	const UnitVector3D &outside_point = polygon.get_outside_point();

	// The candidate between 60 and 120 degrees from the outside point (there is always one, since no
	// point is more than about 23 degrees from a candidate) that is furthest from the outline.
	const std::vector<Candidate> candidates = get_candidates(polygon);
	const Candidate *second_reference_candidate = nullptr;
	for (const Candidate &candidate : candidates)
	{
		if (std::fabs(dot(candidate.point, outside_point).dval()) > 0.5)
		{
			continue;
		}

		if (!second_reference_candidate ||
			candidate.distance_to_outline_cosine < second_reference_candidate->distance_to_outline_cosine)
		{
			second_reference_candidate = &candidate;
		}
	}

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			second_reference_candidate != nullptr,
			GPLATES_ASSERTION_SOURCE);

	const UnitVector3D &second_reference_point = second_reference_candidate->point;

	// Its status is the crossing parity from the outside point (an arc of 60 to 120 degrees, so well
	// defined).
	const bool is_second_reference_point_in_polygon = is_point_in_polygon(
			polygon,
			outside_point,
			second_reference_point,
			false/*use_point_on_polygon_threshold*/);

	return SecondReferencePoint(second_reference_point, is_second_reference_point_in_polygon);
}


int
GPlatesMaths::PointInPolygon::get_ring_winding_offset(
		const double &ring_fan_signed_area)
{
	// A ring with an area within this of half the sphere is a tie, and its interior is its left side.
	// That is rounding half down, with this tolerance.
	static const double TIE_STERADIANS = 1e-8;

	return static_cast<int>(std::ceil((ring_fan_signed_area - TIE_STERADIANS) / (4 * PI) - 0.5));
}

bool
GPlatesMaths::PointInPolygon::is_point_in_polygon(
		const PointOnSphere &point,
		const PolygonOnSphere &polygon,
		bool use_point_on_polygon_threshold)
{
	const UnitVector3D &crossings_arc_start_point = polygon.get_outside_point();

	const UnitVector3D &crossings_arc_end_point = point.position_vector();

	if (is_crossings_arc_degenerate(crossings_arc_start_point, crossings_arc_end_point))
	{
		return is_point_in_polygon_with_degenerate_crossings_arc(
				polygon,
				crossings_arc_end_point,
				use_point_on_polygon_threshold);
	}

	return is_point_in_polygon(
			polygon,
			crossings_arc_start_point,
			crossings_arc_end_point,
			use_point_on_polygon_threshold);
}


GPlatesMaths::PointInPolygon::Polygon::Polygon(
		const GPlatesGlobal::PointerTraits<const PolygonOnSphere>::non_null_ptr_type &polygon,
		bool build_ologn_hint,
		bool keep_shared_reference_to_polygon) :
	d_spherical_lune_tree(
			SphericalLuneTree::create(
					polygon, build_ologn_hint, keep_shared_reference_to_polygon).release())
{
}


bool
GPlatesMaths::PointInPolygon::Polygon::is_point_in_polygon(
		const PointOnSphere &test_point,
		bool use_point_on_polygon_threshold) const
{
	// Query the O(log(N)) spherical lune tree.
	return d_spherical_lune_tree->is_point_in_polygon(
			test_point.position_vector(),
			use_point_on_polygon_threshold);
}


unsigned int
GPlatesMaths::PointInPolygon::Polygon::get_num_queries_testing_edges() const
{
	return d_spherical_lune_tree->get_num_queries_testing_edges();
}


double
GPlatesMaths::PointInPolygon::get_point_on_polygon_threshold_cosine()
{
	// A test point can lie "on" the polygon outline by using an epsilon when testing
	// its distance to the polygon edges (and hence vertices).
	// This helps avoid cases such as a point exactly on the dateline not getting included
	// by a polygon that has an edge exactly aligned with the dateline.
	// So this value should be extremely small (unlike the other epsilons used in this source file).
	static const double POINT_ON_POLYGON_OUTLINE_COSINE_ = 1 - 1e-12;

	return POINT_ON_POLYGON_OUTLINE_COSINE_;
}


double
GPlatesMaths::PointInPolygon::get_point_on_polygon_threshold_sine()
{
	// Base epsilon calculations off a cosine since that usually has the least accuracy for small angles.
	// '1 - 1e-12' in cosine corresponds to a displacement of about 1.4e-6 [=sin(acos(1 - 1e-12))].
	static const double POINT_ON_POLYGON_OUTLINE_SINE_ = std::sin(std::acos(get_point_on_polygon_threshold_cosine()));

	return POINT_ON_POLYGON_OUTLINE_SINE_;
}
