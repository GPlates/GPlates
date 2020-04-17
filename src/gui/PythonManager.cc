/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
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

#include <QByteArray>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QMap>
#include <iostream>
#include <string>

#include "PythonManager.h"

#include "api/PythonExecutionThread.h"
#include "api/PythonInterpreterLocker.h"
#include "api/PythonRunner.h"
#include "api/PythonUtils.h"
#include "api/Sleeper.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/UserPreferences.h"

#include "file-io/ErrorOpeningFileForReadingException.h"

#include "presentation/Application.h"

#include "qt-widgets/QtWidgetUtils.h"
#include "qt-widgets/PythonConsoleDialog.h"

#include "utils/StringUtils.h"

#if !defined(GPLATES_NO_PYTHON)

#if PY_MAJOR_VERSION >= 3
extern "C" PyMODINIT_FUNC PyInit_pygplates(void);
#else
extern "C" void initpygplates();
#endif 

GPlatesGui::PythonManager::PythonManager() : 
	d_python_main_thread_runner(NULL),
	d_python_execution_thread(NULL),
	d_sleeper(NULL),
	d_inited(false), 
	d_python_console_dialog_ptr(NULL),
	d_stopped_event_blackout_for_python_runner(false),
	d_clear_python_prefix_flag(true)
{
	//this UserPreferences must be local.
	//don't use global UserPreferences because it hasn't been constructed yet.
	GPlatesAppLogic::UserPreferences user_pref(NULL);
	d_show_python_init_fail_dlg = user_pref.get_value("python/show_python_init_fail_dialog").toBool();
	d_python_home = user_pref.get_value("python/python_home").toString();

	QRegExp rx("^\\d.\\d"); // Match d.d
	rx.indexIn(QString(Py_GetVersion()));
	d_python_version = rx.cap();
	//qDebug() << "Python Version: " << d_python_version;
}

void
GPlatesGui::PythonManager::initialize(
		char* argv[],
		GPlatesPresentation::Application *app) 
{
	d_application = app;
	if(d_inited)
	{
		qWarning() << "The embedded python interpret has been initialized already.";
		return;
	}

	using namespace boost::python;
	init_python_interpreter(argv);
	// Hold references to the main module and its namespace for easy access from
	// all parts of GPlates.
	try
	{
		using namespace GPlatesApi;
		PythonInterpreterLocker interpreter_locker;
		d_python_main_thread_runner = new PythonRunner(d_python_main_namespace,this);
		d_python_execution_thread = new PythonExecutionThread(d_python_main_namespace, this);
		d_python_execution_thread->start(QThread::IdlePriority);
		check_python_capability();

		d_inited = true;
		set_python_prefix();
		register_utils_scripts();
		init_python_console();
	}
	catch (const error_already_set &)
	{
		qWarning() << GPlatesApi::PythonUtils::get_error_message();
		throw PythonInitFailed(GPLATES_EXCEPTION_SOURCE);
	}
}


void
GPlatesGui::PythonManager::check_python_capability()
{
	// FIXME: For some reason importing 'sys' is required for Python 3.
	//
	// This was discovered because 'register_utils_scripts()' seemed to work,
	// but the code below did not. And the only difference is 'set_python_prefix()'
	// is called between them and that calls 'bp::import("sys")'.
#if PY_MAJOR_VERSION >= 3
	bp::import("sys");
#endif

	QString test_code = QString() +
			"from __future__ import print_function;" +
			"print(\'******Start testing python capability******\');" +
			"import sys;" +
			"import code;"  +
			"import math;"  +
			"import platform;" +
			"import pygplates;" +
			"print(\'python import test passed.\');" +
			"math.log(12);" +
			"print(\'python math test passed.\');" +
			"print(\'Version: \'); print(sys.version_info);" +
			"sys.platform;" +
			"platform.uname();" +
			"print(\'Prefix: \' +sys.prefix);" +
			"print(\'Exec Prefix: \'+sys.exec_prefix);" +
			"print(\'python system test passed.\');" +
			"print(\'******End of testing python capability******\');";

	bool result = true;
	GPlatesApi::PythonInterpreterLocker l;

	result &= (0 == PyRun_SimpleString(test_code.toStdString().c_str()));
	
	if(!result)
	{
		throw PythonInitFailed(GPLATES_EXCEPTION_SOURCE);
	}

}


GPlatesGui::PythonManager::~PythonManager()
{
	if(d_clear_python_prefix_flag)
	{
		set_python_prefix("");//clear python prefix
	}

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
GPlatesGui::PythonManager::set_show_init_fail_dlg(
		bool flag) 
{
	d_show_python_init_fail_dlg = flag;
	GPlatesAppLogic::UserPreferences(NULL).set_value(
			"python/show_python_init_fail_dialog",
			flag);
}


void
GPlatesGui::PythonManager::set_python_prefix(
		const QString& str)
{
	GPlatesAppLogic::UserPreferences(NULL).set_value(
			"python/prefix",
			str);
}


void
GPlatesGui::PythonManager::set_python_prefix()
{
	bp::object module = bp::import("sys");
	const char* prefix = bp::extract<const char*>(module.attr("prefix"));
	set_python_prefix(QString(prefix));
}


QString
GPlatesGui::PythonManager::get_python_prefix_from_preferences()
{
	return GPlatesAppLogic::UserPreferences(NULL).get_value("python/prefix").toString();
}


void
GPlatesGui::PythonManager::init_python_interpreter(
		char* argv[])
{
	using namespace boost::python;
	using namespace GPlatesApi;
	// Initialize the embedded Python interpreter.
	char GPLATES_MODULE_NAME[] = "pygplates";
#if PY_MAJOR_VERSION >= 3
	if (PyImport_AppendInittab(GPLATES_MODULE_NAME, &PyInit_pygplates))
#else
    if (PyImport_AppendInittab(GPLATES_MODULE_NAME, &initpygplates))
#endif
	{
		qWarning() << PythonUtils::get_error_message();
		throw PythonInitFailed(GPLATES_EXCEPTION_SOURCE);
	}

	/*
	Set the Program name properly.
	The following info is taken from Python manual.
	This function should be called before Py_Initialize() is called for the first time, 
	if it is called at all. It tells the interpreter the value of the argv[0] argument to 
	the main() function of the program. This is used by Py_GetPath() and some other functions 
	below to find the Python run-time libraries relative to the interpreter executable. 
	The default value is 'python'. The argument should point to a zero-terminated character 
	string in static storage whose contents will not change for the duration of the program’s 
	execution. No code in the Python interpreter will change the contents of this storage.
	*/
#if PY_MAJOR_VERSION >= 3
	// Convert char* to wchar_t*
	const std::wstring program_name = GPlatesUtils::make_wstring_from_qstring(QString(argv[0]));
	// Seems Py_SetProgramName accepts a wchar_t* pointer to 'non-const', so make a copy.
	std::vector<wchar_t> program_name_non_const(program_name.begin(), program_name.end());
	program_name_non_const.push_back('\0'); // Null terminate the string.

	Py_SetProgramName(&program_name_non_const[0]);
#else
	Py_SetProgramName(argv[0]);
#endif
	/*
	Ignore the environment variables. This is necessary because GPlates only works with the *correct* python version, 
	which is the version that boost python uses. So, the python version has been determined when compiling 
	GPlates. A copy of the python standard library files have been included in GPlates package. It is always 
	safer to use the files in the bundle. However, the PYTHONHOME and PYTHONPATH environment variable setting on user's computer
	could point	to a *wrong* python installation, which could cause GPlates failed to initialize embeded python 
	interpreter. We force GPlates to use the directory which contains GPlates executable binary as the python home 
	by setting Py_IgnoreEnvironmentFlag flag. On Mac OS, it is the python framework inside application bundle.
	*/

	/*
	* Due to the CRT mismatch, putenv cannot set environment variables without restarting. Use this flag for now.
	* The CRT mismatch problem maybe need to be addressed in the future, but not now.
	*/
	// The GPLATES_IGNORE_PYTHON_ENVIRONMENT is defined in 'ConfigDefault.cmake' and 'src/global/config.h.in'
	#ifdef GPLATES_IGNORE_PYTHON_ENVIRONMENT
	Py_IgnoreEnvironmentFlag = 1;
	#endif

	/* 
	Info from Python manual.
	Initialize the Python interpreter. In an application embedding Python, this should be called before 
	using any other Python/C API functions; with the exception of Py_SetProgramName(), 
	Py_SetPythonHome(), PyEval_InitThreads(), PyEval_ReleaseLock(), and PyEval_AcquireLock(). 
	This initializes the table of loaded modules (sys.modules), and creates the fundamental 
	modules __builtin__ ('builtins' for Python 3), __main__ and sys. It also initializes the module search path (sys.path). 
	It does not set sys.argv; use PySys_SetArgvEx() for that. This is a no-op when called for a 
	second time (without calling Py_Finalize() first). There is no return value; it is a fatal error 
	if the initialization fails.
	*/
	Py_Initialize();
		
	// Initialise Python threading support; this grabs the Global Interpreter Lock
	// for this thread.
	PyEval_InitThreads();

	// But then we give up the GIL, so that PythonInterpreterLocker may now be used.
	PyEval_SaveThread();

	PythonInterpreterLocker interpreter_locker;

	// Load the 'pygplates' module.
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
		qWarning() << PythonUtils::get_error_message();
		throw PythonInitFailed(GPLATES_EXCEPTION_SOURCE);
	}

	//The objective of the following code is mysterious to me. 
	//Comment out them until I figure out the purpose of these code.
	//At least one crash is caused by deleting python built-in functions.
	//
	// NOTE: Python 3 names the "__builtin__" module "builtins"
	// (in case this code is later uncommented).
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


QMap<QString/*module name*/, QString/*module filename*/>
GPlatesGui::PythonManager::get_internal_scripts()
{
	QMap<QString/*module name*/, QString> modules;

	// Iterate recursively over all files/directories in the ":/python/scripts/" resources directory.
	QDirIterator module_dir_iterator(
			":/python/scripts",
			QStringList("*.py"),
			QDir::Files,
			QDirIterator::Subdirectories);
	while (module_dir_iterator.hasNext())
	{
		const QString module_file_path = module_dir_iterator.next();
		const QString module_name = QFileInfo(module_file_path).baseName();

		modules.insert(module_name, module_file_path);
	}

	return modules;
}


QMap<QString/*module name*/, QFileInfo/*module filename*/>
GPlatesGui::PythonManager::get_external_scripts()
{
	GPlatesAppLogic::UserPreferences& user_prefs = 
		d_application->get_application_state().get_user_preferences();
	QFileInfoList module_file_list;
	QStringList filters = (QStringList() << "*.py" << "*.pyc");

	// We might not need this (current working directory) now that internal scripts are supported,
	// because development builds no longer need to access the 'scripts/' subdirectory of GPlates source code.
	// But keep anyway, might be useful during development because can try out new scripts
	// simply by placing them in the 'scripts/' subdirectory of the GPlates source code
	// (provided the root of the source directory is the current working directory).
	QDir cwd;
	cwd.setNameFilters(filters);
	if (cwd.cd("scripts")) {
		module_file_list << cwd.entryInfoList(QDir::Files, QDir::Name);
		d_external_scripts_paths.push_back(cwd);
	}

	// Look in system-specific locations for supplied sample scripts, site-specific scripts, etc.
	// The default location will be platform-dependent and is currently set up in UserPreferences.cc.
	QDir system_scripts_dir(user_prefs.get_value("paths/python_system_script_dir").toString());
	system_scripts_dir.setNameFilters(filters);
	if (system_scripts_dir.exists() && system_scripts_dir.isReadable()) {
		module_file_list << system_scripts_dir.entryInfoList(QDir::Files, QDir::Name);
		d_external_scripts_paths.push_back(system_scripts_dir);
	}

	// Also look in user-specific application data locations for scripts the user may have made.
	// The default location will be platform dependent and is constructed based on what Qt tells us about
	// the platform conventions.
	QDir user_scripts_dir(user_prefs.get_value("paths/python_user_script_dir").toString());
	user_scripts_dir.setNameFilters(filters);
	if (user_scripts_dir.exists() && user_scripts_dir.isReadable()) {
		module_file_list << user_scripts_dir.entryInfoList(QDir::Files, QDir::Name);
		d_external_scripts_paths.push_back(user_scripts_dir);
	}

	//Try the best to find python script files at all possible locations.
	//Always fallback to default system scripts directory. 
	//The script files in default system scripts directory have the lowest priority.
	//On Linux, "/usr/share/gplates/scripts"
	//On Mac, QCoreApplication::applicationDirPath() + "/../Resources/scripts/"
	//On Windows, QCoreApplication::applicationDirPath() + "/scripts/"
	QDir default_system_scripts_dir(user_prefs.get_default_value("paths/python_system_script_dir").toString());
	default_system_scripts_dir.setNameFilters(filters);
	if (default_system_scripts_dir.exists() && default_system_scripts_dir.isReadable()) {
		module_file_list << default_system_scripts_dir.entryInfoList(QDir::Files, QDir::Name);
		d_external_scripts_paths.push_back(default_system_scripts_dir);
	}

	// Get a unique list of scripts based on their module names.
	// Modules (with the same name) added first take priority.
	// Which means the above search path order is in order of priority (ie, higher priority searched first).
	// Also note that the internal modules (in Qt resource files), not handled here, have even higher priority.
	QMap<QString/*module name*/, QFileInfo> modules;
	for (int n = 0; n < module_file_list.size(); ++n)
	{
		const QFileInfo &module_file = module_file_list[n];
		const QString module_name = module_file.baseName();

		if (!modules.contains(module_name))
		{
			modules.insert(module_name, module_file);
		}
	}

	return modules;
}


void
GPlatesGui::PythonManager::add_sys_path()
{
	GPlatesApi::PythonInterpreterLocker interpreter_locker;
	PyRun_SimpleString("import sys");
	BOOST_FOREACH(const QDir& dir, d_external_scripts_paths)
	{
		QString p_code = QString() + 
			"sys.path.append(\"" + dir.absolutePath().replace("\\","/") +"\")\n";
		PyRun_SimpleString(p_code.toStdString().c_str());
	}
}


void 
GPlatesGui::PythonManager::register_utils_scripts()
{
	// We need to wait for the user interface is ready
	// before we start running Python scripts.

	const QMap<QString/*module name*/, QString/*module filename*/>
			internal_scripts = get_internal_scripts();

	// Register internal scripts.
	BOOST_FOREACH(const QString internal_module_name, internal_scripts.keys())
	{
		register_internal_script(internal_module_name, internal_scripts[internal_module_name]);
	}

	// Get a unique list of scripts based on their module names.
	const QMap<QString/*module name*/, QFileInfo/*module filename*/>
			external_scripts = get_external_scripts();

	// External script paths need to be added to 'sys.path'.
	add_sys_path();

	// Register external scripts that haven't already been registered internally.
	BOOST_FOREACH(const QString external_module_name, external_scripts.keys())
	{
		if (!internal_scripts.contains(external_module_name))
		{
			register_external_script(external_module_name);
		}
	}
}


void
GPlatesGui::PythonManager::register_internal_script(
		const QString &internal_module_name,
		const QString &internal_module_filename)
{
	namespace bp = boost::python;

	try
	{
		GPlatesApi::PythonInterpreterLocker interpreter_locker;

		QFile internal_module_file(internal_module_filename);
		// This should never fail since we are reading from files that are embedded Qt resources.
		if (!internal_module_file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			throw GPlatesFileIO::ErrorOpeningFileForReadingException(
					GPLATES_EXCEPTION_SOURCE,
					internal_module_filename);
		}

		// Read the entire file.
		const QByteArray internal_module_code = internal_module_file.readAll();

		// Compile the internal module code and import it.
		bp::object compiled_object = bp::object(bp::handle<>(
				// Returns a new reference so no need for 'bp::borrowed'...
				Py_CompileString(
						internal_module_code.constData(),
						internal_module_filename.toLatin1().constData(),
						Py_file_input)));

		// Import the compiled internal module code.
		std::string internal_module_name_string = internal_module_name.toStdString();
		bp::object internal_module = bp::object(bp::handle<>(
				// Returns a new reference so no need for 'bp::borrowed'...
				PyImport_ExecCodeModule(
						const_cast<char*>(internal_module_name_string.c_str()),
						compiled_object.ptr())));

		// Register the internal script.
		internal_module.attr("register")();
	}
	catch (const bp::error_already_set &)
	{
		// The "get_error_message()" function call is essential here - clears Python error indicator.
		GPlatesApi::PythonUtils::get_error_message();
	}
}


void
GPlatesGui::PythonManager::register_external_script(
		const QString &external_module_name)
{
	namespace bp = boost::python ;;
	try
	{
		GPlatesApi::PythonInterpreterLocker interpreter_locker;

		bp::object external_module = bp::import(external_module_name.toStdString().c_str());
		external_module.attr("register")();

		qDebug() << "The script" << external_module_name << "has been registered.";
	}
	catch (const bp::error_already_set &)
	{
		//get_error_message() function call is essential here.
		//The python error bit will be reset in get_error_message(). It is important.
		GPlatesApi::PythonUtils::get_error_message();


		//qDebug() << GPlatesApi::PythonUtils::get_error_message();
		//PySys_WriteStderr("The script is not registered.\n");
		//qDebug() << "Failed to register  " << name;
	}
}


void
GPlatesGui::PythonManager::init_python_console()
{
	using namespace GPlatesPresentation;
	if(!d_python_console_dialog_ptr)
	{
		d_python_console_dialog_ptr = 
			new GPlatesQtWidgets::PythonConsoleDialog(
					d_application->get_application_state(),
					d_application->get_view_state(),
					&d_application->get_main_window());
		d_event_blackout.add_blackout_exemption(d_python_console_dialog_ptr);
	}
}


void
GPlatesGui::PythonManager::pop_up_python_console()
{
	if(d_python_console_dialog_ptr)
	{	
		GPlatesQtWidgets::QtWidgetUtils::pop_up_dialog(d_python_console_dialog_ptr);
	}
}


void
GPlatesGui::PythonManager::python_started()
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
GPlatesGui::PythonManager::python_finished()
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
GPlatesGui::PythonManager::python_runner_started()
{
	d_event_blackout.start();
	boost::function<QWidget* ()> f = 
		boost::bind(
				&GPlatesQtWidgets::PythonConsoleDialog::show_cancel_widget,
				d_python_console_dialog_ptr);
	QWidget* w = GPlatesApi::PythonUtils::run_in_main_thread(f);
	d_event_blackout.add_blackout_exemption(w);
}


void
GPlatesGui::PythonManager::python_runner_finished()
{
	d_event_blackout.stop();
	boost::function<QWidget* ()> f = 
		boost::bind(
				&GPlatesQtWidgets::PythonConsoleDialog::hide_cancel_widget,
				d_python_console_dialog_ptr);
	QWidget* w = GPlatesApi::PythonUtils::run_in_main_thread(f);
	d_event_blackout.remove_blackout_exemption(w);
}

void
GPlatesGui::PythonManager::print_py_msg(const QString& msg)
{
	d_python_console_dialog_ptr->append_text(msg);
}

#endif //GPLATES_NO_PYTHON













