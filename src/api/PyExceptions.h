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

#ifndef GPLATES_API_PYEXCEPTIONS_H
#define GPLATES_API_PYEXCEPTIONS_H

#include <QString>

#include "global/python.h"

#include "global/PreconditionViolationError.h"


#if !defined(GPLATES_NO_PYTHON)

namespace GPlatesApi
{
	//
	// GPlates C++ exceptions translated to python errors.
	//
	// The names of the associated C++ exceptions match the python exception name.
	// For example, in python "AssertionFailureError" and in C++ "AssertionFailureException".
	//
	// These exceptions are not normally needed for pygplates API functions (C++) because the
	// associated C++ exceptions are automatically converted to these python exceptions (below)
	// by registering exception handlers with boost-python - see "export_exceptions()" - and these
	// are handled for us by boost-python (in 'def' binding statements, etc). In other words the
	// C++ exception gets converted to its python equivalent before the pygplates API function returns
	// back to its python calling function. The python calling function is then responsible for
	// dealing with the exception (or ignoring it as mentioned in the example below).
	// However these python exceptions can be needed by C++ code that is not called by python.
	// For example, in the GPlates desktop (which has an embedded python interpreter) there could be
	// some C++ code that delegates to a python function. That python function could then, in turn,
	// call the pygplates API (C++) which could throw a C++ exception. That C++ exception gets
	// converted to a python exception (by our boost-python registered exception handlers) and,
	// let's say, the python code does not catch it. So it propagates back to the original C++ code
	// (that has no python caller). If this C++ code does not catch it then GPlates will catch the
	// exception in its default handler and shutdown.
	// In this case class PythonExceptionHandler below can be used to handle the python exception.
	//
	// NOTE: These are Py_None if the 'pygplates' module has not yet been initialised.
	//

	extern boost::python::object AssertionFailureError;
	extern boost::python::object DifferentAnchoredPlatesInReconstructionTreesError;
	extern boost::python::object FileFormatNotSupportedError;
	extern boost::python::object GeometryTypeError;
	extern boost::python::object GmlTimePeriodBeginTimeLaterThanEndTimeError;
	extern boost::python::object GPlatesError;
	extern boost::python::object IndeterminateArcRotationAxisError;
	extern boost::python::object InformationModelError;
	extern boost::python::object IndeterminateResultError;
	extern boost::python::object InsufficientPointsForMultiPointConstructionError;
	extern boost::python::object InterpolationError;
	extern boost::python::object InvalidLatLonError;
	extern boost::python::object InvalidPointsForPolygonConstructionError;
	extern boost::python::object InvalidPointsForPolylineConstructionError;
	extern boost::python::object MathematicalError;
	extern boost::python::object OpenFileForReadingError;
	extern boost::python::object OpenFileForWritingError;
	extern boost::python::object PreconditionViolationError;
	extern boost::python::object UnableToNormaliseZeroVectorError;
	extern boost::python::object ViolatedUnitVectorInvariantError;


	/**
	 * Retrieves the Python error, enables user to compare it with known exception types and
	 * enables restoration of error (if not handled).
	 *
	 * For example:
	 *
	 *   try
	 *   {
	 *      // Call python functions that might raise an error because either:
	 *      //  (1) the called pure python code raises an exception, or
	 *      //  (2) the called pure python code calls pygplates C++ code that throws a C++ exception, or
	 *      //  (3) the python function is actually pygplates C++ code, that throws a C++ exception.
	 *      ...
	 *   }
	 *   catch (const boost::python::error_already_set &)
	 *   {
	 *      PythonExceptionHandler handler;
	 *      if (handler.exception_matches(OpenFileForReadingError.ptr()))
	 *      {
	 *         // Handle OpenFileForReadingError - this is an exception originally thrown
	 *         // in GPlates C++ code and was translated to a python error before returning to
	 *         // python which, in turn, returned to us (C++ code).
	 *         ...
	 *         // Restore the python error and throw boost::python::error_already_set.
	 *         // Do this only if wish to continue propagation of error up the call stack.
	 *         handler.restore_exception();
	 *      }
	 *      else if (handler.exception_matches(GPlatesError))
	 *      {
	 *         // Handle any derivations of GPlatesError (GPlatesGlobal::Exception) generated by C++ code.
	 *         ...
	 *      }
	 *      else if (handler.exception_matches(PyExc_RuntimeError))
	 *      {
	 *         // Handle run-time error (most likely) generated by some pure python code.
	 *         ...
	 *      }
	 *      else
	 *      {
	 *         // Restore the python error and throw boost::python::error_already_set.
	 *         handler.restore_exception();
	 *      }
	 *   }
	 */
	class PythonExceptionHandler
	{
	public:

		/**
		 * Fetch the current python error.
		 *
		 * @throws PreconditionViolationError if no python error is currently set.
		 */
		PythonExceptionHandler();

		/**
		 * Note that this does *not* restore the exception (in python) fetched in the constructor.
		 *
		 * To restore the exception you must explicitly call @a restore_exception.
		 */
		~PythonExceptionHandler();

		/**
		 * Returns true if @a exception matches the python error fetched in the constructor.
		 *
		 * Also returns true if the python error fetched is a sub-class of @a exception.
		 */
		bool
		exception_matches(
				PyObject *exception) const;

		/**
		 * Restores the python error fetched in the constructor and
		 * throws boost::python::error_already_set to continue propagation of the error.
		 */
		void
		restore_exception();

		/**
		 * Extracts the exception's string error message, or returns empty string if unsuccesful.
		 */
		QString
		get_exception_message() const;

		/**
		 * Returns the exception type.
		 *
		 * Note that the exception type is always non-null.
		 */
		boost::python::object
		get_exception_type() const;

		/**
		 * Returns the exception value, or Py_None if NULL.
		 */
		boost::python::object
		get_exception_value() const;

		/**
		 * Returns the exception traceback, or Py_None if NULL.
		 */
		boost::python::object
		get_exception_traceback() const;

	private:

		bool d_restore_exception;
		PyObject *d_type;
		PyObject *d_value;
		PyObject *d_traceback;
	};
}

#endif   // GPLATES_NO_PYTHON

#endif // GPLATES_API_PYEXCEPTIONS_H
