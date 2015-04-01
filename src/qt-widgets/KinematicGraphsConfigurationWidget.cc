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

#include "KinematicGraphsDialog.h"
#include "KinematicGraphsConfigurationWidget.h"


GPlatesQtWidgets::KinematicGraphsConfigurationWidget::KinematicGraphsConfigurationWidget(
		QWidget *parent_) :
		QWidget(parent_)
{
	setupUi(this);

	d_spin_box_palette = spinbox_dt->palette();

	// These id values determine which values are exported to preferences.
	button_group_velocity_method->setId(radio_t_to_t_minus_dt,KinematicGraphsDialog::T_TO_T_MINUS_DT);
	button_group_velocity_method->setId(radio_t_plus_dt_to_t,KinematicGraphsDialog::T_PLUS_DT_TO_T);
	button_group_velocity_method->setId(radio_t_plus_dt_to_t_minus_dt,KinematicGraphsDialog::T_PLUS_MINUS_HALF_DT);

	QObject::connect(radio_t_to_t_minus_dt,SIGNAL(clicked()),this,SLOT(handle_velocity_method_changed()));
	QObject::connect(radio_t_plus_dt_to_t,SIGNAL(clicked()),this,SLOT(handle_velocity_method_changed()));
	QObject::connect(radio_t_plus_dt_to_t_minus_dt,SIGNAL(clicked()),this,SLOT(handle_velocity_method_changed()));
	QObject::connect(spinbox_dt,SIGNAL(valueChanged(double)),this,SLOT(handle_delta_time_changed()));
	QObject::connect(spinbox_yellow,SIGNAL(valueChanged(double)),this,SLOT(handle_velocity_yellow_changed()));
	QObject::connect(spinbox_red,SIGNAL(valueChanged(double)),this,SLOT(handle_velocity_red_changed()));
}

GPlatesQtWidgets::KinematicGraphsConfigurationWidget::~KinematicGraphsConfigurationWidget()
{
}

GPlatesQtWidgets::KinematicGraphsDialog::VelocityMethod
GPlatesQtWidgets::KinematicGraphsConfigurationWidget::velocity_method()
{
	return d_velocity_method;
}

void
GPlatesQtWidgets::KinematicGraphsConfigurationWidget::handle_velocity_method_changed()
{
	if (radio_t_to_t_minus_dt->isChecked())
	{
		d_velocity_method = KinematicGraphsDialog::T_TO_T_MINUS_DT;
	}
	else if (radio_t_plus_dt_to_t->isChecked())
	{
		d_velocity_method = KinematicGraphsDialog::T_PLUS_DT_TO_T;
	}
	else
	{
		d_velocity_method = KinematicGraphsDialog::T_PLUS_MINUS_HALF_DT;
	}
	Q_EMIT configuration_changed(true);
}

void
GPlatesQtWidgets::KinematicGraphsConfigurationWidget::handle_delta_time_changed()
{
	if (GPlatesMaths::Real(spinbox_dt->value()) == 0.0)
	{
		// Apply a red background colour to the spinbox.
		static QPalette red_palette;
		red_palette.setColor(QPalette::Active, QPalette::Base, Qt::red);
		spinbox_dt->setPalette(red_palette);
		Q_EMIT configuration_changed(false);
	}
	else
	{
		// Apply default background colour to the spinbox.
		spinbox_dt->setPalette(d_spin_box_palette);
		Q_EMIT configuration_changed(true);
	}


}

void
GPlatesQtWidgets::KinematicGraphsConfigurationWidget::handle_velocity_yellow_changed()
{
	Q_EMIT configuration_changed(true);
}

void
GPlatesQtWidgets::KinematicGraphsConfigurationWidget::handle_velocity_red_changed()
{
	Q_EMIT configuration_changed(true);
}
