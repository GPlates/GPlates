/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2014 The University of Sydney, Australia
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

#ifndef GPLATES_API_PYGEOMETRIESONSPHERE_H
#define GPLATES_API_PYGEOMETRIESONSPHERE_H

#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/variant.hpp>
#include <QString>

#include "global/PreconditionViolationError.h"
#include "global/python.h"

#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"


#if !defined(GPLATES_NO_PYTHON)

namespace GPlatesApi
{
	/**
	 * A convenience class for receiving a PointOnSphere sequence function argument as either:
	 *  (1) a sequence of PointOnSphere's (or anything convertible to a PointOnSphere), or
	 *  (2) a PointOnSphere, or
	 *  (3) a MultiPointOnSphere, or
	 *  (4) a PolylineOnSphere, or
	 *  (5) a PolygonOnSphere.
	 *
	 * In cases (3), (4) and (5) the sequence of points is extracted from the geometry in question.
	 * This is a lot more efficient to do in C++ than in Python since the latter invokes
	 * '__iter__' on the (Python) geometry and must go back and forth between C++ and Python
	 * for *each* point in the geometry.
	 * By getting access to the geometry object directly (in C++) we can access all its points
	 * in one go (within C++).
	 *
	 * To get an instance of @a PointSequenceFunctionArgument you can either:
	 *  (1) specify PointSequenceFunctionArgument directly as a function argument type
	 *      (in the C++ function being wrapped), or
	 *  (2) use boost::python::extract<PointSequenceFunctionArgument>().
	 */
	class PointSequenceFunctionArgument
	{
	public:

		/**
		 * Types of function argument.
		 */
		typedef boost::variant<
				// These are the types stored inside the associated Python-wrapped objects (in bp::class_)...
				GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PointOnSphere>,
				GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::MultiPointOnSphere>,
				GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PolylineOnSphere>,
				GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PolygonOnSphere>,
				// Sequence of points...
				boost::python::object>
						function_argument_type;


		/**
		 * Returns true if @a python_function_argument is convertible to an instance of this class.
		 */
		static
		bool
		is_convertible(
				boost::python::object python_function_argument);


		explicit
		PointSequenceFunctionArgument(
				boost::python::object python_function_argument);


		explicit
		PointSequenceFunctionArgument(
				const function_argument_type &function_argument);

		/**
		 * Return the function argument as a sequence of points.
		 */
		const std::vector<GPlatesMaths::PointOnSphere> &
		get_points() const
		{
			return *d_points;
		}

	private:

		typedef std::vector<GPlatesMaths::PointOnSphere> point_seq_type;


		static
		boost::shared_ptr<point_seq_type>
		initialise_points(
				const function_argument_type &function_argument);


		// Using boost::shared_ptr so that a 'PointSequenceFunctionArgument' instance is cheap to copy.
		boost::shared_ptr<point_seq_type> d_points;
	};


	/**
	 * The wrong type of GeometryOnSphere was specified.
	 */
	class GeometryTypeException :
			public GPlatesGlobal::PreconditionViolationError
	{
	public:

		explicit
		GeometryTypeException(
				const GPlatesUtils::CallStack::Trace &exception_source,
				const QString &message) :
			GPlatesGlobal::PreconditionViolationError(exception_source),
			d_message(message)
		{  }

		~GeometryTypeException() throw()
		{  }

	protected:

		virtual
		const char *
		exception_name() const
		{
			return "GeometryTypeException";
		}

		virtual
		void
		write_message(
				std::ostream &os) const
		{
			write_string_message(os, d_message.toStdString());
		}

	private:

		QString d_message;
	};


	/**
	 * Enumeration to determine conversion of geometries to PolylineOnSphere.
	 */
	namespace PolylineConversion
	{
		enum Value
		{
			CONVERT_TO_POLYLINE,   // Arguments that are not a PolylineOnSphere are converted to one.
			IGNORE_NON_POLYLINE,   // Ignore arguments that are not a PolylineOnSphere - may result in no-op.
			RAISE_IF_NON_POLYLINE  // Raises GeometryTypeError if argument(s) is not a PolylineOnSphere.
		};
	};
}

#endif   // GPLATES_NO_PYTHON

#endif // GPLATES_API_PYGEOMETRIESONSPHERE_H
