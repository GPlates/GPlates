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

#include <QString>
#include <QList>

#include "EditWidgetChooser.h"

#include "qt-widgets/EditWidgetGroupBox.h"
#include "model/FeatureHandle.h"
#include "model/TopLevelPropertyInline.h"
#include "model/FeatureRevision.h"

#include "property-values/GmlTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlAge.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlKeyValueDictionary.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlOldPlatesHeader.h"
#include "property-values/GpmlStringList.h"
#include "property-values/Enumeration.h"
#include "property-values/XsBoolean.h"
#include "property-values/XsDouble.h"
#include "property-values/XsInteger.h"
#include "property-values/XsString.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"

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


bool
GPlatesQtWidgets::EditWidgetChooser::initialise_pre_property_values(
		GPlatesModel::TopLevelPropertyInline &top_level_property_inline)
{
	const GPlatesModel::PropertyName &curr_prop_name = top_level_property_inline.property_name();

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
GPlatesQtWidgets::EditWidgetChooser::visit_enumeration(
		GPlatesPropertyValues::Enumeration &enumeration)
{
	d_edit_widget_group_box_ptr->activate_edit_enumeration_widget(enumeration);
}


void
GPlatesQtWidgets::EditWidgetChooser::visit_gml_line_string(
		GPlatesPropertyValues::GmlLineString &gml_line_string)
{
	d_edit_widget_group_box_ptr->activate_edit_line_string_widget(gml_line_string);
}


void
GPlatesQtWidgets::EditWidgetChooser::visit_gml_multi_point(
		GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
{
	d_edit_widget_group_box_ptr->activate_edit_multi_point_widget(gml_multi_point);
}


void
GPlatesQtWidgets::EditWidgetChooser::visit_gml_orientable_curve(
		GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve)
{
	// FIXME: We might want to edit the OrientableCurve directly.
	// For now, simply let the user edit the embedded LineString.
	gml_orientable_curve.base_curve()->accept_visitor(*this);
}


void
GPlatesQtWidgets::EditWidgetChooser::visit_gml_point(
		GPlatesPropertyValues::GmlPoint &gml_point)
{
	d_edit_widget_group_box_ptr->activate_edit_point_widget(gml_point);
}


void
GPlatesQtWidgets::EditWidgetChooser::visit_gml_polygon(
		GPlatesPropertyValues::GmlPolygon &gml_polygon)
{
	d_edit_widget_group_box_ptr->activate_edit_polygon_widget(gml_polygon);
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
GPlatesQtWidgets::EditWidgetChooser::visit_gpml_age(
		GPlatesPropertyValues::GpmlAge &gpml_age)
{
	d_edit_widget_group_box_ptr->activate_edit_age_widget(gpml_age);
}

void
GPlatesQtWidgets::EditWidgetChooser::visit_gpml_array(
		GPlatesPropertyValues::GpmlArray &gpml_array)
{
	d_edit_widget_group_box_ptr->activate_edit_time_sequence_widget(gpml_array);
}

void
GPlatesQtWidgets::EditWidgetChooser::visit_gpml_constant_value(
		GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}

#if 0
void
GPlatesQtWidgets::EditWidgetChooser::visit_gpml_irregular_sampling(
	GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling)
{
	d_edit_widget_group_box_ptr->activate_edit_time_sequence_widget(gpml_irregular_sampling);
}
#endif

void
GPlatesQtWidgets::EditWidgetChooser::visit_gpml_key_value_dictionary(
		GPlatesPropertyValues::GpmlKeyValueDictionary &gpml_key_value_dictionary)
{
	d_edit_widget_group_box_ptr->activate_edit_shapefile_attributes_widget(gpml_key_value_dictionary);
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
GPlatesQtWidgets::EditWidgetChooser::visit_gpml_string_list(
		GPlatesPropertyValues::GpmlStringList &gpml_string_list)
{
	d_edit_widget_group_box_ptr->activate_edit_string_list_widget(gpml_string_list);
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


