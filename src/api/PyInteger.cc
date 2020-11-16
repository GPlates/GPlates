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

#include <boost/cast.hpp>

#include "PythonConverterUtils.h"

#include "global/python.h"


namespace bp = boost::python;


namespace GPlatesApi
{
	/**
	 * Enables numpy integer and floating-point types to be passed from python to C++ floating-point types.
	 *
	 * NOTE: Only provided if we have access to the numpy C-API. Otherwise Python users will have to
	 * explicitly convert their numpy integer/float scalars to Python built-in int/float
	 * (in their Python code) before calling pyGPlates functions.
	 *
	 * For more information on boost python to/from conversions, see:
	 *   http://misspent.wordpress.com/2009/09/27/how-to-write-boost-python-converters/
	 */
#ifdef GPLATES_HAVE_NUMPY_C_API
	template <typename IntegerType>
	struct ConversionNumPyToIntegerType :
			private boost::noncopyable
	{
		explicit
		ConversionNumPyToIntegerType()
		{
			// From python conversion.
			bp::converter::registry::push_back(
					&convertible,
					&construct,
					bp::type_id<IntegerType>());
		}

		static
		void *
		convertible(
				PyObject *obj)
		{
			if (obj == nullptr)
			{
				return nullptr;
			}

			// Any NumPy integer scalar can be converted to a C++ integer type.
			if (PyArray_IsScalar(obj, Integer))
			{
				return obj;
			}

			return nullptr;
		}

		static
		void
		construct(
				PyObject *obj,
				boost::python::converter::rvalue_from_python_stage1_data *data)
		{
			// Convert the numpy type (integer) to a 'long long'.
			// This should not result in an overflow except for a 64-bit NumPy *unsigned* integer that's
			// larger than the maximum 64-bit *signed* integer being passed to a C++ 64-bit *signed* integer
			// (however we're extremely unlikely to use such large numbers).
			npy_longlong np_value;
			PyArray_Descr *np_outcode = PyArray_DescrFromType(NPY_LONGLONG);
			PyArray_CastScalarToCtype(obj, &np_value, np_outcode);
			Py_DECREF(np_outcode);

			void *const storage = reinterpret_cast<
					bp::converter::rvalue_from_python_storage<IntegerType> *>(
							data)->storage.bytes;

			// Cast 'long long' to the desired 'IntegerType'.
			try
			{
				new (storage) IntegerType(boost::numeric_cast<IntegerType>(np_value));
			}
			catch (boost::numeric::bad_numeric_cast &)
			{
				PyErr_SetString(PyExc_ValueError, "Conversion from NumPy integer scalar type to builtin integer type overflowed.");
				bp::throw_error_already_set();
			}

			data->convertible = storage;
		}
	};
#endif // GPLATES_HAVE_NUMPY_C_API
}


void
export_integer()
{
	//
	// Registers from-python converters from numpy integer types to C++ integer types.
	//
	// Only registered if we have access to the numpy C-API. Otherwise Python users will have to
	// explicitly convert their numpy integer scalars to Python built-in int
	// (in their Python code) before calling pyGPlates functions expecting an integer.
	//
#ifdef GPLATES_HAVE_NUMPY_C_API
	GPlatesApi::ConversionNumPyToIntegerType<char>();
	GPlatesApi::ConversionNumPyToIntegerType<unsigned char>();

	GPlatesApi::ConversionNumPyToIntegerType<short>();
	GPlatesApi::ConversionNumPyToIntegerType<unsigned short>();

	GPlatesApi::ConversionNumPyToIntegerType<int>();
	GPlatesApi::ConversionNumPyToIntegerType<unsigned int>();

	GPlatesApi::ConversionNumPyToIntegerType<long>();
	GPlatesApi::ConversionNumPyToIntegerType<unsigned long>();

	GPlatesApi::ConversionNumPyToIntegerType<long long>();
	GPlatesApi::ConversionNumPyToIntegerType<unsigned long long>();
#endif // GPLATES_HAVE_NUMPY_C_API

	//
	// Note: We don't need to register to-from python converters between Python native int and C++
	// int types because boost-python takes care of that for us.
	// Note: We don't need to register to-from python converters for integers because boost-python
	// takes care of that for us.
	// However we do need to register converters for boost::optional<int>, etc, so that python's
	// "None" (ie, Py_None) can be used as a function argument for example.
	//

	GPlatesApi::PythonConverterUtils::register_optional_conversion<char>();
	GPlatesApi::PythonConverterUtils::register_optional_conversion<unsigned char>();

	GPlatesApi::PythonConverterUtils::register_optional_conversion<short>();
	GPlatesApi::PythonConverterUtils::register_optional_conversion<unsigned short>();

	GPlatesApi::PythonConverterUtils::register_optional_conversion<int>();
	GPlatesApi::PythonConverterUtils::register_optional_conversion<unsigned int>();

	GPlatesApi::PythonConverterUtils::register_optional_conversion<long>();
	GPlatesApi::PythonConverterUtils::register_optional_conversion<unsigned long>();

	GPlatesApi::PythonConverterUtils::register_optional_conversion<long long>();
	GPlatesApi::PythonConverterUtils::register_optional_conversion<unsigned long long>();

	// TODO: Perhaps might also need 64-bit integers (when supported) such as boost::uint64_t.
}
