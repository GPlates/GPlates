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

#ifndef GPLATES_API_PYFILEPATHARGUMENT_H
#define GPLATES_API_PYFILEPATHARGUMENT_H

#include <boost/variant.hpp>
#include <QString>

#include "global/python.h"


namespace GPlatesApi
{
	/**
	 * A convenience class for receiving a file path function argument as either:
	 *  (1) a string, or
	 *  (2) an os.PathLike object with a '__fspath__()' method to get the path as a string
	 *      (see PEP 519 and https://docs.python.org/3/library/os.html#os.PathLike).
	 *
	 * To get an instance of @a FilePathFunctionArgument you can either:
	 *  (1) specify FilePathFunctionArgument directly as a function argument type
	 *      (in the C++ function being wrapped), or
	 *  (2) use boost::python::extract<FilePathFunctionArgument>().
	 */
	class FilePathFunctionArgument
	{
	public:

		/**
		 * Types of function argument.
		 */
		typedef boost::variant<
				QString,
				boost::python::object/*os.PathLike object*/> function_argument_type;


		/**
		 * Returns true if @a python_function_argument is convertible to an instance of this class.
		 */
		static
		bool
		is_convertible(
				boost::python::object python_function_argument);


		//! Default constructor with empty file path.
		FilePathFunctionArgument()
		{  }

		explicit
		FilePathFunctionArgument(
				boost::python::object python_function_argument);


		explicit
		FilePathFunctionArgument(
				const function_argument_type &function_argument);

		/**
		 * Return the file path.
		 */
		QString
		get_file_path() const
		{
			return d_file_path;
		}

	private:

		static
		QString
		initialise_file_path(
				const function_argument_type &function_argument);


		QString d_file_path;
	};
}

#endif // GPLATES_API_PYFILEPATHARGUMENT_H
