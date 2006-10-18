/* $Id$ */

/**
 * \file
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 *
 * Copyright (C) 2006 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef GPLATES_UTIL_STRINGSET_H
#define GPLATES_UTIL_STRINGSET_H

#include <algorithm>
#include <set>
#include <boost/intrusive_ptr.hpp>
#include <unicode/unistr.h>


namespace GPlatesUtil {

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
	 *
	 * @par Implementation (white box) description:
	 * (This description complements the abstraction description.)
	 *  -# The conceptual StringSet is implemented using two classes: StringSet and
	 * StringSetImpl. A StringSet instance contains a pointer-to-StringSetImpl (the
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
	 *  -# The class contains the set of UnicodeStrings which have been inserted by client code
	 * (using the @a insert member function), for which the number of SharedIterator instances
	 * pointing to each element has not reached zero since the element's UnicodeString was last
	 * inserted.
	 *  -# All SharedIterator instances which point to elements of the class are valid (ie,
	 * they may be dereferenced to access an element of the class).
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
		struct UnicodeStringAndRefCount
		{
			UnicodeString d_str;
			mutable long d_ref_count;

			explicit
			UnicodeStringAndRefCount(
					const UnicodeString &str) :
				d_str(str),
				d_ref_count(0) {  }

			bool
			operator<(
					const UnicodeStringAndRefCount &other) const
			{
				return (d_str < other.d_str);
			}
		};


		typedef std::set< UnicodeStringAndRefCount > collection_type;
		typedef collection_type::size_type size_type;


		class StringSetImpl
		{
		public:
			typedef long ref_count_type;

			static
			boost::intrusive_ptr<StringSetImpl>
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


		class SharedIterator
		{
		public:
			SharedIterator() :
					d_impl_ptr(NULL) {  }

			SharedIterator(
					collection_type::iterator iter,
					boost::intrusive_ptr<StringSetImpl> impl) :
				d_iter(iter),
				d_impl_ptr(impl)
			{
				increment_ref_count();
			}

			SharedIterator(
					const SharedIterator &other) :
				d_iter(other.d_iter),
				d_impl_ptr(other.d_impl_ptr)
			{
				increment_ref_count();
			}

			~SharedIterator()
			{
				decrement_ref_count();
			}

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

			void
			swap(
					SharedIterator &other)
			{
				std::swap(d_iter, other.d_iter);
				std::swap(d_impl_ptr, other.d_impl_ptr);
			}

			bool
			operator==(
					const SharedIterator &other) const;

			bool
			operator!=(
					const SharedIterator &other) const
			{
				return ( ! (*this == other));
			}

			const UnicodeString &
			operator*() const
			{
				return access_target();
			}

			const UnicodeString *
			operator->() const
			{
				return &(access_target());
			}
		private:
			// The collection-type iterator is only meaningful if the impl-pointer is
			// non-NULL (which means that the shared iterator instance was neither
			// default-constructed, nor copy-constructed/copy-assigned from an instance
			// which was default-constructed).
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
		 * instance, it will be inserted (or an exception will be thrown in the case of
		 * memory exhaustion).
		 *
		 * @return The SharedIterator instance which points to the element of the StringSet
		 * instance which matches the UnicodeString instance @a s.
		 *
		 * @pre True.
		 *
		 * @post An element for the UnicodeString instance @a s exists in the StringSet
		 * instance, or an exception has been thrown.  Return-value is a SharedIterator
		 * instance which points to the element for the UnicodeString instance @a s.
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
	swap<GPlatesUtil::StringSet::SharedIterator>(
			GPlatesUtil::StringSet::SharedIterator &sh_iter1,
			GPlatesUtil::StringSet::SharedIterator &sh_iter2)
	{
		sh_iter1.swap(sh_iter2);
	}
}

#endif  // GPLATES_UTIL_STRINGSET_H
