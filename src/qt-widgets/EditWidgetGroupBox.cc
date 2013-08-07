/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009, 2010, 2011 The University of Sydney, Australia
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

#include <map>
#include <boost/foreach.hpp>

#include "EditWidgetGroupBox.h"

#include "AbstractEditWidget.h"
#include "EditAngleWidget.h"
#include "EditBooleanWidget.h"
#include "EditDoubleWidget.h"
#include "EditEnumerationWidget.h"
#include "EditGeometryWidget.h"
#include "EditIntegerWidget.h"
#include "EditOldPlatesHeaderWidget.h"
#include "EditPlateIdWidget.h"
#include "EditPolarityChronIdWidget.h"
#include "EditShapefileAttributesWidget.h"
#include "EditStringListWidget.h"
#include "EditStringWidget.h"
#include "EditTimeInstantWidget.h"
#include "EditTimePeriodWidget.h"
#include "EditTimeSequenceWidget.h"

#include "EditWidgetChooser.h"
#include "NoActiveEditWidgetException.h"

#include "app-logic/ApplicationState.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "model/Gpgim.h"
#include "model/GpgimEnumerationType.h"
#include "model/GpgimProperty.h"

#include "presentation/ViewState.h"

#include "property-values/GpmlPlateId.h"

GPlatesQtWidgets::EditWidgetGroupBox::EditWidgetGroupBox(
		GPlatesPresentation::ViewState &view_state_,
		QWidget *parent_) :
	QGroupBox(parent_),
	d_gpgim(view_state_.get_application_state().get_gpgim()),
	d_active_widget_ptr(NULL),
	d_edit_time_instant_widget_ptr(new EditTimeInstantWidget(this)),
	d_edit_time_period_widget_ptr(new EditTimePeriodWidget(this)),
	d_edit_old_plates_header_widget_ptr(new EditOldPlatesHeaderWidget(this)),
	d_edit_double_widget_ptr(new EditDoubleWidget(this)),
	d_edit_enumeration_widget_ptr(new EditEnumerationWidget(d_gpgim, this)),
	d_edit_geometry_widget_ptr(new EditGeometryWidget(this)),
	d_edit_integer_widget_ptr(new EditIntegerWidget(this)),
	d_edit_plate_id_widget_ptr(new EditPlateIdWidget(this)),
	d_edit_polarity_chron_id_widget_ptr(new EditPolarityChronIdWidget(this)),
	d_edit_angle_widget_ptr(new EditAngleWidget(this)),
	d_edit_string_list_widget_ptr(new EditStringListWidget(this)),
	d_edit_string_widget_ptr(new EditStringWidget(this)),
	d_edit_boolean_widget_ptr(new EditBooleanWidget(this)),
	d_edit_shapefile_attributes_widget_ptr(new EditShapefileAttributesWidget(this)),
    d_edit_time_sequence_widget_ptr(new EditTimeSequenceWidget(
                view_state_.get_application_state(), this)),
	d_edit_verb(tr("Edit"))
{
	// Build the mapping of property structural types to edit widgets.
	build_widget_map();

	// We stay invisible unless we are called on for a specific widget.
	hide();
	
	QVBoxLayout *edit_layout = new QVBoxLayout;
	edit_layout->setSpacing(0);
	edit_layout->setMargin(4);
	edit_layout->addWidget(d_edit_time_instant_widget_ptr);
	edit_layout->addWidget(d_edit_time_period_widget_ptr);
	edit_layout->addWidget(d_edit_old_plates_header_widget_ptr);
	edit_layout->addWidget(d_edit_double_widget_ptr);
	edit_layout->addWidget(d_edit_enumeration_widget_ptr);
	edit_layout->addWidget(d_edit_geometry_widget_ptr);
	edit_layout->addWidget(d_edit_integer_widget_ptr);
	edit_layout->addWidget(d_edit_plate_id_widget_ptr);
	edit_layout->addWidget(d_edit_polarity_chron_id_widget_ptr);
	edit_layout->addWidget(d_edit_angle_widget_ptr);
	edit_layout->addWidget(d_edit_string_list_widget_ptr);
	edit_layout->addWidget(d_edit_string_widget_ptr);
	edit_layout->addWidget(d_edit_boolean_widget_ptr);
	edit_layout->addWidget(d_edit_shapefile_attributes_widget_ptr);
	edit_layout->addWidget(d_edit_time_sequence_widget_ptr);
	setLayout(edit_layout);
		
	QObject::connect(d_edit_time_instant_widget_ptr, SIGNAL(commit_me()),
			this, SLOT(edit_widget_wants_committing()));
	QObject::connect(d_edit_time_period_widget_ptr, SIGNAL(commit_me()),
			this, SLOT(edit_widget_wants_committing()));
	QObject::connect(d_edit_old_plates_header_widget_ptr, SIGNAL(commit_me()),
			this, SLOT(edit_widget_wants_committing()));
	QObject::connect(d_edit_double_widget_ptr, SIGNAL(commit_me()),
			this, SLOT(edit_widget_wants_committing()));
	QObject::connect(d_edit_enumeration_widget_ptr, SIGNAL(commit_me()),
			this, SLOT(edit_widget_wants_committing()));
	QObject::connect(d_edit_geometry_widget_ptr, SIGNAL(commit_me()),
			this, SLOT(edit_widget_wants_committing()));
	QObject::connect(d_edit_integer_widget_ptr, SIGNAL(commit_me()),
			this, SLOT(edit_widget_wants_committing()));
	QObject::connect(d_edit_plate_id_widget_ptr, SIGNAL(commit_me()),
			this, SLOT(edit_widget_wants_committing()));
	QObject::connect(d_edit_polarity_chron_id_widget_ptr, SIGNAL(commit_me()),
			this, SLOT(edit_widget_wants_committing()));
	QObject::connect(d_edit_angle_widget_ptr, SIGNAL(commit_me()),
			this, SLOT(edit_widget_wants_committing()));
	QObject::connect(d_edit_string_list_widget_ptr, SIGNAL(commit_me()),
			this, SLOT(edit_widget_wants_committing()));
	QObject::connect(d_edit_string_widget_ptr, SIGNAL(commit_me()),
			this, SLOT(edit_widget_wants_committing()));
	QObject::connect(d_edit_boolean_widget_ptr, SIGNAL(commit_me()),
			this, SLOT(edit_widget_wants_committing()));
	QObject::connect(d_edit_shapefile_attributes_widget_ptr, SIGNAL(commit_me()),
			this, SLOT(edit_widget_wants_committing()));
	QObject::connect(d_edit_time_sequence_widget_ptr, SIGNAL(commit_me()),
			this, SLOT(edit_widget_wants_committing()));	
	QObject::connect(d_edit_time_sequence_widget_ptr, SIGNAL(commit_me()),
			this, SLOT(edit_widget_wants_committing()));
}


void
GPlatesQtWidgets::EditWidgetGroupBox::build_widget_map()
{
	d_widget_map[GPlatesPropertyValues::StructuralType::create_gml("TimeInstant")] = d_edit_time_instant_widget_ptr;
	d_widget_map[GPlatesPropertyValues::StructuralType::create_gml("TimePeriod")] = d_edit_time_period_widget_ptr;
	d_widget_map[GPlatesPropertyValues::StructuralType::create_gpml("OldPlatesHeader")] = d_edit_old_plates_header_widget_ptr;
	d_widget_map[GPlatesPropertyValues::StructuralType::create_xsi("double")] = d_edit_double_widget_ptr;
	d_widget_map[GPlatesPropertyValues::StructuralType::create_gml("LineString")] = d_edit_geometry_widget_ptr;
	d_widget_map[GPlatesPropertyValues::StructuralType::create_gml("MultiPoint")] = d_edit_geometry_widget_ptr;
	d_widget_map[GPlatesPropertyValues::StructuralType::create_gml("Point")] = d_edit_geometry_widget_ptr;
	d_widget_map[GPlatesPropertyValues::StructuralType::create_gml("Polygon")] = d_edit_geometry_widget_ptr;
	d_widget_map[GPlatesPropertyValues::StructuralType::create_xsi("integer")] = d_edit_integer_widget_ptr;
	d_widget_map[GPlatesPropertyValues::StructuralType::create_gpml("plateId")] = d_edit_plate_id_widget_ptr;
	d_widget_map[GPlatesPropertyValues::StructuralType::create_gpml("PolarityChronId")] = d_edit_polarity_chron_id_widget_ptr;
	d_widget_map[GPlatesPropertyValues::StructuralType::create_gpml("angle")] = d_edit_angle_widget_ptr;
	d_widget_map[GPlatesPropertyValues::StructuralType::create_xsi("string")] = d_edit_string_widget_ptr;
	d_widget_map[GPlatesPropertyValues::StructuralType::create_xsi("boolean")] = d_edit_boolean_widget_ptr;

	// FIXME: check if IrregularSampling should correspond to the time-sequence-widget, 
	// and if it should be included in this map.
	//
	// UPDATE:
	// 'gpml:Array' is currently the only *template* type (besides the time-dependent wrappers).
	// Currently GPlates assumes the template type is 'gml:TimePeriod' in the following ways:
	// (1) Edit Feature Properties widget selects Edit Time Sequence widget when it visits a 'gpml:Array' property.
	// (2) Add Property dialog selects Edit Time Sequence widget when it finds the 'gpml:Array' string (eg, specified here).
	// Later, when other template types are supported for 'gpml:Array', we'll need to be able to
	// select the appropriate edit widget based not only on'gpml:Array' but also on its template type
	// (essentially both determine the actual type of the property).
	//
	// For now just hardwiring any template of 'gpml:Array' to the Edit Time Sequence widget.
	d_widget_map[GPlatesPropertyValues::StructuralType::create_gpml("Array")] = d_edit_time_sequence_widget_ptr;
	d_widget_map[GPlatesPropertyValues::StructuralType::create_gpml("StringList")] = d_edit_string_list_widget_ptr;
#if 0
	// Keep the KeyValueDictionary out of the map until we have the
	// ability to create one. 
	d_widget_map[GPlatesPropertyValues::StructuralType::create_gpml("KeyValueDictionary")] =
			d_edit_shapefile_attributes_widget_ptr;

#endif

	//
	// Add the enumeration types specified in the GPGIM.
	//

	const GPlatesModel::Gpgim::property_enumeration_type_seq_type &gpgim_property_enumeration_types =
			d_gpgim.get_property_enumeration_types();
	BOOST_FOREACH(
			const GPlatesModel::GpgimEnumerationType::non_null_ptr_to_const_type &gpgim_property_enumeration_type,
			gpgim_property_enumeration_types)
	{
		d_widget_map[gpgim_property_enumeration_type->get_structural_type()] = d_edit_enumeration_widget_ptr;
	}
}


GPlatesQtWidgets::EditWidgetGroupBox::property_types_list_type
GPlatesQtWidgets::EditWidgetGroupBox::get_handled_property_types_list() const
{
	property_types_list_type list;
	
	widget_map_const_iterator it = d_widget_map.begin();
	widget_map_const_iterator end = d_widget_map.end();
	for ( ; it != end; ++it)
	{
		list.push_back(it->first);
	}
	
	return list;
}


bool
GPlatesQtWidgets::EditWidgetGroupBox::get_handled_property_types(
		const GPlatesModel::GpgimProperty &gpgim_property,
		boost::optional<property_types_list_type &> property_types) const
{
	// Get the sequence of structural types allowed (by GPGIM) for the specified property.
	const GPlatesModel::GpgimProperty::structural_type_seq_type &gpgim_structural_types =
			gpgim_property.get_structural_types();
	BOOST_FOREACH(
			const GPlatesModel::GpgimStructuralType::non_null_ptr_to_const_type &gpgim_structural_type,
			gpgim_structural_types)
	{
		const GPlatesPropertyValues::StructuralType structural_type =
				gpgim_structural_type->get_structural_type();

		static const GPlatesPropertyValues::StructuralType OLD_PLATES_HEADER_STRUCTURAL_TYPE =
				GPlatesPropertyValues::StructuralType::create_gpml("OldPlatesHeader");

		// Hack: Since OldPlatesHeaderWidget is no longer editable, we need to exclude this from the list
		// of addable value types (despite it being a valid option for the EditWidgetGroupBox).
		if (structural_type == OLD_PLATES_HEADER_STRUCTURAL_TYPE)
		{
			continue;
		}

		// If the current structural type is implemented as an edit widget then add it to the list.
		if (d_widget_map.find(structural_type) != d_widget_map.end())
		{
			// If the caller only needs to know that at least one property type is
			// supported then return early.
			if (!property_types)
			{
				return true;
			}

			property_types->push_back(structural_type);
		}
	}

	return property_types && !property_types->empty();
}


void
GPlatesQtWidgets::EditWidgetGroupBox::activate_appropriate_edit_widget(
		const GPlatesModel::TopLevelProperty::non_null_ptr_type &top_level_property)
{
	// Get EditWidgetChooser to tell us what widgets to show.
	deactivate_edit_widgets();

	GPlatesQtWidgets::EditWidgetChooser chooser(*this);
	top_level_property->accept_visitor(chooser);

	d_current_property = top_level_property;
	// The property does not belong to a feature.
	d_current_property_iterator = boost::none;

}


void
GPlatesQtWidgets::EditWidgetGroupBox::activate_appropriate_edit_widget(
		GPlatesModel::FeatureHandle::iterator it)
{
	if (!(*it))
	{
		// Always check your property iterators.
		deactivate_edit_widgets();
		return;
	}

	// Note that we have to make a clone of the property in order to edit it.
	// We also save the iterator so we can save the modified property back into the model.
	GPlatesModel::TopLevelProperty::non_null_ptr_type property_clone = (*it)->clone();

	activate_appropriate_edit_widget(property_clone);

	// Property does belong to a feature.
	d_current_property_iterator = it;
}


void
GPlatesQtWidgets::EditWidgetGroupBox::refresh_edit_widget(
		GPlatesModel::FeatureHandle::iterator it)
{
	if (!(*it))
	{
		// Always check your property iterators.
		return;
	}

	// Get EditWidgetChooser to tell us what widgets to update.
	// Note that we have to make a clone of the property in order to edit it.
	// We also save the iterator so we can save the modified property back into the model.
	GPlatesQtWidgets::EditWidgetChooser chooser(*this);
	GPlatesModel::TopLevelProperty::non_null_ptr_type property_clone = (*it)->clone();
	d_current_property = property_clone;
	d_current_property_iterator = it;
	property_clone->accept_visitor(chooser);
}


void
GPlatesQtWidgets::EditWidgetGroupBox::activate_widget_by_property_value_type(
		const GPlatesPropertyValues::StructuralType &property_value_type)
{
	deactivate_edit_widgets();
	AbstractEditWidget *widget_ptr = get_widget_by_property_value_type(property_value_type);
	
	if (widget_ptr != NULL)
	{
		// FIXME: Human readable property value name?
		setTitle(tr("%1 %2")
				.arg(d_edit_verb)
				.arg(convert_qualified_xml_name_to_qstring(property_value_type)));
		show();
		d_active_widget_ptr = widget_ptr;
		widget_ptr->reset_widget_to_default_values();
		widget_ptr->configure_for_property_value_type(property_value_type);
		widget_ptr->show();
	}
}


bool
GPlatesQtWidgets::EditWidgetGroupBox::is_edit_widget_active()
{
	if (isVisible() && d_active_widget_ptr != NULL)
	{
		return true;
	}
	else
	{
		return false;
	}
}


GPlatesModel::PropertyValue::non_null_ptr_type
GPlatesQtWidgets::EditWidgetGroupBox::create_property_value_from_widget()
{
	if (d_active_widget_ptr != NULL)
	{
		return d_active_widget_ptr->create_property_value_from_widget();
	}
	else
	{
		throw NoActiveEditWidgetException(GPLATES_EXCEPTION_SOURCE);
	}
}


bool
GPlatesQtWidgets::EditWidgetGroupBox::update_property_value_from_widget()
{
	if (d_active_widget_ptr != NULL)
	{
		bool result = d_active_widget_ptr->update_property_value_from_widget();

		// Because the above call just updated the clone of the property value
		// we must now commit the changed property back into the model.
		//
		// Note that this does nothing if the current property does not belong to a feature.
		commit_property_to_model();

		return result;
	}
	else
	{
		throw NoActiveEditWidgetException(GPLATES_EXCEPTION_SOURCE);
	}
}


bool
GPlatesQtWidgets::EditWidgetGroupBox::is_dirty()
{
	if (d_active_widget_ptr != NULL)
	{
		return d_active_widget_ptr->is_dirty();
	}
	else
	{
		return false;
	}
}
		
void
GPlatesQtWidgets::EditWidgetGroupBox::set_clean()
{
	if (d_active_widget_ptr != NULL)
	{
		d_active_widget_ptr->set_clean();
	}
}


void
GPlatesQtWidgets::EditWidgetGroupBox::set_dirty()
{
	if (d_active_widget_ptr != NULL)
	{
		d_active_widget_ptr->set_dirty();
	}
}


void
GPlatesQtWidgets::EditWidgetGroupBox::activate_edit_time_instant_widget(
		GPlatesPropertyValues::GmlTimeInstant &gml_time_instant)
{
	setTitle(tr("%1 Time Instant").arg(d_edit_verb));
	show();
	d_edit_time_instant_widget_ptr->update_widget_from_time_instant(gml_time_instant);
	d_active_widget_ptr = d_edit_time_instant_widget_ptr;
	d_edit_time_instant_widget_ptr->show();
}

void
GPlatesQtWidgets::EditWidgetGroupBox::activate_edit_time_period_widget(
		GPlatesPropertyValues::GmlTimePeriod &gml_time_period)
{
	setTitle(tr("%1 Time Period").arg(d_edit_verb));
	show();
	d_edit_time_period_widget_ptr->update_widget_from_time_period(gml_time_period);
	d_active_widget_ptr = d_edit_time_period_widget_ptr;
	d_edit_time_period_widget_ptr->show();
}

void
GPlatesQtWidgets::EditWidgetGroupBox::activate_edit_old_plates_header_widget(
		GPlatesPropertyValues::GpmlOldPlatesHeader &gpml_old_plates_header)
{
	// A bit of a hack: OldPlatesHeader can no longer be edited, so specifying "Edit Old PLATES Header" in the
	// groupbox title is a tease. We ignore d_edit_verb in this case; Add Property has similarly been hacked to
	// prevent users from adding a new OldPlatesHeader which doesn't make a *whole* lot of sense.
	setTitle(tr("View Old PLATES Header"));
	show();
	d_edit_old_plates_header_widget_ptr->update_widget_from_old_plates_header(gpml_old_plates_header);
	d_active_widget_ptr = d_edit_old_plates_header_widget_ptr;
	d_edit_old_plates_header_widget_ptr->show();
}

void
GPlatesQtWidgets::EditWidgetGroupBox::activate_edit_double_widget(
		GPlatesPropertyValues::XsDouble &xs_double)
{
	setTitle(tr("%1 Double").arg(d_edit_verb));
	show();
	d_edit_double_widget_ptr->update_widget_from_double(xs_double);
	d_active_widget_ptr = d_edit_double_widget_ptr;
	d_edit_double_widget_ptr->show();
}

void
GPlatesQtWidgets::EditWidgetGroupBox::activate_edit_enumeration_widget(
		GPlatesPropertyValues::Enumeration &enumeration)
{
	setTitle(tr("%1 Enumeration").arg(d_edit_verb));
	show();
	d_edit_enumeration_widget_ptr->update_widget_from_enumeration(enumeration);
	d_active_widget_ptr = d_edit_enumeration_widget_ptr;
	d_edit_enumeration_widget_ptr->show();
}

void
GPlatesQtWidgets::EditWidgetGroupBox::activate_edit_line_string_widget(
		GPlatesPropertyValues::GmlLineString &gml_line_string)
{
	setTitle(tr("%1 Polyline").arg(d_edit_verb));
	show();
	d_edit_geometry_widget_ptr->update_widget_from_line_string(gml_line_string);
	
	d_active_widget_ptr = d_edit_geometry_widget_ptr;
	d_edit_geometry_widget_ptr->show();
}

void
GPlatesQtWidgets::EditWidgetGroupBox::activate_edit_multi_point_widget(
		GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
{
	setTitle(tr("%1 Multi-Point").arg(d_edit_verb));
	show();
	d_edit_geometry_widget_ptr->update_widget_from_multi_point(gml_multi_point);
	
	d_active_widget_ptr = d_edit_geometry_widget_ptr;
	d_edit_geometry_widget_ptr->show();
}

void
GPlatesQtWidgets::EditWidgetGroupBox::activate_edit_point_widget(
		GPlatesPropertyValues::GmlPoint &gml_point)
{
	setTitle(tr("%1 Point").arg(d_edit_verb));
	show();
	d_edit_geometry_widget_ptr->update_widget_from_point(gml_point);
	
	d_active_widget_ptr = d_edit_geometry_widget_ptr;
	d_edit_geometry_widget_ptr->show();
}

void
GPlatesQtWidgets::EditWidgetGroupBox::activate_edit_polygon_widget(
		GPlatesPropertyValues::GmlPolygon &gml_polygon)
{
	setTitle(tr("%1 Polygon").arg(d_edit_verb));
	show();
	d_edit_geometry_widget_ptr->update_widget_from_polygon(gml_polygon);
	
	d_active_widget_ptr = d_edit_geometry_widget_ptr;
	d_edit_geometry_widget_ptr->show();
}

void
GPlatesQtWidgets::EditWidgetGroupBox::activate_edit_integer_widget(
		GPlatesPropertyValues::XsInteger &xs_integer)
{
	setTitle(tr("%1 Integer").arg(d_edit_verb));
	show();
	d_edit_integer_widget_ptr->update_widget_from_integer(xs_integer);
	d_active_widget_ptr = d_edit_integer_widget_ptr;
	d_edit_integer_widget_ptr->show();
}

void
GPlatesQtWidgets::EditWidgetGroupBox::activate_edit_plate_id_widget(
		GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
{
	setTitle(tr("%1 Plate ID").arg(d_edit_verb));
	show();
	d_edit_plate_id_widget_ptr->update_widget_from_plate_id(gpml_plate_id);
	d_active_widget_ptr = d_edit_plate_id_widget_ptr;
	d_edit_plate_id_widget_ptr->show();
}


void
GPlatesQtWidgets::EditWidgetGroupBox::activate_edit_polarity_chron_id_widget(
		GPlatesPropertyValues::GpmlPolarityChronId &gpml_polarity_chron_id)
{
	setTitle(tr("%1 Polarity Chron ID").arg(d_edit_verb));
	show();
	d_edit_polarity_chron_id_widget_ptr->update_widget_from_polarity_chron_id(gpml_polarity_chron_id);
	d_active_widget_ptr = d_edit_polarity_chron_id_widget_ptr;
	d_edit_polarity_chron_id_widget_ptr->show();
}


void
GPlatesQtWidgets::EditWidgetGroupBox::activate_edit_angle_widget(
		GPlatesPropertyValues::GpmlMeasure &gpml_measure)
{
	setTitle(tr("%1 Angle").arg(d_edit_verb));
	show();
	d_edit_angle_widget_ptr->update_widget_from_angle(gpml_measure);
	d_active_widget_ptr = d_edit_angle_widget_ptr;
	d_edit_angle_widget_ptr->show();
}


void
GPlatesQtWidgets::EditWidgetGroupBox::activate_edit_string_widget(
		GPlatesPropertyValues::XsString &xs_string)
{
	setTitle(tr("%1 String").arg(d_edit_verb));
	show();
	d_edit_string_widget_ptr->update_widget_from_string(xs_string);
	d_active_widget_ptr = d_edit_string_widget_ptr;
	d_edit_string_widget_ptr->show();
}


void
GPlatesQtWidgets::EditWidgetGroupBox::activate_edit_boolean_widget(
		GPlatesPropertyValues::XsBoolean &xs_boolean)
{
	setTitle(tr("%1 Boolean").arg(d_edit_verb));
	show();
	d_edit_boolean_widget_ptr->update_widget_from_boolean(xs_boolean);
	d_active_widget_ptr = d_edit_boolean_widget_ptr;
	d_edit_boolean_widget_ptr->show();
}


void
GPlatesQtWidgets::EditWidgetGroupBox::activate_edit_string_list_widget(
		GPlatesPropertyValues::GpmlStringList &gpml_string_list)
{
	setTitle(tr("%1 String List").arg(d_edit_verb));
	show();
	d_edit_string_list_widget_ptr->update_widget_from_string_list(gpml_string_list);
	d_active_widget_ptr = d_edit_string_list_widget_ptr;
	d_edit_string_list_widget_ptr->show();
}


void
GPlatesQtWidgets::EditWidgetGroupBox::activate_edit_shapefile_attributes_widget(
		GPlatesPropertyValues::GpmlKeyValueDictionary &gpml_key_value_dictionary)
{
	setTitle(tr("%1 Shapefile Attributes").arg(d_edit_verb));
	show();
	d_edit_shapefile_attributes_widget_ptr->update_widget_from_key_value_dictionary(gpml_key_value_dictionary);
	d_active_widget_ptr = d_edit_shapefile_attributes_widget_ptr;
	d_edit_shapefile_attributes_widget_ptr->show();
}

void
GPlatesQtWidgets::EditWidgetGroupBox::activate_edit_time_sequence_widget(
	GPlatesPropertyValues::GpmlArray &gpml_array)
{
	setTitle(tr("%1 Time Sequence").arg(d_edit_verb));
	show();
	d_edit_time_sequence_widget_ptr->update_widget_from_time_period_array(gpml_array);
	d_active_widget_ptr = d_edit_time_sequence_widget_ptr;
	d_edit_time_sequence_widget_ptr->show();
}

void
GPlatesQtWidgets::EditWidgetGroupBox::deactivate_edit_widgets()
{
	hide();
	d_active_widget_ptr = NULL;
	d_edit_time_instant_widget_ptr->hide();
	d_edit_time_instant_widget_ptr->reset_widget_to_default_values();
	d_edit_time_period_widget_ptr->hide();
	d_edit_time_period_widget_ptr->reset_widget_to_default_values();
	d_edit_old_plates_header_widget_ptr->hide();
	d_edit_old_plates_header_widget_ptr->reset_widget_to_default_values();
	d_edit_double_widget_ptr->hide();
	d_edit_double_widget_ptr->reset_widget_to_default_values();
	d_edit_enumeration_widget_ptr->hide();
	d_edit_enumeration_widget_ptr->reset_widget_to_default_values();
	d_edit_geometry_widget_ptr->hide();
	d_edit_geometry_widget_ptr->reset_widget_to_default_values();
	d_edit_integer_widget_ptr->hide();
	d_edit_integer_widget_ptr->reset_widget_to_default_values();
	d_edit_plate_id_widget_ptr->hide();
	d_edit_plate_id_widget_ptr->reset_widget_to_default_values();
	d_edit_polarity_chron_id_widget_ptr->hide();
	d_edit_polarity_chron_id_widget_ptr->reset_widget_to_default_values();
	d_edit_angle_widget_ptr->hide();
	d_edit_angle_widget_ptr->reset_widget_to_default_values();
	d_edit_string_widget_ptr->hide();
	d_edit_string_widget_ptr->reset_widget_to_default_values();
	d_edit_boolean_widget_ptr->hide();
	d_edit_boolean_widget_ptr->reset_widget_to_default_values();
	d_edit_string_list_widget_ptr->hide();
	d_edit_string_list_widget_ptr->reset_widget_to_default_values();
	d_edit_shapefile_attributes_widget_ptr->hide();
	d_edit_time_sequence_widget_ptr->hide();
}



void
GPlatesQtWidgets::EditWidgetGroupBox::edit_widget_wants_committing()
{
	Q_EMIT commit_me();
}


GPlatesQtWidgets::AbstractEditWidget *
GPlatesQtWidgets::EditWidgetGroupBox::get_widget_by_property_value_type(
		const GPlatesPropertyValues::StructuralType &property_value_type)
{
	widget_map_const_iterator result = d_widget_map.find(property_value_type);

	if (result != d_widget_map.end())
	{
		return result->second;
	}
	else
	{
		// FIXME: Exception? When this is finished, we shouldn't really have a situation with
		// an unhandled property-value type in the Add Property dialog.
		return NULL;
	}
}


void
GPlatesQtWidgets::EditWidgetGroupBox::commit_property_to_model()
{
	// Only if the current property belongs to a feature do we need to commit it back to the model.
	if (d_current_property_iterator && d_current_property)
	{
		GPlatesModel::FeatureHandle::iterator &it = *d_current_property_iterator;
		GPlatesModel::TopLevelProperty::non_null_ptr_type &property_clone = *d_current_property;
		*it = property_clone;
	}
}

