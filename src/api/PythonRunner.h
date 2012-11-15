/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_API_PYTHONRUNNER_H
#define GPLATES_API_PYTHONRUNNER_H

#ifndef Q_MOC_RUN //workaround. Qt moc doesn't like BOOST_JOIN. Make them not seeing each other.

#include <boost/any.hpp>
#include <QString>
#include <QObject>

#include "AbstractPythonRunner.h"
#include "api/PythonUtils.h"
#include "global/python.h"

#endif //Q_MOC_RUN

#if !defined(GPLATES_NO_PYTHON)
namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesApi
{
	/**
	 * PythonRunner executes Python code in the same thread as the caller, or if
	 * posted a DeferredCallEvent, in the thread of its creation.
	 */
	class PythonRunner :
			public QObject,
			public AbstractPythonRunner
	{
		Q_OBJECT

	public:

		PythonRunner(
				const boost::python::object &,
				QObject *parent_ = NULL);
		virtual
		~PythonRunner();

		virtual
		void
		exec_interactive_command(
				const QString &line,
				PythonExecutionMonitor *monitor);

		virtual
		void
		exec_string(
				const QString &string,
				PythonExecutionMonitor *monitor);

		virtual
		void
		reset_interactive_buffer();

		virtual
		void
		exec_file(
				const QString &filename,
				const QString &filename_encoding,
				PythonExecutionMonitor *monitor);

		virtual
		void
		eval_string(
				const QString &string,
				PythonExecutionMonitor *monitor);

		virtual
		void
		exec_function(
				const boost::function< void () > &function,
				PythonExecutionMonitor *monitor);

		virtual
		void
		eval_function(
				const boost::function< boost::python::object () > &function,
				PythonExecutionMonitor *monitor);
		
	signals:
		void
		system_exit_exception_raised(
				int exit_status,
				QString exit_error_message);

	protected:
		void
		python_started()
		{
			GPlatesApi::PythonUtils::python_manager().python_runner_started();
		}

		void
		python_finished()
		{
			GPlatesApi::PythonUtils::python_manager().python_runner_finished();
		}

		class PythonExecGuard
		{
		public:
			PythonExecGuard(
					PythonRunner* runner) :
			d_runner(runner)
			{
				d_runner->python_started();
			}

			~PythonExecGuard()
			{
				d_runner->python_finished();
			}
		private:
			PythonRunner * d_runner;
		};

		friend class PythonExecGuard;

	public slots:
		void
		exec_function_slot(
				const boost::function< void () > &f)
		{
			PythonExecGuard g(this);
			f();
		}

	protected:

		virtual
		bool
		event(
				QEvent *ev);

	private:
		/**
		 * Resets the interactive console's buffer.
		 */
		void
		reset_buffer();

		/**
		 * Handles the occurrence of an exception during Python execution.
		 */
		void
		handle_exception(
				PythonExecutionMonitor *monitor);

		/**
		 * Handle the SystemExit exception, which can be raised explicitly or
		 * via quit() or sys.exit().
		 */
		void
		handle_system_exit(
				PythonExecutionMonitor *monitor);

		const boost::python::object& d_namespace;
		boost::python::object d_console;
		boost::python::object d_compile;
		boost::python::object d_eval;
	};
}
#endif //GPLATES_NO_PYTHON
#endif // GPLATES_API_PYTHONRUNNER_H



