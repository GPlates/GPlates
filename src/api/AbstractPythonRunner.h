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

#ifndef GPLATES_API_ABSTRACTPYTHONRUNNER_H
#define GPLATES_API_ABSTRACTPYTHONRUNNER_H

#include <boost/function.hpp>
#include <QString>

#include "global/python.h"


namespace GPlatesApi
{
	// Forward declaration.
	class PythonExecutionMonitor;

	/**
	 * @a AbstractPythonRunner provides an interface to execute Python code in
	 * various ways, monitored by a @a PythonExecutionMonitor instance.
	 *
	 * Note that it is up to the concrete implementations to define whether Python
	 * code is run on a separate thread, or indeed if the public member functions
	 * are thread-safe.
	 */
	class AbstractPythonRunner
	{
	public:

		virtual
		~AbstractPythonRunner()
		{  }

#if !defined(GPLATES_NO_PYTHON)
		/**
		 * Executes @a command as entered on an interactive console.
		 *
		 * The @a command is converted into a Python unicode object for execution.
		 *
		 * @a monitor must not be NULL. At the conclusion of execution, whether
		 * Python is expecting more input is returned to the caller via @a monitor.
		 */
		virtual
		void
		exec_interactive_command(
				const QString &command,
				PythonExecutionMonitor *monitor) = 0;

		/**
		 * Resets the buffer in the interactive console (e.g. when the user presses
		 * Ctrl+C in the console).
		 */
		virtual
		void
		reset_interactive_buffer() = 0;

		/**
		 * Executes the Python code contained in @a string.
		 *
		 * The @a string is converted into a Python unicode object for execution.
		 *
		 * @a monitor must not be NULL.
		 */
		virtual
		void
		exec_string(
				const QString &string,
				PythonExecutionMonitor *monitor) = 0;

		/**
		 * Executes @a filename as a Python script, monitored from another thread by
		 * @a monitor.
		 *
		 * The file is read from disk in text mode (so that newline characters are, on
		 * all platforms, converted to "\n" as Python expects) but otherwise, no
		 * decoding is performed. If the file contains non-ASCII text, the encoding of
		 * the file must be specified using a special comment at the top of the file:
		 * see http://www.python.org/dev/peps/pep-0263/
		 *
		 * The @a filename is encoded using the @a filename_encoding; this encoded
		 * version is what appears in tracebacks/syntax error messages. In most cases,
		 * you will want the @a filename_encoding to be the encoding used by the
		 * console on which stderr appears, otherwise the filename will appear as
		 * jibberish.
		 *
		 * @a monitor must not be NULL.
		 */
		virtual
		void
		exec_file(
				const QString &filename,
				PythonExecutionMonitor *monitor,
				const QString &filename_encoding) = 0;

		/**
		 * Evaluates the Python expression contained in @a string.
		 *
		 * The @a command is converted into a Python unicode object for evaluation.
		 *
		 * @a monitor must not be NULL. At the conclusion of evaluation, the result of
		 * evaluation is returned to the caller via @a monitor.
		 */
		virtual
		void
		eval_string(
				const QString &string,
				PythonExecutionMonitor *monitor) = 0;

		/**
		 * Executes the given @a function.
		 *
		 * @a monitor must not be NULL.
		 */
		virtual
		void
		exec_function(
				const boost::function< void () > &function,
				PythonExecutionMonitor *monitor) = 0;

		/**
		 * Evaluates the given @a function, which returns a Python object.
		 *
		 * @a monitor must not be NULL. At the conclusion of evaluation, the result of
		 * evaluation is returned to the caller via @a monitor.
		 */
		virtual
		void
		eval_function(
				const boost::function< boost::python::object () > &function,
				PythonExecutionMonitor *monitor) = 0;
#endif
	};
}

#endif  // GPLATES_API_ABSTRACTPYTHONRUNNER_H
