/* $Id: HellingerEditPointDialog.cc 255 2012-03-01 13:19:42Z robin.watson@ngu.no $ */

/**
 * \file
 * $Revision: 255 $
 * $Date: 2012-03-01 14:19:42 +0100 (Thu, 01 Mar 2012) $
 *
 * Copyright (C) 2011, 2012 Geological Survey of Norway
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

#include <QDebug>

#include <QRadioButton>
#include <QTextStream>


#include "HellingerDialog.h"
#include "HellingerEditPointDialog.h"

#include "QtWidgetUtils.h"

GPlatesQtWidgets::HellingerEditPointDialog::HellingerEditPointDialog(
		HellingerDialog *hellinger_dialog,
		HellingerModel *hellinger_model,
		QWidget *parent_):
	QDialog(parent_,Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	d_hellinger_dialog_ptr(hellinger_dialog),
	d_hellinger_model_ptr(hellinger_model),
	d_segment(0),
	d_row(0)

{
	setupUi(this);

	QObject::connect(button_apply, SIGNAL(clicked()), this, SLOT(handle_apply()));
	QObject::connect(button_cancel,SIGNAL(clicked()),this,SLOT(reject()));
}


void
GPlatesQtWidgets::HellingerEditPointDialog::initialise_with_pick(
		const int &segment, const int &row)
{
	boost::optional<HellingerPick> pick = d_hellinger_model_ptr->get_pick(segment,row);

	if(pick)
	{
		d_segment = segment;
		d_row = row;

		spinbox_segment->setValue(segment);
		if (pick->d_segment_type == MOVING_PICK_TYPE)
		{
			radio_moving->setChecked(true);
		}
		else
		{
			radio_fixed->setChecked(true);
		}

		spinbox_lat->setValue(pick->d_lat);
		spinbox_long->setValue(pick->d_lon);
		spinbox_uncert->setValue(pick->d_uncertainty);
	}

}

void
GPlatesQtWidgets::HellingerEditPointDialog::edit_point()
{
	QStringList edit_point_model;
	int segment = spinbox_segment -> value();
	int move_fixed;
	if (radio_moving -> isChecked())
	{
		move_fixed = 1;
	}
	else
	{
		move_fixed = 2;
	}
	double lat = spinbox_lat -> value();
	double lon = spinbox_long -> value();
	double unc = spinbox_uncert -> value();
	QString segment_str = QString("%1").arg(segment);
	QString move_fixed_str = QString("%1").arg(move_fixed);
	QString lat_str = QString("%1").arg(lat);
	QString lon_str = QString("%1").arg(lon);
	QString unc_str = QString("%1").arg(unc);
	QString is_enabled = "1";
	edit_point_model << move_fixed_str << segment_str << lat_str << lon_str << unc_str<<is_enabled;
	d_hellinger_model_ptr ->remove_pick(d_segment,d_row);
	d_hellinger_model_ptr ->add_pick(edit_point_model);
	d_hellinger_dialog_ptr ->update();
}






