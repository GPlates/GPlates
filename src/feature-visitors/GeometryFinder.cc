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

#include "GeometryFinder.h"
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


bool
GPlatesFeatureVisitors::GeometryFinder::initialise_pre_property_values(
		const GPlatesModel::TopLevelPropertyInline &top_level_property_inline)
{
	const GPlatesModel::PropertyName &curr_prop_name = top_level_property_inline.get_property_name();

	if ( ! d_property_names_to_allow.empty()) {
		// We're not allowing all property names.
		if ( ! contains_elem(d_property_names_to_allow, curr_prop_name)) {
			// The current property name is not allowed.
			return false;
		}
	}
	return true;
}


void
GPlatesFeatureVisitors::GeometryFinder::visit_gml_line_string(
		const GPlatesPropertyValues::GmlLineString &gml_line_string)
{
	d_found_geometries.push_back(gml_line_string.get_polyline());
	d_found_polyline_geometries.push_back(gml_line_string.get_polyline());
}


void
GPlatesFeatureVisitors::GeometryFinder::visit_gml_multi_point(
		const GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
{
	d_found_geometries.push_back(gml_multi_point.get_multipoint());
	d_found_multi_point_geometries.push_back(gml_multi_point.get_multipoint());
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
	d_found_geometries.push_back(gml_point.get_point());
	d_found_point_geometries.push_back( gml_point.get_point() );
}


void
GPlatesFeatureVisitors::GeometryFinder::visit_gml_polygon(
		const GPlatesPropertyValues::GmlPolygon &gml_polygon)
{
	// First, the exterior ring.
	d_found_geometries.push_back(gml_polygon.get_exterior());
	d_found_polygon_geometries.push_back(gml_polygon.get_exterior());

	// Next, the interior rings (if there are any).
	const GPlatesPropertyValues::GmlPolygon::ring_sequence_type &interiors = gml_polygon.get_interiors();
	GPlatesPropertyValues::GmlPolygon::ring_sequence_type::const_iterator iter = interiors.begin();
	GPlatesPropertyValues::GmlPolygon::ring_sequence_type::const_iterator end = interiors.end();
	for ( ; iter != end; ++iter) 
	{
		d_found_geometries.push_back(*iter);
		d_found_polygon_geometries.push_back(*iter);
	}
}


void
GPlatesFeatureVisitors::GeometryFinder::visit_gpml_constant_value(
		const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.get_value()->accept_visitor(*this);
}


GPlatesFeatureVisitors::GeometryFinder::geometry_elem_type
GPlatesFeatureVisitors::GeometryFinder::first_geometry_found() const
{
	if ( ! has_found_geometries()) {
		// Whoops, the container's empty.
		throw GPlatesGlobal::RetrievalFromEmptyContainerException(GPLATES_EXCEPTION_SOURCE);
	}
	return *(found_geometries_begin());
}
