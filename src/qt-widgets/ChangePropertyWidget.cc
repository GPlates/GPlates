/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#include <boost/static_assert.hpp>
#include <QMessageBox>

#include "ChangePropertyWidget.h"

#include "ChoosePropertyWidget.h"
#include "QtWidgetUtils.h"

#include "global/CompilerWarnings.h"

#include "gui/FeatureFocus.h"

#include "model/Gpgim.h"
#include "model/GpgimProperty.h"
#include "model/ModelUtils.h"

#include "utils/UnicodeStringUtils.h"


GPlatesQtWidgets::ChangePropertyWidget::ChangePropertyWidget(
		const GPlatesModel::Gpgim &gpgim,
		const GPlatesGui::FeatureFocus &feature_focus,
		QWidget *parent_) :
	QWidget(parent_),
	d_gpgim(gpgim),
	d_feature_focus(feature_focus),
	d_property_destinations_widget(
			new ChoosePropertyWidget(
				gpgim,
				SelectionWidget::Q_COMBO_BOX,
				this))
{
	setupUi(this);

	// Save the checkbox's text that was set in the Designer.
	d_default_explanatory_text = change_property_checkbox->text();

	QtWidgetUtils::add_widget_to_placeholder(
			d_property_destinations_widget, property_destinations_placeholder_widget);
	property_destinations_placeholder_widget->setMinimumSize(
			d_property_destinations_widget->sizeHint());

	// Checkbox signals.
	QObject::connect(
			change_property_checkbox,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(handle_checkbox_state_changed(int)));
}


void
GPlatesQtWidgets::ChangePropertyWidget::handle_checkbox_state_changed(
		int state)
{
	d_property_destinations_widget->setEnabled(state == Qt::Checked);
}


void
GPlatesQtWidgets::ChangePropertyWidget::populate(
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
		const GPlatesModel::FeatureHandle::iterator &feature_property,
		const GPlatesPropertyValues::StructuralType &feature_property_type,
		const GPlatesModel::FeatureType &new_feature_type)
{
	d_feature_ref = feature_ref;
	d_property = feature_property;

	if (d_feature_ref.is_valid() && feature_property.is_still_valid())
	{
		// Set up the combobox.
		d_property_destinations_widget->populate(new_feature_type, feature_property_type, feature_ref);

		const GPlatesModel::PropertyName &curr_property_name = (*feature_property)->get_property_name();

		// Get the user-friendly property name from the GPGIM.
		boost::optional<GPlatesModel::GpgimProperty::non_null_ptr_to_const_type> curr_gpgim_property =
				d_gpgim.get_property(curr_property_name);
		const QString curr_property_user_friendly_name = curr_gpgim_property
				? curr_gpgim_property.get()->get_user_friendly_name()
				: GPlatesUtils::make_qstring_from_icu_string(curr_property_name.get_name());

		// Display some explanatory text.
		change_property_checkbox->setText(
				// Put user-friendly property name in quotes since it can contain whitespace...
				QString("'%1'").arg(
						d_default_explanatory_text.arg(
								curr_property_user_friendly_name.toLower())));

		// The checkbox is on by default.
		Qt::CheckState change_property_checkbox_state = Qt::Checked;

		// Most property types have well-defined meanings like 'gml:TimePeriod' and hence it's
		// possible that two different feature types with the same property type but different
		// property names will still have the same meaning for the property.
		// However there are some types like 'xsi:string' that are very generic and could mean
		// anything - for these types we'll leave the "change property" checkbox unchecked -
		// the user can still check them of course.
		static const GPlatesPropertyValues::StructuralType XSI_BOOLEAN_TYPE =
				GPlatesPropertyValues::StructuralType::create_xsi("boolean");
		static const GPlatesPropertyValues::StructuralType XSI_DOUBLE_TYPE =
				GPlatesPropertyValues::StructuralType::create_xsi("double");
		static const GPlatesPropertyValues::StructuralType XSI_INTEGER_TYPE =
				GPlatesPropertyValues::StructuralType::create_xsi("integer");
		static const GPlatesPropertyValues::StructuralType XSI_STRING_TYPE =
				GPlatesPropertyValues::StructuralType::create_xsi("string");
		if (feature_property_type == XSI_BOOLEAN_TYPE ||
			feature_property_type == XSI_DOUBLE_TYPE ||
			feature_property_type == XSI_INTEGER_TYPE ||
			feature_property_type == XSI_STRING_TYPE)
		{
			change_property_checkbox_state = Qt::Unchecked;
		}

		change_property_checkbox->setCheckState(change_property_checkbox_state);
	}
}


// For the BOOST_STATIC_ASSERT below with GCC 4.2.
DISABLE_GCC_WARNING("-Wold-style-cast")

void
GPlatesQtWidgets::ChangePropertyWidget::process(
		GPlatesModel::FeatureHandle::iterator &new_focused_geometry_property)
{
	if (change_property_checkbox->checkState() == Qt::Checked &&
			d_feature_ref.is_valid() &&
			d_property.is_still_valid())
	{
		boost::optional<GPlatesModel::PropertyName> item =
			d_property_destinations_widget->get_property_name();
		if (!item)
		{
			return;
		}

		const GPlatesModel::PropertyName &new_property_name = *item;
		GPlatesModel::ModelUtils::TopLevelPropertyError::Type error_code;

		boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type> new_property =
			GPlatesModel::ModelUtils::rename_property(
					**d_property,
					new_property_name,
					d_gpgim,
					&error_code);
		if (new_property)
		{
			// Successful in converting the property.

			// Change the focused geometry if we're about to delete it.
			// This, of course, only happens to properties that are geometric.
			bool geometric_property_is_focused =
				(d_feature_focus.associated_geometry_property() == d_property);

			d_feature_ref->remove(d_property);
			GPlatesModel::FeatureHandle::iterator new_property_iter =
				d_feature_ref->add(*new_property);

			if (geometric_property_is_focused)
			{
				new_focused_geometry_property = new_property_iter;
			}
		}
		else
		{
			// Not successful in converting the property; show error message.
			const char *error_message = GPlatesModel::ModelUtils::get_error_message(error_code);

			static const char *const ERROR_MESSAGE_APPEND = QT_TR_NOOP("Please modify the property manually.");
			QMessageBox::warning(this, tr("Change Property"),
					tr(error_message) + " " + tr(ERROR_MESSAGE_APPEND));
		}
	}
}

ENABLE_GCC_WARNING("-Wold-style-cast")

