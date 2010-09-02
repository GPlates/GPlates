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

#include "ChangeGeometryPropertyDialog.h"

#include "GeometryDestinationsListWidget.h"
#include "QtWidgetUtils.h"


GPlatesQtWidgets::ChangeGeometryPropertyDialog::ChangeGeometryPropertyDialog(
		QWidget *parent_):
	QDialog(parent_, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	d_geometry_destinations_listwidget(new GeometryDestinationsListWidget(this))
{
	setupUi(this);

	QtWidgetUtils::add_widget_to_placeholder(
			d_geometry_destinations_listwidget,
			geometry_destinations_listwidget_placeholder);

	// ButtonBox signals.
	QObject::connect(
			main_buttonbox,
			SIGNAL(accepted()),
			this,
			SLOT(accept()));
	QObject::connect(
			main_buttonbox,
			SIGNAL(rejected()),
			this,
			SLOT(reject()));

	// Checkbox signals.
	QObject::connect(
			change_property_checkbox,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(handle_checkbox_state_changed(int)));
}


GPlatesQtWidgets::PropertyNameItem *
GPlatesQtWidgets::ChangeGeometryPropertyDialog::get_property_name_item() const
{
	if (change_property_checkbox->checkState() == Qt::Checked)
	{
		return d_geometry_destinations_listwidget->get_current_property_name_item();
	}
	else
	{
		return NULL;
	}
}


void
GPlatesQtWidgets::ChangeGeometryPropertyDialog::populate(
		const GPlatesModel::FeatureType &feature_type,
		const GPlatesModel::PropertyName &old_property_name)
{
	d_geometry_destinations_listwidget->populate(feature_type);

	static const QString EXPLANATORY_TEXT = "The feature has a %1 geometry property, "
		"which is not a valid geometry property for the new feature type, %2.";
	explanatory_label->setText(EXPLANATORY_TEXT.arg(
				GPlatesUtils::make_qstring_from_icu_string(old_property_name.build_aliased_name())).arg(
				GPlatesUtils::make_qstring_from_icu_string(feature_type.build_aliased_name())));
}


void
GPlatesQtWidgets::ChangeGeometryPropertyDialog::handle_checkbox_state_changed(
		int state)
{
	d_geometry_destinations_listwidget->setEnabled(state == Qt::Checked);
}

