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

#include <boost/optional.hpp>
#include <QObject>
#include <QEventLoop>
#include <QString>
#include <QMutex>
#include <QWaitCondition>


namespace GPlatesApi
{
	/**
	 * Provides a local event loop that runs parallel to a @a PythonExecutionThread
	 * and provides facilities for the safe interruption of the thread, if Python
	 * execution is indeed occurring on another thread. If Python execution is
	 * occurring on the main GUI thread, this monitor cannot interrupt execution,
	 * but it is still needed to provide a mechanism by which the results of the
	 * execution can be communicated back to the caller.
	 *
	 * It is assumed that instances of this class live on the main GUI thread.
	 *
	 * This class does not provide a mechanism by which the user can interrupt the
	 * thread; this is intended to be provided for by a subclass.
	 */
	class PythonExecutionMonitor :
			public QObject
	{
		Q_OBJECT

	public:

		/**
		 * Constructs a @a PythonExecutionMonitor that monitors the execution of a
		 * @a PythonExecutionThread with the given @a python_thread_id.
		 */
		explicit
		PythonExecutionMonitor(
				long python_thread_id);

		/**
		 * If the thread was executing interactive input from a console, returns
		 * whether more input is required before the command can be executed.
		 */
		bool
		continue_interactive_input() const;

		/**
		 * Starts the local event loop. Returns true if execution completed
		 * successfully without interruption, and false if execution finished
		 * because it was interrupted.
		 *
		 * Note that it is important that no events be allowed to be processed
		 * between the time when Python execution thread is given a job and the
		 * call to @a exec(); otherwise, it is possible that we miss the
		 * termination of our event loop by the execution thread. In particular,
		 * do not call @a QCoreApplication::processEvents() in between.
		 */
		virtual
		bool
		exec();

		/**
		 * Stops the local event loop, after the execution thread has finished running
		 * an interactive command.
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
		 * Stops the local event loop, after the execution thread has finished
		 * execution (of anything other than an interactive command).
		 *
		 * Note: this function is thread-safe. Regardless of which thread this
		 * function is called from, an event is posted to this object for later
		 * processing, and the function returns immediately without waiting for the
		 * event to have been processed.
		 */
		void
		signal_exec_finished();

		/**
		 * Notifies this monitor that Python code raised a SystemExit exception.
		 * Such an exception normally immediately quits the application, but it
		 * is expected that if this function is called, the caller has
		 * suppressed this default behaviour and has left it up to this monitor
		 * to deal with the SystemExit exception as it sees fit.
		 *
		 * Note: this function is thread-safe. If this function is called from a thread
		 * that is not the thread in which this object lives, an event is posted to this
		 * object, and the function blocks until the event has been processed.
		 */
		void
		signal_system_exit_exception_raised(
				int exit_status,
				const boost::optional<QString> &error_message);

	signals:

		/**
		 * Emitted when the execution thread finishes the execution of an interactive
		 * command.
		 */
		void
		exec_interactive_command_finished(
				bool continue_interactive_input_);

		/**
		 * Emitted when the execution thread finishes the execution of non-interactive
		 * Python code.
		 */
		void
		exec_finished();

		/**
		 * Emitted when the execution thread finishes the current execution or
		 * evaluation. This is a catch-all signal to avoid having to listen to all the
		 * above signals.
		 */
		void
		exec_or_eval_finished();

		/**
		 * Emitted when a SystemExit exception is raised in Python code.
		 */
		void
		system_exit_exception_raised(
				int exit_status,
				const boost::optional<QString> &error_message);

	protected:

		/**
		 * Sends the Python thread a @a KeyboardInterrupt exception.
		 */
		void
		interrupt_python_thread();

	private:

		void
		handle_exec_interactive_command_finished(
				bool continue_interactive_input_);

		void
		handle_exec_finished();

		void
		handle_system_exit_exception_raised(
				int exit_status,
				const boost::optional<QString> &error_message);

		void
		handle_system_exit_exception_raised_async(
				int exit_status,
				const boost::optional<QString> &error_message);

		long d_python_thread_id;
		bool d_was_interrupted;
		bool d_continue_interactive_input;
		QEventLoop d_event_loop;

		// For use if @a signal_system_exit_exception_raised was called from
		// outside our home thread.
		QMutex d_system_exit_mutex;
		QWaitCondition d_system_exit_condition;
	};
}

#endif  // GPLATES_API_PYTHONEXECUTIONMONITOR_H
