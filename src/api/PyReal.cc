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

#include <boost/noncopyable.hpp>

#include "PythonConverterUtils.h"

#include "global/CompilerWarnings.h"
#include "global/python.h"

#include "maths/Real.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
// For PyFloat_Check below.
DISABLE_GCC_WARNING("-Wold-style-cast")

	/**
	 * Enables Real to be passed to and from python (float object).
	 *
	 * For more information on boost python to/from conversions, see:
	 *   http://misspent.wordpress.com/2009/09/27/how-to-write-boost-python-converters/
	 */
	struct python_Real :
			private boost::noncopyable
	{
		explicit
		python_Real()
		{
			namespace bp = boost::python;

			// To python conversion.
			bp::to_python_converter<GPlatesMaths::Real, Conversion>();

			// From python conversion.
			bp::converter::registry::push_back(
					&convertible,
					&construct,
					bp::type_id<GPlatesMaths::Real>());
		}

		struct Conversion
		{
			static
			PyObject *
			convert(
					const GPlatesMaths::Real &real)
			{
				namespace bp = boost::python;

				return bp::incref(bp::object(real.dval()).ptr());
			};
		};

		static
		void *
		convertible(
				PyObject *obj)
		{
			namespace bp = boost::python;

			// Note: We now use 'bp::extract<double>', instead of explicitly checking for a
			// python 'float', because this allows conversion of integers to 'GPlatesMaths::Real'.
#if 0
			return PyFloat_Check(obj) ? obj : NULL;
#endif
			return bp::extract<double>(obj).check() ? obj : NULL;
		}

		static
		void
		construct(
				PyObject *obj,
				boost::python::converter::rvalue_from_python_stage1_data *data)
		{
			namespace bp = boost::python;

			void *const storage = reinterpret_cast<
					bp::converter::rvalue_from_python_storage<GPlatesMaths::Real> *>(
							data)->storage.bytes;

			new (storage) GPlatesMaths::Real(bp::extract<double>(obj));

			data->convertible = storage;
		}
	};

ENABLE_GCC_WARNING("-Wold-style-cast")
}


void
export_real()
{
	// Registers the python to/from converters for GPlatesMaths::Real.
	GPlatesApi::python_Real();

	// Enable boost::optional<GPlatesMaths::Real> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::python_optional<GPlatesMaths::Real>();
}

#endif // GPLATES_NO_PYTHON
