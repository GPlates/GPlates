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
#include <QFile>
#include <QByteArray>

#include "PythonRunner.h"

#include "PythonExecutionMonitor.h"
#include "PythonInterpreterLocker.h"

#include "app-logic/ApplicationState.h"

#include "global/CompilerWarnings.h"

#include "utils/DeferredCallEvent.h"
#include "utils/StringUtils.h"


GPlatesApi::PythonRunner::PythonRunner(
		GPlatesAppLogic::ApplicationState &application_state) :
	d_application_state(application_state)
{
#if !defined(GPLATES_NO_PYTHON)
	using namespace boost::python;

	PythonInterpreterLocker interpreter_locker;
	const object &main_namespace = application_state.get_python_main_namespace();

	try
	{
		PyRun_SimpleString("import code");
		d_console = eval("code.InteractiveConsole(locals())", main_namespace, main_namespace);
		PyRun_SimpleString("del code");
	}
	catch (const error_already_set &)
	{
		std::cerr << "Could not create code.InteractiveConsole instance." << std::endl;
		PyErr_Print();
	}

	try
	{
		object builtin_module = import("__builtin__");
		d_compile = builtin_module.attr("compile");
	}
	catch (const error_already_set &)
	{
		std::cerr << "Could not obtain reference to __builtin__.compile function." << std::endl;
		PyErr_Print();
	}
#endif
}


GPlatesApi::PythonRunner::~PythonRunner()
{
#if !defined(GPLATES_NO_PYTHON)
	PythonInterpreterLocker interpreter_locker;
	d_console = boost::python::object();
	d_compile = boost::python::object();
#endif
}


bool
GPlatesApi::PythonRunner::event(
		QEvent *ev)
{
	if (ev->type() == GPlatesUtils::DeferredCallEvent::TYPE)
	{
		GPlatesUtils::DeferredCallEvent *deferred_call_event =
			static_cast<GPlatesUtils::DeferredCallEvent *>(ev);
		deferred_call_event->execute();
	}
	else
	{
		return QObject::event(ev);
	}

	return true;
}


#if !defined(GPLATES_NO_PYTHON)
void
GPlatesApi::PythonRunner::exec_interactive_command(
		const QString &line,
		PythonExecutionMonitor *monitor)
{
	using namespace boost::python;

	PythonInterpreterLocker interpreter_locker;
	bool continue_interactive_input = false;
	try
	{
		std::wstring wstring = GPlatesUtils::make_wstring_from_qstring(line);
		continue_interactive_input = extract<bool>(d_console.attr("push")(wstring));
	}
	catch (const error_already_set &)
	{
		handle_exception(monitor);

		// Let's reset the console, just in case we interrupted the reset.
		if (!continue_interactive_input)
		{
			reset_buffer();
		}
	}
	interpreter_locker.release();

	monitor->signal_exec_interactive_command_finished(continue_interactive_input);
}


void
GPlatesApi::PythonRunner::exec_string(
		const QString &string,
		PythonExecutionMonitor *monitor)
{
	using namespace boost::python;

	PythonInterpreterLocker interpreter_locker;
	try
	{
		std::wstring wstring = GPlatesUtils::make_wstring_from_qstring(string);
		object code = d_compile(wstring, "<string>", "exec");
		d_console.attr("runcode")(code);
	}
	catch (const error_already_set &)
	{
		handle_exception(monitor);
	}
	interpreter_locker.release();

	monitor->signal_exec_finished();
}


void
GPlatesApi::PythonRunner::reset_interactive_buffer()
{
	PythonInterpreterLocker interpreter_locker;
	reset_buffer();
}


void
GPlatesApi::PythonRunner::exec_file(
		const QString &filename,
		PythonExecutionMonitor *monitor,
		const QString &filename_encoding)
{
	using namespace boost::python;

	QFile file(filename);
	if (file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		// Read the entire file into memory, but perform no decoding.
		QByteArray file_contents = file.readAll();

		// Python expects the file to end with a newline.
		file_contents.append("\n");

		// Encode the filename using the given encoding.
		PythonInterpreterLocker interpreter_locker;
		std::wstring wstring_filename = GPlatesUtils::make_wstring_from_qstring(filename);
		object unicode_filename;
		str encoded_filename;
		try
		{
			unicode_filename = object(wstring_filename);
			encoded_filename = str(unicode_filename.attr("encode")(
					filename_encoding.toUtf8().constData()));
		}
		catch (const error_already_set &)
		{
			try
			{
				encoded_filename = str(unicode_filename.attr("encode")("ascii", "replace"));
			}
			catch (const error_already_set &)
			{
				// We should never get here, but just in case...
				std::cerr << "Failed to encode filename." << std::endl;
				monitor->signal_exec_finished();
				return;
			}
		}

		try
		{
			object code = d_compile(file_contents.constData(), encoded_filename, "exec");
			d_console.attr("runcode")(code);
		}
		catch (const error_already_set &)
		{
			handle_exception(monitor);
		}

		monitor->signal_exec_finished();
	}
	else
	{
		monitor->signal_exec_finished();
	}
}


void
GPlatesApi::PythonRunner::reset_buffer()
{
	using namespace boost::python;

	// GIL assumed to have been acquired.
	try
	{
		d_console.attr("resetbuffer")();
	}
	catch (const error_already_set &)
	{
		PyErr_Print();
	}
}


void
GPlatesApi::PythonRunner::handle_exception(
		PythonExecutionMonitor *monitor)
{
	if (PyErr_ExceptionMatches(PyExc_SystemExit))
	{
		handle_system_exit(monitor);
	}
	else
	{
		PyErr_Print();
	}
}


// For Py_XDECREF below.
DISABLE_GCC_WARNING("-Wold-style-cast")

void
GPlatesApi::PythonRunner::handle_system_exit(
		PythonExecutionMonitor *monitor)
{
	using namespace boost::python;

	// GIL assumed to have been acquired.
	PyObject *type_ptr;
	PyObject *value_ptr;
	PyObject *traceback_ptr;
	PyErr_Fetch(&type_ptr, &value_ptr, &traceback_ptr);

	try
	{
		object value = object(handle<>(value_ptr));
		object code = value.attr("code");
		bool is_int = static_cast<bool>(PyInt_Check(code.ptr()));
		bool is_none = (code.ptr() == Py_None);

		int exit_status;
		boost::optional<QString> error_message;
		if (is_int)
		{
			// If the code is an int, it is the exit status.
			exit_status = extract<int>(code);
		}
		else if (is_none)
		{
			// If the code is None (not specified), the exit status defaults to 0 (success).
			exit_status = 0;
		}
		else
		{
			// Otherwise, the exit status is 1 (failure).
			// We then stringify the object and print it out.
			exit_status = 1;
			std::string std_error_message = extract<std::string>(str(value));
			PySys_WriteStdout("%s\n", std_error_message.c_str());
			error_message = QString::fromStdString(std_error_message);
		}

		// We own the references to these objects but they're never managed by Boost.
		Py_XDECREF(type_ptr);
		Py_XDECREF(traceback_ptr);

		// Give the monitor a chance to handle it.
		monitor->signal_system_exit_exception_raised(exit_status, error_message);
	}
	catch (const error_already_set &)
	{
		// This shouldn't happen.
		return;
	}

	PyErr_Clear();
}

// See above.
ENABLE_GCC_WARNING("-Wold-style-cast")
#endif

