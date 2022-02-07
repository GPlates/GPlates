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

#ifndef GPLATES_MATHS_GEOMETRYINTERPOLATION_H
#define GPLATES_MATHS_GEOMETRYINTERPOLATION_H

#include <vector>
#include <boost/optional.hpp>

#include "PolylineOnSphere.h"
#include "types.h"
#include "UnitVector3D.h"


namespace GPlatesMaths
{
	/**
	 * Enumeration to determine how to flatten longitude overlaps in rotation interpolation function @a interpolate.
	 */
	namespace FlattenLongitudeOverlaps
	{
		enum Value
		{
			NO,        // Don't flatten longitude overlaps.
			USE_FROM,  // Use points in 'from' polyline to zero overlap (copy 'from' points into 'to' points).
			USE_TO     // Use points in 'to' polyline to zero overlap (copy 'to' points into 'from' points).
		};
	};

	/**
	 * Interpolates between two polylines along small circle arcs emanating from @a rotation_axis.
	 *
	 * The maximum distance between adjacent interpolated polylines is @a interpolate_resolution_radians.
	 * This determines the interpolation interval spacing.
	 *
	 * The original polylines @a from_polyline and @a to_polyline are also returned in
	 * @a interpolated_polylines since the points in each geometry are ordered from closest to
	 * @a rotation_axis to furthest (which may be different than the order in the originals).
	 * The original polylines (returned in @a interpolated_polylines) are also modified, if needed, such that
	 * they have monotonically decreasing latitudes (in North pole reference frame of @a rotation_axis).
	 * The latitude range of each polyline is restricted to be between the latitudes of its first
	 * and last points.
	 * They are also modified to have a common overlapping latitude range (with a certain amount
	 * of non-overlapping allowed if @a max_latitude_non_overlap_radians is specified).
	 * They may also be modified due to @a flatten_longitude_overlaps (see below).
	 *
	 * @a minimum_latitude_overlap_radians specifies the amount that @a from_polyline and @a to_polyline
	 * must overlap in latitude (North pole reference frame of @a rotation_axis).
	 *
	 * If @a max_latitude_non_overlap_radians is non-zero then an extra range of non-overlapping
	 * latitudes at the top and bottom of @a from_polyline and @a to_polyline is allowed - this is
	 * useful when one polyline is slightly above and/or below the other polyline (in terms of latitude).
	 * Otherwise only the common overlapping latitude region of both polylines is interpolated.
	 *
	 * Flattening longitude overlaps ensures longitudes of points of the left-most polyline
	 * (in North pole reference frame of @a rotation_axis) don't overlap right-most polyline.
	 * So @a flatten_longitude_overlaps determines whether, and how, to flatten overlaps in longitude:
	 *   1) FlattenLongitudeOverlaps::NO: Don't flatten overlaps.
	 *   2) FlattenLongitudeOverlaps::USE_FROM: For those point pairs where overlap occurs, the points
	 *      in @a from_polyline are copied to the corresponding (same latitude) points in @a to_polyline.
	 *   3) FlattenLongitudeOverlaps::USE_TO: For those point pairs where overlap occurs, the points
	 *      in @a to_polyline are copied to the corresponding (same latitude) points in @a from_polyline.
	 *
	 * Returns false if:
	 *  1) the polylines do not overlap in latitude by at least @a minimum_latitude_overlap_radians
	 *     radians (where North pole is @a rotation_axis), or
	 *  2) any corresponding pair of points (same latitude) of the polylines are separated by a
	 *     distance of more than @a max_distance_threshold_radians (if specified).
	 *
	 * Note that all returned polylines have the same number of points, and corresponding points
	 * have the same latitude (in North pole reference frame of @a rotation_axis) except those points
	 * in the non-overlapping latitude ranges (if @a maximum_latitude_non_overlap_radians is non-zero).
	 */
	bool
	interpolate(
			std::vector<PolylineOnSphere::non_null_ptr_to_const_type> &interpolated_polylines,
			const PolylineOnSphere::non_null_ptr_to_const_type &from_polyline,
			const PolylineOnSphere::non_null_ptr_to_const_type &to_polyline,
			const UnitVector3D &rotation_axis,
			const double &interpolate_resolution_radians,
			const double &minimum_latitude_overlap_radians = 0,
			const double &maximum_latitude_non_overlap_radians = 0,
			boost::optional<double> max_distance_threshold_radians = boost::none,
			FlattenLongitudeOverlaps::Value flatten_longitude_overlaps = FlattenLongitudeOverlaps::NO);

	/**
	 * Interpolates between two polylines along small circle arcs emanating from @a rotation_axis.
	 *
	 * This is the same as the other rotation @a interpolate overload except it accepts a sequence
	 * of interpolation factors in @a interpolate_ratios instead of a distance interval.
	 *
	 * Each interpolate factor in @a interpolate_ratios must be in the range [0,1].
	 * If @a interpolate_ratios is empty then true or false is still returned but there will be no
	 * polylines returned - this can be used as a way to test if @a from_polyline and @a to_polyline
	 * can possibly be interpolated between.
	 */
	bool
	interpolate(
			std::vector<PolylineOnSphere::non_null_ptr_to_const_type> &interpolated_polylines,
			const PolylineOnSphere::non_null_ptr_to_const_type &from_polyline,
			const PolylineOnSphere::non_null_ptr_to_const_type &to_polyline,
			const UnitVector3D &rotation_axis,
			const std::vector<double> &interpolate_ratios,
			const double &minimum_latitude_overlap_radians = 0,
			const double &maximum_latitude_non_overlap_radians = 0,
			boost::optional<double> max_distance_threshold_radians = boost::none,
			FlattenLongitudeOverlaps::Value flatten_longitude_overlaps = FlattenLongitudeOverlaps::NO);
}

#endif // GPLATES_MATHS_GEOMETRYINTERPOLATION_H
