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

#include <cmath>
#include <iterator>
#include <memory>
#include <utility>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <QDebug>

#include "PointInPolygon.h"

#include "Centroid.h"
#include "PointOnSphere.h"
#include "SmallCircleBounds.h"
#include "Vector3D.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "utils/Profile.h"

namespace GPlatesMaths
{
	namespace PointInPolygon
	{
		//! If dot product of crossing arc and polygon edge greater than this then too closely aligned.
		const double MAX_DOT_PRODUCT_CROSSING_ARC_AND_POLYGON_EDGE = 1 - 1e-4;

		//! If dot product of crossing arc and polygon edge less than this then too closely aligned.
		const double MIN_DOT_PRODUCT_CROSSING_ARC_AND_POLYGON_EDGE = -1 + 1e-4;


		// The test point can lie "on" a polygon edge by using an epsilon when testing
		// closeness to the great circle plane of a polygon edge.
		// This helps avoid cases such as a point exactly on the dateline not getting included
		// by a polygon that has an edge exactly aligned with the dateline.
		// So this value should be extremely small (unlike the other epsilons used in this source file).
		const double POINT_ON_POLYGON_OUTLINE_COSINE = 1 - 1e-12;

		// Base epsilon calculations off a cosine since that usually has the least accuracy for small angles.
		// '1 - 1e-12' in cosine corresponds to a displacement of about 1.4e-6 [=sin(acos(1 - 1e-12))].
		const double POINT_ON_POLYGON_OUTLINE_SINE = std::sin(std::acos(POINT_ON_POLYGON_OUTLINE_COSINE));


		/**
		 * Finds the point antipodal to the centroid of the polygon boundary.
		 *
		 * FIXME: It is still possible for the antipodal point of the polygon centroid
		 * to be inside the polygon for some polygon arrangements. Simply assuming it's
		 * outside the polygon is not always true. Find a more robust solution.
		 * One idea is to find the polygon orientation, then find the closest polygon edge
		 * intersection to a great circle passing through the antipodal centroid and then
		 * determine if antipodal centroid is to the left or right of the polygon edge.
		 * If the polygon is oriented counter-clockwise and the antipodal centroid is to
		 * the right of the polygon edge then the antipodal point is outside the polygon.
		 * This has numerical issues in situations where the polygon edge is aligned with
		 * the great circle and when a polygon edge intersects the antipodal centroid -
		 * these could be dealt with by finding a great circle that does not align with
		 * any polygon edge and adjusting the antipodal centroid point until it doesn't
		 * touch any polygon edge (within a suitable epsilon). Although a self-intersecting
		 * polygon presents more problems in that the left/right test switches polarity
		 * as you traverse the polygon edges to a self-intersection point. This could
		 * be dealt with by decomposing a non-simple polygon into one or more simple polygons
		 * and then performing point-in-polygon tests on each in turn.
		 */
		UnitVector3D
		get_polygon_centroid(
				const PolygonOnSphere &polygon)
		{
			return polygon.get_boundary_centroid();
		}


		/**
		 * Returns the un-normalised normal (rotation axis) of the arc
		 * joining the antipodal point of a polygon's centroid and the test point.
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
				// The arc end points are either the same or they are antipodal.
				// In either case we can just pick any plane that passes through
				// one of the points and it will also pass through the other point.
				// The orientation of the plane will be arbitrary but that's ok because
				// the arc is only used to test for polygon edge crossings and:
				// 1) if the arc end points are the same then it will result in no
				//    edge crossings regardless of the plane orientation, and
				// 2) if the arc end points are antipodal then any arc joining them should
				//    produce the same parity of edge crossings regardless of orientation.
				return generate_perpendicular(crossing_arc_end_point);
			}

			return arc_normal.get_normalisation();
		}


		/**
		 * Returns true if the intersection of polygon edge with crossing arc *great circle* lies on
		 * the crossing *arc* defined by @a crossing_arc_start_point, @a crossing_arc_end_point and
		 * @a dot_crossing_arc_end_points.
		 *
		 * Also sets @a intersection_point_coincides_with_crossings_arc_end_point to true if the
		 * intersection point is close enough to @a crossings_arc_end_point within a very tight threshold.
		 * This implies that the crossing arc end point (ie, the point being tested for inclusion in the polygon)
		 * lies "on" the polygon outline.
		 *
		 * @pre @a intersection_point must lie on the great circle of the crossing arc (within a suitable epsilon).
		 */
		bool
		does_polygon_edge_intersection_with_crossing_gc_lie_on_crossing_gca(
				const UnitVector3D &crossings_arc_start_point,
				const UnitVector3D &crossings_arc_end_point,
				const UnitVector3D &crossings_arc_rotation_axis,
				const double &dot_crossings_arc_end_points,
				const UnitVector3D &intersection_point,
				bool use_point_on_polygon_threshold,
				bool &intersection_point_coincides_with_crossings_arc_end_point)
		{
			// First see if intersection point lies very close to the crossings arc end point.
			if (use_point_on_polygon_threshold &&
				dot(crossings_arc_end_point, intersection_point).dval() > POINT_ON_POLYGON_OUTLINE_COSINE)
			{
				// The crossing arc end point (ie, the point being tested
				// for inclusion in the polygon) lies "on" the polygon outline
				// within a very tight numerical threshold.
				intersection_point_coincides_with_crossings_arc_end_point = true;
				return true;
			}

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
		 * @a crossings_arc_end_point_lies_on_polygon_outline is set to true if
		 * @a crossings_arc_end_point lies on a polygon edge, otherwise it is not written to.
		 */
		unsigned int
		get_num_polygon_edges_crossed(
				const PolygonOnSphere::ring_const_iterator &edges_begin,
				const PolygonOnSphere::ring_const_iterator &edges_end,
				const UnitVector3D &crossings_arc_start_point,
				const UnitVector3D &crossings_arc_end_point,
				const UnitVector3D &crossings_arc_plane_normal,
				const double &dot_crossings_arc_end_points,
				bool use_point_on_polygon_threshold,
				bool &crossings_arc_end_point_lies_on_polygon_outline)
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
								crossings_arc_end_point,
								crossings_arc_plane_normal,
								dot_crossings_arc_end_points,
								edge_end_point/*intersection_point*/,
								use_point_on_polygon_threshold,
								crossings_arc_end_point_lies_on_polygon_outline))
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
								if (dot(edge_plane_normal, crossings_arc_start_point).dval() >= 0)
								{
									const double dot_edge_plane_normal_and_crossings_arc_end_point =
											dot(edge_plane_normal, crossings_arc_end_point).dval();
									if (use_point_on_polygon_threshold)
									{
										if (dot_edge_plane_normal_and_crossings_arc_end_point <
											// This allows the crossing arc end point (which is the point
											// being tested for inclusion in the polygon) to lie "on" the
											// polygon outline within a very tight numerical threshold...
											POINT_ON_POLYGON_OUTLINE_SINE)
										{
											if (dot_edge_plane_normal_and_crossings_arc_end_point >
												-POINT_ON_POLYGON_OUTLINE_SINE)
											{
												// The crossing arc end point (ie, the point being tested
												// for inclusion in the polygon) lies "on" the polygon outline
												// within a very tight numerical threshold.
												crossings_arc_end_point_lies_on_polygon_outline = true;
											}

											++num_edges_crossed;
										}
									}
									else // no point-on-polygon threshold 
									{
										if (dot_edge_plane_normal_and_crossings_arc_end_point <= 0)
										{
											++num_edges_crossed;
										}
									}
								}
							}
							else // edge start point is on positive side of crossings-arc plane...
							{
								// The crossings-arc endpoints must be on opposite sides of
								// the edge plane *and* the crossings-arc start point must
								// be on the *negative* side since the polygon edge start point
								// is on the *positive* side of the crossings-arc plane.
								if (dot(edge_plane_normal, crossings_arc_start_point).dval() <= 0)
								{
									const double dot_edge_plane_normal_and_crossings_arc_end_point =
											dot(edge_plane_normal, crossings_arc_end_point).dval();
									if (use_point_on_polygon_threshold)
									{
										if (dot_edge_plane_normal_and_crossings_arc_end_point >
											// This allows the crossing arc end point (which is the point
											// being tested for inclusion in the polygon) to lie "on" the
											// polygon outline within a very tight numerical threshold...
											-POINT_ON_POLYGON_OUTLINE_SINE)
										{
											if (dot_edge_plane_normal_and_crossings_arc_end_point <
												POINT_ON_POLYGON_OUTLINE_SINE)
											{
												// The crossing arc end point (ie, the point being tested
												// for inclusion in the polygon) lies "on" the polygon outline
												// within a very tight numerical threshold.
												crossings_arc_end_point_lies_on_polygon_outline = true;
											}

											++num_edges_crossed;
										}
									}
									else // no point-on-polygon threshold 
									{
										if (dot_edge_plane_normal_and_crossings_arc_end_point >= 0)
										{
											++num_edges_crossed;
										}
									}
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
									crossings_arc_end_point,
									crossings_arc_plane_normal,
									dot_crossings_arc_end_points,
									// Intersection of polygon edge with great circle of crossings arc...
									ring_edge_intersects_crossings_arc_plane/*intersection_point*/,
									use_point_on_polygon_threshold,
									crossings_arc_end_point_lies_on_polygon_outline))
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

			bool crossings_arc_end_point_lies_on_polygon_outline = false;
			unsigned int num_polygon_edges_crossed = get_num_polygon_edges_crossed(
					polygon.exterior_ring_begin(), polygon.exterior_ring_end(),
					crossings_arc_start_point, crossings_arc_end_point,
					crossings_arc_plane_normal, dot_crossings_arc_end_points,
					use_point_on_polygon_threshold, crossings_arc_end_point_lies_on_polygon_outline);
			
			for (unsigned int interior_ring_index = 0;
				interior_ring_index < polygon.number_of_interior_rings();
				++interior_ring_index)
			{
				num_polygon_edges_crossed += get_num_polygon_edges_crossed(
						polygon.interior_ring_begin(interior_ring_index),
						polygon.interior_ring_end(interior_ring_index),
						crossings_arc_start_point, crossings_arc_end_point,
						crossings_arc_plane_normal, dot_crossings_arc_end_points,
						use_point_on_polygon_threshold, crossings_arc_end_point_lies_on_polygon_outline);
			}

			// If the test point lies *on* any polygon outline (exterior or interior) then it's
			// considered to be *inside* the polygon (regardless of the number of polygon edges crossed).
			// Otherwise if the number of edges crossed is odd then test point is inside the polygon.
			// Otherwise the test point is outside the polygon.
			return crossings_arc_end_point_lies_on_polygon_outline ||
					((num_polygon_edges_crossed & 1) == 1);
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
		 * potentially cross. Any edges which cannot possibly be crossed can be excluded.
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
			bool crossings_arc_end_point_lies_on_polygon_outline = false;

			// Iterate over the edge sequences and see how many edges cross the crossings-arc.
			edge_sequence_list_type::const_iterator edge_sequences_iter = edge_sequences_begin;
			for ( ; edge_sequences_iter != edge_sequences_end; ++edge_sequences_iter)
			{
				const EdgeSequence &edge_sequence = *edge_sequences_iter;

				num_polygon_edges_crossed += get_num_polygon_edges_crossed(
						edge_sequence.begin, edge_sequence.end,
						crossings_arc_start_point, crossings_arc_end_point,
						crossings_arc_plane_normal, dot_crossings_arc_end_points,
						use_point_on_polygon_threshold, crossings_arc_end_point_lies_on_polygon_outline);
			}


			// If the test point lies *on* any polygon outline (exterior or interior) then it's
			// considered to be *inside* the polygon (regardless of the number of polygon edges crossed).
			// Otherwise if the number of edges crossed is odd then test point is inside the polygon.
			// Otherwise the test point is outside the polygon.
			return crossings_arc_end_point_lies_on_polygon_outline ||
					((num_polygon_edges_crossed & 1) == 1);
		}


		/**
		 * Returns the coverage of polygon edges as a min/max range of dot products to a point.
		 *
		 * The polygon edges are defined by a sequence of edge sequences of @a edge_sequences.
		 */
		std::pair< double/*min dot product*/, double/*max dot product*/ >
		get_dot_product_range_of_polygon_edges_to_point(
				const edge_sequence_list_type &edge_sequences,
				const UnitVector3D &point)
		{
			double min_dot_product = 1.0;
			double max_dot_product = -1.0;

			// Iterate over the edge sequences.
			BOOST_FOREACH(const EdgeSequence &edge_sequence, edge_sequences)
			{
				// Iterate over the polygon edges to be tested for intersection.
				PolygonOnSphere::ring_const_iterator polygon_edge_iter = edge_sequence.begin;
				for ( ; polygon_edge_iter != edge_sequence.end; ++polygon_edge_iter)
				{
					const GreatCircleArc &edge = *polygon_edge_iter;

					const UnitVector3D &edge_start_point = edge.start_point().position_vector();
					const UnitVector3D &edge_end_point = edge.end_point().position_vector();

					const double edge_start_dot_polygon_centroid_antipodal =
							dot(edge_start_point, point).dval();
					const double edge_end_dot_polygon_centroid_antipodal =
							dot(edge_end_point, point).dval();

					// See if the polygon edge arc endpoints are closer/further than
					// the current closest/furthest so far.
					if (edge_start_dot_polygon_centroid_antipodal > max_dot_product)
					{
						max_dot_product = edge_start_dot_polygon_centroid_antipodal;
					}
					if (edge_start_dot_polygon_centroid_antipodal < min_dot_product)
					{
						min_dot_product = edge_start_dot_polygon_centroid_antipodal;
					}
					if (edge_end_dot_polygon_centroid_antipodal > max_dot_product)
					{
						max_dot_product = edge_end_dot_polygon_centroid_antipodal;
					}
					if (edge_end_dot_polygon_centroid_antipodal < min_dot_product)
					{
						min_dot_product = edge_end_dot_polygon_centroid_antipodal;
					}

					if (edge.is_zero_length())
					{
						continue;
					}

					// Polygon edge is not zero length and hence has a rotation axis...
					const UnitVector3D &edge_plane_normal = edge.rotation_axis();

					const Vector3D polygon_centroid_antipodal_cross_edge_plane_normal =
							cross(point, edge_plane_normal);
					if (polygon_centroid_antipodal_cross_edge_plane_normal.magSqrd() <= 0)
					{
						// The polygon edge rotation axis is aligned, within numerical precision,
						// with the polygon centroid/antipodal axis. Therefore the dot product
						// distance of all points on the polygon edge to the polygon centroid
						// antipodal is the same (within numerical precision).
						// And we've already covered this case above.
						// The issue of precision is covered below where we subtract/add an
						// epsilon to the min/max dot product to expand the region covered
						// by the polygon edges slightly.
						continue;
					}

					// See if the polygon edge arc forms a local minima or maxima between
					// its endpoints. This happens if its endpoints are on opposite sides
					// of the dividing plane.
					const bool edge_start_point_on_negative_side = dot(
							polygon_centroid_antipodal_cross_edge_plane_normal,
							edge_start_point).dval() <= 0;
					const bool edge_end_point_on_negative_side = dot(
							polygon_centroid_antipodal_cross_edge_plane_normal,
							edge_end_point).dval() <= 0;

					// If endpoints on opposite sides of the plane.
					if (edge_start_point_on_negative_side ^ edge_end_point_on_negative_side)
					{
						if (edge_start_point_on_negative_side)
						{
							// The arc forms a local maxima...

							const UnitVector3D maxima_point = cross(
									polygon_centroid_antipodal_cross_edge_plane_normal,
									edge_plane_normal)
								.get_normalisation();
							const double maxima_point_dot_polygon_centroid_antipodal =
									dot(maxima_point, point).dval();
							// See if the current arc is the furthest so far from the
							// polygon centroid antipodal point.
							if (maxima_point_dot_polygon_centroid_antipodal < min_dot_product)
							{
								min_dot_product = maxima_point_dot_polygon_centroid_antipodal;
							}
						}
						else
						{
							// The arc forms a local minima...

							const UnitVector3D minima_point = cross(
									edge_plane_normal,
									polygon_centroid_antipodal_cross_edge_plane_normal)
								.get_normalisation();
							const double minima_point_dot_polygon_centroid_antipodal =
									dot(minima_point, point).dval();
							// See if the current arc is the closest so far to the
							// polygon centroid antipodal point.
							if (minima_point_dot_polygon_centroid_antipodal > max_dot_product)
							{
								max_dot_product = minima_point_dot_polygon_centroid_antipodal;
							}
						}
					}
				}
			}

			// The epsilon expands the dot product range the polygon covers
			// as a protection against numerical precision.
			// This epsilon should be larger than used in class Real (which is about 1e-12).
			min_dot_product -= 1e-6;
			max_dot_product += 1e-6;

			return std::make_pair(min_dot_product, max_dot_product);
		}


		/**
		 * A recursive spatial partition of the surface of the sphere into spherical lunes.
		 *
		 * A spherical lune is the surface of the sphere bounded by two half great circles
		 * that intersect along a common axis. The volume bounded by the two semi-planes
		 * (of the half circles) and the spherical lune is a spherical wedge.
		 *
		 * The common axis is the polygon centroid and its antipodal point and the spherical
		 * lunes partition the possible paths that a crossings-arc can take from the polygon
		 * centroid antipodal point to any test point (point-in-polygon) on the globe.
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
			std::auto_ptr<SphericalLuneTree>
			create(
					const PolygonOnSphere::non_null_ptr_to_const_type &polygon,
					bool build_ologn_hint,
					bool keep_shared_reference_to_polygon);


			bool
			is_point_in_polygon(
					const UnitVector3D &test_point,
					bool use_point_on_polygon_threshold) const;

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

				//! The first/second child node is on the negative/positive side of the plane.
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
						const InnerOuterBoundingSmallCircle &antipodal_centroid_bounds_) :
					edge_sequences_start_index(edge_sequences_start_index_),
					num_edge_sequences(num_edge_sequences_),
					antipodal_centroid_bounds(antipodal_centroid_bounds_)
				{  }

				/**
				 * Default constructor for the case where no sequences intersect spherical lune of leaf node.
				 *
				 * This happens when polygon centroid is outside polygon and spherical lune wedge
				 * does not intersect polygon.
				 */
				LeafNode() :
					edge_sequences_start_index(0),
					num_edge_sequences(0)
				{  }

				edge_sequence_index_type edge_sequences_start_index;
				unsigned int num_edge_sequences;
				boost::optional<InnerOuterBoundingSmallCircle> antipodal_centroid_bounds;
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
						const UnitVector3D &polygon_centroid_antipodal,
						const PolygonOnSphere &polygon);

				/**
				 * The small circle inner/outer bounds of all polygon edges relative
				 * to the polygon centroid antipodal point.
				 */
				InnerOuterBoundingSmallCircleBuilder d_antipodal_centroid_bounds_builder;

				/**
				 * Whether the polygon centroid is inside the polygon.
				 */
				bool d_is_polygon_centroid_in_polygon;
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
				 * to the polygon centroid antipodal point.
				 */
				InnerOuterBoundingSmallCircle d_antipodal_centroid_bounds;

				/**
				 * Whether the polygon centroid is inside the polygon.
				 */
				bool d_is_polygon_centroid_in_polygon;
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
						const UnitVector3D &polygon_centroid_antipodal);

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
				const UnitVector3D d_polygon_centroid_antipodal;
				const UnitVector3D d_polygon_centroid;
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
			 * The antipodal of the polygon centroid that is also the start of the crossings
			 * arc (the end is the test point of the point-in-polygon test).
			 *
			 * This is assumed to be outside the polygon (FIXME: not necessarily true).
			 */
			const UnitVector3D d_polygon_centroid_antipodal;

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


			SphericalLuneTree(
					const PolygonOnSphere::non_null_ptr_to_const_type &polygon,
					bool keep_shared_reference_to_polygon,
					const UnitVector3D &polygon_centroid_antipodal,
					const BoundsDataBuilder &bounds_data_builder,
					const boost::optional<TreeData> &tree_data = boost::none);

			bool
			is_point_in_polygon(
					const InternalNode &node,
					const UnitVector3D &test_point,
					bool use_point_on_polygon_threshold) const;
		};


		std::auto_ptr<SphericalLuneTree>
		SphericalLuneTree::create(
				const PolygonOnSphere::non_null_ptr_to_const_type &polygon,
				bool build_ologn_hint,
				bool keep_shared_reference_to_polygon)
		{
			// Get the point that is antipodal to the polygon centroid.
			// This point is deemed to be outside the polygon (FIXME: which might not be true).
			const UnitVector3D polygon_centroid_antipodal(-get_polygon_centroid(*polygon));

			// Only build a tree if we've been asked to *and* there are enough polygon edges
			// to make it worthwhile.
			if (!build_ologn_hint ||
				polygon->number_of_segments() <= TreeBuilder::AVERAGE_NUM_EDGES_PER_LEAF_NODE)
			{
				// Don't build a spherical lune tree.
				// But can still benefit from the bounds data.

				BoundsDataBuilder bounds_data_builder(polygon_centroid_antipodal, *polygon);
				bounds_data_builder.d_antipodal_centroid_bounds_builder.add(*polygon);

				return std::auto_ptr<SphericalLuneTree>(
						new SphericalLuneTree(
								polygon,
								keep_shared_reference_to_polygon,
								polygon_centroid_antipodal,
								bounds_data_builder));
			}

			// Build the spherical lune tree.
			TreeBuilder tree_builder(*polygon, polygon_centroid_antipodal);
			tree_builder.build_tree();

			// Copy the built tree structures - copying also has the advantage of removing
			// any excess memory usage caused by std::vector.
			// Also copy the polygon bounds data built by the tree builder.
			return std::auto_ptr<SphericalLuneTree>(
					new SphericalLuneTree(
							polygon,
							keep_shared_reference_to_polygon,
							polygon_centroid_antipodal,
							tree_builder.d_bounds_data_builder,
							tree_builder.d_tree_data));
		}


		SphericalLuneTree::SphericalLuneTree(
				const PolygonOnSphere::non_null_ptr_to_const_type &polygon,
				bool keep_shared_reference_to_polygon,
				const UnitVector3D &polygon_centroid_antipodal,
				const BoundsDataBuilder &bounds_data_builder,
				const boost::optional<TreeData> &tree_data) :
			d_polygon(*polygon),
			d_polygon_centroid_antipodal(polygon_centroid_antipodal),
			d_bounds_data(bounds_data_builder),
			d_tree_data(tree_data)
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
					d_bounds_data.d_antipodal_centroid_bounds.test(test_point);
			// See if the test point is clearly outside the polygon.
			// Note that this means inside the inner bounds since the centre of the bounding
			// inner/outer small circles is polygon centroid *antipodal* point.
			if (bounds_result == InnerOuterBoundingSmallCircle::INSIDE_INNER_BOUNDS)
			{
				// The test point is outside the polygon.
				return false;
			}
			// See if the test point is closer to the polygon centroid than any polygon
			// edge (this is like a clearly-inside test except the centroid might not be
			// inside the polygon so we'll take the result of the centroid test whether
			// that's inside or outside).
			if (bounds_result == InnerOuterBoundingSmallCircle::OUTSIDE_OUTER_BOUNDS)
			{
				return d_bounds_data.d_is_polygon_centroid_in_polygon;
			}

			// If it's more efficient to bypass the spherical lune tree then simply
			// test all the edges of the polygon. We still have the advantage of not having
			// to calculate the polygon centroid for each point tested.
			if (!d_tree_data)
			{
				const UnitVector3D &crossings_arc_start_point = d_polygon_centroid_antipodal;
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

			// If there are no intersections in the current spherical lune then it means the
			// polygon centroid is outside the polygon (which can happen for concave polygons -
			// picture a U-shaped polygon like a horse shoe on the surface of the globe and the
			// spherical lune wedge with axis from antipodal centroid to centroid and its two
			// half-planes - the U-shaped polygon does not intersect the wedge).
			// Therefore the test point, being in the spherical lune wedge, is also outside the polygon.
			if (!leaf_node.antipodal_centroid_bounds)
			{
				return false;
			}

			// First determine if we can get an early result to avoid testing the polygon edges.
			// Test the point against the outer/inner small circle bounds.
			const InnerOuterBoundingSmallCircle::Result bounds_result =
					leaf_node.antipodal_centroid_bounds.get().test(test_point);
			// See if the test point is clearly outside the polygon.
			// Note that this means inside the inner bounds since the centre of the bounding
			// inner/outer small circles is polygon centroid *antipodal* point.
			if (bounds_result == InnerOuterBoundingSmallCircle::INSIDE_INNER_BOUNDS)
			{
				// The test point is outside the polygon.
				return false;
			}
			// See if the test point is closer to the polygon centroid than any polygon
			// edge in the current leaf node (this is like a clearly-inside test except
			// the centroid might not be inside the polygon so we'll take the result of
			// the centroid test whether that's inside or outside).
			if (bounds_result == InnerOuterBoundingSmallCircle::OUTSIDE_OUTER_BOUNDS)
			{
				return d_bounds_data.d_is_polygon_centroid_in_polygon;
			}

			// Get the iterator range of edge sequences referenced by the leaf node.
			const edge_sequence_list_type::const_iterator edge_sequence_list_begin =
					d_tree_data->d_edge_sequences.begin() + leaf_node.edge_sequences_start_index;
			const edge_sequence_list_type::const_iterator edge_sequence_list_end =
					edge_sequence_list_begin + leaf_node.num_edge_sequences;

			// Test using the subset of polygon edges associated with the leaf node.
			const UnitVector3D &crossings_arc_start_point = d_polygon_centroid_antipodal;
			const UnitVector3D &crossings_arc_end_point = test_point;
			return GPlatesMaths::PointInPolygon::is_point_in_polygon(
					edge_sequence_list_begin,
					edge_sequence_list_end,
					crossings_arc_start_point,
					crossings_arc_end_point,
					use_point_on_polygon_threshold);
		}


		SphericalLuneTree::BoundsDataBuilder::BoundsDataBuilder(
				const UnitVector3D &polygon_centroid_antipodal,
				const PolygonOnSphere &polygon) :
			d_antipodal_centroid_bounds_builder(polygon_centroid_antipodal)
		{
			// Test whether the polygon centroid is inside the polygon.
			// Note that the crossings-arc is arbitrary (because the arc start and end points are
			// antipodal to each other - polygon centroid and antipodal centroid), but it doesn't
			// matter because the result should be the same regardless of which arc is chosen.
			const UnitVector3D &crossings_arc_start_point = polygon_centroid_antipodal;
			const UnitVector3D crossings_arc_end_point = -polygon_centroid_antipodal;
			d_is_polygon_centroid_in_polygon =
					GPlatesMaths::PointInPolygon::is_point_in_polygon(
							polygon,
							crossings_arc_start_point,
							crossings_arc_end_point,
							false/*use_point_on_polygon_threshold*/);
		}


		SphericalLuneTree::BoundsData::BoundsData(
				const BoundsDataBuilder &bounds_data_builder) :
			d_antipodal_centroid_bounds(
					bounds_data_builder.d_antipodal_centroid_bounds_builder
							.get_inner_outer_bounding_small_circle()),
			d_is_polygon_centroid_in_polygon(bounds_data_builder.d_is_polygon_centroid_in_polygon)
		{  }


		// '1e-4' represents an angular deviation of 0.34 minutes away from the plane.
		// The spherical wedge is expanded slightly to include polygon edges that are
		// near the wedge (to protect against numerical precision issues).
		const double SphericalLuneTree::TreeBuilder::SPHERICAL_WEDGE_PLANE_EPSILON = 1e-4;

		SphericalLuneTree::TreeBuilder::TreeBuilder(
				const PolygonOnSphere &polygon,
				const UnitVector3D &polygon_centroid_antipodal) :
			d_bounds_data_builder(polygon_centroid_antipodal, polygon),
			d_polygon(polygon),
			d_polygon_centroid_antipodal(polygon_centroid_antipodal),
			d_polygon_centroid(-polygon_centroid_antipodal)
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
			// The axis of all spherical lunes is the polygon centroid.

			// The splitting plane of the root node can be any plane passes through the
			// polygon centroid (and hence also the polygon centroid antipodal point).
			const UnitVector3D root_node_splitting_plane =
					generate_perpendicular(d_polygon_centroid);

			const UnitVector3D child_nodes_splitting_plane(
					cross(d_polygon_centroid, root_node_splitting_plane));

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

			// Get the range of dot products (from polygon centroid antipodal) that the
			// intersecting polygon edges lie within.
			// This will be used for early inside/outside point-in-polygon tests.

			// Determine the bounds for the current subtree (which is a leaf).
			InnerOuterBoundingSmallCircleBuilder leaf_node_bounds_data_builder(
					d_polygon_centroid_antipodal);

			// If there are no intersections in the current spherical lune then it means the
			// polygon centroid is outside the polygon (which can happen for concave polygons -
			// picture a U-shaped polygon like a horse shoe on the surface of the globe and the
			// spherical lune wedge with axis from antipodal centroid to centroid and its two
			// half-planes - the U-shaped polygon does not intersect the wedge).
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
					leaf_node_bounds_data_builder.get_inner_outer_bounding_small_circle();

			// Expand/contract the outer/inner bounds of the entire spherical lune tree
			// to include the current leaf node bounds.
			d_bounds_data_builder.d_antipodal_centroid_bounds_builder.add(
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
					// crosses one half-circle then it must also cross the other half-circle.
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
						// NOTE: the spherical wedge start point is the polygon centroid.
						if (dot(first_spherical_wedge_bounding_plane, edge_end_point).dval() >= 0 &&
							dot(edge_plane_normal, d_polygon_centroid).dval() >= 0)
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
						// NOTE: the spherical wedge start point is the polygon centroid.
						if (dot(first_spherical_wedge_bounding_plane, edge_end_point).dval() <= 0 &&
							dot(edge_plane_normal, d_polygon_centroid).dval() <= 0)
						{
							// Polygon edge intersects spherical lune so continue to next edge.
							++num_intersecting_edges;
							continue;
						}
					}
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


bool
GPlatesMaths::PointInPolygon::is_point_in_polygon(
		const PointOnSphere &point,
		const PolygonOnSphere &polygon,
		bool use_point_on_polygon_threshold)
{
	// Get the point that is antipodal to the polygon centroid.
	// This point is deemed to be outside the polygon (FIXME: which might not be true).
	const UnitVector3D crossings_arc_start_point = -get_polygon_centroid(polygon);

	const UnitVector3D &crossings_arc_end_point = point.position_vector();

	return is_point_in_polygon(polygon, crossings_arc_start_point, crossings_arc_end_point, use_point_on_polygon_threshold);
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
