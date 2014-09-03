/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2014 The University of Sydney, Australia
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
#include <functional>
#include <iterator>
#include <list>
#include <vector>
#include <boost/optional.hpp>

//Due to a gcc bug, 
//gcc 4.6 reports uninitialized variable warning falsely.
//disable the uninitialized warning for gcc 4.6 here
// The warning occurs in 'calculate_interpolate_point_rotations()'.
#include "global/CompilerWarnings.h"
#if (__GNUC__ == 4 && __GNUC_MINOR__ == 6)
DISABLE_GCC_WARNING("-Wuninitialized")
#endif 

#include "GeometryInterpolation.h"

#include "AngularDistance.h"
#include "GeometryDistance.h"
#include "GreatCircleArc.h"
#include "MathsUtils.h"
#include "PointOnSphere.h"
#include "Rotation.h"
#include "types.h"
#include "UnitVector3D.h"
#include "Vector3D.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


namespace GPlatesMaths
{
	/**
	 * Implementation for rotation interpolation of polylines.
	 */
	namespace RotationInterpolateImpl
	{
		/**
		 * Compares latitude of two points (distances relative to a North pole) using greater than.
		 */
		struct LatitudeGreaterCompare :
				public std::binary_function<PointOnSphere, PointOnSphere, bool>
		{
			explicit
			LatitudeGreaterCompare(
					const UnitVector3D &north_pole) :
				d_north_pole(north_pole)
			{  }

			bool
			operator()(
					const PointOnSphere &p1,
					const PointOnSphere &p2) const
			{
				return dot(p1.position_vector(), d_north_pole).dval() >
					dot(p2.position_vector(), d_north_pole).dval();
			}

		private:
			UnitVector3D d_north_pole;
		};

		/**
		 * Ensure polyline has points that are monotonically decreasing in latitude
		 * (distance from rotation axis).
		 */
		void
		ensure_points_are_monotonically_decreasing_in_latitude(
				std::list<PointOnSphere> &polyline_points,
				const UnitVector3D &rotation_axis)
		{
			// Ensure polyline has points ordered from closest to furthest from the rotation axis.
			// Use dot product instead of angle since faster.
			if (dot(polyline_points.front().position_vector(), rotation_axis).dval() <
				dot(polyline_points.back().position_vector(), rotation_axis).dval())
			{
				polyline_points.reverse();
			}

			// Ensure polyline points are monotonically decreasing in latitude.
			std::list<PointOnSphere>::iterator polyline_points_iter = polyline_points.begin();
			std::list<PointOnSphere>::iterator polyline_points_end = polyline_points.end();
			real_t southmost_dot_product_so_far = dot(polyline_points_iter->position_vector(), rotation_axis);
			for (++polyline_points_iter;
				polyline_points_iter != polyline_points_end;
				++polyline_points_iter)
			{
				const real_t dot_product = dot(polyline_points_iter->position_vector(), rotation_axis);

				if (dot_product >= southmost_dot_product_so_far) // epsilon test
				{
					// Reduce the southmost latitude slightly to ensure our latitudes are decreasing.
					// Otherwise due to numerical tolerance the rotated point might not have a lower latitude.
					// A reduction of 1e-10 equates to a maximum angular deviation of 80 metres distance
					// at the pole (rotation axis).
					southmost_dot_product_so_far -= 1e-10;
					if (southmost_dot_product_so_far.is_precisely_less_than(-1))
					{
						// The lowest possible latitude is the antipodal of the rotation axis.
						southmost_dot_product_so_far = -1;
						*polyline_points_iter = PointOnSphere(-rotation_axis);
						continue;
					}

					// Rotate the current point away from the rotation axis so that it has a slightly
					// lower latitude than the current southmost point.
					const Vector3D rotate_to_southmost_latitude_axis =
							cross(rotation_axis, polyline_points_iter->position_vector());
					if (rotate_to_southmost_latitude_axis.magSqrd() > 0)
					{
						const real_t southmost_distance_so_far = acos(southmost_dot_product_so_far);
						const real_t distance = acos(dot_product);
						const real_t rotate_to_southmost_latitude_angle = southmost_distance_so_far - distance;

						const Rotation rotate_to_southmost_latitude = Rotation::create(
								rotate_to_southmost_latitude_axis.get_normalisation(),
								rotate_to_southmost_latitude_angle);

						// Rotate the current point to satisfy decreasing latitude requirement.
						*polyline_points_iter = rotate_to_southmost_latitude * *polyline_points_iter;
					}
					else
					{
						// The current point is too close to the rotation axis to rotate it away
						// from the rotation axis - so we'll just set it equal to the rotation axis.
						*polyline_points_iter = PointOnSphere(rotation_axis);
					}
				}
				else
				{
					southmost_dot_product_so_far = dot_product;
				}
			}
		}

		/**
		 * Linearly interpolate between the @a point1 and @a point2 (along their connecting great
		 * circle arc) such that the resultant point has a dot product with @a small_circle_axis of
		 * @a small_circle_axis_dot_product.
		 *
		 * It is assumed that one of the points is above, and one below, the small circle.
		 */
		PointOnSphere
		intersect_small_circle_with_great_circle_arc(
				const PointOnSphere &point1,
				const PointOnSphere &point2,
				const double &small_circle_axis_dot_product,
				const UnitVector3D &small_circle_axis)
		{
			// Intersection of two planes n1.r = d1 and n2.r = d2 is the line:
			//   r = c1 * n1 + c2 * n2 + t * n1 x n2
			//     = A + t * B
			// ...where...
			//   c1 = [d1 - d2*(n1.n2)] / [1 - (n1.n2)^2]
			//   c2 = [d2 - d1*(n1.n2)] / [1 - (n1.n2)^2]
			//   A  = c1 * n1 + c2 * n2
			//   B  = n1 x n2
			// ...and first plane is small circle (d1!=0) and second plane is great circle arc (d2==0).
			//   c1 = d1 / [1 - (n1.n2)^2]
			//   c2 = -d1*(n1.n2) / [1 - (n1.n2)^2]
			//
			// Line intersects unit sphere when...
			//   |r|   = 1
			//   |r|^2 = 1
			//       1 = |A + t * B| ^ 2
			//       0 = (A + t * B) . (A + t * B) - 1
			//       0 = B.B * t^2 + 2 * A.B * t + A.A - 1
			//         = a * t^2 + b * t + c
			// ...where a = B.B and b = 2 * A.B and c = A.A - 1.
			//
			// Solve quadratic equation...
			//       t = (-b +\- sqrt(b^2 - 4*a*c)] / (2*a)
			// ...then find up to two intersection points.
			// The closest one to the great circle arc (between 'point1' and 'point2') is the solution.
			// Theoretically it'll actually be *on* the great circle arc.
			//
			const GreatCircleArc gca = GreatCircleArc::create(point1, point2);
			// Return either end point if the arc is zero length.
			if (gca.is_zero_length())
			{
				return point1;
			}

			const UnitVector3D &gca_axis = gca.rotation_axis();
			const real_t n1_dot_n2 = dot(small_circle_axis, gca_axis);
			if (n1_dot_n2 >= 1) // epsilon test for parallel axes
			{
				// Small circle and great circle are parallel.
				// Just return the first point (both points will lie on the small circle).
				return point1;
			}

			const double inv_c12 = 1.0 / (1 - (n1_dot_n2 * n1_dot_n2).dval());
			const double c1 = small_circle_axis_dot_product * inv_c12;
			const double c2 = -c1 * n1_dot_n2.dval();
			const Vector3D A = c1 * small_circle_axis + c2 * gca_axis;
			const Vector3D B = cross(small_circle_axis, gca_axis);
			const real_t a = dot(B, B);
			const real_t b = 2 * dot(A, B);
			const real_t c = dot(A, A) - 1;
			const real_t discriminant = b * b - 4 * a * c;
			if (discriminant <= 0) // epsilon test
			{
				// We really shouldn't get too negative a result so emit warning if we do.
				if (discriminant.dval() < -0.001)
				{
					qWarning() << "GeometryInterpolation.cc: negative discriminant.";
				}

				// Only one intersection point.
				const real_t t = -b / (2 * a);
				return PointOnSphere((A + t * B).get_normalisation());
			}

			// Two intersection points.
			const real_t pm = sqrt(discriminant);
			const real_t t1 = (-b - pm) / (2 * a);
			const real_t t2 = (-b + pm) / (2 * a);
			const UnitVector3D intersect1 = (A + t1 * B).get_normalisation();
			const UnitVector3D intersect2 = (A + t2 * B).get_normalisation();
			const AngularDistance dist_intersect1 = minimum_distance(intersect1, gca);
			const AngularDistance dist_intersect2 = minimum_distance(intersect2, gca);

			return dist_intersect1.is_precisely_less_than(dist_intersect2)
					? PointOnSphere(intersect1)
					: PointOnSphere(intersect2);
		}

		/**
		 * Clip away any latitude ranges of either polyline that is not common to both polylines.
		 *
		 * Assumes the latitudes of @a from_polyline_points and @a to_polyline_points polylines are
		 * ordered from closest to furthest from the @a rotation_axis.
		 *
		 * Returns false if the polylines do not overlap in latitude (where North pole is @a rotation_axis).
		 */
		bool
		limit_latitude_range(
				std::list<PointOnSphere> &from_polyline_points,
				std::list<PointOnSphere> &to_polyline_points,
				const UnitVector3D &rotation_axis)
		{
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					from_polyline_points.size() >= 2 &&
							to_polyline_points.size() >= 2,
					GPLATES_ASSERTION_SOURCE);


			//
			// Limit the start point of polyline with higher latitude start point(s) to match the other.
			//

			// Latitudes are ordered from closest to furthest from the rotation axis.
			// Use dot product instead of angle since faster.
			const bool from_start_latitude_higher_than_to_start =
					dot(from_polyline_points.front().position_vector(), rotation_axis).dval() >
						dot(to_polyline_points.front().position_vector(), rotation_axis).dval();

			const PointOnSphere &src_start_polyline_point = from_start_latitude_higher_than_to_start
					? to_polyline_points.front()
					: from_polyline_points.front();
			std::list<PointOnSphere> &dst_start_polyline_points = from_start_latitude_higher_than_to_start
					? from_polyline_points
					: to_polyline_points;

			// Search for two consecutive points of 'dst' that overlap the latitude of the first 'src' point.
			std::list<PointOnSphere>::iterator dst_start_points_iter = dst_start_polyline_points.begin();
			++dst_start_points_iter; // second element
			for ( ; ; ++dst_start_points_iter)
			{
				// If all points have a higher latitude then there is no
				// latitude overlap between the polylines.
				if (dst_start_points_iter == dst_start_polyline_points.end())
				{
					return false;
				}

				if (dot(dst_start_points_iter->position_vector(), rotation_axis).dval() <=
					dot(src_start_polyline_point.position_vector(), rotation_axis).dval())
				{
					break;
				}
			}

			// Interpolate between previous point and current point.
			std::list<PointOnSphere>::iterator prev_dst_start_points_iter = dst_start_points_iter;
			--prev_dst_start_points_iter;

			// Calculate the new interpolated point.
			const PointOnSphere interp_start_point = 
					intersect_small_circle_with_great_circle_arc(
							*prev_dst_start_points_iter,
							*dst_start_points_iter,
							dot(src_start_polyline_point.position_vector(), rotation_axis).dval(),
							rotation_axis);

			// Erase those points that are outside the overlapping latitude range.
			dst_start_polyline_points.erase(dst_start_polyline_points.begin(), dst_start_points_iter);

			// Ensure the new point is inserted at the correct location in the sequence such that
			// the points remain monotonically decreasing in latitude.
			// Due to numerical tolerance in interpolated position the latitudes can be slightly re-ordered.
			for (dst_start_points_iter = dst_start_polyline_points.begin();
				dst_start_points_iter != dst_start_polyline_points.end();
				++dst_start_points_iter)
			{
				if (dot(dst_start_points_iter->position_vector(), rotation_axis).dval() <=
					dot(interp_start_point.position_vector(), rotation_axis).dval())
				{
					break;
				}
			}
			dst_start_polyline_points.insert(dst_start_points_iter, interp_start_point);


			//
			// Limit the end point of polyline with lower latitude end point(s) to match the other.
			//

			// Latitudes are ordered from closest to furthest from the rotation axis.
			// Use dot product instead of angle since faster.
			const bool from_end_latitude_lower_than_to_start =
					dot(from_polyline_points.back().position_vector(), rotation_axis).dval() <
						dot(to_polyline_points.back().position_vector(), rotation_axis).dval();

			const PointOnSphere &src_end_polyline_point = from_end_latitude_lower_than_to_start
					? to_polyline_points.back()
					: from_polyline_points.back();
			std::list<PointOnSphere> &dst_end_polyline_points = from_end_latitude_lower_than_to_start
					? from_polyline_points
					: to_polyline_points;

			// Search for two consecutive points of 'dst' that overlap the latitude of the last 'src' point.
			std::list<PointOnSphere>::iterator dst_end_points_iter = dst_end_polyline_points.end();
			--dst_end_points_iter; // last element
			--dst_end_points_iter; // second last element
			for ( ; ; --dst_end_points_iter)
			{
				if (dot(dst_end_points_iter->position_vector(), rotation_axis).dval() >=
					dot(src_end_polyline_point.position_vector(), rotation_axis).dval())
				{
					break;
				}

				// If all points have a lower latitude then there is no
				// latitude overlap between the polylines.
				if (dst_end_points_iter == dst_end_polyline_points.begin())
				{
					return false;
				}
			}

			// Interpolate between previous point and current point.
			std::list<PointOnSphere>::iterator prev_dst_end_points_iter = dst_end_points_iter;
			++prev_dst_end_points_iter;

			// Calculate the new interpolated point.
			// Insert the new point after the current point (which is before the previous point).
			const PointOnSphere interp_end_point = 
					intersect_small_circle_with_great_circle_arc(
							*prev_dst_end_points_iter,
							*dst_end_points_iter,
							dot(src_end_polyline_point.position_vector(), rotation_axis).dval(),
							rotation_axis);

			// Erase those points that are outside the overlapping latitude range.
			dst_end_polyline_points.erase(prev_dst_end_points_iter, dst_end_polyline_points.end());

			// Ensure the new point is inserted at the correct location in the sequence such that
			// the points remain monotonically decreasing in latitude.
			// Due to numerical tolerance in interpolated position the latitudes can be slightly re-ordered.
			for (dst_end_points_iter = dst_end_polyline_points.end();
				dst_end_points_iter != dst_end_polyline_points.begin();
				)
			{
				--dst_end_points_iter;

				if (dot(dst_end_points_iter->position_vector(), rotation_axis).dval() >
					dot(interp_end_point.position_vector(), rotation_axis).dval())
				{
					++dst_end_points_iter;
					break;
				}
			}
			dst_end_polyline_points.insert(dst_end_points_iter, interp_end_point);


			// Polylines overlap in latitude.
			return true;
		}

		/**
		 * Ensure that @a points has a point at each latitude in @a all_points.
		 *
		 * Upon returning @a points will have the same number of points as @a all_points.
		 */
		void
		ensure_aligned_latitudes(
				std::list<PointOnSphere> &points,
				const std::vector<PointOnSphere> &all_points,
				const UnitVector3D &rotation_axis)
		{
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					!points.empty() &&
							all_points.size() >= points.size(),
					GPLATES_ASSERTION_SOURCE);

			std::list<PointOnSphere>::iterator points_iter = points.begin();
			std::list<PointOnSphere>::iterator points_end = points.end();

			std::vector<PointOnSphere>::const_iterator all_points_iter = all_points.begin();
			std::vector<PointOnSphere>::const_iterator all_points_end = all_points.end();

			// Insert duplicate starting points if there are higher latitudes (in the other polyline).
			// This is only needed due to numerical tolerance because the 'limit_latitude_range()'
			// function should have already ensured equal latitude ranges for both polylines.
			for ( ; all_points_iter != all_points_end; ++all_points_iter)
			{
				if (dot(points_iter->position_vector(), rotation_axis).dval() >=
					dot(all_points_iter->position_vector(), rotation_axis).dval())
				{
					break;
				}

				// The latitudes are equal to within numerical tolerance (so no interpolation needed).
				points.insert(points.begin(), *points_iter);
			}

			// Iterate over the latitude range that is common to both polylines.
			while (points_iter != points_end &&
				all_points_iter != all_points_end)
			{
				const real_t point_dot_product = dot(points_iter->position_vector(), rotation_axis);
				const real_t all_point_dot_product = dot(all_points_iter->position_vector(), rotation_axis);

				if (point_dot_product == all_point_dot_product) // epsilon test
				{
					// The current point in 'all' points is either:
					//  1) the same as the current point, or
					//  2) a point in the other polyline that happens to have the same latitude.
					//
					// ...either way we don't need to create a new point.
					++points_iter;
					++all_points_iter;
					continue;
				}

				// Current point is always in 'all' points so it should either have same latitude
				// (handled above) or a lower latitude.
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						point_dot_product.is_precisely_less_than(all_point_dot_product.dval()),
						GPLATES_ASSERTION_SOURCE);

				// Should always have a previous point (due to above latitude equality comparison).
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						points_iter != points.begin(),
						GPLATES_ASSERTION_SOURCE);

				std::list<PointOnSphere>::iterator prev_point_iter = points_iter;
				--prev_point_iter;

				// Insert the new interpolated point before the current point.
				const PointOnSphere interp_point = 
						intersect_small_circle_with_great_circle_arc(
								*prev_point_iter,
								*points_iter,
								all_point_dot_product.dval(),
								rotation_axis);
				points.insert(points_iter, interp_point);

				++all_points_iter;
			}

			// We should run out of points before (or at same time as) we run out of 'all' points.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					points_iter == points_end,
					GPLATES_ASSERTION_SOURCE);

			// Insert duplicate ending points if there are lower latitudes (in the other polyline).
			// All remaining points are points that are not in the polyline (so must be in other polyline).
			// This is only needed due to numerical tolerance because the 'limit_latitude_range()'
			// function should have already ensured equal latitude ranges for both polylines.
			for ( ; all_points_iter != all_points_end; ++all_points_iter)
			{
				// Any remaining 'all' points should have lower (or equal) latitudes.
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						dot(all_points_iter->position_vector(), rotation_axis) <=
								dot(points.back().position_vector(), rotation_axis), // epsilon test
						GPLATES_ASSERTION_SOURCE);

				// The latitudes are equal to within numerical tolerance (so no interpolation needed).
				points.push_back(points.back());
			}

			// We should have same number of points as 'all' points.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					points.size() == all_points.size(),
					GPLATES_ASSERTION_SOURCE);
		}

		/**
		 * Returns true if @a point1 is mostly to the left of @a point2
		 * in the reference frame where @a rotation_axis is the North pole.
		 *
		 * Left meaning longitude in North pole reference frame.
		 */
		bool
		point1_is_left_of_point2(
				const UnitVector3D &point1,
				const UnitVector3D &point2,
				const UnitVector3D &rotation_axis)
		{
			// The plane divides into one half space to left, and one to right, of 'point1'.
			const Vector3D from_point_dividing_plane_normal = cross(rotation_axis, point1);

			// If 'point2' in the positive half of the dividing plane of 'point1'
			// then it means 'point1' is to the left (longitude-wise) of 'point2'.
			return is_strictly_positive(dot(from_point_dividing_plane_normal, point2));
		}

		/**
		 * Returns true if @a from_point is mostly to the left of @a to_point
		 * in the reference frame where @a rotation_axis is the North pole.
		 *
		 * Left meaning longitude in North pole reference frame.
		 */
		void
		flatten_longitude_overlaps(
				std::list<PointOnSphere> &from_polyline_points,
				std::list<PointOnSphere> &to_polyline_points,
				const UnitVector3D &rotation_axis)
		{
			// We should have same number of points in both polylines.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					from_polyline_points.size() == to_polyline_points.size(),
					GPLATES_ASSERTION_SOURCE);

			// Initialise with false values.
			std::vector<bool> from_points_left_of_to_points(from_polyline_points.size());

			std::list<PointOnSphere>::iterator from_points_iter = from_polyline_points.begin();
			const std::list<PointOnSphere>::iterator from_points_end = from_polyline_points.end();

			std::list<PointOnSphere>::iterator to_points_iter = to_polyline_points.begin();

			std::vector<bool>::iterator from_points_left_of_to_points_iter =
					from_points_left_of_to_points.begin();
			unsigned int num_from_points_left_of_to_points = 0;
			unsigned int num_to_points_left_of_from_points = 0;

			// Loop through all points and determine which of 'from' and 'to' points is on left side.
			for ( ;
				from_points_iter != from_points_end;
				++from_points_iter, ++to_points_iter, ++from_points_left_of_to_points_iter)
			{
				const bool from_point_is_left_of_to_point =
						point1_is_left_of_point2(
								from_points_iter->position_vector(),
								to_points_iter->position_vector(),
								rotation_axis);
				if (from_point_is_left_of_to_point)
				{
					++num_from_points_left_of_to_points;
				}
				else
				{
					++num_to_points_left_of_from_points;
				}

				*from_points_left_of_to_points_iter = from_point_is_left_of_to_point;
			}

			// 'from' polyline is left of 'to' polyline is most of its points are on the left side.
			const bool is_from_polyline_mostly_left_of_to_polyline =
					num_from_points_left_of_to_points > num_to_points_left_of_from_points;

			from_points_iter = from_polyline_points.begin();
			to_points_iter = to_polyline_points.begin();
			from_points_left_of_to_points_iter = from_points_left_of_to_points.begin();

			// Loop through points and flatten overlaps as needed.
			for ( ;
				from_points_iter != from_points_end;
				++from_points_iter, ++to_points_iter, ++from_points_left_of_to_points_iter)
			{
				const bool from_point_is_left_of_to_point = *from_points_left_of_to_points_iter;

				const bool points_overlap = is_from_polyline_mostly_left_of_to_polyline
						? !from_point_is_left_of_to_point
						: from_point_is_left_of_to_point;
				if (points_overlap)
				{
					// The points overlap - favour the 'from' point by assigning it to the 'to' point.
					*to_points_iter = *from_points_iter;
				}
			}
		}

		/**
		 * Returns the number of interpolations between the two polylines based on the maximum
		 * distance between related points and the interpolation resolution.
		 *
		 * Returns zero if no interpolations are needed (eg, if maximum distance between polylines
		 * is less than the resolution).
		 */
		unsigned int
		calculate_num_interpolations(
				const std::list<PointOnSphere> &from_polyline_points,
				const std::list<PointOnSphere> &to_polyline_points,
				const UnitVector3D &rotation_axis,
				const double &interpolate_resolution_radians)
		{
			AngularDistance max_from_to_point_distance = AngularDistance::ZERO;

			std::list<PointOnSphere>::const_iterator from_points_iter = from_polyline_points.begin();
			const std::list<PointOnSphere>::const_iterator from_points_end = from_polyline_points.end();

			std::list<PointOnSphere>::const_iterator to_points_iter = to_polyline_points.begin();

			// We should have same number of points in both polylines.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					from_polyline_points.size() == to_polyline_points.size(),
					GPLATES_ASSERTION_SOURCE);

			for ( ; from_points_iter != from_points_end; ++from_points_iter, ++to_points_iter)
			{
				const AngularDistance distance_between_points =
						minimum_distance(*from_points_iter, *to_points_iter);
				if (distance_between_points.is_precisely_greater_than(max_from_to_point_distance))
				{
					max_from_to_point_distance = distance_between_points;
				}
			}

			return static_cast<unsigned int>(
					max_from_to_point_distance.calculate_angle().dval() / interpolate_resolution_radians);
		}

		/**
		 * Create a rotation for each 'from' / 'to' point pair that rotates one interpolation interval.
		 */
		void
		calculate_interpolate_point_rotations(
				std::vector<Rotation> &interpolate_point_rotations,
				const std::list<PointOnSphere> &from_polyline_points,
				const std::list<PointOnSphere> &to_polyline_points,
				const UnitVector3D &rotation_axis,
				unsigned int num_interpolations)
		{
			boost::optional<double> inv_num_interp_intervals;
			if (num_interpolations != 0)
			{
				inv_num_interp_intervals = 1.0 / (num_interpolations + 1);
			}

			std::list<PointOnSphere>::const_iterator from_points_iter = from_polyline_points.begin();
			const std::list<PointOnSphere>::const_iterator from_points_end = from_polyline_points.end();

			std::list<PointOnSphere>::const_iterator to_points_iter = to_polyline_points.begin();

			// We should have same number of points in both polylines.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					from_polyline_points.size() == to_polyline_points.size(),
					GPLATES_ASSERTION_SOURCE);

			interpolate_point_rotations.reserve(from_polyline_points.size());
			for ( ; from_points_iter != from_points_end; ++from_points_iter, ++to_points_iter)
			{
				// Calculate angle of rotation about the rotation axis between the two points.
				// Default to zero if points happen to be coincident with the rotation axis.
				real_t rotation_angle = 0;

				const Vector3D cross_rotation_axis_and_from_point =
						cross(rotation_axis, from_points_iter->position_vector());
				const Vector3D cross_rotation_axis_and_to_point =
						cross(rotation_axis, to_points_iter->position_vector());
				if (cross_rotation_axis_and_from_point.magSqrd() > 0 &&
					cross_rotation_axis_and_to_point.magSqrd() > 0)
				{
					rotation_angle = acos(dot(
							cross_rotation_axis_and_from_point.get_normalisation(),
							cross_rotation_axis_and_to_point.get_normalisation()));

					// Reverse rotation direction if 'to' point is left of 'from' point.
					if (dot(
							cross(cross_rotation_axis_and_from_point, cross_rotation_axis_and_to_point),
							rotation_axis).dval() < 0)
					{
						rotation_angle = -rotation_angle;
					}
				}

				// If we're interpolating then divide the angle equally among the intervals.
				if (inv_num_interp_intervals)
				{
					rotation_angle *= inv_num_interp_intervals.get();
				}

				interpolate_point_rotations.push_back(
						Rotation::create(rotation_axis, rotation_angle));
			}
		}

		/**
		 * Generate interpolated polylines and append to @a interpolated_polylines.
		 */
		void
		interpolate_polylines(
				std::vector<PolylineOnSphere::non_null_ptr_to_const_type> &interpolated_polylines,
				const std::list<PointOnSphere> &from_polyline_points,
				const std::list<PointOnSphere> &to_polyline_points,
				const std::vector<Rotation> &interpolate_point_rotations,
				unsigned int num_interpolations)
		{
			// Create and add the 'from' polyline first.
			const PolylineOnSphere::non_null_ptr_to_const_type from_polyline =
					PolylineOnSphere::create_on_heap(
							from_polyline_points.begin(),
							from_polyline_points.end());
			interpolated_polylines.push_back(from_polyline);

			const unsigned int num_points = from_polyline->number_of_vertices();

			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					interpolate_point_rotations.size() == num_points,
					GPLATES_ASSERTION_SOURCE);

			// Create and add the interpolated polylines.
			std::vector<PointOnSphere> interpolated_points(
					from_polyline_points.begin(),
					from_polyline_points.end());
			for (unsigned int n = 0; n < num_interpolations; ++n)
			{
				// Rotate the points by one interpolation interval.
				// Each point has its own rotation because interval spacing varies along polyline.
				for (unsigned int p = 0; p < num_points; ++p)
				{
					interpolated_points[p] = interpolate_point_rotations[p] * interpolated_points[p];
				}

				const PolylineOnSphere::non_null_ptr_to_const_type interpolated_polyline =
						PolylineOnSphere::create_on_heap(
								interpolated_points.begin(),
								interpolated_points.end());
				interpolated_polylines.push_back(interpolated_polyline);
			}

			// Create and add the 'to' polyline last.
			const PolylineOnSphere::non_null_ptr_to_const_type to_polyline =
					PolylineOnSphere::create_on_heap(
							to_polyline_points.begin(),
							to_polyline_points.end());
			interpolated_polylines.push_back(to_polyline);

			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					to_polyline->number_of_vertices() == num_points,
					GPLATES_ASSERTION_SOURCE);
		}
	}
}


bool
GPlatesMaths::interpolate(
		std::vector<PolylineOnSphere::non_null_ptr_to_const_type> &interpolated_polylines,
		const PolylineOnSphere::non_null_ptr_to_const_type &from_polyline,
		const PolylineOnSphere::non_null_ptr_to_const_type &to_polyline,
		const UnitVector3D &rotation_axis,
		const double &interpolate_resolution_radians)
{
	// Get a copy of the polyline points so we can insert, modify and erase them as needed.
	std::list<PointOnSphere> from_polyline_points(from_polyline->vertex_begin(), from_polyline->vertex_end());
	std::list<PointOnSphere> to_polyline_points(to_polyline->vertex_begin(), to_polyline->vertex_end());

	// Ensure both polylines have points that are monotonically decreasing in latitude
	// (distance from rotation axis).
	RotationInterpolateImpl::ensure_points_are_monotonically_decreasing_in_latitude(
			from_polyline_points,
			rotation_axis);
	RotationInterpolateImpl::ensure_points_are_monotonically_decreasing_in_latitude(
			to_polyline_points,
			rotation_axis);

	// Clip away any latitude ranges of either polyline that is not common to both polylines.
	if (!RotationInterpolateImpl::limit_latitude_range(from_polyline_points, to_polyline_points, rotation_axis))
	{
		// If the 'from' and 'to' polylines don't overlap in latitude then we cannot interpolate.
		return false;
	}

	// Merge already sorted (in decreasing latitude) 'from' and 'to' sequences into one sequence
	// containing all points.
	// Note that duplicate latitudes are not removed - so total number of points is sum of 'from' and 'to'.
	std::vector<PointOnSphere> all_points;
	std::merge(
			from_polyline_points.begin(),
			from_polyline_points.end(),
			to_polyline_points.begin(),
			to_polyline_points.end(),
			std::back_inserter(all_points),
			RotationInterpolateImpl::LatitudeGreaterCompare(rotation_axis));

	// Ensure all points in both lines have matching latitudes so we can interpolate between them.
	RotationInterpolateImpl::ensure_aligned_latitudes(
			from_polyline_points,
			all_points,
			rotation_axis);
	RotationInterpolateImpl::ensure_aligned_latitudes(
			to_polyline_points,
			all_points,
			rotation_axis);

	// Make sure the polylines don't overlap in longitude.
	RotationInterpolateImpl::flatten_longitude_overlaps(
			from_polyline_points,
			to_polyline_points,
			rotation_axis);

	const unsigned int num_interpolations =
			RotationInterpolateImpl::calculate_num_interpolations(
					from_polyline_points,
					to_polyline_points,
					rotation_axis,
					interpolate_resolution_radians);

	std::vector<Rotation> interpolate_point_rotations;
	RotationInterpolateImpl::calculate_interpolate_point_rotations(
			interpolate_point_rotations,
			from_polyline_points,
			to_polyline_points,
			rotation_axis,
			num_interpolations);

	// Generate the interpolated polylines.
	// Add both the interpolated polylines and the 'from' and 'to' polylines to 'interpolated_polylines'.
	RotationInterpolateImpl::interpolate_polylines(
			interpolated_polylines,
			from_polyline_points,
			to_polyline_points,
			interpolate_point_rotations,
			num_interpolations);

	return true;
}
