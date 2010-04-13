/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009 Geological Survey of Norway
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

#ifndef GPLATES_FEATUREVISITORS_GEOMETRYTYPEFINDER_H
#define GPLATES_FEATUREVISITORS_GEOMETRYTYPEFINDER_H

#include "maths/ConstGeometryOnSphereVisitor.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "model/FeatureVisitor.h"

namespace GPlatesFeatureVisitors
{
	/**
	 * This feature visitor can be used to determine which geometry types exist in a feature.
	 *
	 */
	class GeometryTypeFinder:
			public GPlatesModel::ConstFeatureVisitor,
			public GPlatesMaths::ConstGeometryOnSphereVisitor
	{
	public:

		GeometryTypeFinder():
			d_num_point_geometries_found(0),
			d_num_multi_point_geometries_found(0),
			d_num_polyline_geometries_found(0),
			d_num_polygon_geometries_found(0)
		{  }

		virtual
		~GeometryTypeFinder() {  }

	protected:

		virtual
		void
		visit_gml_line_string(
				const GPlatesPropertyValues::GmlLineString &gml_line_string);

		virtual
		void
		visit_gml_multi_point(
				const GPlatesPropertyValues::GmlMultiPoint &gml_multi_point);

		virtual
		void
		visit_gml_orientable_curve(
				const GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve);

		virtual
		void
		visit_gml_point(
				const GPlatesPropertyValues::GmlPoint &gml_point);

		virtual
		void
		visit_gml_polygon(
				const GPlatesPropertyValues::GmlPolygon &gml_polygon);

		virtual
		void
		visit_gpml_constant_value(
				const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);


		virtual
		void
		visit_multipoint_on_sphere(
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere);

		virtual
		void
		visit_point_on_sphere(
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere);

		virtual
		void
		visit_polygon_on_sphere(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere);

		virtual
		void
		visit_polyline_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere);

	public:

		bool
		found_point_geometries() const
		{
			return d_num_point_geometries_found != 0;
		}

		bool
		found_multi_point_geometries() const
		{
			return d_num_multi_point_geometries_found != 0;
		}

		bool
		found_polyline_geometries() const
		{
			return d_num_polyline_geometries_found != 0;
		}

		bool
		found_polygon_geometries() const
		{
			return d_num_polygon_geometries_found != 0;
		}

		int
		num_point_geometries_found() const
		{
			return d_num_point_geometries_found;
		}

		int
		num_multi_point_geometries_found() const
		{
			return d_num_multi_point_geometries_found;
		}

		int
		num_polyline_geometries_found() const
		{
			return d_num_polyline_geometries_found;
		}

		int
		num_polygon_geometries_found() const
		{
			return d_num_polygon_geometries_found;
		}

		bool
		has_found_geometries() const
		{
			return found_point_geometries() ||
					found_multi_point_geometries() ||
					found_polyline_geometries() ||
					found_polygon_geometries();
		}

		/**
		 * Returns true if different types of geometry were found.
		 * For example, a point and a polyline.
		 */
		bool
		has_found_multiple_geometry_types() const;

		/**
		 * Returns true if found more than one geometry of the same type.
		 */
		bool
		has_found_multiple_geometries_of_the_same_type() const;


		void
		clear_found_geometries()
		{
			d_num_point_geometries_found = 0;
			d_num_multi_point_geometries_found = 0;
			d_num_polyline_geometries_found = 0;
			d_num_polygon_geometries_found = 0;
		}

	private:

		int d_num_point_geometries_found;
		int d_num_multi_point_geometries_found;
		int d_num_polyline_geometries_found;
		int d_num_polygon_geometries_found;
	};

	/**
	 * Find the first geometry property from a feature.
	 *
	 * The iterator of the property will be returned.
	 */
	boost::optional<GPlatesModel::FeatureHandle::iterator>
	find_first_geometry_property(
			GPlatesModel::FeatureHandle::weak_ref feature_ref);

	/**
	 * Determine if the given property contains a geometry.
	 *
	 * Return true if the property is not a geometry, otherwise false.
	 */
	bool
	is_not_geometry_property(
			const GPlatesModel::TopLevelProperty::non_null_ptr_to_const_type &top_level_prop_ptr);

	/**
	 * Find the first geometry from a property.
	 *
	 * A const pointer of GeometryOnSphere will be returned.
	 */
	GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type 
	find_first_geometry(
			GPlatesModel::FeatureHandle::iterator iter);

}

#endif  // GPLATES_FEATUREVISITORS_GEOMETRYTYPEFINDER_H
