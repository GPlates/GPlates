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

#ifndef GPLATES_API_PYTHONEXECUTIONMONITOR_H
#define GPLATES_API_PYTHONEXECUTIONMONITOR_H

#include <QEventLoop>
#include <QString>
#include <QMutex>
#include <QWaitCondition>

#include "global/GPlatesException.h"
#include "global/python.h"
#include "utils/CallStackTracker.h"

namespace GPlatesApi
{
	class PythonExecutionMonitorNotInMainGUIThread : 
		public GPlatesGlobal::Exception
	{
	public:
		PythonExecutionMonitorNotInMainGUIThread(
			const GPlatesUtils::CallStack::Trace &exception_source) :
		GPlatesGlobal::Exception(exception_source)
		{ }
		//GPLATES_EXCEPTION_SOURCE
	protected:
		const char *
			exception_name() const
		{
			return "PythonExecutionMonitorNotInMainGUIThread";
		}

		void
			write_message(
			std::ostream &os) const
		{
			os << "The Python Execution Monitor must live in main GUI thread.";
		}
	};

	/**
	 * Provides a local event loop that runs parallel to a @a PythonExecutionThread
	 * and allows the main GUI thread to remain responsive while waiting for
	 * execution of Python code to finish.
	 *
	 * It is assumed that instances of this class live on the main GUI thread.
	 */
	class PythonExecutionMonitor : public QObject
	{
		Q_OBJECT

	public:

		/**
		 * An enumeration of reasons why the execution or evaluation finished.
		 */
		enum FinishReason
		{
			SUCCESS,

			KEYBOARD_INTERRUPT_EXCEPTION,
			SYSTEM_EXIT_EXCEPTION,
			OTHER_EXCEPTION
		};

		/**
		 * Constructs a @a PythonExecutionMonitor.
		 */
		PythonExecutionMonitor();

		/**
		 * If we are monitoring the execution of interactive input from a console,
		 * returns whether more input is required before the command can be executed.
		 */
		bool
		continue_interactive_input() const;

		/**
		 * If we are monitoring an evaluation of a Python expression, returns the
		 * value of that expression if the evaluation was successful.
		 */
		const boost::python::object &
		get_evaluation_result() const
		{
			return d_evaluation_result;
		}

		/**
		 * Once execution or evaluation has finished, returns the reason why execution
		 * or evaluation finished.
		 */
		FinishReason
		get_finish_reason() const
		{
			return d_finish_reason;
		}

		/**
		 * Returns the exit status upon finishing execution or evaluation.
		 */
		int
		get_exit_status() const
		{
			return d_exit_status;
		}

		/**
		 * Returns the exit error message, if set. Returns empty string if no error
		 * message was set. Typically, the error message is set when an unhandled
		 * SystemExit exception was raised in Python code.
		 */
		const QString &
		get_exit_error_message() const
		{
			return d_exit_error_message;
		}

		/**
		 * Starts the local event loop. Returns the reason why execution or evaluation
		 * finished.
		 *
		 * Only call this if monitoring Python code running on another thread.
		 *
		 * Note that it is important that no events be allowed to be processed
		 * between the time when Python execution thread is given a job and the
		 * call to @a exec(); otherwise, it is possible that we miss the
		 * termination of our event loop by the execution thread. In particular,
		 * do not call @a QCoreApplication::processEvents() in between.
		 */
		FinishReason
		exec();

		// The following functions are intended for use by @a PythonRunner.

		/**
		 * Stops the local event loop, after an interactive command has finished
		 * executing.
		 *
		 * Note: this function is thread-safe. Regardless of which thread this
		 * function is called from, an event is posted to this object for later
		 * processing, and the function returns immediately without waiting for the
		 * event to have been processed.
		 */
		void
		signal_exec_interactive_command_finished(
				bool continue_interactive_input_);

		/**
		 * Stops the local event loop, after the execution (of anything other than an
		 * interactive command) has finished.
		 *
		 * Note: this function is thread-safe. Regardless of which thread this
		 * function is called from, an event is posted to this object for later
		 * processing, and the function returns immediately without waiting for the
		 * event to have been processed.
		 */
		void
		signal_exec_finished();

		/**
		 * Stops the local event loop, after the evaluation of a Python expression
		 * has finished.
		 *
		 * Note: this function is thread-safe. Regardless of which thread this
		 * function is called from, an event is posted to this object for later
		 * processing, and the function returns immediately without waiting for the
		 * event to have been processed.
		 */
		void
		signal_eval_finished(
				const boost::python::object &result);

		/**
		 * Sets the finish reason to be @a SYSTEM_EXIT_EXCEPTION, so that the
		 * caller of the Python code can work out how it finished.
		 *
		 * Note: this function is thread-safe. Regardless of which thread this
		 * function is called from, an event is posted to this object for later
		 * processing, and the function returns immediately without waiting for the
		 * event to have been processed.
		 */
		void
		set_system_exit_exception_raised(
				int exit_status,
				QString exit_error_message);

		/**
		 * Sets the finish reason to be @a KEYBOARD_INTERRUPT_EXCEPTION.
		 */
		void
		set_keyboard_interrupt_exception_raised();

		/**
		 * Sets the finish reason to be @a OTHER_EXCEPTION.
		 */
		void
		set_other_exception_raised();

	private Q_SLOTS:
		void
		handle_exec_interactive_command_finished(
				bool continue_interactive_input_);
	private:
		void
		handle_exec_finished();

		void
		handle_eval_finished(
				const boost::python::object &result);

		void
		handle_system_exit_exception_raised(
				int exit_status,
				QString exit_error_message);

		void
		stop_monitor();


		PythonExecutionMonitor(const PythonExecutionMonitor&);

		bool d_continue_interactive_input;

		boost::python::object d_evaluation_result;


		QEventLoop d_event_loop;

		FinishReason d_finish_reason;
		int d_exit_status;
		QString d_exit_error_message;
	};
}

#endif  // GPLATES_API_PYTHONEXECUTIONMONITOR_H
