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
		 * What @a check_sequence does when given an iterator (which cannot be iterated over twice).
		 */
		enum IfIterator
		{
			// Raise Python 'TypeError' saying that a sequence is needed, not an iterator.
			// Suits a constructor, where the message tells the user what to pass instead.
			RAISE_TYPE_ERROR_IF_ITERATOR,

			// Return none (not a sequence), without raising.
			// Suits a query (such as 'contains_features()'), and a from-python converter's
			// 'convertible()', which must answer "no" rather than raise: a raise there stops
			// Boost.Python's overload resolution instead of letting it try the next overload.
			NOT_A_SEQUENCE_IF_ITERATOR
		};


		/**
		 * Checks whether a Python iterable is a sequence (not iterator) of objects of
		 * type 'ObjectType' (such as a list or tuple).
		 *
		 * If @a sequence_object is an iterator then, depending on @a if_iterator, either returns none
		 * or raises Python exception 'TypeError' with the following error message:
		 *
		 *   "Iterable must be a sequence (eg, list or tuple), not an iterator, since need more than one iteration pass"
		 *
		 * ...this is because an iterator can only be iterated over once and a successful
		 * call to @a check_sequence is followed by a call to @a extract_sequence
		 * (and both calls iterate over the iterable).
		 *
		 * An exception raised by the iteration itself (eg, by a user-defined '__next__') propagates.
		 *
		 * Returns the number of items in sequence, or none if not a sequence of 'ObjectType'.
		 */
		template <typename ObjectType>
		boost::optional<unsigned int>
		check_sequence(
				boost::python::object sequence_object,
				IfIterator if_iterator = RAISE_TYPE_ERROR_IF_ITERATOR);


		/**
		 * The same as @a check_sequence, except that each item is checked by @a is_item_convertible
		 * (a callable taking a 'boost::python::object' item and returning bool) instead of
		 * by 'boost::python::extract<ObjectType>(item).check()'.
		 *
		 * This lets a caller pass @a if_iterator down to the check of each item (eg, a sequence
		 * of sequences, where a nested iterator should also give none rather than raise).
		 */
		template <typename IsItemConvertibleType>
		boost::optional<unsigned int>
		check_sequence_items(
				boost::python::object sequence_object,
				const IsItemConvertibleType &is_item_convertible,
				IfIterator if_iterator = RAISE_TYPE_ERROR_IF_ITERATOR);


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
		 * Any other exception raised while iterating (eg, a 'ValueError' raised by a user's generator,
		 * or 'KeyboardInterrupt') propagates unchanged.
		 *
		 * This makes one pass over @a iterable_object.
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

			/**
			 * Returns the next item of @a iterator, or a null handle at the end of the iteration.
			 *
			 * If the iteration raised (eg, a user's '__next__' raised 'ValueError') then that
			 * exception is re-raised (as 'boost::python::error_already_set'), rather than being
			 * mistaken for the end of the iteration and left pending.
			 */
			boost::python::handle<>
			get_next_item(
					const boost::python::handle<> &iterator);


			template <typename ObjectType>
			class IsItemExtractable
			{
			public:
				bool
				operator()(
						const boost::python::object &item) const
				{
					return boost::python::extract<ObjectType>(item).check();
				}
			};
		}


		template <typename ObjectType>
		boost::optional<unsigned int>
		check_sequence(
				boost::python::object sequence_object,
				IfIterator if_iterator)
		{
			// NOTE: We avoid iterating using 'boost::python::stl_input_iterator<ObjectType>'
			// because we want to avoid actually extracting the objects.
			// We're just checking if there's a sequence of 'ObjectType' here.
			return check_sequence_items(
					sequence_object,
					Implementation::IsItemExtractable<ObjectType>(),
					if_iterator);
		}


		template <typename IsItemConvertibleType>
		boost::optional<unsigned int>
		check_sequence_items(
				boost::python::object sequence_object,
				const IsItemConvertibleType &is_item_convertible,
				IfIterator if_iterator)
		{
			// We don't want an iterator because we can't iterate over an iterator more than once.
			if (if_iterator == NOT_A_SEQUENCE_IF_ITERATOR)
			{
				if (PyIter_Check(sequence_object.ptr()))
				{
					return boost::none;
				}
			}
			else
			{
				Implementation::raise_type_error_if_iterator(sequence_object);
			}

			boost::python::handle<> iterator =
					boost::python::handle<>(boost::python::allow_null(PyObject_GetIter(sequence_object.ptr())));
			if (!iterator)
			{
				PyErr_Clear(); // PyObject_GetIter() raises TypeError when returns NULL.
				return boost::none;
			}

			unsigned int num_items = 0;

			// Iterate using iterator and check that each item is convertible.
			while (boost::python::handle<> item = Implementation::get_next_item(iterator))
			{
				if (!is_item_convertible(boost::python::object(item)))
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
			// NOTE: We iterate by hand, rather than with 'boost::python::stl_input_iterator<ObjectType>',
			// so that an exception raised by the iteration itself propagates unchanged, and only
			// "not iterable" and "item not convertible" become the caller's 'TypeError'.
			boost::python::handle<> iterator =
					boost::python::handle<>(boost::python::allow_null(PyObject_GetIter(iterable_object.ptr())));
			if (!iterator)
			{
				// PyObject_GetIter() raises TypeError when not iterable.
				// Anything else (eg, raised by a user's '__iter__') propagates.
				if (PyErr_ExceptionMatches(PyExc_TypeError))
				{
					PyErr_Clear();
					PyErr_SetString(PyExc_TypeError, type_error_string);
				}
				boost::python::throw_error_already_set();
			}

			while (boost::python::handle<> item = Implementation::get_next_item(iterator))
			{
				boost::python::extract<ObjectType> extract_item(item.get());
				if (!extract_item.check())
				{
					PyErr_SetString(PyExc_TypeError, type_error_string);
					boost::python::throw_error_already_set();
				}

				// The extraction itself can still fail after a successful check
				// (eg, a negative integer extracted as 'unsigned int' raises OverflowError).
				// That is a conversion failure too, so it also becomes the caller's 'TypeError'.
				try
				{
					iterable.push_back(extract_item());
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
}

#endif // GPLATES_API_PYTHONEXTRACTUTILS_H
