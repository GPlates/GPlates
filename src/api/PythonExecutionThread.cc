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
#include <QCoreApplication>
#include <QDebug>
#include <QMetaType>
#include <QMutexLocker>

#include "PythonExecutionThread.h"

#include "PythonInterpreterLocker.h"
#include "PythonRunner.h"

#include "api/PythonUtils.h"
#include "api/Sleeper.h"

#include "app-logic/ApplicationState.h"

#include "global/python.h"  // PY_MAJOR_VERSION

#include "gui/PythonManager.h"

#include "utils/DeferredCallEvent.h"


GPlatesApi::PythonExecutionThread::PythonExecutionThread(
		const  boost::python::object &main_namespace,
		QObject *parent_) :
	QThread(parent_),
	d_namespace(main_namespace),
	d_python_runner(NULL),
	d_event_loop(NULL),
	d_python_thread_id(0)
{  }


void
GPlatesApi::PythonExecutionThread::exec_interactive_command(
		const QString &command)
{
	check_python_runner();

	boost::function< void () > f = 
		boost::bind(
				&PythonRunner::exec_interactive_command,
				d_python_runner,
				command,
				&d_monitor);

	run_in_python_thread(f);
}


void
GPlatesApi::PythonExecutionThread::reset_interactive_buffer()
{
	check_python_runner();

	boost::function< void () > f = 
		boost::bind(
				&PythonRunner::reset_interactive_buffer,
				d_python_runner);

	run_in_python_thread(f);
}


void
GPlatesApi::PythonExecutionThread::exec_string(
		const QString &string)
{
	check_python_runner();

	boost::function< void () > f = 
		boost::bind(
				&PythonRunner::exec_string,
				d_python_runner,
				string,
				&d_monitor);

	run_in_python_thread(f);
}


void
GPlatesApi::PythonExecutionThread::exec_file(
		const QString &filename,
		const QString &filename_encoding)
{
	check_python_runner();

	boost::function< void () > f = 
		boost::bind(
				&PythonRunner::exec_file,
				d_python_runner,
				filename,
				filename_encoding,
				&d_monitor);

	run_in_python_thread(f);
}


void
GPlatesApi::PythonExecutionThread::eval_string(
		const QString &string)
{
	check_python_runner();

	boost::function< void () > f = 
		boost::bind(
				&PythonRunner::eval_string,
				d_python_runner,
				string,
				&d_monitor);

	run_in_python_thread(f);
}


void
GPlatesApi::PythonExecutionThread::exec_function(
		const boost::function< void () > &function)
{
	check_python_runner();

	boost::function< void () > f = 
		boost::bind(
				&PythonRunner::exec_function,
				d_python_runner,
				function,
				&d_monitor);

	run_in_python_thread(f);
}


void
GPlatesApi::PythonExecutionThread::eval_function(
		const boost::function< boost::python::object () > &function)
{
	check_python_runner();

	boost::function< void () > f = 
		boost::bind(
				&PythonRunner::eval_function,
				d_python_runner,
				function,
				&d_monitor);

	run_in_python_thread(f);
}


void
GPlatesApi::PythonExecutionThread::quit_event_loop()
{
	if (d_event_loop != NULL)
	{
		d_event_loop->quit();
	}
}


long
GPlatesApi::PythonExecutionThread::get_python_thread_id() const
{
	return d_python_thread_id;
}


void
GPlatesApi::PythonExecutionThread::raise_keyboard_interrupt_exception()
{
	PythonInterpreterLocker interpreter_locker;
	PyThreadState_SetAsyncExc(d_python_thread_id, PyExc_KeyboardInterrupt);
}


void
GPlatesApi::PythonExecutionThread::run()
{
	QEventLoop event_loop;
	d_event_loop = &event_loop;
	d_python_runner = new PythonRunner(d_namespace);

	// Get the Python thread id for the current thread.
	using namespace boost::python;
	{
		PythonInterpreterLocker interpreter_locker;
		try
		{
			// Python 3 renamed module 'thread' to '_thread' (and added a higher-level API 'threading' on top).
#if PY_MAJOR_VERSION >= 3
			PyRun_SimpleString("import _thread");
			d_python_thread_id = extract<long>(eval("_thread.get_ident()", d_namespace, d_namespace));
			PyRun_SimpleString("del _thread");
#else
			PyRun_SimpleString("import thread");
			d_python_thread_id = extract<long>(eval("thread.get_ident()", d_namespace, d_namespace));
			PyRun_SimpleString("del thread");
#endif
		}
		catch (const error_already_set &)
		{
			qWarning() << GPlatesApi::PythonUtils::get_error_message();
		}
	}

	event_loop.exec();

	d_python_thread_id = 0;
	delete d_python_runner;
	d_python_runner = NULL;
	d_event_loop = NULL;
}


//
// We're compiling qRegisterMetaType<type>() (ie, without a string argument) below in CC file.
// And qRegisterMetaType<type>() expects Q_DECLARE_METATYPE(typename), so we declare that here.
//
// We avoid compiling qRegisterMetaType<type>(typename) (ie, *with* a string argument) in the header file
// since qRegisterMetaType<type>(typename) declares QMetaTypeId<type> (in Qt5). And there might be a
// CC file that includes the header but also declares Q_DECLARE_METATYPE(typename)
// (presumably because it uses  'type' in a QVariant) which in turn declares QMetaTypeId<type>.
// This duplicate declaration of QMetaTypeId<type> would result in a compile error.
//
// We could compile qRegisterMetaType(typename) below in CC file and not declare Q_DECLARE_METATYPE(typename).
// That should achieve the same result.
//
Q_DECLARE_METATYPE(boost::function< void() >)

void
GPlatesApi::PythonExecutionThread::run_in_python_thread(
		boost::function< void () > &f)
{
	if(PythonUtils::is_main_thread())
	{
		PythonUtils::ThreadSwitchGuard g;
		qRegisterMetaType<boost::function< void () > >();
		QMetaObject::invokeMethod(
				d_python_runner, 
				"exec_function_slot", 
				Qt::AutoConnection,
				Q_ARG(boost::function< void () > , f));
		wait_done();
	}
}
