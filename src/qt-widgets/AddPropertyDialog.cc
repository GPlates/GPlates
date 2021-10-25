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
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <QDebug>
#include <QMessageBox>
#include <QStringList>

#include "AddPropertyDialog.h"
#include "InvalidPropertyValueException.h"

#include "app-logic/ApplicationState.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "gui/FeatureFocus.h"

#include "model/Gpgim.h"
#include "model/GpgimFeatureClass.h"
#include "model/GpgimProperty.h"
#include "model/GpgimStructuralType.h"
#include "model/ModelUtils.h"
#include "model/PropertyName.h"

#include "presentation/ViewState.h"

#include "property-values/StructuralType.h"

#include "utils/UnicodeStringUtils.h"

namespace
{
	bool
	feature_has_property_name(
			const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
			const GPlatesModel::PropertyName &property_name)
	{
		if (!feature_ref.is_valid())
		{
			return false;
		}

		// Iterate over all feature properties.
		GPlatesModel::FeatureHandle::const_iterator iter = feature_ref->begin();
		GPlatesModel::FeatureHandle::const_iterator end = feature_ref->end();
		for ( ; iter != end; ++iter)
		{
			if ((*iter)->property_name() == property_name)
			{
				// Found a matching property name.
				return true;
			}
		}

		return false;
	}
}


const GPlatesModel::FeatureType &
GPlatesQtWidgets::AddPropertyDialog::get_default_feature_type()
{
	static const GPlatesModel::FeatureType DEFAULT_FEATURE_TYPE =
			GPlatesModel::FeatureType::create_gml("AbstractFeature");

	return DEFAULT_FEATURE_TYPE;
}


GPlatesQtWidgets::AddPropertyDialog::AddPropertyDialog(
		GPlatesGui::FeatureFocus &feature_focus_,
		GPlatesPresentation::ViewState &view_state_,
		QWidget *parent_):
	QDialog(parent_, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	d_feature_focus(feature_focus_),
	// Start off with the most basic feature type.
	// It's actually an 'abstract' feature but it'll get reset to a 'concrete' feature...
	d_feature_type(get_default_feature_type()),
	d_edit_widget_group_box_ptr(new GPlatesQtWidgets::EditWidgetGroupBox(view_state_, this))
{
	setupUi(this);

	set_up_add_property_box();
	set_up_edit_widgets();

	populate_property_name_combobox();
	reset();
}


void
GPlatesQtWidgets::AddPropertyDialog::set_feature(
		const GPlatesModel::FeatureHandle::weak_ref &new_feature_ref)
{
	d_feature_ref = new_feature_ref;

	// Determine the new feature type.
	d_feature_type = new_feature_ref.is_valid()
			? new_feature_ref->feature_type()
			: get_default_feature_type();

	// NOTE: We always populate the list of property names even if the feature reference is
	// the same (as before) and the feature type is the same - because it's possible the properties
	// of the feature have changed which (due to GPGIM property multiplicity) can affect which
	// properties get listed. The alternative is to listen to model callbacks but this is easier
	// since the Add Property dialog is modal nothing within the dialog can change the feature
	// (until the property is added and then the dialog closes).
	populate_property_name_combobox();
	reset();
}


void
GPlatesQtWidgets::AddPropertyDialog::reset()
{
	// Choose a property name that all feature types have.
	// "gml:name" is a property of "gml:AbstractFeature" which all feature types inherit from.
	static const int default_property_name_index = combobox_add_property_name->findText("gml:name");

	combobox_add_property_name->setCurrentIndex(default_property_name_index);
}


void
GPlatesQtWidgets::AddPropertyDialog::pop_up()
{
	reset();
	exec();
}


void
GPlatesQtWidgets::AddPropertyDialog::connect_to_combobox_add_property_name_signals(
		bool connects_signal_slot)
{
	if (connects_signal_slot)
	{
		// Choose appropriate property value type for property name.
		QObject::connect(combobox_add_property_name, SIGNAL(currentIndexChanged(int)),
				this, SLOT(populate_property_type_combobox()));
		// Check if the selected property name is appropriate for the current feature's type.
		QObject::connect(combobox_add_property_name, SIGNAL(currentIndexChanged(int)),
				this, SLOT(check_property_name_validity()));
	}
	else // disconnect...
	{
		// Disconnect this receiver from all signals from 'combobox_add_property_name':
		QObject::disconnect(combobox_add_property_name, 0, this, 0);
	}
}


void
GPlatesQtWidgets::AddPropertyDialog::connect_to_combobox_add_property_type_signals(
		bool connects_signal_slot)
{
	if (connects_signal_slot)
	{
		// Choose appropriate edit widget for property type.
		QObject::connect(combobox_add_property_type, SIGNAL(currentIndexChanged(int)),
				this, SLOT(set_appropriate_edit_widget()));
	}
	else // disconnect...
	{
		// Disconnect this receiver from all signals from 'combobox_add_property_type':
		QObject::disconnect(combobox_add_property_type, 0, this, 0);
	}
}


void
GPlatesQtWidgets::AddPropertyDialog::set_up_add_property_box()
{
	connect_to_combobox_add_property_name_signals(true/*connect*/);
	connect_to_combobox_add_property_type_signals(true/*connect*/);

	// Add property when user hits "Add".
	QObject::connect(buttonBox, SIGNAL(accepted()),
			this, SLOT(add_property()));
}


void
GPlatesQtWidgets::AddPropertyDialog::set_up_edit_widgets()
{
	// Add the EditWidgetGroupBox. Ugly, but this is the price to pay if you want
	// to mix Qt Designer UIs with coded-by-hand UIs.
	QVBoxLayout *edit_layout = new QVBoxLayout;
	edit_layout->setSpacing(0);
	edit_layout->setMargin(0);
	edit_layout->addWidget(d_edit_widget_group_box_ptr);
	placeholder_edit_widget->setLayout(edit_layout);
	
	d_edit_widget_group_box_ptr->set_edit_verb("Add");
	QObject::connect(d_edit_widget_group_box_ptr, SIGNAL(commit_me()),
			buttonBox, SLOT(setFocus()));
}


void
GPlatesQtWidgets::AddPropertyDialog::set_appropriate_edit_widget()
{
	boost::optional<EditWidgetGroupBox::property_value_type> type_of_property;

	// Get the property value type from the combobox_add_property_type text.
	const QString combobox_property_type_string = combobox_add_property_type->currentText();

	// See if property type string is a template type (has a value type between '<' and '>').
	const int template_left_angle_bracket_index = combobox_property_type_string.indexOf('<');
	if (template_left_angle_bracket_index >= 0 &&
		combobox_property_type_string.endsWith('>'))
	{
		const boost::optional<GPlatesPropertyValues::StructuralType> structural_type =
				GPlatesModel::convert_qstring_to_qualified_xml_name<GPlatesPropertyValues::StructuralType>(
						combobox_property_type_string.left(template_left_angle_bracket_index));
		const boost::optional<GPlatesPropertyValues::StructuralType> value_type =
				GPlatesModel::convert_qstring_to_qualified_xml_name<GPlatesPropertyValues::StructuralType>(
						combobox_property_type_string.mid(
								template_left_angle_bracket_index + 1,
								combobox_property_type_string.size() - template_left_angle_bracket_index - 2));
		// Should always be able to convert to a qualified XML name.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				structural_type && value_type,
				GPLATES_ASSERTION_SOURCE);

		type_of_property = EditWidgetGroupBox::property_value_type(structural_type.get(), value_type.get());
	}
	else
	{
		const boost::optional<GPlatesPropertyValues::StructuralType> structural_type =
				GPlatesModel::convert_qstring_to_qualified_xml_name<GPlatesPropertyValues::StructuralType>(
						combobox_property_type_string);
		// Should always be able to convert to a qualified XML name.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				structural_type,
				GPLATES_ASSERTION_SOURCE);

		type_of_property = structural_type.get();
	}

	d_edit_widget_group_box_ptr->activate_widget_by_property_type(type_of_property.get());
}


void
GPlatesQtWidgets::AddPropertyDialog::check_property_name_validity()
{
	boost::optional<GPlatesModel::PropertyName> property_name =
			GPlatesModel::convert_qstring_to_qualified_xml_name<GPlatesModel::PropertyName>(
					combobox_add_property_name->currentText());
	if (!property_name)
	{
		static const char *WARNING_TEXT = "Internal error: '%1' is a malformed property name.";
		label_warning->setText(tr(WARNING_TEXT)
				.arg(combobox_add_property_name->currentText()));

		widget_warning->setVisible(true);
		return;
	}

	// Check whether the selected property name is valid for the feature type.
	if (!GPlatesModel::Gpgim::instance().get_feature_property(d_feature_type, property_name.get()))
	{
		static const char *WARNING_TEXT = "'%1' is not a valid property for a '%2' feature.";
		label_warning->setText(tr(WARNING_TEXT)
				.arg(combobox_add_property_name->currentText())
				.arg(convert_qualified_xml_name_to_qstring(d_feature_type)));

		widget_warning->setVisible(true);
		return;
	}

	widget_warning->setVisible(false);
}


void
GPlatesQtWidgets::AddPropertyDialog::add_property()
{
	if (!d_feature_ref.is_valid())
	{
		QMessageBox::warning(this, tr("Unable to add property"),
				tr("The feature, to contain the property, is no longer valid."),
				QMessageBox::Ok);
		return;
	}

	if (d_edit_widget_group_box_ptr->is_edit_widget_active()) {
		// Calculate name.
		boost::optional<GPlatesModel::PropertyName> property_name = 
				GPlatesModel::convert_qstring_to_qualified_xml_name<GPlatesModel::PropertyName>(
						combobox_add_property_name->currentText());
		if (property_name) {
		
			// If the name was something we understood, create the property value too.
			try {
				GPlatesModel::PropertyValue::non_null_ptr_type property_value =
						d_edit_widget_group_box_ptr->create_property_value_from_widget();

				// Add the property to the feature.
				GPlatesModel::ModelUtils::TopLevelPropertyError::Type add_property_error_code;
				if (!GPlatesModel::ModelUtils::add_property(
						d_feature_ref,
						property_name.get(),
						property_value,
						// We're allowing *any* property to be added to the feature...
						false/*check_property_name_allowed_for_feature_type*/,
						false/*check_property_multiplicity*/,
						false/*check_property_value_type*/,
						&add_property_error_code))
				{
					// Not successful in adding property; show error message.
					QMessageBox::warning(this,
							QObject::tr("Unable to add property."),
							QObject::tr(GPlatesModel::ModelUtils::get_error_message(add_property_error_code)),
							QMessageBox::Ok);
					return;
				}

				// We have just changed the model. Tell anyone who cares to know.
				d_feature_focus.announce_modification_of_focused_feature();

			} catch (const InvalidPropertyValueException &e) {
				// Not enough points for a constructable polyline, etc.
				QMessageBox::warning(this, tr("Property Value Invalid"),
						tr("The property can not be added: %1").arg(e.reason()),
						QMessageBox::Ok);
				return;
			}
		} else {
			// User supplied an incomprehensible property name.
			QMessageBox::warning(this, tr("Property Name Invalid"),
					tr("The supplied property name could not be understood. "
					"Please restrict property names to the 'gml:' or 'gpml:' namespace."),
					QMessageBox::Ok);
			return;
		}
	} else {
		// Something is wrong - we have no edit widget available.
		QMessageBox::warning(this, tr("Unable to add property"),
				tr("Sorry! Since there is no editing control available for this property "
				"value yet, it cannot be added to the feature."),
				QMessageBox::Ok);
		return;
	}
	accept();
}


void
GPlatesQtWidgets::AddPropertyDialog::populate_property_name_combobox()
{
	// Icons to indicate useful properties. Blanks to keep list spaced properly.
	static const QIcon allowed_property_icon(":/gnome_emblem_new_16.png");
	static const QIcon blank_icon(":/blank_16.png");

	// Temporarily disconnect slots from the combobox.
	// This avoids updates (such as to the property 'type' combobox) when we clear this combobox.
	connect_to_combobox_add_property_name_signals(false/*connects_signal_slot*/);

	// Clear the combobox first.
	combobox_add_property_name->clear();

	// Re-connects_signal_slot slots to the combobox.
	connect_to_combobox_add_property_name_signals(true/*connects_signal_slot*/);

	// First add the property names allowed for the current feature type as dictated by the GPGIM
	// along with their suggested property structural type. This list can't (easily) be in bold,
	// because we'd have to implement it as a Model/View rather than a simple item list,
	// but we can set a "Favourite" icon or somesuch.
	//
	// Query the GPGIM for the feature class.
	boost::optional<GPlatesModel::GpgimFeatureClass::non_null_ptr_to_const_type> gpgim_feature_class =
			GPlatesModel::Gpgim::instance().get_feature_class(d_feature_type);
	if (gpgim_feature_class)
	{
		// Get allowed properties for the feature type.
		GPlatesModel::GpgimFeatureClass::gpgim_property_seq_type gpgim_feature_properties;
		gpgim_feature_class.get()->get_feature_properties(gpgim_feature_properties);

		// Iterate over the allowed properties for the feature type and add each property name
		// and associated default structural type.
		BOOST_FOREACH(
				const GPlatesModel::GpgimProperty::non_null_ptr_to_const_type &gpgim_feature_property,
				gpgim_feature_properties)
		{
			// Only add property types supported by edit widgets, otherwise the user will be
			// left with the inability to actually add their selected property.
			if ( ! d_edit_widget_group_box_ptr->get_handled_property_types(*gpgim_feature_property))
			{
				continue;
			}

			// If the current property is allowed to occur at most once per feature then only allow
			// the user to add the property if it doesn't already exist in the feature.
			if (gpgim_feature_property->get_multiplicity() == GPlatesModel::GpgimProperty::ZERO_OR_ONE ||
				gpgim_feature_property->get_multiplicity() == GPlatesModel::GpgimProperty::ONE)
			{
				if (feature_has_property_name(d_feature_ref, gpgim_feature_property->get_property_name()))
				{
					continue;
				}
			}

			// Passed all tests so we can add the current property name.
			combobox_add_property_name->addItem(
					allowed_property_icon,
					convert_qualified_xml_name_to_qstring(gpgim_feature_property->get_property_name()));
		}
	}
	else
	{
		// FIXME: Is this an exceptional state?
		qWarning() << "Internal error: Unable to find feature type '"
			<< convert_qualified_xml_name_to_qstring(d_feature_type)
			<< "' in the GPGIM.";
	}

	// Then add all property names present in the GPGIM.
	// This will also duplicate the feature properties added above (but this time they'll only have
	// a blank icon to indicate they are not allowed by the GPGIM for the current feature type).
	const GPlatesModel::Gpgim::property_seq_type &gpgim_properties =
			GPlatesModel::Gpgim::instance().get_properties();
	BOOST_FOREACH(
			const GPlatesModel::GpgimProperty::non_null_ptr_to_const_type &gpgim_property,
			gpgim_properties)
	{
		// Only add property types supported by edit widgets, otherwise the user will be
		// left with the inability to actually add their selected property.
		if (d_edit_widget_group_box_ptr->get_handled_property_types(*gpgim_property))
		{
			combobox_add_property_name->addItem(
					blank_icon,
					convert_qualified_xml_name_to_qstring(gpgim_property->get_property_name()));
		}
	}
}


void
GPlatesQtWidgets::AddPropertyDialog::populate_property_type_combobox()
{
	// Temporarily disconnect slots from the combobox.
	// This avoids updates (such as setting up the appropriate edit widget) when we clear this combobox.
	connect_to_combobox_add_property_type_signals(false/*connects_signal_slot*/);

	// Clear the combobox.
	combobox_add_property_type->clear();

	// Re-connects_signal_slot slots to the combobox.
	connect_to_combobox_add_property_type_signals(true/*connects_signal_slot*/);

	// Get the current property name.
	boost::optional<GPlatesModel::PropertyName> property_name = 
			GPlatesModel::convert_qstring_to_qualified_xml_name<GPlatesModel::PropertyName>(
					combobox_add_property_name->currentText());
	if (!property_name)
	{
		// FIXME: Is this an exceptional state?
		qWarning() << "Internal error: '"
			<< combobox_add_property_name->currentText()
			<< "' is a malformed property name.";
		return;
	}

	//
	// Add the property types allowed for the current property name as dictated by the GPGIM.
	//

	// Query the GPGIM for the property definition.
	boost::optional<GPlatesModel::GpgimProperty::non_null_ptr_to_const_type> gpgim_property =
			GPlatesModel::Gpgim::instance().get_property(property_name.get());
	if (!gpgim_property)
	{
		// FIXME: Is this an exceptional state?
		qWarning() << "Internal error: Unable to find property name '"
			<< convert_qualified_xml_name_to_qstring(property_name.get())
			<< "' in the GPGIM - no property structural types will be listed.";
		return;
	}

	// Index into structural type combobox.
	int structural_type_index = -1;

	// Get the sequence of structural types allowed (by GPGIM) for the current property name.
	// Only add property types supported by edit widgets, there's no point listing structural types
	// that do not have an edit widget.
	EditWidgetGroupBox::property_types_list_type property_types;
	if (!d_edit_widget_group_box_ptr->get_handled_property_types(*gpgim_property.get(), property_types))
	{
		// None of the current property's structural types are supported by edit widgets.
		return;
	}

	// The default structural type for the current property.
	const GPlatesPropertyValues::StructuralType &default_structural_type =
			gpgim_property.get()->get_default_structural_type()->get_structural_type();

	BOOST_FOREACH(const EditWidgetGroupBox::property_value_type &type_of_property, property_types)
	{
		if (type_of_property.is_template())
		{
			combobox_add_property_type->addItem(QString("%1<%2>")
					.arg(convert_qualified_xml_name_to_qstring(type_of_property.get_structural_type()))
					.arg(convert_qualified_xml_name_to_qstring(type_of_property.get_value_type().get())));
		}
		else
		{
			combobox_add_property_type->addItem(
					convert_qualified_xml_name_to_qstring(type_of_property.get_structural_type()));
		}

		// Get the combobox index of the 'default' structural type.
		if (type_of_property.get_structural_type() == default_structural_type)
		{
			structural_type_index = combobox_add_property_type->count() - 1;
		}
	}

	// Set the combobox index to the default structural type.
	if (structural_type_index >= 0)
	{
		combobox_add_property_type->setCurrentIndex(structural_type_index);
	}
}
