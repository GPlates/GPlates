/* $Id: HellingerErrorOrder.cc 161 2012-01-30 15:42:19Z juraj.cirbus $ */

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
#include "HellingerErrorOrder.h"
#include "HellingerDialogUi.h"
#include "HellingerModel.h"
#include "QtWidgetUtils.h"

GPlatesQtWidgets::HellingerErrorOrder::HellingerErrorOrder(
                HellingerDialog *hellinger_dialog,
                HellingerModel *hellinger_model,
                QWidget *parent_):
    QDialog(parent_,Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
    d_hellinger_dialog_ptr(hellinger_dialog),
    d_hellinger_model_ptr(hellinger_model)



{
	setupUi(this);

        QObject::connect(button_ok, SIGNAL(clicked()), this, SLOT(continue_process()));
        QObject::connect(button_close, SIGNAL(clicked()), this, SLOT(close_application()));
        update_buttons();
        d_process = true;
}


void
GPlatesQtWidgets::HellingerErrorOrder::continue_process()
{
    d_process = true;
    d_hellinger_model_ptr->set_error_order(d_process);
    d_hellinger_dialog_ptr->renumber_segments();
}

void GPlatesQtWidgets::HellingerErrorOrder::close_application()
{
    d_process = false;
    d_hellinger_model_ptr->set_error_order(d_process);
}

void
GPlatesQtWidgets::HellingerErrorOrder::update_buttons()
{
 
}





