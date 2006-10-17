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

		StringSet() :
			d_impl(StringSetImpl::create())
		{  }

		size_type
		size() const
		{
			return d_impl->collection().size();
		}

		bool
		contains(
				const UnicodeString &s) const
		{
			UnicodeStringAndRefCount tmp(s);
			return (d_impl->collection().find(tmp) != d_impl->collection().end());
		}

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
