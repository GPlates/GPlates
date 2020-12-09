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
#include <boost/noncopyable.hpp>

#include "PythonConverterUtils.h"

#include "global/CompilerWarnings.h"
#include "global/python.h"

#include "maths/Real.h"


namespace bp = boost::python;


namespace GPlatesApi
{
	//
	// The following to/from Python conversions are handled:
	//
	// To Python                 Numpy scalar
	//     /\                       |
	//     |                        |
	//     |                        |
	//     |                        \/
	//     |                      float
	//     |                        /\
	//     |                        |
	//     |                        |
	//     \/                       \/
	// From Python           GPlatesMaths::Real
	//

// For PyFloat_Check below.
DISABLE_GCC_WARNING("-Wold-style-cast")

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
	template <typename FloatType>
	struct ConversionNumPyToFloatType :
			private boost::noncopyable
	{
		explicit
		ConversionNumPyToFloatType()
		{
			// From python conversion.
			bp::converter::registry::push_back(
					&convertible,
					&construct,
					bp::type_id<FloatType>());
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

			// Any NumPy integer/float scalar can be converted to a C++ floating-point type.
			if (PyArray_IsScalar(obj, Integer) ||
				PyArray_IsScalar(obj, Float) ||
				PyArray_IsScalar(obj, Double) ||
				PyArray_IsScalar(obj, LongDouble))
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
			// Convert the numpy type (integer/float/double/long double) to a 'double'.
			// Except for 'long double' this should not result in an overflow.
			// We could make this a 'long double' instead of just 'double' in future if needed.
			npy_double np_value;
			PyArray_Descr *np_outcode = PyArray_DescrFromType(NPY_DOUBLE);
			PyArray_CastScalarToCtype(obj, &np_value, np_outcode);
			Py_DECREF(np_outcode);

			void *const storage = reinterpret_cast<
					bp::converter::rvalue_from_python_storage<FloatType> *>(
							data)->storage.bytes;

			// Cast 'double' to the desired 'FloatType'.
			try
			{
				new (storage) FloatType(boost::numeric_cast<FloatType>(np_value));
			}
			catch (boost::numeric::bad_numeric_cast &)
			{
				PyErr_SetString(PyExc_ValueError, "Conversion from NumPy scalar type to builtin floating-point overflowed.");
				bp::throw_error_already_set();
			}

			data->convertible = storage;
		}
	};
#endif // GPLATES_HAVE_NUMPY_C_API


	/**
	 * Enables Real to be passed to and from python (float object).
	 *
	 * For more information on boost python to/from conversions, see:
	 *   http://misspent.wordpress.com/2009/09/27/how-to-write-boost-python-converters/
	 */
	struct ConversionReal :
			private boost::noncopyable
	{
		explicit
		ConversionReal()
		{
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
				return bp::incref(bp::object(real.dval()).ptr());
			};
		};

		static
		void *
		convertible(
				PyObject *obj)
		{
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
export_float()
{
	//
	// Registers from-python converters from numpy integer/floating-point types to C++ floating-point types.
	//
	// Only registered if we have access to the numpy C-API. Otherwise Python users will have to
	// explicitly convert their numpy integer/float scalars to Python built-in int/float
	// (in their Python code) before calling pyGPlates functions expecting a float/double.
	//
#ifdef GPLATES_HAVE_NUMPY_C_API
	GPlatesApi::ConversionNumPyToFloatType<double>();
	GPlatesApi::ConversionNumPyToFloatType<float>();
#endif // GPLATES_HAVE_NUMPY_C_API

	//
	// Note: We don't need to register to-from python converters between Python native int/float and C++
	// floating-point types because boost-python takes care of that for us.
	// However we do need to register converters for boost::optional<double>, etc, so that python's
	// "None" (ie, Py_None) can be used as a function argument for example.
	//

	// Enable boost::optional<double> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<double>();

	// Enable boost::optional<float> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<float>();
}

void
export_real()
{
	// Registers the python to/from converters for GPlatesMaths::Real.
	GPlatesApi::ConversionReal();

	// Enable boost::optional<GPlatesMaths::Real> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesMaths::Real>();
}
