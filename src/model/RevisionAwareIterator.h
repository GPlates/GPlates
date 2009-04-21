/* $Id$ */

/**
 * \file 
 * Contains the definition of the class RevisionAwareIterator.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2009 The University of Sydney, Australia
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
#include "WeakObserver.h"
//#include "WeakObserverVisitor.h"


namespace GPlatesModel
{
	/**
	 * A revision-aware iterator to iterate over the container within a revisioning collection.
	 *
	 * This class was originally designed to iterate over the container of feature-handles in a
	 * feature-collection-handle.  It was later generalised to also iterate over the container
	 * of feature-collection-handles in the feature-store-root, and later still to iterate over
	 * the container of property-containers in a feature-handle.
	 *
	 * @par Revision awareness
	 * By "revision-aware" is meant that instances of this class will not be fooled, by a
	 * revisioning operation, to point to an old revision of the container.  Each and every
	 * iterator operation first gets the current revision of the container, before accessing
	 * the elements of the container.
	 *
	 * @par The WeakObserver base
	 * The base class WeakObserver contains the pointer to the handle (which contains the
	 * container over which this iterator is iterating).  The benefit of using WeakObserver to
	 * contain the pointer-to-handle is that an instance of RevisionAwareIterator, which is
	 * pointing to a particular handle, will be informed if that handle is deactivated (ie,
	 * logically deleted) or deallocated (ie, deleted in the C++ memory-allocation sense).
	 *
	 * @par
	 * The member function @a is_valid is used to determine whether an iterator instance is
	 * valid to be dereferenced.
	 *
	 * @par The template parameters
	 * The template parameters are:
	 *  - @em H: the type of the handle of the revisioning collection (for example,
	 * '@c FeatureCollectionHandle ')
	 *  - @em C: the type of the container over which this iterator will iterate (for example,
	 * '@c std::vector@<boost::intrusive_ptr@<FeatureHandle@>@> ')
	 *  - @em D: the type of the container elements, the type to which the iterator will
	 * dereference (for example,
	 * '@c boost::intrusive_ptr@<FeatureHandle@> ')
	 */
	template<typename H, typename C, typename D>
	class RevisionAwareIterator:
			public WeakObserver<H>,
			public std::iterator<std::bidirectional_iterator_tag, D>
	{
	public:
		/**
		 * This is the type of the handle of the revisioning collection.
		 *
		 * (For example, '@c FeatureCollectionHandle ')
		 */
		typedef H collection_handle_type;

		/**
		 * This is the type of the container over which this iterator will iterate.
		 *
		 * (For example, '@c std::vector@<boost::intrusive_ptr@<FeatureHandle@>@> '.)
		 */
		typedef C container_type;

		/**
		 * This is the type of the container elements, the type to which the iterator will
		 * dereference.
		 *
		 * (For example, '@c boost::intrusive_ptr@<FeatureHandle@> ' or
		 * '@c boost::intrusive_ptr@<const @c FeatureHandle@> '.)
		 */
		typedef D dereference_type;

		/**
		 * This is the type used to index the elements of the container.
		 */
		typedef typename container_type::size_type index_type;

		static
		const RevisionAwareIterator
		create_index(
				collection_handle_type &collection_handle,
				index_type index_)
		{
			return RevisionAwareIterator(collection_handle, index_);
		}

		/**
		 * This factory function is used to instantiate "begin" iterators for a collection
		 * handle.
		 *
		 * This function will not throw.
		 */
		static
		const RevisionAwareIterator
		create_begin(
				collection_handle_type &collection_handle)
		{
			return RevisionAwareIterator(collection_handle, 0);
		}

		/**
		 * This factory function is used to instantiate "end" iterators for a collection
		 * handle.
		 *
		 * This function will not throw.
		 */
		static
		const RevisionAwareIterator
		create_end(
				collection_handle_type &collection_handle)
		{
			return RevisionAwareIterator(collection_handle,
					container_size(collection_handle));
		}

		/**
		 * Default constructor.
		 *
		 * Iterator instances which are initialised using the default constructor are not
		 * valid to be dereferenced.
		 */
		RevisionAwareIterator():
			d_index(0)
		{  }

		virtual
		~RevisionAwareIterator()
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
			return WeakObserver<H>::publisher_ptr();
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
		RevisionAwareIterator &
		operator=(
				const RevisionAwareIterator &other)
		{
			WeakObserver<H>::operator=(other);
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
				const RevisionAwareIterator &other) const
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
				const RevisionAwareIterator &other) const
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
		RevisionAwareIterator &
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
		const RevisionAwareIterator
		operator++(int)
		{
			RevisionAwareIterator original(*this);
			++d_index;
			return original;
		}

		/**
		 * The pre-decrement operator.
		 *
		 * This function will not throw.
		 */
		RevisionAwareIterator &
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
		const RevisionAwareIterator
		operator--(int)
		{
			RevisionAwareIterator original(*this);
			--d_index;
			return original;
		}

		/**
		 * Accept a WeakObserverVisitor instance.
		 */
		virtual
		void
		accept_weak_observer_visitor(
				WeakObserverVisitor<H> &visitor)
		{
#if 0
			// FIXME:  What I'm doing right now is very very bad.
			// Class RevisionAwareIterator is not telling the WeakObserverVisitor to
			// visit it.  I'm doing this because class RevisionAwareIterator has extra
			// template parameters, and I can't work out an easy way to handle these in
			// the function 'visit_revision_aware_iterator' in WeakObserverVisitor.
			visitor.visit_revision_aware_iterator(*this);
#endif
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
		RevisionAwareIterator(
				collection_handle_type &collection_handle,
				index_type index_):
			WeakObserver<H>(collection_handle),
			d_index(index_)
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
		 * This is the current index in the container.
		 */
		index_type d_index;
	};

}

#endif  // GPLATES_MODEL_REVISIONAWAREITERATOR_H
