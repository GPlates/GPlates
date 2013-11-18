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
	 * Returns the minimum angular distance between two @a GeometryOnSphere objects.
	 *
	 * Each geometry can be any of the four derived geometry types (PointOnSphere, MultiPointOnSphere,
	 * PolylineOnSphere and PolygonOnSphere) and they don't have to be the same type.
	 *
	 * If @a geometry1_interior_is_solid is true (and @a geometry1 is a PolygonOnSphere) then if any
	 * part of @a geometry2 intersects the interior of the @a geometry1 PolygonOnSphere the returned
	 * distance will be zero, otherwise the distance to the outline of the @a geometry1 PolygonOnSphere.
	 * If @a geometry2_interior_is_solid is true (and @a geometry2 is a PolygonOnSphere) then if any
	 * part of @a geometry1 intersects the interior of the @a geometry2 PolygonOnSphere the returned
	 * distance will be zero, otherwise the distance to the outline of the @a geometry2 PolygonOnSphere.
	 * NOTE: @a geometry1_interior_is_solid (@a geometry2_interior_is_solid) is ignored if
	 * @a geometry1 (@a geometry2) is not a PolygonOnSphere.
	 *
	 * If @a minimum_distance_threshold is specified then the returned distance will either be less
	 * than the threshold or AngularDistance::PI (maximum possible distance) to signify threshold exceeded.
	 * If the threshold is exceeded then then closest points are *not* stored in
	 * @a closest_positions (even if it's not none).
	 *
	 * If @a closest_positions is specified then the closest point on each geometry
	 * is stored in the unit vectors it references (unless the threshold is exceeded, if specified).
	 */
	AngularDistance
	minimum_distance(
			const GeometryOnSphere &geometry1,
			const GeometryOnSphere &geometry2,
			bool geometry1_interior_is_solid = false,
			bool geometry2_interior_is_solid = false,
			boost::optional<const AngularExtent &> minimum_distance_threshold = boost::none,
			boost::optional<
					boost::tuple<UnitVector3D &/*geometry1*/, UnitVector3D &/*geometry2*/>
							> closest_positions = boost::none);


	/**
	 * Returns the minimum angular distance between two points.
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
	 * Returns the minimum angular distance between a point and a multi-point.
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
	 * Returns the minimum angular distance between a point and a polyline.
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
	 * Returns the minimum angular distance between a point and a polygon.
	 *
	 * If @a polygon_interior_is_solid is true then anything intersecting the interior of @a polygon
	 * has a distance of zero, otherwise the distance to the polygon outline.
	 *
	 * If @a minimum_distance_threshold is specified then the returned distance will either be less
	 * than the threshold or AngularDistance::PI (maximum possible distance) to signify threshold exceeded.
	 * If the threshold is exceeded then then closest point is *not* stored in
	 * @a closest_position_on_polygon (even if it's not none).
	 *
	 * If @a closest_position_on_polygon is specified then the closest point on the polygon
	 * is stored in the unit vector it references (unless the threshold is exceeded, if specified).
	 * If @a polygon_interior_is_solid is true and the point is inside the polygon interior then
	 * the point position itself is returned as the closest point.
	 */
	AngularDistance
	minimum_distance(
			const PointOnSphere &point,
			const PolygonOnSphere &polygon,
			bool polygon_interior_is_solid = false,
			boost::optional<const AngularExtent &> minimum_distance_threshold = boost::none,
			boost::optional<UnitVector3D &> closest_position_on_polygon = boost::none);


	/**
	 * Returns the minimum angular distance between a point and a multi-point.
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
	 * Returns the minimum angular distance between a multi-point and a polyline.
	 *
	 * If @a minimum_distance_threshold is specified then the returned distance will either be less
	 * than the threshold or AngularDistance::PI (maximum possible distance) to signify threshold exceeded.
	 * If the threshold is exceeded then then closest points are *not* stored in
	 * @a closest_positions (even if it's not none).
	 *
	 * If @a closest_positions is specified then the closest point in the multi-point and on the polyline
	 * is stored in the unit vectors it references (unless the threshold is exceeded, if specified).
	 */
	AngularDistance
	minimum_distance(
			const MultiPointOnSphere &multipoint,
			const PolylineOnSphere &polyline,
			boost::optional<const AngularExtent &> minimum_distance_threshold = boost::none,
			boost::optional<
					boost::tuple<UnitVector3D &/*multipoint*/, UnitVector3D &/*polyline*/>
							> closest_positions = boost::none);


	/**
	 * Returns the minimum angular distance between a multi-point and a polygon.
	 *
	 * If @a polygon_interior_is_solid is true then anything intersecting the interior of @a polygon
	 * has a distance of zero, otherwise the distance to the polygon outline.
	 *
	 * If @a minimum_distance_threshold is specified then the returned distance will either be less
	 * than the threshold or AngularDistance::PI (maximum possible distance) to signify threshold exceeded.
	 * If the threshold is exceeded then then closest points are *not* stored in
	 * @a closest_positions (even if it's not none).
	 *
	 * If @a closest_positions is specified then the closest point in the multi-point and in/on the polygon
	 * is stored in the unit vectors it references (unless the threshold is exceeded, if specified).
	 * If @a polygon_interior_is_solid is true and any point is inside the polygon interior then
	 * that point position itself is returned as the closest point.
	 */
	AngularDistance
	minimum_distance(
			const MultiPointOnSphere &multipoint,
			const PolygonOnSphere &polygon,
			bool polygon_interior_is_solid = false,
			boost::optional<const AngularExtent &> minimum_distance_threshold = boost::none,
			boost::optional<
					boost::tuple<UnitVector3D &/*multipoint*/, UnitVector3D &/*polygon*/>
							> closest_positions = boost::none);


	/**
	 * Returns the minimum angular distance between a point and a polyline.
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


	/**
	 * Returns the minimum angular distance between a multi-point and a polyline.
	 *
	 * This function simply reverses the arguments of the other @a minimum_distance overload.
	 */
	AngularDistance
	minimum_distance(
			const PolylineOnSphere &polyline,
			const MultiPointOnSphere &multipoint,
			boost::optional<const AngularExtent &> minimum_distance_threshold = boost::none,
			boost::optional<
					boost::tuple<UnitVector3D &/*polyline*/, UnitVector3D &/*multipoint*/>
							> closest_positions = boost::none);


	/**
	 * Returns the minimum angular distance between two polylines.
	 *
	 * If @a minimum_distance_threshold is specified then the returned distance will either be less
	 * than the threshold or AngularDistance::PI (maximum possible distance) to signify threshold exceeded.
	 * If the threshold is exceeded then then closest points are *not* stored in
	 * @a closest_positions (even if it's not none).
	 *
	 * If @a closest_positions is specified then the closest point on each polyline
	 * is stored in the unit vectors it references (unless the threshold is exceeded, if specified).
	 */
	AngularDistance
	minimum_distance(
			const PolylineOnSphere &polyline1,
			const PolylineOnSphere &polyline2,
			boost::optional<const AngularExtent &> minimum_distance_threshold = boost::none,
			boost::optional<
					boost::tuple<UnitVector3D &/*polyline1*/, UnitVector3D &/*polyline2*/>
							> closest_positions = boost::none);


	/**
	 * Returns the minimum angular distance between a polyline and a polygon.
	 *
	 * If @a polygon_interior_is_solid is true then anything intersecting the interior of @a polygon
	 * has a distance of zero, otherwise the distance to the polygon outline.
	 *
	 * If @a minimum_distance_threshold is specified then the returned distance will either be less
	 * than the threshold or AngularDistance::PI (maximum possible distance) to signify threshold exceeded.
	 * If the threshold is exceeded then then closest points are *not* stored in
	 * @a closest_positions (even if it's not none).
	 *
	 * If @a closest_positions is specified then the closest points on the polyline and polygon
	 * are stored in the unit vectors it references (unless the threshold is exceeded, if specified).
	 * If @a polygon_interior_is_solid is true and the polyline is entirely inside the polygon interior
	 * then any point on the polyline could be returned as the closest point.
	 */
	AngularDistance
	minimum_distance(
			const PolylineOnSphere &polyline,
			const PolygonOnSphere &polygon,
			bool polygon_interior_is_solid = false,
			boost::optional<const AngularExtent &> minimum_distance_threshold = boost::none,
			boost::optional<
					boost::tuple<UnitVector3D &/*polyline*/, UnitVector3D &/*polygon*/>
							> closest_positions = boost::none);


	/**
	 * Returns the minimum angular distance between a point and a polygon.
	 *
	 * This function simply reverses the arguments of the other @a minimum_distance overload.
	 */
	inline
	AngularDistance
	minimum_distance(
			const PolygonOnSphere &polygon,
			const PointOnSphere &point,
			bool polygon_interior_is_solid = false,
			boost::optional<const AngularExtent &> minimum_distance_threshold = boost::none,
			boost::optional<UnitVector3D &> closest_position_on_polygon = boost::none)
	{
		return minimum_distance(
				point,
				polygon,
				polygon_interior_is_solid,
				minimum_distance_threshold,
				closest_position_on_polygon);
	}


	/**
	 * Returns the minimum angular distance between a multi-point and a polygon.
	 *
	 * This function simply reverses the arguments of the other @a minimum_distance overload.
	 */
	AngularDistance
	minimum_distance(
			const PolygonOnSphere &polygon,
			const MultiPointOnSphere &multipoint,
			bool polygon_interior_is_solid = false,
			boost::optional<const AngularExtent &> minimum_distance_threshold = boost::none,
			boost::optional<
					boost::tuple<UnitVector3D &/*polygon*/, UnitVector3D &/*multipoint*/>
							> closest_positions = boost::none);


	/**
	 * Returns the minimum angular distance between a polyline and a polygon.
	 *
	 * This function simply reverses the arguments of the other @a minimum_distance overload.
	 */
	AngularDistance
	minimum_distance(
			const PolygonOnSphere &polygon,
			const PolylineOnSphere &polyline,
			bool polygon_interior_is_solid = false,
			boost::optional<const AngularExtent &> minimum_distance_threshold = boost::none,
			boost::optional<
					boost::tuple<UnitVector3D &/*polygon*/, UnitVector3D &/*polyline*/>
							> closest_positions = boost::none);


	/**
	 * Returns the minimum angular distance between two polygons.
	 *
	 * If @a polygon1_interior_is_solid is true then if boundary of @a polygon2 intersects the interior
	 * of @a polygon1 the returned distance will be zero, otherwise the distance to the outline of @a polygon1.
	 * If @a polygon2_interior_is_solid is true then if boundary of @a polygon1 intersects the interior
	 * of @a polygon2 the returned distance will be zero, otherwise the distance to the outline of @a polygon2.
	 *
	 * If @a minimum_distance_threshold is specified then the returned distance will either be less
	 * than the threshold or AngularDistance::PI (maximum possible distance) to signify threshold exceeded.
	 * If the threshold is exceeded then then closest points are *not* stored in
	 * @a closest_positions (even if it's not none).
	 *
	 * If @a closest_positions is specified then the closest point on each polygon
	 * is stored in the unit vectors it references (unless the threshold is exceeded, if specified).
	 * If @a polygon1_interior_is_solid is true and @a polyline2 is entirely inside @a polygon1
	 * then any point on @a polyline2 could be returned as the closest point.
	 * If @a polygon2_interior_is_solid is true and @a polyline1 is entirely inside @a polygon2
	 * then any point on @a polyline1 could be returned as the closest point.
	 */
	AngularDistance
	minimum_distance(
			const PolygonOnSphere &polygon1,
			const PolygonOnSphere &polygon2,
			bool polygon1_interior_is_solid = false,
			bool polygon2_interior_is_solid = false,
			boost::optional<const AngularExtent &> minimum_distance_threshold = boost::none,
			boost::optional<
					boost::tuple<UnitVector3D &/*polygon1*/, UnitVector3D &/*polygon2*/>
							> closest_positions = boost::none);
}

#endif // GPLATES_MATHS_GEOMETRYDISTANCE_H
