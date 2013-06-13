/* $Id: CalculateReconstructionPoleDialog.cc 10957 2011-02-09 07:53:12Z elau $ */

/**
 * \file 
 * $Revision: 10957 $
 * $Date: 2011-02-09 08:53:12 +0100 (St, 09 úno 2011) $ 
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
#include <QFileDialog>
#include <QLocale>
#include <QRadioButton>
#include <QTextStream>
#include <QThread>

#include <boost/python.hpp>

#include "api/PythonInterpreterLocker.h"
#include "api/PythonUtils.h"
#include "global/CompilerWarnings.h"
#include "HellingerDialog.h"
#include "HellingerDialogUi.h"
#include "HellingerModel.h"
#include "HellingerThread.h"
#include "QtWidgetUtils.h"

using namespace std;

DISABLE_GCC_WARNING("-Wwrite-strings")

GPlatesQtWidgets::HellingerThread::HellingerThread(
                HellingerDialog *hellinger_dialog,
                HellingerModel *hellinger_model):
	QThread(0),
    d_hellinger_dialog_ptr(hellinger_dialog),
    d_hellinger_model_ptr(hellinger_model),
    d_iteration(0),
	d_thread_type(POLE_THREAD_TYPE)
{
}

void
GPlatesQtWidgets::HellingerThread::initialization_pole_part(QString &import_file_path, std::vector<double> &input_data, std::vector<int> &bool_data, int &iteration, QString &python_file, QString &temporary_folder, QString &temp_pick_file, QString &temp_result, QString &temp_par, QString &temp_res)
{
    d_input_data = input_data;
    d_import_file_path = import_file_path;
    d_bool_data = bool_data;
    d_iteration = iteration;
    d_python_file = python_file;
    d_temporary_folder = temporary_folder;
    d_path = d_temporary_folder;
    d_temp_pick_file = temp_pick_file;
    d_temp_result = temp_result;
    d_temp_par = temp_par;
    d_temp_res = temp_res;



}

void
GPlatesQtWidgets::HellingerThread::initialization_stats_part(QString &path_file,QString &file_name,QString &filename_dat, QString &filename_up, QString &filename_do, QString &python_file, QString &temporary_folder, QString &temp_pick_file, QString &temp_result, QString &temp_par, QString &temp_res)
{
    d_temporary_folder = temporary_folder;
    d_temp_pick_file = temp_pick_file;
    d_temp_result = temp_result;
    d_temp_par = temp_par;
    d_temp_res = temp_res;
    if (!path_file.isEmpty())
    {
        d_path = path_file;
    }
    else
    {
        d_path = d_temporary_folder;
    }

    if (!file_name.isEmpty())
    {
        d_file_name = file_name;
    }
    else
    {
        d_file_name = d_temp_pick_file;
    }

    if (!filename_dat.isEmpty())
    {
        d_filename_dat = filename_dat;
    }
    else
    {
        d_filename_dat = "temp_dat";  // only temporary name (not used), NONE value is not accepted in python script
    }

    if (!filename_up.isEmpty())
    {
        d_filename_up = filename_up;
    }
    else
    {
        d_filename_up = "temp_up"; // only temporary name (not used), NONE value is not accepted in python script
    }

    if (!filename_do.isEmpty())
    {
        d_filename_do = filename_do;
    }
    else
    {
        d_filename_do = "temp_do"; // only temporary name (not used), NONE value is not accepted in python script
    }

    d_python_file = python_file;

}

void
GPlatesQtWidgets::HellingerThread::set_python_script(ThreadType thread_type)
{
	d_thread_type = thread_type;
}

void
GPlatesQtWidgets::HellingerThread::run()
{
    QString temp_file = d_temporary_folder + d_temp_pick_file;
    QString temp_file_temp_result = d_temporary_folder + d_temp_result;
    QString temp_file_par = d_temporary_folder + d_temp_par;
    QString temp_file_res = d_temporary_folder + d_temp_res;
    
	try{
		if (d_thread_type == POLE_THREAD_TYPE)
		{
			GPlatesApi::PythonInterpreterLocker interpreter_locker;

			// Retrieve the main module.
			boost::python::object main = boost::python::import("__main__");
			// Retrieve the main module's namespace
			boost::python::object global(main.attr("__dict__"));
			// Define greet function in Python.
			boost::python::object ignored = boost::python::exec_file(d_python_file.toStdString().c_str(),global, global);
			boost::python::object pythonCode = global["start"];
			QFile::remove(temp_file_temp_result);
			QFile::remove(temp_file_par);
			QFile::remove(temp_file_res);


			boost::python::extract<std::string>(pythonCode(temp_file.toStdString().c_str(),d_input_data[0],d_input_data[1],
				d_input_data[2], d_input_data[3], d_input_data[4], d_iteration,
				d_bool_data[0], d_bool_data[1],d_bool_data[2], d_path.toStdString(), d_filename_dat.toStdString(),
				d_filename_up.toStdString(), d_filename_do.toStdString() ));
		}
		else if (d_thread_type == STATS_THREAD_TYPE)
		{
			QFile::remove(temp_file_temp_result);
			QFile::remove(temp_file_par);
			QFile::remove(temp_file_res);     
			GPlatesApi::PythonInterpreterLocker interpreter_locker;
			boost::python::object main = boost::python::import("__main__");
			boost::python::object global(main.attr("__dict__"));
			boost::python::object ignored = boost::python::exec_file(d_python_file.toStdString().c_str(),global, global);
			boost::python::object pythonCode = global["cont"];

			boost::python::extract<std::string>(pythonCode(temp_file.toStdString().c_str(),d_input_data[0],d_input_data[1],
				d_input_data[2], d_input_data[3], d_input_data[4], d_iteration,
				d_bool_data[0], d_bool_data[1],d_bool_data[2], d_path.toStdString(), d_filename_dat.toStdString(),
				d_filename_up.toStdString(), d_filename_do.toStdString() ));

		}
	}
	catch(const boost::python::error_already_set &)
	{
		qWarning() << "Python error: " << GPlatesApi::PythonUtils::get_error_message();		
	}


}


ENABLE_GCC_WARNING("-Wwrite-strings")


