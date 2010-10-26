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

#include "ChangeGeometryPropertyWidget.h"

#include "ChooseGeometryPropertyWidget.h"
#include "QtWidgetUtils.h"

#include "gui/FeatureFocus.h"

#include "model/GPGIMInfo.h"
#include "model/ModelUtils.h"


GPlatesQtWidgets::ChangeGeometryPropertyWidget::ChangeGeometryPropertyWidget(
		const GPlatesGui::FeatureFocus &feature_focus,
		QWidget *parent_) :
	QWidget(parent_),
	d_feature_focus(feature_focus),
	d_geometry_destinations_widget(
			new ChooseGeometryPropertyWidget(
				SelectionWidget::Q_COMBO_BOX,
				this))
{
	setupUi(this);

	// Save the checkbox's text that was set in the Designer.
	d_default_explanatory_text = change_property_checkbox->text();

	QtWidgetUtils::add_widget_to_placeholder(
			d_geometry_destinations_widget, geometry_destinations_placeholder_widget);
	geometry_destinations_placeholder_widget->setMinimumSize(
			d_geometry_destinations_widget->sizeHint());

	// Checkbox signals.
	QObject::connect(
			change_property_checkbox,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(handle_checkbox_state_changed(int)));
}


void
GPlatesQtWidgets::ChangeGeometryPropertyWidget::handle_checkbox_state_changed(
		int state)
{
	d_geometry_destinations_widget->setEnabled(state == Qt::Checked);
}


void
GPlatesQtWidgets::ChangeGeometryPropertyWidget::populate(
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
		const GPlatesModel::FeatureHandle::iterator &geometric_property,
		const GPlatesModel::FeatureType &new_feature_type)
{
	d_feature_ref = feature_ref;
	d_geometric_property = geometric_property;

	if (d_feature_ref.is_valid() && geometric_property.is_still_valid())
	{
		// Set up the combobox.
		d_geometry_destinations_widget->populate(new_feature_type);
		change_property_checkbox->setCheckState(Qt::Checked);

		// Display some explanatory text.
		change_property_checkbox->setText(
				d_default_explanatory_text.arg(
					GPlatesModel::GPGIMInfo::get_geometric_property_name(
						(*geometric_property)->property_name()).toLower()));
	}
}


void
GPlatesQtWidgets::ChangeGeometryPropertyWidget::process(
		GPlatesModel::FeatureHandle::iterator &new_focused_geometry_property)
{
	if (change_property_checkbox->checkState() == Qt::Checked &&
			d_feature_ref.is_valid() &&
			d_geometric_property.is_still_valid())
	{
		boost::optional<GPlatesModel::PropertyName> item =
			d_geometry_destinations_widget->get_property_name();
		if (!item)
		{
			return;
		}

		const GPlatesModel::PropertyName &new_property_name = *item;
		GPlatesModel::ModelUtils::RenameGeometricPropertyError::Type error_code;

		boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type> new_property =
			GPlatesModel::ModelUtils::rename_geometric_property(
					**d_geometric_property,
					new_property_name,
					&error_code);
		if (new_property)
		{
			// Successful in converting the property.

			// Change the focused geometry if we're about to delete it.
			bool geometric_property_is_focused =
				(d_feature_focus.associated_geometry_property() == d_geometric_property);

			d_feature_ref->remove(d_geometric_property);
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
			static const char *ERROR_MESSAGES[] = {
				QT_TR_NOOP("GPlates cannot change the property name of a top-level property that does not have exactly one property value."),
				QT_TR_NOOP("The property could not be identified as a geometry."),
				QT_TR_NOOP("GPlates was unable to unwrap the time-dependent property."),
				QT_TR_NOOP("GPlates cannot change the property name of a top-level property that is not inline.")
			};
			BOOST_STATIC_ASSERT(sizeof(ERROR_MESSAGES) / sizeof(const char *) ==
					GPlatesModel::ModelUtils::RenameGeometricPropertyError::NUM_ERRORS);

			static const char *ERROR_MESSAGE_APPEND = QT_TR_NOOP("Please modify the geometry manually.");
			const char *error_message = ERROR_MESSAGES[static_cast<unsigned int>(error_code)];
			QMessageBox::warning(this, tr("Change Geometry Property"),
					tr(error_message) + " " + tr(ERROR_MESSAGE_APPEND));
		}
	}
}

