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

#include <algorithm>
#include <iterator>
#include <utility>
#include <vector>
#include <boost/optional.hpp>

#include "global/python.h"
// This is not included by <boost/python.hpp>.
// Also we must include this after <boost/python.hpp> which means after "global/python.h".
#include <boost/python/stl_iterator.hpp>


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
		 * Raises Python exception 'TypeError' with the error message @a type_error_string on failure.
		 *
		 * The error message @a type_error_string should contain something like:
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


		/**
		 * Checks whether a Python iterable is a sequence (not iterator) of objects of
		 * type 'ObjectType' (such as a list or tuple).
		 *
		 * Raises Python exception 'TypeError' with the following error message
		 * if @a sequence_object is an iterator:
		 *
		 *   "Iterable must be a sequence (eg, list or tuple), not an iterator, since need more than one iteration pass"
		 *
		 * ...this is because an iterator can only be iterated over once and a successful
		 * call to @a check_sequence is followed by a call to @a extract_sequence
		 * (and both calls iterate over the iterable).
		 *
		 * Returns the number of items in sequence, or none if not a sequence of 'ObjectType'.
		 */
		template <typename ObjectType>
		boost::optional<unsigned int>
		check_sequence(
				boost::python::object sequence_object);


		/**
		 * Extracts objects of type 'ObjectType' from a Python sequence (not iterator).
		 *
		 * This assumes the sequence has been checked with @a check_sequence and hence should not fail.
		 */
		template <typename ObjectType>
		void
		extract_sequence(
				std::vector<ObjectType> &sequence,
				boost::python::object sequence_object);


		/**
		 * Extracts objects of type 'ObjectType' from a Python iterable
		 * (ie, any iterable such as a list sequence or a list iterator).
		 *
		 * Raises Python exception 'TypeError' with the error message @a type_error_string if
		 * @a iterable_object is not iterable or if it contains any items not convertible to 'ObjectType'.
		 *
		 * The error message @a type_error_string should contain something like:
		 *
		 *  "Expected an iterable of 'ObjectType'"
		 *
		 * Note that, unlike @a extract_sequence, the iterable object can be an iterator.
		 */
		template <typename ObjectType>
		void
		extract_iterable(
				std::vector<ObjectType> &iterable,
				boost::python::object iterable_object,
				const char *type_error_string);
	}
}

//
// Implementation
//

namespace GPlatesApi
{
	namespace PythonExtractUtils
	{
		namespace Implementation
		{
			void
			raise_type_error_if_iterator(
					boost::python::object iterable_object);
		}


		template <typename ObjectType>
		boost::optional<unsigned int>
		check_sequence(
				boost::python::object sequence_object)
		{
			// We don't want an iterator because we can't iterate over an iterator more than once.
			Implementation::raise_type_error_if_iterator(sequence_object);

			// Iterate over the sequence of 'ObjectType'.
			//
			// NOTE: We avoid iterating using 'boost::python::stl_input_iterator<ObjectType>'
			// because we want to avoid actually extracting the objects.
			// We're just checking if there's a sequence of 'ObjectType' here.
			boost::python::handle<> iterator =
					boost::python::handle<>(boost::python::allow_null(PyObject_GetIter(sequence_object.ptr())));
			if (!iterator)
			{
				PyErr_Clear(); // PyObject_GetIter() raises TypeError when returns NULL.
				return boost::none;
			}

			unsigned int num_items = 0;

			// Iterate using iterator and check that each item can be converted to a 'ObjectType'.
			while (boost::python::handle<> item =
					boost::python::handle<>(boost::python::allow_null(PyIter_Next(iterator.get()))))
			{
				if (!boost::python::extract<ObjectType>(item.get()).check())
				{
					return boost::none;
				}

				++num_items;
			}

			return num_items;
		}


		template <typename ObjectType>
		void
		extract_sequence(
				std::vector<ObjectType> &sequence,
				boost::python::object sequence_object)
		{
			// No error checking needed - caller should have called 'check_sequence()' first.
			std::copy(
					boost::python::stl_input_iterator<ObjectType>(sequence_object),
					boost::python::stl_input_iterator<ObjectType>(),
					std::back_inserter(sequence));
		}


		template <typename ObjectType>
		void
		extract_iterable(
				std::vector<ObjectType> &iterable,
				boost::python::object iterable_object,
				const char *type_error_string)
		{
			try
			{
				std::copy(
						boost::python::stl_input_iterator<ObjectType>(iterable_object),
						boost::python::stl_input_iterator<ObjectType>(),
						std::back_inserter(iterable));
			}
			catch (const boost::python::error_already_set &)
			{
				PyErr_Clear();

				PyErr_SetString(PyExc_TypeError, type_error_string);
				boost::python::throw_error_already_set();
			}
		}
	}
}

#endif   //GPLATES_NO_PYTHON)

#endif // GPLATES_API_PYTHONEXTRACTUTILS_H
