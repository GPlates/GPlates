/* $Id: CommandLineParser.h 7795 2010-03-16 04:26:54Z jcannon $ */

/**
* \file
* File specific comments.
*
* Most recent change:
*   $Date: 2010-03-16 15:26:54 +1100 (Tue, 16 Mar 2010) $
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
		void initialize();
		bool is_initialized(){return d_inited;}
		~PythonManager();
		
		static
		PythonManager*
		instance()
		{
			//static variable will be initialized only once.
			static PythonManager* inst = new PythonManager();
			return inst;
		}

		/*
		* This function is a little bit tricky.
		* The boost python object could exist in embedded python interpreter. 
		* When we destruct object in python interpreter, the python GIL must be obtained.
		* However, boost::python::object is actually a "smart pointer". We don't exactly know when it will be acturally destructed.
		* Instead of acquiring python GIL all over the source code, we do it in this function.
		* Put all python object which is not needed anymore in this recycle bin.
		*/
		void
		recycle_python_object(const boost::python::object& obj);


		void
		init_python_interpreter(
				std::string program_name = "gplates");

		void
		init_python_console();

		void
		pop_up_python_console();

		void 
		register_utils_scripts();

		QFileInfoList
		get_scripts();

		void
		register_script(const QString& name);

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


		/*
		* Validate the python home setting in preference,
		* and try the best to find a valid python installation.
		* If a good python has been found, set the PYTHONHOME env variable.
		* On Windows, GPlates needs to restart to make env variable effective.
		*/
		void
		set_python_home();


		void
		find_python();
		

		void
		set_show_init_fail_dlg(bool b) 
		{
			d_show_python_init_fail_dlg = b;
		}


		void
		set_python_home(const QString& str) 
		{
			d_python_home = str;
		}


		void
		python_runner_started();

		void
		python_runner_finished();

		void
		print_py_msg(const QString& msg);

	signals:
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
			qDebug() << path_of_code_py;
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

	public slots:
		
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

		std::vector<QDir> d_scripts_paths;

		/**
		 * If true, we stopped the event blackout temporarily because the PythonRunner
		 * started to run something on the main thread.
		*/
		bool d_stopped_event_blackout_for_python_runner;
	
		/**
		 * Lock down the user interface during Python execution.
		*/
		GPlatesGui::EventBlackout d_event_blackout;

		bool d_show_python_init_fail_dlg;

		QString d_python_home;
		
		QString d_python_version;
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
		void initialize(GPlatesPresentation::Application& app){app.get_viewport_window().hide_python_menu();}
		void pop_up_python_console(){}
	};
}
#endif    //GPLATES_NO_PYTHON
#endif    //GPLATES_UTILS_PYTHON_MANAGER_H


