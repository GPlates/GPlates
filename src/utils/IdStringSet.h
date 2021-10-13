/* $Id$ */

/**
 * \file
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 *
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATES_UTILS_IDSTRINGSET_H
#define GPLATES_UTILS_IDSTRINGSET_H

#ifndef GPLATES_ICU_BOOL
#define GPLATES_ICU_BOOL(b) ((b) != 0)
#endif

#include <algorithm>
#include <set>
#include <boost/intrusive_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>

#include "SmartNodeLinkedList.h"
#include "ReferenceCount.h"

#include "global/unicode.h"


namespace GPlatesUtils
{
	/**
	 * An extension of class StringSet in which the strings are intended to be IDs.
	 *
	 * In addition to the regular uses of class StringSet, an element in this class can also
	 * hold references back to the objects (eg, features) for which the ID is the identifier
	 * (eg, its feature ID).  This makes it possible to determine at O(n) cost which objects
	 * contain a particular ID, rather than requiring an O(N) cost search through @em all
	 * objects which might contain the ID (where n is the number of objects which do contain
	 * the ID and N is the total number of objects -- which is usually much greater than n).
	 *
	 * Not all feature ID instances in memory identify an object; for example, the feature ID
	 * instance within a PropertyDelegate instance is not identifying the PropertyDelegate, but
	 * is instead indicating a particular feature (which may or may not be loaded).  Thus, not
	 * all ID instances result in a reference back to the containing object.
	 *
	 * The mechanism also allows for duplicate definitions of the same ID.
	 *
	 * When the object is deleted, its contained ID will also be deleted, which will result in
	 * the back-reference to the object being removed from the IdStringSet instance
	 * automatically.  This removes the problem of "dangling IDs", without requiring expensive
	 * data-structure construction and maintenance.
	 *
	 * For more on the behaviour and structure of this class, read the class comment for
	 * StringSet.
	 *
	 * As already mentioned, the feature ID of a feature will result in a back-reference to the
	 * feature, while the feature ID in a PropertyDelegate will not result in a back-reference.
	 * It is anticipated that, in the normal usage of the program, the number of features
	 * loaded will be significantly greater than the number of PropertyDelegates, so the number
	 * of IDs with back-references will be significantly greater than the number without.
	 * Further, it is assumed that in general, most (if not all) IDs will have one back-ref. 
	 * As a result, the classes below (particularly class UnicodeStringAndRefCountWithBackRef)
	 * were optimised for the presence of back-references.
	 */
	class IdStringSet
	{
	public:
		/**
		 * The abstract base class of back-references.
		 *
		 * This class basically serves as a type-safe union.  To get down to the derived
		 * class, use a dynamic_cast.
		 */
		class AbstractBackRef
		{
		public:
			virtual
			~AbstractBackRef() = 0;
		};

		/**
		 * The type of a back-reference.
		 *
		 * It is assumed that whatever manages the memory of the
		 * SmartNodeLinkedList Node will also manage the memory of the back-ref.
		 */
		typedef AbstractBackRef *back_ref_type;

		/**
		 * The type of a list of back-references.
		 */
		typedef SmartNodeLinkedList<back_ref_type> back_ref_list_type;


		/**
		 * This is the element which is contained in the @c std::set inside IdStringSetImpl.
		 */
		struct UnicodeStringAndRefCountWithBackRef
		{
			GPlatesUtils::UnicodeString d_str;
			mutable long d_ref_count;
			mutable back_ref_list_type d_back_refs;

			/**
			 * Construct a UnicodeStringAndRefCountWithBackRef instance for the
			 * UnicodeString instance @a str.
			 */
			explicit
			UnicodeStringAndRefCountWithBackRef(
					const GPlatesUtils::UnicodeString &str) :
				d_str(str),
				d_ref_count(0),
				d_back_refs(back_ref_type())
			{  }

			/**
			 * Create a copy-constructed UnicodeStringAndRefCountWithBackRef instance.
			 *
			 * This constructor should act exactly the same as the default
			 * (auto-generated) copy-constructor would, except that it should
			 * initialise the ref-count to zero.
			 *
			 * This constructor is necessary so that
			 * UnicodeStringAndRefCountWithBackRef can be stored as an element in
			 * @c std::set.
			 */
			UnicodeStringAndRefCountWithBackRef(
					const UnicodeStringAndRefCountWithBackRef &other):
				d_str(other.d_str),
				d_ref_count(0),
				d_back_refs(back_ref_type())
			{  }

			/**
			 * Provide a "less than" comparison for UnicodeStringAndRefCountWithBackRef
			 * instances.
			 *
			 * A "less than" comparison is used by 'std::set' to position elements in
			 * its internal balanced binary tree.
			 */
			bool
			operator<(
					const UnicodeStringAndRefCountWithBackRef &other) const
			{
				return GPLATES_ICU_BOOL(d_str < other.d_str);
			}
		private:
			/**
			 * Do not define the copy-assignment operator.
			 */
			UnicodeStringAndRefCountWithBackRef &
			operator=(
					const UnicodeStringAndRefCountWithBackRef &);
			
		};


		typedef std::set< UnicodeStringAndRefCountWithBackRef > collection_type;
		typedef collection_type::size_type size_type;


		/**
		 * A set of UnicodeString instances, each with an associated reference-count and a
		 * (possibly-empty) list of back-references.
		 *
		 * See the class comment for IdStringSet for more information.
		 */
		class IdStringSetImpl:
				public GPlatesUtils::ReferenceCount<IdStringSetImpl>
		{
		public:
			static
			const boost::intrusive_ptr<IdStringSetImpl>
			create()
			{
				boost::intrusive_ptr<IdStringSetImpl> ptr(new IdStringSetImpl());
				return ptr;
			}

			~IdStringSetImpl()
			{  }

			IdStringSet::collection_type &
			collection()
			{
				return d_collection;
			}

		private:
			IdStringSet::collection_type d_collection;

			// This constructor should not be public, because we don't want to allow
			// instantiation of this type on the stack.
			IdStringSetImpl()
			{  }

			// This constructor should never be defined, because we don't want/need to
			// allow copy-construction.
			IdStringSetImpl(
					const IdStringSetImpl &);

			// This operator should never be defined, because we don't want/need to
			// allow copy-assignment.
			IdStringSetImpl &
			operator=(
					const IdStringSetImpl &);
		};


		/**
		 * A reference to an element of an IdStringSet instance.
		 *
		 * @par Abstraction (black box) description:
		 *  -# The SharedIterator class represents a reference to an element of an
		 * IdStringSet instance.  It models (a subset of) the interface of a pointer.  An
		 * instance may be: default-constructed (resulting in an uninitialised reference);
		 * constructed with parameters (resulting in an initialised reference);
		 * copy-constructed (resulting in: another reference to the element which is
		 * referenced by the original SharedIterator instance, if the original instance was
		 * initialised; or another uninitialised instance, if the original SharedIterator
		 * instance was uninitialised); copy-assigned; swapped with another instance; and
		 * compared for equality or inequality with another instance.  An instance which is
		 * initialised may be dereferenced to access the (const) UnicodeString instance
		 * contained as the IdStringSet element.
		 *  -# All the instances which reference a particular element of IdStringSet are
		 * collectively responsible for managing that element:  When there are no more
		 * instances referencing a given element, the element is removed from the
		 * IdStringSet.  (Hence the name of the class:  The SharedIterator instances share
		 * the management of the lifetime of the element within the IdStringSet instance.)
		 *  -# A SharedIterator which is initialised will remain valid (able to be
		 * dereferenced) even if the IdStringSet instance itself no longer exists.
		 *
		 * @par Implementation (white box) description:
		 * (This description complements the abstraction description.)
		 *  -# An instance of SharedIterator contains an iterator into the collection
		 * contained within the IdStringSetImpl instance of the SharedIterator's
		 * IdStringSet.  It also contains a pointer-to-IdStringSetImpl.
		 *  -# If a SharedIterator instance was default-constructed, the contained iterator
		 * will be uninitialised and the pointer-to-IdStringSetImpl will be NULL.  Thus, by
		 * examining the pointer-to-IdStringSetImpl, it may be determined whether an
		 * instance was default-constructed or not.
		 *  -# If a SharedIterator instance was constructed with parameters, it will have
		 * been passed an iterator which is assumed to point into the @c std::set contained
		 * within an IdStringSetImpl, and a pointer-to-IdStringSetImple which is assumed to
		 * point to the IdStringSetImpl instance containing the @c std::set.  The
		 * SharedIterator instance will assume part of the responsibility for the
		 * management of the lifetime of the IdStringSetImpl instance.
		 *  -# Each element contained within the @c std::set inside an IdStringSetImpl
		 * instance is a UnicodeString instance with an associated reference-count.  When
		 * a SharedIterator instance is constructed with parameters, it is assumed to be
		 * referencing the an element within the @c std::set; the reference-count of the
		 * element will be incremented.
		 *  -# When a SharedIterator instance is copy-constructed, if the original
		 * SharedIterator instance references an element within the @c std::set, the
		 * newly-instantiated SharedIterator instance will reference that same element, and
		 * the reference-count of the element will be incremented.  If the original
		 * SharedIterator instance is uninitialised, the newly-instantiated instance will
		 * be uninitialised also.
		 *  -# When a SharedIterator instance is destroyed, if it referenced an element of
		 * the @c std::set, the reference-count of the element will be decremented; if the
		 * SharedIterator instance held the last reference to the element, the element will
		 * be removed from the @c std::set.  If the SharedIterator instance was the last
		 * SharedIterator or IdStringSet instance responsible for managing the lifetime of
		 * the IdStringSetImpl instance, the IdStringSetImpl instance will also be
		 * de-allocated.
		 *  -# When a SharedIterator instance is copy-assigned to another instance, the
		 * copy-assignment function acts to handle the increment/decrement of the number of
		 * references to elements of the @c std::set :  if a SharedIterator instance is
		 * being assigned to itself, there will be no net change in the number of
		 * references; if the l-value of the assignment referenced an element before the
		 * assignment, that reference will be undone (the reference-count will be
		 * decremented); if the r-value of the assignment references an element, the
		 * reference-count of the element will be incremented.  If the r-value of the
		 * assignment is an uninitialised instance, the l-value will become an
		 * uninitialised instance also.
		 *  -# Swapping two SharedIterator instances will result in no net change in the
		 * number of references to the elements referenced by the SharedIterator instances.
		 *  -# Comparing two SharedIterator instances for equality will return @c true if
		 * both instances are uninitialised, to enable client-code to determine whether an
		 * instance was initialised or not.  Aside from this, equality will describe
		 * whether the two SharedIterator instances reference the same element of the same
		 * IdStringSet instance or not.
		 *  -# Dereferencing a SharedIterator instance is only valid if the instance is
		 * initialised.
		 *
		 * @par Abstraction invariants:
		 *  -# A SharedIterator instance is either initialised (in which case it references
		 * a UnicodeString element of an IdStringSet instance) or uninitialised (in which
		 * case it does not reference anything).
		 *  -# A SharedIterator instance which is initialised may be dereferenced to access
		 * a (const) UnicodeString element of an IdStringSet instance; a SharedIterator
		 * instance which is uninitialised may not be dereferenced.
		 *
		 * @par Implementation invariants:
		 * (These collectively imply the abstraction invariants.)
		 *  -# Either the pointer-to-IdStringSetImpl is NULL, or it points to the
		 * IdStringSetImpl instance contained within an IdStringSet instance and the
		 * iterator points to an element of the @a std::set contained within the
		 * IdStringSetImpl instance.
		 *  -# If the pointer-to-IdStringSetImpl is non-NULL, the IdStringSetImpl instance
		 * will have a reference-count which is one greater than it would be if the
		 * pointer-to-IdStringSetImpl were not pointing to that IdStringSetImpl instance,
		 * and the UnicodeString element of the @c std::set will have a reference-count
		 * which is one greater than it would be if the iterator did not reference it.
		 */
		class SharedIterator
		{
		public:
			/**
			 * Construct a new SharedIterator instance which is uninitialised.
			 *
			 * This function will not throw.
			 */
			SharedIterator() :
					d_impl_ptr(NULL) {  }

			/**
			 * Construct a new SharedIterator instance which references a UnicodeString
			 * element of an IdStringSet instance.
			 *
			 * It is assumed that @a impl is a non-NULL pointer to an IdStringSetImpl
			 * instance, and @a iter points to an element of the @a std::set contained
			 * within the IdStringSetImpl instance.
			 *
			 * This function will not throw.
			 */
			SharedIterator(
					collection_type::iterator iter,
					boost::intrusive_ptr<IdStringSetImpl> impl) :
				d_iter(iter),
				d_impl_ptr(impl)
			{
				increment_ref_count();
			}

			/**
			 * Construct a copy of @a other.
			 *
			 * This function will not throw.
			 */
			SharedIterator(
					const SharedIterator &other) :
				d_iter(other.d_iter),
				d_impl_ptr(other.d_impl_ptr)
			{
				increment_ref_count();
			}

			/**
			 * Destroy this SharedIterator instance.
			 *
			 * This function will not throw.
			 */
			~SharedIterator()
			{
				decrement_ref_count();
			}

			/**
			 * Copy-assign @a other to this instance.
			 *
			 * @a return A reference to this instance.
			 *
			 * @pre True.
			 *
			 * @post If this instance was assigned to itself, there is no change to the
			 * reference-count of the referenced element of the IdStringSet instance
			 * (if this instance is initialised).  If this instance was not assigned to
			 * itself and it was initialised before the assignment, the reference-count
			 * of the referenced element of the IdStringSet instance is one less than it
			 * was before the assignment.  If this instance was not assigned to itself
			 * and @a other was initialised before the assignment, the reference-count
			 * of the referenced element contained within @a other is one greater than
			 * it was before the assignment.  If this instance was not assigned to
			 * itself and @a other was uninitialised before the assignment, this
			 * instance is now uninitialised.
			 *
			 * This function will not throw.
			 */
			SharedIterator &
			operator=(
					const SharedIterator &other)
			{
				if (this != &other)
				{
					// Use the copy constructor to get a duplicate copy.
					SharedIterator dup(other);
					this->swap(dup);
				}
				return *this;
			}

			/**
			 * Access the list of back-references for this IdStringSet element.
			 */
			back_ref_list_type &
			back_refs() const
			{
				return d_iter->d_back_refs;
			}

			/**
			 * Access the list of back-references for this IdStringSet element.
			 */
			back_ref_list_type &
			back_refs()
			{
				return d_iter->d_back_refs;
			}

			/**
			 * Swap the internals of this instance with @a other.
			 *
			 * @pre True.
			 *
			 * @post This instance now references the UnicodeString element which was
			 * referenced by @a other, if @a other was initialised; else, this instance
			 * is now uninitialised.  @a other now references the UnicodeString element
			 * which was referenced by this instance, if this instance was initialised;
			 * else @a other is now uninitialised.
			 *
			 * This function will not throw.
			 */
			void
			swap(
					SharedIterator &other)
			{
				std::swap(d_iter, other.d_iter);
				std::swap(d_impl_ptr, other.d_impl_ptr);
			}

			/**
			 * Determine whether this instance is equal to @a other.
			 *
			 * Two instances are considered equal if both instances are uninitialised,
			 * or if both instances reference the same element of the same IdStringSet.
			 *
			 * @return @c true if this instance is equal to @a other; @c false
			 * otherwise.
			 *
			 * @pre True.
			 *
			 * @post Return-value is @c true if this instance is equal to @a other;
			 * @c false otherwise.
			 *
			 * This function will not throw.
			 */
			bool
			operator==(
					const SharedIterator &other) const;

			/**
			 * Determine whether this instance is @em not equal to @a other.
			 *
			 * Two instances are considered equal if both instances are uninitialised,
			 * or if both instances reference the same element of the same IdStringSet.
			 *
			 * @return @c true if this instance is unequal to @a other; @c false
			 * otherwise.
			 *
			 * @pre True.
			 *
			 * @post Return-value is @c true if this instance is unequal to @a other;
			 * @c false otherwise.
			 *
			 * This function will not throw.
			 */
			bool
			operator!=(
					const SharedIterator &other) const
			{
				return ( ! (*this == other));
			}

			/**
			 * Dereference this instance to access the UnicodeString element which it
			 * references.
			 *
			 * Note that this operation is only valid if this instance is initialised.
			 *
			 * @return A reference to the const UnicodeString element referenced by
			 * this element.
			 *
			 * @pre This instance is initialised.
			 *
			 * @post Return-value is a reference to the UnicodeString element
			 * referenced by this element.
			 *
			 * This function will not throw.
			 */
			const GPlatesUtils::UnicodeString &
			operator*() const
			{
				return access_target();
			}

			/**
			 * Access a member of the UnicodeString element which is referenced by this
			 * instance.
			 *
			 * Note that this operation is only valid if this instance is initialised.
			 *
			 * @return A non-NULL pointer to the const UnicodeString element referenced
			 * by this element.
			 *
			 * @pre This instance is initialised.
			 *
			 * @post Return-value is a non-NULL pointer to the UnicodeString element
			 * referenced by this element.
			 *
			 * This function will not throw.
			 */
			const GPlatesUtils::UnicodeString *
			operator->() const
			{
				return &(access_target());
			}
		private:
			/**
			 * An iterator to an element in the std::set contained in IdStringSetImpl.
			 *
			 * The collection-type iterator is only meaningful if the impl-pointer is
			 * non-NULL (which means that the shared iterator instance is initialised).
			 */
			collection_type::iterator d_iter;

			/**
			 * An intrusive-pointer which manages the IdStringSetImpl instance.
			 *
			 * We need a pointer to the IdStringSetImpl instance (or the std::set which
			 * it contains) in order to be able to invoke the 'erase' member function
			 * of std::set.
			 *
			 * Since we have a pointer to the IdStringSetImpl instance, we're also
			 * using it to indicate (based upon whether it is NULL or non-NULL) whether
			 * this SharedIterator instance has been initialised yet.
			 *
			 * And since we have a pointer to the IdStringSetImpl instance, we might as
			 * well make it an intrusive-pointer, which manages the IdStringSetImpl
			 * instance, to ensure the pointer never becomes a dangling pointer.
			 */
			boost::intrusive_ptr<IdStringSetImpl> d_impl_ptr;

			const GPlatesUtils::UnicodeString &
			access_target() const
			{
				return d_iter->d_str;
			}

			void
			increment_ref_count();

			void
			decrement_ref_count();
		};

		/**
		 * Construct a new, empty IdStringSet instance.
		 *
		 * This function might throw in the case of memory exhaustion.  This function is
		 * strongly exception-safe and exception-neutral.
		 */
		IdStringSet() :
			d_impl(IdStringSetImpl::create())
		{  }

		/**
		 * Return the number of UnicodeString instances contained within the IdStringSet
		 * instance.
		 *
		 * @pre True.
		 *
		 * @post Return-value is the number of elements in the IdStringSet instance.
		 *
		 * This function will not throw.
		 */
		size_type
		size() const
		{
			return d_impl->collection().size();
		}

		/**
		 * Determine whether the IdStringSet instance contains the UnicodeString instance
		 * @a s, without modifying the contents of the IdStringSet instance.
		 * 
		 * @return a @c boost::optional instance which contains a SharedIterator instance
		 * which points to the element of the IdStringSet instance which matches the
		 * UnicodeString instance @a s; or @c boost::none if @a s is not contained within
		 * the IdStringSet instance.
		 *
		 * @pre True.
		 *
		 * @post Return-value is a @c boost::optional instance which contains a
		 * SharedIterator instance which points to the element of the IdStringSet instance
		 * which matches the UnicodeString instance @a s, or is @c boost::none if @a s is
		 * not contained within the IdStringSet instance.
		 *
		 * This function might throw whatever the copy-constructor and less-than-comparison
		 * operator of UnicodeString might throw.  This function is strongly exception-safe
		 * and exception-neutral.
		 */
		const boost::optional<SharedIterator>
		contains(
				const GPlatesUtils::UnicodeString &s) const;

		/**
		 * Obtain a SharedIterator instance which points to the UnicodeString instance
		 * @a s within an IdStringSet instance.
		 *
		 * If the UnicodeString instance @a s is not yet contained within the IdStringSet
		 * instance, it will be inserted (or an exception will be thrown, in the case of
		 * copy-construction failure or less-than-comparison failure for the UnicodeString
		 * instance, or memory allocation failure for @c std::set).
		 *
		 * @return The SharedIterator instance which points to the element of the
		 * IdStringSet instance which matches the UnicodeString instance @a s.
		 *
		 * @pre True.
		 *
		 * @post An element for the UnicodeString instance @a s exists in the IdStringSet
		 * instance, or an exception has been thrown.  Return-value is a SharedIterator
		 * instance which points to the element for the UnicodeString instance @a s.
		 *
		 * This function might throw whatever the copy-constructor and less-than-comparison
		 * operator of UnicodeString might throw, as well as whatever the @a insert
		 * function of @c std::set might throw.  This function is strongly exception-safe
		 * and exception-neutral.
		 */
		SharedIterator
		insert(
				const GPlatesUtils::UnicodeString &s);

	private:
		boost::intrusive_ptr<IdStringSetImpl> d_impl;

		// This constructor should never be defined, because we don't want to allow
		// copy-construction (since the copy-constructed instance might contain strings
		// with non-zero reference-counts, without SharedIterators referencing them).
		IdStringSet(
				const IdStringSet &);

		// This operator should never be defined, because we don't want to allow
		// copy-assignment (since the copy-assigned instance might contain strings with
		// non-zero reference-counts, without SharedIterators referencing them).
		IdStringSet &
		operator=(
				const IdStringSet &);
	};
}


namespace std
{
	template<>
	inline
	void
	swap<GPlatesUtils::IdStringSet::SharedIterator>(
			GPlatesUtils::IdStringSet::SharedIterator &sh_iter1,
			GPlatesUtils::IdStringSet::SharedIterator &sh_iter2)
	{
		sh_iter1.swap(sh_iter2);
	}
}

#endif  // GPLATES_UTILS_IDSTRINGSET_H
