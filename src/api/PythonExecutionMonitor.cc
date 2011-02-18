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
#include <QApplication>
#include <QThread>
#include <QDebug>

#include "PythonExecutionMonitor.h"

#include "PythonInterpreterLocker.h"

#include "utils/DeferredCallEvent.h"


GPlatesApi::PythonExecutionMonitor::PythonExecutionMonitor() :
	d_continue_interactive_input(false),
	d_finish_reason(SUCCESS),
	d_exit_status(0)
{  }


bool
GPlatesApi::PythonExecutionMonitor::continue_interactive_input() const
{
	return d_continue_interactive_input;
}


GPlatesApi::PythonExecutionMonitor::FinishReason
GPlatesApi::PythonExecutionMonitor::exec()
{
	d_event_loop.exec();
	return d_finish_reason;
}


void
GPlatesApi::PythonExecutionMonitor::signal_exec_interactive_command_finished(
		bool continue_interactive_input_)
{
	// This is necessary because the event loop must be stopped in the thread in
	// which it was created (assumed to be the GUI thread).
	GPlatesUtils::DeferCall<void>::defer_call(
			boost::bind(
				&PythonExecutionMonitor::handle_exec_interactive_command_finished,
				boost::ref(*this),
				continue_interactive_input_),
			QThread::currentThread() == qApp->thread());
}


void
GPlatesApi::PythonExecutionMonitor::signal_exec_finished()
{
	// This is necessary because the event loop must be stopped in the thread in
	// which it was created (assumed to be the GUI thread).
	GPlatesUtils::DeferCall<void>::defer_call(
			boost::bind(
				&PythonExecutionMonitor::handle_exec_finished,
				boost::ref(*this)),
			QThread::currentThread() == qApp->thread());
}


#if !defined(GPLATES_NO_PYTHON)
void
GPlatesApi::PythonExecutionMonitor::signal_eval_finished(
		const boost::python::object &result)
{
	// This is necessary because the event loop must be stopped in the thread in
	// which it was created (assumed to be the GUI thread).
	GPlatesUtils::DeferCall<void>::defer_call(
			boost::bind(
				&PythonExecutionMonitor::handle_eval_finished,
				boost::ref(*this),
				result),
			QThread::currentThread() == qApp->thread());
}


void
GPlatesApi::PythonExecutionMonitor::handle_eval_finished(
		const boost::python::object &result)
{
	d_evaluation_result = result;
	d_event_loop.quit();
}
#endif


void
GPlatesApi::PythonExecutionMonitor::set_system_exit_exception_raised(
		int exit_status,
		QString exit_error_message)
{
	// This is necessary to stop Qt complaining that it can't queue values of type
	// boost::optional<QString>. Let's just emit everything from the main thread.
	GPlatesUtils::DeferCall<void>::defer_call(
			boost::bind(
				&PythonExecutionMonitor::handle_system_exit_exception_raised,
				boost::ref(*this),
				exit_status,
				exit_error_message),
			QThread::currentThread() == qApp->thread());
}


void
GPlatesApi::PythonExecutionMonitor::handle_exec_interactive_command_finished(
		bool continue_interactive_input_)
{
	d_continue_interactive_input = continue_interactive_input_;
	d_event_loop.quit();
}


void
GPlatesApi::PythonExecutionMonitor::handle_exec_finished()
{
	d_event_loop.quit();
}


void
GPlatesApi::PythonExecutionMonitor::handle_system_exit_exception_raised(
		int exit_status,
		QString exit_error_message)
{
	d_finish_reason = SYSTEM_EXIT_EXCEPTION;
	d_exit_status = exit_status;
	d_exit_error_message = exit_error_message;
}


void
GPlatesApi::PythonExecutionMonitor::set_keyboard_interrupt_exception_raised()
{
	d_finish_reason = KEYBOARD_INTERRUPT_EXCEPTION;
}


void
GPlatesApi::PythonExecutionMonitor::set_other_exception_raised()
{
	d_finish_reason = OTHER_EXCEPTION;
}

