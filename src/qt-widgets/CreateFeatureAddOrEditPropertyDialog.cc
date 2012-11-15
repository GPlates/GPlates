/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
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

#include <algorithm>
#include <boost/optional.hpp>
#include <QDebug>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QStringList>

#include "CreateFeatureAddOrEditPropertyDialog.h"
#include "EditWidgetGroupBox.h"
#include "InvalidPropertyValueException.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "model/GpgimProperty.h"
#include "model/GpgimStructuralType.h"
#include "model/ModelUtils.h"
#include "model/PropertyName.h"
#include "model/TopLevelPropertyInline.h"

#include "property-values/StructuralType.h"

#include "utils/UnicodeStringUtils.h"

#include <boost/foreach.hpp>

GPlatesQtWidgets::CreateFeatureAddOrEditPropertyDialog::CreateFeatureAddOrEditPropertyDialog(
		GPlatesPresentation::ViewState &view_state_,
		QWidget *parent_):
	QDialog(parent_, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	d_edit_widget_group_box(new EditWidgetGroupBox(view_state_, this))
{
	setupUi(this);

	set_up_edit_widgets();
}


bool
GPlatesQtWidgets::CreateFeatureAddOrEditPropertyDialog::is_property_supported(
		const GPlatesModel::GpgimProperty &gpgim_property) const
{
	return d_edit_widget_group_box->get_handled_property_types(gpgim_property);
}


boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type>
GPlatesQtWidgets::CreateFeatureAddOrEditPropertyDialog::add_property(
		const GPlatesModel::GpgimProperty &gpgim_feature_property)
{
	d_add_property = AddProperty(gpgim_feature_property);

	// Set the property name label.
	property_name_line_edit->setText(
			convert_qualified_xml_name_to_qstring(
					gpgim_feature_property.get_property_name()));

	// Populate the property types combobox with the types allowed for the GPGIM property.
	populate_add_property_type_combobox(gpgim_feature_property);

	// Enable both the "OK" and "Cancel" buttons.
	buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

	// When the user presses "OK" we need to attempt to create the new property.
	// If that fails then the dialog will not be accepted or rejected and then user must either
	// try editing the property again or press "Cancel".
	QObject::disconnect(buttonBox, SIGNAL(accepted()), 0, 0);
	QObject::connect(buttonBox, SIGNAL(accepted()), this, SLOT(create_property_from_edit_widget()));

	d_edit_widget_group_box->set_edit_verb("Add");

	// Set the focus to the edit widget since the user cannot change the property name and, for
	// most property types, they will not change the property type (will only be one type) so
	// they'll want to start editing the property immediately.
	d_edit_widget_group_box->setFocus();

	if (exec() != QDialog::Accepted)
	{
		// The user canceled the addition of a new property.
		d_add_property = boost::none;
		return boost::none;
	}

	boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type> feature_property =
			d_add_property->feature_property;

	d_add_property = boost::none;

	return feature_property;
}


void
GPlatesQtWidgets::CreateFeatureAddOrEditPropertyDialog::edit_property(
		const GPlatesModel::TopLevelProperty::non_null_ptr_type &feature_property)
{
	// Set the property name label.
	property_name_line_edit->setText(
			convert_qualified_xml_name_to_qstring(
					feature_property->property_name()));

	// Populate the property type combobox with the single type of the specified feature property.
	populate_edit_property_type_combobox(feature_property);

	// Enable only the "OK" button.
	// The "Cancel" button is not needed - any edits the user does cannot be undone.
	buttonBox->setStandardButtons(QDialogButtonBox::Ok);

	// When the user presses "OK" we need to update the feature property from the edit widget.
	QObject::disconnect(buttonBox, SIGNAL(accepted()), 0, 0);
	QObject::connect(buttonBox, SIGNAL(accepted()), this, SLOT(update_property_from_edit_widget()));

	// Activate the appropriate edit widget based on the type of the feature property.
	d_edit_widget_group_box->activate_appropriate_edit_widget(feature_property);

	d_edit_widget_group_box->set_edit_verb("Edit");

	// Set the focus to the edit widget since the user cannot change the property name or type so
	// they'll want to start editing the property immediately.
	d_edit_widget_group_box->setFocus();

	exec();
}


void
GPlatesQtWidgets::CreateFeatureAddOrEditPropertyDialog::set_up_edit_widgets()
{
	// Add the EditWidgetGroupBox. Ugly, but this is the price to pay if you want
	// to mix Qt Designer UIs with coded-by-hand UIs.
	QVBoxLayout *edit_layout = new QVBoxLayout;
	edit_layout->setSpacing(0);
	edit_layout->setContentsMargins(0, 0, 0, 0);
	edit_layout->addWidget(d_edit_widget_group_box);
	placeholder_edit_widget->setLayout(edit_layout);

	QObject::connect(d_edit_widget_group_box, SIGNAL(commit_me()), buttonBox, SLOT(setFocus()));
}


void
GPlatesQtWidgets::CreateFeatureAddOrEditPropertyDialog::connect_to_combobox_property_type_signals(
		bool connects_signal_slot)
{
	if (connects_signal_slot)
	{
		// Choose appropriate edit widget for property type.
		QObject::connect(combobox_property_type, SIGNAL(currentIndexChanged(int)),
				this, SLOT(set_appropriate_edit_widget_by_property_value_type()));
	}
	else // disconnect...
	{
		// Disconnect this receiver from all signals from 'combobox_property_type':
		QObject::disconnect(combobox_property_type, 0, this, 0);
	}
}


void
GPlatesQtWidgets::CreateFeatureAddOrEditPropertyDialog::set_appropriate_edit_widget_by_property_value_type()
{
	// Get the property value type from the property_name_combobox text.
	boost::optional<GPlatesPropertyValues::StructuralType> property_value_type =
			GPlatesModel::convert_qstring_to_qualified_xml_name<
					GPlatesPropertyValues::StructuralType>(
							combobox_property_type->currentText());
	// Should always be able to convert to a qualified XML name.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			property_value_type,
			GPLATES_ASSERTION_SOURCE);

	d_edit_widget_group_box->activate_widget_by_property_value_type(property_value_type.get());
}


void
GPlatesQtWidgets::CreateFeatureAddOrEditPropertyDialog::populate_add_property_type_combobox(
		const GPlatesModel::GpgimProperty &gpgim_property)
{
	// Temporarily disconnect slots from the combobox.
	// This avoids updates (such as setting up the appropriate edit widget) when we clear this combobox.
	connect_to_combobox_property_type_signals(false/*connects_signal_slot*/);

	// Clear the combobox.
	combobox_property_type->clear();

	// Re-connects_signal_slot slots to the combobox.
	connect_to_combobox_property_type_signals(true/*connects_signal_slot*/);

	//
	// Add the property types allowed for the GPGIM property.
	//

	// Index into structural type combobox.
	int structural_type_index = -1;

	// Get the sequence of structural types allowed (by GPGIM) for the GPGIM property.
	// Only add property types supported by edit widgets, there's no point listing structural types
	// that do not have an edit widget.
	EditWidgetGroupBox::property_types_list_type structural_types;
	if (!d_edit_widget_group_box->get_handled_property_types(gpgim_property, structural_types))
	{
		// None of the current property's structural types are supported by edit widgets.
		return;
	}

	// The default structural type for the current property.
	const GPlatesPropertyValues::StructuralType &default_structural_type =
			gpgim_property.get_default_structural_type()->get_structural_type();

	BOOST_FOREACH(
			const GPlatesPropertyValues::StructuralType &structural_type,
			structural_types)
	{
		combobox_property_type->addItem(convert_qualified_xml_name_to_qstring(structural_type));

		// Get the combobox index of the 'default' structural type.
		if (structural_type == default_structural_type)
		{
			structural_type_index = combobox_property_type->count() - 1;
		}
	}

	// Set the combobox index to the default structural type.
	if (structural_type_index >= 0)
	{
		combobox_property_type->setCurrentIndex(structural_type_index);
	}
}


void
GPlatesQtWidgets::CreateFeatureAddOrEditPropertyDialog::populate_edit_property_type_combobox(
		const GPlatesModel::TopLevelProperty::non_null_ptr_type &feature_property)
{
	// Disconnect slots from the combobox.
	// There's only one property type in the combobox - it's really just for display purposes -
	// we don't want the user to change it (that's for the "add" property code path).
	connect_to_combobox_property_type_signals(false/*connects_signal_slot*/);

	// Clear the combobox.
	combobox_property_type->clear();

	//
	// Only allow the property type of the feature property being edited.
	// For example, the user cannot change the type from 'gml:Point' to 'gml:LineString'.
	//

	boost::optional<GPlatesModel::PropertyValue::non_null_ptr_to_const_type> property_value =
			GPlatesModel::ModelUtils::get_property_value(*feature_property);
	// Should always have a valid *inline* top-level property.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			property_value,
			GPLATES_ASSERTION_SOURCE);

	const GPlatesPropertyValues::StructuralType property_type =
			GPlatesModel::ModelUtils::get_non_time_dependent_property_structural_type(
					*property_value.get());


	// Set the property value type in the combobox.
	combobox_property_type->addItem(convert_qualified_xml_name_to_qstring(property_type));

	// Set the index to the only property type in the combobox.
	combobox_property_type->setCurrentIndex(0);
}


void
GPlatesQtWidgets::CreateFeatureAddOrEditPropertyDialog::create_property_from_edit_widget()
{
	if (!d_edit_widget_group_box->is_edit_widget_active())
	{
		// Something is wrong - we have no edit widget available.
		QMessageBox::warning(this, tr("Unable to add property"),
				tr("Sorry! Since there is no editing control available for this property "
				"value yet, it cannot be added to the feature."),
				QMessageBox::Ok);
		return;
	}

	// Should only be able to get here from the 'add_property()' method which sets 'd_add_property'.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_add_property,
			GPLATES_ASSERTION_SOURCE);

	try
	{
		// Get the edit widget to create the property value.
		GPlatesModel::PropertyValue::non_null_ptr_type property_value =
				d_edit_widget_group_box->create_property_value_from_widget();

		// Create the top-level property.
		GPlatesModel::ModelUtils::TopLevelPropertyError::Type error_code;
		d_add_property->feature_property =
				GPlatesModel::ModelUtils::create_top_level_property(
						*d_add_property->gpgim_property,
						property_value,
						&error_code);
		if (!d_add_property->feature_property)
		{
			// Not successful in creating property; show error message.
			QMessageBox::warning(this,
					QObject::tr("Unable to add property."),
					QObject::tr(GPlatesModel::ModelUtils::get_error_message(error_code)),
					QMessageBox::Ok);
			return;
		}

		accept();

	}
	catch (const InvalidPropertyValueException &e)
	{
		// Not enough points for a constructible polyline, etc.
		QMessageBox::warning(this, tr("Property Value Invalid"),
				tr("The property can not be added: %1").arg(e.reason()),
				QMessageBox::Ok);
		return;
	}
}


void
GPlatesQtWidgets::CreateFeatureAddOrEditPropertyDialog::update_property_from_edit_widget()
{
	if (!d_edit_widget_group_box->is_edit_widget_active())
	{
		// Something is wrong - we have no edit widget available.
		QMessageBox::warning(this, tr("Unable to edit property"),
				tr("Sorry! Since there is no editing control available for this property "
				"value yet, it cannot be added to the feature."),
				QMessageBox::Ok);

		// There's no "Cancel" button for the user so we need to reject the dialog to close it.
		reject();
		return;
	}

	// Need to commit edit widget data back to the feature property in case the user edits the property.
	// Update the property value.
	d_edit_widget_group_box->update_property_value_from_widget();

	accept();
}
