/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009, 2010 The University of Sydney, Australia
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

#include "FromQvariantConverter.h"
#include "model/FeatureHandle.h"
#include "model/TopLevelPropertyInline.h"
#include "model/FeatureRevision.h"

#include "property-values/GmlTimeInstant.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlOldPlatesHeader.h"
#include "property-values/Enumeration.h"
#include "property-values/XsBoolean.h"
#include "property-values/XsDouble.h"
#include "property-values/XsInteger.h"
#include "property-values/XsString.h"

#include "utils/UnicodeStringUtils.h"
#include "model/ModelUtils.h"


void
GPlatesFeatureVisitors::FromQvariantConverter::visit_enumeration(
		const GPlatesPropertyValues::Enumeration &enumeration)
{
}


void
GPlatesFeatureVisitors::FromQvariantConverter::visit_gml_time_instant(
		const GPlatesPropertyValues::GmlTimeInstant &gml_time_instant)
{
	double d = d_qvariant.toDouble();
	GPlatesModel::PropertyValue::non_null_ptr_type new_value = 
			GPlatesModel::ModelUtils::create_gml_time_instant(GPlatesPropertyValues::GeoTimeInstant(d));
	set_return_value(new_value);
}


void
GPlatesFeatureVisitors::FromQvariantConverter::visit_gpml_constant_value(
		const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}


void
GPlatesFeatureVisitors::FromQvariantConverter::visit_gpml_plate_id(
		const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
{
	int plateid = d_qvariant.toInt();
	GPlatesModel::PropertyValue::non_null_ptr_type new_value = GPlatesPropertyValues::GpmlPlateId::create(plateid);
	set_return_value(new_value);
}


void
GPlatesFeatureVisitors::FromQvariantConverter::visit_gpml_old_plates_header(
		const GPlatesPropertyValues::GpmlOldPlatesHeader &gpml_old_plates_header)
{
}


void
GPlatesFeatureVisitors::FromQvariantConverter::visit_xs_boolean(
		const GPlatesPropertyValues::XsBoolean &xs_boolean)
{
	bool b = d_qvariant.toBool();
	GPlatesModel::PropertyValue::non_null_ptr_type new_value = GPlatesPropertyValues::XsBoolean::create(b);
	set_return_value(new_value);
}


void
GPlatesFeatureVisitors::FromQvariantConverter::visit_xs_double(
		const GPlatesPropertyValues::XsDouble& xs_double)
{
	double d = d_qvariant.toDouble();
	GPlatesModel::PropertyValue::non_null_ptr_type new_value = GPlatesPropertyValues::XsDouble::create(d);
	set_return_value(new_value);
}

void
GPlatesFeatureVisitors::FromQvariantConverter::visit_xs_integer(
		const GPlatesPropertyValues::XsInteger& xs_integer)
{
	int i = d_qvariant.toInt();
	GPlatesModel::PropertyValue::non_null_ptr_type new_value = GPlatesPropertyValues::XsInteger::create(i);
	set_return_value(new_value);
}

void
GPlatesFeatureVisitors::FromQvariantConverter::visit_xs_string(
		const GPlatesPropertyValues::XsString &xs_string)
{
	QString qs = d_qvariant.toString();
	GPlatesModel::PropertyValue::non_null_ptr_type new_value = GPlatesPropertyValues::XsString::create(
			GPlatesUtils::make_icu_string_from_qstring(qs));
	set_return_value(new_value);
}


void
GPlatesFeatureVisitors::FromQvariantConverter::set_return_value(
		GPlatesModel::PropertyValue::non_null_ptr_type new_value)
{
	if ( ! d_property_value) {
		d_property_value = new_value;
	}
}


#if 0

bool
GPlatesFeatureVisitors::FromQvariantConverter::assign_single_value(
		GPlatesModel::PropertyValue::non_null_ptr_type new_value)
{
	if (d_last_top_level_property_inline_visited_ptr == NULL) {
		return false;
	}

	// Assign new non_null_ptr_type
	if ( ! d_dry_run) {
		*(d_last_top_level_property_inline_visited_ptr->begin()) = new_value;
	}
	d_property_has_been_set = true;
	
	// We have done what we came here to do - reset the pointer back to NULL
	d_last_top_level_property_inline_visited_ptr = NULL;
	return true;
}

#endif


