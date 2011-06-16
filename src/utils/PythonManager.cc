/* $Id: ApplicationState.cc 11614 2011-05-20 15:10:51Z jcannon $ */

/**
 * \file 
 * $Revision: 11614 $
 * $Date: 2011-05-21 01:10:51 +1000 (Sat, 21 May 2011) $
 * 
 * Copyright (C) 2009, 2010, 2011 The University of Sydney, Australia
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
#include <QDir>
#include <iostream>

#include "PythonManager.h"

#include "api/PythonExecutionThread.h"
#include "api/PythonInterpreterLocker.h"
#include "api/PythonRunner.h"
#include "api/PythonUtils.h"
#include "api/Sleeper.h"
#include "app-logic/ApplicationState.h"
#include "app-logic/UserPreferences.h"

#include "presentation/Application.h"
#include "qt-widgets/QtWidgetUtils.h"
#include "qt-widgets/PythonConsoleDialog.h"

#include "utils/StringUtils.h"

#if !defined(GPLATES_NO_PYTHON)
extern "C" void initpygplates();

void
GPlatesUtils::PythonManager::initialize() 
{
	if(d_inited)
	{
		qWarning() << "The embedded python interpret has been initialized already.";
		return;
	}
	using namespace boost::python;
	init_python_interpreter();
	// Hold references to the main module and its namespace for easy access from
	// all parts of GPlates.
	GPlatesApi::PythonInterpreterLocker interpreter_locker;
	try
	{
		d_python_main_thread_runner = new GPlatesApi::PythonRunner(d_python_main_namespace,this);
		d_python_execution_thread = new GPlatesApi::PythonExecutionThread(d_python_main_namespace, this);
		d_python_execution_thread->start(QThread::IdlePriority);
	
		d_inited = true;
		register_utils_scripts();
		init_python_console();
	}
	catch (const error_already_set &)
	{
		qWarning() << GPlatesApi::PythonUtils::get_error_message();
		throw PythonInitFailed(GPLATES_EXCEPTION_SOURCE);
	}
}

GPlatesUtils::PythonManager::~PythonManager()
{
	// Stop the Python execution thread.
	static const int WAIT_TIME = 1000 /* milliseconds */;
	if(d_python_execution_thread)
	{
		d_python_execution_thread->quit_event_loop();
		if(!d_python_execution_thread->wait(WAIT_TIME))
		{
			d_python_execution_thread->terminate();
			d_python_execution_thread->wait();
		}
	}
	delete d_python_execution_thread;
	delete d_python_console_dialog_ptr;
	delete d_python_main_thread_runner;
	delete d_sleeper;
}

void
GPlatesUtils::PythonManager::init_python_interpreter(std::string program_name)
{
	using namespace boost::python;

	// Initialise the embedded Python interpreter.
	char GPLATES_MODULE_NAME[] = "pygplates";
	if (PyImport_AppendInittab(GPLATES_MODULE_NAME, &initpygplates))
	{
		qWarning() << GPlatesApi::PythonUtils::get_error_message();
		throw PythonInitFailed(GPLATES_EXCEPTION_SOURCE);
	}

	const std::size_t MaxProgramNameLen = 256; 
	char name[MaxProgramNameLen];
	program_name.copy(name,(program_name.size() < MaxProgramNameLen ? program_name.size() : MaxProgramNameLen));

	Py_SetProgramName(name);
	Py_Initialize();

	// Initialise Python threading support; this grabs the Global Interpreter Lock
	// for this thread.
	PyEval_InitThreads();

	// But then we give up the GIL, so that PythonInterpreterLocker may now be used.
	PyEval_SaveThread();

	GPlatesApi::PythonInterpreterLocker interpreter_locker;

	// Load the pygplates module.
	try
	{
		d_python_main_module = import("__main__");
		d_python_main_namespace = d_python_main_module.attr("__dict__");
		object pygplates_module = import("pygplates");
		d_python_main_namespace["pygplates"] = pygplates_module;
	}
	catch (const error_already_set &)
	{
		std::cerr << "Fatal error while loading pygplates module" << std::endl;
		qWarning() << GPlatesApi::PythonUtils::get_error_message();
		throw PythonInitFailed(GPLATES_EXCEPTION_SOURCE);
	}

	//The objective of the following code is mysterious to me. 
	//Comment out them until I figure out the purpose of these code.
	//At least one crash is caused by deleting python built-in functions.
#if 0
	// Importing "sys" enables the printing of the value of expressions in
	// the interactive Python console window, and importing "builtin" enables
	// the magic variable "_" (the last result in the interactive window).
	// We then delete them so that the packages don't linger around, but their
	// effect remains even after deletion.
	// Note: using boost::python::exec doesn't achieve the desired effects.
	if (PyRun_SimpleString("import sys, __builtin__; del sys; del __builtin__"))
	{
		std::cerr << "Failed to import sys, __builtin__" << std::endl;
		qWarning() << GPlatesApi::PythonUtils::get_error_message();
	}

	// Get rid of some built-in functions.
	if (PyRun_SimpleString("import __builtin__; del __builtin__.copyright, __builtin__.credits, __builtin__.license, __builtin__"))
	{
		std::cerr << "Failed to delete some built-in functions" << std::endl;
		qWarning() << GPlatesApi::PythonUtils::get_error_message();
	}
#endif
}


QFileInfoList
GPlatesUtils::PythonManager::get_scripts()
{
	GPlatesAppLogic::UserPreferences& user_prefs = 
		GPlatesPresentation::Application::instance()->get_application_state().get_user_preferences();
	QFileInfoList file_list;
	//The ".pyc" files will be generated every time the python script is executed.
	//And this caused duplicate menu items.
	QStringList filters = (QStringList() << "*.py" ); //<< "*.pyc");

	QDir cwd;
	cwd.setNameFilters(filters);
	if (cwd.cd("scripts")) {
		file_list << cwd.entryInfoList(QDir::Files, QDir::Name);
		d_scripts_paths.push_back(cwd);
	}

	// Look in system-specific locations for supplied sample scripts, site-specific scripts, etc.
	// The default location will be platform-dependent and is currently set up in UserPreferences.cc.
	QDir system_scripts_dir(user_prefs.get_value("paths/python_system_script_dir").toString());
	system_scripts_dir.setNameFilters(filters);
	if (system_scripts_dir.exists() && system_scripts_dir.isReadable()) {
		file_list << system_scripts_dir.entryInfoList(QDir::Files, QDir::Name);
		d_scripts_paths.push_back(system_scripts_dir);
	}

	// Also look in user-specific application data locations for scripts the user may have made.
	// The default location will be platform dependent and is constructed based on what Qt tells us about
	// the platform conventions.
	QDir user_scripts_dir(user_prefs.get_value("paths/python_user_script_dir").toString());
	user_scripts_dir.setNameFilters(filters);
	if (user_scripts_dir.exists() && user_scripts_dir.isReadable()) {
		file_list << user_scripts_dir.entryInfoList(QDir::Files, QDir::Name);
		d_scripts_paths.push_back(user_scripts_dir);
	}
	return file_list;
}

void
GPlatesUtils::PythonManager::add_sys_path()
{
	GPlatesApi::PythonInterpreterLocker interpreter_locker;
	PyRun_SimpleString("import sys");
	BOOST_FOREACH(const QDir& dir, d_scripts_paths)
	{
		QString p_code = QString() + 
			"sys.path.append(\"" + dir.absolutePath().replace("\\","/") +"\")\n";
		PyRun_SimpleString(p_code.toStdString().c_str());
	}
}


void 
GPlatesUtils::PythonManager::register_utils_scripts()
{
	// We need to wait for the user interface is ready
	// before we start running Python scripts.
	QFileInfoList file_list = get_scripts();
	add_sys_path();
	BOOST_FOREACH(const QFileInfo& file, file_list)
	{
		register_script(file.absoluteFilePath(),"utf-8"); 
	}
}


void
GPlatesUtils::PythonManager::register_script(
		const QString& name, 
		const QString& encoding)
{
	using namespace boost::python;
	try
	{
		GPlatesApi::PythonInterpreterLocker interpreter_locker;

		QFileInfo file_info(name);

		object module = import(file_info.baseName().toStdString().c_str());
		module.attr("register")(); //TODO: define static const for this. 
		qDebug() << "The script " << name << " has been registered.";
	}
	catch (const error_already_set &)
	{
		qWarning() << GPlatesApi::PythonUtils::get_error_message();
		PySys_WriteStderr("The script is not registered.\n");
		qWarning() << "Failed to register  " << name;
	}
}

void
GPlatesUtils::PythonManager::init_python_console()
{
	using namespace GPlatesPresentation;
	if(!d_python_console_dialog_ptr)
	{
		d_python_console_dialog_ptr = new GPlatesQtWidgets::PythonConsoleDialog(
				Application::instance()->get_application_state(),
				Application::instance()->get_view_state(),
				&Application::instance()->get_viewport_window());
		d_event_blackout.add_blackout_exemption(d_python_console_dialog_ptr);
	}
}


void
GPlatesUtils::PythonManager::pop_up_python_console()
{
	if(d_python_console_dialog_ptr)
	{	
		GPlatesQtWidgets::QtWidgetUtils::pop_up_dialog(d_python_console_dialog_ptr);
	}
}


void
GPlatesUtils::PythonManager::python_started()
{
	// We need to stop the event blackout if it has started. This is because one of
	// the reasons to run code on the main thread is to run PyQt related code - and
	// if we're eating all Qt events, that's a bad thing! Also, if Python code is
	// running on the main thread, the user interface is unresponsive anyway, so
	// there is no need for the event blackout.
	if (d_event_blackout.has_started())
	{
		d_event_blackout.stop();
		d_stopped_event_blackout_for_python_runner = true;
	}
}


void
GPlatesUtils::PythonManager::python_finished()
{
	// Restore the event blackout if it was started before we stopped it when code
	// started to run on the main thread.
	if (d_stopped_event_blackout_for_python_runner)
	{
		d_event_blackout.start();
		d_stopped_event_blackout_for_python_runner = false;
	}
}

void
GPlatesUtils::PythonManager::python_runner_started()
{
	d_event_blackout.start();
	boost::function<void ()> f = 
		boost::bind(
				&GPlatesQtWidgets::PythonConsoleDialog::show_cancel_widget,
				d_python_console_dialog_ptr,
				&d_event_blackout);
	GPlatesApi::PythonUtils::run_in_main_thread(f);
}


void
GPlatesUtils::PythonManager::python_runner_finished()
{
	d_event_blackout.stop();
	boost::function<void ()> f = 
		boost::bind(
				&GPlatesQtWidgets::PythonConsoleDialog::hide_cancel_widget,
				d_python_console_dialog_ptr,
				&d_event_blackout);
	GPlatesApi::PythonUtils::run_in_main_thread(f);
}

#endif //GPLATES_NO_PYTHON













