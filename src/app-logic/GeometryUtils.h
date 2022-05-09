/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
#ifndef GPLATES_APP_LOGIC_GEOMETRY_UTILS_H
#define GPLATES_APP_LOGIC_GEOMETRY_UTILS_H

#include <vector>
#include <boost/none.hpp>
#include <boost/optional.hpp>
#include <QDebug>

#include "maths/GeometryOnSphere.h"
#include "maths/GeometryType.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolygonOrientation.h"
#include "maths/PolylineOnSphere.h"

#include "model/FeatureHandle.h"
#include "model/PropertyValue.h"
#include "model/TopLevelProperty.h"


namespace GPlatesAppLogic
{
	namespace GeometryUtils
	{
		/**
		 * Returns the specified geometry-on-sphere as a point-on-sphere.
		 */
		boost::optional<const GPlatesMaths::PointOnSphere &>
		get_point_on_sphere(
				const GPlatesMaths::GeometryOnSphere &geometry_on_sphere);

		/**
		 * Returns the specified geometry-on-sphere as a multi-point-on-sphere.
		 */
		boost::optional<GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type>
		get_multi_point_on_sphere(
				const GPlatesMaths::GeometryOnSphere &geometry_on_sphere);

		/**
		 * Returns the specified geometry-on-sphere as a polyline-on-sphere.
		 */
		boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>
		get_polyline_on_sphere(
				const GPlatesMaths::GeometryOnSphere &geometry_on_sphere);

		/**
		 * Returns the specified geometry-on-sphere as a polygon-on-sphere.
		 */
		boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type>
		get_polygon_on_sphere(
				const GPlatesMaths::GeometryOnSphere &geometry_on_sphere);


		/**
		 * Returns the type of the specified @a GeometryOnSphere object.
		 */
		GPlatesMaths::GeometryType::Value
		get_geometry_type(
				const GPlatesMaths::GeometryOnSphere &geometry_on_sphere);


		/**
		 * Returns the number of points in the specified geometry.
		 *
		 * If @a geometry_on_sphere is a polygon then both its *exterior* ring and *interior* ring
		 * points are counted.
		 */
		unsigned int
		get_num_geometry_points(
				const GPlatesMaths::GeometryOnSphere &geometry_on_sphere);

		/**
		 * Returns the number of points in the specified geometry.
		 *
		 * If @a geometry_on_sphere is a polygon then only its *exterior* ring points are counted.
		 */
		unsigned int
		get_num_geometry_exterior_points(
				const GPlatesMaths::GeometryOnSphere &geometry_on_sphere);


		/**
		 * Copies the @a PointOnSphere points from @a geometry_on_sphere to the @a points array.
		 *
		 * If @a geometry_on_sphere is a polygon then both its *exterior* ring and *interior* ring
		 * points are copied.
		 *
		 * Does not clear @a points - just appends whatever points it finds in @a geometry_on_sphere.
		 *
		 * If @a reverse_points is true then the order of the points in @a geometry_on_sphere
		 * are reversed before appending to @a points.
		 *
		 * Also returns the type of the specified @a GeometryOnSphere object.
		 */
		GPlatesMaths::GeometryType::Value
		get_geometry_points(
				const GPlatesMaths::GeometryOnSphere &geometry_on_sphere,
				std::vector<GPlatesMaths::PointOnSphere> &points,
				bool reverse_points = false);

		/**
		 * Same as @a get_geometry_points except only the points in the specified range are returned.
		 *
		 * Note that [@a start_vertex_index, @a end_vertex_index) is a half-range where @a end_vertex_index
		 * is one past the last vertex to be returned (this is similar to begin/end iterators).
		 *
		 * If @a start_vertex_index and @a end_vertex_index are equal then no points are returned.
		 */
		GPlatesMaths::GeometryType::Value
		get_geometry_points_range(
				const GPlatesMaths::GeometryOnSphere &geometry_on_sphere,
				std::vector<GPlatesMaths::PointOnSphere> &points,
				unsigned int start_vertex_index,
				unsigned int end_vertex_index,
				bool reverse_points = false);

		/**
		 * Same as @a get_geometry_points except, if @a geometry_on_sphere is a polygon then only
		 * its *exterior* ring points are copied.
		 */
		GPlatesMaths::GeometryType::Value
		get_geometry_exterior_points(
				const GPlatesMaths::GeometryOnSphere &geometry_on_sphere,
				std::vector<GPlatesMaths::PointOnSphere> &points,
				bool reverse_points = false);

		/**
		 * Same as @a get_geometry_exterior_points except only the points in the specified range are returned.
		 *
		 * Note that [@a start_vertex_index, @a end_vertex_index) is a half-range where @a end_vertex_index
		 * is one past the last vertex to be returned (this is similar to begin/end iterators).
		 *
		 * If @a start_vertex_index and @a end_vertex_index are equal then no points are returned.
		 */
		GPlatesMaths::GeometryType::Value
		get_geometry_exterior_points_range(
				const GPlatesMaths::GeometryOnSphere &geometry_on_sphere,
				std::vector<GPlatesMaths::PointOnSphere> &points,
				unsigned int start_vertex_index,
				unsigned int end_vertex_index,
				bool reverse_points = false);

		/**
		 * Returns the end points of @a geometry_on_sphere.
		 *
		 * If @a geometry_on_sphere is a polygon then only the *exterior* ring is considered.
		 *
		 * If @a reverse_points is true then the order of the returned end points is reversed.
		 *
		 * This is faster than calling @a get_geometry_exterior_points and then picking out the
		 * first and last points as it doesn't retrieve all the points.
		 */
		std::pair<
				GPlatesMaths::PointOnSphere/*start point*/,
				GPlatesMaths::PointOnSphere/*end point*/>
		get_geometry_exterior_end_points(
				const GPlatesMaths::GeometryOnSphere &geometry_on_sphere,
				bool reverse_points = false);


		/**
		 * Returns the small circle that bounds the specified geometry.
		 *
		 * Returns boost::none if the geometry is a @a PointOnSphere otherwise it returns
		 * a valid bounding small circle.
		 */
		boost::optional<const GPlatesMaths::BoundingSmallCircle &>
		get_geometry_bounding_small_circle(
				const GPlatesMaths::GeometryOnSphere &geometry_on_sphere);


		/**
		 * Converts the specified geometry to a @a MultiPointOnSphere by treating storing the
		 * geometry points as a multi-point.
		 *
		 * If @a include_polygon_interior_ring_points is true (default) and the geometry is a
		 * polygon then the points in its interior rings (if any) are added to the multi-point.
		 */
		GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type
		convert_geometry_to_multi_point(
				const GPlatesMaths::GeometryOnSphere &geometry_on_sphere,
				bool include_polygon_interior_ring_points = true);


		/**
		 * Converts the specified geometry to a @a PolylineOnSphere if it is a polygon or multipoint
		 * (or already a polyline) by treating the geometry points as a linear list of polyline points.
		 *
		 * Returns boost::none if the specified geometry has less than two points
		 * (ie, not enough to form a polyline) or the specified geometry is a point geometry.
		 * If @a exclude_polygons_with_interior_rings is true (default) and the geometry is a
		 * polygon with interior rings then returns boost::none (since it is not obvious how to
		 * create a polyline from multiple rings). If it is false then only the exterior ring
		 * is converted to a polyline (the interior rings are ignored). If the last exterior ring
		 * segment is *not* zero length (which is usually the case) then an extra segment from the
		 * last vertex to first vertex of exterior ring is created as the final polyline segment.
		 */
		boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>
		convert_geometry_to_polyline(
				const GPlatesMaths::GeometryOnSphere &geometry_on_sphere,
				bool exclude_polygons_with_interior_rings = true);


		/**
		 * Same as @a convert_geometry_to_polyline except, if geometry has less than two points then
		 * duplicates last point, or if geometry is a polygon then only the exterior ring is
		 * converted to a polyline (the interior rings are ignored).
		 *
		 * This turns a point (or multi-point containing a single point) into a polyline with
		 * two identical vertices.
		 */
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type
		force_convert_geometry_to_polyline(
				const GPlatesMaths::GeometryOnSphere &geometry_on_sphere);


		/**
		 * Converts the specified geometry to a @a PolygonOnSphere if it is a polyline or multipoint
		 * (or already a polygon) by treating the geometry points as a linear list of polygon points.
		 *
		 * Returns boost::none if the specified geometry has less than three points
		 * (ie, not enough to form a polygon) or the specified geometry is a point geometry.
		 */
		boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type>
		convert_geometry_to_polygon(
				const GPlatesMaths::GeometryOnSphere &geometry_on_sphere);


		/**
		 * Same as @a convert_geometry_to_polygon except, if geometry has less than three points then,
		 * duplicates last point until has three points.
		 *
		 * This turns a point (or multi-point containing a single point) into a polygon with three
		 * identical vertices. And turns a polyline (or multi-point) with two points into a polygon
		 * that has no internal area (looks like a single line segment).
		 */
		GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type
		force_convert_geometry_to_polygon(
				const GPlatesMaths::GeometryOnSphere &geometry_on_sphere);


		/**
		 * Convert the polygon to the specified orientation (if necessary).
		 *
		 * If the polygon is already the correct orientation then it is simply returned.
		 */
		GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type
		convert_polygon_to_oriented_polygon(
				const GPlatesMaths::PolygonOnSphere &polygon_on_sphere,
				GPlatesMaths::PolygonOrientation::Orientation polygon_orientation,
				bool ensure_interior_ring_orientation_opposite_to_exterior_ring = true);


		/**
		 * Converts @a geometry to the specified orientation if it's a polygon and has
		 * a different orientation, otherwise @a geometry is returned.
		 *
		 * Note that for a point, multipoint or polyline this simply returns the geometry.
		 */
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		convert_geometry_to_oriented_geometry(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
				GPlatesMaths::PolygonOrientation::Orientation polygon_orientation,
				bool ensure_interior_ring_orientation_opposite_to_exterior_ring = true);


		/**
		 * Returns the geometry contained within the specified property.
		 *
		 * Returns boost::none if the property value is not geometric.
		 *
		 * @a reconstruction_time only applies to time-dependent properties in which case the
		 * value of the property at the specified time is returned.
		 * It is effectively ignored for constant-valued properties.
		 */
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
		get_geometry_from_property(
				const GPlatesModel::FeatureHandle::iterator &property,
				const double &reconstruction_time = 0);


		/**
		 * Returns the geometry contained within the specified property.
		 *
		 * Returns boost::none if the property value is not geometric.
		 *
		 * @a reconstruction_time only applies to time-dependent properties in which case the
		 * value of the property at the specified time is returned.
		 * It is effectively ignored for constant-valued properties.
		 */
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
		get_geometry_from_property(
				const GPlatesModel::TopLevelProperty::non_null_ptr_type &property,
				const double &reconstruction_time = 0);


		/**
		 * Returns the geometry contained within the specified property value.
		 *
		 * Returns boost::none if the property value is not geometric.
		 *
		 * @a reconstruction_time only applies to time-dependent properties in which case the
		 * value of the property at the specified time is returned.
		 * It is effectively ignored for constant-valued properties.
		 */
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
		get_geometry_from_property_value(
				const GPlatesModel::PropertyValue &property_value,
				const double &reconstruction_time = 0);


		/**
		 * Visits a @a geometry and attempts to create a suitable geometric @a PropertyValue using it.
		 */
		GPlatesModel::PropertyValue::non_null_ptr_type
		create_geometry_property_value(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry);
		
		/**
		 * Creates a suitable geometric @a PropertyValue using @a point.
		 */
		GPlatesModel::PropertyValue::non_null_ptr_type
		create_point_geometry_property_value(
				const GPlatesMaths::PointOnSphere &point);

		/**
		 * Creates a suitable geometric @a PropertyValue using @a multipoint.
		 */
		GPlatesModel::PropertyValue::non_null_ptr_type
		create_multipoint_geometry_property_value(
				const GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type &multipoint);

		/**
		 * Creates a suitable geometric @a PropertyValue using @a polyline.
		 */
		GPlatesModel::PropertyValue::non_null_ptr_type
		create_polyline_geometry_property_value(
				const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type &polyline);

		/**
		 * Creates a suitable geometric @a PropertyValue using @a polygon.
		 */
		GPlatesModel::PropertyValue::non_null_ptr_type
		create_polygon_geometry_property_value(
				const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type &polygon);


		/**
		 * Create PropertyValue object given an iterator of @a PointOnSphere objects and
		 * a geometry type.
		 */
		template <typename PointForwardIter>
		boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type>
		create_geometry_property_value(
				PointForwardIter begin, 
				PointForwardIter end,
				GPlatesMaths::GeometryType::Value type);


		/**
		 * Removes any properties that contain geometry from @a feature_ref.
		 */
		void
		remove_geometry_properties_from_feature(
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref);
	}


	//
	// Implementation
	//
	namespace GeometryUtils
	{
		template <typename PointForwardIter>
		boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type>
		create_geometry_property_value(
				PointForwardIter begin, 
				PointForwardIter end,
				GPlatesMaths::GeometryType::Value type)
		{
			if (begin == end)
			{
				return boost::none;
			}

			switch(type)
			{
			case GPlatesMaths::GeometryType::POLYLINE:
				return create_polyline_geometry_property_value(
						GPlatesMaths::PolylineOnSphere::create(begin, end));

			case GPlatesMaths::GeometryType::MULTIPOINT:
				return create_multipoint_geometry_property_value(
						GPlatesMaths::MultiPointOnSphere::create(begin, end));

			case GPlatesMaths::GeometryType::POLYGON:
				return create_polygon_geometry_property_value(
						GPlatesMaths::PolygonOnSphere::create(begin, end));

			case GPlatesMaths::GeometryType::POINT:
				return create_point_geometry_property_value(*begin);

			case GPlatesMaths::GeometryType::NONE:
				break;

			default:
				qWarning() << "Unrecognised GPlatesMaths::GeometryType";
				break;
			}

			return boost::none;
		}
	}
}

#endif // GPLATES_APP_LOGIC_GEOMETRY_UTILS_H

