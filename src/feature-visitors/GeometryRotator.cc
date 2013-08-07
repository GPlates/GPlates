/* $Id$ */
/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include <utility>

#include "GeometryRotator.h"

#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"

#include "model/FeatureHandle.h"
#include "model/NotificationGuard.h"

#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GpmlConstantValue.h"


void
GPlatesFeatureVisitors::GeometryRotator::visit_gml_line_string(
		GPlatesPropertyValues::GmlLineString &gml_line_string)
{
	gml_line_string.set_polyline(
			d_finite_rotation * gml_line_string.get_polyline());
}


void
GPlatesFeatureVisitors::GeometryRotator::visit_gml_multi_point(
		GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
{
	gml_multi_point.set_multipoint(
			d_finite_rotation * gml_multi_point.get_multipoint());
}


void
GPlatesFeatureVisitors::GeometryRotator::visit_gml_orientable_curve(
		GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve)
{
	GPlatesModel::PropertyValue::non_null_ptr_type base_curve =
			gml_orientable_curve.get_base_curve()->clone();
	base_curve->accept_visitor(*this);
	gml_orientable_curve.set_base_curve(base_curve);
}


void
GPlatesFeatureVisitors::GeometryRotator::visit_gml_point(
		GPlatesPropertyValues::GmlPoint &gml_point)
{
	gml_point.set_point(
			d_finite_rotation * gml_point.get_point());
}


void
GPlatesFeatureVisitors::GeometryRotator::visit_gml_polygon(
		GPlatesPropertyValues::GmlPolygon &gml_polygon)
{
	// Merge model events across this scope to avoid excessive number of model callbacks.
	GPlatesModel::NotificationGuard model_notification_guard(gml_polygon.get_model());

	// Rotate the exterior polygon.
	gml_polygon.set_exterior(
			d_finite_rotation * gml_polygon.get_exterior());

	const GPlatesPropertyValues::GmlPolygon::ring_sequence_type &interior_polygons =
			gml_polygon.get_interiors();

	// Reserver space for rotated interior polygons.
	GPlatesPropertyValues::GmlPolygon::ring_sequence_type rotated_interior_polygons;
	rotated_interior_polygons.reserve(
			std::distance(interior_polygons.begin(), interior_polygons.end()));

	// Rotate the interior polygons into temporary storage first.
	GPlatesPropertyValues::GmlPolygon::ring_sequence_type::const_iterator interior_iter =
			interior_polygons.begin();
	GPlatesPropertyValues::GmlPolygon::ring_sequence_type::const_iterator interior_end =
			interior_polygons.end();
	for ( ; interior_iter != interior_end; ++interior_iter)
	{
		const GPlatesPropertyValues::GmlPolygon::ring_type &interior_polygon = *interior_iter;

		GPlatesPropertyValues::GmlPolygon::ring_type rotated_interior_polygon =
				d_finite_rotation * interior_polygon;

		rotated_interior_polygons.push_back(rotated_interior_polygon);
	}

	// Add the rotated interior polygons to the GmlPolygon.
	gml_polygon.set_interiors(rotated_interior_polygons);
}


void
GPlatesFeatureVisitors::GeometryRotator::visit_gpml_constant_value(
		GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	GPlatesModel::PropertyValue::non_null_ptr_type property_value =
			gpml_constant_value.get_value()->clone();
	property_value->accept_visitor(*this);
	gpml_constant_value.set_value(property_value);
}
