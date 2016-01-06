/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2014 The University of Sydney, Australia
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

#ifndef GPLATES_API_PYREVISIONEDVECTOR_H
#define GPLATES_API_PYREVISIONEDVECTOR_H

#include <algorithm>
#include <iterator>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/iterator/iterator_traits.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "PythonConverterUtils.h"
#include "PythonExtractUtils.h"
#include "PythonHashDefVisitor.h"

#include "global/CompilerWarnings.h"

#include "global/python.h"
// This is not included by <boost/python.hpp>.
// Also we must include this after <boost/python.hpp> which means after "global/python.h".
#include <boost/python/slice.hpp>

#include "model/RevisionedVector.h"


#if !defined(GPLATES_NO_PYTHON)

namespace GPlatesApi
{
	/**
	 * Adds boost::python::class_ def methods that treat a python class as if it was a python list
	 * by delegating to a RevisionedVector (which behaves like a python list).
	 *
	 * Template parameter 'ClassType' is the C++ class from which the revisioned vector is obtained
	 * via template parameter function 'GetRevisionedVectorFunction'.
	 * Template parameter 'PythonClassType' is the boost::python::class_ that wraps 'ClassType'.
	 * Template parameter 'PythonClassHeldType' is the 'HeldType' of the 'PythonClassType' boost::python::class_.
	 * Template parameter function 'ConvertClassToPointerFunction' converts a non-const reference
	 * of 'ClassType' to 'PythonClassHeldType'.
	 * Template parameter function 'CreateClassFromVectorFunction' creates a new 'PythonClassHeldType'
	 * from a RevisionedVector (also accepts a 'ClassType' object which is an original object -
	 * this is used for '__add__' and '__radd__' which create new 'PythonClassHeldType' objects).
	 */
	template <
			class ClassType,
			class RevisionableType,
			class PythonClassHeldType,
			PythonClassHeldType
					(*CreateClassFromVectorFunction)(
							typename GPlatesModel::RevisionedVector<RevisionableType>::non_null_ptr_type,
							const ClassType &),
			PythonClassHeldType
					(*ConvertClassToPointerFunction)(
							ClassType &),
			typename GPlatesModel::RevisionedVector<RevisionableType>::non_null_ptr_type
					(*GetRevisionedVectorFunction)(
							ClassType &),
			class PythonClassType>
	void
	wrap_python_class_as_revisioned_vector(
			PythonClassType &python_class,
			const char *class_name);


	/**
	 * Returns a string listing the operations supported by a python list in the form of
	 * reStructuredText (see http://sphinx-doc.org/rest.html) such that it can be added to a
	 * class docstring.
	 *
	 * The returned string looks like:
	 *
	 * "=========================== ==========================================================\n"
	 * "Operation                   Result\n"
	 * "=========================== ==========================================================\n"
	 * ...[list of operations]...
	 * "=========================== ==========================================================\n"
	 */
	std::string
	get_python_list_operations_docstring(
			const char *class_name);


	namespace Implementation
	{
		DISABLE_GCC_WARNING("-Wshadow")

		/**
		 * Adds 'def' methods to boost::python::class_ type 'PythonClass' to make the class
		 * behave like a python list.
		 *
		 * The implementation of the 'def' methods is provided by 'WrapperClassType' which should
		 * either be 'RevisionedVectorWrapper' or 'RevisionedVectorDelegateWrapper'.
		 */
		template <class WrapperClassType, class PythonClassType>
		void
		add_revisioned_vector_def_methods_to_python_class(
				PythonClassType &python_class)
		{
			namespace bp = boost::python;

			python_class
				.def("__iter__", &WrapperClassType::get_iter)
				.def("__len__", &WrapperClassType::len)
				.def("__setitem__", &WrapperClassType::set_item)
				.def("__delitem__", &WrapperClassType::delete_item)
				.def("__getitem__", &WrapperClassType::get_item)
				.def("__contains__", &WrapperClassType::contains)
				.def("__iadd__", &WrapperClassType::iadd) // +=
				// __add__ is useful for situations like 'list1 += list2 + list3' where list1 belongs
				// to a GpmlArray for example, but the addition 'list2 + list3' does not...
				.def("__add__", &WrapperClassType::add)
				.def("__radd__", &WrapperClassType::radd)
				.def("append",
						&WrapperClassType::append,
						(bp::arg("x")),
						"append(x)\n"
						"  Add element *x* to the end.\n")
				.def("extend",
						&WrapperClassType::extend,
						(bp::arg("t")),
						"extend(t)\n"
						"  Add the elements in sequence *t* to the end.\n")
				.def("insert",
						&WrapperClassType::insert,
						(bp::arg("i"), bp::arg("x")),
						"insert(i,x)\n"
						"  Insert element *x* at index *i*.\n")
				.def("remove",
						&WrapperClassType::remove,
						(bp::arg("x")),
						"remove(x)\n"
						"  Removes the first element that equals *x* (raises ``ValueError`` if not found).\n")
				.def("pop",
						&WrapperClassType::pop,
						(bp::arg("i")),
						"pop([i])\n"
						"  Removes the element at index *i* and returns it (defaults to last element).\n")
				.def("pop",
						&WrapperClassType::pop1,
						"\n") // A non-empty string otherwise boost-python will double docstring of other overload.
				.def("index",
						&WrapperClassType::index,
						(bp::arg("x"), bp::arg("i"), bp::arg("j")),
						"index(x[,i[,j]])\n"
						"  Smallest *k* such that the *k* th element equals ``x`` and ``i <= k < j`` (raises ``ValueError`` if not found).\n")
				.def("index",
						&WrapperClassType::index2,
						(bp::arg("x")),
						"\n") // A non-empty string otherwise boost-python will double docstring of other overload.
				.def("index",
						&WrapperClassType::index3,
						(bp::arg("x"), bp::arg("i")),
						"\n") // A non-empty string otherwise boost-python will double docstring of other overload.
				.def("count",
						&WrapperClassType::count,
						(bp::arg("x")),
						"count(x)\n"
						"  Number of occurrences of *x*.\n")
				.def("reverse",
						&WrapperClassType::reverse,
						"reverse()\n"
						"  Reverses the items in place.\n")
				.def("sort",
						&WrapperClassType::sort,
						(bp::arg("key"), bp::arg("reverse")=false),
						"sort(key[,reverse])\n"
						"  Sort the items in place (note that *key* is **not** optional and, like python 3.0, we removed *cmp*).\n")
			;
		}


		/**
		 * Implements wrapped functions for @a RevisionedVector and provides the wrapped class definition.
		 *
		 * The python interface matches that of python's list class such that elements can be added and
		 * removed using slices or indices.
		 *
		 * Template parameter 'TypeTag' is used purely to avoid potential overlap of boost::python::class_
		 * definitions when RevisionedVectorWrapper and RevisionedVectorDelegateWrapper happen to be
		 * instantiated with the same 'RevisionableType' - boost python might then complain of multiple
		 * to-python converters for the same C++ type (such as the nested Iterator type).
		 */
		template <class RevisionableType, class TypeTag=void>
		class RevisionedVectorWrapper
		{
		public:

			//! Typedef for revisioned vector non_null_ptr_type.
			typedef typename GPlatesModel::RevisionedVector<RevisionableType>::non_null_ptr_type
					revisioned_vector_non_null_ptr_type;

			//! Typedef for revisioned vector element type.
			typedef typename GPlatesModel::RevisionedVector<RevisionableType>::element_type element_type;

			//! Typedef for revisioned vector size type.
			typedef typename GPlatesModel::RevisionedVector<RevisionableType>::size_type size_type;


			/**
			 * Wrap a template instantiation of @a RevisionedVector to expose to Python.
			 */
			static
			void
			wrap(
					const char *element_class_name);

		private:

			//! Typedef for this type.
			typedef RevisionedVectorWrapper<RevisionableType, TypeTag> this_type;

			//! Typedef for revisioned vector iterator type.
			typedef typename GPlatesModel::RevisionedVector<RevisionableType>::iterator iterator_type;

			//! Typedef for revisioned vector iterator difference type.
			typedef typename boost::iterator_difference<iterator_type>::type iterator_difference_type;


			/**
			 * Sort key for the @a sort function sorts based on the result of converting 'element_type'
			 * to an unknown key type (done by a 'key' function provided on the python side).
			 */
			struct SortKey
			{
				bool
				operator()(
						const std::pair<boost::python::object/*key*/, element_type> &lhs,
						const std::pair<boost::python::object/*key*/, element_type> &rhs) const
				{
					// Use '__lt__', otherwise '__cmp__', for comparison.
					// Python 'int' has no '__lt__', but 'float' does.
					if (PyObject_HasAttrString(lhs.first.ptr(), "__lt__"))
					{
						return boost::python::extract<bool>(lhs.first.attr("__lt__")(rhs.first));
					}
					return boost::python::extract<int>(lhs.first.attr("__cmp__")(rhs.first)) < 0;
				}
			};

		public:

			/**
			 * An iterator that checks the index is dereferenceable - we don't use bp::iterator in order
			 * to avoid issues with C++ iterators being invalidated from the C++ side and causing issues
			 * on the python side (eg, removing container elements on the C++ side while iterating over
			 * the container on the python side, for example, via a pygplates call back into the C++ code).
			 */
			class Iterator
			{
			public:

				explicit
				Iterator(
						const revisioned_vector_non_null_ptr_type &revisioned_vector) :
					d_revisioned_vector(revisioned_vector),
					d_index(0)
				{  }

				Iterator &
				self()
				{
					return *this;
				}

				element_type
				next()
				{
					if (d_index >= d_revisioned_vector->size())
					{
						PyErr_SetString(PyExc_StopIteration, "No more data.");
						boost::python::throw_error_already_set();
					}
					return (*d_revisioned_vector)[d_index++];
				}

			private:
				revisioned_vector_non_null_ptr_type d_revisioned_vector;
				size_type d_index;
			};


			static
			Iterator
			get_iter(
					revisioned_vector_non_null_ptr_type revisioned_vector)
			{
				return Iterator(revisioned_vector);
			}

			static
			std::size_t
			len(
					revisioned_vector_non_null_ptr_type revisioned_vector)
			{
				return revisioned_vector->size();
			}


			static
			void
			set_item(
					revisioned_vector_non_null_ptr_type revisioned_vector,
					boost::python::object i,
					boost::python::object v);

			static
			void
			delete_item(
					revisioned_vector_non_null_ptr_type revisioned_vector,
					boost::python::object i);

			static
			boost::python::object
			get_item(
					revisioned_vector_non_null_ptr_type revisioned_vector,
					boost::python::object i);

			static
			bool
			contains(
					revisioned_vector_non_null_ptr_type revisioned_vector,
					boost::python::object element_object);

			static
			revisioned_vector_non_null_ptr_type
			add(
					revisioned_vector_non_null_ptr_type revisioned_vector,
					boost::python::object sequence);

			static
			revisioned_vector_non_null_ptr_type
			radd(
					revisioned_vector_non_null_ptr_type revisioned_vector,
					boost::python::object sequence);

			static
			revisioned_vector_non_null_ptr_type
			iadd(
					revisioned_vector_non_null_ptr_type revisioned_vector,
					boost::python::object sequence);

			static
			void
			append(
					revisioned_vector_non_null_ptr_type revisioned_vector,
					const element_type &element);

			static
			void
			extend(
					revisioned_vector_non_null_ptr_type revisioned_vector,
					boost::python::object sequence);

			static
			void
			insert(
					revisioned_vector_non_null_ptr_type revisioned_vector,
					long index,
					const element_type &element);

			static
			void
			remove(
					revisioned_vector_non_null_ptr_type revisioned_vector,
					boost::python::object element_object);

			static
			element_type
			pop(
					revisioned_vector_non_null_ptr_type revisioned_vector,
					// Defaults to popping the last element...
					long index = -1);

			// Default argument overloads of 'pop()'.
			// Avoiding BOOST_PYTHON_FUNCTION_OVERLOADS since it generates 'keywords' shadow warning.
			static
			element_type
			pop1(
					revisioned_vector_non_null_ptr_type revisioned_vector)
			{
				return pop(revisioned_vector);
			}

			static
			long
			index(
					revisioned_vector_non_null_ptr_type revisioned_vector,
					// Using bp::object instead of 'element_type' since need to throw ValueError if wrong type...
					boost::python::object element_object,
					// Defaults to first element...
					long i = 0,
					// Defaults to one past last element...
					long j = 0);

			// Default argument overloads of 'index()'.
			// Avoiding BOOST_PYTHON_FUNCTION_OVERLOADS since it generates 'keywords' shadow warning.
			static
			long
			index2(
					revisioned_vector_non_null_ptr_type revisioned_vector,
					// Using bp::object instead of 'element_type' since need to throw ValueError if wrong type...
					boost::python::object element_object)
			{
				return index(revisioned_vector, element_object);
			}
			static
			long
			index3(
					revisioned_vector_non_null_ptr_type revisioned_vector,
					// Using bp::object instead of 'element_type' since need to throw ValueError if wrong type...
					boost::python::object element_object,
					long i)
			{
				return index(revisioned_vector, element_object, i);
			}

			static
			int
			count(
					revisioned_vector_non_null_ptr_type revisioned_vector,
					const element_type &element);

			static
			void
			reverse(
					revisioned_vector_non_null_ptr_type revisioned_vector);

			static
			void
			sort(
					revisioned_vector_non_null_ptr_type revisioned_vector,
					boost::python::object key_callable,
					bool reverse_);

		private:

			/**
			 * Helper function to get the slice range from a slice object.
			 *
			 * Returns boost::none if slice range would be empty.
			 */
			static
			boost::optional<boost::python::slice::range<iterator_type> >
			get_slice_range(
					revisioned_vector_non_null_ptr_type revisioned_vector,
					boost::python::slice slice);

			/**
			 * Helper function to check the integer index (and adjust if negative index).
			 *
			 * @a allow_index_to_last_element_plus_one is useful for getting indices of end iterators.
			 *
			 * Throws IndexError exception if index is out-of-range.
			 */
			static
			long
			get_index(
					revisioned_vector_non_null_ptr_type revisioned_vector,
					long index,
					bool allow_index_to_last_element_plus_one = false);

		};

		ENABLE_GCC_WARNING("-Wshadow")


		template <class RevisionableType, class TypeTag>
		void
		RevisionedVectorWrapper<RevisionableType, TypeTag>::wrap(
				const char *element_class_name)
		{
			namespace bp = boost::python;

			// Since the RevisionVector wrapper behaves just like a python built-in list we will
			// suffix it with 'List'.
			std::string class_name = std::string(element_class_name) + "List";

			// The name of the list iterator class.
			std::string iterator_class_name = class_name + "Iterator";

			// Note: We don't docstring this - it's not an interface the python user needs to know about.
			bp::class_<Iterator>(iterator_class_name.c_str(), bp::no_init)
				.def("__iter__", &Iterator::self, bp::return_value_policy<bp::copy_non_const_reference>())
				.def("next", &Iterator::next)
			;

			std::stringstream class_docstring_stream;
			class_docstring_stream <<
					"A list of :class:`" << element_class_name << "` instances. The list behaves like a regular "
					"python ``list`` in that the following operations are supported:\n"
					"\n"
					<< GPlatesApi::get_python_list_operations_docstring(class_name.c_str())
					<< "\n";

			//
			// RevisionedVector - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
			//
			bp::class_<
					GPlatesModel::RevisionedVector<RevisionableType>,
					typename GPlatesModel::RevisionedVector<RevisionableType>::non_null_ptr_type,
					boost::noncopyable> revisioned_vector_class(
						class_name.c_str(),
						class_docstring_stream.str().c_str(),
						bp::no_init);

			// Add the 'def' methods.
			add_revisioned_vector_def_methods_to_python_class<this_type>(revisioned_vector_class);

			// Also add equality comparison operators.
			//
			// Due to the numerical tolerance in comparisons we cannot make hashable.
			// Make unhashable, with no *equality* comparison operators (we explicitly define them)...
			revisioned_vector_class
					.def(GPlatesApi::NoHashDefVisitor(false, true))
					.def(bp::self == bp::self)
					.def(bp::self != bp::self)
			;
		}


		template <class RevisionableType, class TypeTag>
		void
		RevisionedVectorWrapper<RevisionableType, TypeTag>::set_item(
				revisioned_vector_non_null_ptr_type revisioned_vector,
				boost::python::object i,
				boost::python::object v)
		{
			namespace bp = boost::python;
			using namespace GPlatesModel;

			// Set if the index is a slice object.
			bp::extract<bp::slice> extract_slice(i);
			if (extract_slice.check())
			{
				// Copy the new elements into a vector so we can count the number of new elements.
				std::vector<element_type> new_elements;
				PythonExtractUtils::extract_iterable(new_elements, v, "Expected a sequence of elements");

				const bp::slice slice_object = extract_slice;
				boost::optional<bp::slice::range<iterator_type> >
						slice_range = get_slice_range(revisioned_vector, slice_object);
				if (!slice_range)
				{
					// Empty range, so delete nothing.
					// But still need to insert new elements at the correct location.

					// Since bp::slice::get_indices(), in 'get_slice_range()', doesn't return
					// empty slices we have to explicitly look at start, stop and step ourselves.
					bp::object slice_start = slice_object.start();
					bp::object slice_step = slice_object.step();

					if (slice_step != bp::object() &&
						bp::extract<long>(slice_step)() != 1)
					{
						// Note the step size has already been checked for zero by 'get_slice_range()'.
						std::stringstream error_string_stream;
						error_string_stream << "attempt to assign sequence of size "
								<< new_elements.size()
								<< " to extended slice of size 0";

						PyErr_SetString(PyExc_ValueError, error_string_stream.str().c_str());
						bp::throw_error_already_set();
					}
					// ...else step size is 1 (which is OK for inserting - anything else isn't).

					iterator_type insert_iter = revisioned_vector->begin();

					if (slice_start == bp::object())
					{
						insert_iter = revisioned_vector->begin();
					}
					else
					{
						const iterator_difference_type max_dist =
								std::distance(revisioned_vector->begin(), revisioned_vector->end());

						const iterator_difference_type start = bp::extract<long>(slice_start);
						// If at, or past, the end then set to the end.
						if (start >= max_dist)
						{
							insert_iter = revisioned_vector->end();
						}
						else if (start >= 0)
						{
							// Advance towards end.
							insert_iter = revisioned_vector->begin();
							std::advance(insert_iter, start);
						}
						else if (start >= -max_dist)
						{
							// Advance towards beginning.
							insert_iter = revisioned_vector->end();
							std::advance(insert_iter, start);
						}
						else
						{
							insert_iter = revisioned_vector->begin();
						}
					}

					// Insert the new elements.
					revisioned_vector->insert(
							insert_iter,
							new_elements.begin(),
							new_elements.end());
					return;
				}

				// If single (forward) step then can erase range in one call.
				if (slice_range->step == iterator_difference_type(1))
				{
					iterator_type erase_begin = slice_range->start;
					iterator_type erase_end = slice_range->stop;
					// Convert from closed range to half-open range.
					std::advance(erase_end, 1);

					// Erase the existing elements and insert the new elements.
					revisioned_vector->insert(
							revisioned_vector->erase(erase_begin, erase_end),
							new_elements.begin(),
							new_elements.end());

					return;
				}

				// Count the number of elements in the extended slice.
				std::size_t extended_slice_size = 1; 
				iterator_type iter = slice_range->start;
				for ( ; iter != slice_range->stop; std::advance(iter, slice_range->step))
				{
					++extended_slice_size;
				}

				if (new_elements.size() != extended_slice_size)
				{
					std::stringstream error_string_stream;
					error_string_stream << "attempt to assign sequence of size "
							<< new_elements.size()
							<< " to extended slice of size " << extended_slice_size;

					PyErr_SetString(PyExc_ValueError, error_string_stream.str().c_str());
					bp::throw_error_already_set();
					return;
				}

				// Assign the new elements to the revisioned vector.
				typename std::vector<element_type>::const_iterator new_elements_iter = new_elements.begin();
				iter = slice_range->start;
				for ( ; iter != slice_range->stop; std::advance(iter, slice_range->step))
				{
					*iter = *new_elements_iter++;
				}
				*iter = *new_elements_iter++;

				return;
			}

			// See if index is an integer.
			bp::extract<long> extract_index(i);
			if (extract_index.check())
			{
				long index = get_index(revisioned_vector, extract_index());

				// Attempt to extract the element.
				bp::extract<element_type> extract_element(v);
				if (!extract_element.check())
				{
					PyErr_SetString(PyExc_TypeError, "Invalid assignment");
					bp::throw_error_already_set();
					return;
				}
				element_type element = extract_element();

				// Set the new element.
				(*revisioned_vector)[index] = element;

				return;
			}

			PyErr_SetString(PyExc_TypeError, "Invalid index type");
			bp::throw_error_already_set();
		}


		template <class RevisionableType, class TypeTag>
		void
		RevisionedVectorWrapper<RevisionableType, TypeTag>::delete_item(
				revisioned_vector_non_null_ptr_type revisioned_vector,
				boost::python::object i)
		{
			namespace bp = boost::python;
			using namespace GPlatesModel;

			// Set if the index is a slice object.
			bp::extract<bp::slice> extract_slice(i);
			if (extract_slice.check())
			{
				boost::optional<bp::slice::range<iterator_type> >
						slice_range = get_slice_range(revisioned_vector, extract_slice());
				if (!slice_range)
				{
					// Empty range - nothing to delete.
					return;
				}

				// If single (forward) step then can erase range in one call.
				if (slice_range->step == iterator_difference_type(1))
				{
					iterator_type erase_begin = slice_range->start;
					iterator_type erase_end = slice_range->stop;
					// Convert from closed range to half-open range.
					std::advance(erase_end, 1);

					// Erase the range of elements.
					revisioned_vector->erase(erase_begin, erase_end);

					return;
				}

				// Always iterate backwards since it's easier to erase without invalidating iterators.
				if (slice_range->step > 0)
				{
					slice_range->step = -slice_range->step;
					// Swap the start and stop iterator.
					iterator_type tmp = slice_range->start;
					slice_range->start = slice_range->stop;
					slice_range->stop = tmp;
				}

				iterator_type iter = slice_range->start;
				for ( ; iter != slice_range->stop; std::advance(iter, slice_range->step))
				{
					iter = revisioned_vector->erase(iter);
				}
				revisioned_vector->erase(iter);

				return;
			}

			// See if the index is an integer.
			bp::extract<long> extract_index(i);
			if (extract_index.check())
			{
				long index = get_index(revisioned_vector, extract_index());

				// Erase the element.
				iterator_type erase_iter = revisioned_vector->begin();
				std::advance(erase_iter, index);
				revisioned_vector->erase(erase_iter);

				return;
			}

			PyErr_SetString(PyExc_TypeError, "Invalid index type");
			bp::throw_error_already_set();
		}


		template <class RevisionableType, class TypeTag>
		boost::python::object
		RevisionedVectorWrapper<RevisionableType, TypeTag>::get_item(
				revisioned_vector_non_null_ptr_type revisioned_vector,
				boost::python::object i)
		{
			namespace bp = boost::python;
			using namespace GPlatesModel;

			// Set if the index is a slice object.
			bp::extract<bp::slice> extract_slice(i);
			if (extract_slice.check())
			{
				boost::optional<bp::slice::range<iterator_type> >
						slice_range = get_slice_range(revisioned_vector, extract_slice());
				if (!slice_range)
				{
					// Empty range - return empty list.
					return bp::list();
				}

				bp::list slice_list;

				iterator_type iter = slice_range->start;
				for ( ; iter != slice_range->stop; std::advance(iter, slice_range->step))
				{
					slice_list.append(iter->get());
				}
				slice_list.append(iter->get());

				return slice_list;
			}

			// See if the index is an integer.
			bp::extract<long> extract_index(i);
			if (extract_index.check())
			{
				long index = get_index(revisioned_vector, extract_index());

				element_type element = (*revisioned_vector)[index].get();
				return bp::object(element);
			}

			PyErr_SetString(PyExc_TypeError, "Invalid index type");
			bp::throw_error_already_set();

			return bp::object();
		}


		template <class RevisionableType, class TypeTag>
		bool
		RevisionedVectorWrapper<RevisionableType, TypeTag>::contains(
				revisioned_vector_non_null_ptr_type revisioned_vector,
				boost::python::object element_object)
		{
			namespace bp = boost::python;
			using namespace GPlatesModel;

			bp::extract<element_type> extract_element(element_object);
			if (!extract_element.check())
			{
				return false;
			}
			element_type element = extract_element();

			// Search the revisioned vector for the element.
			BOOST_FOREACH(element_type revisioned_element, *revisioned_vector)
			{
				// Compare the pointed-to elements, not the pointers.
				if (*revisioned_element == *element)
				{
					return true;
				}
			}

			return false;
		}


		template <class RevisionableType, class TypeTag>
		typename RevisionedVectorWrapper<RevisionableType, TypeTag>::revisioned_vector_non_null_ptr_type
		RevisionedVectorWrapper<RevisionableType, TypeTag>::add(
				revisioned_vector_non_null_ptr_type revisioned_vector,
				boost::python::object sequence)
		{
			namespace bp = boost::python;

			revisioned_vector_non_null_ptr_type concat_revisioned_vector = revisioned_vector->clone();

			std::vector<element_type> new_elements;
			PythonExtractUtils::extract_iterable(new_elements, sequence, "Expected a sequence of elements");

			// Insert the new elements.
			concat_revisioned_vector->insert(
					// Insert at the *end* ...
					concat_revisioned_vector->end(),
					new_elements.begin(),
					new_elements.end());

			return concat_revisioned_vector;
		}


		template <class RevisionableType, class TypeTag>
		typename RevisionedVectorWrapper<RevisionableType, TypeTag>::revisioned_vector_non_null_ptr_type
		RevisionedVectorWrapper<RevisionableType, TypeTag>::radd(
				revisioned_vector_non_null_ptr_type revisioned_vector,
				boost::python::object sequence)
		{
			namespace bp = boost::python;

			revisioned_vector_non_null_ptr_type concat_revisioned_vector = revisioned_vector->clone();

			std::vector<element_type> new_elements;
			PythonExtractUtils::extract_iterable(new_elements, sequence, "Expected a sequence of elements");

			// Insert the new elements.
			concat_revisioned_vector->insert(
					// Insert at the *beginning* ...
					concat_revisioned_vector->begin(),
					new_elements.begin(),
					new_elements.end());

			return concat_revisioned_vector;
		}


		template <class RevisionableType, class TypeTag>
		typename RevisionedVectorWrapper<RevisionableType, TypeTag>::revisioned_vector_non_null_ptr_type
		RevisionedVectorWrapper<RevisionableType, TypeTag>::iadd(
				revisioned_vector_non_null_ptr_type revisioned_vector,
				boost::python::object sequence)
		{
			namespace bp = boost::python;

			std::vector<element_type> new_elements;
			PythonExtractUtils::extract_iterable(new_elements, sequence, "Expected a sequence of elements");

			// Insert the new elements.
			revisioned_vector->insert(
					// Insert at the *end* ...
					revisioned_vector->end(),
					new_elements.begin(),
					new_elements.end());

			return revisioned_vector;
		}


		template <class RevisionableType, class TypeTag>
		void
		RevisionedVectorWrapper<RevisionableType, TypeTag>::append(
				revisioned_vector_non_null_ptr_type revisioned_vector,
				const element_type &element)
		{
			revisioned_vector->push_back(element);
		}

		template <class RevisionableType, class TypeTag>
		void
		RevisionedVectorWrapper<RevisionableType, TypeTag>::extend(
				revisioned_vector_non_null_ptr_type revisioned_vector,
				boost::python::object sequence)
		{
			iadd(revisioned_vector, sequence);
		}


		template <class RevisionableType, class TypeTag>
		void
		RevisionedVectorWrapper<RevisionableType, TypeTag>::insert(
				revisioned_vector_non_null_ptr_type revisioned_vector,
				long index,
				const element_type &element)
		{
			// It's OK to use an 'end' iterator when inserting.
			index = get_index(revisioned_vector, index, true/*allow_index_to_last_element_plus_one*/);

			// Create an iterator referencing the 'index'th element.
			iterator_type iter = revisioned_vector->begin();
			std::advance(iter, index);

			revisioned_vector->insert(iter, element);
		}


		template <class RevisionableType, class TypeTag>
		void
		RevisionedVectorWrapper<RevisionableType, TypeTag>::remove(
				revisioned_vector_non_null_ptr_type revisioned_vector,
				boost::python::object element_object)
		{
			namespace bp = boost::python;

			const long element_index = index(revisioned_vector, element_object);

			// Create an iterator referencing the 'element_index'th element.
			iterator_type iter = revisioned_vector->begin();
			std::advance(iter, element_index);

			revisioned_vector->erase(iter);
		}


		template <class RevisionableType, class TypeTag>
		typename RevisionedVectorWrapper<RevisionableType, TypeTag>::element_type
		RevisionedVectorWrapper<RevisionableType, TypeTag>::pop(
				revisioned_vector_non_null_ptr_type revisioned_vector,
				long index)
		{
			index = get_index(revisioned_vector, index);

			element_type element = (*revisioned_vector)[index];

			// Create an iterator referencing the 'index'th element.
			iterator_type iter = revisioned_vector->begin();
			std::advance(iter, index);

			revisioned_vector->erase(iter);

			return element;
		}


		template <class RevisionableType, class TypeTag>
		long
		RevisionedVectorWrapper<RevisionableType, TypeTag>::index(
				revisioned_vector_non_null_ptr_type revisioned_vector,
				boost::python::object element_object,
				long i,
				long j)
		{
			namespace bp = boost::python;

			bp::extract<element_type> extract_element(element_object);
			if (!extract_element.check())
			{
				PyErr_SetString(PyExc_TypeError, "Invalid type");
				bp::throw_error_already_set();
			}
			element_type element = extract_element();

			// A zero value for 'j' indicates one past the last element.
			if (j == 0)
			{
				j += revisioned_vector->size();
			}

			if (i >= j)
			{
				PyErr_SetString(PyExc_IndexError, "Invalid index range");
				bp::throw_error_already_set();
			}

			i = get_index(revisioned_vector, i);
			j = get_index(revisioned_vector, j, true/*allow_index_to_last_element_plus_one*/);

			// Search the specified index range for the specified element.
			for (long k = i; k < j; ++k)
			{
				element_type revisioned_element = (*revisioned_vector)[k];

				// Compare the pointed-to elements, not the pointers.
				if (*revisioned_element == *element)
				{
					return k;
				}
			}

			PyErr_SetString(PyExc_ValueError, "Value not found");
			bp::throw_error_already_set();

			return -1; // Shouldn't be able to get here - but keep compiler happy.
		}


		template <class RevisionableType, class TypeTag>
		int
		RevisionedVectorWrapper<RevisionableType, TypeTag>::count(
				revisioned_vector_non_null_ptr_type revisioned_vector,
				const element_type &element)
		{
			int num_matches = 0;

			// Search the revisioned vector for the element.
			BOOST_FOREACH(element_type revisioned_element, *revisioned_vector)
			{
				// Compare the pointed-to elements, not the pointers.
				if (*revisioned_element == *element)
				{
					++num_matches;
				}
			}

			return num_matches;
		}


		template <class RevisionableType, class TypeTag>
		void
		RevisionedVectorWrapper<RevisionableType, TypeTag>::reverse(
				revisioned_vector_non_null_ptr_type revisioned_vector)
		{
			// Reverse the existing elements.
			std::vector<element_type> reversed_elements;
			std::copy(revisioned_vector->begin(), revisioned_vector->end(), std::back_inserter(reversed_elements));
			std::reverse(reversed_elements.begin(), reversed_elements.end());

			// Assign the reversed elements in one go (in one revision update).
			revisioned_vector->assign(reversed_elements.begin(), reversed_elements.end());
		}


		template <class RevisionableType, class TypeTag>
		void
		RevisionedVectorWrapper<RevisionableType, TypeTag>::sort(
				revisioned_vector_non_null_ptr_type revisioned_vector,
				boost::python::object key_callable,
				bool reverse_)
		{
			namespace bp = boost::python;

			typedef std::pair<bp::object/*key*/, element_type> key_element_type;
			std::vector<key_element_type> sorted_keys;

			// Decorate the elements with their keys...
			// Extract the existing elements and associate with their key
			// provided by the python 'key_callable' function.
			BOOST_FOREACH(element_type element, *revisioned_vector)
			{
				// Convert the element to its key.
				boost::python::object key = key_callable(element);
				sorted_keys.push_back(key_element_type(key, element));
			}

			// Sort the elements by their key.
			std::stable_sort(sorted_keys.begin(), sorted_keys.end(), SortKey());

			// Un-decorate the elements (remove the keys).
			std::vector<element_type> sorted_elements;
			BOOST_FOREACH(const key_element_type &key, sorted_keys)
			{
				sorted_elements.push_back(key.second);
			}

			if (reverse_)
			{
				std::reverse(sorted_elements.begin(), sorted_elements.end());
			}

			// Assign the sorted elements in one go (in one revision update).
			revisioned_vector->assign(sorted_elements.begin(), sorted_elements.end());
		}


		template <class RevisionableType, class TypeTag>
		boost::optional<
				boost::python::slice::range<
						typename RevisionedVectorWrapper<RevisionableType, TypeTag>::iterator_type> >
		RevisionedVectorWrapper<RevisionableType, TypeTag>::get_slice_range(
				revisioned_vector_non_null_ptr_type revisioned_vector,
				boost::python::slice slice)
		{
			// Use boost::python::slice to manage index variations such as negative indices or
			// indices that are None.
			try
			{
				return slice.get_indicies(revisioned_vector->begin(), revisioned_vector->end());
			}
			catch (std::invalid_argument)
			{
				// The range was empty.
			}

			// Empty range.
			return boost::none;
		}


		template <class RevisionableType, class TypeTag>
		long
		RevisionedVectorWrapper<RevisionableType, TypeTag>::get_index(
				revisioned_vector_non_null_ptr_type revisioned_vector,
				long index,
				bool allow_index_to_last_element_plus_one)
		{
			namespace bp = boost::python;

			if (index < 0)
			{
				index += revisioned_vector->size();
			}

			long highest_allowed_index = revisioned_vector->size();
			if (!allow_index_to_last_element_plus_one)
			{
				--highest_allowed_index;
			}

			if (index > highest_allowed_index ||
				index < 0)
			{
				PyErr_SetString(PyExc_IndexError, "Index out of range");
				bp::throw_error_already_set();
			}

			return index;
		}


		/**
		 * Implements wrapped functions for types that wish to behave like a python list by
		 * delegating to a contained @a RevisionedVector.
		 *
		 * See @a wrap_python_class_as_revisioned_vector for more details on the template parameters.
		 */
		template <
				class ClassType,
				class RevisionableType,
				class PythonClassHeldType,
				PythonClassHeldType
						(*CreateClassFromVectorFunction)(
								typename GPlatesModel::RevisionedVector<RevisionableType>::non_null_ptr_type,
								const ClassType &),
				PythonClassHeldType
						(*ConvertClassToPointerFunction)(
								ClassType &),
				typename GPlatesModel::RevisionedVector<RevisionableType>::non_null_ptr_type
						(*GetRevisionedVectorFunction)(ClassType &)>
		class RevisionedVectorDelegateWrapper
		{
		public:

			/**
			 * Typedef for revisioned vector wrapper.
			 *
			 * Using 'ClassType' for 'TypeTag' to ensure 'RevisionedVectorWrapper<...>' type does not clash.
			 */
			typedef RevisionedVectorWrapper<RevisionableType, ClassType> revisioned_vector_wrapper_type;

			//! Typedef for revisioned vector non_null_ptr_type.
			typedef typename revisioned_vector_wrapper_type::revisioned_vector_non_null_ptr_type
					revisioned_vector_non_null_ptr_type;

			//! Typedef for revisioned vector element type.
			typedef typename revisioned_vector_wrapper_type::element_type element_type;

			//! Typedef for revisioned vector size type.
			typedef typename revisioned_vector_wrapper_type::size_type size_type;

			//! Typedef for class type.
			typedef ClassType class_type;


			/**
			 * Wrap @a python_class such that is appears like a python list.
			 */
			template <class PythonClassType>
			static
			void
			wrap(
					PythonClassType &python_class,
					const char *class_name)
			{
				namespace bp = boost::python;

				// The name of the list iterator class.
				std::string iterator_class_name = std::string(class_name) + "Iterator";

				typedef typename revisioned_vector_wrapper_type::Iterator iterator_type;

				// Note: We don't docstring this - it's not an interface the python user needs to know about.
				bp::class_<iterator_type>(iterator_class_name.c_str(), bp::no_init)
					.def("__iter__", &iterator_type::self, bp::return_value_policy<bp::copy_non_const_reference>())
					.def("next", &iterator_type::next)
				;

				// Add the 'def' methods to 'python_class'.
				add_revisioned_vector_def_methods_to_python_class<this_type>(python_class);
			}

		private:

			//! Typedef for this type.
			typedef RevisionedVectorDelegateWrapper<
					ClassType,
					RevisionableType,
					PythonClassHeldType,
					CreateClassFromVectorFunction,
					ConvertClassToPointerFunction,
					GetRevisionedVectorFunction>
							this_type;

		public:

			static
			typename revisioned_vector_wrapper_type::Iterator
			get_iter(
					class_type &class_instance)
			{
				return typename revisioned_vector_wrapper_type::Iterator(
						GetRevisionedVectorFunction(class_instance));
			}

			static
			size_type
			len(
					class_type &class_instance)
			{
				return GetRevisionedVectorFunction(class_instance)->size();
			}

			static
			void
			set_item(
					class_type &class_instance,
					boost::python::object i,
					boost::python::object v)
			{
				revisioned_vector_wrapper_type::set_item(
						GetRevisionedVectorFunction(class_instance), i, v);
			}

			static
			void
			delete_item(
					class_type &class_instance,
					boost::python::object i)
			{
				revisioned_vector_wrapper_type::delete_item(
						GetRevisionedVectorFunction(class_instance), i);
			}

			static
			boost::python::object
			get_item(
					class_type &class_instance,
					boost::python::object i)
			{
				return revisioned_vector_wrapper_type::get_item(
						GetRevisionedVectorFunction(class_instance), i);
			}

			static
			bool
			contains(
					class_type &class_instance,
					boost::python::object element_object)
			{
				return revisioned_vector_wrapper_type::contains(
						GetRevisionedVectorFunction(class_instance), element_object);
			}

			static
			PythonClassHeldType
			add(
					class_type &class_instance,
					boost::python::object sequence)
			{
				const revisioned_vector_non_null_ptr_type revisioned_vector =
						revisioned_vector_wrapper_type::add(
								GetRevisionedVectorFunction(class_instance), sequence);

				return CreateClassFromVectorFunction(revisioned_vector, class_instance);
			}

			static
			PythonClassHeldType
			radd(
					class_type &class_instance,
					boost::python::object sequence)
			{
				const revisioned_vector_non_null_ptr_type revisioned_vector =
						revisioned_vector_wrapper_type::radd(
								GetRevisionedVectorFunction(class_instance), sequence);

				return CreateClassFromVectorFunction(revisioned_vector, class_instance);
			}

			static
			PythonClassHeldType
			iadd(
					class_type &class_instance,
					boost::python::object sequence)
			{
				revisioned_vector_wrapper_type::iadd(
						GetRevisionedVectorFunction(class_instance), sequence);

				return ConvertClassToPointerFunction(class_instance);
			}

			static
			void
			append(
					class_type &class_instance,
					const element_type &element)
			{
				revisioned_vector_wrapper_type::append(
						GetRevisionedVectorFunction(class_instance), element);
			}

			static
			void
			extend(
					class_type &class_instance,
					boost::python::object sequence)
			{
				revisioned_vector_wrapper_type::extend(
						GetRevisionedVectorFunction(class_instance), sequence);
			}

			static
			void
			insert(
					class_type &class_instance,
					long index,
					const element_type &element)
			{
				revisioned_vector_wrapper_type::insert(
						GetRevisionedVectorFunction(class_instance), index, element);
			}

			static
			void
			remove(
					class_type &class_instance,
					boost::python::object element_object)
			{
				revisioned_vector_wrapper_type::remove(
						GetRevisionedVectorFunction(class_instance), element_object);
			}

			static
			element_type
			pop(
					class_type &class_instance,
					// Defaults to popping the last element...
					long index = -1)
			{
				return revisioned_vector_wrapper_type::pop(
						GetRevisionedVectorFunction(class_instance), index);
			}

			// Default argument overloads of 'pop()'.
			// Avoiding BOOST_PYTHON_FUNCTION_OVERLOADS since it generates 'keywords' shadow warning.
			static
			element_type
			pop1(
					class_type &class_instance)
			{
				return pop(class_instance);
			}

			static
			long
			index(
					class_type &class_instance,
					// Using bp::object instead of 'element_type' since need to throw ValueError if wrong type...
					boost::python::object element_object,
					// Defaults to first element...
					long i = 0,
					// Defaults to one past last element...
					long j = 0)
			{
				return revisioned_vector_wrapper_type::index(
						GetRevisionedVectorFunction(class_instance), element_object, i, j);
			}

			// Default argument overloads of 'index()'.
			// Avoiding BOOST_PYTHON_FUNCTION_OVERLOADS since it generates 'keywords' shadow warning.
			static
			long
			index2(
					class_type &class_instance,
					// Using bp::object instead of 'element_type' since need to throw ValueError if wrong type...
					boost::python::object element_object)
			{
				return index(class_instance, element_object);
			}
			static
			long
			index3(
					class_type &class_instance,
					// Using bp::object instead of 'element_type' since need to throw ValueError if wrong type...
					boost::python::object element_object,
					long i)
			{
				return index(class_instance, element_object, i);
			}

			static
			int
			count(
					class_type &class_instance,
					const element_type &element)
			{
				return revisioned_vector_wrapper_type::count(
						GetRevisionedVectorFunction(class_instance), element);
			}

			static
			void
			reverse(
					class_type &class_instance)
			{
				revisioned_vector_wrapper_type::reverse(
						GetRevisionedVectorFunction(class_instance));
			}

			static
			void
			sort(
					class_type &class_instance,
					boost::python::object key_callable,
					bool reverse_)
			{
				revisioned_vector_wrapper_type::sort(
						GetRevisionedVectorFunction(class_instance), key_callable, reverse_);
			}
		};

		ENABLE_GCC_WARNING("-Wshadow")
	}


	template <
			class ClassType,
			class RevisionableType,
			class PythonClassHeldType,
			PythonClassHeldType
					(*CreateClassFromVectorFunction)(
							typename GPlatesModel::RevisionedVector<RevisionableType>::non_null_ptr_type,
							const ClassType &),
			PythonClassHeldType
					(*ConvertClassToPointerFunction)(
							ClassType &),
			typename GPlatesModel::RevisionedVector<RevisionableType>::non_null_ptr_type
					(*GetRevisionedVectorFunction)(
							ClassType &),
			class PythonClassType>
	void
	wrap_python_class_as_revisioned_vector(
			PythonClassType &python_class,
			const char *class_name)
	{
		Implementation::RevisionedVectorDelegateWrapper<
				ClassType,
				RevisionableType,
				PythonClassHeldType,
				CreateClassFromVectorFunction,
				ConvertClassToPointerFunction,
				GetRevisionedVectorFunction>
						::wrap(python_class, class_name);
	}
}

#endif // GPLATES_NO_PYTHON

#endif // GPLATES_API_PYREVISIONEDVECTOR_H
