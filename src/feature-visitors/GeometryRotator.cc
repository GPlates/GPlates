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
			d_finite_rotation * gml_line_string.polyline());
}


void
GPlatesFeatureVisitors::GeometryRotator::visit_gml_multi_point(
		GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
{
	gml_multi_point.set_multipoint(
			d_finite_rotation * gml_multi_point.multipoint());
}


void
GPlatesFeatureVisitors::GeometryRotator::visit_gml_orientable_curve(
		GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve)
{
	gml_orientable_curve.base_curve()->accept_visitor(*this);
}


void
GPlatesFeatureVisitors::GeometryRotator::visit_gml_point(
		GPlatesPropertyValues::GmlPoint &gml_point)
{
	gml_point.set_point(
			d_finite_rotation * gml_point.point());
}


void
GPlatesFeatureVisitors::GeometryRotator::visit_gml_polygon(
		GPlatesPropertyValues::GmlPolygon &gml_polygon)
{
	// Rotate the exterior polygon.
	gml_polygon.set_exterior(
			d_finite_rotation * gml_polygon.exterior());

	// Reserver space for rotated interior polygons.
	GPlatesPropertyValues::GmlPolygon::ring_sequence_type rotated_interior_polygons;
	rotated_interior_polygons.reserve(
			std::distance(gml_polygon.interiors_begin(), gml_polygon.interiors_end()));

	// Rotate the interior polygons into temporary storage first.
	GPlatesPropertyValues::GmlPolygon::ring_const_iterator interior_iter =
			gml_polygon.interiors_begin();
	GPlatesPropertyValues::GmlPolygon::ring_const_iterator interior_end =
			gml_polygon.interiors_end();
	for ( ; interior_iter != interior_end; ++interior_iter)
	{
		const GPlatesPropertyValues::GmlPolygon::ring_type &interior_polygon = *interior_iter;

		GPlatesPropertyValues::GmlPolygon::ring_type rotated_interior_polygon =
				d_finite_rotation * interior_polygon;

		rotated_interior_polygons.push_back(rotated_interior_polygon);
	}

	gml_polygon.clear_interiors();

	// Add the rotated interior polygons to the GmlPolygon.
	GPlatesPropertyValues::GmlPolygon::ring_const_iterator rotated_interior_iter =
			rotated_interior_polygons.begin();
	GPlatesPropertyValues::GmlPolygon::ring_const_iterator rotated_interior_end =
			rotated_interior_polygons.end();
	for ( ; rotated_interior_iter != rotated_interior_end; ++rotated_interior_iter)
	{
		const GPlatesPropertyValues::GmlPolygon::ring_type &rotated_interior_polygon =
				*rotated_interior_iter;

		gml_polygon.add_interior(rotated_interior_polygon);
	}
}


void
GPlatesFeatureVisitors::GeometryRotator::visit_gpml_constant_value(
		GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}
