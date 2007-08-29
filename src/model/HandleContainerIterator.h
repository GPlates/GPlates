/* $Id$ */

/**
 * \file 
 * Contains the definition of the class HandleContainerIterator.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_HANDLECONTAINERITERATOR_H
#define GPLATES_MODEL_HANDLECONTAINERITERATOR_H

#include <iterator>  /* iterator, bidirectional_iterator_tag */
#include "WeakObserver.h"


namespace GPlatesModel
{
	/**
	 * A revision-aware iterator to iterate over the container of whatever-handles contained
	 * within a revisioning collection.
	 *
	 * @par Revision awareness
	 * By "revision-aware" is meant that instances of this class will not be fooled, by a
	 * revisioning operation, to point to an old revision of the container.  Each and every
	 * iterator operation first gets the current revision of the container, before accessing
	 * the elements of the container (the contained handles).
	 *
	 * @par The WeakObserver base
	 * The base class WeakObserver contains the pointer to the collection handle (which
	 * contains the handle container over which this iterator is iterating).  The benefit of
	 * using WeakObserver to contain the pointer-to-collection-handle is that an instance of
	 * HandleContainerIterator, which is pointing to a particular collection handle, will be
	 * informed if that collection handle is deactivated (ie, logically deleted) or deallocated
	 * (ie, deleted in the C++ memory-allocation sense).
	 *
	 * @par
	 * The member function @a is_valid is used to determine whether an iterator instance is
	 * valid to be dereferenced.
	 *
	 * @par The template parameters
	 * The template parameters are:
	 *  - @em H: the type of the collection handle (for example, '@c FeatureCollectionHandle '
	 * or '@c const @c FeatureCollectionHandle ')
	 *  - @em ConstH: the const-type of the collection handle (for example,
	 * '@c const @c FeatureCollectionHandle ')
	 *  - @em C: the type of the handle container (for example,
	 * '@c std::vector@<boost::intrusive_ptr@<FeatureHandle@>@> ')
	 *  - @em D: the type to which the iterator will dereference (for example,
	 * '@c boost::intrusive_ptr@<FeatureHandle@> ' or
	 * '@c boost::intrusive_ptr@<const @c FeatureHandle@> ')
	 */
	template<typename H, typename ConstH, typename C, typename D>
	class HandleContainerIterator:
			public WeakObserver<H, ConstH>,
			public std::iterator<std::bidirectional_iterator_tag, D>
	{
	public:
		/**
		 * This is the type of the collection handle.
		 *
		 * (For example, '@c FeatureCollectionHandle ' or
		 * '@c const @c FeatureCollectionHandle '.)
		 */
		typedef H collection_handle_type;

		/**
		 * This is the const-type of the collection handle.
		 *
		 * (For example, '@c const @c FeatureCollectionHandle '.)
		 */
		typedef ConstH const_collection_handle_type;

		/**
		 * This is the type of the handle container.
		 *
		 * (For example, '@c std::vector@<boost::intrusive_ptr@<FeatureHandle@>@> '.)
		 */
		typedef C handle_container_type;

		/**
		 * This is the type to which the iterator will dereference.
		 *
		 * (For example, '@c boost::intrusive_ptr@<FeatureHandle@> ' or
		 * '@c boost::intrusive_ptr@<const @c FeatureHandle@> '.)
		 */
		typedef D dereference_type;

		/**
		 * This is the type used to index the elements of the handle container.
		 */
		typedef typename handle_container_type::size_type index_type;

		static
		const HandleContainerIterator
		create_index(
				collection_handle_type &collection_handle,
				index_type index)
		{
			return HandleContainerIterator(collection_handle, index);
		}

		/**
		 * This factory function is used to instantiate "begin" iterators for a collection
		 * handle.
		 *
		 * This function will not throw.
		 */
		static
		const HandleContainerIterator
		create_begin(
				collection_handle_type &collection_handle)
		{
			return HandleContainerIterator(collection_handle, 0);
		}

		/**
		 * This factory function is used to instantiate "end" iterators for a collection
		 * handle.
		 *
		 * This function will not throw.
		 */
		static
		const HandleContainerIterator
		create_end(
				collection_handle_type &collection_handle)
		{
			return HandleContainerIterator(collection_handle,
					container_size(collection_handle));
		}

		/**
		 * Default constructor.
		 *
		 * Iterator instances which are initialised using the default constructor are not
		 * valid to be dereferenced.
		 */
		HandleContainerIterator():
			d_index(0)
		{  }

		/**
		 * Return the pointer to the collection handle.
		 *
		 * This function will not throw.
		 *
		 * Note that we return a non-const instance of collection_handle_type from a const
		 * member function:  collection_handle_type may already be "const", in which case,
		 * the "const" would be redundant; OTOH, if collection_handle_type *doesn't*
		 * include "const", an instance of this class should behave like an STL iterator
		 * (or a pointer) rather than an STL const-iterator.  We're simply declaring the
		 * member function "const" to ensure that it may be invoked on const instances too.
		 */
		collection_handle_type *
		collection_handle_ptr() const
		{
			return WeakObserver<H, ConstH>::publisher_ptr();
		}
		/**
		 * Return the current index.
		 *
		 * This function will not throw.
		 */
		index_type
		index() const
		{
			return d_index;
		}

		/**
		 * Return whether this iterator is valid to be dereferenced.
		 *
		 * This function will not throw.
		 */
		bool
		is_valid() const
		{
			return (collection_handle_is_valid() && index_is_within_bounds());
		}

		/**
		 * Copy-assignment operator.
		 *
		 * This function will not throw.
		 */
		HandleContainerIterator &
		operator=(
				const HandleContainerIterator &other)
		{
			WeakObserver<H, ConstH>::operator=(other);
			d_index = other.d_index;

			return *this;
		}

		/**
		 * Return whether this instance is equal to @a other.
		 *
		 * This function will not throw.
		 */
		bool
		operator==(
				const HandleContainerIterator &other) const
		{
			return (collection_handle_ptr() == other.collection_handle_ptr() &&
					index() == other.index());
		}

		/**
		 * Return whether this instance is not equal to @a other.
		 *
		 * This function will not throw.
		 */
		bool
		operator!=(
				const HandleContainerIterator &other) const
		{
			return (collection_handle_ptr() != other.collection_handle_ptr() ||
					index() != other.index());
		}

		/**
		 * The dereference operator.
		 *
		 * This operator should only be invoked when the iterator is valid (i.e., when
		 * @a is_valid would return true).
		 *
		 * Note that it is a deliberate limitation of this operator, that the return-value
		 * is not an L-value (ie, it cannot be assigned-to).  This is to ensure that the
		 * revisioning mechanism is not bypassed.
		 *
		 * As long as the iterator is valid, this function will not throw.
		 */
		const dereference_type
		operator*() const
		{
			return current_element();
		}

		/**
		 * The dereference operator.
		 *
		 * This operator should only be invoked when the iterator is valid (i.e., when
		 * @a is_valid would return true).
		 *
		 * Note that it is a deliberate limitation of this operator, that the return-value
		 * is not an L-value (ie, it cannot be assigned-to).  This is to ensure that the
		 * revisioning mechanism is not bypassed.
		 *
		 * As long as the iterator is valid, this function will not throw.
		 */
		const dereference_type
		operator*()
		{
			return current_element();
		}

		/**
		 * The pre-increment operator.
		 *
		 * This function will not throw.
		 */
		HandleContainerIterator &
		operator++()
		{
			++d_index;
			return *this;
		}

		/**
		 * The post-increment operator.
		 *
		 * This function will not throw.
		 */
		const HandleContainerIterator
		operator++(int)
		{
			HandleContainerIterator original(*this);
			++d_index;
			return original;
		}

		/**
		 * The pre-decrement operator.
		 *
		 * This function will not throw.
		 */
		HandleContainerIterator &
		operator--()
		{
			--d_index;
			return *this;
		}

		/**
		 * The post-decrement operator.
		 *
		 * This function will not throw.
		 */
		const HandleContainerIterator
		operator--(int)
		{
			HandleContainerIterator original(*this);
			--d_index;
			return original;
		}
	private:
		/**
		 * Return the size of the container in the supplied collection_handle_type.
		 *
		 * This function will not throw.
		 */
		static
		index_type
		container_size(
				const collection_handle_type &collection_handle_ptr_)
		{
			return collection_handle_ptr_.current_revision()->size();
		}

		/**
		 * Construct an iterator to iterate over the container inside @a collection_handle,
		 * beginning at @a index.
		 *
		 * This constructor will not throw.
		 */
		HandleContainerIterator(
				collection_handle_type &collection_handle,
				index_type index):
			WeakObserver<H, ConstH>(collection_handle),
			d_index(index)
		{  }

		/**
		 * Access the currently-indicated element.
		 *
		 * This function should only be invoked when the iterator is valid to be
		 * dereferenced (i.e., when @a is_valid would return true).
		 *
		 * As long as the index is valid, this function will not throw.
		 */
		const dereference_type
		current_element() const
		{
			return (*(collection_handle_ptr()->current_revision()))[index()];
		}

		/**
		 * Access the currently-indicated element.
		 *
		 * This function should only be invoked when the iterator is valid to be
		 * dereferenced (i.e., when @a is_valid would return true).
		 *
		 * As long as the index is valid, this function will not throw.
		 */
		const dereference_type
		current_element()
		{
			return (*(collection_handle_ptr()->current_revision()))[index()];
		}

		/**
		 * Return whether the collection handle is valid (i.e., whether it is indicated by
		 * a non-NULL pointer).
		 */
		bool
		collection_handle_is_valid() const
		{
			return (collection_handle_ptr() != NULL);
		}

		/**
		 * Return whether the index indicates an element which is within the bounds of the
		 * container.
		 *
		 * This function should only be invoked whin the collection handle is valid (i.e.,
		 * when @a collection_handle_is_valid would return true).
		 *
		 * This function will not throw.
		 */
		bool
		index_is_within_bounds() const
		{
			// The index indicates an element which is before the end of the container
			// when the index is less than the size of the container.
			return ((static_cast<long>(d_index) >= 0) &&
					(d_index < container_size(*collection_handle_ptr())));
		}

		/**
		 * This is the current index in the handle container.
		 */
		index_type d_index;
	};

}

#endif  // GPLATES_MODEL_HANDLECONTAINERITERATOR_H
