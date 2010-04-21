/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 Geological Survey of Norway
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
 
#include <QLocale>
 
#include "ReconstructionPoleWidget.h"

GPlatesQtWidgets::ReconstructionPoleWidget::ReconstructionPoleWidget(
	QWidget *parent_):
	QWidget(parent_)
{
	setupUi(this);
}

void
GPlatesQtWidgets::ReconstructionPoleWidget::set_fields(
	const GPlatesModel::integer_plate_id_type &moving_plate_id, 
	const double &time, 
	const double &latitude, 
	const double &longitude, 
	const double &angle, 
	const GPlatesModel::integer_plate_id_type &fixed_plate_id)
{
	QLocale locale_;
	lineedit_moving_plate->setText(locale_.toString(static_cast<unsigned>(moving_plate_id)));
	lineedit_time->setText(locale_.toString(time));
	lineedit_latitude->setText(locale_.toString(latitude));
	lineedit_longitude->setText(locale_.toString(longitude));
	lineedit_angle->setText(locale_.toString(angle));
	lineedit_fixed_plate->setText(locale_.toString(static_cast<unsigned>(fixed_plate_id)));
	
	d_reconstruction_pole = ReconstructionPole(moving_plate_id,time,latitude,longitude,angle,fixed_plate_id);
	
	
}

void
GPlatesQtWidgets::ReconstructionPoleWidget::set_fields(
	const ReconstructionPole &reconstruction_pole)
{
	d_reconstruction_pole = reconstruction_pole;
	set_fields(
		reconstruction_pole.d_moving_plate,
		reconstruction_pole.d_age,
		reconstruction_pole.d_latitude,
		reconstruction_pole.d_longitude,
		reconstruction_pole.d_angle,
		reconstruction_pole.d_fixed_plate);
}

