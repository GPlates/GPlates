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

#ifndef GPLATES_API_PYREVISIONEDVECTOR_H
#define GPLATES_API_PYREVISIONEDVECTOR_H

#include <iterator>
#include <boost/foreach.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "PythonConverterUtils.h"

#include "global/CompilerWarnings.h"

#include "global/python.h"
// This is not included by <boost/python.hpp>.
// Also we must include this after <boost/python.hpp> which means after "global/python.h".
#include <boost/python/slice.hpp>
#include <boost/python/stl_iterator.hpp>

#include "model/RevisionedVector.h"


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
					GPlatesModel::RevisionedVector<RevisionableType>::non_null_ptr_type,
					boost::noncopyable>(class_name, bp::no_init)
				.def("__len__", &GPlatesModel::RevisionedVector<RevisionableType>::size)
 				.def("__setitem__", &set_item)
 				.def("__delitem__", &delete_item)
 				.def("__getitem__", &get_item)
				.def("__contains__", &contains)
			;
		}

	private:

		static
		void
		set_item(
				typename GPlatesModel::RevisionedVector<RevisionableType>::non_null_ptr_type revisioned_vector,
				boost::python::object i,
				boost::python::object v);

		static
		void
		delete_item(
				typename GPlatesModel::RevisionedVector<RevisionableType>::non_null_ptr_type revisioned_vector,
				boost::python::object i);

		static
		boost::python::object
		get_item(
				typename GPlatesModel::RevisionedVector<RevisionableType>::non_null_ptr_type revisioned_vector,
				boost::python::object i);

		static
		bool
		contains(
				typename GPlatesModel::RevisionedVector<RevisionableType>::non_null_ptr_type revisioned_vector,
				boost::python::object element_object);
	};
}

////////////////////
// Implementation //
////////////////////

namespace GPlatesApi
{
// For PySlice_Check below.
DISABLE_GCC_WARNING("-Wold-style-cast")

	template <class RevisionableType>
	void
	RevisionedVectorWrapper<RevisionableType>::set_item(
			typename GPlatesModel::RevisionedVector<RevisionableType>::non_null_ptr_type revisioned_vector,
			boost::python::object i,
			boost::python::object v)
	{
		namespace bp = boost::python;
		using namespace GPlatesModel;

		if (PySlice_Check(i.ptr()))
		{
			PySliceObject *slice_object_ptr = static_cast<PySliceObject *>(static_cast<void *>(i.ptr()));

			bp::object slice_start(bp::handle<>(bp::borrowed(slice_object_ptr->start)));
			bp::object slice_stop(bp::handle<>(bp::borrowed(slice_object_ptr->stop)));
			bp::object slice_step(bp::handle<>(bp::borrowed(slice_object_ptr->step)));
			bp::slice slice(slice_start, slice_stop, slice_step);
			if (slice == bp::object())
			{
				// Invalid slice so return.
				return;
			}

			// Use boost::python::slice to manage index variations such as negative indices or
			// indices that are None.
			bp::slice::range<typename RevisionedVector<RevisionableType>::iterator> bounds;
			try
			{
				bounds = slice.get_indicies(revisioned_vector->begin(), revisioned_vector->end());
			}
			catch (std::invalid_argument)
			{
				// Empty range so return.
				return;
			}

			// Begin/end iterators over the python iterable.
			bp::stl_input_iterator<RevisionedVector<RevisionableType>::element_type>
					new_elements_begin(v),
					new_elements_end;

			typename RevisionedVector<RevisionableType>::iterator erase_begin = bounds.start;
			typename RevisionedVector<RevisionableType>::iterator erase_end = bounds.stop;
			// Convert from closed range to half-open range.
			std::advance(erase_end, 1);

			// Erase the existing elements and insert the new elements.
			revisioned_vector->insert(
					revisioned_vector->erase(erase_begin, erase_end),
					new_elements_begin,
					new_elements_end);

			return;
		}

		bp::extract<long> extract_index(i);
		if (extract_index.check())
		{
			long index = extract_index();
			if (index < 0)
			{
				index += revisioned_vector->size();
			}
			if (index >= long(revisioned_vector->size()) || index < 0)
			{
				PyErr_SetString(PyExc_IndexError, "Index out of range");
				bp::throw_error_already_set();
			}

			// Attempt to extract the element.
			bp::extract<RevisionedVector<RevisionableType>::element_type> extract_element(v);
			if (!extract_element.check())
			{
				PyErr_SetString(PyExc_TypeError, "Invalid assignment");
				bp::throw_error_already_set();
				return;
			}
			RevisionedVector<RevisionableType>::element_type element = extract_element();

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
			typename GPlatesModel::RevisionedVector<RevisionableType>::non_null_ptr_type revisioned_vector,
			boost::python::object i)
	{
		namespace bp = boost::python;
		using namespace GPlatesModel;

		if (PySlice_Check(i.ptr()))
		{
			PySliceObject *slice_object_ptr = static_cast<PySliceObject *>(static_cast<void *>(i.ptr()));

			bp::object slice_start(bp::handle<>(bp::borrowed(slice_object_ptr->start)));
			bp::object slice_stop(bp::handle<>(bp::borrowed(slice_object_ptr->stop)));
			bp::object slice_step(bp::handle<>(bp::borrowed(slice_object_ptr->step)));
			bp::slice slice(slice_start, slice_stop, slice_step);
			if (slice == bp::object())
			{
				// Invalid slice so return.
				return;
			}

			// Use boost::python::slice to manage index variations such as negative indices or
			// indices that are None.
			bp::slice::range<typename RevisionedVector<RevisionableType>::iterator> bounds;
			try
			{
				bounds = slice.get_indicies(revisioned_vector->begin(), revisioned_vector->end());
			}
			catch (std::invalid_argument)
			{
				// Empty range so return.
				return;
			}

			typename RevisionedVector<RevisionableType>::iterator erase_begin = bounds.start;
			typename RevisionedVector<RevisionableType>::iterator erase_end = bounds.stop;
			// Convert from closed range to half-open range.
			std::advance(erase_end, 1);

			// Erase the range of elements.
			revisioned_vector->erase(erase_begin, erase_end);

			return;
		}

		bp::extract<long> extract_index(i);
		if (extract_index.check())
		{
			long index = extract_index();
			if (index < 0)
			{
				index += revisioned_vector->size();
			}
			if (index >= long(revisioned_vector->size()) || index < 0)
			{
				PyErr_SetString(PyExc_IndexError, "Index out of range");
				bp::throw_error_already_set();
			}

			// Erase the element.
			typename RevisionedVector<RevisionableType>::iterator erase_iter = revisioned_vector->begin();
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
			typename GPlatesModel::RevisionedVector<RevisionableType>::non_null_ptr_type revisioned_vector,
			boost::python::object i)
	{
		namespace bp = boost::python;
		using namespace GPlatesModel;

		if (PySlice_Check(i.ptr()))
		{
			PySliceObject *slice_object_ptr = static_cast<PySliceObject *>(static_cast<void *>(i.ptr()));

			bp::list slice_list;

			bp::object slice_start(bp::handle<>(bp::borrowed(slice_object_ptr->start)));
			bp::object slice_stop(bp::handle<>(bp::borrowed(slice_object_ptr->stop)));
			bp::object slice_step(bp::handle<>(bp::borrowed(slice_object_ptr->step)));
			bp::slice slice(slice_start, slice_stop, slice_step);
			if (slice == bp::object())
			{
				// Invalid slice so return empty list.
				return slice_list;
			}

			// Use boost::python::slice to manage index variations such as negative indices or
			// indices that are None.
			bp::slice::range<typename RevisionedVector<RevisionableType>::iterator> bounds;
			try
			{
				bounds = slice.get_indicies(revisioned_vector->begin(), revisioned_vector->end());
			}
			catch (std::invalid_argument)
			{
				// Empty range so return empty list.
				return slice_list;
			}

			typename RevisionedVector<RevisionableType>::iterator iter = bounds.start;
			for ( ; iter != bounds.stop; std::advance(iter, bounds.step))
			{
				slice_list.append(iter->get());
			}
			slice_list.append(iter->get());

			return slice_list;
		}

		bp::extract<long> extract_index(i);
		if (extract_index.check())
		{
			long index = extract_index();
			if (index < 0)
			{
				index += revisioned_vector->size();
			}
			if (index >= long(revisioned_vector->size()) || index < 0)
			{
				PyErr_SetString(PyExc_IndexError, "Index out of range");
				bp::throw_error_already_set();
			}

			RevisionedVector<RevisionableType>::element_type element = (*revisioned_vector)[index].get();
			return bp::object(element);
		}

		PyErr_SetString(PyExc_TypeError, "Invalid index type");
		bp::throw_error_already_set();

		return bp::object();
	}

ENABLE_GCC_WARNING("-Wold-style-cast")


	template <class RevisionableType>
	bool
	RevisionedVectorWrapper<RevisionableType>::contains(
			typename GPlatesModel::RevisionedVector<RevisionableType>::non_null_ptr_type revisioned_vector,
			boost::python::object element_object)
	{
		namespace bp = boost::python;
		using namespace GPlatesModel;

		bp::extract<RevisionedVector<RevisionableType>::element_type> extract_element(element_object);
		if (!extract_element.check())
		{
			return false;
		}
		RevisionedVector<RevisionableType>::element_type element = extract_element();

		// Search the revisioned vector for the element.
		BOOST_FOREACH(
				RevisionedVector<RevisionableType>::element_type revisioned_element,
				*revisioned_vector)
		{
			// Compare the pointed-to elements, not the pointers.
			if (*revisioned_element == *element)
			{
				return true;
			}
		}

		return false;
	}
}

#endif // GPLATES_NO_PYTHON

#endif // GPLATES_API_PYREVISIONEDVECTOR_H
