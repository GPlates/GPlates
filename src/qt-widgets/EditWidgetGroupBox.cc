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

#include <map>
#include "EditWidgetGroupBox.h"

#include "AbstractEditWidget.h"
#include "EditTimeInstantWidget.h"
#include "EditTimePeriodWidget.h"
#include "EditOldPlatesHeaderWidget.h"
#include "EditDoubleWidget.h"
#include "EditEnumerationWidget.h"
#include "EditGeometryWidget.h"
#include "EditIntegerWidget.h"
#include "EditPlateIdWidget.h"
#include "EditPolarityChronIdWidget.h"
#include "EditAngleWidget.h"
#include "EditStringWidget.h"
#include "EditBooleanWidget.h"
#include "EditShapefileAttributesWidget.h"

#include "EditWidgetChooser.h"
#include "NoActiveEditWidgetException.h"
#include "feature-visitors/PropertyValueFinder.h"
#include "property-values/GpmlPlateId.h"


namespace
{
	/**
	 * EditGeometryWidget is a little special; it wants a Plate ID for reconstruction.
	 * Find it, and set it (or unset it).
	 */
	void
	find_reconstruction_plate_id_for_geometry_widget(
			GPlatesModel::FeatureHandle::weak_ref feature_ref,
			GPlatesQtWidgets::EditGeometryWidget *edit_geometry_widget_ptr)
	{
		static const GPlatesModel::PropertyName plate_id_property_name =
				GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");
		// Find it.
		const GPlatesPropertyValues::GpmlPlateId *recon_plate_id;
		if (GPlatesFeatureVisitors::get_property_value(
				feature_ref, plate_id_property_name, recon_plate_id))
		{
			// Set it.
			edit_geometry_widget_ptr->set_reconstruction_plate_id(
					recon_plate_id->value());
		} else {
			// Unset it.
			edit_geometry_widget_ptr->unset_reconstruction_plate_id();
		}
	}
}


GPlatesQtWidgets::EditWidgetGroupBox::EditWidgetGroupBox(
		GPlatesPresentation::ViewState &view_state_,
		QWidget *parent_):
	d_active_widget_ptr(NULL),
	d_edit_time_instant_widget_ptr(new EditTimeInstantWidget(this)),
	d_edit_time_period_widget_ptr(new EditTimePeriodWidget(this)),
	d_edit_old_plates_header_widget_ptr(new EditOldPlatesHeaderWidget(this)),
	d_edit_double_widget_ptr(new EditDoubleWidget(this)),
	d_edit_enumeration_widget_ptr(new EditEnumerationWidget(this)),
	d_edit_geometry_widget_ptr(new EditGeometryWidget(view_state_, this)),
	d_edit_integer_widget_ptr(new EditIntegerWidget(this)),
	d_edit_plate_id_widget_ptr(new EditPlateIdWidget(this)),
	d_edit_polarity_chron_id_widget_ptr(new EditPolarityChronIdWidget(this)),
	d_edit_angle_widget_ptr(new EditAngleWidget(this)),
	d_edit_string_widget_ptr(new EditStringWidget(this)),
	d_edit_boolean_widget_ptr(new EditBooleanWidget(this)),
	d_edit_shapefile_attributes_widget_ptr(new EditShapefileAttributesWidget(this)),
	d_edit_verb(tr("Edit"))
{
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
	edit_layout->addWidget(d_edit_string_widget_ptr);
	edit_layout->addWidget(d_edit_boolean_widget_ptr);
	edit_layout->addWidget(d_edit_shapefile_attributes_widget_ptr);
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
	QObject::connect(d_edit_string_widget_ptr, SIGNAL(commit_me()),
			this, SLOT(edit_widget_wants_committing()));
	QObject::connect(d_edit_boolean_widget_ptr, SIGNAL(commit_me()),
			this, SLOT(edit_widget_wants_committing()));
	QObject::connect(d_edit_shapefile_attributes_widget_ptr, SIGNAL(commit_me()),
			this, SLOT(edit_widget_wants_committing()));
}


const GPlatesQtWidgets::EditWidgetGroupBox::widget_map_type &
GPlatesQtWidgets::EditWidgetGroupBox::build_widget_map() const
{
	static widget_map_type map;
	map["gml:TimeInstant"] = d_edit_time_instant_widget_ptr;
	map["gml:TimePeriod"] = d_edit_time_period_widget_ptr;
	map["gpml:OldPlatesHeader"] = d_edit_old_plates_header_widget_ptr;
	map["xs:double"] = d_edit_double_widget_ptr;
	map["gpml:ContinentalBoundaryCrustEnumeration"] = d_edit_enumeration_widget_ptr;
	map["gpml:ContinentalBoundaryEdgeEnumeration"] = d_edit_enumeration_widget_ptr;
	map["gpml:ContinentalBoundarySideEnumeration"] = d_edit_enumeration_widget_ptr;
	map["gpml:ReconstructionMethodEnumeration"] = d_edit_enumeration_widget_ptr;
	map["gpml:SubductionPolarityEnumeration"] = d_edit_enumeration_widget_ptr;
	map["gpml:StrikeSlipEnumeration"] = d_edit_enumeration_widget_ptr;
	map["gpml:DipSlipEnumeration"] = d_edit_enumeration_widget_ptr;
	map["gpml:DipSideEnumeration"] = d_edit_enumeration_widget_ptr;
	map["gpml:SlipComponentEnumeration"] = d_edit_enumeration_widget_ptr;
	map["gpml:FoldPlaneAnnotationEnumeration"] = d_edit_enumeration_widget_ptr;
	map["gpml:AbsoluteReferenceFrameEnumeration"] = d_edit_enumeration_widget_ptr;
	map["gml:LineString"] = d_edit_geometry_widget_ptr;
	map["gml:MultiPoint"] = d_edit_geometry_widget_ptr;
	map["gml:Point"] = d_edit_geometry_widget_ptr;
	map["gml:Polygon"] = d_edit_geometry_widget_ptr;
	map["xs:integer"] = d_edit_integer_widget_ptr;
	map["gpml:plateId"] = d_edit_plate_id_widget_ptr;
	map["gpml:PolarityChronId"] = d_edit_polarity_chron_id_widget_ptr;
	map["gpml:angle"] = d_edit_angle_widget_ptr;
	map["xs:string"] = d_edit_string_widget_ptr;
	map["xs:boolean"] = d_edit_boolean_widget_ptr;
#if 0
	// Keep the KeyValueDictionary out of the map until we have the
	// ability to create one. 
	map["gpml:KeyValueDictionary"] = d_edit_shapefile_attributes_widget_ptr;
#endif
	return map;
}


GPlatesQtWidgets::EditWidgetGroupBox::property_types_list_type
GPlatesQtWidgets::EditWidgetGroupBox::get_handled_property_types_list() const
{
	static const widget_map_type map = build_widget_map();
	property_types_list_type list;
	
	widget_map_const_iterator it = map.begin();
	widget_map_const_iterator end = map.end();
	for ( ; it != end; ++it)
	{
		list.push_back(it->first);
	}
	
	return list;
}


void
GPlatesQtWidgets::EditWidgetGroupBox::activate_appropriate_edit_widget(
		GPlatesModel::FeatureHandle::weak_ref feature_ref,
		GPlatesModel::FeatureHandle::iterator it)
{
	if ( ! feature_ref.is_valid())
	{
		// Although in principle we only need the properties_iterator here,
		// not having a valid feature ref is still worth checking for.
		deactivate_edit_widgets();
		return;
	}

	if (!(*it))
	{
		// Always check your property iterators.
		deactivate_edit_widgets();
		return;
	}

	// Get EditWidgetChooser to tell us what widgets to show.
	// Note that we have to make a clone of the property in order to edit it.
	// We also save the iterator so we can save the modified property back into the model.
	deactivate_edit_widgets();
	GPlatesQtWidgets::EditWidgetChooser chooser(*this, feature_ref);
	GPlatesModel::TopLevelProperty::non_null_ptr_type property_clone = (*it)->deep_clone();
	d_current_property_clone = property_clone;
	d_current_property_iterator = it;
	property_clone->accept_visitor(chooser);
}


void
GPlatesQtWidgets::EditWidgetGroupBox::refresh_edit_widget(
		GPlatesModel::FeatureHandle::weak_ref feature_ref,
		GPlatesModel::FeatureHandle::iterator it)
{
	if ( ! feature_ref.is_valid())
	{
		// Although in principle we only need the properties_iterator here,
		// not having a valid feature ref is still worth checking for.
		return;
	}

	if (!(*it))
	{
		// Always check your property iterators.
		return;
	}

	// Get EditWidgetChooser to tell us what widgets to update.
	// Note that we have to make a clone of the property in order to edit it.
	// We also save the iterator so we can save the modified property back into the model.
	GPlatesQtWidgets::EditWidgetChooser chooser(*this, feature_ref);
	GPlatesModel::TopLevelProperty::non_null_ptr_type property_clone = (*it)->deep_clone();
	d_current_property_clone = property_clone;
	d_current_property_iterator = it;
	property_clone->accept_visitor(chooser);
}


void
GPlatesQtWidgets::EditWidgetGroupBox::activate_widget_by_property_value_name(
		const QString &property_value_name)
{
	deactivate_edit_widgets();
	AbstractEditWidget *widget_ptr = get_widget_by_name(property_value_name);
	
	if (widget_ptr != NULL)
	{
		// FIXME: Human readable property value name?
		setTitle(tr("%1 %2").arg(d_edit_verb).arg(property_value_name));
		show();
		d_active_widget_ptr = widget_ptr;
		widget_ptr->reset_widget_to_default_values();
		widget_ptr->configure_for_property_value_type(property_value_name);
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
	setTitle(tr("%1 Old PLATES Header").arg(d_edit_verb));
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
		GPlatesPropertyValues::GmlLineString &gml_line_string,
		GPlatesModel::FeatureHandle::weak_ref feature_ref)
{
	setTitle(tr("%1 Polyline").arg(d_edit_verb));
	show();
	d_edit_geometry_widget_ptr->update_widget_from_line_string(gml_line_string);
	
	// EditGeometryWidget is a little special; it wants a Plate ID for reconstruction.
	// Find it, and set it (or unset it).
	find_reconstruction_plate_id_for_geometry_widget(feature_ref, d_edit_geometry_widget_ptr);
	
	d_active_widget_ptr = d_edit_geometry_widget_ptr;
	d_edit_geometry_widget_ptr->show();
}

void
GPlatesQtWidgets::EditWidgetGroupBox::activate_edit_multi_point_widget(
		GPlatesPropertyValues::GmlMultiPoint &gml_multi_point,
		GPlatesModel::FeatureHandle::weak_ref feature_ref)
{
	setTitle(tr("%1 Multi-Point").arg(d_edit_verb));
	show();
	d_edit_geometry_widget_ptr->update_widget_from_multi_point(gml_multi_point);
	
	// EditGeometryWidget is a little special; it wants a Plate ID for reconstruction.
	// Find it, and set it (or unset it).
	find_reconstruction_plate_id_for_geometry_widget(feature_ref, d_edit_geometry_widget_ptr);
	
	d_active_widget_ptr = d_edit_geometry_widget_ptr;
	d_edit_geometry_widget_ptr->show();
}

void
GPlatesQtWidgets::EditWidgetGroupBox::activate_edit_point_widget(
		GPlatesPropertyValues::GmlPoint &gml_point,
		GPlatesModel::FeatureHandle::weak_ref feature_ref)
{
	setTitle(tr("%1 Point").arg(d_edit_verb));
	show();
	d_edit_geometry_widget_ptr->update_widget_from_point(gml_point);
	
	// EditGeometryWidget is a little special; it wants a Plate ID for reconstruction.
	// Find it, and set it (or unset it).
	find_reconstruction_plate_id_for_geometry_widget(feature_ref, d_edit_geometry_widget_ptr);
	
	d_active_widget_ptr = d_edit_geometry_widget_ptr;
	d_edit_geometry_widget_ptr->show();
}

void
GPlatesQtWidgets::EditWidgetGroupBox::activate_edit_polygon_widget(
		GPlatesPropertyValues::GmlPolygon &gml_polygon,
		GPlatesModel::FeatureHandle::weak_ref feature_ref)
{
	setTitle(tr("%1 Polygon").arg(d_edit_verb));
	show();
	d_edit_geometry_widget_ptr->update_widget_from_polygon(gml_polygon);
	
	// EditGeometryWidget is a little special; it wants a Plate ID for reconstruction.
	// Find it, and set it (or unset it).
	find_reconstruction_plate_id_for_geometry_widget(feature_ref, d_edit_geometry_widget_ptr);
	
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
	d_edit_shapefile_attributes_widget_ptr->hide();
}



void
GPlatesQtWidgets::EditWidgetGroupBox::edit_widget_wants_committing()
{
	emit commit_me();
}


GPlatesQtWidgets::AbstractEditWidget *
GPlatesQtWidgets::EditWidgetGroupBox::get_widget_by_name(
		const QString &property_value_type_name)
{
	static const widget_map_type &map = build_widget_map();
	widget_map_const_iterator result = map.find(property_value_type_name);

	if (result != map.end())
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
	if (d_current_property_iterator && d_current_property_clone)
	{
		GPlatesModel::FeatureHandle::iterator &it = *d_current_property_iterator;
		GPlatesModel::TopLevelProperty::non_null_ptr_type &property_clone = *d_current_property_clone;
		*it = property_clone;
	}
}

