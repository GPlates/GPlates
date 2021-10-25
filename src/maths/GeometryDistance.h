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
	 * If @a geometry1_interior_is_solid is true (and @a geometry1 is a PolygonOnSphere) and if any
	 * part of @a geometry2 overlaps the interior of the @a geometry1 PolygonOnSphere then the returned
	 * distance will be zero, otherwise...
	 * if @a geometry2_interior_is_solid is true (and @a geometry2 is a PolygonOnSphere) and if any
	 * part of @a geometry1 overlaps the interior of the @a geometry2 PolygonOnSphere then the returned
	 * distance will be zero, otherwise...
	 * the returned distance will be the minimum distance between the two geometries.
	 * NOTE: @a geometry1_interior_is_solid (@a geometry2_interior_is_solid) is ignored if
	 * @a geometry1 (@a geometry2) is not a PolygonOnSphere.
	 * Also note that the solid polygon interior region is defined similarly to point-in-polygon tests,
	 * that is crossing from outside the polygon to an interior region crosses an odd number of polygon
	 * edges (including edges of any polygon interior rings), and this holds even when the exterior and
	 * interior rings intersect each other.
	 *
	 * If @a minimum_distance_threshold is specified then the returned distance will either be less
	 * than the threshold or AngularDistance::PI (maximum possible distance) to signify threshold exceeded.
	 * If the threshold is exceeded then the closest points are *not* stored in @a closest_positions
	 * (even if it's not none) and the closest indices are not stored in @a closest_indices
	 * (even if it's not none).
	 *
	 * If @a closest_positions is specified then the closest point on each geometry (for polygons this means
	 * the polygon's exterior/interior rings, which we'll call *outline* to distinguish from the solid interior
	 * region) is stored in the unit vectors it references (unless the threshold is exceeded, if specified).
	 * Note that the closest points on polylines and polygon outlines can be anywhere on segments of
	 * the polyline/polygon (ie, it's not the nearest vertices of the polyline/polygon - it's the
	 * nearest points *on* the polyline/polygon).
	 * If both geometries are polyline/polygon and they intersect then the intersection point is returned
	 * (for both geometries).
	 * If both geometries are polyline/polygon and they intersect more than once then any intersection
	 * point is returned (for both geometries).
	 * Note that if @a geometry1_interior_is_solid is true and @a geometry1 is a polygon and @a geometry2
	 * is entirely inside the polygon (without intersecting its outline) then the threshold is not exceeded
	 * and hence @a closest_positions (if specified) will always store the closest point on @a geometry2
	 * and the corresponding closest point on the polygon outline.
	 * Note that if @a geometry2_interior_is_solid is true and @a geometry2 is a polygon and @a geometry1
	 * is entirely inside the polygon (without intersecting its outline) then the threshold is not exceeded
	 * and hence @a closest_positions (if specified) will always store the closest point on @a geometry1
	 * and the corresponding closest point on the polygon outline.
	 *
	 * If @a closest_indices is specified then the index of the closest *point* (for multi-points)
	 * or the index of the closest *segment* (for polylines and polygons) is stored in the integers it
	 * references (unless the threshold is exceeded, if specified).
	 * Note that for PointOnSphere geometries the index will always be zero.
	 * The point indices can be used with MultiPointOnSphere::get_point().
	 * The segment indices can be used with PolylineOnSphere::get_segment() or PolygonOnSphere::get_segment()
	 * (where, for polygons, the segment index can refer to an interior ring).
	 * Note that if @a geometry1_interior_is_solid is true and @a geometry1 is a polygon and @a geometry2
	 * is entirely inside the polygon (without intersecting its outline) then the threshold is not exceeded
	 * and hence @a closest_indices (if specified) will always store indices.
	 * Note that if @a geometry2_interior_is_solid is true and @a geometry2 is a polygon and @a geometry1
	 * is entirely inside the polygon (without intersecting its outline) then the threshold is not exceeded
	 * and hence @a closest_indices (if specified) will always store indices.
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
							> closest_positions = boost::none,
			boost::optional<
					boost::tuple<unsigned int &/*geometry1*/, unsigned int &/*geometry2*/>
							> closest_indices = boost::none);


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
	 * If the threshold is exceeded then the closest point is *not* stored in
	 * @a closest_position_in_multipoint (even if it's not none) and the closest position index is
	 * *not* stored in @a closest_position_index_in_multipoint (even if it's not none).
	 *
	 * If @a closest_position_in_multipoint is specified then the closest point in the multi-point
	 * is stored in the unit vector it references (unless the threshold is exceeded, if specified).
	 *
	 * If @a closest_position_index_in_multipoint is specified then the index of the closest point in the
	 * multi-point is stored in the integer it references (unless the threshold is exceeded, if specified).
	 * The index can be used with MultiPointOnSphere::get_point().
	 */
	AngularDistance
	minimum_distance(
			const PointOnSphere &point,
			const MultiPointOnSphere &multipoint,
			boost::optional<const AngularExtent &> minimum_distance_threshold = boost::none,
			boost::optional<UnitVector3D &> closest_position_in_multipoint = boost::none,
			boost::optional<unsigned int &> closest_position_index_in_multipoint = boost::none);


	/**
	 * Returns the minimum angular distance between a point and a polyline.
	 *
	 * If @a minimum_distance_threshold is specified then the returned distance will either be less
	 * than the threshold or AngularDistance::PI (maximum possible distance) to signify threshold exceeded.
	 * If the threshold is exceeded then the closest point is *not* stored in
	 * @a closest_position_on_polyline (even if it's not none) and the closest segment index is
	 * *not* stored in @a closest_segment_index_in_polyline (even if it's not none).
	 *
	 * If @a closest_position_on_polyline is specified then the closest point on the polyline
	 * is stored in the unit vector it references (unless the threshold is exceeded, if specified).
	 * Note that this closest point can be anywhere on a segment of the polyline (ie, it's not
	 * the nearest vertex of polyline - it's the nearest point *on* the polyline).
	 *
	 * If @a closest_segment_index_in_polyline is specified then the index of the closest segment
	 * (great circle arc on which the closest point lies) in the polyline is stored in the integer
	 * it references (unless the threshold is exceeded, if specified).
	 * The index can be used with PolylineOnSphere::get_segment().
	 */
	AngularDistance
	minimum_distance(
			const PointOnSphere &point,
			const PolylineOnSphere &polyline,
			boost::optional<const AngularExtent &> minimum_distance_threshold = boost::none,
			boost::optional<UnitVector3D &> closest_position_on_polyline = boost::none,
			boost::optional<unsigned int &> closest_segment_index_in_polyline = boost::none);


	/**
	 * Returns the minimum angular distance between a point and a polygon.
	 *
	 * If @a polygon_interior_is_solid is true then anything overlapping the interior of @a polygon
	 * has a distance of zero (and AngularDistance::ZERO), otherwise the distance to the polygon outline.
	 * Note that the solid polygon interior region is defined similarly to point-in-polygon tests,
	 * that is crossing from outside the polygon to an interior region crosses an odd number of polygon
	 * edges (including edges of any polygon interior rings), and this holds even when the exterior and
	 * interior rings intersect each other.
	 *
	 * If @a minimum_distance_threshold is specified then the returned distance will either be less
	 * than the threshold or AngularDistance::PI (maximum possible distance) to signify threshold exceeded.
	 * If the threshold is exceeded then the closest point is *not* stored in
	 * @a closest_position_on_polygon_outline (even if it's not none) and the closest segment index is
	 * *not* stored in @a closest_segment_index_in_polygon (even if it's not none).
	 *
	 * If @a closest_position_on_polygon_outline is specified then the closest point on the polygon
	 * *outline* is stored in the unit vector it references (unless the threshold is exceeded, if specified).
	 * Note that this closest point can be anywhere on a segment of the polygon outline (ie, it's not
	 * the nearest vertex of polygon - it's the nearest point *on* the polygon outline).
	 * Note that if @a polygon_interior_is_solid is true and the point is inside the polygon then
	 * the threshold is not exceeded and hence @a closest_position_on_polygon_outline (if specified)
	 * will always store the closest point on the polygon outline.
	 *
	 * If @a closest_segment_index_in_polygon is specified then the index of the closest segment
	 * (great circle arc on which the closest point on polygon *outline* lies) in the polygon is
	 * stored in the integer it references (unless the threshold is exceeded, if specified).
	 * The index can be used with PolygonOnSphere::get_segment().
	 * Note that the segment index can refer to an interior ring.
	 * Note that if @a polygon_interior_is_solid is true and the point is inside the polygon then
	 * the threshold is not exceeded and hence @a closest_segment_index_in_polygon (if specified)
	 * will always store the index of the closest segment in the polygon.
	 */
	AngularDistance
	minimum_distance(
			const PointOnSphere &point,
			const PolygonOnSphere &polygon,
			bool polygon_interior_is_solid = false,
			boost::optional<const AngularExtent &> minimum_distance_threshold = boost::none,
			boost::optional<UnitVector3D &> closest_position_on_polygon_outline = boost::none,
			boost::optional<unsigned int &> closest_segment_index_in_polygon = boost::none);


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
			boost::optional<UnitVector3D &> closest_position_in_multipoint = boost::none,
			boost::optional<unsigned int &> closest_position_index_in_multipoint = boost::none)
	{
		return minimum_distance(
				point,
				multipoint,
				minimum_distance_threshold,
				closest_position_in_multipoint,
				closest_position_index_in_multipoint);
	}


	/**
	 * Returns the minimum angular distance between two multi-points.
	 *
	 * If @a minimum_distance_threshold is specified then the returned distance will either be less
	 * than the threshold or AngularDistance::PI (maximum possible distance) to signify threshold exceeded.
	 * If the threshold is exceeded then the closest points are *not* stored in
	 * @a closest_positions (even if it's not none) and the closest position indices are
	 * *not* stored in @a closest_position_indices (even if it's not none).
	 *
	 * If @a closest_positions is specified then the closest point in each multi-point
	 * is stored in the unit vectors it references (unless the threshold is exceeded, if specified).
	 *
	 * If @a closest_position_indices is specified then the index of the closest point in each
	 * multi-point is stored in the integers it references (unless the threshold is exceeded, if specified).
	 * The indices can be used with MultiPointOnSphere::get_point().
	 */
	AngularDistance
	minimum_distance(
			const MultiPointOnSphere &multipoint1,
			const MultiPointOnSphere &multipoint2,
			boost::optional<const AngularExtent &> minimum_distance_threshold = boost::none,
			boost::optional<
					boost::tuple<UnitVector3D &/*multipoint1*/, UnitVector3D &/*multipoint2*/>
							> closest_positions = boost::none,
			boost::optional<
					boost::tuple<unsigned int &/*multipoint1*/, unsigned int &/*multipoint2*/>
							> closest_position_indices = boost::none);


	/**
	 * Returns the minimum angular distance between a multi-point and a polyline.
	 *
	 * If @a minimum_distance_threshold is specified then the returned distance will either be less
	 * than the threshold or AngularDistance::PI (maximum possible distance) to signify threshold exceeded.
	 * If the threshold is exceeded then the closest points are *not* stored in
	 * @a closest_positions (even if it's not none) and the closest indices are *not* stored in
	 * @a closest_indices (even if it's not none).
	 *
	 * If @a closest_positions is specified then the closest point in the multi-point and the closest
	 * point on the polyline are stored in the unit vectors it references (unless the threshold is
	 * exceeded, if specified).
	 * Note that the closest point on the polyline can be anywhere on a segment of the polyline
	 * (ie, it's not the nearest vertex of polyline - it's the nearest point *on* the polyline).
	 *
	 * If @a closest_indices is specified then the index of the closest point in the multi-point and
	 * the index of the closest *segment* on the polyline is stored in the integers it references
	 * (unless the threshold is exceeded, if specified).
	 * Note that the closest point on the polyline lies on the closest segment on the polyline.
	 * The multi-point index can be used with MultiPointOnSphere::get_point().
	 * The polyline index can be used with PolylineOnSphere::get_segment().
	 */
	AngularDistance
	minimum_distance(
			const MultiPointOnSphere &multipoint,
			const PolylineOnSphere &polyline,
			boost::optional<const AngularExtent &> minimum_distance_threshold = boost::none,
			boost::optional<
					boost::tuple<UnitVector3D &/*multipoint*/, UnitVector3D &/*polyline*/>
							> closest_positions = boost::none,
			boost::optional<
					boost::tuple<unsigned int &/*multipoint*/, unsigned int &/*polyline*/>
							> closest_indices = boost::none);


	/**
	 * Returns the minimum angular distance between a multi-point and a polygon.
	 *
	 * If @a polygon_interior_is_solid is true then anything overlapping the interior of @a polygon
	 * has a distance of zero (and AngularDistance::ZERO), otherwise the distance to the polygon outline.
	 * Note that the solid polygon interior region is defined similarly to point-in-polygon tests,
	 * that is crossing from outside the polygon to an interior region crosses an odd number of polygon
	 * edges (including edges of any polygon interior rings), and this holds even when the exterior and
	 * interior rings intersect each other.
	 *
	 * If @a minimum_distance_threshold is specified then the returned distance will either be less
	 * than the threshold or AngularDistance::PI (maximum possible distance) to signify threshold exceeded.
	 * If the threshold is exceeded then the closest points are *not* stored in
	 * @a closest_positions (even if it's not none) and the closest indices are *not* stored in
	 * @a closest_indices (even if it's not none).
	 *
	 * If @a closest_positions is specified then the closest point in the multi-point and the closest
	 * point on the polygon *outline* is stored in the unit vectors it references (unless the threshold
	 * is exceeded, if specified).
	 * Note that the closest point on the polygon outline can be anywhere on a segment of the polygon
	 * outline (ie, it's not the nearest vertex of polygon - it's the nearest point *on* the polygon outline).
	 * If @a polygon_interior_is_solid is true and more than one point is inside the polygon interior
	 * then any point is returned (as the closest point in the multi-point) along with the corresponding
	 * closest point on the polygon outline.
	 * Note that if @a polygon_interior_is_solid is true and any point is inside the polygon then the
	 * threshold is not exceeded and hence @a closest_positions (if specified) will always store a
	 * closest point in the multi-point and the corresponding closest point on the polygon outline.
	 *
	 * If @a closest_indices is specified then the index of the closest point in the multi-point and
	 * the index of the closest *segment* on the polygon *outline* (the great circle arc on which the
	 * closest point on polygon *outline* lies) is stored in the integers it references (unless the
	 * threshold is exceeded, if specified).
	 * The multi-point index can be used with MultiPointOnSphere::get_point().
	 * The polygon index can be used with PolygonOnSphere::get_segment().
	 * Note that the segment index can refer to an interior ring.
	 * Note that if @a polygon_interior_is_solid is true and any point is inside the polygon then
	 * the threshold is not exceeded and hence @a closest_indices (if specified) will always store
	 * the index of the closest point in the multi-point and the index of the closest segment on
	 * the polygon outline.
	 */
	AngularDistance
	minimum_distance(
			const MultiPointOnSphere &multipoint,
			const PolygonOnSphere &polygon,
			bool polygon_interior_is_solid = false,
			boost::optional<const AngularExtent &> minimum_distance_threshold = boost::none,
			boost::optional<
					boost::tuple<UnitVector3D &/*multipoint*/, UnitVector3D &/*polygon*/>
							> closest_positions = boost::none,
			boost::optional<
					boost::tuple<unsigned int &/*multipoint*/, unsigned int &/*polygon*/>
							> closest_indices = boost::none);


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
			boost::optional<UnitVector3D &> closest_position_on_polyline = boost::none,
			boost::optional<unsigned int &> closest_segment_index_in_polyline = boost::none)
	{
		return minimum_distance(
				point,
				polyline,
				minimum_distance_threshold,
				closest_position_on_polyline,
				closest_segment_index_in_polyline);
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
							> closest_positions = boost::none,
			boost::optional<
					boost::tuple<unsigned int &/*polyline*/, unsigned int &/*multipoint*/>
							> closest_indices = boost::none);


	/**
	 * Returns the minimum angular distance between two polylines.
	 *
	 * If the polylines intersect then AngularDistance::ZERO is returned.
	 *
	 * If @a minimum_distance_threshold is specified then the returned distance will either be less
	 * than the threshold or AngularDistance::PI (maximum possible distance) to signify threshold exceeded.
	 * If the threshold is exceeded then the closest points are *not* stored in
	 * @a closest_positions (even if it's not none) and the closest segment indices are *not* stored
	 * in @a closest_segment_indices (even if it's not none).
	 *
	 * If @a closest_positions is specified then the closest point on each polyline is stored in the
	 * unit vectors it references (unless the threshold is exceeded, if specified).
	 * Note that the closest points on the polylines can be anywhere on segments of the polylines
	 * (ie, it's not the nearest vertices of the polylines - it's the nearest points *on* the polylines).
	 * If the polylines intersect then the intersection point is returned for both polylines.
	 * If the polylines intersect more than once then any intersection point is returned (for both polylines).
	 *
	 * If @a closest_segment_indices is specified then the index of the closest *segment* on @a polyline1
	 * and the index of the closest *segment* on @a polyline2 is stored in the integers it references
	 * (unless the threshold is exceeded, if specified).
	 * The closest point on each polyline lies on the closest segment of each polyline.
	 * The segment indices can be used with PolylineOnSphere::get_segment().
	 */
	AngularDistance
	minimum_distance(
			const PolylineOnSphere &polyline1,
			const PolylineOnSphere &polyline2,
			boost::optional<const AngularExtent &> minimum_distance_threshold = boost::none,
			boost::optional<
					boost::tuple<UnitVector3D &/*polyline1*/, UnitVector3D &/*polyline2*/>
							> closest_positions = boost::none,
			boost::optional<
					boost::tuple<unsigned int &/*polyline1*/, unsigned int &/*polyline2*/>
							> closest_segment_indices = boost::none);


	/**
	 * Returns the minimum angular distance between a polyline and a polygon.
	 *
	 * If the polyline and polygon intersect then AngularDistance::ZERO is returned.
	 *
	 * If @a polygon_interior_is_solid is true then anything overlapping the interior of @a polygon
	 * has a distance of zero (AngularDistance::ZERO), otherwise the distance to the polygon outline.
	 * Note that the solid polygon interior region is defined similarly to point-in-polygon tests,
	 * that is crossing from outside the polygon to an interior region crosses an odd number of polygon
	 * edges (including edges of any polygon interior rings), and this holds even when the exterior and
	 * interior rings intersect each other.
	 *
	 * If @a minimum_distance_threshold is specified then the returned distance will either be less
	 * than the threshold or AngularDistance::PI (maximum possible distance) to signify threshold exceeded.
	 * If the threshold is exceeded then the closest points are *not* stored in @a closest_positions
	 * (even if it's not none) and the closest segment indices are *not* stored in
	 * @a closest_segment_indices (even if it's not none).
	 *
	 * If @a closest_positions is specified then the closest point on the polyline and the closest
	 * point on the polygon *outline* are stored in the unit vectors it references (unless the
	 * threshold is exceeded, if specified).
	 * Note that the closest points on the polyline and polygon outline can be anywhere on their segments
	 * (ie, it's not the nearest vertices of the polyline and polygon outline - it's the nearest points
	 * *on* the polyline and polygon outline).
	 * If the polyline and polygon *outline* intersect then the intersection point is returned for both.
	 * If the polyline and polygon *outline* intersect more than once then any intersection point is
	 * returned (for both).
	 * Note that if @a polygon_interior_is_solid is true and the polyline is entirely inside the
	 * polygon interior (without intersecting its outline) then the threshold is not exceeded and
	 * hence @a closest_positions (if specified) will always store the closest point on the polyline
	 * and the closest point on the polygon outline.
	 *
	 * If @a closest_segment_indices is specified then the index of the closest *segment* on the polyline
	 * and the index of the closest *segment* on the polygon *outline* are stored in the integers it
	 * references (unless the threshold is exceeded, if specified).
	 * The closest point on the polyline lies on the closest segment on the polyline.
	 * The closest point on the polygon outline lies on the closest segment on the polygon outline.
	 * The polyline segment index can be used with PolylineOnSphere::get_segment().
	 * The polygon segment index can be used with PolygonOnSphere::get_segment() - note that the
	 * segment index can refer to an interior ring.
	 * Note that if @a polygon_interior_is_solid is true and the polyline is entirely inside the
	 * polygon interior (without intersecting its outline) then the threshold is not exceeded and
	 * hence @a closest_segment_indices (if specified) will always store the closest segment on the
	 * polyline and the closest segment on the polygon outline.
	 */
	AngularDistance
	minimum_distance(
			const PolylineOnSphere &polyline,
			const PolygonOnSphere &polygon,
			bool polygon_interior_is_solid = false,
			boost::optional<const AngularExtent &> minimum_distance_threshold = boost::none,
			boost::optional<
					boost::tuple<UnitVector3D &/*polyline*/, UnitVector3D &/*polygon*/>
							> closest_positions = boost::none,
			boost::optional<
					boost::tuple<unsigned int &/*polyline*/, unsigned int &/*polygon*/>
							> closest_segment_indices = boost::none);


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
			boost::optional<UnitVector3D &> closest_position_on_polygon = boost::none,
			boost::optional<unsigned int &> closest_segment_index_in_polygon = boost::none)
	{
		return minimum_distance(
				point,
				polygon,
				polygon_interior_is_solid,
				minimum_distance_threshold,
				closest_position_on_polygon,
				closest_segment_index_in_polygon);
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
							> closest_positions = boost::none,
			boost::optional<
					boost::tuple<unsigned int &/*polygon*/, unsigned int &/*multipoint*/>
							> closest_indices = boost::none);


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
							> closest_positions = boost::none,
			boost::optional<
					boost::tuple<unsigned int &/*polygon*/, unsigned int &/*polyline*/>
							> closest_segment_indices = boost::none);


	/**
	 * Returns the minimum angular distance between two polygons.
	 *
	 * If @a polygon1_interior_is_solid is true and the boundary of @a polygon2 overlaps the interior
	 * of @a polygon1 then the returned distance will be zero, otherwise...
	 * if @a polygon2_interior_is_solid is true and the boundary of @a polygon1 overlaps the interior
	 * of @a polygon2 then the returned distance will be zero, otherwise...
	 * the returned distance will be the minimum distance between the polygon outlines.
	 * Note that the solid polygon interior region is defined similarly to point-in-polygon tests,
	 * that is crossing from outside the polygon to an interior region crosses an odd number of polygon
	 * edges (including edges of any polygon interior rings), and this holds even when the exterior and
	 * interior rings intersect each other.
	 *
	 * If @a minimum_distance_threshold is specified then the returned distance will either be less
	 * than the threshold or AngularDistance::PI (maximum possible distance) to signify threshold exceeded.
	 * If the threshold is exceeded then the closest points are *not* stored in @a closest_positions
	 * (even if it's not none) and the closest segment indices are *not* stored in
	 * @a closest_segment_indices (even if it's not none).
	 *
	 * If @a closest_positions is specified then the closest point on each polygon *outline* is stored
	 * in the unit vectors it references (unless the threshold is exceeded, if specified).
	 * Note that the closest points on the polygon outlines can be anywhere on segments of the polygons
	 * (ie, it's not the nearest vertices of the polygons - it's the nearest points *on* the polygons).
	 * If the polygon outlines intersect then the intersection point is returned for both polygons.
	 * If the polygon outlines intersect more than once then any intersection point is returned (for both polygons).
	 * Note that if @a polygon1_interior_is_solid is true and @a polyline2 is entirely inside @a polygon1
	 * (without intersecting its outline) then the threshold is not exceeded and hence @a closest_positions
	 * (if specified) will always store the closest point on each polygon outline.
	 * Note that if @a polygon2_interior_is_solid is true and @a polyline1 is entirely inside @a polygon2
	 * (without intersecting its outline) then the threshold is not exceeded and hence @a closest_positions
	 * (if specified) will always store the closest point on each polygon outline.
	 *
	 * If @a closest_segment_indices is specified then the index of the closest *segment* on each polygon
	 * is stored in the integers it references (unless the threshold is exceeded, if specified).
	 * The closest point on each polygon outline lies on the closest segment of each polygon.
	 * The segment indices can be used with PolygonOnSphere::get_segment().
	 * Note that the segment indices can refer to interior rings.
	 * Note that if @a polygon1_interior_is_solid is true and @a polyline2 is entirely inside @a polygon1
	 * (without intersecting its outline) then the threshold is not exceeded and hence @a closest_segment_indices
	 * (if specified) will always store the closest segment on each polygon.
	 * Note that if @a polygon2_interior_is_solid is true and @a polyline1 is entirely inside @a polygon2
	 * (without intersecting its outline) then the threshold is not exceeded and hence @a closest_segment_indices
	 * (if specified) will always store the closest segment on each polygon.
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
							> closest_positions = boost::none,
			boost::optional<
					boost::tuple<unsigned int &/*polygon1*/, unsigned int &/*polygon2*/>
							> closest_segment_indices = boost::none);
}

#endif // GPLATES_MATHS_GEOMETRYDISTANCE_H
