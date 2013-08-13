/* $Id: HellingerEditPointDialog.cc 255 2012-03-01 13:19:42Z robin.watson@ngu.no $ */

/**
 * \file
 * $Revision: 255 $
 * $Date: 2012-03-01 14:19:42 +0100 (Thu, 01 Mar 2012) $
 *
 * Copyright (C) 2011, 2012, 2013 Geological Survey of Norway
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

GPlatesQtWidgets::HellingerEditPointDialog::HellingerEditPointDialog(HellingerDialog *hellinger_dialog,
		HellingerModel *hellinger_model,
		bool create_new_point,
		QWidget *parent_):
	QDialog(parent_,Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	d_hellinger_dialog_ptr(hellinger_dialog),
	d_hellinger_model_ptr(hellinger_model),
	d_segment(0),
	d_row(0),
	d_create_new_point(create_new_point)
{
	setupUi(this);

	set_initial_values();

	QObject::connect(button_apply, SIGNAL(clicked()), this, SLOT(handle_apply()));
	QObject::connect(button_cancel,SIGNAL(clicked()),this,SLOT(reject()));

	if (d_create_new_point)
	{
		button_apply->setText(QObject::tr("&Add point"));
	}
}


void
GPlatesQtWidgets::HellingerEditPointDialog::initialise_with_pick(
		const int &segment, const int &row)
{
	boost::optional<HellingerPick> pick = d_hellinger_model_ptr->get_pick(segment,row);

	if(pick)
	{
		// We need to store these so that we can delete the correct pick
		// before adding the new (edited) one.
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
		spinbox_lon->setValue(pick->d_lon);
		spinbox_uncert->setValue(pick->d_uncertainty);
	}

}

void GPlatesQtWidgets::HellingerEditPointDialog::initialise_with_segment_number(
		const int &segment_number)
{
	spinbox_segment->setValue(segment_number);
}

void
GPlatesQtWidgets::HellingerEditPointDialog::handle_apply()
{
	d_hellinger_dialog_ptr->store_expanded_status();
	int segment_number = spinbox_segment->value();
	HellingerPickType type;
	if (radio_moving->isChecked())
	{
		type = MOVING_PICK_TYPE;
	}
	else
	{
		type = FIXED_PICK_TYPE;
	}

	double lat = spinbox_lat->value();
	double lon = spinbox_lon->value();
	double uncertainty = spinbox_uncert->value();

	if (!d_create_new_point)
	{
		d_hellinger_model_ptr->remove_pick(d_segment,d_row);
	}
	d_hellinger_model_ptr->add_pick(
				HellingerPick(type,
							  lat,
							  lon,
							  uncertainty,
							  true),
				segment_number);

	d_hellinger_dialog_ptr->update_tree_from_model();
	d_hellinger_dialog_ptr->restore_expanded_status();
	d_hellinger_dialog_ptr->expand_segment(segment_number);
}

void GPlatesQtWidgets::HellingerEditPointDialog::set_initial_values()
{
	spinbox_segment->setValue(1);
	spinbox_lat->setValue(0.);
	spinbox_lon->setValue(0.);
	spinbox_uncert->setValue(0.);
}






