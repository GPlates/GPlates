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

#include "maths/MathsUtils.h"
#include "VelocityMethodWidget.h"


GPlatesQtWidgets::VelocityMethodWidget::VelocityMethodWidget(
		bool show_threshold_spinboxes,
		QWidget *parent_) :
		QWidget(parent_),
		d_show_threshold_spinboxes(show_threshold_spinboxes)
{
	setupUi(this);

	d_spin_box_palette = spinbox_dt->palette();

	// These id values determine which values are exported to preferences.
	button_group_velocity_method->setId(radio_t_to_t_minus_dt,T_TO_T_MINUS_DT);
	button_group_velocity_method->setId(radio_t_plus_dt_to_t,T_PLUS_DT_TO_T);
	button_group_velocity_method->setId(radio_t_plus_dt_to_t_minus_dt,T_PLUS_MINUS_HALF_DT);

	if (!d_show_threshold_spinboxes)
	{
		spinbox_red->hide();
		spinbox_yellow->hide();
		label_red->hide();
		label_yellow->hide();
	}

	QObject::connect(radio_t_to_t_minus_dt,SIGNAL(clicked()),this,SLOT(handle_velocity_method_changed()));
	QObject::connect(radio_t_plus_dt_to_t,SIGNAL(clicked()),this,SLOT(handle_velocity_method_changed()));
	QObject::connect(radio_t_plus_dt_to_t_minus_dt,SIGNAL(clicked()),this,SLOT(handle_velocity_method_changed()));
	QObject::connect(spinbox_dt,SIGNAL(valueChanged(double)),this,SLOT(handle_delta_time_changed()));
	QObject::connect(spinbox_yellow,SIGNAL(valueChanged(double)),this,SLOT(handle_velocity_yellow_changed()));
	QObject::connect(spinbox_red,SIGNAL(valueChanged(double)),this,SLOT(handle_velocity_red_changed()));
}

GPlatesQtWidgets::VelocityMethodWidget::~VelocityMethodWidget()
{
}

GPlatesQtWidgets::VelocityMethodWidget::VelocityMethod
GPlatesQtWidgets::VelocityMethodWidget::velocity_method()
{
	return d_velocity_method;
}

void
GPlatesQtWidgets::VelocityMethodWidget::handle_velocity_method_changed()
{
	if (radio_t_to_t_minus_dt->isChecked())
	{
		d_velocity_method = T_TO_T_MINUS_DT;
	}
	else if (radio_t_plus_dt_to_t->isChecked())
	{
		d_velocity_method = T_PLUS_DT_TO_T;
	}
	else
	{
		d_velocity_method = T_PLUS_MINUS_HALF_DT;
	}
	Q_EMIT configuration_changed(true);
}

void
GPlatesQtWidgets::VelocityMethodWidget::handle_delta_time_changed()
{
	if (GPlatesMaths::are_almost_exactly_equal(spinbox_dt->value(),0.))
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
GPlatesQtWidgets::VelocityMethodWidget::handle_velocity_yellow_changed()
{
	Q_EMIT configuration_changed(true);
}

void
GPlatesQtWidgets::VelocityMethodWidget::handle_velocity_red_changed()
{
	Q_EMIT configuration_changed(true);
}
