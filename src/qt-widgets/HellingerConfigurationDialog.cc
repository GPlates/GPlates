/**
 * \file
 * $Revision$
 * $Date$
 *
 * Copyright (C) 2015 Geological Survey of Norway
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

#include <QSettings>

#include "app-logic/ApplicationState.h"
#include "app-logic/UserPreferences.h"
#include "HellingerConfigurationDialog.h"
#include "HellingerConfigurationWidget.h"


GPlatesQtWidgets::HellingerConfigurationDialog::HellingerConfigurationDialog(
		HellingerDialog::Configuration &configuration,
		GPlatesAppLogic::ApplicationState &app_state,
		QWidget *parent_) :
	QDialog(parent_),
	d_configuration_widget(new HellingerConfigurationWidget()),
	d_configuration_ref(configuration),
	d_app_state_ref(app_state)
{
	setupUi(this);

	QGridLayout *layout_ = new QGridLayout(placeholder_widget);
	layout_->addWidget(d_configuration_widget);

	read_values_from_settings();

	initialise_widget();

	QObject::connect(button_close,SIGNAL(clicked()),this,SLOT(close()));
	QObject::connect(button_apply,SIGNAL(clicked()),this,SLOT(handle_apply()));
	QObject::connect(d_configuration_widget,SIGNAL(configuration_changed(bool)),this,SLOT(handle_configuration_changed(bool)));

}

GPlatesQtWidgets::HellingerConfigurationDialog::~HellingerConfigurationDialog()
{
	write_values_to_settings();
}

void
GPlatesQtWidgets::HellingerConfigurationDialog::handle_apply()
{
	d_configuration_ref.d_best_fit_pole_colour = d_configuration_widget->best_fit_pole_colour();
	d_configuration_ref.d_ellipse_colour = d_configuration_widget->ellipse_colour();
	d_configuration_ref.d_ellipse_line_thickness = d_configuration_widget->ellipse_line_thickness();
	d_configuration_ref.d_initial_estimate_pole_colour = d_configuration_widget->initial_estimate_pole_colour();
	d_configuration_ref.d_pole_arrow_height = d_configuration_widget->pole_arrow_height();
	d_configuration_ref.d_pole_arrow_radius = d_configuration_widget->pole_arrow_radius();
	Q_EMIT configuration_changed();
}

void
GPlatesQtWidgets::HellingerConfigurationDialog::handle_configuration_changed(
		bool valid)
{
	button_apply->setEnabled(valid);
}

void
GPlatesQtWidgets::HellingerConfigurationDialog::read_values_from_settings() const
{
	GPlatesAppLogic::UserPreferences &prefs = d_app_state_ref.get_user_preferences();
	static const HellingerConfigurationWidget::colour_description_map_type
			map = HellingerConfigurationWidget::build_colour_description_map();

	if (prefs.exists("tools/hellinger/ellipse_thickness"))
	{
		d_configuration_ref.d_ellipse_line_thickness =
				prefs.get_value("tools/hellinger/ellipse_thickness").toInt();
	}

	if (prefs.exists("tools/hellinger/best_fit_pole_colour"))
	{
		QString colour_as_string = prefs.get_value("tools/hellinger/best_fit_pole_colour").toString();
		d_configuration_ref.d_best_fit_pole_colour = map.key(colour_as_string);
	}

	if (prefs.exists("tools/hellinger/ellipse_colour"))
	{
		QString colour_as_string = prefs.get_value("tools/hellinger/ellipse_colour").toString();
		d_configuration_ref.d_ellipse_colour = map.key(colour_as_string);
	}

	if (prefs.exists("tools/hellinger/estimate_pole_colour"))
	{
		QString colour_as_string = prefs.get_value("tools/hellinger/estimate_pole_colour").toString();
		d_configuration_ref.d_initial_estimate_pole_colour = map.key(colour_as_string);
	}

	if (prefs.exists("tools/hellinger/pole_arrow_height"))
	{
		d_configuration_ref.d_pole_arrow_height =
				prefs.get_value("tools/hellinger/pole_arrow_height").toFloat();
	}

	if (prefs.exists("tools/hellinger/pole_arrow_radius"))
	{
		d_configuration_ref.d_pole_arrow_radius =
				prefs.get_value("tools/hellinger/pole_arrow_radius").toFloat();
	}
}

void
GPlatesQtWidgets::HellingerConfigurationDialog::write_values_to_settings()
{
	GPlatesAppLogic::UserPreferences &prefs = d_app_state_ref.get_user_preferences();

	static const HellingerConfigurationWidget::colour_description_map_type
			map = HellingerConfigurationWidget::build_colour_description_map();

	prefs.set_value("tools/hellinger/ellipse_thickness",d_configuration_ref.d_ellipse_line_thickness);

	QString best_fit_pole_colour_string = map[d_configuration_ref.d_best_fit_pole_colour];
	prefs.set_value("tools/hellinger/best_fit_pole_colour",best_fit_pole_colour_string);

	QString ellipse_colour_string = map[d_configuration_ref.d_ellipse_colour];
	prefs.set_value("tools/hellinger/ellipse_colour",ellipse_colour_string);

	QString initial_estimate_colour_string = map[d_configuration_ref.d_initial_estimate_pole_colour];
	prefs.set_value("tools/hellinger/estimate_pole_colour",initial_estimate_colour_string);

	prefs.set_value("tools/hellinger/pole_arrow_height",d_configuration_ref.d_pole_arrow_height);

	prefs.set_value("tools/hellinger/pole_arrow_radius",d_configuration_ref.d_pole_arrow_radius);
}

void
GPlatesQtWidgets::HellingerConfigurationDialog::initialise_widget()
{
	d_configuration_widget->set_ellipse_line_thickness(d_configuration_ref.d_ellipse_line_thickness);
	d_configuration_widget->set_best_fit_pole_colour(d_configuration_ref.d_best_fit_pole_colour);
	d_configuration_widget->set_ellipse_colour(d_configuration_ref.d_ellipse_colour);
	d_configuration_widget->set_initial_estimate_pole_colour(d_configuration_ref.d_initial_estimate_pole_colour);
	d_configuration_widget->set_pole_arrow_height(d_configuration_ref.d_pole_arrow_height);
	d_configuration_widget->set_pole_arrow_radius(d_configuration_ref.d_pole_arrow_radius);
}
