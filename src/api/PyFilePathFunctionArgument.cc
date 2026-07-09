/**
 * Copyright (C) 2024 The University of Sydney, Australia
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
#include <QString>

#include "PyFilePathFunctionArgument.h"

#include "PythonConverterUtils.h"
#include "PythonExtractUtils.h"

#include "global/python.h"

#include "utils/ReferenceCount.h"


namespace bp = boost::python;


namespace GPlatesApi
{
	/**
	 * A from-python converter from a string filename or a 'os.PathLike' object to a @a FilePathFunctionArgument.
	 */
	struct ConversionFilePathFunctionArgument :
			private boost::noncopyable
	{
		static
		void *
		convertible(
				PyObject *obj)
		{
			namespace bp = boost::python;

			if (FilePathFunctionArgument::is_convertible(
				bp::object(bp::handle<>(bp::borrowed(obj)))))
			{
				return obj;
			}

			return NULL;
		}

		static
		void
		construct(
				PyObject *obj,
				boost::python::converter::rvalue_from_python_stage1_data *data)
		{
			namespace bp = boost::python;

			void *const storage = reinterpret_cast<
					bp::converter::rvalue_from_python_storage<
							FilePathFunctionArgument> *>(
									data)->storage.bytes;

			new (storage) FilePathFunctionArgument(
					bp::object(bp::handle<>(bp::borrowed(obj))));

			data->convertible = storage;
		}
	};


	/**
	 * Registers converter from a string filename or a 'os.PathLike' object to a @a FilePathFunctionArgument.
	 */
	void
	register_conversion_file_path_function_argument()
	{
		// Register function argument types variant.
		PythonConverterUtils::register_variant_conversion<
				FilePathFunctionArgument::function_argument_type>();

		// NOTE: We don't define a to-python conversion.

		// From python conversion.
		bp::converter::registry::push_back(
				&ConversionFilePathFunctionArgument::convertible,
				&ConversionFilePathFunctionArgument::construct,
				bp::type_id<FilePathFunctionArgument>());
	}
}


bool
GPlatesApi::FilePathFunctionArgument::is_convertible(
		bp::object python_function_argument)
{
	// Test all supported types (in function_argument_type).
	return bp::extract<QString>(python_function_argument).check() ||
			// See if the argument is an 'os.PathLike' object with a '__fspath__()' method to get the path as a string
			// (see PEP 519 and https://docs.python.org/3/library/os.html#os.PathLike)...
			PyObject_HasAttrString(python_function_argument.ptr(), "__fspath__");
}


GPlatesApi::FilePathFunctionArgument::FilePathFunctionArgument(
		bp::object python_function_argument) :
	d_file_path(
			initialise_file_path(
					bp::extract<function_argument_type>(python_function_argument)))
{
}


GPlatesApi::FilePathFunctionArgument::FilePathFunctionArgument(
		const function_argument_type &function_argument) :
	d_file_path(initialise_file_path(function_argument))
{
}


QString
GPlatesApi::FilePathFunctionArgument::initialise_file_path(
		const function_argument_type &function_argument)
{
	if (const QString *string_function_argument =
		boost::get<QString>(&function_argument))
	{
		return *string_function_argument;
	}
	else
	{
		const bp::object function_argument_object = boost::get<bp::object>(function_argument);

		// The argument is an 'os.PathLike' object, so call its '__fspath__()' method to get the path as a string
		// (see PEP 519 and https://docs.python.org/3/library/os.html#os.PathLike).
		bp::object fspath_object = function_argument_object.attr("__fspath__")();
		bp::extract<QString> extract_fspath(fspath_object);
		if (!extract_fspath.check())
		{
			PyErr_SetString(PyExc_TypeError, "expected __fspath__() to return str or bytes");
			bp::throw_error_already_set();
		}

		return extract_fspath();
	}
}


void
export_file_path_function_argument()
{
	// Register converter from a feature collection or a string filename to a @a FilePathFunctionArgument.
	GPlatesApi::register_conversion_file_path_function_argument();
}
