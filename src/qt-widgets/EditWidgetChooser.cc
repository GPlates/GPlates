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

#include <QString>
#include <QList>

#include "EditWidgetChooser.h"

#include "qt-widgets/EditWidgetGroupBox.h"
#include "model/FeatureHandle.h"
#include "model/InlinePropertyContainer.h"
#include "model/FeatureRevision.h"

#include "property-values/GmlTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlOldPlatesHeader.h"
#include "property-values/Enumeration.h"
#include "property-values/XsBoolean.h"
#include "property-values/XsDouble.h"
#include "property-values/XsInteger.h"
#include "property-values/XsString.h"

#include "qt-widgets/EditTimePeriodWidget.h"

#include "utils/UnicodeStringUtils.h"


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
GPlatesQtWidgets::EditWidgetChooser::visit_feature_handle(
		GPlatesModel::FeatureHandle &feature_handle)
{
	// Visit each of the properties in turn.
	visit_feature_properties(feature_handle);
}


void
GPlatesQtWidgets::EditWidgetChooser::visit_inline_property_container(
		GPlatesModel::InlinePropertyContainer &inline_property_container)
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
GPlatesQtWidgets::EditWidgetChooser::visit_enumeration(
		GPlatesPropertyValues::Enumeration &enumeration)
{
	d_edit_widget_group_box_ptr->activate_edit_enumeration_widget(enumeration);
}


void
GPlatesQtWidgets::EditWidgetChooser::visit_gml_time_instant(
		GPlatesPropertyValues::GmlTimeInstant &gml_time_instant)
{
	d_edit_widget_group_box_ptr->activate_edit_time_instant_widget(gml_time_instant);
}


void
GPlatesQtWidgets::EditWidgetChooser::visit_gml_time_period(
		GPlatesPropertyValues::GmlTimePeriod &gml_time_period)
{
	d_edit_widget_group_box_ptr->activate_edit_time_period_widget(gml_time_period);
}


void
GPlatesQtWidgets::EditWidgetChooser::visit_gpml_constant_value(
		GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}


void
GPlatesQtWidgets::EditWidgetChooser::visit_gpml_plate_id(
		GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
{
	d_edit_widget_group_box_ptr->activate_edit_plate_id_widget(gpml_plate_id);
}


void
GPlatesQtWidgets::EditWidgetChooser::visit_gpml_polarity_chron_id(
		GPlatesPropertyValues::GpmlPolarityChronId &gpml_polarity_chron_id)
{
	d_edit_widget_group_box_ptr->activate_edit_polarity_chron_id_widget(gpml_polarity_chron_id);
}


void
GPlatesQtWidgets::EditWidgetChooser::visit_gpml_measure(
		GPlatesPropertyValues::GpmlMeasure &gpml_measure)
{
	// FIXME: Check what kind of gpml:measure it is! In this case, assuming it's a gpml:angle.
	d_edit_widget_group_box_ptr->activate_edit_angle_widget(gpml_measure);
}


void
GPlatesQtWidgets::EditWidgetChooser::visit_gpml_old_plates_header(
		GPlatesPropertyValues::GpmlOldPlatesHeader &gpml_old_plates_header)
{
	d_edit_widget_group_box_ptr->activate_edit_old_plates_header_widget(gpml_old_plates_header);
}


void
GPlatesQtWidgets::EditWidgetChooser::visit_xs_boolean(
		GPlatesPropertyValues::XsBoolean &xs_boolean)
{
	d_edit_widget_group_box_ptr->activate_edit_boolean_widget(xs_boolean);
}


void
GPlatesQtWidgets::EditWidgetChooser::visit_xs_double(
		GPlatesPropertyValues::XsDouble& xs_double)
{
	d_edit_widget_group_box_ptr->activate_edit_double_widget(xs_double);
}

void
GPlatesQtWidgets::EditWidgetChooser::visit_xs_integer(
		GPlatesPropertyValues::XsInteger& xs_integer)
{
	d_edit_widget_group_box_ptr->activate_edit_integer_widget(xs_integer);
}

void
GPlatesQtWidgets::EditWidgetChooser::visit_xs_string(
		GPlatesPropertyValues::XsString &xs_string)
{
	d_edit_widget_group_box_ptr->activate_edit_string_widget(xs_string);
}


