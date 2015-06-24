/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#ifndef GPLATES_API_PYTHONEXTRACTUTILS_H
#define GPLATES_API_PYTHONEXTRACTUTILS_H

#include <utility>
#include <vector>

#include "global/python.h"


#if !defined(GPLATES_NO_PYTHON)

namespace GPlatesApi
{
	namespace PythonExtractUtils
	{
		/**
		 * Typedef for a key/value object pair.
		 */
		typedef std::pair<boost::python::object/*key*/, boost::python::object/*value*/> key_value_type;

		/**
		 * Typedef for a sequence of key/value object pairs.
		 */
		typedef std::vector<key_value_type> key_value_map_type;


		/**
		 * Extracts the key/value pairs from a Python 'dict' or from a sequence of (key, value) tuples.
		 *
		 * The keys/values are extracted as generic boost::python::object (rather than extracting a specific
		 * C++ type) since the caller might want to consider more than one type for each value (for example).
		 *
		 * The error message @a type_error_string should be something like:
		 *
		 *  "Expected a 'dict' or a sequence of (key, value) 2-tuples"
		 *
		 * ...where you can replace 'key' and 'value' with the types you are expecting.
		 */
		void
		extract_key_value_map(
					key_value_map_type &key_value_map,
					boost::python::object key_value_mapping_object,
					const char *type_error_string);
	}
}

#endif   //GPLATES_NO_PYTHON)

#endif // GPLATES_API_PYTHONEXTRACTUTILS_H
