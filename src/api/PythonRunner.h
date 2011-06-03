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

#include <QString>
#include <QObject>

#include "AbstractPythonRunner.h"

#include "global/python.h"


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
#if !defined(GPLATES_NO_PYTHON)
		explicit
		PythonRunner(
				GPlatesAppLogic::ApplicationState &application_state,
				const boost::python::object &,
				QObject *parent_ = NULL);
#endif
		virtual
		~PythonRunner();

#if !defined(GPLATES_NO_PYTHON)
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
				PythonExecutionMonitor *monitor,
				const QString &filename_encoding);

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
#endif

	signals:

		void
		exec_or_eval_started();

		void
		exec_or_eval_finished();

		void
		system_exit_exception_raised(
				int exit_status,
				QString exit_error_message);

	protected:

		virtual
		bool
		event(
				QEvent *ev);

	private:

#if !defined(GPLATES_NO_PYTHON)
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

		GPlatesAppLogic::ApplicationState &d_application_state;
		boost::python::object d_console;
		boost::python::object d_compile;
		boost::python::object d_eval;
#endif
	};

}

#endif  // GPLATES_API_PYTHONRUNNER_H
