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

#include <algorithm>
#include <iterator>
#include <utility>
#include <vector>

#include "PythonExtractUtils.h"

#include "global/python.h"
// This is not included by <boost/python.hpp>.
// Also we must include this after <boost/python.hpp> which means after "global/python.h".
#include <boost/python/stl_iterator.hpp>


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


void
GPlatesApi::PythonExtractUtils::extract_key_value_map(
			key_value_map_type &key_value_map,
			bp::object key_value_mapping_object,
			const char *type_error_string)
{
	// See if it's a Python 'dict'.
	bp::extract<bp::dict> extract_key_value_dict(key_value_mapping_object);
	if (extract_key_value_dict.check())
	{
		// Get the iterator over the list of items in the dictionary.
		// This is an iterator over (key,value) tuples.
		key_value_mapping_object = extract_key_value_dict().iteritems();
	}

	// Attempt to extract the python key/value sequence.
	std::vector<bp::object> key_value_objects;
	try
	{
		std::copy(
				bp::stl_input_iterator<bp::object>(key_value_mapping_object),
				bp::stl_input_iterator<bp::object>(),
				std::back_inserter(key_value_objects));
	}
	catch (const bp::error_already_set &)
	{
		PyErr_Clear();

		PyErr_SetString(PyExc_TypeError, type_error_string);
		bp::throw_error_already_set();
	}

	// Extract the individual key/value object pairs.
	std::vector<bp::object>::const_iterator key_value_objects_iter = key_value_objects.begin();
	std::vector<bp::object>::const_iterator key_value_objects_end = key_value_objects.end();
	for ( ; key_value_objects_iter != key_value_objects_end; ++key_value_objects_iter)
	{
		const bp::object &key_value_object = *key_value_objects_iter;

		// Attempt to extract the (key, value) 2-tuples/2-sequences.
		std::vector<bp::object> key_value_pair;
		try
		{
			std::copy(
					bp::stl_input_iterator<bp::object>(key_value_object),
					bp::stl_input_iterator<bp::object>(),
					std::back_inserter(key_value_pair));
		}
		catch (const bp::error_already_set &)
		{
			PyErr_Clear();

			PyErr_SetString(PyExc_TypeError, type_error_string);
			bp::throw_error_already_set();
		}

		if (key_value_pair.size() != 2)
		{
			PyErr_SetString(PyExc_TypeError, type_error_string);
			bp::throw_error_already_set();
		}

		key_value_map.push_back(
				std::make_pair(key_value_object[0], key_value_object[1]));
	}
}

#endif // GPLATES_NO_PYTHON
