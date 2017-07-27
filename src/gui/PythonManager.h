/* $Id$ */

/**
* \file
* File specific comments.
*
* Most recent change:
*   $Date$
*
* Copyright (C) 2010 The University of Sydney, Australia
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
#ifndef GPLATES_GUI_PYTHON_MANAGER_H
#define GPLATES_GUI_PYTHON_MANAGER_H

#include <QDir>
#include <QFileInfoList>
#include <QMap>
#include <QStringList>
#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include "global/GPlatesException.h"
#include "global/python.h"
#include "EventBlackout.h"

namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesApi
{
	class PythonRunner;
	class PythonExecutionThread;
	class Sleeper;
}

namespace GPlatesPresentation
{
	class Application;
}

namespace GPlatesQtWidgets
{
	class PythonConsoleDialog;
}

#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python ;

namespace GPlatesGui
{
	class PyManagerNotReady : 
		public GPlatesGlobal::Exception
	{
	public:
		PyManagerNotReady(
			const GPlatesUtils::CallStack::Trace &exception_source) :
		GPlatesGlobal::Exception(exception_source)
		{ }
	protected:
		const char *
		exception_name() const
		{
			return "Python Manager Uninitialized Exception";
		}

		void
		write_message(
				std::ostream &os) const
		{
			os << "The Python Manager has not been initialized yet.";
		}
	};

	class PythonInitFailed : 
		public GPlatesGlobal::Exception
	{
	public:
		PythonInitFailed(
				const GPlatesUtils::CallStack::Trace &exception_source) :
			GPlatesGlobal::Exception(exception_source)
		{ }
		//GPLATES_EXCEPTION_SOURCE
	protected:
		const char *
		exception_name() const
		{
			return "PythonInitFailed Exception";
		}

		void
		write_message(
				std::ostream &os) const
		{
			os << "Error occurred during python initialization.";
		}
	};


	class PythonManager : public QObject
	{
		Q_OBJECT

	public:
		void initialize(
				char* argv[],
				GPlatesPresentation::Application *app=NULL);

		bool is_initialized()
		{
			return d_inited;
		}
		
		~PythonManager();
		
		static
		PythonManager*
		instance()
		{
			//static variable will be initialized only once.
			static PythonManager* inst = new PythonManager();
			return inst;
		}


		void
		init_python_interpreter(
				char* argv[]);

		void
		init_python_console();

		void
		pop_up_python_console();

		void 
		register_utils_scripts();


		/**
		 * Returns a list of built-in scripts (stored internally in Qt resource files).
		 */
		QMap<QString/*module name*/, QString/*module filename*/>
		get_internal_scripts();

		/**
		 * Returns a unique list of scripts (based on their module name) found in the external file search paths.
		 */
		QMap<QString/*module name*/, QFileInfo/*module filename*/>
		get_external_scripts();


		/**
		 * Register an internal script (stored in Qt resource files).
		 *
		 * This excludes modules found in the external file search paths.
		 */
		void
		register_internal_script(
				const QString &internal_module_name,
				const QString &internal_module_filename);

		/**
		 * Register a script that was found in the external file search paths.
		 *
		 * This excludes internal modules (stored in Qt resource files).
		 *
		 * Note: The module file path must already have been added to 'sys.path'.
		 */
		void
		register_external_script(
				const QString &external_module_name);


		/**
		 * Returns a thread on which Python code can be run off the main thread.
		 */
		GPlatesApi::PythonExecutionThread *
		get_python_execution_thread()
		{
			check_init();
			return d_python_execution_thread;
		}

		
		bool
		show_init_fail_dlg() const
		{
			return d_show_python_init_fail_dlg;
		}

		
		const QString&
		python_version() const
		{
			return d_python_version;
		}


		const QString&
		python_home() const
		{
			return d_python_home;
		}


		void
		find_python();
		

		void
		set_show_init_fail_dlg(
				bool b);


		void
		set_python_prefix(
				const QString& str);


		void
		set_python_prefix();


		QString
		get_python_prefix_from_preferences();


		void
		python_runner_started();

		void
		python_runner_finished();

		void
		print_py_msg(
				const QString& msg);

	Q_SIGNALS:
		void
		system_exit_exception_raised(
				int exit_status,
				QString exit_error_message);

	protected:
		PythonManager() ;


		void
		check_python_capability();
		

		bool
		validate_python_home()
		{
			return validate_python_home(d_python_home);
		}

		bool
		validate_python_home(
				const QString& new_home)
		{
#ifndef __WINDOWS__ 
			QString path_of_code_py = 
				new_home + "/lib/python" +
				d_python_version + "/os.py"; //use this file as hint to find python home.
#else
			QString path_of_code_py = 
				new_home + "/Lib/os.py"; 
#endif
			//qDebug() << path_of_code_py;
			QFileInfo file_hint(path_of_code_py);
			return file_hint.isFile();
		}


		class PythonExecGuard
		{
		public:
			PythonExecGuard(
					PythonManager* pm) :
				d_pm(pm)
			{
				d_pm->python_started();
			}

			~PythonExecGuard()
			{
				d_pm->python_finished();
			}
		private:
			PythonManager * d_pm;
		};

		friend class PythonExecGuard;

	public Q_SLOTS:
		
		void
		exec_function_slot(
				const boost::function< void () > &f)
		{
			PythonExecGuard g(this);
			return f();
		}

	private:
		void
		python_started();

		void
		python_finished();
		
		void
		add_sys_path();
		/*
		* I wrote this function. But I don't like it.
		* It'd better if we init python in PythonManager constructor.
		* However, currently, we cannot do that.
		* Sort this out someday pls.
		*/
		void
		check_init() const
		{
			if(!d_inited)
				throw PyManagerNotReady(GPLATES_EXCEPTION_SOURCE);
		}

		/**
		 * The "__main__" Python module.
		 */
		boost::python::object d_python_main_module;

		/**
		 * The "__dict__" attribute of the "__main__" Python module.
		 * This is useful for passing into exec() and eval() for context.
		 */
		boost::python::object d_python_main_namespace;
		/**
		 * Runs Python code on the main thread.
		 *
		 * Memory managed by Qt.
		 */
		GPlatesApi::PythonRunner *d_python_main_thread_runner;

		/**
		 * The thread on which Python is executed, off the main thread.
		 *
		 * Memory is managed by Qt.
		 */
		GPlatesApi::PythonExecutionThread *d_python_execution_thread;

		/**
		 * Replaces Python's time.sleep() with our own implementation.
		 */
		GPlatesApi::Sleeper* d_sleeper;

		bool d_inited;

		/*
		* TODO: This dialog should be taken out from python manager -- MC.
		*/
		GPlatesQtWidgets::PythonConsoleDialog* d_python_console_dialog_ptr;

		/**
		 * Paths to external scripts.
		 */
		std::vector<QDir> d_external_scripts_paths;

		/**
		 * If true, we stopped the event blackout temporarily because the PythonRunner
		 * started to run something on the main thread.
		*/
		bool d_stopped_event_blackout_for_python_runner;
	
		/**
		 * Lock down the user interface during Python execution.
		*/
		GPlatesGui::EventBlackout d_event_blackout;

		bool d_show_python_init_fail_dlg, d_clear_python_prefix_flag;

		QString d_python_home;
		
		QString d_python_version;

		/*
		* Keep a pointer of GPlatesPresentation::Application here.
		* It is initialized when calling PythonManager::initialize().
		*/
		GPlatesPresentation::Application *d_application;
	};
}
#else
#include "presentation/Application.h"
namespace GPlatesGui
{
	class PythonManager : public QObject
	{
	public:
		static
		PythonManager*
		instance()
		{
			//static variable will be initialized only once.
			static PythonManager* inst = new PythonManager();
			return inst;
		}
		void initialize(GPlatesPresentation::Application& app){app.get_main_window().hide_python_menu();}
		void pop_up_python_console(){}
	};
}
#endif    //GPLATES_NO_PYTHON
#endif    //GPLATES_UTILS_PYTHON_MANAGER_H


