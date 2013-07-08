/* $Id: HellingerEditPoint.cc 255 2012-03-01 13:19:42Z robin.watson@ngu.no $ */

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

#include <fstream>
#include <iostream>
#include <list>
#include <string>

#include <QDebug>
#include "QFileDialog"
#include <QLocale>
#include <QRadioButton>
#include <QTextStream>

#include "global/CompilerWarnings.h"
#include "HellingerDialog.h"
#include "HellingerEditPoint.h"
#include "HellingerDialogUi.h"
#include "QtWidgetUtils.h"

GPlatesQtWidgets::HellingerEditPoint::HellingerEditPoint(
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

        QObject::connect(button_edit_point, SIGNAL(clicked()), this, SLOT(edit_point()));
        update_buttons();
}

void
GPlatesQtWidgets::HellingerEditPoint::initialization_table(QStringList &input_value)
{
    spinbox_segment -> setValue(input_value.at(0).toInt());
    if (input_value.at(1).toInt() == 1)
    {        
        radiobtn_move -> setChecked(true);
    }
    else if (input_value.at(1).toInt() == 2)
    {        
        radiobtn_fixed -> setChecked(true);
    }
    spinbox_lat -> setValue(input_value.at(2).toDouble());
    spinbox_long -> setValue(input_value.at(3).toDouble());
    spinbox_uncert -> setValue(input_value.at(4).toDouble());
}

void
GPlatesQtWidgets::HellingerEditPoint::initialization(int &segment, int &row)
{
    QStringList get_data_line = d_hellinger_model_ptr->get_line(segment, row);
    initialization_table(get_data_line);
    d_segment = segment;
    d_row = row;

}

void
GPlatesQtWidgets::HellingerEditPoint::edit_point()
{
    QStringList edit_point_model;
    int segment = spinbox_segment -> value();
    int move_fixed;
    if (radiobtn_move -> isChecked())
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

void
GPlatesQtWidgets::HellingerEditPoint::update_buttons()
{
 
}





