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

#include "ConsoleWriter.h"

#include "PythonInterpreterLocker.h"
#include "PythonInterpreterUnlocker.h"
#include "PythonUtils.h"

#if !defined(GPLATES_NO_PYTHON)
namespace
{
	const char *
	get_stream_name(
			bool error)
	{
		static const char *STDOUT_NAME = "stdout";
		static const char *STDERR_NAME = "stderr";

		return error ? STDERR_NAME : STDOUT_NAME;
	}
}


GPlatesApi::ConsoleWriter::ConsoleWriter(
		bool error,
		AbstractConsole *console) :
	d_error(error),
	d_console(console)
{
	using namespace boost::python;
	const char *stream_name = get_stream_name(error);

	PythonInterpreterLocker interpreter_locker;
	try
	{
		// Save the old stdout/stderr before we replace it, so we can restore it later.
		object sys_module = import("sys");
		d_old_object = sys_module.attr(stream_name);

		// Replace stdout/stderr with ourselves.
		sys_module.attr(stream_name) = ptr(this);
	}
	catch (const error_already_set &)
	{
		std::cerr << "Could not replace Python's sys." << stream_name << "." << std::endl;
		qWarning() << GPlatesApi::PythonUtils::get_error_message();
	}
}


GPlatesApi::ConsoleWriter::~ConsoleWriter()
{
	using namespace boost::python;
	const char *stream_name = get_stream_name(d_error);

	PythonInterpreterLocker interpreter_locker;
	try
	{
		// Restore the old stdout/stderr.
		object sys_module = import("sys");
		sys_module.attr(stream_name) = d_old_object;
		d_old_object = object();
	}
	catch (const error_already_set &)
	{
		std::cerr << "Could not restore Python's sys." << stream_name << "." << std::endl;
		qWarning() << GPlatesApi::PythonUtils::get_error_message();
	}
}



void
GPlatesApi::ConsoleWriter::write(
		const boost::python::object &text)
{
	using namespace boost::python;

	try
	{
		// We must first guarantee that we have the lock before attempting to unlock it.
		PythonInterpreterLocker interpreter_locker;
		PythonInterpreterUnlocker interpreter_unlocker;
		d_console->append_text(text, d_error);
	}
	catch (const error_already_set &)
	{
		std::cerr << "Failed to write to console" << std::endl;
		// Do not call @a PyErr_Print() here because that would attempt to write to
		// sys.stderr, which would end up calling this, and... you get the idea.
		qWarning() << GPlatesApi::PythonUtils::get_error_message();
	}
}


void
export_console_writer()
{
	using namespace boost::python;

	class_<GPlatesApi::ConsoleWriter, boost::noncopyable>("GPlatesConsoleWriter")
		.def("write", &GPlatesApi::ConsoleWriter::write);
}
#endif //GPLATES_NO_PYTHON

