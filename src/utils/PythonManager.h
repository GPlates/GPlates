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
#ifndef GPLATES_UTILS_PYTHON_MANAGER_H
#define GPLATES_UTILS_PYTHON_MANAGER_H
#include <boost/scoped_ptr.hpp>
#include "global/GPlatesException.h"
#include "global/python.h"

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

namespace GPlatesUtils
{
	class PyManagerNotReady : GPlatesGlobal::Exception
	{
	public:
		PyManagerNotReady(
			const GPlatesUtils::CallStack::Trace &exception_source) :
		GPlatesGlobal::Exception(exception_source)
		{ }
		//GPLATES_EXCEPTION_SOURCE
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

	class PythonManager : public QObject
	{
	public:
		void initialize(GPlatesAppLogic::ApplicationState&);
		bool is_initialized(){return d_inited;}
		PythonManager() : d_inited(false){}
		~PythonManager();

#if !defined(GPLATES_NO_PYTHON)
		const boost::python::object &
		get_python_main_module() const
		{
			return d_python_main_module;
		}

		const boost::python::object &
		get_python_main_namespace() const
		{
			return d_python_main_namespace;
		}
#endif

		/**
		 * Returns an object that runs Python on the main thread.
		 */
		GPlatesApi::PythonRunner *
		get_python_runner() const
		{
			check_init();
			return d_python_runner;
		}

		/**
		 * Returns a thread on which Python code can be run off the main thread.
		 */
		GPlatesApi::PythonExecutionThread *
		get_python_execution_thread()
		{
			check_init();
			return d_python_execution_thread;
		}

	private:
		void
		check_init() const
		{
			if(!d_inited)
				throw PyManagerNotReady(GPLATES_EXCEPTION_SOURCE);
		}

#if !defined(GPLATES_NO_PYTHON)
		/**
		 * The "__main__" Python module.
		 */
		boost::python::object d_python_main_module;

		/**
		 * The "__dict__" attribute of the "__main__" Python module.
		 * This is useful for passing into exec() and eval() for context.
		 */
		boost::python::object d_python_main_namespace;
#endif
		/**
		 * Runs Python code on the main thread.
		 *
		 * Memory managed by Qt.
		 */
		GPlatesApi::PythonRunner *d_python_runner;

		/**
		 * The thread on which Python is executed, off the main thread.
		 *
		 * Memory is managed by Qt.
		 */
		GPlatesApi::PythonExecutionThread *d_python_execution_thread;

		/**
		 * Replaces Python's time.sleep() with our own implementation.
		 */
		boost::scoped_ptr< GPlatesApi::Sleeper > d_sleeper;

		bool d_inited;
	};
}
#endif    //GPLATES_UTILS_PYTHON_MANAGER_H


