/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009 Geological Survey of Norway
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
#include "model/ConstFeatureVisitor.h"

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
			d_found_point_geometries(false),
			d_found_multi_point_geometries(false),
			d_found_polyline_geometries(false),
			d_found_polygon_geometries(false)
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
			return d_found_point_geometries;
		}

		bool
		found_multi_point_geometries() const
		{
			return d_found_multi_point_geometries;
		}

		bool
		found_polyline_geometries() const
		{
			return d_found_polyline_geometries;
		}

		bool
		found_polygon_geometries() const
		{
			return d_found_polygon_geometries;
		}

		bool
		has_found_geometries() const
		{
			return (d_found_point_geometries ||
					d_found_multi_point_geometries ||
					d_found_polyline_geometries ||
					d_found_polygon_geometries);
		}

		bool
		has_found_multiple_geometries() const;


		void
		clear_found_geometries()
		{
			d_found_point_geometries = false;
			d_found_multi_point_geometries = false;
			d_found_polyline_geometries = false;
			d_found_polygon_geometries = false;
		}

	private:

		bool d_found_point_geometries;
		bool d_found_multi_point_geometries;
		bool d_found_polyline_geometries;
		bool d_found_polygon_geometries;
	};
}

#endif  // GPLATES_FEATUREVISITORS_GEOMETRYTYPEFINDER_H
