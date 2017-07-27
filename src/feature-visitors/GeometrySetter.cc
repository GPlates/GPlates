/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009 The University of Sydney, Australia
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

#include <algorithm>  // std::find
#include <QDebug>

#include "GeometrySetter.h"
#include "model/FeatureHandle.h"
#include "model/TopLevelPropertyInline.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GpmlConstantValue.h"
#include "global/RetrievalFromEmptyContainerException.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"

void
GPlatesFeatureVisitors::GeometrySetter::set_geometry(
		GPlatesModel::PropertyValue *geometry_property_value)
{
	geometry_property_value->accept_visitor(*this);
}

void
GPlatesFeatureVisitors::GeometrySetter::set_geometry(
		GPlatesModel::TopLevelProperty *geometry_top_level_property)
{
	geometry_top_level_property->accept_visitor(*this);
}

void
GPlatesFeatureVisitors::GeometrySetter::visit_gml_line_string(
		GPlatesPropertyValues::GmlLineString &gml_line_string)
{
	// We use a dynamic cast here (despite the fact that dynamic casts are
	// generally considered bad form) because we only care about one specific
	// derivation. Despite the fact that we could be given ANY PropertyValue
	// and ANY GeometryOnSphere, there are only a few combinations that
	// make any sense, and to hell with the rest of them.
	const GPlatesMaths::PolylineOnSphere *polyline_on_sphere =
			dynamic_cast<const GPlatesMaths::PolylineOnSphere *>(d_geometry_to_set.get());
	if (polyline_on_sphere != NULL)
	{
		gml_line_string.set_polyline(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type(polyline_on_sphere));
	}
}


void
GPlatesFeatureVisitors::GeometrySetter::visit_gml_multi_point(
		GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
{
	// We use a dynamic cast here (despite the fact that dynamic casts are
	// generally considered bad form) because we only care about one specific
	// derivation. Despite the fact that we could be given ANY PropertyValue
	// and ANY GeometryOnSphere, there are only a few combinations that
	// make any sense, and to hell with the rest of them.
	const GPlatesMaths::MultiPointOnSphere *multi_point_on_sphere =
			dynamic_cast<const GPlatesMaths::MultiPointOnSphere *>(d_geometry_to_set.get());
	if (multi_point_on_sphere != NULL)
	{
		gml_multi_point.set_multipoint(
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type(multi_point_on_sphere));
	}
}


void
GPlatesFeatureVisitors::GeometrySetter::visit_gml_orientable_curve(
		GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve)
{
	gml_orientable_curve.base_curve()->accept_visitor(*this);
}


void
GPlatesFeatureVisitors::GeometrySetter::visit_gml_point(
		GPlatesPropertyValues::GmlPoint &gml_point)
{
	// We use a dynamic cast here (despite the fact that dynamic casts are
	// generally considered bad form) because we only care about one specific
	// derivation. Despite the fact that we could be given ANY PropertyValue
	// and ANY GeometryOnSphere, there are only a few combinations that
	// make any sense, and to hell with the rest of them.
	const GPlatesMaths::PointOnSphere *point_on_sphere =
			dynamic_cast<const GPlatesMaths::PointOnSphere *>(d_geometry_to_set.get());
	if (point_on_sphere != NULL)
	{
		gml_point.set_point(
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type(point_on_sphere));
	}
}


void
GPlatesFeatureVisitors::GeometrySetter::visit_gml_polygon(
		GPlatesPropertyValues::GmlPolygon &gml_polygon)
{
	// We use a dynamic cast here (despite the fact that dynamic casts are
	// generally considered bad form) because we only care about one specific
	// derivation. Despite the fact that we could be given ANY PropertyValue
	// and ANY GeometryOnSphere, there are only a few combinations that
	// make any sense, and to hell with the rest of them.
	const GPlatesMaths::PolygonOnSphere *polygon_on_sphere =
			dynamic_cast<const GPlatesMaths::PolygonOnSphere *>(d_geometry_to_set.get());
	if (polygon_on_sphere != NULL)
	{
		gml_polygon.set_polygon(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type(polygon_on_sphere));
	}
}


void
GPlatesFeatureVisitors::GeometrySetter::visit_gpml_constant_value(
		GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}
