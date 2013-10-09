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

#include "PyReconstructionTree.h"
#include "PythonConverterUtils.h"

#include "file-io/ErrorOpeningFileForReadingException.h"
#include "file-io/ErrorOpeningFileForWritingException.h"
#include "file-io/FileFormatNotSupportedException.h"

#include "global/AssertionFailureException.h"
#include "global/CompilerWarnings.h"
#include "global/PreconditionViolationError.h"
#include "global/python.h"

#include "maths/IndeterminateResultException.h"
#include "maths/IndeterminateArcRotationAxisException.h"
#include "maths/InvalidLatLonException.h"
#include "maths/MathematicalException.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/ViolatedUnitVectorInvariantException.h"

#include "property-values/GmlTimePeriod.h"


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

		/**
		 * Creates a new python exception that maps to C++ exception 'ExceptionType'.
		 *
		 * Instance can be passed to 'bp::register_exception_translator()'.
		 */
		explicit
		python_Exception(
				const char *python_exception_name,
				bp::object python_base_exception_type)
		{
			std::string scope_name = bp::extract<std::string>(bp::scope().attr("__name__"));
			std::string qualified_name = scope_name + "." + python_exception_name;

			// Create a new exception on the python side that maps to C++ exception 'ExceptionType'.
			d_python_exception_type = bp::object(bp::handle<>(
					// Returns a new reference so no need for 'bp::borrowed'...
					PyErr_NewException(
							const_cast<char*>(qualified_name.c_str()),
							python_base_exception_type.ptr(),
							0)));

			// Add the new exception name to the current scope (should be the 'pygplates' module).
			bp::scope().attr(python_exception_name) = d_python_exception_type;
		}

		/**
		 * Returns the exception type.
		 *
		 * This is useful if need exception Derived to inherit exception Base
		 * (which, for example, in turn inherits 'Exception') - can then specify the return
		 * value as the Base python exception type when creating python Derived exception.
		 */
		bp::object
		get_python_exception_type() const
		{
			return d_python_exception_type;
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

		/**
		 * The type of exception on the python side (this maps to C++ exception 'ExceptionType').
		 */
		bp::object d_python_exception_type;
	};


	/**
	 * Creates a python exception with class name @a python_exception_name for C++ 'ExceptionType'
	 * and registers an exception translator for it.
	 *
	 * The base class of the python exception is of type @a python_base_exception_type.
	 *
	 * Returns the *type* of the python exception created.
	 *
	 * If 'ExceptionType' is a base class of another exception then the returned type can be used
	 * when *later* exporting the derived exception type. This works out well because the order of
	 * registration of exception translators is important as the following example demonstrates...
	 * 'DerivedExceptionType' inherits from 'BaseExceptionType', so 'DerivedExceptionType' should be
	 * registered *after* 'BaseExceptionType' because according to the docs:
	 *   "Any subsequently-registered translators will be allowed to translate the exception earlier."
	 * ...and this ensures that a (thrown) exception 'DerivedExceptionType' will be translated to
	 * the python equivalent 'DerivedExceptionType' (instead of 'BaseExceptionType').
	 */
	template <class ExceptionType>
	bp::object
	export_exception(
			const char *python_exception_name,
			bp::object python_base_exception_type)
	{
		python_Exception<ExceptionType> python_exception(
				python_exception_name,
				python_base_exception_type);

		bp::register_exception_translator<ExceptionType>(python_exception);

		return python_exception.get_python_exception_type();
	}
}


void
export_exceptions()
{
	using namespace GPlatesApi;

	//
	// NOTE: We use the convention of replacing the "Exception" part of the C++ exception name with
	// "Error" for the python exception name (since the standard python exceptions end with "Error").
	//

	// The base class of all GPlates exceptions - enables python user to catch any GPlates exception.
	// And it, in turn, inherits from python's 'Exception'.
	const bp::object gplates_exception_type =
			export_exception<GPlatesGlobal::Exception>(
					"GPlatesError",
					bp::object(bp::handle<>(bp::borrowed(PyExc_Exception))));

	//
	// Direct subclasses of GPlatesGlobal::Exception.
	//
	export_exception<GPlatesGlobal::AssertionFailureException>(
			"AssertionFailureError",
			gplates_exception_type);
	export_exception<GPlatesFileIO::FileFormatNotSupportedException>(
			"FileFormatNotSupportedError",
			gplates_exception_type);
	export_exception<GPlatesFileIO::ErrorOpeningFileForReadingException>(
			"OpenFileForReadingError",
			gplates_exception_type);
	export_exception<GPlatesFileIO::ErrorOpeningFileForWritingException>(
			"OpenFileForWritingError",
			gplates_exception_type);

	//
	// PreconditionViolationError and direct subclasses.
	//
	const bp::object precondition_violation_exception_type =
			export_exception<GPlatesGlobal::PreconditionViolationError>(
					"PreconditionViolationError",
					gplates_exception_type);
	export_exception<GPlatesPropertyValues::GmlTimePeriod::BeginTimeLaterThanEndTimeException>(
			"GmlTimePeriodBeginTimeLaterThanEndTimeError",
			precondition_violation_exception_type);
	export_exception<DifferentAnchoredPlatesInReconstructionTreesException>(
			"DifferentAnchoredPlatesInReconstructionTreesError",
			precondition_violation_exception_type);
	export_exception<GPlatesMaths::IndeterminateArcRotationAxisException>(
			"IndeterminateArcRotationAxisError",
			precondition_violation_exception_type);
	export_exception<GPlatesMaths::InsufficientPointsForMultiPointConstructionError>(
			"InsufficientPointsForMultiPointConstructionError",
			precondition_violation_exception_type);
	export_exception<GPlatesMaths::InvalidPointsForPolygonConstructionError>(
			"InvalidPointsForPolygonConstructionError",
			precondition_violation_exception_type);
	export_exception<GPlatesMaths::InvalidPointsForPolylineConstructionError>(
			"InvalidPointsForPolylineConstructionError",
			precondition_violation_exception_type);
	export_exception<GPlatesMaths::InvalidLatLonException>(
			"InvalidLatLonError",
			precondition_violation_exception_type);

	//
	// MathematicalException and direct subclasses.
	//
	const bp::object mathematical_exception_type =
			export_exception<GPlatesMaths::MathematicalException>(
					"MathematicalError",
					gplates_exception_type);
	export_exception<GPlatesMaths::IndeterminateResultException>(
			"IndeterminateResultError",
			mathematical_exception_type);
	export_exception<GPlatesMaths::ViolatedUnitVectorInvariantException>(
			"ViolatedUnitVectorInvariantError",
			mathematical_exception_type);
}

#endif // GPLATES_NO_PYTHON
