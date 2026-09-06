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
		 * @a python_exception_doc becomes the docstring of the python exception class
		 * (it is rendered on the exception's page in the API reference).
		 *
		 * Instance can be passed to 'bp::register_exception_translator()'.
		 *
		 * If @a include_call_stack_trace is true then print out the call stack trace (see class CallStack)
		 * when the Python exception is printed.
		 */
		explicit
		PythonException(
				const char *python_exception_name,
				const char *python_exception_doc,
				bp::object python_base_exception_type,
				bool include_call_stack_trace) :
			d_include_call_stack_trace(include_call_stack_trace)
		{
			std::string scope_name = bp::extract<std::string>(bp::scope().attr("__name__"));
			std::string qualified_name = scope_name + "." + python_exception_name;

			// Create a new exception on the python side that maps to C++ exception 'ExceptionType'.
			d_python_exception_type = bp::object(bp::handle<>(
					// Returns a new reference so no need for 'bp::borrowed'...
					PyErr_NewExceptionWithDoc(
							qualified_name.c_str(),
							python_exception_doc,
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
			// Extract the message.
			std::ostringstream exc_message_stream;
			exc.write(
					exc_message_stream,
					// We don't include the exception name since it's the name of the C++ exception, not the Python exception...
					false/*include_exception_name*/,
					// Only those exceptions indicating an internal error/bug should print out the call stack trace...
					d_include_call_stack_trace);

			PyErr_SetString(d_python_exception_type.ptr(), exc_message_stream.str().c_str());
		}

	private:

		/**
		 * The type of exception on the python side (this maps to C++ exception 'ExceptionType').
		 */
		bp::object d_python_exception_type;

		/**
		 * Whether to print out the call stack trace when a Python exception is printed.
		 */
		bool d_include_call_stack_trace;
	};


	/**
	 * Creates a python exception with class name @a python_exception_name for C++ 'ExceptionType'
	 * and registers an exception translator for it.
	 *
	 * The base class of the python exception is of type @a python_base_exception_type, and
	 * @a python_exception_doc is its docstring (reStructuredText, like the class docstrings).
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
	 *
	 * If @a include_call_stack_trace is true then print out the call stack trace (see class CallStack)
	 * when the Python exception is printed. It is false by default because it was only meant for
	 * uncaught exceptions in GPlates. Uncaught exceptions are much more common in
	 * Python because users who write their own scripts will typically not
	 * write exception-handling Python code. However some C++ exceptions
	 * (such as AssertionFailureException, AbortExeption and PreconditionViolationError) indicate
	 * an internal problem and also do not have a message to print (hence confusing the Python user).
	 * So for these exceptions it's useful to print out the call stack trace so that when
	 * users encounter the associated Python exceptions they can contact a GPlates developer with
	 * information on where the exception happened (inside pyGPlates) so they can fix it.
	 */
	template <class ExceptionType>
	bp::object
	export_exception(
			const char *python_exception_name,
			const char *python_exception_doc,
			bp::object python_base_exception_type,
			bool include_call_stack_trace = false)
	{
		PythonException<ExceptionType> python_exception(
				python_exception_name,
				python_exception_doc,
				python_base_exception_type,
				include_call_stack_trace);

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
					"The base class of all pyGPlates exceptions.\n"
					"\n"
					"  Catch this to handle any error raised by pyGPlates itself. It inherits from Python's ``Exception``,\n"
					"  so an ``except Exception`` handler also catches it.\n"
					"\n"
					"  ::\n"
					"\n"
					"    try:\n"
					"        feature_collection = pygplates.FeatureCollection('features.gpml')\n"
					"    except pygplates.GPlatesError as error:\n"
					"        print('pyGPlates could not load the file: {}'.format(error))\n",
					bp::object(bp::handle<>(bp::borrowed(PyExc_Exception))));

	//
	// Direct subclasses of GPlatesGlobal::Exception.
	//
	GPlatesApi::AbortError =
			export_exception<GPlatesGlobal::AbortException>(
					"AbortError",
					"PyGPlates aborted the current operation because of an unexpected internal error.\n"
					"\n"
					"  This indicates a bug in pyGPlates rather than a mistake in your script. The error message includes the call stack trace - please report it (and the trace) to the GPlates developers.\n",
					GPlatesApi::GPlatesError,
					// Low-level unexpected internal errors should print out the call stack trace...
					true/*include_call_stack_trace*/);
	GPlatesApi::AssertionFailureError =
			export_exception<GPlatesGlobal::AssertionFailureException>(
					"AssertionFailureError",
					"An internal assertion failed inside pyGPlates.\n"
					"\n"
					"  This indicates a bug in pyGPlates rather than a mistake in your script. The error message includes the call stack trace - please report it (and the trace) to the GPlates developers.\n",
					GPlatesApi::GPlatesError,
					// Low-level unexpected internal errors should print out the call stack trace...
					true/*include_call_stack_trace*/);
	GPlatesApi::FileFormatNotSupportedError =
			export_exception<GPlatesFileIO::FileFormatNotSupportedException>(
					"FileFormatNotSupportedError",
					"The format of a file (determined by its filename extension) is not supported for the requested read or write.\n"
					"\n"
					"  .. seealso:: :class:`FeatureCollection` for the supported file formats.\n",
					GPlatesApi::GPlatesError);
	GPlatesApi::OpenFileForReadingError =
			export_exception<GPlatesFileIO::ErrorOpeningFileForReadingException>(
					"OpenFileForReadingError",
					"A file could not be opened for reading (for example, it does not exist or is not readable).\n",
					GPlatesApi::GPlatesError);
	GPlatesApi::OpenFileForWritingError =
			export_exception<GPlatesFileIO::ErrorOpeningFileForWritingException>(
					"OpenFileForWritingError",
					"A file could not be opened for writing (for example, its directory does not exist or is not writable).\n",
					GPlatesApi::GPlatesError);

	//
	// PreconditionViolationError and direct subclasses.
	//
	GPlatesApi::PreconditionViolationError =
			export_exception<GPlatesGlobal::PreconditionViolationError>(
					"PreconditionViolationError",
					"A precondition of a pyGPlates function or method was violated.\n"
					"\n"
					"  This is the base class of the exceptions raised when an argument does not satisfy the requirements\n"
					"  documented for a function or method (such as :class:`InvalidLatLonError` or :class:`InformationModelError`).\n"
					"\n"
					"  When *this* class is raised directly, rather than one of its subclasses, the precondition was violated by\n"
					"  pyGPlates itself.\n"
					"  This indicates a bug in pyGPlates rather than a mistake in your script. The error message includes the call stack trace - please report it (and the trace) to the GPlates developers.\n",
					GPlatesApi::GPlatesError,
					// Low-level unexpected internal errors should print out the call stack trace.
					// Note that a precondition violation might not be representative of an internal error.
					// It's possible the Python user has incorrectly used the pyGPlates API.
					// However those errors are usually an exception class *derived* from PreconditionViolationError
					// and those exception classes do *not* print out the call stack trace.
					// But if the *base* class PreconditionViolationError is thrown then it's usually thrown
					// by GPlates/pyGPlates when it violated a precondition itself (ie, not violated by the user)
					// and so it should be reported to a GPlates developer (along with the call stack trace)...
					true/*include_call_stack_trace*/);
	GPlatesApi::AmbiguousGeometryCoverageError =
			export_exception<GPlatesApi::AmbiguousGeometryCoverageException>(
					"AmbiguousGeometryCoverageError",
					"A coverage range could not be unambiguously matched to its geometry (coverage domain).\n"
					"\n"
					"  Raised when a feature has more than one coverage and two or more of the coverage ranges have the same\n"
					"  number of scalar values as a geometry has points, so pyGPlates cannot tell which range belongs to which geometry.\n"
					"\n"
					"  .. seealso:: :meth:`Feature.set_geometry` and :meth:`Feature.get_geometry`\n",
					GPlatesApi::PreconditionViolationError);
	GPlatesApi::DifferentAnchoredPlatesInReconstructionTreesError =
			export_exception<GPlatesApi::DifferentAnchoredPlatesInReconstructionTreesException>(
					"DifferentAnchoredPlatesInReconstructionTreesError",
					"Two reconstruction trees do not have the same anchor plate.\n"
					"\n"
					"  Raised when rotations from two :class:`reconstruction trees <ReconstructionTree>` built with\n"
					"  different anchor plates are combined (for example, to calculate a stage rotation).\n",
					GPlatesApi::PreconditionViolationError);
	GPlatesApi::DifferentTimesInPartitioningPlatesError =
			export_exception<GPlatesApi::DifferentTimesInPartitioningPlatesException>(
					"DifferentTimesInPartitioningPlatesError",
					"The partitioning plates given to a :class:`PlatePartitioner` do not all have the same reconstruction time.\n",
					GPlatesApi::PreconditionViolationError);
	GPlatesApi::GeometryTypeError =
			export_exception<GPlatesApi::GeometryTypeException>(
					"GeometryTypeError",
					"A geometry is not of the required type.\n"
					"\n"
					"  For example, a :class:`PointOnSphere` was passed where a :class:`PolylineOnSphere` is required and\n"
					"  *PolylineConversion.raise_if_non_polyline* was requested.\n",
					GPlatesApi::PreconditionViolationError);
	GPlatesApi::GmlTimePeriodBeginTimeLaterThanEndTimeError =
			export_exception<GPlatesPropertyValues::GmlTimePeriod::BeginTimeLaterThanEndTimeException>(
					"GmlTimePeriodBeginTimeLaterThanEndTimeError",
					"The begin time of a time period is later (more recent) than its end time.\n"
					"\n"
					"  Begin times must be further in the past than end times. For example, a valid time of ``(100, 0)``\n"
					"  begins at 100 Ma and ends at present day, whereas ``(0, 100)`` raises this error.\n",
					GPlatesApi::PreconditionViolationError);
	GPlatesApi::IndeterminateArcRotationAxisError =
			export_exception<GPlatesMaths::IndeterminateArcRotationAxisException>(
					"IndeterminateArcRotationAxisError",
					"The rotation axis of a :class:`GreatCircleArc` cannot be determined because the arc has zero length.\n",
					GPlatesApi::PreconditionViolationError);
	GPlatesApi::IndeterminateGreatCircleArcDirectionError =
			export_exception<GPlatesApi::IndeterminateGreatCircleArcDirectionException>(
					"IndeterminateGreatCircleArcDirectionError",
					"The direction of a :class:`GreatCircleArc` cannot be determined because the arc has zero length.\n",
					GPlatesApi::PreconditionViolationError);
	GPlatesApi::IndeterminateGreatCircleArcNormalError =
			export_exception<GPlatesApi::IndeterminateGreatCircleArcNormalException>(
					"IndeterminateGreatCircleArcNormalError",
					"The normal of a :class:`GreatCircleArc` cannot be determined because the arc has zero length.\n",
					GPlatesApi::PreconditionViolationError);
	GPlatesApi::InformationModelError =
			export_exception<GPlatesApi::InformationModelException>(
					"InformationModelError",
					"A feature type, property name or property value does not conform to the GPlates Geological Information Model (GPGIM).\n"
					"\n"
					"  Raised when *verify_information_model* is *VerifyInformationModel.yes* (the default) and, for example,\n"
					"  a feature type is not recognised, a property name is not supported by the feature type, or a property\n"
					"  value is not of the type the property expects.\n"
					"\n"
					"  .. seealso:: :class:`VerifyInformationModel`\n",
					GPlatesApi::PreconditionViolationError);
	GPlatesApi::InsufficientPointsForMultiPointConstructionError =
			export_exception<GPlatesMaths::InsufficientPointsForMultiPointConstructionError>(
					"InsufficientPointsForMultiPointConstructionError",
					"A :class:`MultiPointOnSphere` could not be constructed because the sequence of points is empty.\n",
					GPlatesApi::PreconditionViolationError);
	GPlatesApi::InterpolationError =
			export_exception<GPlatesApi::InterpolationException>(
					"InterpolationError",
					"A time-dependent property value could not be interpolated.\n"
					"\n"
					"  For example, one of the sample times is the :meth:`distant past <GeoTimeInstant.is_distant_past>`\n"
					"  or the :meth:`distant future <GeoTimeInstant.is_distant_future>`.\n",
					GPlatesApi::PreconditionViolationError);
	GPlatesApi::InvalidPointsForPolygonConstructionError =
			export_exception<GPlatesMaths::InvalidPointsForPolygonConstructionError>(
					"InvalidPointsForPolygonConstructionError",
					"A :class:`PolygonOnSphere` could not be constructed from a sequence of points.\n"
					"\n"
					"  For example, a ring has fewer than three points, or two adjacent points in a ring are antipodal\n"
					"  (on opposite sides of the globe).\n",
					GPlatesApi::PreconditionViolationError);
	GPlatesApi::InvalidPointsForPolylineConstructionError =
			export_exception<GPlatesMaths::InvalidPointsForPolylineConstructionError>(
					"InvalidPointsForPolylineConstructionError",
					"A :class:`PolylineOnSphere` could not be constructed from a sequence of points.\n"
					"\n"
					"  For example, there are fewer than two points, or two adjacent points are antipodal\n"
					"  (on opposite sides of the globe).\n",
					GPlatesApi::PreconditionViolationError);
	GPlatesApi::InvalidLatLonError =
			export_exception<GPlatesMaths::InvalidLatLonException>(
					"InvalidLatLonError",
					"A latitude is outside the range [-90, 90] or a longitude is outside the range [-360, 360].\n"
					"\n"
					"  .. seealso:: :meth:`LatLonPoint.is_valid_latitude` and :meth:`LatLonPoint.is_valid_longitude`\n",
					GPlatesApi::PreconditionViolationError);

	//
	// MathematicalException and direct subclasses.
	//
	GPlatesApi::MathematicalError =
			export_exception<GPlatesMaths::MathematicalException>(
					"MathematicalError",
					"The base class of the exceptions raised when a mathematical operation cannot be completed.\n",
					GPlatesApi::GPlatesError);
	GPlatesApi::IndeterminateResultError =
			export_exception<GPlatesMaths::IndeterminateResultException>(
					"IndeterminateResultError",
					"The result of a mathematical operation is indeterminate.\n"
					"\n"
					"  For example, the rotation axis of an :meth:`identity rotation <FiniteRotation.represents_identity_rotation>`\n"
					"  is undefined.\n",
					GPlatesApi::MathematicalError);
	GPlatesApi::UnableToNormaliseZeroVectorError =
			export_exception<GPlatesMaths::UnableToNormaliseZeroVectorException>(
					"UnableToNormaliseZeroVectorError",
					"A vector of zero magnitude cannot be normalised because it has no direction.\n",
					GPlatesApi::MathematicalError);
	GPlatesApi::ViolatedUnitVectorInvariantError =
			export_exception<GPlatesMaths::ViolatedUnitVectorInvariantException>(
					"ViolatedUnitVectorInvariantError",
					"A vector that must have unit magnitude does not.\n"
					"\n"
					"  For example, an (x, y, z) triplet passed to :class:`PointOnSphere` does not lie on the unit sphere.\n",
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
