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

#include <iterator>
#include <sstream>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/iterator/iterator_traits.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "PythonConverterUtils.h"

#include "global/python.h"
// This is not included by <boost/python.hpp>.
// Also we must include this after <boost/python.hpp> which means after "global/python.h".
#include <boost/python/slice.hpp>
#include <boost/python/stl_iterator.hpp>

#include "model/RevisionedVector.h"

#include "property-values/GpmlTimeSample.h"


#if !defined(GPLATES_NO_PYTHON)

namespace GPlatesApi
{
	/**
	 * Implements wrapped functions for @a RevisionedVector and provides the wrapped class definition.
	 *
	 * The python interface matches that of python's list class such that elements can be added and
	 * removed using slices or indices.
	 */
	template <class RevisionableType>
	class RevisionedVectorWrapper
	{
	public:

		/**
		 * Wrap a template instantiation of @a RevisionedVector to expose to Python.
		 */
		static
		void
		wrap(
				const char *class_name)
		{
			namespace bp = boost::python;

			//
			// RevisionedVector - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
			//
			bp::class_<
					GPlatesModel::RevisionedVector<RevisionableType>,
					typename GPlatesModel::RevisionedVector<RevisionableType>::non_null_ptr_type,
					boost::noncopyable>(class_name, bp::no_init)
				.def("__len__", &GPlatesModel::RevisionedVector<RevisionableType>::size)
 				.def("__setitem__", &set_item)
 				.def("__delitem__", &delete_item)
 				.def("__getitem__", &get_item)
				.def("__contains__", &contains)
			;
		}

	private:

		//! Typedef for revisioned vector non_null_ptr_type.
		typedef typename GPlatesModel::RevisionedVector<RevisionableType>::non_null_ptr_type
				revisioned_vector_non_null_ptr_type;

		//! Typedef for revisioned vector element type.
		typedef typename GPlatesModel::RevisionedVector<RevisionableType>::element_type element_type;

		//! Typedef for revisioned vector iterator type.
		typedef typename GPlatesModel::RevisionedVector<RevisionableType>::iterator iterator_type;

		//! Typedef for revisioned vector iterator difference type.
		typedef typename boost::iterator_difference<iterator_type>::type iterator_difference_type;


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
		 * Throws IndexError exception if index is out-of-range.
		 */
		static
		long
		get_index(
				revisioned_vector_non_null_ptr_type revisioned_vector,
				long index);

	};

	////////////////////
	// Implementation //
	////////////////////

	template <class RevisionableType>
	void
	RevisionedVectorWrapper<RevisionableType>::set_item(
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
			boost::optional<bp::slice::range<iterator_type> >
					slice_range = get_slice_range(revisioned_vector, extract_slice());
			if (!slice_range)
			{
				// Invalid slice.
				return;
			}

			// Begin/end iterators over the python iterable.
			bp::stl_input_iterator<element_type>
					new_elements_begin(v),
					new_elements_end;

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
						new_elements_begin,
						new_elements_end);

				return;
			}

			// Copy into a vector so we can count the number of new elements.
			std::vector<element_type> new_elements_vector;
			std::copy(new_elements_begin, new_elements_end, std::back_inserter(new_elements_vector));

			// Count the number of elements in the extended slice.
			std::size_t extended_slice_size = 1; 
			iterator_type iter = slice_range->start;
			for ( ; iter != slice_range->stop; std::advance(iter, slice_range->step))
			{
				++extended_slice_size;
			}

			if (new_elements_vector.size() != extended_slice_size)
			{
				std::stringstream error_string_stream;
				error_string_stream << "attempt to assign sequence of size "
						<< new_elements_vector.size()
						<< " to extended slice of size " << extended_slice_size;

				PyErr_SetString(PyExc_ValueError, error_string_stream.str().c_str());
				bp::throw_error_already_set();
				return;
			}

			// Assign the new elements to the revisioned vector.
			std::vector<element_type>::const_iterator new_elements_iter = new_elements_vector.begin();
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


	template <class RevisionableType>
	void
	RevisionedVectorWrapper<RevisionableType>::delete_item(
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
				// Invalid slice.
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


	template <class RevisionableType>
	boost::python::object
	RevisionedVectorWrapper<RevisionableType>::get_item(
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
				// Invalid slice - return empty list.
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


	template <class RevisionableType>
	bool
	RevisionedVectorWrapper<RevisionableType>::contains(
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


	template <class RevisionableType>
	boost::optional<
			boost::python::slice::range<
					typename RevisionedVectorWrapper<RevisionableType>::iterator_type> >
	RevisionedVectorWrapper<RevisionableType>::get_slice_range(
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
		}

		// Empty range.
		return boost::none;
	}


	template <class RevisionableType>
	long
	RevisionedVectorWrapper<RevisionableType>::get_index(
			revisioned_vector_non_null_ptr_type revisioned_vector,
			long index)
	{
		namespace bp = boost::python;

		if (index < 0)
		{
			index += revisioned_vector->size();
		}

		if (index >= long(revisioned_vector->size()) ||
			index < 0)
		{
			PyErr_SetString(PyExc_IndexError, "Index out of range");
			bp::throw_error_already_set();
		}

		return index;
	}
}


void
export_revisioned_vector()
{
	// Export all required instantiations of class template RevisionedVector...

	// GpmlTimeSample.
	GPlatesApi::RevisionedVectorWrapper<GPlatesPropertyValues::GpmlTimeSample>::wrap(
			"RevisionedVector<GpmlTimeSample>");
}

#endif // GPLATES_NO_PYTHON
