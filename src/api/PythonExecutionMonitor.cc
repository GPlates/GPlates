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
#include "api/PythonUtils.h"

GPlatesApi::PythonExecutionMonitor::PythonExecutionMonitor() :
	d_continue_interactive_input(false),
	d_finish_reason(SUCCESS),
	d_exit_status(0)
{ 
	if(this->thread() != qApp->thread())
		throw PythonExecutionMonitorNotInMainGUIThread(GPLATES_EXCEPTION_SOURCE);
}


bool
GPlatesApi::PythonExecutionMonitor::continue_interactive_input() const
{
	return d_continue_interactive_input;
}


GPlatesApi::PythonExecutionMonitor::FinishReason
GPlatesApi::PythonExecutionMonitor::exec()
{
	if(d_event_loop.isRunning()) 
	// this means we are trying to re-enter python thread. 
	{
		qWarning() << "The PythonExecutionMonitor is already running.";
		qWarning() << "It seems GPlates is trying to re-enter python thread.";
	}
	d_event_loop.exec();
	return d_finish_reason;
}


void
GPlatesApi::PythonExecutionMonitor::signal_exec_interactive_command_finished(
		bool continue_interactive_input_)
{
	// This is necessary because the event loop must be stopped in the thread in
	// which it was created (assumed to be the GUI thread).
	boost::function<void ()> f = 
		boost::bind(
				&PythonExecutionMonitor::handle_exec_interactive_command_finished,
				this, 
				continue_interactive_input_);
	GPlatesApi::PythonUtils::run_in_main_thread(f);
}


void
GPlatesApi::PythonExecutionMonitor::signal_exec_finished()
{
	// This is necessary because the event loop must be stopped in the thread in
	// which it was created (assumed to be the GUI thread).
	boost::function<void ()> f = 
		boost::bind(
				&PythonExecutionMonitor::handle_exec_finished,
				this);
	GPlatesApi::PythonUtils::run_in_main_thread(f);
}


void
GPlatesApi::PythonExecutionMonitor::signal_eval_finished(
		const boost::python::object &result)
{
	// This is necessary because the event loop must be stopped in the thread in
	// which it was created (assumed to be the GUI thread).
	boost::function<void ()> f = 
		boost::bind(
				&PythonExecutionMonitor::handle_eval_finished,
				this,
				result);
	GPlatesApi::PythonUtils::run_in_main_thread(f);
}


void
GPlatesApi::PythonExecutionMonitor::handle_eval_finished(
		const boost::python::object &result)
{
	d_evaluation_result = result;
	stop_monitor();
}



void
GPlatesApi::PythonExecutionMonitor::set_system_exit_exception_raised(
		int exit_status,
		QString exit_error_message)
{
	// This is necessary to stop Qt complaining that it can't queue values of type
	// boost::optional<QString>. Let's just emit everything from the main thread.
	boost::function<void ()> f = 
		boost::bind(
				&PythonExecutionMonitor::handle_system_exit_exception_raised,
				this,
				exit_status,
				exit_error_message);
	GPlatesApi::PythonUtils::run_in_main_thread(f);
}


void
GPlatesApi::PythonExecutionMonitor::handle_exec_interactive_command_finished(
		bool continue_interactive_input_)
{
	d_continue_interactive_input = continue_interactive_input_;
	stop_monitor();
}


void
GPlatesApi::PythonExecutionMonitor::handle_exec_finished()
{
	stop_monitor();
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

void
GPlatesApi::PythonExecutionMonitor::stop_monitor()
{
	unsigned c = 0;
	while(!d_event_loop.isRunning())
	{
		QThread::currentThread()->wait(100);
		c++;
		if(c > 10)
		{
			qWarning() << "The event loop in PythonExecutionMonitor is not running after";
			qWarning() << " Waiting for a reasonable time. Quit anyway. Consequence unknown.";
			break;
		}
	}
	d_event_loop.quit();
}
