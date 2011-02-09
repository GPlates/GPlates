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
 
#include "ConfigureGraticulesDialog.h"

#include "ChooseColourButton.h"
#include "QtWidgetUtils.h"

#include "gui/GraticuleSettings.h"

#include "maths/MathsUtils.h"


GPlatesQtWidgets::ConfigureGraticulesDialog::ConfigureGraticulesDialog(
		QWidget *parent_):
	QDialog(parent_, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::MSWindowsFixedSizeDialogHint),
	d_colour_button(new ChooseColourButton(this))
{
	setupUi(this);
	QtWidgetUtils::add_widget_to_placeholder(
			d_colour_button,
			colour_button_placeholder_widget);
	colour_label->setBuddy(d_colour_button);

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

	QtWidgetUtils::resize_based_on_size_hint(this);
}


int
GPlatesQtWidgets::ConfigureGraticulesDialog::exec(
		GPlatesGui::GraticuleSettings &settings)
{
	populate(settings);
	int dialog_code = QDialog::exec();
	if (dialog_code == QDialog::Accepted)
	{
		save(settings);
	}
	return dialog_code;
}


void
GPlatesQtWidgets::ConfigureGraticulesDialog::populate(
		const GPlatesGui::GraticuleSettings &settings)
{
	latitude_delta_spinbox->setValue(
			GPlatesMaths::convert_rad_to_deg(settings.get_delta_lat()));
	longitude_delta_spinbox->setValue(
			GPlatesMaths::convert_rad_to_deg(settings.get_delta_lon()));
	d_colour_button->set_colour(settings.get_colour());
}


void
GPlatesQtWidgets::ConfigureGraticulesDialog::save(
		GPlatesGui::GraticuleSettings &settings)
{
	settings.set_delta_lat(
			GPlatesMaths::convert_deg_to_rad(latitude_delta_spinbox->value()));
	settings.set_delta_lon(
			GPlatesMaths::convert_deg_to_rad(longitude_delta_spinbox->value()));
	settings.set_colour(d_colour_button->get_colour());
}

