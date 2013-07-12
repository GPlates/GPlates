/* $Id: HellingerNewSegmentError.cc 161 2012-01-30 15:42:19Z juraj.cirbus $ */

/**
 * \file 
 * $Revision: 161 $
 * $Date: 2012-01-30 16:42:19 +0100 (Mon, 30 Jan 2012) $ 
 * 
 * Copyright (C) 2012 Geological Survey of Norway
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
#include <QFileDialog>
#include <QLabel>
#include <QLocale>
#include <QRadioButton>
#include <QTextStream>

#include "HellingerNewSegmentWarning.h"
#include "HellingerNewSegmentWarningUi.h"
#include "QtWidgetUtils.h"

GPlatesQtWidgets::HellingerNewSegmentWarning::HellingerNewSegmentWarning(
                HellingerDialog *hellinger_dialog,
				const int &segment_number,
                QWidget *parent_):
    QDialog(parent_,Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
    d_hellinger_dialog_ptr(hellinger_dialog),
	d_type_error_new_segment(0),
	d_segment_number(segment_number)
{
	setupUi(this);

	QString warning_text = QObject::tr("There already exists a segment with number %1").arg(d_segment_number);
	label_warning_text->setText(warning_text);

	d_button_group.addButton(radio_add);
	d_button_group.addButton(radio_replace);
	d_button_group.addButton(radio_insert);


	QObject::connect(button_ok, SIGNAL(clicked()), this, SLOT(handle_ok()));
	QObject::connect(button_cancel, SIGNAL(clicked()), this, SLOT(reject()));
	QObject::connect(&d_button_group, SIGNAL(buttonClicked(QAbstractButton*)),this,SLOT(handle_button_clicked()));

	button_ok->setEnabled(false);
}


void
GPlatesQtWidgets::HellingerNewSegmentWarning::handle_ok()
{
	if (radio_add->isChecked())
    {
		d_type_error_new_segment = ACTION_ADD_NEW_SEGMENT;
    }
	else if (radio_replace->isChecked())
    {
		d_type_error_new_segment = ACTION_REPLACE_NEW_SEGMENT;
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

void GPlatesQtWidgets::HellingerNewSegmentWarning::initialise_buttons()
{
	radio_add->setChecked(false);
	radio_insert->setChecked(false);
	radio_replace->setChecked(false);
	button_ok->setEnabled(false);
}

void
GPlatesQtWidgets::HellingerNewSegmentWarning::handle_button_clicked()
{
	button_ok->setEnabled(true);
}




