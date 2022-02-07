/**
 * \file
 * $Revision: 12148 $
 * $Date: 2011-08-18 14:01:47 +0200 (Thu, 18 Aug 2011) $
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

#include "KinematicGraphsConfigurationWidget.h"

#include "KinematicGraphsConfigurationDialog.h"


GPlatesQtWidgets::KinematicGraphsConfigurationDialog::KinematicGraphsConfigurationDialog(
		KinematicGraphsDialog::Configuration &configuration,
		QWidget *parent_) :
		QDialog(parent_),
		d_configuration_widget(new KinematicGraphsConfigurationWidget()),
		d_configuration_ref(configuration)
{
	setupUi(this);

	QGridLayout *layout_ = new QGridLayout(placeholder_widget);
	layout_->addWidget(d_configuration_widget);

	initialise_widget();

	QObject::connect(button_close,SIGNAL(clicked()),this,SLOT(close()));
	QObject::connect(button_apply,SIGNAL(clicked()),this,SLOT(handle_apply()));
	QObject::connect(d_configuration_widget,SIGNAL(configuration_changed(bool)),this,SLOT(handle_configuration_changed(bool)));
}

GPlatesQtWidgets::KinematicGraphsConfigurationDialog::~KinematicGraphsConfigurationDialog()
{
}

void
GPlatesQtWidgets::KinematicGraphsConfigurationDialog::handle_apply()
{
	d_configuration_ref.d_delta_t = d_configuration_widget->delta_time();
	d_configuration_ref.d_yellow_threshold = d_configuration_widget->yellow_velocity_threshold();
	d_configuration_ref.d_red_threshold = d_configuration_widget->red_velocity_threshold();
	d_configuration_ref.d_velocity_method = d_configuration_widget->velocity_method();
}

void
GPlatesQtWidgets::KinematicGraphsConfigurationDialog::handle_configuration_changed(
		bool valid)
{
	button_apply->setEnabled(valid);
}

void
GPlatesQtWidgets::KinematicGraphsConfigurationDialog::initialise_widget()
{	
	d_configuration_widget->set_delta_time(d_configuration_ref.d_delta_t);
	d_configuration_widget->set_yellow_velocity_threshold(d_configuration_ref.d_yellow_threshold);
	d_configuration_widget->set_red_velocity_threshold(d_configuration_ref.d_red_threshold);
	d_configuration_widget->set_velocity_method(d_configuration_ref.d_velocity_method);
}
