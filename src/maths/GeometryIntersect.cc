/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2018 The University of Sydney, Australia
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
#include <functional>
#include <limits>
#include <boost/optional.hpp>

#include "GeometryIntersect.h"

#include "GreatCircleArc.h"
#include "MathsUtils.h"
#include "PolyGreatCircleArcBoundingTree.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


namespace GPlatesMaths
{
	namespace GeometryIntersect
	{
		// Set our thickness threshold to match the maximum length of a zero-length great circle arc.
		//
		// This ensures that if the start and end points of a zero-length segment are separated by the
		// plane of another segment, that both points will still be close enough to the plane that they
		// can touch the segment. This avoids the possibility of incorrectly missing an intersection
		// when a zero-length segment is involved.
		const double THICKNESS_THRESHOLD_COSINE = GreatCircleArc::get_zero_length_threshold_cosine().dval();

		// Base epsilon calculations off a cosine since that usually has the least accuracy for small angles.
		// '1 - 1e-12' in cosine corresponds to a displacement of about 1.4e-6 [=sin(acos(1 - 1e-12))].
		const double THICKNESS_THRESHOLD_SINE = std::sin(std::acos(THICKNESS_THRESHOLD_COSINE));


		// We don't need any special handling of the last segment in a polygon ring, so for polygons we
		// just specify the maximum unsigned integer (so that no segment indices will compare equal with it
		// and activate the special handling).
		// Polylines, on the other hand, do not wraparound from last to first segment and so special
		// handling is needed for the end point of the last segment.
		const unsigned int POLYGON_NEEDS_NO_LAST_SEGMENT_INDEX = (std::numeric_limits<unsigned int>::max)();


		/**
		 * Is the graph empty?
		 */
		bool
		empty(
				Graph &graph)
		{
			return graph.unordered_intersections.empty();
		}


		/**
		 * Clear the graph.
		 */
		void
		clear(
				Graph &graph)
		{
			graph.unordered_intersections.clear();
			graph.geometry1_ordered_intersections.clear();
			graph.geometry2_ordered_intersections.clear();
		}


		/**
		 * Predicate to sort intersections from beginning of the geometry to end.
		 */
		class SortGeometryIntersection :
				public std::binary_function<unsigned int, unsigned int, bool>
		{
		public:

			SortGeometryIntersection(
					const intersection_seq_type &intersections,
					const unsigned int Intersection::*segment_index_ptr,
					const AngularDistance Intersection::*angle_in_segment_ptr) :
				d_intersections(&intersections),
				d_segment_index_ptr(segment_index_ptr),
				d_angle_in_segment_ptr(angle_in_segment_ptr)
			{  }

			bool
			operator()(
					unsigned int lhs,
					unsigned int rhs) const
			{
				const intersection_seq_type &intersections = *d_intersections;

				const Intersection &lhs_intersection = intersections[lhs];
				const Intersection &rhs_intersection = intersections[rhs];

				const unsigned int lhs_segment_index = lhs_intersection.*d_segment_index_ptr;
				const unsigned int rhs_segment_index = rhs_intersection.*d_segment_index_ptr;

				// Sort intersections from low to high indices (of segments) in the geometry.
				if (lhs_segment_index < rhs_segment_index)
				{
					return true;
				}
				if (lhs_segment_index > rhs_segment_index)
				{
					return false;
				}

				const AngularDistance &lhs_angle_in_segment = lhs_intersection.*d_angle_in_segment_ptr;
				const AngularDistance &rhs_angle_in_segment = rhs_intersection.*d_angle_in_segment_ptr;

				// Sort by angle closest to start of segment when both intersections are within the same segment.
				return lhs_angle_in_segment.is_precisely_less_than(rhs_angle_in_segment);
			}

		private:
			const intersection_seq_type *d_intersections;
			const unsigned int Intersection::*d_segment_index_ptr;
			const AngularDistance Intersection::*d_angle_in_segment_ptr;
		};

		/**
		 * For each of the two geometries, sort its intersection list such that intersections
		 * are ordered from the geometries start to end.
		 */
		void
		sort_geometry_intersections(
				Graph &graph)
		{
			// Sort intersections along geometry1.
			std::sort(
					graph.geometry1_ordered_intersections.begin(),
					graph.geometry1_ordered_intersections.end(),
					SortGeometryIntersection(
							graph.unordered_intersections,
							&Intersection::segment_index1,
							&Intersection::angle_in_segment1));
			// Sort intersections along geometry2.
			std::sort(
					graph.geometry2_ordered_intersections.begin(),
					graph.geometry2_ordered_intersections.end(),
					SortGeometryIntersection(
							graph.unordered_intersections,
							&Intersection::segment_index2,
							&Intersection::angle_in_segment2));
		}


		/**
		 * Add an @a Intersection to the graph.
		 */
		void
		add_intersection(
				Graph &graph,
				Intersection::Type intersection_type,
				const UnitVector3D &intersection_position,
				const UnitVector3D &segment1_start_point,
				const UnitVector3D &segment2_start_point,
				unsigned int segment1_index,
				unsigned int segment2_index)
		{
			// Calculate the angle from start point to intersection point in each segment.
			AngularDistance angle_in_segment1(AngularDistance::ZERO);
			AngularDistance angle_in_segment2(AngularDistance::ZERO);
			if (intersection_type == Intersection::SEGMENTS_CROSS)
			{
				angle_in_segment1 = AngularDistance::create_from_cosine(
						dot(intersection_position, segment1_start_point));
				angle_in_segment2 = AngularDistance::create_from_cosine(
						dot(intersection_position, segment2_start_point));
			}
			else if (intersection_type == Intersection::SEGMENT1_START_ON_SEGMENT2)
			{
				angle_in_segment2 = AngularDistance::create_from_cosine(
						dot(intersection_position, segment2_start_point));
			}
			else if (intersection_type == Intersection::SEGMENT2_START_ON_SEGMENT1)
			{
				angle_in_segment1 = AngularDistance::create_from_cosine(
						dot(intersection_position, segment1_start_point));
			}
			// else Intersection::SEGMENT_START_ON_SEGMENT_START ...

			const Intersection intersection(
					intersection_type,
					PointOnSphere(intersection_position),
					segment1_index, segment2_index,
					angle_in_segment1, angle_in_segment2);

			// Add the intersection.
			const unsigned unordered_intersection_index = graph.unordered_intersections.size();
			graph.unordered_intersections.push_back(intersection);

			// Also keep track of the intersection for each geometry.
			graph.geometry1_ordered_intersections.push_back(unordered_intersection_index);
			graph.geometry2_ordered_intersections.push_back(unordered_intersection_index);
		}


		/**
		 * Two non-zero-length segments cross each other's *thick* plane - find and add the intersection.
		 */
		void
		add_segments_crossing_intersection(
				Graph &graph,
				const UnitVector3D &segment1_start_point,
				const UnitVector3D &segment1_end_point,
				const UnitVector3D &segment1_plane,
				const UnitVector3D &segment2_start_point,
				const UnitVector3D &segment2_end_point,
				const UnitVector3D &segment2_plane,
				bool segment1_start_point_on_positive_side_of_segment2_plane,
				const double &segment1_start_point_dot_segment2_plane,
				const double &segment1_end_point_dot_segment2_plane,
				const double &segment2_start_point_dot_segment1_plane,
				const double &segment2_end_point_dot_segment1_plane,
				unsigned int segment1_index,
				unsigned int segment2_index)
		{
			const Vector3D cross_segment_planes = cross(segment1_plane, segment2_plane);

			// If both segments are *not* on the same *thick* great circle - this is the most common case.
			if (cross_segment_planes.magSqrd() > 0)
			{
				const UnitVector3D normalised_cross_segment_planes = cross_segment_planes.get_normalisation();

				// We must choose between the two possible antipodal cross product directions based
				// on the orientation of the segments relative to each other.
				const UnitVector3D intersection = segment1_start_point_on_positive_side_of_segment2_plane
						? normalised_cross_segment_planes
						: -normalised_cross_segment_planes;

				add_intersection(
						graph,
						Intersection::SEGMENTS_CROSS,
						intersection,
						segment1_start_point, segment2_start_point,
						segment1_index, segment2_index);

				return;
			}
			// else both segments are pretty much on the same great circle...

			//
			// Both segments have the same (or opposite) rotation axis (within numerical tolerance).
			//
			// Due to the precondition that the start and end points of each segment not lie *on* the
			// *thick* plane of the other segment, we probably only get here when both segments are
			// quite long, otherwise there would be a large enough angle between their great circle
			// planes to avoid getting here in the first place.
			//
			// We use the signed distances of one segment's start and end points from the other segment's plane
			// to interpolate along the straight line joining the first segment's start and end points.
			// That interpolated position along the line will be less than unit-length (from origin)
			// so we then normalise it to obtain the intersection point on the unit-radius globe.
			// The interpolation ratio is:
			//
			//                 signed_distance_to_start_point
			//   -------------------------------------------------------------
			//   signed_distance_to_start_point - signed_distance_to_end_point
			//
			// ...where the negative sign is because the start and end points are on opposite sides of the plane.
			//

			// Make sure segment1's start and end points are not close to being antipodal.
			const Vector3D segment1_mid_point_unnormalised = Vector3D(segment1_start_point) + Vector3D(segment1_end_point);
			// NOTE: We're avoiding the more expensive square-root calculation here.
			if (segment1_mid_point_unnormalised.magSqrd().dval() > 1e-6 /*equivalent to a magnitude of 1e-3*/)
			{
				// The denominator of the ratios used to interpolate segment1's start and end points.
				//
				// Note: We should not get a divide-by-zero here because the denominator should satisfy...
				//
				//   abs(denom) >= 2 * THICKNESS_THRESHOLD_SINE
				//
				// ...since segment1's start and end points should be on opposite sides of
				// segment2's plane by a distance of at least THICKNESS_THRESHOLD_SINE.
				const double denom = segment1_start_point_dot_segment2_plane - segment1_end_point_dot_segment2_plane;
				const double inv_denom = 1.0 / denom;

				const UnitVector3D intersection =
						((segment1_start_point_dot_segment2_plane * inv_denom) * segment1_end_point -
						(segment1_end_point_dot_segment2_plane * inv_denom) * segment1_start_point)
					.get_normalisation();

				add_intersection(
						graph,
						Intersection::SEGMENTS_CROSS,
						intersection,
						segment1_start_point, segment2_start_point,
						segment1_index, segment2_index);

				return;
			}

			// Segment1's start and end points are close to being antipodal (shouldn't be able to
			// get a segment that spans a full half-circle but it can happen within numerical tolerance).
			// Because the points are antipodal the absolute value of their signed distances from
			// any plane (passing through origin) will always be equal regardless of the orientation
			// of the splitting plane. This means the intersection cannot be calculated using
			// signed distance ratios.
			//
			// So instead we swap and try comparing segment2's start and end points to segment1's plane.

			// Make sure segment2's start and end points are not close to being antipodal.
			const Vector3D segment2_mid_point_unnormalised = Vector3D(segment2_start_point) + Vector3D(segment2_end_point);
			// NOTE: We're avoiding the more expensive square-root calculation here.
			if (segment2_mid_point_unnormalised.magSqrd().dval() > 1e-6 /*equivalent to a magnitude of 1e-3*/)
			{
				// The denominator of the ratios used to interpolate segment2's start and end points.
				//
				// Note: We should not get a divide-by-zero here because the denominator should satisfy...
				//
				//   abs(denom) >= 2 * THICKNESS_THRESHOLD_SINE
				//
				// ...since segment2's start and end points should be on opposite sides of
				// segment1's plane by a distance of at least THICKNESS_THRESHOLD_SINE.
				const double denom = segment2_start_point_dot_segment1_plane - segment2_end_point_dot_segment1_plane;
				const double inv_denom = 1.0 / denom;

				const UnitVector3D intersection =
						((segment2_start_point_dot_segment1_plane * inv_denom) * segment2_end_point -
						(segment2_end_point_dot_segment1_plane * inv_denom) * segment2_start_point)
					.get_normalisation();

				add_intersection(
						graph,
						Intersection::SEGMENTS_CROSS,
						intersection,
						segment1_start_point, segment2_start_point,
						segment1_index, segment2_index);

				return;
			}

			// Both segments are (pretty close to) half great circles. This scenario is extremely
			// unlikely (both segments being half circles *and* on the same great circle plane).
			//
			// The solution is to divide one of the half-circle segments into two segments of
			// equal length and determine the intersection from those. We arbitrarily divide segment1.

			// We cannot normalize the segment1's mid-point vector but since segment1 is (close to)
			// a half circle we can use a cross product to find its mid-point.
			const UnitVector3D segment1_mid_point(cross(segment1_plane, segment1_start_point));

			// Get signed distance of segment1's mid-point from segment2's plane.
			const double segment1_mid_point_dot_segment2_plane = dot(segment1_mid_point, segment2_plane).dval();
			const bool segment1_mid_point_on_positive_side_of_segment2_plane = (segment1_mid_point_dot_segment2_plane > 0);

			// If the first half segment of segment1 crosses segment2's plane.
			if (segment1_start_point_on_positive_side_of_segment2_plane ^
				segment1_mid_point_on_positive_side_of_segment2_plane)
			{
				// The denominator of the ratios used to interpolate segment1's start and *mid* points.
				//
				// Note: We should not get a divide-by-zero here because the denominator should satisfy...
				//
				//   abs(denom) >= THICKNESS_THRESHOLD_SINE
				//
				// ...since the distance of segment1's start point from segment2's plane should be at least
				// THICKNESS_THRESHOLD_SINE (even though segment1's mid-point could be closer to segment2's plane),
				// noting that segment1's start and mid points are on opposite sides of segment2's plane.
				const double denom = segment1_start_point_dot_segment2_plane - segment1_mid_point_dot_segment2_plane;
				const double inv_denom = 1.0 / denom;

				const UnitVector3D intersection =
						((segment1_start_point_dot_segment2_plane * inv_denom) * segment1_mid_point -
						(segment1_mid_point_dot_segment2_plane * inv_denom) * segment1_start_point)
					.get_normalisation();

				add_intersection(
						graph,
						Intersection::SEGMENTS_CROSS,
						intersection,
						segment1_start_point, segment2_start_point,
						segment1_index, segment2_index);

				return;
			}
			// ...else the second half segment of segment1 must cross segment2's plane
			// (we know this because a precondition to this function is segment1 must cross segment2's plane).

			// The denominator of the ratios used to interpolate segment1's *mid* and end points.
			//
			// Note: We should not get a divide-by-zero here because the denominator should satisfy...
			//
			//   abs(denom) >= THICKNESS_THRESHOLD_SINE
			//
			// ...since the distance of segment1's end point from segment2's plane should be at least
			// THICKNESS_THRESHOLD_SINE (even though segment1's mid-point could be closer to segment2's plane),
			// noting that segment1's mid and end points are on opposite sides of segment2's plane.
			const double denom = segment1_end_point_dot_segment2_plane - segment1_mid_point_dot_segment2_plane;
			const double inv_denom = 1.0 / denom;

			const UnitVector3D intersection =
					((segment1_end_point_dot_segment2_plane * inv_denom) * segment1_mid_point -
					(segment1_mid_point_dot_segment2_plane * inv_denom) * segment1_end_point)
				.get_normalisation();

			add_intersection(
					graph,
					Intersection::SEGMENTS_CROSS,
					intersection,
					segment1_start_point, segment2_start_point,
					segment1_index, segment2_index);
		}


		/**
		 * Returns true if the specified point lies within the lune of the specified segment.
		 *
		 * A lune is the surface of the globe in the wedge region of space formed by two planes
		 * (great circles) that touch a segment's start and end points and are perpendicular to the segment.
		 *
		 * A point is in the lune if the segment's start and end points are on opposite sides of the
		 * dividing plane (passing through the point and perpendicular to the segment) *and* the
		 * segment's start point is on the positive side of the dividing plane.
		 */
		bool
		point_is_in_segment_lune(
				const UnitVector3D &point,
				const UnitVector3D &segment_plane,
				const UnitVector3D &segment_start_point,
				const UnitVector3D &segment_end_point)
		{
			const Vector3D point_cross_segment_plane = cross(point, segment_plane);

			return dot(point_cross_segment_plane, segment_start_point).dval() >= 0 &&
					dot(point_cross_segment_plane, segment_end_point).dval() <= 0;
		}


		/**
		 * See if two segments cross or touch each other's *thick* plane - if so, find and add the intersection(s).
		 *
		 * If both segments cross each other's *thick* plane then there is one intersection.
		 * Alternatively, two segments can overlap fully or partially, in which case if the start point of one segment
		 * is *on* the other *thick* segment (but not *on* its end point) then a *touching* intersection is created.
		 */
		void
		intersect_segments(
				Graph &graph,
				const GreatCircleArc &segment1,
				const GreatCircleArc &segment2,
				unsigned int segment1_index,
				unsigned int segment2_index,
				unsigned int last_segment1_index,
				unsigned int last_segment2_index)
		{
			const UnitVector3D &segment1_start_point = segment1.start_point().position_vector();
			const UnitVector3D &segment1_end_point = segment1.end_point().position_vector();

			const UnitVector3D &segment2_start_point = segment2.start_point().position_vector();
			const UnitVector3D &segment2_end_point = segment2.end_point().position_vector();

			//
			// Test if the start vertex of segment1 coincides with the start vertex of segment2.
			//
			// If they coincide, we arbitrarily choose the intersection to be the start vertex of segment1.
			// Both vertices will be coincident within a numerical threshold so the difference in position
			// should be very small.
			//

			//
			// Also note that the test for coincident vertices is exactly *complementary* to the test
			// for non-coincident vertices. In other words, we use...
			//
			//     dot_product >= THICKNESS_THRESHOLD_COSINE
			// 
			// ...for coincident vertices, and...
			//
			//     dot_product < THICKNESS_THRESHOLD_COSINE
			// 
			// ...for non-coincident vertices. This is done consistently throughout this function.
			//

			//
			// A note on finite precision:
			//
			// Normally this part of the code (that tests if the start points of both segments are coincident)
			// would be handled *after* testing whether the two segments *cross* each other (ie, cross *thick* segments),
			// where there's some more code to test if the start point of each segment *touches* the other segment
			// (please see the comment in that section, it covers this coincident start points test).
			//
			// However for robustness in the presence of finite numerical precision we need to test for coincident
			// start points first. This is because the distance-to-point using THICKNESS_THRESHOLD_COSINE and the
			// signed-distance-to-plane using THICKNESS_THRESHOLD_SINE are not guaranteed to give the same results
			// for a point that is right at the threshold distance from both a point and a plane (even though
			// mathematically they should both give exactly the same result). In other words, if we did the
			// distance-to-point test after the signed-distance-to-plane test then the signed-distance-to-plane test
			// might return false and so we'd never do the distance-to-point test but the distance-to-point test could
			// actually return true, which might mean that a start point of segment A is determined to *not* touch
			// segment B yet the end point of A's previous segment *is* found to touch segment B (but it's the same
			// point as A's start point and so should give the same result).
			//
			// To highlight this, the following diagram shows segment 'A' and it's previous segment 'A_prev'...
			//
			//               /     /   
			//   _  _  _  _ /_  _ /_  _
			//             /     /     
			//   A_prev---/--+  /      
			//   _  _  _ /_ /_ x_  _  _
			//          /  /  /        
			//         /  A  /         
			//
			// ...where the *thick* planes are shown around both segments. Both segments share the vertex '+'.
			// Any point is considered coincident with the vertex '+' if the point lies within the rhombus
			// surrounding the '+' (it should actually be a circle but that's hard to draw with ascii art).
			// Let 'x' represent the start vertex of segment B (note we haven't drawn the full segment B).
			// Assume that 'x' is just outside segment A's *thick* plane and therefore fails the
			// signed-distance-to-plane test, but assume the distance-to-point '+' test would succeed
			// (due to finite precision issues) if it was tested. So, if the distance-to-point '+' test is
			// after the signed-distance-to-plane test (ie, does not get executed in this case) then 'x'
			// would not be coincident with A's start vertex '+'. However, when testing 'x' against
			// segment 'A_prev' it is clearly inside A_prev's *thick* plane, in which case we then do
			// the distance-to-point '+' test, which succeeds. Now we've got the situation where 'x' is *on*
			// A's start vertex '+', but 'x' is *not* on A_prev's end vertex '+' (which is contradictory).
			// This could manifest as segment B tunneling through 'A' and 'A_prev' without an intersection
			// getting detected. So the solution is to perform the distance-to-point '+' test *before*
			// the signed-distance-to-plane test.
			//
			// On a related note, it's possible the start points of segment1 and segment2 are coincident
			// and so we proceed to see if they *cross* each other's *thick* planes. This is very similar
			// to the above issue and, like the above issue, the distance-to-start-point test succeeded,
			// which would normally mean the signed-distance-to-plane test would also place the point *on*
			// the *thick* plane, but it might not (due to finite precision issues). So if it's not *on*
			// the *thick* plane then it means it can *cross* the *thick* plane and we could possibly
			// get a *crossing* intersection (in addition to the start point *touching* intersection).
			// However, if this does happen then it doesn't cause our intersection logic to fail like the
			// above tunneling situation, in this case we just get an extra (possibly unwanted) intersection,
			// but it's extremely unlikely to happen, so should be quite rare in practice.
			//

			if (dot(segment1_start_point, segment2_start_point).dval() >= THICKNESS_THRESHOLD_COSINE)
			{
				// Segment1's start point is *on* segment2's start point (and vice versa).
				//
				// If either segment's start point is on the other segment's end point then let that
				// other segment's *next* segment generate the intersection (at its start point).
				// Otherwise we have an intersection.
				if (dot(segment1_start_point, segment2_end_point).dval() < THICKNESS_THRESHOLD_COSINE &&
					dot(segment2_start_point, segment1_end_point).dval() < THICKNESS_THRESHOLD_COSINE)
				{
					add_intersection(
							graph,
							Intersection::SEGMENT_START_ON_SEGMENT_START,
							segment1_start_point, // intersection
							segment1_start_point, segment2_start_point,
							segment1_index, segment2_index);
				}
			}

			//
			// Handle special cases where the *last* vertex in either geometry touches any vertex of the
			// other geometry (also including the case where the *last* vertices of both geometries touch).
			//
			// Note that these special cases only apply to polylines (not polygons, which wraparound in rings).
			// For polygons, the last segment index is the maximum possible integer 'POLYGON_NEEDS_NO_LAST_SEGMENT_INDEX',
			// so the 'if' statements below are always false.
			//
			// These special cases are handled in a similar manner to if we had instead explicitly generated
			// fictitious one-past-the-last segments and thrown those segments into the mix (ie, if we had
			// called this function with combinations of those segments and regular nearby segments).
			// In other words, the logic below is the same logic, just included here instead since it's easier
			// to implement it that way.
			// The equivalence of the code below (to explicitly generated fictitious segments) is:
			// 
			//  - The first 'if (segment1_index == last_segment1_index)' represents testing (the start of) the fictitious
			//    one-past-the-last segment of geometry1 with (the start of) any non-fictitious segment of geometry2.
			//  - The second 'if (segment1_index == last_segment1_index)' represents testing (the start of) the fictitious
			//    one-past-the-last segment of geometry2 with (the start of) any non-fictitious segment of geometry1.
			//  - The third 'if (segment1_index == last_segment1_index && segment2_index == last_segment2_index)'
			//    represents testing (the start of) the fictitious one-past-the-last segment of geometry1 with
			//    (the start of) the fictitious one-past-the-last segment of geometry2.
			// 
			// As an example of this logic, assume this function is testing the last segments of both geometries
			// (this function is never called with fictitious segments; that's why we have the code below, to emulate that).
			// And assume the last vertex of geometry1 is coincident with both the last and second-last vertices of geometry2
			// (ie, the start and end points of its zero-length last segment). Using the above logic, an intersection
			// is only generated by the combination of the fictitious one-past-the-last segments of both geometries
			// (ie, the third 'if' statement below). The other two segment combinations (ie, the other two 'if' statements)
			// should fall through using complementary dot product tests.
			//

			if (segment1_index == last_segment1_index)
			{
				if (dot(segment1_end_point, segment2_start_point).dval() >= THICKNESS_THRESHOLD_COSINE)
				{
					// Segment1's end point is *on* segment2's start point.
					//
					// If it's also *on* segment2's end point then let segment2's *next* segment generate
					// the intersection (at its start point).
					// Otherwise we have an intersection.
					if (dot(segment1_end_point, segment2_end_point).dval() < THICKNESS_THRESHOLD_COSINE)
					{
						add_intersection(
								graph,
								Intersection::SEGMENT_START_ON_SEGMENT_START,
								segment1_end_point, // intersection
								segment1_start_point, segment2_start_point,
								// NOTE: Segment index of geometry1 is its 'number of segments'.
								//       This is the fictitious one-past-the-last-segment...
								segment1_index + 1, segment2_index);
					}
				}
			}

			if (segment2_index == last_segment2_index)
			{
				if (dot(segment2_end_point, segment1_start_point).dval() >= THICKNESS_THRESHOLD_COSINE)
				{
					// Segment2's end point is *on* segment1's start point.
					//
					// If it's also *on* segment1's end point then let segment1's *next* segment generate
					// the intersection (at its start point).
					// Otherwise we have an intersection.
					if (dot(segment2_end_point, segment1_end_point).dval() < THICKNESS_THRESHOLD_COSINE)
					{
						add_intersection(
								graph,
								Intersection::SEGMENT_START_ON_SEGMENT_START,
								segment2_end_point, // intersection
								segment1_start_point, segment2_start_point,
								// NOTE: Segment index of geometry2 is its 'number of segments'.
								//       This is the fictitious one-past-the-last-segment...
								segment1_index, segment2_index + 1);
					}
				}
			}

			// Handle the doubly-special case where the *last* vertices of both geometries coincide with each other.
			if (segment1_index == last_segment1_index &&
				segment2_index == last_segment2_index)
			{
				if (dot(segment1_end_point, segment2_end_point).dval() >= THICKNESS_THRESHOLD_COSINE)
				{
					// Segment1's end point is *on* segment2's end point (and vice versa).
					add_intersection(
							graph,
							Intersection::SEGMENT_START_ON_SEGMENT_START,
							segment1_end_point, // intersection
							segment1_start_point, segment2_start_point,
							// NOTE: Segment indices of both geometries are their 'number of segments'.
							//       These are the fictitious one-past-the-last-segments...
							segment1_index + 1, segment2_index + 1);
				}
			}


			// Each segment only has a plane if it is not zero length (ie, has a rotation axis).
			const bool segment1_has_plane = !segment1.is_zero_length();
			const bool segment2_has_plane = !segment2.is_zero_length();

			//
			// The great circle plane of each segment is a "thick" plane in that it has an
			// epsilon thickness to account for issues with finite numerical precision.
			//
			// So each start/end point of each segment is classified as:
			//  - in the *positive* half-space of other segment's great circle plane, or
			//  - in the *negative* half-space of other segment's great circle plane, or
			//  - *on* the other segment's great circle plane.
			//

			boost::optional<UnitVector3D> segment1_plane;

			double segment2_start_point_dot_segment1_plane;
			bool segment2_start_point_on_positive_side_of_segment1_plane;
			bool segment2_start_point_on_negative_side_of_segment1_plane;

			double segment2_end_point_dot_segment1_plane;
			bool segment2_end_point_on_positive_side_of_segment1_plane;
			bool segment2_end_point_on_negative_side_of_segment1_plane;

			if (segment1_has_plane)
			{
				segment1_plane = segment1.rotation_axis();

				segment2_start_point_dot_segment1_plane = dot(segment2_start_point, segment1_plane.get()).dval();
				segment2_start_point_on_positive_side_of_segment1_plane =
						(segment2_start_point_dot_segment1_plane > THICKNESS_THRESHOLD_SINE);
				segment2_start_point_on_negative_side_of_segment1_plane =
						(segment2_start_point_dot_segment1_plane < -THICKNESS_THRESHOLD_SINE);

				segment2_end_point_dot_segment1_plane = dot(segment2_end_point, segment1_plane.get()).dval();
				segment2_end_point_on_positive_side_of_segment1_plane =
						segment2_end_point_dot_segment1_plane > THICKNESS_THRESHOLD_SINE;
				segment2_end_point_on_negative_side_of_segment1_plane =
						segment2_end_point_dot_segment1_plane < -THICKNESS_THRESHOLD_SINE;
			}
			else // segment1 is zero length ...
			{
				// These variables will be unused, but initialise with dummy values to avoid
				// compiler warnings/errors about potentially using un-initialised variables.

				segment2_start_point_dot_segment1_plane = 0.0;
				segment2_start_point_on_positive_side_of_segment1_plane = false;
				segment2_start_point_on_negative_side_of_segment1_plane = false;

				segment2_end_point_dot_segment1_plane = 0.0;
				segment2_end_point_on_positive_side_of_segment1_plane = false;
				segment2_end_point_on_negative_side_of_segment1_plane = false;
			}

			boost::optional<UnitVector3D> segment2_plane;

			double segment1_start_point_dot_segment2_plane;
			bool segment1_start_point_on_positive_side_of_segment2_plane;
			bool segment1_start_point_on_negative_side_of_segment2_plane;

			double segment1_end_point_dot_segment2_plane;
			bool segment1_end_point_on_positive_side_of_segment2_plane;
			bool segment1_end_point_on_negative_side_of_segment2_plane;

			if (segment2_has_plane)
			{
				segment2_plane = segment2.rotation_axis();

				segment1_start_point_dot_segment2_plane = dot(segment1_start_point, segment2_plane.get()).dval();
				segment1_start_point_on_positive_side_of_segment2_plane =
						(segment1_start_point_dot_segment2_plane > THICKNESS_THRESHOLD_SINE);
				segment1_start_point_on_negative_side_of_segment2_plane =
						(segment1_start_point_dot_segment2_plane < -THICKNESS_THRESHOLD_SINE);

				segment1_end_point_dot_segment2_plane = dot(segment1_end_point, segment2_plane.get()).dval();
				segment1_end_point_on_positive_side_of_segment2_plane =
						segment1_end_point_dot_segment2_plane > THICKNESS_THRESHOLD_SINE;
				segment1_end_point_on_negative_side_of_segment2_plane =
						segment1_end_point_dot_segment2_plane < -THICKNESS_THRESHOLD_SINE;
			}
			else // segment2 is zero length ...
			{
				// These variables will be unused, but initialise with dummy values to avoid
				// compiler warnings/errors about potentially using un-initialised variables.

				segment1_start_point_dot_segment2_plane = 0.0;
				segment1_start_point_on_positive_side_of_segment2_plane = false;
				segment1_start_point_on_negative_side_of_segment2_plane = false;

				segment1_end_point_dot_segment2_plane = 0.0;
				segment1_end_point_on_positive_side_of_segment2_plane = false;
				segment1_end_point_on_negative_side_of_segment2_plane = false;
			}

			//
			// If one (or both) segment is zero length then it cannot *cross* the other segment's *thick* plane
			// because the maximum length of a zero length segment (which can actually be non-zero within a threshold)
			// is less than the thickness of the plane (2 * THICKNESS_THRESHOLD_SINE) and hence its start and end
			// points cannot be on opposite sides of the *thick* plane.
			//
			// So we only need to test for "crossing" if both segments are non-zero length.
			//
			if (segment1_has_plane &&
				segment2_has_plane)
			{
				//
				// Two segments *cross* if the end points of one segment are in opposite half-spaces of the plane of
				// the other segment (and vice versa) and the start (or end) point of one segment is in the positive
				// half-space of the other segment (and the opposite true for the other segment).
				//

				if (segment1_start_point_on_positive_side_of_segment2_plane)
				{
					if (segment1_end_point_on_positive_side_of_segment2_plane)
					{
						// Segment1 on positive side of segment2's plane.
						return; // No intersection.
					}
					else if (segment1_end_point_on_negative_side_of_segment2_plane)
					{
						// Segment1 crosses segment2's plane.

						if (segment2_start_point_on_positive_side_of_segment1_plane)
						{
							// Start points of both segments are on positive side of other segment's plane.
							return; // No intersection.
						}
						else if (segment2_start_point_on_negative_side_of_segment1_plane)
						{
							if (segment2_end_point_on_negative_side_of_segment1_plane)
							{
								// Segment2 on negative side of segment1's plane.
								return; // No intersection.
							}
							else if (segment2_end_point_on_positive_side_of_segment1_plane)
							{
								// Both segments cross the other segment's plane, hence they intersect.
								add_segments_crossing_intersection(
										graph,
										segment1_start_point, segment1_end_point, segment1_plane.get(),
										segment2_start_point, segment2_end_point, segment2_plane.get(),
										segment1_start_point_on_positive_side_of_segment2_plane,
										segment1_start_point_dot_segment2_plane, segment1_end_point_dot_segment2_plane,
										segment2_start_point_dot_segment1_plane, segment2_end_point_dot_segment1_plane,
										segment1_index, segment2_index);

								// Only one intersection possible when both segments cross each other.
								return;
							}
							// else segment2 end point is *on* segment1's plane ...
						}
						// else segment2 start point is *on* segment1's plane ...
					}
					// else segment1 end point is *on* segment2's plane ...
				}
				else if (segment1_start_point_on_negative_side_of_segment2_plane)
				{
					if (segment1_end_point_on_negative_side_of_segment2_plane)
					{
						// Segment1 on negative side of segment2's plane.
						return; // No intersection.
					}
					else if (segment1_end_point_on_positive_side_of_segment2_plane)
					{
						// Segment1 crosses segment2's plane.

						if (segment2_start_point_on_negative_side_of_segment1_plane)
						{
							// Start points of both segments are on negative side of other segment's plane.
							return; // No intersection.
						}
						else if (segment2_start_point_on_positive_side_of_segment1_plane)
						{
							if (segment2_end_point_on_positive_side_of_segment1_plane)
							{
								// Segment2 on positive side of segment1's plane.
								return; // No intersection.
							}
							else if (segment2_end_point_on_negative_side_of_segment1_plane)
							{
								// Both segments cross the other segment's plane, hence they intersect.
								add_segments_crossing_intersection(
										graph,
										segment1_start_point, segment1_end_point, segment1_plane.get(),
										segment2_start_point, segment2_end_point, segment2_plane.get(),
										segment1_start_point_on_positive_side_of_segment2_plane,
										segment1_start_point_dot_segment2_plane, segment1_end_point_dot_segment2_plane,
										segment2_start_point_dot_segment1_plane, segment2_end_point_dot_segment1_plane,
										segment1_index, segment2_index);

								// Only one intersection possible when both segments cross each other.
								return;
							}
							// else segment2 end point is *on* segment1's plane ...
						}
						// else segment2 start point is *on* segment1's plane ...
					}
					// else segment1 end point is *on* segment2's plane ...
				}
				// else segment1 start point is *on* segment2's plane ...
			}

			//
			// If we get here then the two segments do not *cross* each other (ie, don't cross *thick* segments).
			//
			// However the start or end point of one segment (A) can still *touch* the other segment (B)
			// if A's start or end point lies *on* the *thick* plane of segment B and is between
			// segment B's start and end points.
			//
			// However we only record an intersection if the *start* point of one segment (A) is *on*
			// the other segment (B) (and vice versa for segment B's start point *on* segment A).
			// The reason we don't record an intersection when the *end* point of segment A is *on*
			// segment B is because that gets taken care of when segment A's next adjacent segment is
			// tested against segment B (where the *start* point of segment A's next adjacent segment is the
			// *end* point of segment A). Doing this avoids duplicate intersections and simplifies the logic.
			// The issue then becomes what to do for the last segment (since it has no next segment),
			// however that's addressed in a separate large comment block at the end of this function.
			// 
			// By using the exact same floating-point test for start and end points we can ensure that an
			// intersection is not missed when the end point of segment A (and start point of segment A's
			// next segment A_next) touches segment B as in...
			//
			//             ^ B
			//             |
			//  A o------->o-------> A_next
			//             |
			//             o
			//
			// ...in this case segment A does not intersect segment B, but segment A_next does intersect
			// segment B. By using the same floating-point tests we know that if segment A_next's start
			// point moves slightly off segment B (such that it is no longer *on* B's *thick* plane)
			// then segment A should now intersect segment B (which it did not previously) as in...
			//
			//             ^ B
			//             |
			//    A o------+>o-------> A_next
			//             |
			//             o
			//
			// Note that the test for being *on* a segment (excluding its start and end points for now)
			// requires that segment to have a plane (ie, be non-zero length). If a segment doesn't have a
			// plane then we don't test the plane, we only test its start point, but that is taken care
			// of by the start point proximity test at the beginning of this function.
			// Note that a zero length segment can have still a finite length (it's just below a
			// numerical threshold) and hence its start and end points can actually be separate.
			// However we don't need to worry about when its start and end points are on opposite sides
			// of the other segment's plane and hence missing an intersection because its start and/or end
			// point will then be *on* the other segment's plane and hence generate an intersection.
			// Note that the reason the start and/or end point of a zero-length segment will be *on* the plane is
			// because the maximum length of a zero-length segment (which can actually be non-zero within a threshold)
			// is less than the thickness of the plane (2 * THICKNESS_THRESHOLD_SINE).
			//
			//                   ^ B
			//                 A |
			//    A_prev o----->o>o-------> A_next
			//                   |
			//                   o
			//
			// ...where both the start point of zero-length segment A and the start point of segment A_next
			// (end point of segment A) will record an intersection with segment B since both start points
			// (of segments A and A_next) are *on* segment B (because segment A is zero length).
			//
			// For the same reason that we don't record an intersection when the *end* point of segment A is *on*
			// segment B (noted at beginning of this comment), we also don't record an intersection when the
			// *end* point of segment B is *on* the start point of segment A. The reason is that segment B's
			// next adjacent segment (B_next) will instead record the intersection with A since B_next's *start*
			// point (end point of segment B) will then be *on* segment's A start point as in...
			//
			//           B_next ^
			//                  |
			//                  |
			//                  |
			//  A_prev o------->o-------> A
			//                B ^
			//                  |
			//                  |
			//                  |
			//                  o
			//

			if (segment2_has_plane)
			{
				if (!segment1_start_point_on_negative_side_of_segment2_plane &&
					!segment1_start_point_on_positive_side_of_segment2_plane)
				{
					// Segment1 start point is *on* segment2's plane.
					// See if it is *on* segment2 (ie, not just on its plane),
					// but not *on* segment2's start and end points.

					// See if segment1's start point is not *on* segment2's start and end points.
					if (dot(segment1_start_point, segment2_start_point).dval() < THICKNESS_THRESHOLD_COSINE &&
						dot(segment1_start_point, segment2_end_point).dval() < THICKNESS_THRESHOLD_COSINE)
					{
						if (point_is_in_segment_lune(
								segment1_start_point,
								segment2_plane.get(),
								segment2_start_point,
								segment2_end_point))
						{
							add_intersection(
									graph,
									Intersection::SEGMENT1_START_ON_SEGMENT2,
									// Choose the intersection to be the start point of segment1.
									// It can be slightly off segment2 (within a numerical threshold)...
									segment1_start_point, // intersection
									segment1_start_point, segment2_start_point,
									segment1_index, segment2_index);
						}
					}
				}
			}

			if (segment1_has_plane)
			{
				if (!segment2_start_point_on_negative_side_of_segment1_plane &&
					!segment2_start_point_on_positive_side_of_segment1_plane)
				{
					// Segment2 start point is *on* segment1's plane.
					// See if it is *on* segment1 (ie, not just on its plane),
					// but not *on* segment1's start and end points.

					// See if segment2's start point is not *on* segment1's start and end points.
					if (dot(segment2_start_point, segment1_start_point).dval() < THICKNESS_THRESHOLD_COSINE &&
						dot(segment2_start_point, segment1_end_point).dval() < THICKNESS_THRESHOLD_COSINE)
					{
						if (point_is_in_segment_lune(
								segment2_start_point,
								segment1_plane.get(),
								segment1_start_point,
								segment1_end_point))
						{
							add_intersection(
									graph,
									Intersection::SEGMENT2_START_ON_SEGMENT1,
									// Choose the intersection to be the start point of segment2.
									// It can be slightly off segment1 (within a numerical threshold)...
									segment2_start_point, // intersection
									segment1_start_point, segment2_start_point,
									segment1_index, segment2_index);
						}
					}
				}
			}

			//
			// Handle special cases where the *last* vertex in either geometry touches any segment
			// of the other geometry (but not its vertices).
			//
			// Note that these special cases only apply to polylines (not polygons, which wraparound in rings).
			// For polygons, the last segment index is the maximum possible integer 'POLYGON_NEEDS_NO_LAST_SEGMENT_INDEX',
			// so the 'if' statements below are always false.
			//
			// These special cases are handled in a similar manner to if we had instead explicitly generated
			// fictitious one-past-the-last segments and thrown those segments into the mix (ie, if we had
			// called this function with combinations of those segments and regular nearby segments).
			// In other words, the logic below is the same logic, just included here instead since it's easier
			// to implement it that way.
			// The equivalence of the code below (to explicitly generated fictitious segments) is:
			// 
			//  - The first 'if (segment1_index == last_segment1_index)' represents testing (the start of) the fictitious
			//    one-past-the-last segment of geometry1 with any non-fictitious segment of geometry2 (excluding its vertices).
			//  - The second 'if (segment2_index == last_segment2_index)' represents testing (the start of) the fictitious
			//    one-past-the-last segment of geometry2 with any non-fictitious segment of geometry1 (excluding its vertices).
			// 
			// As an example of this logic, assume this function is testing the last segments of both geometries
			// (this function is never called with fictitious segments; that's why we have the code below, to emulate that).
			// And assume the last vertex of geometry1 touches the last segment of geometry2 (excluding its start and end points).
			// Using the above logic, an intersection is generated by the combination of (the start of) the fictitious one-past-the-last
			// segment of geometry1 with the last (non-fictitious) segment of geometry2 (ie, the first 'if' statement below).
			//

			if (segment1_index == last_segment1_index)
			{
				if (segment2_has_plane)
				{
					if (!segment1_end_point_on_negative_side_of_segment2_plane &&
						!segment1_end_point_on_positive_side_of_segment2_plane)
					{
						// See if segment1's end point is not *on* segment2's start and end points.
						if (dot(segment1_end_point, segment2_start_point).dval() < THICKNESS_THRESHOLD_COSINE &&
							dot(segment1_end_point, segment2_end_point).dval() < THICKNESS_THRESHOLD_COSINE)
						{
							// Segment1 end point is *on* segment2's plane.
							// but not *on* segment2's start and end points.
							// See if it is *on* segment2 (ie, not just on its plane),
							if (point_is_in_segment_lune(
									segment1_end_point,
									segment2_plane.get(),
									segment2_start_point,
									segment2_end_point))
							{
								add_intersection(
										graph,
										Intersection::SEGMENT1_START_ON_SEGMENT2,
										// Choose the intersection to be the end point of segment1.
										// It can be slightly off segment2 (within a numerical threshold)...
										segment1_end_point, // intersection
										segment1_start_point, segment2_start_point,
										// NOTE: Segment index of geometry1 is its 'number of segments'.
										//       This is the fictitious one-past-the-last-segment...
										segment1_index + 1, segment2_index);
							}
						}
					}
				}
			}

			if (segment2_index == last_segment2_index)
			{
				if (segment1_has_plane)
				{
					if (!segment2_end_point_on_negative_side_of_segment1_plane &&
						!segment2_end_point_on_positive_side_of_segment1_plane)
					{
						// See if segment2's end point is not *on* segment1's start and end points.
						if (dot(segment2_end_point, segment1_start_point).dval() < THICKNESS_THRESHOLD_COSINE &&
							dot(segment2_end_point, segment1_end_point).dval() < THICKNESS_THRESHOLD_COSINE)
						{
							// Segment2 end point is *on* segment1's plane.
							// but not *on* segment1's start and end points.
							// See if it is *on* segment1 (ie, not just on its plane),
							if (point_is_in_segment_lune(
									segment2_end_point,
									segment1_plane.get(),
									segment1_start_point,
									segment1_end_point))
							{
								add_intersection(
										graph,
										Intersection::SEGMENT2_START_ON_SEGMENT1,
										// Choose the intersection to be the end point of segment2.
										// It can be slightly off segment1 (within a numerical threshold)...
										segment2_end_point, // intersection
										segment1_start_point, segment2_start_point,
										// NOTE: Segment index of geometry2 is its 'number of segments'.
										//       This is the fictitious one-past-the-last-segment...
										segment1_index, segment2_index + 1);
							}
						}
					}
				}
			}
		}


		/**
		 * Find any intersections between a bounding tree node (of segments) of one polyline or polygon,
		 * and the bounding tree node (of segments) of another polyline or polygon.
		 */
		template <typename GreatCircleArcConstIterator1Type, typename GreatCircleArcConstIterator2Type>
		void
		intersect_bounding_tree_nodes(
				Graph &graph,
				const PolyGreatCircleArcBoundingTree<GreatCircleArcConstIterator1Type> &geometry1_bounding_tree,
				const typename PolyGreatCircleArcBoundingTree<GreatCircleArcConstIterator1Type>::node_type &geometry1_sub_tree_node,
				const unsigned int last_segment1_index,
				const PolyGreatCircleArcBoundingTree<GreatCircleArcConstIterator2Type> &geometry2_bounding_tree,
				const typename PolyGreatCircleArcBoundingTree<GreatCircleArcConstIterator2Type>::node_type &geometry2_sub_tree_node,
				const unsigned int last_segment2_index)
		{
			// If the bounding small circles of the nodes of the two geometries don't intersect then return early.
			//
			// Note that the small circle radii have been expanded slightly to account for numerical tolerance.
			// See BoundingSmallCircleBuilder::get_bounding_small_circle().
			//
			// TODO: Ensure that this expansion is as large as the thickness threshold around points and
			// segments used during intersection detection.
			if (!intersect(
					geometry1_sub_tree_node.get_bounding_small_circle(),
					geometry2_sub_tree_node.get_bounding_small_circle()))
			{
				return;
			}

			if (geometry1_sub_tree_node.is_internal_node() &&
				geometry2_sub_tree_node.is_internal_node())
			{
				// Recurse into the largest internal node first.
				// This can result in fewer tests between bounding small circles of sub-tree nodes.
				if (geometry1_sub_tree_node.get_bounding_small_circle().get_angular_extent().is_precisely_greater_than(
					geometry2_sub_tree_node.get_bounding_small_circle().get_angular_extent()))
				{
					// Recurse into the child nodes of the first geometry.
					for (unsigned int n = 0; n < 2; ++n)
					{
						intersect_bounding_tree_nodes(
								graph,
								geometry1_bounding_tree,
								geometry1_bounding_tree.get_child_node(geometry1_sub_tree_node, n),
								last_segment1_index,
								geometry2_bounding_tree,
								geometry2_sub_tree_node,
								last_segment2_index);
					}
				}
				else // second geometry's internal node is larger...
				{
					// Recurse into the child nodes of the second geometry.
					for (unsigned int n = 0; n < 2; ++n)
					{
						intersect_bounding_tree_nodes(
								graph,
								geometry1_bounding_tree,
								geometry1_sub_tree_node,
								last_segment1_index,
								geometry2_bounding_tree,
								geometry2_bounding_tree.get_child_node(geometry2_sub_tree_node, n),
								last_segment2_index);
					}
				}
			}
			else if (geometry1_sub_tree_node.is_internal_node())
			{
				// The first geometry is at an internal node and the second is at a leaf node.

				// Recurse into the child nodes of the first geometry.
				for (unsigned int n = 0; n < 2; ++n)
				{
					intersect_bounding_tree_nodes(
							graph,
							geometry1_bounding_tree,
							geometry1_bounding_tree.get_child_node(geometry1_sub_tree_node, n),
							last_segment1_index,
							geometry2_bounding_tree,
							geometry2_sub_tree_node,
							last_segment2_index);
				}
			}
			else if (geometry2_sub_tree_node.is_internal_node())
			{
				// The first geometry is at a leaf node and the second is at an internal node.

				// Recurse into the child nodes of the second geometry.
				for (unsigned int n = 0; n < 2; ++n)
				{
					intersect_bounding_tree_nodes(
							graph,
							geometry1_bounding_tree,
							geometry1_sub_tree_node,
							last_segment1_index,
							geometry2_bounding_tree,
							geometry2_bounding_tree.get_child_node(geometry2_sub_tree_node, n),
							last_segment2_index);
				}
			}
			else // both geometries are at a leaf node ...
			{
				// Search for possible intersections between the N segments in first geometry's
				// leaf node and the M segments in the second geometry's leaf node.

				typedef PolyGreatCircleArcBoundingTree<GreatCircleArcConstIterator1Type> geometry1_bounding_tree_type;
				typedef PolyGreatCircleArcBoundingTree<GreatCircleArcConstIterator2Type> geometry2_bounding_tree_type;

				// Iterate over the segments of the leaf node of the first geometry.
				typename geometry1_bounding_tree_type::great_circle_arc_const_iterator_type
						gca1_iter = geometry1_sub_tree_node.get_bounded_great_circle_arcs_begin();
				typename geometry1_bounding_tree_type::great_circle_arc_const_iterator_type
						gca1_end = geometry1_sub_tree_node.get_bounded_great_circle_arcs_end();
				unsigned int segment1_index = geometry1_sub_tree_node.get_bounded_great_circle_arcs_begin_index();
				for ( ; gca1_iter != gca1_end; ++gca1_iter, ++segment1_index)
				{
					const GreatCircleArc &segment1 = *gca1_iter;

					// Iterate over the segments of the leaf node of the second geometry.
					typename geometry2_bounding_tree_type::great_circle_arc_const_iterator_type
							segment2_iter = geometry2_sub_tree_node.get_bounded_great_circle_arcs_begin();
					typename geometry2_bounding_tree_type::great_circle_arc_const_iterator_type
							segment2_end = geometry2_sub_tree_node.get_bounded_great_circle_arcs_end();
					unsigned int segment2_index = geometry2_sub_tree_node.get_bounded_great_circle_arcs_begin_index();
					for ( ; segment2_iter != segment2_end; ++segment2_iter, ++segment2_index)
					{
						const GreatCircleArc &segment2 = *segment2_iter;

						intersect_segments(
								graph,
								segment1, segment2,
								segment1_index, segment2_index,
								last_segment1_index, last_segment2_index);
					}
				}
			}
		}


		/**
		 * Find any intersections between two polyline/polygon geometries.
		 */
		template <typename PolyGeometryBoundingTree1Type, typename PolyGeometryBoundingTree2Type>
		bool
		intersect_geometries(
				Graph &graph,
				const PolyGeometryBoundingTree1Type &poly_geometry1_bounding_tree,
				const unsigned int last_segment1_index,
				const PolyGeometryBoundingTree2Type &poly_geometry2_bounding_tree,
				const unsigned int last_segment2_index)
		{
			// Make sure we start with an empty graph.
			clear(graph);

			intersect_bounding_tree_nodes(
					graph,
					poly_geometry1_bounding_tree,
					poly_geometry1_bounding_tree.get_root_node(),
					last_segment1_index,
					poly_geometry2_bounding_tree,
					poly_geometry2_bounding_tree.get_root_node(),
					last_segment2_index);

			if (empty(graph))
			{
				return false;
			}

			sort_geometry_intersections(graph);

			return true;
		}
	}
}


bool
GPlatesMaths::GeometryIntersect::intersect(
		Graph &graph,
		const PolylineOnSphere &polyline1,
		const PolylineOnSphere &polyline2)
{
	return intersect_geometries(
			graph,
			polyline1.get_bounding_tree(),
			polyline1.number_of_segments() - 1 /*last_segment_index*/,
			polyline2.get_bounding_tree(),
			polyline2.number_of_segments() - 1 /*last_segment_index*/);
}


bool
GPlatesMaths::GeometryIntersect::intersect(
		Graph &graph,
		const PolygonOnSphere &polygon1,
		const PolygonOnSphere &polygon2,
		bool include_polygon1_interior_rings,
		bool include_polygon2_interior_rings)
{
	if (include_polygon1_interior_rings)
	{
		if (include_polygon2_interior_rings)
		{
			return intersect_geometries(
					graph,
					polygon1.get_bounding_tree(),
					POLYGON_NEEDS_NO_LAST_SEGMENT_INDEX,
					polygon2.get_bounding_tree(),
					POLYGON_NEEDS_NO_LAST_SEGMENT_INDEX);
		}
		else
		{
			return intersect_geometries(
					graph,
					polygon1.get_bounding_tree(),
					POLYGON_NEEDS_NO_LAST_SEGMENT_INDEX,
					polygon2.get_exterior_ring_bounding_tree(),
					POLYGON_NEEDS_NO_LAST_SEGMENT_INDEX);
		}
	}
	else
	{
		if (include_polygon2_interior_rings)
		{
			return intersect_geometries(
					graph,
					polygon1.get_exterior_ring_bounding_tree(),
					POLYGON_NEEDS_NO_LAST_SEGMENT_INDEX,
					polygon2.get_bounding_tree(),
					POLYGON_NEEDS_NO_LAST_SEGMENT_INDEX);
		}
		else
		{
			return intersect_geometries(
					graph,
					polygon1.get_exterior_ring_bounding_tree(),
					POLYGON_NEEDS_NO_LAST_SEGMENT_INDEX,
					polygon2.get_exterior_ring_bounding_tree(),
					POLYGON_NEEDS_NO_LAST_SEGMENT_INDEX);
		}
	}
}


bool
GPlatesMaths::GeometryIntersect::intersect(
		Graph &graph,
		const PolylineOnSphere &polyline,
		const PolygonOnSphere &polygon,
		bool include_polygon_interior_rings)
{
	if (include_polygon_interior_rings)
	{
		return intersect_geometries(
				graph,
				polyline.get_bounding_tree(),
				polyline.number_of_segments() - 1 /*last_segment_index*/,
				polygon.get_bounding_tree(),
				POLYGON_NEEDS_NO_LAST_SEGMENT_INDEX);
	}
	else
	{
		return intersect_geometries(
				graph,
				polyline.get_bounding_tree(),
				polyline.number_of_segments() - 1 /*last_segment_index*/,
				polygon.get_exterior_ring_bounding_tree(),
				POLYGON_NEEDS_NO_LAST_SEGMENT_INDEX);
	}
}


bool
GPlatesMaths::GeometryIntersect::intersect(
		Graph &graph,
		const PolygonOnSphere &polygon,
		const PolylineOnSphere &polyline,
		bool include_polygon_interior_rings)
{
	if (include_polygon_interior_rings)
	{
		return intersect_geometries(
				graph,
				polygon.get_bounding_tree(),
				POLYGON_NEEDS_NO_LAST_SEGMENT_INDEX,
				polyline.get_bounding_tree(),
				polyline.number_of_segments() - 1 /*last_segment_index*/);
	}
	else
	{
		return intersect_geometries(
				graph,
				polygon.get_exterior_ring_bounding_tree(),
				POLYGON_NEEDS_NO_LAST_SEGMENT_INDEX,
				polyline.get_bounding_tree(),
				polyline.number_of_segments() - 1 /*last_segment_index*/);
	}
}
