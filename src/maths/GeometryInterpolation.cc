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
#include <boost/bind.hpp>

#include "GeometryInterpolation.h"

#include "AngularDistance.h"
#include "GeometryDistance.h"
#include "Centroid.h"
#include "LatLonPoint.h"
#include "MathsUtils.h"
#include "Rotation.h"
#include "types.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


namespace GPlatesMaths
{
	namespace
	{
		/**
		 * Returns true if @a from_polyline is mostly to the left of @a to_polyline in the reference
		 * frame where @a rotation_axis is the North pole.
		 *
		 * Left meaning longitude in North pole reference frame.
		 */
		bool
		from_polyline_is_mostly_left_of_to_polyline(
				const PolylineOnSphere &from_polyline,
				const PolylineOnSphere &to_polyline,
				const UnitVector3D &rotation_axis)
		{
			// The plane divides into one half space to left, and one to right, of 'from_polyline'.
			const Vector3D from_polygon_dividing_plane_normal = cross(
					rotation_axis,
					Centroid::calculate_outline_centroid(from_polyline));

			// If 'to_polyline's centroid in the positive half of the dividing plane of 'from_polyline'
			// then it means 'from_polyline' is mostly to the left (longitude-wise) of 'to_polyline'.
			return is_strictly_positive(dot(
					from_polygon_dividing_plane_normal,
					Centroid::calculate_outline_centroid(to_polyline)));
		}

		/**
		 * Convert PolylineOnSphere to a list of LatLonPoint's.
		 */
		void
		convert_polyline_to_lat_lon_points(
				std::list<LatLonPoint> &polyline_points,
				const PolylineOnSphere &polyline)
		{
			PolylineOnSphere::vertex_const_iterator polyline_vertex_iter = polyline.vertex_begin();
			PolylineOnSphere::vertex_const_iterator polyline_vertex_end = polyline.vertex_end();
			for ( ; polyline_vertex_iter != polyline_vertex_end; ++polyline_vertex_iter)
			{
				polyline_points.push_back(make_lat_lon_point(*polyline_vertex_iter));
			}
		}

		/**
		 * Ensure polyline has points that are monotonically decreasing in latitude (North to South).
		 */
		void
		ensure_points_are_monotonically_decreasing_in_latitude(
				std::list<LatLonPoint> &polyline_points)
		{
			// Ensure polyline has points ordered from North to South.
			if (polyline_points.front().latitude() < polyline_points.back().latitude())
			{
				polyline_points.reverse();
			}

			// Ensure polyline points are monotonically decreasing in latitude.
			std::list<LatLonPoint>::iterator polyline_points_iter = polyline_points.begin();
			std::list<LatLonPoint>::iterator polyline_points_end = polyline_points.end();
			double southmost_latitude_so_far = polyline_points_iter->latitude();
			for (++polyline_points_iter;
				polyline_points_iter != polyline_points_end;
				++polyline_points_iter)
			{
				if (polyline_points_iter->latitude() > southmost_latitude_so_far)
				{
					*polyline_points_iter = LatLonPoint(southmost_latitude_so_far, polyline_points_iter->longitude());
				}
				else
				{
					southmost_latitude_so_far = polyline_points_iter->latitude();
				}
			}
		}

		/**
		 * Crudely attempt to ensure that the latitudes of the endpoints of the two lines are reasonably
		 * close together before interpolation occurs.
		 */
		void
		limit_latitude_range(
				std::list<LatLonPoint> &polyline_points,
				const double &min_latitude,
				const double &max_latitude)
		{
			// Erase those points that are outside the [min_latitude, max_latitude] range.
			std::list<LatLonPoint>::iterator points_iter = polyline_points.begin();
			while (points_iter != polyline_points.end())
			{
				const LatLonPoint &point = *points_iter;

				if (point.latitude() < min_latitude ||
					point.latitude() > max_latitude)
				{
					points_iter = polyline_points.erase(points_iter);
					continue;
				}

				++points_iter;
			}
		}

		/**
		 * Ensure that @a lat_lon_points has a point at each latitude in @a all_lat_lon_points.
		 *
		 * Upon returning @a lat_lon_points will have the same number of points as @a all_lat_lon_points.
		 */
		void
		ensure_aligned_latitudes(
				std::list<LatLonPoint> &lat_lon_points,
				const std::vector<LatLonPoint> &all_lat_lon_points)
		{
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					!lat_lon_points.empty() &&
							all_lat_lon_points.size() >= lat_lon_points.size(),
					GPLATES_ASSERTION_SOURCE);

			std::list<LatLonPoint>::iterator lat_lon_points_iter = lat_lon_points.begin();
			std::list<LatLonPoint>::iterator lat_lon_points_end = lat_lon_points.end();

			std::vector<LatLonPoint>::const_iterator all_lat_lon_points_iter = all_lat_lon_points.begin();
			std::vector<LatLonPoint>::const_iterator all_lat_lon_points_end = all_lat_lon_points.end();

			// Insert duplicate starting points if there are higher latitudes (in the other polyline).
			for ( ;
				all_lat_lon_points_iter != all_lat_lon_points_end &&
					lat_lon_points_iter->latitude() < all_lat_lon_points_iter->latitude();
				++all_lat_lon_points_iter)
			{
				lat_lon_points.insert(lat_lon_points.begin(), *lat_lon_points_iter);
			}

			// Iterate over the latitude range that is common to both polylines.
			while (lat_lon_points_iter != lat_lon_points_end &&
				all_lat_lon_points_iter != all_lat_lon_points_end)
			{
				if (lat_lon_points_iter->latitude() == all_lat_lon_points_iter->latitude())
				{
					// The current point in 'all' points is either:
					//  1) the same as the current point, or
					//  2) a point in the other polyline that happens to have the same latitude.
					//
					// ...either way we don't need to create a new point.
					++lat_lon_points_iter;
					++all_lat_lon_points_iter;
					continue;
				}

				// Current point is always in 'all' points so it should either have same latitude
				// (handled above) or a lower latitude.
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						lat_lon_points_iter->latitude() < all_lat_lon_points_iter->latitude(),
						GPLATES_ASSERTION_SOURCE);

				// Should always have a previous point (due to above latitude equality comparison).
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						lat_lon_points_iter != lat_lon_points.begin(),
						GPLATES_ASSERTION_SOURCE);

				std::list<LatLonPoint>::iterator prev_lat_lon_point_iter = lat_lon_points_iter;
				--prev_lat_lon_point_iter;

				// Linearly interpolate between the current and previous points at the current latitude.
				//
				// FIXME:  We should slerp rather than lerp.
				const double interp =
						(all_lat_lon_points_iter->latitude() - lat_lon_points_iter->latitude()) /
								(prev_lat_lon_point_iter->latitude() - lat_lon_points_iter->latitude());
				const double interp_lon = lat_lon_points_iter->longitude() + interp *
						(prev_lat_lon_point_iter->longitude() - lat_lon_points_iter->longitude());

				// Insert the new point before the current point.
				const LatLonPoint new_point(all_lat_lon_points_iter->latitude(), interp_lon);
				lat_lon_points.insert(lat_lon_points_iter, new_point);

				++all_lat_lon_points_iter;
			}

			// We should run out of points before (or at same time as) we run out of 'all' points.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					lat_lon_points_iter == lat_lon_points_end,
					GPLATES_ASSERTION_SOURCE);

			// Insert duplicate ending points if there are lower latitudes (in the other polyline).
			// All remaining points are points that are not in the polyline (so must be in other polyline).
			for ( ;
				all_lat_lon_points_iter != all_lat_lon_points_end;
				++all_lat_lon_points_iter)
			{
				// Any remaining 'all' points should have lower (or equal) latitudes.
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						all_lat_lon_points_iter->latitude() <= lat_lon_points.back().latitude(),
						GPLATES_ASSERTION_SOURCE);

				lat_lon_points.push_back(lat_lon_points.back());
			}

			// We should have same number of points as 'all' points.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					lat_lon_points.size() == all_lat_lon_points.size(),
					GPLATES_ASSERTION_SOURCE);
		}

#if 0
		/**
		 * 
		 */
		void
		flatten_longitude_overlaps(
				const LatLonPoint &from_lat_lon_point,
				LatLonPoint to_lat_lon_point,
				std::vector<PointOnSphere> &from_polyline_points,
				std::vector<PointOnSphere> &to_polyline_points,
				std::vector<Rotation> &interpolate_point_rotations,
				bool is_from_polyline_mostly_left_of_to_polyline,
				AngularDistance &max_from_to_point_distance)
		{
			const bool points_overlap = is_from_polyline_mostly_left_of_to_polyline
					? (from_lat_lon_point.longitude() > to_lat_lon_point.longitude())
					: (from_lat_lon_point.longitude() < to_lat_lon_point.longitude());
			if (points_overlap)
			{
				to_lat_lon_point = LatLonPoint(
						to_lat_lon_point.latitude(),
						from_lat_lon_point.longitude());
			}

			const PointOnSphere from_point = make_point_on_sphere(from_lat_lon_point);
			const PointOnSphere to_point = make_point_on_sphere(to_lat_lon_point);

			const AngularDistance from_to_point_distance = minimum_distance(from_point, to_point);
			if (from_to_point_distance.is_precisely_greater_than(max_from_to_point_distance))
			{
				max_from_to_point_distance = from_to_point_distance;
			}

			const Rotation interpolate_point_rotation = Rotation::create(
					UnitVector3D::zBasis(),
					convert_deg_to_rad(to_lat_lon_point.longitude() - from_lat_lon_point.longitude()));

			from_polyline_points.push_back(from_point);
			to_polyline_points.push_back(to_point);
			interpolate_point_rotations.push_back(interpolate_point_rotation);
		}
#endif

		unsigned int
		calculate_num_interpolations(
				const double &interpolate_resolution_radians)
		{
			AngularDistance max_from_to_point_distance = AngularDistance::ZERO;


			return static_cast<unsigned int>(
					max_from_to_point_distance.calculate_angle().dval() / interpolate_resolution_radians);
		}

		/**
		 * Generate interpolated polylines and append to @a interpolated_polylines.
		 *
		 * The interpolated polylines are rotated from their North pole reference frame back to
		 * their original reference frame.
		 */
		void
		interpolate_polylines(
				std::vector<PolylineOnSphere::non_null_ptr_to_const_type> &interpolated_polylines,
				const std::vector<PointOnSphere> &from_polyline_points,
				const std::vector<PointOnSphere> &to_polyline_points,
				const std::vector<Rotation> &interpolate_point_rotations,
				const Rotation &rotate_from_north_pole,
				unsigned int num_interpolations)
		{
			const unsigned int num_points = from_polyline_points.size();

			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					to_polyline_points.size() == num_points &&
							interpolate_point_rotations.size() == num_points,
					GPLATES_ASSERTION_SOURCE);

			// Create and add the 'from' polyline first.
			const PolylineOnSphere::non_null_ptr_to_const_type from_polyline =
					rotate_from_north_pole *
							PolylineOnSphere::create_on_heap(
									from_polyline_points.begin(),
									from_polyline_points.end());
			interpolated_polylines.push_back(from_polyline);

			// Create and add the interpolated polylines.
			std::vector<PointOnSphere> interpolated_points(from_polyline_points);
			for (unsigned int n = 0; n < num_interpolations; ++n)
			{
				// Rotate the points by one interpolation interval.
				// Each point has its own rotation because interval spacing varies along polyline.
				for (unsigned int p = 0; p < num_points; ++p)
				{
					interpolated_points[p] = interpolate_point_rotations[p] * interpolated_points[p];
				}

				const PolylineOnSphere::non_null_ptr_to_const_type interpolated_polyline =
						rotate_from_north_pole *
								PolylineOnSphere::create_on_heap(
										interpolated_points.begin(),
										interpolated_points.end());
				interpolated_polylines.push_back(interpolated_polyline);
			}

			// Create and add the 'to' polyline last.
			const PolylineOnSphere::non_null_ptr_to_const_type to_polyline =
					rotate_from_north_pole *
							PolylineOnSphere::create_on_heap(
									to_polyline_points.begin(),
									to_polyline_points.end());
			interpolated_polylines.push_back(to_polyline);
		}
	}
}


void
GPlatesMaths::interpolate(
		std::vector<PolylineOnSphere::non_null_ptr_to_const_type> &interpolated_polylines,
		const PolylineOnSphere::non_null_ptr_to_const_type &from_polyline,
		const PolylineOnSphere::non_null_ptr_to_const_type &to_polyline,
		const UnitVector3D &rotation_axis,
		const double &interpolate_resolution_radians)
{
	const bool is_from_polyline_mostly_left_of_to_polyline =
			from_polyline_is_mostly_left_of_to_polyline(*from_polyline, *to_polyline, rotation_axis);

	// Rotation to plate rotation axis (and polylines) at the North pole.
	const Rotation rotate_to_north_pole = Rotation::create(rotation_axis, UnitVector3D::zBasis());
	const Rotation rotate_from_north_pole = rotate_to_north_pole.get_reverse();

	// Rotate the polylines to the North pole rotation reference frame.
	const PolylineOnSphere::non_null_ptr_to_const_type from_polyline_at_north_pole =
			rotate_to_north_pole * from_polyline;
	const PolylineOnSphere::non_null_ptr_to_const_type to_polyline_at_north_pole =
			rotate_to_north_pole * to_polyline;

	// Convert each polyline to a sequence of lat/lon points.
	std::list<LatLonPoint> from_polyline_at_north_pole_lat_lon_points;
	std::list<LatLonPoint> to_polyline_at_north_pole_lat_lon_points;
	convert_polyline_to_lat_lon_points(from_polyline_at_north_pole_lat_lon_points, *from_polyline_at_north_pole);
	convert_polyline_to_lat_lon_points(to_polyline_at_north_pole_lat_lon_points, *to_polyline_at_north_pole);

	// Ensure both polylines have points that are monotonically decreasing in latitude (North to South).
	ensure_points_are_monotonically_decreasing_in_latitude(from_polyline_at_north_pole_lat_lon_points);
	ensure_points_are_monotonically_decreasing_in_latitude(to_polyline_at_north_pole_lat_lon_points);

	// Crudely attempt to ensure that the latitudes of the endpoints of the two lines are reasonably
	// close together before interpolation occurs.
	const double limit_latitude_threshold = 5; // Limit the latitudinal difference to 5 degrees.
	const double from_polyline_min_latitude = from_polyline_at_north_pole_lat_lon_points.back().latitude();
	const double from_polyline_max_latitude = from_polyline_at_north_pole_lat_lon_points.front().latitude();
	const double to_polyline_min_latitude = to_polyline_at_north_pole_lat_lon_points.back().latitude();
	const double to_polyline_max_latitude = to_polyline_at_north_pole_lat_lon_points.front().latitude();
	limit_latitude_range(
			from_polyline_at_north_pole_lat_lon_points,
			to_polyline_min_latitude - limit_latitude_threshold,
			to_polyline_max_latitude + limit_latitude_threshold);
	limit_latitude_range(
			to_polyline_at_north_pole_lat_lon_points,
			from_polyline_min_latitude - limit_latitude_threshold,
			from_polyline_max_latitude + limit_latitude_threshold);

	// Merge already sorted (North to South) 'from' and 'to' sequences into one sequence containing all points.
	// Note that duplicate latitudes are not removed - so the total number of points is the sum of 'from' and 'to'.
	std::vector<LatLonPoint> all_north_pole_lat_lon_points;
	std::merge(
			from_polyline_at_north_pole_lat_lon_points.begin(),
			from_polyline_at_north_pole_lat_lon_points.end(),
			to_polyline_at_north_pole_lat_lon_points.begin(),
			to_polyline_at_north_pole_lat_lon_points.end(),
			std::back_inserter(all_north_pole_lat_lon_points),
			bind(&LatLonPoint::latitude, _1) > bind(&LatLonPoint::latitude, _2));

	// Ensure all points in both lines have matching latitudes so we can interpolate between them.
	ensure_aligned_latitudes(
			from_polyline_at_north_pole_lat_lon_points,
			all_north_pole_lat_lon_points);
	ensure_aligned_latitudes(
			to_polyline_at_north_pole_lat_lon_points,
			all_north_pole_lat_lon_points);

	std::vector<PointOnSphere> from_polyline_at_north_pole_points;
	std::vector<PointOnSphere> to_polyline_at_north_pole_points;
	std::vector<Rotation> interpolate_point_rotations;

	const unsigned int num_interpolations = 0;

	// Generate the interpolated polylines.
	// Add both the interpolated polylines and the 'from' and 'to' polylines to 'interpolated_polylines'.
	interpolate_polylines(
			interpolated_polylines,
			from_polyline_at_north_pole_points,
			to_polyline_at_north_pole_points,
			interpolate_point_rotations,
			rotate_from_north_pole,
			num_interpolations);
}
