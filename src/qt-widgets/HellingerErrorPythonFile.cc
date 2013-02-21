/* $Id: HellingerErrorPythonFile.cc 161 2012-01-30 15:42:19Z juraj.cirbus $ */

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
#include "HellingerErrorPythonFile.h"
#include "HellingerThread.h"
#include "QtWidgetUtils.h"

GPlatesQtWidgets::HellingerErrorPythonFile::HellingerErrorPythonFile(

                QWidget *parent_):
    QDialog(parent_,Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint)


{
        setupUi(this);
        QObject::connect(button_close, SIGNAL(clicked()), this, SLOT(close_application()));
}

void
GPlatesQtWidgets::HellingerErrorPythonFile::close_application()
{

}

void
GPlatesQtWidgets::HellingerErrorPythonFile::update_buttons()
{
 
}





