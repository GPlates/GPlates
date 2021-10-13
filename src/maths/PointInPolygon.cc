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
#include <utility>
#include <boost/foreach.hpp>

#include "PointInPolygon.h"

#include "PointOnSphere.h"
#include "Vector3D.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "utils/Profile.h"


namespace GPlatesMaths
{
	namespace PointInPolygon
	{
		namespace
		{
			//! If dot product of crossing arc and polygon edge greater than this then too closely aligned.
			static const double MAX_DOT_PRODUCT_CROSSING_ARC_AND_POLYGON_EDGE = 1 - 1e-4;

			//! If dot product of crossing arc and polygon edge less than this then too closely aligned.
			static const double MIN_DOT_PRODUCT_CROSSING_ARC_AND_POLYGON_EDGE = -1 + 1e-4;


			/**
			 * Finds the point antipodal to the centroid (average vertex) of the polygon.
			 *
			 * If the centroid is the origin then attempts to recalculate centroid by summing
			 * the edge midpoints.
			 *
			 * FIXME: Use a more accurate heuristic to determine the antipodal point such as
			 * using area of spherical polygon.
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
				// Iterate through the polygon vertices and calculate the sum of vertex positions.
				Vector3D summed_position(0,0,0);
				PolygonOnSphere::vertex_const_iterator vertex_iter = polygon.vertex_begin();
				const PolygonOnSphere::vertex_const_iterator vertex_end = polygon.vertex_end();
				for ( ; vertex_iter != vertex_end; ++vertex_iter)
				{
					const PointOnSphere &vertex = *vertex_iter;
					const Vector3D point(vertex.position_vector());
					summed_position = summed_position + point;
				}

				// If the magnitude of the summed vertex position is zero then all
				// the points averaged to zero. This most likely happens when the
				// vertices roughly form a great circle arc - which should be quite unlikely.
				if (summed_position.magSqrd() <= 0.0)
				{
					// Iterate through the polygon edges and sum the edge midpoints
					// instead of the vertices in an attempt to get a different answer.
					summed_position = Vector3D(0,0,0);
					PolygonOnSphere::const_iterator edge_iter = polygon.begin();
					const PolygonOnSphere::const_iterator edge_end = polygon.end();
					for ( ; edge_iter != edge_end; ++edge_iter)
					{
						const GreatCircleArc &edge = *edge_iter;
						const Vector3D edge_mid_point(
								(Vector3D(edge.start_point().position_vector()) +
									Vector3D(edge.end_point().position_vector()))
								.get_normalisation());
						summed_position = summed_position + edge_mid_point;
					}

					if (summed_position.magSqrd() <= 0.0)
					{
						// The edge midpoints summed to origin so just bail out and
						// return the north pole.
						// FIXME: Either all point-in-polygon tests will be wrong or they
						// will all be right and either way is equally likely at this point.
						summed_position = Vector3D(0, 0, 1);
					}
				}

				// Return the centroid.
				return summed_position.get_normalisation();
			}


			/**
			 * Returns the un-normalised normal (rotation axis) of the arc
			 * joining the antipodal point of a polygon's centroid and the test point.
			 *
			 * If the arc is zero length or the points are antipodal to each other then
			 * a normal to an arbitrary great circle passing through both points is returned.
			 *
			 * Note that @a antipodal_polygon_centroid is not necessarily normalised -
			 * this is ok since we only need to generate a plane normal that is perpendicular.
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
			 * Returns true if @a test_point lies on the great circle arc defined by
			 * @a arc_start_point, @a arc_rotation_axis and @a dot_arc_end_points.
			 *
			 * @pre the test point @a test_point must lie on the great circle, within a
			 * suitable epsilon, defined by @a arc_rotation_axis.
			 */
			bool
			if_point_lies_on_gc_does_it_also_lie_on_gca(
					const UnitVector3D &arc_start_point,
					const UnitVector3D &arc_rotation_axis,
					const double &dot_arc_end_points,
					const UnitVector3D &test_point)
			{
				// We are assured that the test point lies on the great circle that the
				// arc lies on.
				// So we just need to determine if the test point lies on the arc itself.
				return
					// Is the test point closer to the arc start point than the arc end point is...
					dot(arc_start_point, test_point).dval() >= dot_arc_end_points &&
					// Does the test point lie on the half-circle starting at the arc start point...
					dot(cross(arc_start_point, test_point), arc_rotation_axis).dval() >= 0;
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
			 * @a crossing_arc_start_point and @a crossing_arc_end_point.
			 *
			 * The sequence of edges begins at @a polygon_edges_begin and ends
			 * at @a polygon_edges_end. Note that the sequence of edges must *not* wrap
			 * past the last edge of the polygon.
			 *
			 * @a crossing_arc_plane_normal is the plane normal of the great circle
			 * passing through the crossing arc points.
			 * If the arc is zero length or the arc endpoints are antipodal then the plane normal
			 * can be any plane that passes through one (and hence both) of the arc points.
			 */
			unsigned int
			get_num_polygon_edges_crossed(
					const PolygonOnSphere::const_iterator &polygon_edges_begin,
					const PolygonOnSphere::const_iterator &polygon_edges_end,
					const UnitVector3D &crossing_arc_start_point,
					const UnitVector3D &crossing_arc_end_point,
					const UnitVector3D &crossing_arc_plane_normal,
					const double &dot_crossings_arc_end_points)
			{
				if (polygon_edges_begin == polygon_edges_end)
				{
					return 0;
				}

				unsigned int num_polygon_edges_crossed = 0;
				PolygonOnSphere::const_iterator polygon_edge_iter = polygon_edges_begin;

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
				const GreatCircleArc &first_edge = *polygon_edge_iter;
				bool is_edge_start_on_negative_side_of_crossings_arc_plane =
						dot(crossing_arc_plane_normal,
							first_edge.start_point().position_vector()).dval() < 0;

				// Iterate over the polygon edges in the sequence.
				do
				{
					const GreatCircleArc &edge = *polygon_edge_iter;
					const UnitVector3D &edge_end_point = edge.end_point().position_vector();

					// Determine which side of arc plane the end vertex of the current edge is on.
					// Note: this is *not* an epsilon test.
					// Note: this should be the exact same comparison as with the start vertex.
					const bool is_edge_end_on_negative_side_of_crossings_arc_plane =
							dot(crossing_arc_plane_normal, edge_end_point).dval() < 0;

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
							if (if_point_lies_on_gc_does_it_also_lie_on_gca(
									crossing_arc_start_point,
									crossing_arc_plane_normal,
									dot_crossings_arc_end_points,
									edge_end_point))
							{
								++num_polygon_edges_crossed;
							}
						}
						else // The polygon edge (GCA) has a rotation axis (plane normal)...
						{
							const UnitVector3D &edge_plane_normal = edge.rotation_axis();

							// Get a measure of how closely aligned the polygon edge arc
							// is relative to the crossings arc.
							const double crossings_arc_normal_dot_edge_normal =
									dot(crossing_arc_plane_normal, edge_plane_normal).dval();

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
									if (dot(edge_plane_normal, crossing_arc_start_point).dval() >= 0 &&
										dot(edge_plane_normal, crossing_arc_end_point).dval() <= 0)
									{
										++num_polygon_edges_crossed;
									}
								}
								else // edge start point is on positive side of crossings-arc plane...
								{
									// The crossings-arc endpoints must be on opposite sides of
									// the edge plane *and* the crossings-arc start point must
									// be on the *negative* side since the polygon edge start point
									// is on the *positive* side of the crossings-arc plane.
									if (dot(edge_plane_normal, crossing_arc_start_point).dval() <= 0 &&
										dot(edge_plane_normal, crossing_arc_end_point).dval() >= 0)
									{
										++num_polygon_edges_crossed;
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
								const UnitVector3D polygon_edge_intersects_crossings_arc_plane =
										if_plane_divides_gca_get_intersection_of_gca_and_plane(
												edge, crossing_arc_plane_normal);

								// If the intersection lies on the crossings-arc then
								// increment the number of polygon edges crossed.
								if (if_point_lies_on_gc_does_it_also_lie_on_gca(
										crossing_arc_start_point,
										crossing_arc_plane_normal,
										dot_crossings_arc_end_points,
										polygon_edge_intersects_crossings_arc_plane))
								{
									++num_polygon_edges_crossed;
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
				while (++polygon_edge_iter != polygon_edges_end);

				return num_polygon_edges_crossed;
			}


			//! Keeps track of an iteration range of polygon edges.
			class EdgeSequence
			{
			public:
				EdgeSequence(
						const PolygonOnSphere::const_iterator &begin_,
						const PolygonOnSphere::const_iterator &end_) :
					begin(begin_),
					end(end_)
				{  }

				PolygonOnSphere::const_iterator begin;
				PolygonOnSphere::const_iterator end;
			};

			//! Typedef for a list of polygon edge ranges.
			typedef std::vector<EdgeSequence> edge_sequence_list_type;


			/**
			 * Returns the number of polygon edges crossed by the arc defined by
			 * @a crossings_arc_start_point, @a crossings_arc_end_point and
			 * @a dot_crossings_arc_end_points.
			 *
			 * The polygon edges are defined by a sequence of edge sequences
			 * @a edge_sequences_begin and @a edge_sequences_end.
			 */
			unsigned int
			get_num_polygon_edges_crossed(
					const edge_sequence_list_type::const_iterator &edge_sequences_begin,
					const edge_sequence_list_type::const_iterator &edge_sequences_end,
					const UnitVector3D &crossings_arc_start_point,
					const UnitVector3D &crossings_arc_end_point,
					const double &dot_crossings_arc_end_points)
			{
				const UnitVector3D crossings_arc_plane_normal = get_crossings_arc_plane_normal(
						crossings_arc_start_point, crossings_arc_end_point);

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

				return num_polygon_edges_crossed;
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
					PolygonOnSphere::const_iterator polygon_edge_iter = edge_sequence.begin;
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
			SphericalLuneTree(
					const PolygonOnSphere::non_null_ptr_to_const_type &polygon,
					bool build_ologn_hint);


			unsigned int
			get_num_polygon_edges_crossed(
					const UnitVector3D &test_point) const;

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
						const double &min_dot_product_of_poly_edges_to_centroid_antipodal_,
						const double &max_dot_product_of_poly_edges_to_centroid_antipodal_) :
					edge_sequences_start_index(edge_sequences_start_index_),
					num_edge_sequences(num_edge_sequences_),
					min_dot_product_of_poly_edges_to_centroid_antipodal(
							min_dot_product_of_poly_edges_to_centroid_antipodal_),
					max_dot_product_of_poly_edges_to_centroid_antipodal(
							max_dot_product_of_poly_edges_to_centroid_antipodal_)
				{  }

				edge_sequence_index_type edge_sequences_start_index;
				unsigned int num_edge_sequences;
				double min_dot_product_of_poly_edges_to_centroid_antipodal;
				double max_dot_product_of_poly_edges_to_centroid_antipodal;
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
			 * The inner and outer bounding small circles used for quickly testing
			 * if a point is inside/outside the polygon.
			 */
			class BoundsData
			{
			public:
				//! Default constructor.
				BoundsData() :
					d_min_dot_poly_edges_with_antipodal_centroid(1.0),
					d_max_dot_poly_edges_with_antipodal_centroid(-1.0),
					d_parity_of_num_edges_crossed_from_polygon_centroid_to_antipodal(0)
				{  }

				//! Minimum dot product of all polygon edges with polygon centroid antipodal.
				double d_min_dot_poly_edges_with_antipodal_centroid;

				//! Maximum dot product of all polygon edges with polygon centroid antipodal.
				double d_max_dot_poly_edges_with_antipodal_centroid;

				/**
				 * The parity of polygon edge crossings from polygon centroid to its antipodal point.
				 */
				unsigned int d_parity_of_num_edges_crossed_from_polygon_centroid_to_antipodal;
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
				 * Bounds data for the entire polygon - byproduct of calculating bounds for
				 * the leaf nodes of the tree.
				 */
				BoundsData d_bounds_data;

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
			 */
			PolygonOnSphere::non_null_ptr_to_const_type d_polygon;

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


			unsigned int
			get_num_polygon_edges_crossed(
					const InternalNode &node,
					const UnitVector3D &test_point) const;
		};


		SphericalLuneTree::SphericalLuneTree(
				const PolygonOnSphere::non_null_ptr_to_const_type &polygon,
				bool build_ologn_hint) :
			d_polygon(polygon),
			// Get the point that is antipodal to the polygon centroid.
			// This point is deemed to be outside the polygon (FIXME: which might not be true).
			d_polygon_centroid_antipodal(-get_polygon_centroid(*polygon))
		{
			// Only build a tree if we've been asked to *and* there are enough polygon edges
			// to make it worthwhile.
			if (!build_ologn_hint ||
				polygon->number_of_segments() <= TreeBuilder::AVERAGE_NUM_EDGES_PER_LEAF_NODE)
			{
				// Don't build a spherical lune tree.
				// But can still benefit from the bounds data.

				const edge_sequence_list_type all_polygon_edges(1,
						EdgeSequence(d_polygon->begin(), d_polygon->end()));

				// Get the range of dot products (from polygon centroid antipodal) that all
				// the polygon edges lie within.
				// This will be used for early inside/outside point-in-polygon tests.
				std::pair< double/*min dot product*/, double/*max dot product*/ >
						dot_product_range_of_all_edges_to_polygon_centroid_antipodal =
								get_dot_product_range_of_polygon_edges_to_point(
										all_polygon_edges,
										d_polygon_centroid_antipodal);
				d_bounds_data.d_min_dot_poly_edges_with_antipodal_centroid =
						dot_product_range_of_all_edges_to_polygon_centroid_antipodal.first;
				d_bounds_data.d_max_dot_poly_edges_with_antipodal_centroid =
						dot_product_range_of_all_edges_to_polygon_centroid_antipodal.second;

				// Get the number of edges crossed from polygon centroid antipodal to polygon centroid.
				// This will vary depending on which arbitrary crossings-arc is chosen (it's arbitrary
				// because the arc start and end points are antipodal to each other), but it
				// doesn't matter because we're only interested the parity which should be the same
				// regardless of which arc is chosen.
				d_bounds_data.d_parity_of_num_edges_crossed_from_polygon_centroid_to_antipodal =
						(1 & GPlatesMaths::PointInPolygon::get_num_polygon_edges_crossed(
								all_polygon_edges.begin(), all_polygon_edges.end(),
								d_polygon_centroid_antipodal,
								-d_polygon_centroid_antipodal,
								-1.0/*dot product polygon centroid with its antipodal point*/));

				return;
			}

			// Build the spherical lune tree.
			TreeBuilder tree_builder(*polygon, d_polygon_centroid_antipodal);
			tree_builder.build_tree();

			// Copy the built tree structures - copying also has the advantage of removing
			// any excess memory usage caused by std::vector.
			d_tree_data = tree_builder.d_tree_data;

			// Also copy the polygon bounds data built by the tree builder.
			d_bounds_data = tree_builder.d_bounds_data;
		}


		unsigned int
		SphericalLuneTree::get_num_polygon_edges_crossed(
				const UnitVector3D &test_point) const
		{
			//
			// First determine if we can get an early result to avoid testing the polygon edges.
			//

			const double test_point_dot_polygon_centroid_antipodal =
					dot(d_polygon_centroid_antipodal, test_point).dval();
			// See if the test point is clearly outside the polygon.
			if (test_point_dot_polygon_centroid_antipodal >
				d_bounds_data.d_max_dot_poly_edges_with_antipodal_centroid)
			{
				// The test point is outside the polygon so no edges have been crossed
				// from the polygon centroid antipodal point (which is also outside the polygon)
				// to the test point.
				return 0;
			}
			// See if the test point is closer to the polygon centroid than any polygon
			// edge (this is like a clearly-inside test except the centroid might not be
			// inside the polygon so we'll take the result of the centroid test whether
			// that's inside or outside).
			if (test_point_dot_polygon_centroid_antipodal <
				d_bounds_data.d_min_dot_poly_edges_with_antipodal_centroid)
			{
				return d_bounds_data.d_parity_of_num_edges_crossed_from_polygon_centroid_to_antipodal;
			}

			// If it's more efficient to bypass the spherical lune tree then simply
			// test all the edges of the polygon. We still have the advantage of not having
			// to calculate the polygon centroid for each point tested.
			if (!d_tree_data)
			{
				const UnitVector3D &crossings_arc_start_point = d_polygon_centroid_antipodal;
				const UnitVector3D &crossings_arc_end_point = test_point;

				const UnitVector3D crossings_arc_plane_normal = get_crossings_arc_plane_normal(
						crossings_arc_start_point, crossings_arc_end_point);

				const double dot_crossings_arc_end_points =
						dot(crossings_arc_start_point, crossings_arc_end_point).dval();

				return GPlatesMaths::PointInPolygon::get_num_polygon_edges_crossed(
							d_polygon->begin(), d_polygon->end(),
							crossings_arc_start_point, crossings_arc_end_point,
							crossings_arc_plane_normal, dot_crossings_arc_end_points);
			}

			// Start by recursing into the root node.
			return get_num_polygon_edges_crossed(
					d_tree_data->d_internal_nodes[d_tree_data->d_root_node_index],
					test_point);
		}


		unsigned int
		SphericalLuneTree::get_num_polygon_edges_crossed(
				const InternalNode &node,
				const UnitVector3D &test_point) const
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
				return get_num_polygon_edges_crossed(
						d_tree_data->d_internal_nodes[child_node_index],
						test_point);
			}

			// We've reached a leaf node...
			const LeafNode &leaf_node = d_tree_data->d_leaf_nodes[child_node_index];

			// First determine if we can get an early result to avoid testing the polygon edges.
			const double test_point_dot_polygon_centroid_antipodal =
					dot(d_polygon_centroid_antipodal, test_point).dval();
			// See if the test point is clearly outside the polygon.
			if (test_point_dot_polygon_centroid_antipodal >
				leaf_node.max_dot_product_of_poly_edges_to_centroid_antipodal)
			{
				// The test point is outside the polygon so no edges have been crossed
				// from the polygon centroid antipodal point (which is also outside the polygon)
				// to the test point.
				return 0;
			}
			// See if the test point is closer to the polygon centroid than any polygon
			// edge in the current leaf node (this is like a clearly-inside test except
			// the centroid might not be inside the polygon so we'll take the result of
			// the centroid test whether that's inside or outside).
			if (test_point_dot_polygon_centroid_antipodal <
				leaf_node.min_dot_product_of_poly_edges_to_centroid_antipodal)
			{
				return d_bounds_data.d_parity_of_num_edges_crossed_from_polygon_centroid_to_antipodal;
			}

			// Get the iterator range of edge sequences referenced by the leaf node.
			const edge_sequence_list_type::const_iterator edge_sequence_list_begin =
					d_tree_data->d_edge_sequences.begin() + leaf_node.edge_sequences_start_index;
			const edge_sequence_list_type::const_iterator edge_sequence_list_end =
					edge_sequence_list_begin + leaf_node.num_edge_sequences;

			// Return the number of polygon edges crossed in the subset of edges
			// associated with the leaf node.
			return GPlatesMaths::PointInPolygon::get_num_polygon_edges_crossed(
					edge_sequence_list_begin,
					edge_sequence_list_end,
					d_polygon_centroid_antipodal,
					test_point,
					test_point_dot_polygon_centroid_antipodal);
		}


		// '1e-4' represents an angular deviation of 0.34 minutes away from the plane.
		// The spherical wedge is expanded slightly to include polygon edges that are
		// near the wedge (to protect against numerical precision issues).
		const double SphericalLuneTree::TreeBuilder::SPHERICAL_WEDGE_PLANE_EPSILON = 1e-4;

		SphericalLuneTree::TreeBuilder::TreeBuilder(
				const PolygonOnSphere &polygon,
				const UnitVector3D &polygon_centroid_antipodal) :
			d_polygon(polygon),
			d_polygon_centroid_antipodal(polygon_centroid_antipodal),
			d_polygon_centroid(-polygon_centroid_antipodal)
		{
			const unsigned int num_edges = polygon.number_of_segments();
			const double num_leaf_nodes = double(num_edges) / AVERAGE_NUM_EDGES_PER_LEAF_NODE;
			// The tree depth is less log2(num_leaf_nodes) rounded up.
			// For example if 'num_leaf_nodes' is 65 then 'max_tree_depth' is 7 which supports
			// 128 leaf nodes but since leaf nodes cannot have more than
			// 'AVERAGE_NUM_EDGES_PER_LEAF_NODE' edges the tree should not be full (of nodes).
			d_max_tree_depth = static_cast<unsigned int>(
					std::log(num_leaf_nodes) / std::log(2.0) + 1 - 1e-6);

			const edge_sequence_list_type all_polygon_edges(1,
					EdgeSequence(d_polygon.begin(), d_polygon.end()));

			// Get the number of edges crossed from polygon centroid antipodal to polygon centroid.
			// This will vary depending on which arbitrary crossings-arc is chosen (it's arbitrary
			// because the arc start and end points are antipodal to each other), but it
			// doesn't matter because we're only interested the parity which should be the same
			// regardless of which arc is chosen.
			d_bounds_data.d_parity_of_num_edges_crossed_from_polygon_centroid_to_antipodal =
					(1 & GPlatesMaths::PointInPolygon::get_num_polygon_edges_crossed(
							all_polygon_edges.begin(), all_polygon_edges.end(),
							d_polygon_centroid_antipodal,
							d_polygon_centroid,
							-1.0/*dot product polygon centroid with its antipodal point*/));
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

			const edge_sequence_list_type all_polygon_edges(1,
					EdgeSequence(d_polygon.begin(), d_polygon.end()));

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
			std::pair< double/*min dot product*/, double/*max dot product*/ >
					dot_product_range_of_intersecting_edges_to_polygon_centroid_antipodal =
							get_dot_product_range_of_polygon_edges_to_point(
									sub_tree_edge_sequences,
									d_polygon_centroid_antipodal);
			const double &min_dot_intersecting_edges_to_polygon_centroid_antipodal =
					dot_product_range_of_intersecting_edges_to_polygon_centroid_antipodal.first;
			const double &max_dot_intersecting_edges_to_polygon_centroid_antipodal =
					dot_product_range_of_intersecting_edges_to_polygon_centroid_antipodal.second;

			// Find the min/max dot product over all polygon edges so far.
			if (min_dot_intersecting_edges_to_polygon_centroid_antipodal <
				d_bounds_data.d_min_dot_poly_edges_with_antipodal_centroid)
			{
				d_bounds_data.d_min_dot_poly_edges_with_antipodal_centroid =
						min_dot_intersecting_edges_to_polygon_centroid_antipodal;
			}
			if (max_dot_intersecting_edges_to_polygon_centroid_antipodal >
				d_bounds_data.d_max_dot_poly_edges_with_antipodal_centroid)
			{
				d_bounds_data.d_max_dot_poly_edges_with_antipodal_centroid =
						max_dot_intersecting_edges_to_polygon_centroid_antipodal;
			}

			// Create a leaf node - it references edge that will be stored in the global list.
			const LeafNode leaf_node(
					d_tree_data.d_edge_sequences.size(),
					sub_tree_edge_sequences.size(),
					min_dot_intersecting_edges_to_polygon_centroid_antipodal,
					max_dot_intersecting_edges_to_polygon_centroid_antipodal);
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
			PolygonOnSphere::const_iterator current_intersecting_edge_sequence_start =
					edge_sequence.begin;

			// Iterate over the polygon edges to be tested for intersection.
			PolygonOnSphere::const_iterator polygon_edge_iter = edge_sequence.begin;
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


GPlatesMaths::PointInPolygon::Result
GPlatesMaths::PointInPolygon::is_point_in_polygon(
		const PointOnSphere &point,
		const PolygonOnSphere::non_null_ptr_to_const_type &polygon_ptr)
{
	const PolygonOnSphere &polygon = *polygon_ptr;

	// Get the point that is antipodal to the polygon centroid.
	// This point is deemed to be outside the polygon (FIXME: which might not be true).
	const UnitVector3D crossings_arc_start_point = -get_polygon_centroid(polygon);

	const UnitVector3D &crossings_arc_end_point = point.position_vector();

	const UnitVector3D crossings_arc_plane_normal = get_crossings_arc_plane_normal(
			crossings_arc_start_point, crossings_arc_end_point);

	const double dot_crossings_arc_end_points =
			dot(crossings_arc_start_point, crossings_arc_end_point).dval();

	const unsigned int num_polygon_edges_crossed = get_num_polygon_edges_crossed(
			polygon.begin(), polygon.end(),
			crossings_arc_start_point, crossings_arc_end_point,
			crossings_arc_plane_normal, dot_crossings_arc_end_points);

	// If number of edges crossed is even then point is outside the polygon.
	return ((num_polygon_edges_crossed & 1) == 0) ? POINT_OUTSIDE_POLYGON : POINT_INSIDE_POLYGON;
}


GPlatesMaths::PointInPolygon::Polygon::Polygon(
		const PolygonOnSphere::non_null_ptr_to_const_type &polygon,
		bool build_ologn_hint) :
	d_spherical_lune_tree(new SphericalLuneTree(polygon, build_ologn_hint))
{
}


GPlatesMaths::PointInPolygon::Polygon::~Polygon()
{
	// boost::scoped_ptr destructor requires complete type
}


GPlatesMaths::PointInPolygon::Result
GPlatesMaths::PointInPolygon::Polygon::is_point_in_polygon(
		const PointOnSphere &test_point) const
{
	// Get the number of polygon edges crossed by querying the O(log(N)) spherical lune tree.
	const unsigned int num_polygon_edges_crossed =
		d_spherical_lune_tree->get_num_polygon_edges_crossed(test_point.position_vector());

	// If number of edges crossed is even then point is outside the polygon.
	return ((num_polygon_edges_crossed & 1) == 0) ? POINT_OUTSIDE_POLYGON : POINT_INSIDE_POLYGON;
}
