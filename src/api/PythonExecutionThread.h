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

#ifndef GPLATES_API_PYTHONEXECUTIONTHREAD_H
#define GPLATES_API_PYTHONEXECUTIONTHREAD_H

#include <boost/function.hpp>
#include <QThread>
#include <QEvent>
#include <QMutex>
#include <QEventLoop>
#include <QString>

#include "global/LogException.h"
#include "global/python.h"
#include "PythonExecutionMonitor.h"
#include "PythonUtils.h"
#include "PythonRunner.h"

#if !defined(GPLATES_NO_PYTHON)
namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesApi
{
	// Forward declaration.
	class PythonExecutionMonitor;
	class PythonRunner;

	/**
	 * The thread on which Python gets executed, away from the main thread.
	 */
	class PythonExecutionThread :
			public QThread
	{
		Q_OBJECT

	public:
		PythonExecutionThread(
				const  boost::python::object &main_namespace,
				QObject *parent_);

		/**
		 * Executes @a command as entered on an interactive console on this thread,
		 * monitored from another thread by @a monitor.
		 *
		 * The @a command is converted into a Python unicode object for execution.
		 *
		 * @a monitor must not be NULL. At the conclusion of execution, whether
		 * Python is expecting more input is returned to the called via @a monitor.
		 *
		 * Note: this function is thread-safe.
		 */
		virtual
		void
		exec_interactive_command(
				const QString &command);

		/**
		 * Resets the buffer in the interactive console (e.g. when the user presses
		 * Ctrl+C in the console).
		 *
		 * It is not possible to monitor the execution of the reset from another
		 * thread by means of a monitor object; the reset can be assumed to occur
		 * almost instantaneously.
		 *
		 * Note: this function is thread-safe.
		 */
		virtual
		void
		reset_interactive_buffer();

		/**
		 * Executes the Python code in @a string on this thread, monitored from another
		 * thread by @a monitor. This function should not be used with Python code
		 * that was entered from an interactive console; use
		 * @a exec_interactive_command instead.
		 *
		 * The @a string is converted into a Python unicode object for execution.
		 *
		 * @a monitor must not be NULL.
		 *
		 * Note: this function is thread-safe.
		 */
		virtual
		void
		exec_string(
				const QString &string);

		/**
		 * Executes @a filename as a Python script, monitored from another thread by
		 * @a monitor.
		 *
		 * The file is read from disk in text mode (so that newline characters are, on
		 * all platforms, converted to "\n" as Python expects) but otherwise, no
		 * decoding is performed. If the file contains non-ASCII text, the encoding of
		 * the file must be specified using a special comment at the top of the file:
		 * see http://www.python.org/dev/peps/pep-0263/
		 *
		 * The @a filename is encoded using the @a filename_encoding; this encoded
		 * version is what appears in tracebacks/syntax error messages. In most cases,
		 * you will want the @a filename_encoding to be the encoding used by the
		 * console on which stderr appears, otherwise the filename will appear as
		 * jibberish.
		 *
		 * @a monitor must not be NULL.
		 *
		 * Note: this function is thread-safe.
		 */
		virtual
		void
		exec_file(
				const QString &filename,
				const QString &filename_encoding);

		/**
		 * Evaluates the Python expression contained in @a string, monitored from
		 * another thread by @a monitor.
		 *
		 * The @a command is converted into a Python unicode object for evaluation.
		 *
		 * @a monitor must not be NULL. At the conclusion of evaluation, the result of
		 * evaluation is returned to the caller via @a monitor.
		 *
		 * Note: this function is thread-safe.
		 */
		virtual
		void
		eval_string(
				const QString &string);

		/**
		 * Executes the given @a function, monitored from another thread by @a monitor.
		 *
		 * @a monitor must not be NULL.
		 *
		 * Note: this function is thread-safe.
		 */
		virtual
		void
		exec_function(
				const boost::function< void () > &function);

		/**
		 * Evaluates the given @a function, which returns a Python object, monitored
		 * from another thread by @a monitor.
		 *
		 * @a monitor must not be NULL. At the conclusion of evaluation, the result of
		 * evaluation is returned to the caller via @a monitor.
		 *
		 * Note: this function is thread-safe.
		 */
		virtual
		void
		eval_function(
				const boost::function< boost::python::object () > &function);


		/**
		 * Quit the event loop, if it is running.
		 *
		 * Note: this function is thread-safe.
		 */
		void
		quit_event_loop();

		/**
		 * Returns the thread id as reported by Python, if the thread is running. If
		 * the thread is not running, returns 0.
		 *
		 * Note: this function is thread-safe.
		 */
		long
		get_python_thread_id() const;

		/**
		 * Raises a Python KeyboardInterrupt exception in the Python thread, if it is
		 * running. This will typically interrupt execution of any currently running
		 * Python code, in a safe manner.
		 *
		 * Note: this function is thread-safe.
		 */
		void
		raise_keyboard_interrupt_exception();

		
		bool
		continue_interactive_input()
		{
			return d_monitor.continue_interactive_input();
		}

	signals:
		/**
		 * Emitted when an unhandled Python SystemExit exception is raised in the thread.
		 */
		void
		system_exit_exception_raised(
				int exit_status,
				QString exit_error_message);

	protected:

		virtual
		void
		run();

		void
		check_python_runner()
		{
			if(!d_python_runner)
			{
				throw GPlatesGlobal::LogException(
						GPLATES_EXCEPTION_SOURCE,
						"Python Runner has not been initialized yet.");
			}

		}

		void
		run_in_python_thread(
				boost::function< void () > &f)
		{
			if(PythonUtils::is_main_thread())
			{
				PythonUtils::ThreadSwitchGuard g;
				qRegisterMetaType<boost::function< void () > >("boost::function< void () >");
				QMetaObject::invokeMethod(
						d_python_runner, 
						"exec_function_slot", 
						Qt::AutoConnection,
						Q_ARG(boost::function< void () > , f));
				wait_done();
			}
		}

		void
		wait_done()
		{
			d_monitor.exec();
		}

	private slots:

		void
		handle_system_exit_exception_raised(
				int exit_status,
				QString exit_error_message)
		{
			emit system_exit_exception_raised(exit_status, exit_error_message);
		}

	private:
		const  boost::python::object & d_namespace;
		PythonRunner *d_python_runner;
		QEventLoop *d_event_loop;
		long d_python_thread_id;
		PythonExecutionMonitor d_monitor;
	};
}
#endif   //GPLATES_NO_PYTHON
#endif  // GPLATES_API_PYTHONEXECUTIONTHREAD_H
