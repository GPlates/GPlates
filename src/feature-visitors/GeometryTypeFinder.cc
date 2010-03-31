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


#include <QDebug>

#include "GeometryTypeFinder.h"
#include "GeometryFinder.h"

#include "model/FeatureHandle.h"
#include "model/TopLevelPropertyInline.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GpmlConstantValue.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"


void
GPlatesFeatureVisitors::GeometryTypeFinder::visit_gml_line_string(
		const GPlatesPropertyValues::GmlLineString &gml_line_string)
{
	++d_num_polyline_geometries_found;
}


void
GPlatesFeatureVisitors::GeometryTypeFinder::visit_gml_multi_point(
		const GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
{
	++d_num_multi_point_geometries_found;
}


void
GPlatesFeatureVisitors::GeometryTypeFinder::visit_gml_orientable_curve(
		const GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve)
{
	gml_orientable_curve.base_curve()->accept_visitor(*this);
}


void
GPlatesFeatureVisitors::GeometryTypeFinder::visit_gml_point(
		const GPlatesPropertyValues::GmlPoint &gml_point)
{
	++d_num_point_geometries_found;
}


void
GPlatesFeatureVisitors::GeometryTypeFinder::visit_gml_polygon(
		const GPlatesPropertyValues::GmlPolygon &gml_polygon)
{
	++d_num_polygon_geometries_found;
}


void
GPlatesFeatureVisitors::GeometryTypeFinder::visit_gpml_constant_value(
		const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}


void
GPlatesFeatureVisitors::GeometryTypeFinder::visit_multipoint_on_sphere(
	const GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
{
	++d_num_multi_point_geometries_found;
}

void
GPlatesFeatureVisitors::GeometryTypeFinder::visit_point_on_sphere(
	const GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
{
	++d_num_point_geometries_found;
}

void
GPlatesFeatureVisitors::GeometryTypeFinder::visit_polygon_on_sphere(
	const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
{
	++d_num_polygon_geometries_found;
}

void
GPlatesFeatureVisitors::GeometryTypeFinder::visit_polyline_on_sphere(
	const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
{
	++d_num_polyline_geometries_found;
}

bool
GPlatesFeatureVisitors::GeometryTypeFinder::has_found_multiple_geometry_types() const
{
	int number_of_geometry_types = 0;

	if (found_point_geometries())
	{
		number_of_geometry_types++;
	}
	if (found_multi_point_geometries())
	{
		number_of_geometry_types++;
	}
	if (found_polyline_geometries())
	{
		number_of_geometry_types++;
	}
	if (found_polygon_geometries())
	{
		number_of_geometry_types++;
	}

	return (number_of_geometry_types > 1);
}


bool
GPlatesFeatureVisitors::GeometryTypeFinder::has_found_multiple_geometries_of_the_same_type() const
{
	return num_point_geometries_found() > 1 ||
			num_multi_point_geometries_found() > 1 ||
			num_polyline_geometries_found() > 1 ||
			num_polygon_geometries_found() > 1;
}


boost::optional<GPlatesModel::FeatureHandle::children_iterator>
GPlatesFeatureVisitors::find_first_geometry_property(
		GPlatesModel::FeatureHandle::weak_ref feature_ref)
{
		if(!feature_ref.is_valid())
			return boost::none;

		GPlatesModel::FeatureHandle::children_iterator iter =
			feature_ref->children_begin();
		GPlatesModel::FeatureHandle::children_iterator iter_end =
			feature_ref->children_end();

		GPlatesFeatureVisitors::GeometryFinder geometry_finder;
		for(; iter != iter_end; iter++)
		{
			if(!iter.is_valid())
			{
				continue;
			}
			(*iter)->accept_visitor(geometry_finder);
			if (geometry_finder.has_found_geometries()) 
			{
				return iter;					
			}
		}
		return iter;
	
}

/**
 * Returns true if @a feature_properties_iter is not a geometry property.
 */

bool
GPlatesFeatureVisitors::is_not_geometry_property(
	const GPlatesModel::FeatureHandle::children_iterator &feature_properties_iter)
{
	GPlatesFeatureVisitors::GeometryTypeFinder geom_type_finder;
	(*feature_properties_iter)->accept_visitor(geom_type_finder);
	return !geom_type_finder.has_found_geometries();
}

GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type 
GPlatesFeatureVisitors::find_first_geometry(
	GPlatesModel::FeatureHandle::children_iterator iter)
{
	GPlatesFeatureVisitors::GeometryFinder geometry_finder;
	(*iter)->accept_visitor(geometry_finder);
	return *geometry_finder.found_geometries_begin();
}
