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

#include <boost/bind.hpp>
#include <QEvent>
#include <QCoreApplication>
#include <QThread>

#include "PythonExecutionMonitor.h"

#include "PythonInterpreterLocker.h"

#include "global/python.h"

#include "utils/DeferredCallEvent.h"


GPlatesApi::PythonExecutionMonitor::PythonExecutionMonitor(
		long python_thread_id) :
	d_python_thread_id(python_thread_id),
	d_was_interrupted(false),
	d_continue_interactive_input(false)
{  }


bool
GPlatesApi::PythonExecutionMonitor::continue_interactive_input() const
{
	return d_continue_interactive_input;
}


bool
GPlatesApi::PythonExecutionMonitor::exec()
{
	d_event_loop.exec();
	return d_was_interrupted;
}


void
GPlatesApi::PythonExecutionMonitor::signal_exec_interactive_command_finished(
		bool continue_interactive_input_)
{
	GPlatesUtils::DeferredCallEvent::defer_call(
			boost::bind(
				&PythonExecutionMonitor::handle_exec_interactive_command_finished,
				boost::ref(*this),
				continue_interactive_input_));
}


void
GPlatesApi::PythonExecutionMonitor::signal_exec_finished()
{
	GPlatesUtils::DeferredCallEvent::defer_call(
			boost::bind(
				&PythonExecutionMonitor::handle_exec_finished,
				boost::ref(*this)));
}


void
GPlatesApi::PythonExecutionMonitor::signal_system_exit_exception_raised(
		int exit_status,
		const boost::optional<QString> &error_message)
{
	if (QThread::currentThread() == thread())
	{
		handle_system_exit_exception_raised(exit_status, error_message);
	}
	else
	{
		d_system_exit_mutex.lock();
		GPlatesUtils::DeferredCallEvent::defer_call(
				boost::bind(
					&PythonExecutionMonitor::handle_system_exit_exception_raised_async,
					boost::ref(*this),
					exit_status,
					error_message));
		d_system_exit_condition.wait(&d_system_exit_mutex);
		d_system_exit_mutex.unlock();
	}
}


void
GPlatesApi::PythonExecutionMonitor::interrupt_python_thread()
{
#if !defined(GPLATES_NO_PYTHON)
	PythonInterpreterLocker interpreter_locker;
	PyThreadState_SetAsyncExc(d_python_thread_id, PyExc_KeyboardInterrupt);
	d_was_interrupted = true;
#endif
}


void
GPlatesApi::PythonExecutionMonitor::handle_exec_interactive_command_finished(
		bool continue_interactive_input_)
{
	d_event_loop.quit();
	d_continue_interactive_input = continue_interactive_input_;
	emit exec_interactive_command_finished(d_continue_interactive_input);
	emit exec_or_eval_finished();
}


void
GPlatesApi::PythonExecutionMonitor::handle_exec_finished()
{
	d_event_loop.quit();
	emit exec_finished();
	emit exec_or_eval_finished();
}


void
GPlatesApi::PythonExecutionMonitor::handle_system_exit_exception_raised(
		int exit_status,
		const boost::optional<QString> &error_message)
{
	emit system_exit_exception_raised(exit_status, error_message);
}


void
GPlatesApi::PythonExecutionMonitor::handle_system_exit_exception_raised_async(
		int exit_status,
		const boost::optional<QString> &error_message)
{
	handle_system_exit_exception_raised(exit_status, error_message);
	d_system_exit_mutex.lock();
	d_system_exit_condition.wakeAll();
	d_system_exit_mutex.unlock();
}

