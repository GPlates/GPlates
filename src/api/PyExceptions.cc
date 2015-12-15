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

#include "PyExceptions.h"

#include "PyFeature.h"
#include "PyGeometriesOnSphere.h"
#include "PyGreatCircleArc.h"
#include "PyInformationModel.h"
#include "PyInterpolationException.h"
#include "PyPlatePartitioner.h"
#include "PyReconstructionTree.h"
#include "PythonConverterUtils.h"

#include "file-io/ErrorOpeningFileForReadingException.h"
#include "file-io/ErrorOpeningFileForWritingException.h"
#include "file-io/FileFormatNotSupportedException.h"

#include "global/AbortException.h"
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
#include "maths/UnableToNormaliseZeroVectorException.h"
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
	class PythonException
	{
	public:

		/**
		 * Creates a new python exception that maps to C++ exception 'ExceptionType'.
		 *
		 * Instance can be passed to 'bp::register_exception_translator()'.
		 */
		explicit
		PythonException(
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
		PythonException<ExceptionType> python_exception(
				python_exception_name,
				python_base_exception_type);

		bp::register_exception_translator<ExceptionType>(python_exception);

		return python_exception.get_python_exception_type();
	}


	//
	// GPlates C++ exceptions translated to python errors.
	//

	bp::object AbortError;
	bp::object AmbiguousGeometryCoverageError;
	bp::object AssertionFailureError;
	bp::object DifferentAnchoredPlatesInReconstructionTreesError;
	bp::object DifferentTimesInPartitioningPlatesError;
	bp::object FileFormatNotSupportedError;
	bp::object GeometryTypeError;
	bp::object GmlTimePeriodBeginTimeLaterThanEndTimeError;
	bp::object GPlatesError;
	bp::object IndeterminateArcRotationAxisError;
	bp::object IndeterminateGreatCircleArcDirectionError;
	bp::object IndeterminateGreatCircleArcNormalError;
	bp::object IndeterminateResultError;
	bp::object InformationModelError;
	bp::object InsufficientPointsForMultiPointConstructionError;
	bp::object InterpolationError;
	bp::object InvalidLatLonError;
	bp::object InvalidPointsForPolygonConstructionError;
	bp::object InvalidPointsForPolylineConstructionError;
	bp::object MathematicalError;
	bp::object OpenFileForReadingError;
	bp::object OpenFileForWritingError;
	bp::object PreconditionViolationError;
	bp::object UnableToNormaliseZeroVectorError;
	bp::object ViolatedUnitVectorInvariantError;
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
	GPlatesApi::GPlatesError =
			export_exception<GPlatesGlobal::Exception>(
					"GPlatesError",
					bp::object(bp::handle<>(bp::borrowed(PyExc_Exception))));

	//
	// Direct subclasses of GPlatesGlobal::Exception.
	//
	GPlatesApi::AbortError =
			export_exception<GPlatesGlobal::AbortException>(
					"AbortError",
					GPlatesApi::GPlatesError);
	GPlatesApi::AssertionFailureError =
			export_exception<GPlatesGlobal::AssertionFailureException>(
					"AssertionFailureError",
					GPlatesApi::GPlatesError);
	GPlatesApi::FileFormatNotSupportedError =
			export_exception<GPlatesFileIO::FileFormatNotSupportedException>(
					"FileFormatNotSupportedError",
					GPlatesApi::GPlatesError);
	GPlatesApi::OpenFileForReadingError =
			export_exception<GPlatesFileIO::ErrorOpeningFileForReadingException>(
					"OpenFileForReadingError",
					GPlatesApi::GPlatesError);
	GPlatesApi::OpenFileForWritingError =
			export_exception<GPlatesFileIO::ErrorOpeningFileForWritingException>(
					"OpenFileForWritingError",
					GPlatesApi::GPlatesError);

	//
	// PreconditionViolationError and direct subclasses.
	//
	GPlatesApi::PreconditionViolationError =
			export_exception<GPlatesGlobal::PreconditionViolationError>(
					"PreconditionViolationError",
					GPlatesApi::GPlatesError);
	GPlatesApi::AmbiguousGeometryCoverageError =
			export_exception<GPlatesApi::AmbiguousGeometryCoverageException>(
					"AmbiguousGeometryCoverageError",
					GPlatesApi::PreconditionViolationError);
	GPlatesApi::DifferentAnchoredPlatesInReconstructionTreesError =
			export_exception<GPlatesApi::DifferentAnchoredPlatesInReconstructionTreesException>(
					"DifferentAnchoredPlatesInReconstructionTreesError",
					GPlatesApi::PreconditionViolationError);
	GPlatesApi::DifferentTimesInPartitioningPlatesError =
			export_exception<GPlatesApi::DifferentTimesInPartitioningPlatesException>(
					"DifferentTimesInPartitioningPlatesError",
					GPlatesApi::PreconditionViolationError);
	GPlatesApi::GeometryTypeError =
			export_exception<GPlatesApi::GeometryTypeException>(
					"GeometryTypeError",
					GPlatesApi::PreconditionViolationError);
	GPlatesApi::GmlTimePeriodBeginTimeLaterThanEndTimeError =
			export_exception<GPlatesPropertyValues::GmlTimePeriod::BeginTimeLaterThanEndTimeException>(
					"GmlTimePeriodBeginTimeLaterThanEndTimeError",
					GPlatesApi::PreconditionViolationError);
	GPlatesApi::IndeterminateArcRotationAxisError =
			export_exception<GPlatesMaths::IndeterminateArcRotationAxisException>(
					"IndeterminateArcRotationAxisError",
					GPlatesApi::PreconditionViolationError);
	GPlatesApi::IndeterminateGreatCircleArcDirectionError =
			export_exception<GPlatesApi::IndeterminateGreatCircleArcDirectionException>(
					"IndeterminateGreatCircleArcDirectionError",
					GPlatesApi::PreconditionViolationError);
	GPlatesApi::IndeterminateGreatCircleArcNormalError =
			export_exception<GPlatesApi::IndeterminateGreatCircleArcNormalException>(
					"IndeterminateGreatCircleArcNormalError",
					GPlatesApi::PreconditionViolationError);
	GPlatesApi::InformationModelError =
			export_exception<GPlatesApi::InformationModelException>(
					"InformationModelError",
					GPlatesApi::PreconditionViolationError);
	GPlatesApi::InsufficientPointsForMultiPointConstructionError =
			export_exception<GPlatesMaths::InsufficientPointsForMultiPointConstructionError>(
					"InsufficientPointsForMultiPointConstructionError",
					GPlatesApi::PreconditionViolationError);
	GPlatesApi::InterpolationError =
			export_exception<GPlatesApi::InterpolationException>(
					"InterpolationError",
					GPlatesApi::PreconditionViolationError);
	GPlatesApi::InvalidPointsForPolygonConstructionError =
			export_exception<GPlatesMaths::InvalidPointsForPolygonConstructionError>(
					"InvalidPointsForPolygonConstructionError",
					GPlatesApi::PreconditionViolationError);
	GPlatesApi::InvalidPointsForPolylineConstructionError =
			export_exception<GPlatesMaths::InvalidPointsForPolylineConstructionError>(
					"InvalidPointsForPolylineConstructionError",
					GPlatesApi::PreconditionViolationError);
	GPlatesApi::InvalidLatLonError =
			export_exception<GPlatesMaths::InvalidLatLonException>(
					"InvalidLatLonError",
					GPlatesApi::PreconditionViolationError);

	//
	// MathematicalException and direct subclasses.
	//
	GPlatesApi::MathematicalError =
			export_exception<GPlatesMaths::MathematicalException>(
					"MathematicalError",
					GPlatesApi::GPlatesError);
	GPlatesApi::IndeterminateResultError =
			export_exception<GPlatesMaths::IndeterminateResultException>(
					"IndeterminateResultError",
					GPlatesApi::MathematicalError);
	GPlatesApi::UnableToNormaliseZeroVectorError =
			export_exception<GPlatesMaths::UnableToNormaliseZeroVectorException>(
					"UnableToNormaliseZeroVectorError",
					GPlatesApi::MathematicalError);
	GPlatesApi::ViolatedUnitVectorInvariantError =
			export_exception<GPlatesMaths::ViolatedUnitVectorInvariantException>(
					"ViolatedUnitVectorInvariantError",
					GPlatesApi::MathematicalError);
}


GPlatesApi::PythonExceptionHandler::PythonExceptionHandler() :
	d_restore_exception(false)
{
	PyErr_Fetch(&d_type, &d_value, &d_traceback);

	// Make sure the error condition was set prior to this constructor.
	// If it wasn't set then all three PyObject's will be NULL.
	// In particular, if the error condition was set, then the exception type can never be NULL.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_type,
			GPLATES_ASSERTION_SOURCE);
}


// For Py_XDECREF below.
DISABLE_GCC_WARNING("-Wold-style-cast")

GPlatesApi::PythonExceptionHandler::~PythonExceptionHandler()
{
	if (d_restore_exception)
	{
		// Transfer ownership of PyObject's back to python.
		PyErr_Restore(d_type, d_value, d_traceback);
	}
	else
	{
		// We have ownership of PyObject's so make sure reference counts get decremented.
		Py_XDECREF(d_type);
		Py_XDECREF(d_value);
		Py_XDECREF(d_traceback);
	}
}

ENABLE_GCC_WARNING("-Wold-style-cast")


bool
GPlatesApi::PythonExceptionHandler::exception_matches(
		PyObject *exception) const
{
	return PyErr_GivenExceptionMatches(d_type, exception);
}


void
GPlatesApi::PythonExceptionHandler::restore_exception()
{
	// Make sure the error status gets restored (in the destructor)
	// when we throw the error_already_set exception.
	d_restore_exception = true;
	bp::throw_error_already_set();
}


QString
GPlatesApi::PythonExceptionHandler::get_exception_message() const
{
	bp::object exception_value = get_exception_value();

	bp::extract<QString> extract_message(exception_value);
	if (!extract_message.check())
	{
		return QString();
	}

	return extract_message();
}


bp::object
GPlatesApi::PythonExceptionHandler::get_exception_type() const
{
	// Exception type is not a NULL PyObject pointer.
	return bp::object(bp::handle<>(bp::borrowed(d_type)));
}


bp::object
GPlatesApi::PythonExceptionHandler::get_exception_value() const
{
	// Could be a NULL PyObject pointer.
	bp::handle<> value(bp::borrowed(bp::allow_null(d_value)));
	if (!value)
	{
		return bp::object(); // Py_None
	}

	return bp::object(value);
}


bp::object
GPlatesApi::PythonExceptionHandler::get_exception_traceback() const
{
	// Could be a NULL PyObject pointer.
	bp::handle<> traceback(bp::borrowed(bp::allow_null(d_traceback)));
	if (!traceback)
	{
		return bp::object(); // Py_None
	}

	return bp::object(traceback);
}

#endif // GPLATES_NO_PYTHON
