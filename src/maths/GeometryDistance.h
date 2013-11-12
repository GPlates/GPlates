/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#ifndef GPLATES_MATHS_GEOMETRYDISTANCE_H
#define GPLATES_MATHS_GEOMETRYDISTANCE_H

#include <boost/optional.hpp>
#include <boost/tuple/tuple.hpp>

#include "AngularDistance.h"
#include "AngularExtent.h"
#include "GreatCircleArc.h"
#include "MultiPointOnSphere.h"
#include "PointOnSphere.h"
#include "PolygonOnSphere.h"
#include "PolylineOnSphere.h"
#include "UnitVector3D.h"


namespace GPlatesMaths
{
	/**
	 * Returns the minimum angular distance between two points - optionally within a minimum threshold distance.
	 *
	 * If @a minimum_distance_threshold is specified then the returned distance will either be less
	 * than the threshold or AngularDistance::PI (maximum possible distance) to signify threshold exceeded.
	 */
	AngularDistance
	minimum_distance(
			const PointOnSphere &point1,
			const PointOnSphere &point2,
			boost::optional<const AngularExtent &> minimum_distance_threshold = boost::none);


	/**
	 * Returns the minimum angular distance between a point and a multi-point, and optionally the
	 * closest point in the multi-point - optionally within a minimum threshold distance.
	 *
	 * If @a minimum_distance_threshold is specified then the returned distance will either be less
	 * than the threshold or AngularDistance::PI (maximum possible distance) to signify threshold exceeded.
	 * If the threshold is exceeded then then closest point is *not* stored in
	 * @a closest_position_in_multipoint (even if it's not none).
	 *
	 * If @a closest_position_in_multipoint is specified then the closest point in the multi-point
	 * is stored in the unit vector it references (unless the threshold is exceeded, if specified).
	 */
	AngularDistance
	minimum_distance(
			const PointOnSphere &point,
			const MultiPointOnSphere &multipoint,
			boost::optional<const AngularExtent &> minimum_distance_threshold = boost::none,
			boost::optional<UnitVector3D &> closest_position_in_multipoint = boost::none);


	/**
	 * Returns the minimum angular distance between a point and a polyline, and optionally the
	 * closest point on the polyline - optionally within a minimum threshold distance.
	 *
	 * If @a minimum_distance_threshold is specified then the returned distance will either be less
	 * than the threshold or AngularDistance::PI (maximum possible distance) to signify threshold exceeded.
	 * If the threshold is exceeded then then closest point is *not* stored in
	 * @a closest_position_on_polyline (even if it's not none).
	 *
	 * If @a closest_position_on_polyline is specified then the closest point on the polyline
	 * is stored in the unit vector it references (unless the threshold is exceeded, if specified).
	 */
	AngularDistance
	minimum_distance(
			const PointOnSphere &point,
			const PolylineOnSphere &polyline,
			boost::optional<const AngularExtent &> minimum_distance_threshold = boost::none,
			boost::optional<UnitVector3D &> closest_position_on_polyline = boost::none);


	/**
	 * Returns the minimum angular distance between a point and a multi-point, and optionally the
	 * closest point in the multi-point - optionally within a minimum threshold distance.
	 *
	 * This function simply reverses the arguments of the other @a minimum_distance overload.
	 */
	inline
	AngularDistance
	minimum_distance(
			const MultiPointOnSphere &multipoint,
			const PointOnSphere &point,
			boost::optional<const AngularExtent &> minimum_distance_threshold = boost::none,
			boost::optional<UnitVector3D &> closest_position_in_multipoint = boost::none)
	{
		return minimum_distance(point, multipoint, minimum_distance_threshold, closest_position_in_multipoint);
	}


	/**
	 * Returns the minimum angular distance between two multi-points.
	 *
	 * If @a minimum_distance_threshold is specified then the returned distance will either be less
	 * than the threshold or AngularDistance::PI (maximum possible distance) to signify threshold exceeded.
	 * If the threshold is exceeded then then closest points are *not* stored in
	 * @a closest_positions (even if it's not none).
	 *
	 * If @a closest_positions is specified then the closest point in each multi-point
	 * is stored in the unit vectors it references (unless the threshold is exceeded, if specified).
	 */
	AngularDistance
	minimum_distance(
			const MultiPointOnSphere &multipoint1,
			const MultiPointOnSphere &multipoint2,
			boost::optional<const AngularExtent &> minimum_distance_threshold = boost::none,
			boost::optional<
					boost::tuple<UnitVector3D &/*multipoint1*/, UnitVector3D &/*multipoint2*/>
							> closest_positions = boost::none);


	/**
	 * Returns the minimum angular distance between a point and a polyline, and optionally the
	 * closest point on the polyline - optionally within a minimum threshold distance.
	 *
	 * This function simply reverses the arguments of the other @a minimum_distance overload.
	 */
	inline
	AngularDistance
	minimum_distance(
			const PolylineOnSphere &polyline,
			const PointOnSphere &point,
			boost::optional<const AngularExtent &> minimum_distance_threshold = boost::none,
			boost::optional<UnitVector3D &> closest_position_on_polyline = boost::none)
	{
		return minimum_distance(point, polyline, minimum_distance_threshold, closest_position_on_polyline);
	}
}

#endif // GPLATES_MATHS_GEOMETRYDISTANCE_H
