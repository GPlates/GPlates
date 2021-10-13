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

#include "utils/StringUtils.h"


GPlatesApi::ConsoleReader::ConsoleReader(
		AbstractConsole *console) :
	d_console(console)
{
#if !defined(GPLATES_NO_PYTHON)
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
		PyErr_Print();
	}
#endif
}


GPlatesApi::ConsoleReader::~ConsoleReader()
{
#if !defined(GPLATES_NO_PYTHON)
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
		PyErr_Print();
	}
#endif
}


#if !defined(GPLATES_NO_PYTHON)
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
	d_console->append_text(result);
	interpreter_unlocker.restore_thread();

	try
	{
		object unicode(GPlatesUtils::make_wstring_from_qstring(result));
		return unicode.attr("encode")("utf-8", "replace"); // FIXME: hard coded codec
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
#endif

