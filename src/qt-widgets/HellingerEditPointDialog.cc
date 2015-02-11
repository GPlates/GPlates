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
#include <QObject>

#include <QRadioButton>
#include <QTextStream>


#include "HellingerDialog.h"
#include "HellingerEditPointDialog.h"

#include "QtWidgetUtils.h"

const double INITIAL_UNCERTAINTY = 5.;

GPlatesQtWidgets::HellingerEditPointDialog::HellingerEditPointDialog(HellingerDialog *hellinger_dialog,
		HellingerModel *hellinger_model,
		bool create_new_pick,
		QWidget *parent_):
	QDialog(parent_,Qt::CustomizeWindowHint |
			Qt::WindowTitleHint |
			Qt::WindowSystemMenuHint |
			Qt::WindowStaysOnTopHint),
	d_hellinger_dialog_ptr(hellinger_dialog),
	d_hellinger_model_ptr(hellinger_model),
	d_segment(0),
	d_row(0),
	d_create_new_pick(create_new_pick)
{
	setupUi(this);

	set_initial_values();

	QObject::connect(button_apply, SIGNAL(clicked()), this, SLOT(handle_apply()));
	QObject::connect(button_cancel,SIGNAL(clicked()),this,SLOT(close()));
	QObject::connect(spinbox_lat,SIGNAL(valueChanged(double)),this,SLOT(handle_pick_changed()));
	QObject::connect(spinbox_lon,SIGNAL(valueChanged(double)),this,SLOT(handle_pick_changed()));
	QObject::connect(radio_moving,SIGNAL(toggled(bool)),this,SLOT(handle_pick_changed()));


	QString description;
	if (d_create_new_pick)
	{
		button_apply->setText(QObject::tr("&Add pick"));
		setWindowTitle(QObject::tr("Create New Pick"));
		description.append("Click on the canvas to select coordinates of a new pick.\n");
		description.append("Shift-click to use coordinates of an existing point feature.\n");
	}
	else {
		button_apply->setText(QObject::tr("&Apply"));
		setWindowTitle(QObject::tr("Edit Pick"));
		description.append("Click and drag the highlighted pick on the canvas.\n");
	}

	label_description->setText(description);
}


void
GPlatesQtWidgets::HellingerEditPointDialog::update_pick_from_model(
		const int &segment, const int &row)
{
	hellinger_model_type::const_iterator it = d_hellinger_model_ptr->get_pick(segment,row);

	if(it != d_hellinger_model_ptr->end())
	{
		// We need to store these so that we can delete the correct pick
		// before adding the new (edited) one.
		d_segment = segment;
		d_row = row;

		HellingerPick pick = it->second;

		spinbox_segment->setValue(segment);
		if (pick.d_segment_type == MOVING_PICK_TYPE)
		{
			radio_moving->setChecked(true);
		}
		else
		{
			radio_fixed->setChecked(true);
		}

		spinbox_lat->setValue(pick.d_lat);
		spinbox_lon->setValue(pick.d_lon);
		spinbox_uncert->setValue(pick.d_uncertainty);
	}

}

void GPlatesQtWidgets::HellingerEditPointDialog::update_segment_number(
		const int &segment_number)
{
	spinbox_segment->setValue(segment_number);
}

void GPlatesQtWidgets::HellingerEditPointDialog::update_pick_coords(const GPlatesMaths::LatLonPoint &llp)
{
	spinbox_lat->setValue(llp.latitude());
	spinbox_lon->setValue(llp.longitude());
	update_pick_from_widgets();
	Q_EMIT update_editing();
}

void GPlatesQtWidgets::HellingerEditPointDialog::set_active(bool active)
{
	button_apply->setEnabled(active);
	spinbox_segment->setEnabled(active);
	spinbox_lat->setEnabled(active);
	spinbox_lon->setEnabled(active);
	spinbox_uncert->setEnabled(active);
	radio_moving->setEnabled(active);
	radio_fixed->setEnabled(active);
	label_segment->setEnabled(active);
}

const GPlatesQtWidgets::HellingerPick &
GPlatesQtWidgets::HellingerEditPointDialog::current_pick() const
{
	return d_pick;
}

void
GPlatesQtWidgets::HellingerEditPointDialog::handle_apply()
{
	d_hellinger_dialog_ptr->store_expanded_status();

	int segment_number = spinbox_segment->value();
	update_pick_from_widgets();

	if (!d_create_new_pick)
	{
		d_hellinger_model_ptr->remove_pick(d_segment,d_row);
	}

	hellinger_model_type::const_iterator it =
		d_hellinger_model_ptr->add_pick(
				d_pick,
				segment_number);

	d_hellinger_dialog_ptr->update_after_new_pick(it,segment_number);

	if (!d_create_new_pick)
	{
		close();
	}
}

void GPlatesQtWidgets::HellingerEditPointDialog::handle_pick_changed()
{
	update_pick_from_widgets();
	Q_EMIT update_editing();
}

void GPlatesQtWidgets::HellingerEditPointDialog::update_pick_from_widgets()
{
	d_pick.d_is_enabled = true;
	d_pick.d_lat = spinbox_lat->value();
	d_pick.d_lon = spinbox_lon->value();
	d_pick.d_uncertainty = spinbox_uncert->value();
	d_pick.d_segment_type = radio_moving->isChecked() ? MOVING_PICK_TYPE : FIXED_PICK_TYPE;
}

void GPlatesQtWidgets::HellingerEditPointDialog::close()
{
	Q_EMIT finished_editing();
	reject();
}


void GPlatesQtWidgets::HellingerEditPointDialog::set_initial_values()
{
	spinbox_segment->setValue(1);
	spinbox_lat->setValue(0.);
	spinbox_lon->setValue(0.);
	spinbox_uncert->setValue(INITIAL_UNCERTAINTY);

	update_pick_from_widgets();
}






