/* $Id$ */

/**
 * \file 
 * Contains the definition of the class RevisionAwareIterator.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_REVISIONAWAREITERATOR_H
#define GPLATES_MODEL_REVISIONAWAREITERATOR_H

#include <iterator>  /* iterator, bidirectional_iterator_tag */
#include <boost/operators.hpp>

#include "FeatureCollectionHandle.h"
#include "FeatureHandle.h"
#include "FeatureStoreRootHandle.h"
#include "HandleTraits.h"
#include "TopLevelPropertyRef.h"
#include "WeakReference.h"
#include "types.h"

namespace GPlatesModel
{
	namespace RevisionAwareIteratorInternals
	{
		/**
		 * A helper traits class to differentiate between const and non-const Handles.
		 */
		template<class HandleType>
		struct Traits
		{
			typedef typename HandleTraits<HandleType>::iterator_value_type value_type;
			typedef typename HandleTraits<HandleType>::weak_ref handle_weak_ref_type;
		};

		template<class HandleType>
		struct Traits<const HandleType>
		{
			typedef typename HandleTraits<HandleType>::const_iterator_value_type value_type;
			typedef typename HandleTraits<HandleType>::const_weak_ref handle_weak_ref_type;
		};
	}

	// Forward declarations.
	template<class HandleType>
	class RevisionAwareIterator;

	template<class HandleType>
	bool
	operator<(
			const RevisionAwareIterator<HandleType> &lhs,
			const RevisionAwareIterator<HandleType> &rhs);

	/**
	 * A revision-aware iterator to iterate over the container within a revisioning collection.
	 *
	 * This class was originally designed to iterate over the container of feature-handles in a
	 * feature-collection-handle.  It was later generalised to iterate over the container of
	 * feature-collection-handles in the feature-store-root also, and later still, to iterate
	 * over the container of property-containers in a feature-handle.
	 *
	 * @par Revision awareness
	 * By "revision-aware" is meant that instances of this class will not be fooled, by a
	 * revisioning operation, to point to an old revision of the container.  Every dereference
	 * operation first obtains the current revision of the container, before accessing the
	 * elements within that container.
	 *
	 * @par Important
	 * @strong Always check that the iterator @a is_valid before every dereference operation!
	 */
	template<class HandleType>
	class RevisionAwareIterator :
			public std::iterator<
				std::bidirectional_iterator_tag,
				typename RevisionAwareIteratorInternals::Traits<HandleType>::value_type,
				ptrdiff_t,
				// The 'pointer' inner type is set to void, because the dereference
				// operator returns a temporary, and it is not desirable to take a pointer
				// to a temporary.
				void,
				// The 'reference' inner type is not a reference, because the dereference
				// operator returns a temporary, and it is not desirable to take a reference
				// to a temporary.
				typename RevisionAwareIteratorInternals::Traits<HandleType>::value_type
			>,
			public boost::equivalent<RevisionAwareIterator<HandleType> >,
			public boost::equality_comparable<RevisionAwareIterator<HandleType> >
	{

	public:

		/**
		 * The type of Handle we are iterating over, e.g. FeatureHandle, const FeatureCollectionHandle.
		 */
		typedef HandleType handle_type;

		/**
		 * The type of this class.
		 */
		typedef RevisionAwareIterator<handle_type> this_type;

		/**
		 * The type of the Revision corresponding to the Handle.
		 */
		typedef typename HandleTraits<handle_type>::revision_type revision_type;

		/**
		 * The type returned by this iterator on dereference, with appropriate const-ness.
		 */
		typedef typename RevisionAwareIteratorInternals::Traits<handle_type>::value_type value_type;

		/**
		 * The type of a weak-ref to the Handle we're iterating over, with appropriate const-ness.
		 */
		typedef typename RevisionAwareIteratorInternals::Traits<handle_type>::handle_weak_ref_type handle_weak_ref_type;

		/**
		 * The type used to index the elements of the container.
		 */
		typedef container_size_type index_type;

		/**
		 * Default constructor.
		 *
		 * Iterator instances which are initialised using the default constructor are not
		 * valid to be dereferenced.
		 */
		RevisionAwareIterator();

		/**
		 * Construct an iterator to iterate over the container inside @a handle,
		 * beginning at @a index.
		 *
		 * Set @a index to be the size of the underlying container to create an "end" iterator.
		 *
		 * This constructor will not throw.
		 */
		explicit
		RevisionAwareIterator(
				handle_type &handle,
				index_type index_ = 0);

		/**
		 * Converts this RevisionAwareIterator<HandleType> into a RevisionAwareIterator<const HandleType>.
		 *
		 * If HandleType is already const, this effectively does nothing useful and returns RevisionAwareIterator<HandleType>.
		 */
		operator RevisionAwareIterator<const HandleType>() const;

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
		handle_weak_ref_type
		handle_weak_ref() const;

		/**
		 * Return the current index.
		 *
		 * This function will not throw.
		 */
		index_type
		index() const;

		/**
		 * The dereference operator.
		 *
		 * This operator should only be invoked when the iterator is valid.
		 *
		 * As long as the iterator is valid, this function will not throw.
		 */
		value_type
		operator*() const;

#if 0
		/**
		 * The dereference operator.
		 *
		 * This operator should only be invoked when the iterator is valid.
		 *
		 * As long as the iterator is valid, this function will not throw.
		 */
		value_type
		operator*();
#endif

		/**
		 * The pre-increment operator.
		 *
		 * This function will not throw.
		 */
		RevisionAwareIterator &
		operator++();

		/**
		 * The post-increment operator.
		 *
		 * This function will not throw.
		 */
		const RevisionAwareIterator
		operator++(int);

		/**
		 * The pre-decrement operator.
		 *
		 * This function will not throw.
		 */
		RevisionAwareIterator &
		operator--();

		/**
		 * The post-decrement operator.
		 *
		 * This function will not throw.
		 */
		const RevisionAwareIterator
		operator--(int);

		/**
		 * Returns whether the underlying weak-ref to the Handle is valid, and if so
		 * whether the child of the Handle being pointed to is still in existence.
		 *
		 * Note: You should not call this function if you are simply iterating over a
		 * Handle's container of children, and you just obtained your iterator -
		 * RevisionAwareIterator now skips over NULLs in the container. You should
		 * only call this function if you have held onto your iterator for some time
		 * and there is the possibility that in the intervening period since you got
		 * your iterator, the whole Handle to which this is an interator has gone
		 * away or perhaps just the child to which we are pointing has gone away.
		 *
		 * This function will not throw.
		 */
		bool
		is_still_valid() const;

		/**
		 * Returns whether @a lhs is less than @a rhs.
		 *
		 * This function will now throw.
		 */
		friend
		bool
		operator< <>(
				const RevisionAwareIterator<HandleType> &lhs,
				const RevisionAwareIterator<HandleType> &rhs);

	private:

		/**
		 * Access the currently-indicated element.
		 *
		 * This function should only be invoked when the iterator is valid to be
		 * dereferenced (i.e., when @a is_valid would return true).
		 *
		 * As long as the handle and index are valid, this function will not throw.
		 */
		value_type
		current_element() const;

		/**
		 * A weak-ref to the Handle whose contents this Iterator iterates over.
		 */
		handle_weak_ref_type d_handle_weak_ref;

		/**
		 * This is the current index in the container.
		 */
		index_type d_index;

	};


	template<class HandleType>
	RevisionAwareIterator<HandleType>::RevisionAwareIterator() :
		d_index(INVALID_INDEX)
	{
	}


	template<class HandleType>
	RevisionAwareIterator<HandleType>::RevisionAwareIterator(
			handle_type &handle,
			index_type index_) :
		d_handle_weak_ref(handle.reference()),
		d_index(index_)
	{
		// Sanity check.
		const revision_type &revision = *(d_handle_weak_ref->current_revision());
		if (d_index > revision.container_size())
		{
			d_index = revision.container_size();
		}

		// Move the iterator along until the first element that is not NULL.
		if (d_index < revision.container_size() && !revision.has_element_at(d_index))
		{
			operator++();
		}
	}


	template<class HandleType>
	RevisionAwareIterator<HandleType>::operator RevisionAwareIterator<const HandleType>() const
	{
		const HandleType *handle_ptr = d_handle_weak_ref.handle_ptr();

		if (handle_ptr)
		{
			return RevisionAwareIterator<const HandleType>(*handle_ptr, d_index);
		}
		else
		{
			return RevisionAwareIterator<const HandleType>();
		}
	}


	template<class HandleType>
	typename RevisionAwareIterator<HandleType>::handle_weak_ref_type
	RevisionAwareIterator<HandleType>::handle_weak_ref() const
	{
		return d_handle_weak_ref;
	}


	template<class HandleType>
	typename RevisionAwareIterator<HandleType>::index_type
	RevisionAwareIterator<HandleType>::index() const
	{
		return d_index;
	}


	template<class HandleType>
	typename RevisionAwareIterator<HandleType>::value_type
	RevisionAwareIterator<HandleType>::operator*() const
	{
		return current_element();
	}

#if 0
	template<class HandleType>
	typename RevisionAwareIterator<HandleType>::value_type
	RevisionAwareIterator<HandleType>::operator*()
	{
		return current_element();
	}
#endif

	template<class HandleType>
	RevisionAwareIterator<HandleType> &
	RevisionAwareIterator<HandleType>::operator++()
	{
		const revision_type &revision = *(d_handle_weak_ref->current_revision());
		do
		{
			++d_index;
		}
		while (d_index < revision.container_size() && !revision.has_element_at(d_index));

		return *this;
	}


	template<class HandleType>
	const RevisionAwareIterator<HandleType>
	RevisionAwareIterator<HandleType>::operator++(int)
	{
		this_type original(*this);
		++(*this);
		return original;
	}


	template<class HandleType>
	RevisionAwareIterator<HandleType> &
	RevisionAwareIterator<HandleType>::operator--()
	{
		const revision_type &revision = *(d_handle_weak_ref->current_revision());
		while (!revision.has_element_at(--d_index))
		{
			if (d_index == 0)
			{
				// The child at index 0 has been deleted, so let's move forward until we
				// find the first undeleted child (or end, if no children).
				operator++();
				break;
			}
		}

		return *this;
	}


	template<class HandleType>
	const RevisionAwareIterator<HandleType>
	RevisionAwareIterator<HandleType>::operator--(int)
	{
		this_type original(*this);
		--(*this);
		return original;
	}


	template<class HandleType>
	bool
	RevisionAwareIterator<HandleType>::is_still_valid() const
	{
		return d_handle_weak_ref.is_valid() &&
			d_handle_weak_ref->current_revision()->get(d_index);
	}


	template<class HandleType>
	bool
	operator<(
			const RevisionAwareIterator<HandleType> &lhs,
			const RevisionAwareIterator<HandleType> &rhs)
	{
		if (lhs.d_handle_weak_ref == rhs.d_handle_weak_ref)
		{
			return lhs.d_index < rhs.d_index;
		}
		else
		{
			return lhs.d_handle_weak_ref < rhs.d_handle_weak_ref;
		}
	}


	// Private methods ////////////////////////////////////////////////////////////

	template<class HandleType>
	typename RevisionAwareIterator<HandleType>::value_type
	RevisionAwareIterator<HandleType>::current_element() const
	{
		return d_handle_weak_ref->get(d_index);
	}


	// Template specialisations are in .cc file.
	template<>
	RevisionAwareIterator<FeatureHandle>::value_type
	RevisionAwareIterator<FeatureHandle>::current_element() const;

}


#endif  // GPLATES_MODEL_REVISIONAWAREITERATOR_H
