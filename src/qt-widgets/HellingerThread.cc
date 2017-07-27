/* $Id:$ */

/**
 * \file
 * $Revision: 10957 $
 * $Date: 2011-02-09 08:53:12 +0100 (St, 09 úno 2011) $
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

// This definition sets the maximum number of
// parameters that you can send to a boost python function.
#define BOOST_PYTHON_MAX_ARITY 16

#include <QDebug>
#include <QThread>


#include "api/PythonInterpreterLocker.h"
#include "api/PythonUtils.h"
#include "global/CompilerWarnings.h"
#include "global/python.h"
#include "HellingerDialog.h"
#include "HellingerModel.h"
#include "HellingerThread.h"
#include "QtWidgetUtils.h"

const QString TEMP_PICK_FILENAME("temp_pick");
const QString TEMP_RESULT_FILENAME("temp_pick_temp_result");
const QString TEMP_PAR_FILENAME("temp_pick_par");
const QString TEMP_RES_FILENAME("temp_pick_res");

GPlatesQtWidgets::HellingerThread::HellingerThread(
		HellingerDialog *hellinger_dialog,
		HellingerModel *hellinger_model):
	QThread(0),
	d_hellinger_dialog_ptr(hellinger_dialog),
	d_hellinger_model_ptr(hellinger_model),
	d_thread_type(TWO_WAY_POLE_THREAD_TYPE),
	d_temp_pick_file(TEMP_PICK_FILENAME),
	d_temp_result_filename(TEMP_RESULT_FILENAME),
	d_temp_par(TEMP_PAR_FILENAME),
	d_temp_res(TEMP_RES_FILENAME),
	d_thread_failed(false)
{
}

void
GPlatesQtWidgets::HellingerThread::set_python_script_type(ThreadType thread_type)
{
	d_thread_type = thread_type;
}

void
GPlatesQtWidgets::HellingerThread::calculate_two_way_fit()
{
	QString temp_file = d_path_for_temporary_files + d_temp_pick_file;

	GPlatesApi::PythonInterpreterLocker interpreter_locker;

	// Retrieve the main module.
	boost::python::object main = boost::python::import("__main__");

	boost::python::object global(main.attr("__dict__"));
	boost::python::object ignored = boost::python::exec_file(d_python_file.toStdString().c_str(),global, global);
	boost::python::object pythonCode = global["calculate_pole_2_way"];

	HellingerPoleEstimate estimate_12 = d_hellinger_model_ptr->get_initial_guess_12();

	boost::python::extract<std::string>(pythonCode(
											temp_file.toStdString().c_str(),
											estimate_12.d_lat,
											estimate_12.d_lon,
											estimate_12.d_angle,
											d_hellinger_model_ptr->get_search_radius(),
											d_hellinger_model_ptr->get_confidence_level(),
											d_hellinger_model_ptr->get_grid_iterations(),
											d_hellinger_model_ptr->get_grid_search(),
											d_hellinger_model_ptr->get_use_amoeba_tolerance(),
											d_hellinger_model_ptr->get_amoeba_tolerance(),
											d_hellinger_model_ptr->get_use_amoeba_iterations(),
											d_hellinger_model_ptr->get_amoeba_iterations(),
											d_output_path.toStdString().c_str(),
											d_results_filename_root.toStdString().c_str(),
											d_path_for_temporary_files.toStdString().c_str()));
}

void
GPlatesQtWidgets::HellingerThread::calculate_three_way_fit()
{
	QString pick_file = d_path_for_temporary_files + d_temp_pick_file;

	GPlatesApi::PythonInterpreterLocker interpreter_locker;

	// Retrieve the main module.
	boost::python::object main = boost::python::import("__main__");

	boost::python::object global(main.attr("__dict__"));
	boost::python::object ignored = boost::python::exec_file(d_python_file.toStdString().c_str(),global, global);
	boost::python::object pythonCode = global["calculate_pole_3_way"];

	HellingerPoleEstimate estimate_12 = d_hellinger_model_ptr->get_initial_guess_12();
	HellingerPoleEstimate estimate_13 = d_hellinger_model_ptr->get_initial_guess_13();

	boost::python::extract<std::string>(pythonCode(
											pick_file.toStdString().c_str(),
											estimate_12.d_lat,
											estimate_12.d_lon,
											estimate_12.d_angle,
											estimate_13.d_lat,
											estimate_13.d_lon,
											estimate_13.d_angle,
											d_hellinger_model_ptr->get_search_radius(),
											d_hellinger_model_ptr->get_confidence_level(),
											d_hellinger_model_ptr->get_use_amoeba_tolerance(),
											d_hellinger_model_ptr->get_amoeba_tolerance(),
											d_hellinger_model_ptr->get_use_amoeba_iterations(),
											d_hellinger_model_ptr->get_amoeba_iterations(),
											d_output_path.toStdString(),
											d_results_filename_root.toStdString(),
											d_path_for_temporary_files.toStdString()));
}

void
GPlatesQtWidgets::HellingerThread::calculate_two_way_uncertainties()
{
	QString pick_file = d_path_for_temporary_files + d_temp_pick_file;

	GPlatesApi::PythonInterpreterLocker interpreter_locker;

	boost::python::object main = boost::python::import("__main__");
	boost::python::object global(main.attr("__dict__"));
	boost::python::object ignored = boost::python::exec_file(d_python_file.toStdString().c_str(),global, global);
	boost::python::object pythonCode = global["calculate_uncertainty_2_way"];

	HellingerPoleEstimate estimate_12 = d_hellinger_model_ptr->get_initial_guess_12();

	boost::python::extract<std::string>(pythonCode(
											pick_file.toStdString().c_str(),
											estimate_12.d_lat,
											estimate_12.d_lon,
											estimate_12.d_angle,
											d_hellinger_model_ptr->get_search_radius(),
											d_hellinger_model_ptr->get_confidence_level(),
											d_hellinger_model_ptr->get_grid_iterations(),
											d_hellinger_model_ptr->get_grid_search(),
											d_hellinger_model_ptr->get_use_amoeba_tolerance(),
											d_hellinger_model_ptr->get_amoeba_tolerance(),
											d_hellinger_model_ptr->get_use_amoeba_iterations(),
											d_hellinger_model_ptr->get_amoeba_iterations(),
											d_output_path.toStdString(),
											d_results_filename_root.toStdString(),
											d_path_for_temporary_files.toStdString()));
}

void
GPlatesQtWidgets::HellingerThread::calculate_three_way_uncertainties()
{
	qDebug() << "calculate_three_way_uncertainties";
	QString pick_file = d_path_for_temporary_files + d_temp_pick_file;

	GPlatesApi::PythonInterpreterLocker interpreter_locker;

	// Retrieve the main module.
	boost::python::object main = boost::python::import("__main__");

	boost::python::object global(main.attr("__dict__"));
	boost::python::object ignored = boost::python::exec_file(d_python_file.toStdString().c_str(),global, global);
	boost::python::object pythonCode = global["calculate_uncertainty_3_way"];

	HellingerPoleEstimate estimate_12 = d_hellinger_model_ptr->get_initial_guess_12();
	HellingerPoleEstimate estimate_13 = d_hellinger_model_ptr->get_initial_guess_13();

	boost::python::extract<std::string>(pythonCode(
											pick_file.toStdString().c_str(),
											estimate_12.d_lat,
											estimate_12.d_lon,
											estimate_12.d_angle,
											estimate_13.d_lat,
											estimate_13.d_lon,
											estimate_13.d_angle,
											d_hellinger_model_ptr->get_search_radius(),
											d_hellinger_model_ptr->get_confidence_level(),
											d_hellinger_model_ptr->get_use_amoeba_tolerance(),
											d_hellinger_model_ptr->get_amoeba_tolerance(),
											d_hellinger_model_ptr->get_use_amoeba_iterations(),
											d_hellinger_model_ptr->get_amoeba_iterations(),
											d_output_path.toStdString(),
											d_results_filename_root.toStdString(),
											d_path_for_temporary_files.toStdString()));
}

void
GPlatesQtWidgets::HellingerThread::run()
{
	// FIXME: the output file names are hard-coded in the python script, so changing these
	// filenames here, or elsewhere in this class, will likely result in not being able to find/open
	// the result files....
	QString temp_file = d_path_for_temporary_files + d_temp_pick_file;
	QString temp_file_temp_result = d_path_for_temporary_files + d_temp_result_filename;
	QString temp_file_par = d_path_for_temporary_files + d_temp_par;
	QString temp_file_res = d_path_for_temporary_files + d_temp_res;

	// Remove any old temporary files arising from previous fits.
	QFile::remove(temp_file_temp_result);
	QFile::remove(temp_file_par);
	QFile::remove(temp_file_res);

#if 1
	qDebug() << "temp file: " << temp_file;
	qDebug() << "temp_file_temp_result: " << temp_file_temp_result;
	qDebug() << "temp file par: " << temp_file_par;
	qDebug() << "temp_file_res: " << temp_file_res;
	qDebug() << "about to run thread. result file: " << temp_file_temp_result;
	qDebug() << "python file: " << d_python_file;
#endif


	d_thread_failed = false;

	try{
		switch(d_thread_type)
		{
			case TWO_WAY_POLE_THREAD_TYPE:
				calculate_two_way_fit();
				break;
			case THREE_WAY_POLE_THREAD_TYPE:
				calculate_three_way_fit();
				break;
			case TWO_WAY_UNCERTAINTY_THREAD_TYPE:
				calculate_two_way_uncertainties();
				break;
			case THREE_WAY_UNCERTAINTY_THREAD_TYPE:
				calculate_three_way_uncertainties();
				break;
		default:
			return;
		}
	}
	catch(const boost::python::error_already_set &)
	{
		qWarning() << "Python error: " << GPlatesApi::PythonUtils::get_error_message();
		d_thread_failed = true;

	}
	catch(const std::exception &e)
	{
		qWarning() << "Caught exception " << e.what() << "from HellingerThread::run()";
		d_thread_failed = true;
	}


}

QString
GPlatesQtWidgets::HellingerThread::temp_pick_filename() const
{
	return d_temp_pick_file;
}

QString
GPlatesQtWidgets::HellingerThread::temp_result_filename() const
{
	return d_temp_result_filename;
}

QString
GPlatesQtWidgets::HellingerThread::temp_par_filename() const
{
	return d_temp_par;
}

void
GPlatesQtWidgets::HellingerThread::initialise(
		const QString &python_file,
		const QString &output_path,
		const QString &results_filename_root,
		const QString &temporary_path)
{
	d_python_file = python_file;
	d_output_path = output_path;
	d_results_filename_root = results_filename_root;
	d_path_for_temporary_files = temporary_path;
}



