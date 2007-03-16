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
		 * This factory function is used to instantiate "begin" iterators.
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
		 */
		index_type
		index() const
		{
			return d_index;
		}

		/**
		 * Return whether this instance is equal to @a other.
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
		 * Note that it is a deliberate limitation of this operator, that the return-value
		 * is not an L-value (ie, it cannot be assigned-to).  This is to ensure that the
		 * revisioning mechanism is not bypassed.
		 */
		const dereference_type
		operator*() const
		{
			return current_element();
		}

		/**
		 * The pointer-indirection-member-access operator.
		 *
		 * Note that it is a deliberate limitation of this operator, that when the
		 * return-value is dereferenced, the result is not an L-value (ie, it cannot be
		 * assigned-to).  This is to ensure that the revisioning mechanism is not bypassed.
		 */
		const dereference_type *
		operator->() const
		{
			return &current_element();
		}

		/**
		 * The pre-increment operator.
		 */
		HandleContainerIterator &
		operator++()
		{
			advance();
			return *this;
		}

		/**
		 * The post-increment operator.
		 */
		const HandleContainerIterator
		operator++(int)
		{
			HandleContainerIterator orig(*this);
			advance();
			return orig;
		}
	private:
		/**
		 * Return the size of the container in the supplied collection_handle_type.
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
		 */
		HandleContainerIterator(
				collection_handle_type &collection_handle,
				index_type index):
			d_collection_handle_ptr(&collection_handle),
			d_index(index)
		{
			increment_until_non_null_elem();
		}

		/**
		 * Access the currently-indicated element.
		 *
		 * This function should only be invoked when the index indicates an element which
		 * is @em before the end of the container (i.e., when @a index_is_before_end would
		 * return true).
		 */
		dereference_type
		current_element() const
		{
			return (*(d_collection_handle_ptr->current_revision()))[d_index];
		}

		/**
		 * Return whether the index indicates an element which is before the end of the
		 * container.
		 */
		bool
		index_is_before_end() const
		{
			// The index indicates an element which is before the end of the container
			// when the index is less than the size of the container.
			return (d_index < container_size(*d_collection_handle_ptr));
		}

		/**
		 * Return whether the index indicates an element of the container which is NULL.
		 */
		bool
		index_indicates_null_elem() const
		{
			return ( ! current_element());
		}

		/**
		 * Continue incrementing the index until an element is reached which is not NULL,
		 * or there's no more container through which to iterate.
		 */
		void
		increment_until_non_null_elem()
		{
			// Note that the loop condition is that the index is *before* the end,
			// rather than *not equal to* the end, to ensure that the loop will neither
			// run forever nor access memory outside the container, in the event that
			// the client code increments an end-iterator.
			while (index_is_before_end() && index_indicates_null_elem()) {
				++d_index;
			}
		}

		/**
		 * Advance the iterator in response to an increment request.
		 */
		void
		advance()
		{
			++d_index;
			increment_until_non_null_elem();
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
