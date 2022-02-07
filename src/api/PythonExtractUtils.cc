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

#include "PyExceptions.h"
#include "PyGPlatesModule.h"

#include "global/python.h"


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
		key_value_mapping_object = extract_key_value_dict().
#if PY_MAJOR_VERSION >= 3
				items();
#else
				iteritems();
#endif 
	}

	// Attempt to extract the python key/value sequence.
	std::vector<bp::object> key_value_objects;
	extract_iterable(key_value_objects, key_value_mapping_object, type_error_string);

	// Extract the individual key/value object pairs.
	std::vector<bp::object>::const_iterator key_value_objects_iter = key_value_objects.begin();
	std::vector<bp::object>::const_iterator key_value_objects_end = key_value_objects.end();
	for ( ; key_value_objects_iter != key_value_objects_end; ++key_value_objects_iter)
	{
		const bp::object &key_value_object = *key_value_objects_iter;

		// Attempt to extract the (key, value) 2-tuples/2-sequences.
		std::vector<bp::object> key_value_pair;
		extract_iterable(key_value_pair, key_value_object, type_error_string);

		if (key_value_pair.size() != 2)
		{
			PyErr_SetString(PyExc_TypeError, type_error_string);
			bp::throw_error_already_set();
		}

		key_value_map.push_back(
				std::make_pair(key_value_object[0], key_value_object[1]));
	}
}


void
GPlatesApi::PythonExtractUtils::Implementation::raise_type_error_if_iterator(
		bp::object iterable_object)
{
	// Calling builtin function 'next()' will work on an iterator, but not on a sequence.
	try
	{
		// Use python builtin next() function to see if iterable is an iterator.
		get_builtin_next()(iterable_object);
	}
	catch (const bp::error_already_set &)
	{
		PythonExceptionHandler handler;
		if (handler.exception_matches(PyExc_TypeError))
		{
			// It's not an iterator.
			return;
		}
		else if (handler.exception_matches(PyExc_StopIteration))
		{
			// It's an iterator that has stopped iteration (ie, there were no items to iterate over).
			// Let's fall through to our error message.
		}
		else
		{
			// It's some other error so let's restore it (this also throws boost::python::error_already_set).
			handler.restore_exception();
		}
	}

	PyErr_SetString(
			PyExc_TypeError,
			"Iterable must be a sequence (eg, list or tuple), not an iterator, since need more than one iteration pass");

	boost::python::throw_error_already_set();
}

#endif // GPLATES_NO_PYTHON
