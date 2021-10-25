/* $Id: HellingerNewSegmentError.cc 161 2012-01-30 15:42:19Z juraj.cirbus $ */

/**
 * \file
 * $Revision: 161 $
 * $Date: 2012-01-30 16:42:19 +0100 (Mon, 30 Jan 2012) $
 *
 * Copyright (C) 2012, 2013 Geological Survey of Norway
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

#include <QButtonGroup>
#include <QDebug>
#include <QLabel>
#include <QRadioButton>

#include "HellingerNewSegmentWarning.h"
#include "ui_HellingerNewSegmentWarningUi.h"
#include "QtWidgetUtils.h"

GPlatesQtWidgets::HellingerNewSegmentWarning::HellingerNewSegmentWarning(QWidget *parent_):
	QDialog(parent_,Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	d_type_error_new_segment(0)
{
	setupUi(this);


	d_radio_button_group.addButton(radio_add);
	d_radio_button_group.addButton(radio_replace);
	d_radio_button_group.addButton(radio_insert);

	initialise(0);

	QObject::connect(button_ok, SIGNAL(clicked()), this, SLOT(handle_ok()));
	QObject::connect(button_cancel, SIGNAL(clicked()), this, SLOT(handle_cancel()));
	QObject::connect(&d_radio_button_group, SIGNAL(buttonClicked(QAbstractButton*)),this,SLOT(handle_radio_button_clicked()));

}


void
GPlatesQtWidgets::HellingerNewSegmentWarning::handle_ok()
{
	if (radio_add->isChecked())
	{
		d_type_error_new_segment = ACTION_ADD_TO_EXISTING_SEGMENT;
	}
	else if (radio_replace->isChecked())
	{
		d_type_error_new_segment = ACTION_REPLACE_SEGMENT;
	}
	else if (radio_insert->isChecked())
	{
		d_type_error_new_segment = ACTION_INSERT_NEW_SEGMENT;
	}
}

int
GPlatesQtWidgets::HellingerNewSegmentWarning::error_type_new_segment()
{
	return d_type_error_new_segment;
}

void
GPlatesQtWidgets::HellingerNewSegmentWarning::initialise(
		int segment_number)
{
	radio_add->setChecked(false);
	radio_insert->setChecked(true);
	radio_replace->setChecked(false);


	QString warning_text = QObject::tr("There already exists a segment with number %1.").arg(segment_number);
	label_warning_text->setText(warning_text);

	QString add_string = QObject::tr("Add picks to segment %1").arg(segment_number);
	QString replace_string = QObject::tr("Replace segment %1").arg(segment_number);
	QString insert_string =
			QObject::tr("Insert segment as segment %1, renumbering the \nfollowing segments from %2").
			arg(segment_number).
			arg(segment_number+1);
	radio_add->setText(add_string);
	radio_replace->setText(replace_string);
	radio_insert->setText(insert_string);

}

void
GPlatesQtWidgets::HellingerNewSegmentWarning::handle_radio_button_clicked()
{
	button_ok->setEnabled(true);
}

void
GPlatesQtWidgets::HellingerNewSegmentWarning::handle_cancel()
{
	d_type_error_new_segment = ACTION_CANCEL;
}



