/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#include "GeometryFinder.h"
#include "model/FeatureHandle.h"
#include "model/InlinePropertyContainer.h"
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
GPlatesFeatureVisitors::GeometryFinder::visit_feature_handle(
		const GPlatesModel::FeatureHandle &feature_handle)
{
	// Now visit each of the properties in turn.
	visit_feature_properties(feature_handle);
}


namespace
{
	template<typename C, typename E>
	bool
	contains_elem(
			const C &container,
			const E &elem)
	{
		return (std::find(container.begin(), container.end(), elem) != container.end());
	}
}


void
GPlatesFeatureVisitors::GeometryFinder::visit_inline_property_container(
		const GPlatesModel::InlinePropertyContainer &inline_property_container)
{
	const GPlatesModel::PropertyName &curr_prop_name = inline_property_container.property_name();

	if ( ! d_property_names_to_allow.empty()) {
		// We're not allowing all property names.
		if ( ! contains_elem(d_property_names_to_allow, curr_prop_name)) {
			// The current property name is not allowed.
			return;
		}
	}

	visit_property_values(inline_property_container);
}


void
GPlatesFeatureVisitors::GeometryFinder::visit_gml_line_string(
		const GPlatesPropertyValues::GmlLineString &gml_line_string)
{
	d_found_geometries.push_back(gml_line_string.polyline());
}


void
GPlatesFeatureVisitors::GeometryFinder::visit_gml_multi_point(
		const GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
{
	d_found_geometries.push_back(gml_multi_point.multipoint());
}


void
GPlatesFeatureVisitors::GeometryFinder::visit_gml_orientable_curve(
		const GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve)
{
	gml_orientable_curve.base_curve()->accept_visitor(*this);
}


void
GPlatesFeatureVisitors::GeometryFinder::visit_gml_point(
		const GPlatesPropertyValues::GmlPoint &gml_point)
{
	d_found_geometries.push_back(gml_point.point());
}


void
GPlatesFeatureVisitors::GeometryFinder::visit_gml_polygon(
		const GPlatesPropertyValues::GmlPolygon &gml_polygon)
{
	// First, the exterior ring.
	d_found_geometries.push_back(gml_polygon.exterior());

	// Next, the interior rings (if there are any).
	GPlatesPropertyValues::GmlPolygon::ring_const_iterator iter = gml_polygon.interiors_begin();
	GPlatesPropertyValues::GmlPolygon::ring_const_iterator end = gml_polygon.interiors_end();
	for ( ; iter != end; ++iter) {
		d_found_geometries.push_back(*iter);
	}
}


void
GPlatesFeatureVisitors::GeometryFinder::visit_gpml_constant_value(
		const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}


GPlatesFeatureVisitors::GeometryFinder::geometry_elem_type
GPlatesFeatureVisitors::GeometryFinder::first_geometry_found() const
{
	if ( ! has_found_geometries()) {
		// Whoops, the container's empty.
		throw GPlatesGlobal::RetrievalFromEmptyContainerException(__FILE__, __LINE__);
	}
	return *(found_geometries_begin());
}
