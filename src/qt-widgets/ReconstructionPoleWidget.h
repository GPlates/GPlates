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
 
#ifndef GPLATES_QTWIDGETS_RECONSTRUCTIONPOLEWIDGET_H
#define GPLATES_QTWIDGETS_RECONSTRUCTIONPOLEWIDGET_H

#include <QWidget>

#include "model/types.h"
#include "ui_ReconstructionPoleWidgetUi.h"

namespace GPlatesQtWidgets
{

	struct ReconstructionPole
	{
		unsigned long d_moving_plate;
		double d_age;
		double d_latitude;
		double d_longitude;
		double d_angle;
		unsigned long d_fixed_plate;

		ReconstructionPole():
			d_moving_plate(0),
			d_age(0),
			d_latitude(0),
			d_longitude(0),
			d_angle(0),
			d_fixed_plate(0)
		{}

		ReconstructionPole(
			unsigned long moving_plate,
			double age,
			double latitude,
			double longitude,
			double angle,
			unsigned long fixed_plate
			):
			d_moving_plate(moving_plate),
			d_age(age),
			d_latitude(latitude),
			d_longitude(longitude),
			d_angle(angle),
			d_fixed_plate(fixed_plate)
		{  }
	};	

	class ReconstructionPoleWidget:
		public QWidget,
		protected Ui_ReconstructionPoleWidget
	{
		Q_OBJECT
		
	public:
		ReconstructionPoleWidget(
			QWidget *parent_ = NULL);
		
		void
		set_fields(
			const GPlatesModel::integer_plate_id_type &moving_plate_id,
			const double &time,
			const double &latitude,
			const double &longitude,
			const double &angle,
			const GPlatesModel::integer_plate_id_type &fixed_plate_id);
			
		void
		set_fields(
			const ReconstructionPole &reconstruction_pole);
			
		const ReconstructionPole &
		get_reconstruction_pole()
		{
			return d_reconstruction_pole;
		}
		
	private:
		ReconstructionPole d_reconstruction_pole;
		
	};
}


#endif // GPLATES_QTWIDGETS_RECONSTRUCTIONPOLEWIDGET_H  

