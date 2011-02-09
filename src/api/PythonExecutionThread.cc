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

#include <iostream>
#include <boost/bind.hpp>
#include <QMutexLocker>
#include <QCoreApplication>
#include <QDebug>

#include "PythonExecutionThread.h"

#include "PythonExecutionMonitor.h"
#include "PythonInterpreterLocker.h"
#include "PythonRunner.h"

#include "app-logic/ApplicationState.h"

#include "utils/DeferredCallEvent.h"


GPlatesApi::PythonExecutionThread::PythonExecutionThread(
		GPlatesAppLogic::ApplicationState &application_state,
		QObject *parent_) :
	QThread(parent_),
	d_application_state(application_state),
	d_python_runner(NULL),
	d_event_loop(NULL),
	d_python_thread_id(0)
{  }


#if !defined(GPLATES_NO_PYTHON)
void
GPlatesApi::PythonExecutionThread::exec_interactive_command(
		const QString &command,
		PythonExecutionMonitor *monitor)
{
	QMutexLocker lock(&d_state_mutex);
	if (d_python_runner != NULL)
	{
		emit exec_or_eval_started();
		listen_to(monitor);
		QCoreApplication::postEvent(
				d_python_runner,
				new GPlatesUtils::DeferredCallEvent(
					boost::bind(
						&PythonRunner::exec_interactive_command,
						boost::ref(*d_python_runner),
						command,
						monitor)));
	}
}


void
GPlatesApi::PythonExecutionThread::reset_interactive_buffer()
{
	QMutexLocker lock(&d_state_mutex);
	if (d_python_runner != NULL)
	{
		QCoreApplication::postEvent(
				d_python_runner,
				new GPlatesUtils::DeferredCallEvent(
					boost::bind(
						&PythonRunner::reset_interactive_buffer,
						boost::ref(*d_python_runner))));
	}
}


void
GPlatesApi::PythonExecutionThread::exec_string(
		const QString &string,
		PythonExecutionMonitor *monitor)
{
	QMutexLocker lock(&d_state_mutex);
	if (d_python_runner != NULL)
	{
		emit exec_or_eval_started();
		listen_to(monitor);
		QCoreApplication::postEvent(
				d_python_runner,
				new GPlatesUtils::DeferredCallEvent(
					boost::bind(
						&PythonRunner::exec_string,
						boost::ref(*d_python_runner),
						string,
						monitor)));
	}
}


void
GPlatesApi::PythonExecutionThread::exec_file(
		const QString &filename,
		PythonExecutionMonitor *monitor,
		const QString &filename_encoding)
{
	QMutexLocker lock(&d_state_mutex);
	if (d_python_runner != NULL)
	{
		emit exec_or_eval_started();
		listen_to(monitor);
		QCoreApplication::postEvent(
				d_python_runner,
				new GPlatesUtils::DeferredCallEvent(
					boost::bind(
						&PythonRunner::exec_file,
						boost::ref(*d_python_runner),
						filename,
						monitor,
						filename_encoding)));
	}
}
#endif


void
GPlatesApi::PythonExecutionThread::quit_event_loop()
{
	QMutexLocker lock(&d_state_mutex);
	if (d_event_loop != NULL)
	{
		d_event_loop->quit();
	}
}


long
GPlatesApi::PythonExecutionThread::get_python_thread_id() const
{
	QMutexLocker lock(&d_state_mutex);
	return d_python_thread_id;
}


void
GPlatesApi::PythonExecutionThread::run()
{
	d_state_mutex.lock();

	QEventLoop event_loop;
	d_event_loop = &event_loop;
#if !defined(GPLATES_NO_PYTHON)
	d_python_runner = new PythonRunner(d_application_state);

	// Get the Python thread id for the current thread.
	using namespace boost::python;
	const object &main_namespace = d_application_state.get_python_main_namespace();
	PythonInterpreterLocker interpreter_locker;
	try
	{
		if (PyRun_SimpleString("import thread"))
		{
			throw error_already_set();
		}
		d_python_thread_id = extract<long>(eval("thread.get_ident()", main_namespace, main_namespace));
		if (PyRun_SimpleString("del thread"))
		{
			throw error_already_set();
		}
	}
	catch (const error_already_set &)
	{
		PyErr_Print();
	}
	interpreter_locker.release();
#endif

	d_state_mutex.unlock();

	event_loop.exec();

	d_state_mutex.lock();
	
	d_python_thread_id = 0;
	delete d_python_runner;
	d_python_runner = NULL;
	d_event_loop = NULL;

	d_state_mutex.unlock();
}


void
GPlatesApi::PythonExecutionThread::handle_monitor_exec_or_eval_finished()
{
	emit exec_or_eval_finished();
}


void
GPlatesApi::PythonExecutionThread::listen_to(
		PythonExecutionMonitor *monitor)
{
	QObject::connect(
			monitor,
			SIGNAL(exec_or_eval_finished()),
			this,
			SLOT(handle_monitor_exec_or_eval_finished()));
}

