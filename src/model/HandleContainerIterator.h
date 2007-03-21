/* $Id$ */

/**
 * \file 
 * Contains the definition of the class HandleContainerIterator.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006 The University of Sydney, Australia
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


namespace GPlatesModel
{
	/**
	 * A revision-aware iterator to iterate over the container of whatever-handles contained
	 * within a revisioning collection.
	 */
	template<typename H, typename C, typename D>
	class HandleContainerIterator
	{
	public:
		/**
		 * This is the type of the collection handle.
		 *
		 * (For example, 'FeatureCollectionHandle'.)
		 */
		typedef H collection_handle_type;

		/**
		 * This is the type of the handle container.
		 *
		 * (For example, 'std::vector<boost::intrusive_ptr<FeatureHandle> >'.)
		 */
		typedef C handle_container_type;

		/**
		 * This is the type to which the iterator will dereference.
		 *
		 * (For example, 'boost::intrusive_ptr<FeatureHandle>' or
		 * 'boost::intrusive_ptr<const FeatureHandle>'.)
		 */
		typedef D dereference_type;

		/**
		 * This is the type used to index the elements of the handle container.
		 */
		typedef typename handle_container_type::size_type index_type;

		/**
		 * Make the collection-handle-type a friend.
		 *
		 * This is a hack to enable the collection-handle-type to invoke the constructor of
		 * this class with a specific index value, without abandoning @em all privacy.
		 */
		friend class collection_handle_type::this_type;

		/**
		 * This factory function is used to instantiate "begin" iterators.
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
		 * This factory function is used to instantiate "end" iterators.
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
			return index_is_within_bounds();
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
			return (d_collection_handle_ptr == other.d_collection_handle_ptr &&
					d_index == other.d_index);
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
			return (d_collection_handle_ptr != other.d_collection_handle_ptr ||
					d_index != other.d_index);
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
		 * The pointer-indirection-member-access operator.
		 *
		 * This operator should only be invoked when the iterator is valid (i.e., when
		 * @a is_valid would return true).
		 *
		 * Note that it is a deliberate limitation of this operator, that when the
		 * return-value is dereferenced, the result is not an L-value (ie, it cannot be
		 * assigned-to).  This is to ensure that the revisioning mechanism is not bypassed.
		 *
		 * As long as the iterator is valid, this function will not throw.
		 */
		const dereference_type *
		operator->() const
		{
			return &current_element();
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
				const collection_handle_type &collection_handle_ptr)
		{
			return collection_handle_ptr.current_revision()->size();
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
			d_collection_handle_ptr(&collection_handle),
			d_index(index)
		{  }

		/**
		 * Access the currently-indicated element.
		 *
		 * This function should only be invoked when the index indicates an element which
		 * is within the bounds of the container (i.e., when @a index_is_within_bounds
		 * would return true).
		 *
		 * As long as the index is valid, this function will not throw.
		 */
		dereference_type
		current_element() const
		{
			return (*(d_collection_handle_ptr->current_revision()))[d_index];
		}

		/**
		 * Return whether the index indicates an element which is within the bounds of the
		 * container.
		 *
		 * This function will not throw.
		 */
		bool
		index_is_within_bounds() const
		{
			// The index indicates an element which is before the end of the container
			// when the index is less than the size of the container.
			return ((static_cast<long>(d_index) >= 0) &&
					(d_index < container_size(*d_collection_handle_ptr)));
		}

		/**
		 * This is the collection handle which contains the handle container over which
		 * this iterator is iterating.
		 */
		collection_handle_type *d_collection_handle_ptr;

		/**
		 * This is the current index in the handle container.
		 */
		index_type d_index;
	};

}

#endif  // GPLATES_MODEL_HANDLECONTAINERITERATOR_H
