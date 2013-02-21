/* $Id: HellingerNewPoint.cc 241 2012-02-28 11:28:13Z robin.watson@ngu.no $ */

/**
 * \file 
 * $Revision: 241 $
 * $Date: 2012-02-28 12:28:13 +0100 (Tue, 28 Feb 2012) $ 
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
#include <list>
#include <iostream>
#include <string>

#include <QDebug>
#include <QFileDialog>
#include <QLocale>
#include <QRadioButton>
#include <QTextStream>

#include <boost/python.hpp>

#include "api/PythonInterpreterLocker.h"
#include "global/CompilerWarnings.h"
#include "HellingerDialog.h"
#include "HellingerDialogUi.h"
#include "HellingerModel.h"
#include "HellingerNewPoint.h"
#include "QtWidgetUtils.h"

GPlatesQtWidgets::HellingerNewPoint::HellingerNewPoint(
                HellingerDialog *hellinger_dialog,
                HellingerModel *hellinger_model,
        QWidget *parent_):
	QDialog(parent_,Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
        d_hellinger_dialog_ptr(hellinger_dialog),
        d_hellinger_model_ptr(hellinger_model)

{
	setupUi(this);
        QObject::connect(button_add_point, SIGNAL(clicked()), this, SLOT(add_point()));
        update_buttons();
}

void
GPlatesQtWidgets::HellingerNewPoint::add_point()
{
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
    QStringList new_point;
    double lat = spinbox_lat -> value();
    double lon = spinbox_long -> value();
    double unc = spinbox_uncert -> value();

    QString segment_str = QString("%1").arg(segment);
    QString move_fixed_str = QString("%1").arg(move_fixed);
    QString lat_str = QString("%1").arg(lat);
    QString lon_str = QString("%1").arg(lon);
    QString unc_str = QString("%1").arg(unc);
    QString is_enabled = "1";
    new_point << move_fixed_str << segment_str << lat_str << lon_str << unc_str<<is_enabled;

    d_hellinger_model_ptr ->add_pick(new_point);
    d_hellinger_dialog_ptr->update();
    
}

void
GPlatesQtWidgets::HellingerNewPoint::update_buttons()
{
    
}





