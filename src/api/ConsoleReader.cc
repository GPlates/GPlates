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

#include "ConsoleReader.h"

#include "PythonInterpreterLocker.h"
#include "PythonInterpreterUnlocker.h"

#include "api/PythonUtils.h"

GPlatesApi::ConsoleReader::ConsoleReader(
		AbstractConsole *console) :
	d_console(console)
{
	using namespace boost::python;
	PythonInterpreterLocker interpreter_locker;
	try
	{
		// Save the old stdin before we replace it, so we can restore it later.
		object sys_module = import("sys");
		d_old_object = sys_module.attr("stdin");

		// Replace stdin with ourselves.
		sys_module.attr("stdin") = ptr(this);
	}
	catch (const error_already_set &)
	{
		std::cerr << "Could not replace Python's sys.stdin." << std::endl;
		qWarning() << GPlatesApi::PythonUtils::get_error_message();
	}
}


GPlatesApi::ConsoleReader::~ConsoleReader()
{
	using namespace boost::python;
	PythonInterpreterLocker interpreter_locker;
	try
	{
		// Restore the old stdin.
		object sys_module = import("sys");
		sys_module.attr("stdin") = d_old_object;
		d_old_object = object();
	}
	catch (const error_already_set &)
	{
		std::cerr << "Could not restore Python's sys.stdin." << std::endl;
		qWarning() << GPlatesApi::PythonUtils::get_error_message();
	}
}


boost::python::object
GPlatesApi::ConsoleReader::readline()
{
	using namespace boost::python;

	PythonInterpreterLocker interpreter_locker;
	QString result;

	// Note that even if we called interpreter_locker.release(), this thread may
	// still have the GIL because of the presence of a PythonInterpreterLocker
	// further up the call stack.
	PythonInterpreterUnlocker interpreter_unlocker;
	result = d_console->read_line();

	// Echo the input out to the console.
	//
	// Note that an empty result means end-of-file (the user cancelled). There is
	// nothing to echo, but we still terminate the prompt line Python has written.
	d_console->append_text(result.isEmpty() ? QString("\n") : result);
	interpreter_unlocker.restore_thread();

	try
	{
		// Use the registered QString conversion (see PyStrings.cc), which returns a
		// Python 'str' in both Python 2 (encoded as UTF-8) and Python 3 (Unicode).
		//
		// Note that Python 3 must not be given 'bytes' here, because that silently
		// breaks callers that compare the line against 'str' literals. In particular
		// the interactive 'help()' utility could then never be exited, since pydoc's
		// test for "q"/"quit" never matched, and this left its modal readline dialog
		// reappearing indefinitely.
		return object(result);
	}
	catch (const error_already_set &)
	{
		// This should be a fail-safe conversion.
		return str(result.toStdString());
	}
}


void
export_console_reader()
{
	using namespace boost::python;

	class_<GPlatesApi::ConsoleReader, boost::noncopyable>("GPlatesConsoleReader")
		.def("readline", &GPlatesApi::ConsoleReader::readline);
}
