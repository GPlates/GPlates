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
#include "utils/non_null_intrusive_ptr.h"
#include "utils/UnicodeStringUtils.h"
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
	d_found_line_strings.push_back(
			GPlatesPropertyValues::GmlLineString::non_null_ptr_to_const_type(gml_line_string));
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
	d_found_points.push_back(
			GPlatesPropertyValues::GmlPoint::non_null_ptr_to_const_type(gml_point));
}


const GPlatesMaths::PointOnSphere *
GPlatesFeatureVisitors::GeometryFinder::get_first_point()
{
	if (found_points_begin() != found_points_end()) {
		GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_ptr =
				(*found_points_begin())->point();
		return &(*point_ptr);
		
	} else if (found_line_strings_begin() != found_line_strings_end()) {
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_ptr =
				(*found_line_strings_begin())->polyline();

		GPlatesMaths::PolylineOnSphere::vertex_const_iterator begin = polyline_ptr->vertex_begin();
		GPlatesMaths::PolylineOnSphere::vertex_const_iterator end = polyline_ptr->vertex_end();
		return &(*begin);
	
	} else {
		// Returning null because otherwise we'll run into "control reaches end of non-void function";
		// Smart people should be checking has_found_geometry() first anyway.
		return NULL;
	}
}


void
GPlatesFeatureVisitors::GeometryFinder::visit_gpml_constant_value(
		const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}

