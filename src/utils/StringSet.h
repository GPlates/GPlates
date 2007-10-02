/* $Id$ */

/**
 * \file
 * File specific comments.
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

#ifndef GPLATES_UTILS_STRINGSET_H
#define GPLATES_UTILS_STRINGSET_H

#include <algorithm>
#include <set>
#include <boost/intrusive_ptr.hpp>
#include <unicode/unistr.h>


namespace GPlatesUtils {

	/**
	 * A set of UnicodeString instances.
	 *
	 * @par Abstraction (black box) description:
	 *  -# The StringSet class represents a set (an unordered collection of unique, immutable
	 * elements) of UnicodeString instances.
	 *  -# The @a size member function returns the number of UnicodeString instances contained
	 * within a StringSet instance.
	 *  -# It is possible to determine whether a StringSet instance contains a particular
	 * UnicodeString instance, without modifying the contents of the StringSet instance, using
	 * the @a contains member function.  This member function will return @c true if the
	 * supplied UnicodeString instance is contained within the StringSet instance; @c false
	 * otherwise.
	 *  -# The elements contained within a StringSet instance are accessed through
	 * SharedIterator instances.  To obtain a SharedIterator instance which points to a
	 * particular UnicodeString instance within a StringSet instance, use the @a insert member
	 * function.  If the supplied UnicodeString instance was not already contained within the
	 * StringSet instance, it will be inserted (or an exception will be thrown in the case of
	 * memory exhaustion).  The SharedIterator instance which is returned will point to an
	 * element of the StringSet instance matching the supplied UnicodeString instance.
	 *  -# A UnicodeString instance is only contained within a StringSet instance as long as
	 * there is one or more SharedIterator instances which reference it.  When there are no
	 * more SharedIterator instances referencing an element of the StringSet instance, the
	 * element will be automatically removed.
	 *  -# A StringSet instance is neither copy-constructable nor copy-assignable, as the
	 * copy-constructed/copy-assigned instance might then contain UnicodeString instances with
	 * non-zero reference-counts, without SharedIterators associated with that StringSet
	 * instance referencing them.
	 *
	 * @par Implementation (white box) description:
	 * (This description complements the abstraction description.)
	 *  -# The conceptual StringSet is implemented using two classes: StringSet and
	 * StringSetImpl.  A StringSet instance contains a pointer-to-StringSetImpl (the
	 * "impl-pointer") which points to a StringSetImpl instance allocated in the StringSet's
	 * constructor.  The StringSet instance wraps the StringSetImpl, providing an interface to
	 * manipulate its contents.  The StringSet instance also assumes part of the responsibility
	 * for the management of the lifetime of the StringSetImpl.
	 *  -# A StringSetImpl instance contains a @c std::set of UnicodeString instances, each
	 * with an associated reference-count.  The UnicodeString instance and associated
	 * reference-count together compose an element of the @c std::set, although only the
	 * UnicodeString instance is accessible by clients of StringSet (the reference-count is not
	 * part of the class abstraction).  The reference-count of an element is the number of
	 * SharedIterator instances which currently reference that element.
	 *  -# Since a UnicodeString instance is only contained within the conceptual StringSet
	 * instance as long as there are one or more SharedIterator instances which reference the
	 * StringSet element, every element in StringSetImpl's @c std::set has a reference-count
	 * which is greater than zero.  When the reference-count reaches zero, the element is
	 * removed.
	 *
	 * @par Abstraction invariants:
	 *  -# The StringSet instance contains the set of UnicodeStrings which have been inserted
	 * by client code (using the @a insert member function), for which the number of
	 * SharedIterator instances pointing to each element has not reached zero since the
	 * element's UnicodeString was last inserted.
	 *  -# All SharedIterator instances which point to elements of the StringSet instance are
	 * valid (ie, they may be dereferenced to access an element of the class).
	 *
	 * @par Implementation invariants:
	 * (These collectively imply the abstraction invariants.)
	 *  -# The impl-pointer is not NULL:  It points to a StringSetImpl instance which is not
	 * pointed-to by the impl-pointer of any other StringSet instance.
	 *  -# The reference-count of each element corresponds to the number of SharedIterator
	 * instances referencing that element.
	 *  -# The @c std::set contains at most one element for any UnicodeString instance.  (Note
	 * that this is not automatically implied by the uniqueness of elements in the @a std::set,
	 * since an element of the @c std::set is a UnicodeString instance and a reference-count.)
	 *  -# The location in memory of the element for a particular UnicodeString will not change
	 * as long as the reference count of that element is greater than zero.
	 *  -# The class does not contain any elements which have a reference-count less than one.
	 */
	class StringSet
	{
	public:
		/**
		 * This is the element which is contained in the @c std::set inside StringSetImpl.
		 */
		struct UnicodeStringAndRefCount
		{
			UnicodeString d_str;
			mutable long d_ref_count;

			/**
			 * Construct a UnicodeStringAndRefCount instance for the UnicodeString
			 * instance @a str.
			 */
			explicit
			UnicodeStringAndRefCount(
					const UnicodeString &str) :
				d_str(str),
				d_ref_count(0) {  }

			/**
			 * Create a copy-constructed UnicodeStringAndRefCount instance.
			 *
			 * This constructor should act exactly the same as the default
			 * (auto-generated) copy-constructor would, except that it should
			 * initialise the ref-count to zero.
			 *
			 * This constructor is necessary so that UnicodeStringAndRefCount can be
			 * stored as an element in @c std::set.
			 */
			UnicodeStringAndRefCount(
					const UnicodeStringAndRefCount &other):
				d_str(other.d_str),
				d_ref_count(0)
			{  }

			/**
			 * Provide a "less than" comparison for UnicodeStringAndRefCount instances.
			 *
			 * A "less than" comparison is used by 'std::set' to position elements in
			 * its internal balanced binary tree.
			 */
			bool
			operator<(
					const UnicodeStringAndRefCount &other) const
			{
				return (d_str < other.d_str);
			}
		private:
			/**
			 * Do not define the copy-assignment operator.
			 */
			UnicodeStringAndRefCount &
			operator=(
					const UnicodeStringAndRefCount &);
		};


		typedef std::set< UnicodeStringAndRefCount > collection_type;
		typedef collection_type::size_type size_type;


		/**
		 * A set of UnicodeString instances, each with an associated reference-count.
		 *
		 * See the class comment for StringSet for more information.
		 */
		class StringSetImpl
		{
		public:
			typedef long ref_count_type;

			static
			const boost::intrusive_ptr<StringSetImpl>
			create()
			{
				boost::intrusive_ptr<StringSetImpl> ptr(new StringSetImpl());
				return ptr;
			}

			~StringSetImpl()
			{  }

			void
			increment_ref_count()
			{
				++d_ref_count;
			}

			ref_count_type
			decrement_ref_count()
			{
				return --d_ref_count;
			}

			StringSet::collection_type &
			collection()
			{
				return d_collection;
			}

		private:
			ref_count_type d_ref_count;
			StringSet::collection_type d_collection;

			// This constructor should not be public, because we don't want to allow
			// instantiation of this type on the stack.
			StringSetImpl() :
				d_ref_count(0)
			{  }

			// This constructor should never be defined, because we don't want/need to
			// allow copy-construction.
			StringSetImpl(
					const StringSetImpl &);

			// This operator should never be defined, because we don't want/need to
			// allow copy-assignment.
			StringSetImpl &
			operator=(
					const StringSetImpl &);
		};


		/**
		 * A reference to an element of a StringSet instance.
		 *
		 * @par Abstraction (black box) description:
		 *  -# The SharedIterator class represents a reference to an element of a StringSet
		 * instance.  It models (a subset of) the interface of a pointer.  An instance may
		 * be: default-constructed (resulting in an uninitialised reference); constructed
		 * with parameters (resulting in an initialised reference); copy-constructed
		 * (resulting in: another reference to the element which is referenced by the
		 * original SharedIterator instance, if the original instance was initialised; or
		 * another uninitialised instance, if the original SharedIterator instance was
		 * uninitialised); copy-assigned; swapped with another instance; and compared for
		 * equality or inequality with another instance.  An instance which is initialised
		 * may be dereferenced to access the (const) UnicodeString instance contained as
		 * the StringSet element.
		 *  -# All the instances which reference a given element of StringSet are
		 * collectively responsible for managing that element:  When there are no more
		 * instances referencing a given element, the element is removed from the
		 * StringSet.
		 *  -# A SharedIterator which is initialised will remain valid (able to be
		 * dereferenced) even if the StringSet instance itself no longer exists.
		 *
		 * @par Implementation (white box) description:
		 * (This description complements the abstraction description.)
		 *  -# An instance of SharedIterator contains an iterator into the collection
		 * contained within the StringSetImpl instance of the SharedIterator's StringSet.
		 * It also contains a pointer-to-StringSetImpl.
		 *  -# If a SharedIterator instance was default-constructed, the contained iterator
		 * will be uninitialised and the pointer-to-StringSetImpl will be NULL.  Thus, by
		 * examining the pointer-to-StringSetImpl, it may be determined whether an instance
		 * was default-constructed or not.
		 *  -# If a SharedIterator instance was constructed with parameters, it will have
		 * been passed an iterator which is assumed to point into the @c std::set contained
		 * within a StringSetImpl, and a pointer-to-StringSetImple which is assumed to
		 * point to the StringSetImpl instance containing the @c std::set.  The
		 * SharedIterator instance will assume part of the responsibility for the
		 * management of the lifetime of the StringSetImpl instance.
		 *  -# Each element contained within the @c std::set inside a StringSetImpl
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
		 * SharedIterator or StringSet instance responsible for managing the lifetime of
		 * the StringSetImpl instance, the StringSetImpl instance will also be
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
		 * StringSet instance or not.
		 *  -# Dereferencing a SharedIterator instance is only valid if the instance is
		 * initialised.
		 *
		 * @par Abstraction invariants:
		 *  -# A SharedIterator instance is either initialised (in which case it references
		 * a UnicodeString element of a StringSet instance) or uninitialised (in which case
		 * it does not reference anything).
		 *  -# A SharedIterator instance which is initialised may be dereferenced to access
		 * a (const) UnicodeString element of a StringSet instance; a SharedIterator
		 * instance which is uninitialised may not be dereferenced.
		 *
		 * @par Implementation invariants:
		 * (These collectively imply the abstraction invariants.)
		 *  -# Either the pointer-to-StringSetImpl is NULL, or it points to the
		 * StringSetImpl instance contained within a StringSet instance and the iterator
		 * points to an element of the @a std::set contained within the StringSetImpl
		 * instance.
		 *  -# If the pointer-to-StringSetImpl is non-NULL, the StringSetImpl instance will
		 * have a reference-count which is one greater than it would be if the
		 * pointer-to-StringSetImpl were not pointing to that StringSetImpl instance, and
		 * the UnicodeString element of the @c std::set will have a reference-count which
		 * is one greater than it would be if the iterator did not reference it.
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
			 * element of a StringSet instance.
			 *
			 * It is assumed that @a impl is a non-NULL pointer to a StringSetImpl
			 * instance, and @a iter points to an element of the @a std::set contained
			 * within the StringSetImpl instance.
			 *
			 * This function will not throw.
			 */
			SharedIterator(
					collection_type::iterator iter,
					boost::intrusive_ptr<StringSetImpl> impl) :
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
			 * reference-count of the referenced element of the StringSet instance (if
			 * this instance is initialised).  If this instance was not assigned to
			 * itself and it was initialised before the assignment, the reference-count
			 * of the referenced element of the StringSet instance is one less than it
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
			 * or if both instances reference the same element of the same StringSet.
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
			 * or if both instances reference the same element of the same StringSet.
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
			const UnicodeString &
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
			const UnicodeString *
			operator->() const
			{
				return &(access_target());
			}
		private:
			// The collection-type iterator is only meaningful if the impl-pointer is
			// non-NULL (which means that the shared iterator instance is initialised).
			collection_type::iterator d_iter;
			boost::intrusive_ptr<StringSetImpl> d_impl_ptr;

			const UnicodeString &
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
		 * Construct a new, empty StringSet instance.
		 *
		 * This function might throw in the case of memory exhaustion.  This function is
		 * strongly exception-safe and exception-neutral.
		 */
		StringSet() :
			d_impl(StringSetImpl::create())
		{  }

		/**
		 * Return the number of UnicodeString instances contained within the StringSet
		 * instance.
		 *
		 * @pre True.
		 *
		 * @post Return-value is the number of elements in the StringSet instance.
		 *
		 * This function will not throw.
		 */
		size_type
		size() const
		{
			return d_impl->collection().size();
		}

		/**
		 * Determine whether the StringSet instance contains the UnicodeString instance
		 * @a s, without modifying the contents of the StringSet instance.
		 * 
		 * @return @c true if @a s is contained within the StringSet instance; @c false
		 * otherwise.
		 *
		 * @pre True.
		 *
		 * @post Return-value is @c true if the StringSet instance contains an element for
		 * the UnicodeString instance @a s; @c false otherwise.
		 *
		 * This function might throw whatever the copy-constructor and less-than-comparison
		 * operator of UnicodeString might throw.  This function is strongly exception-safe
		 * and exception-neutral.
		 */
		bool
		contains(
				const UnicodeString &s) const
		{
			UnicodeStringAndRefCount tmp(s);
			return (d_impl->collection().find(tmp) != d_impl->collection().end());
		}

		/**
		 * Obtain a SharedIterator instance which points to the UnicodeString instance
		 * @a s within a StringSet instance.
		 *
		 * If the UnicodeString instance @a s is not yet contained within the StringSet
		 * instance, it will be inserted (or an exception will be thrown, in the case of
		 * copy-construction failure or less-than-comparison failure for the UnicodeString
		 * instance, or memory allocation failure for @c std::set).
		 *
		 * @return The SharedIterator instance which points to the element of the StringSet
		 * instance which matches the UnicodeString instance @a s.
		 *
		 * @pre True.
		 *
		 * @post An element for the UnicodeString instance @a s exists in the StringSet
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
				const UnicodeString &s);

	private:
		boost::intrusive_ptr<StringSetImpl> d_impl;

		// This constructor should never be defined, because we don't want to allow
		// copy-construction (since the copy-constructed instance might contain strings
		// with non-zero reference-counts, without SharedIterators referencing them).
		StringSet(
				const StringSet &);

		// This operator should never be defined, because we don't want to allow
		// copy-assignment (since the copy-assigned instance might contain strings with
		// non-zero reference-counts, without SharedIterators referencing them).
		StringSet &
		operator=(
				const StringSet &);
	};


	inline
	void
	intrusive_ptr_add_ref(
			StringSet::StringSetImpl *p)
	{
		p->increment_ref_count();
	}


	inline
	void
	intrusive_ptr_release(
			StringSet::StringSetImpl *p)
	{
		if (p->decrement_ref_count() == 0)
		{
			delete p;
		}
	}
}


namespace std
{
	template<>
	inline
	void
	swap<GPlatesUtils::StringSet::SharedIterator>(
			GPlatesUtils::StringSet::SharedIterator &sh_iter1,
			GPlatesUtils::StringSet::SharedIterator &sh_iter2)
	{
		sh_iter1.swap(sh_iter2);
	}
}

#endif  // GPLATES_UTILS_STRINGSET_H
