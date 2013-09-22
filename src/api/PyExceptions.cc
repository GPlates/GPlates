/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#include <sstream>
#include <string>

#include "PythonConverterUtils.h"

#include "file-io/ErrorOpeningFileForReadingException.h"
#include "file-io/ErrorOpeningFileForWritingException.h"
#include "file-io/FileFormatNotSupportedException.h"

#include "global/AssertionFailureException.h"
#include "global/CompilerWarnings.h"
#include "global/python.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	/**
	 * Creates a new python exception of a given name (inheriting from python's 'Exception' by default)
	 * and translates C++ exceptions of type 'ExceptionType' into the python exception of given name.
	 *
	 * By inheriting from 'Exception' *on the python side* this enables python users to catch
	 * our (C++ thrown) exceptions with:
	 *   try:
	 *       ...
	 *   except Exception:
	 *       ...
	 *
	 * The exception gets translated at the C++ / python boundary by boost-python.
	 * 'ExceptionType' should (indirectly) inherit from 'GPlatesGlobal::Exception'.
	 *
	 * By default our GPlates C++ exceptions are translated to python's 'RuntimeError' exception with
	 * a string message from 'exc.what()' - if they inherit (indirectly) from 'std::exception'
	 * (which they all should do) - otherwise they get translated to 'RuntimeError' with a message of
	 * "unidentifiable C++ exception".
	 * So we only need to explicitly register exceptions that we don't want translated to 'RuntimeError'.
	 * This is usually an exception we want the python user to be able to catch as a specific error,
	 * as opposed to 'RuntimeError' which could be caused by anything really. For example:
	 *
	 *   try:
	 *       feature_collection_file_format_registry.read(filename)
	 *   except pygplates.FileFormatNotSupportedError:
	 *       # Handle unrecognised file format.
	 *       ...
	 *
	 * More information on the implementation can be found at:
	 *   http://stackoverflow.com/questions/11448735/boostpython-export-custom-exception-and-inherit-from-pythons-exception?lq=1
	 *   https://mail.python.org/pipermail/cplusplus-sig/2003-July/004459.html
	 */
	template <class ExceptionType>
	class python_Exception
	{
	public:

		static
		void
		register_exception_translator(
				const char *python_exception_name,
				PyObject* python_base_exception_type)
		{
			bp::register_exception_translator<ExceptionType>(
					python_Exception(python_exception_name, python_base_exception_type));
		}

		/**
		 * The translate call from boost-python as it goes through its list of registered handlers.
		 */
		void
		operator()(
				const ExceptionType &exc) const
		{
			// Extract the message
			std::ostringstream exc_message_stream;
			exc.write(exc_message_stream);

			PyErr_SetString(d_python_exception_type.ptr(), exc_message_stream.str().c_str());
		}

	private:

		explicit
		python_Exception(
				const char *python_exception_name,
				PyObject* python_base_exception_type)
		{
			std::string scope_name = bp::extract<std::string>(bp::scope().attr("__name__"));
			std::string qualified_name = scope_name + "." + python_exception_name;

			// Create a new exception on the python side that maps to C++ exception 'ExceptionType'.
			d_python_exception_type = bp::object(bp::handle<>(
					// Returns a new reference so no need for 'bp::borrowed'...
					PyErr_NewException(
							const_cast<char*>(qualified_name.c_str()),
							python_base_exception_type,
							0)));

			// Add the new exception name to the current scope (eg, module).
			bp::scope().attr(python_exception_name) = d_python_exception_type;
		}


		/**
		 * The type of exception on the python side (this maps to C++ exception 'ExceptionType').
		 */
		bp::object d_python_exception_type;
	};


	/**
	 * Register an exception translator for 'ExceptionType' with boost-python.
	 *
	 * As described above, this also creates a python-side exception that maps to 'ExceptionType'
	 * and inherits from the python exception @a python_base_exception_type which defaults
	 * to python's 'Exception'.
	 */
	template <class ExceptionType>
	void
	export_exception(
			const char *python_exception_name,
			PyObject* python_base_exception_type = PyExc_Exception)
	{
		python_Exception<ExceptionType>::register_exception_translator(
				python_exception_name,
				python_base_exception_type);
	}
}


void
export_exceptions()
{
	using namespace GPlatesApi;

	//
	// NOTE: If exception Derived inherits from exception Base (and both are registered) then
	// exception Derived should be registered *after* exception Base because according to the docs:
	//   "Any subsequently-registered translators will be allowed to translate the exception earlier."
	//

	//
	// NOTE: We use the convention of replacing the "Exception" part of the C++ exception name with
	// "Error" for the python exception name (since the standard python exceptions end with "Error").
	//
	export_exception<GPlatesFileIO::FileFormatNotSupportedException>("FileFormatNotSupportedError");
	export_exception<GPlatesFileIO::ErrorOpeningFileForReadingException>("OpenFileForReadingError");
	export_exception<GPlatesFileIO::ErrorOpeningFileForWritingException>("OpenFileForWritingError");
	// Inherit from python's 'AssertionError'.
	export_exception<GPlatesGlobal::AssertionFailureException>("AssertionFailureError", PyExc_AssertionError);
}

#endif // GPLATES_NO_PYTHON
