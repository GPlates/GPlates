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

#include <set>
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

		class SharedIterator
		{
		public:
			SharedIterator() :
					d_collection_ptr(NULL) {  }

			SharedIterator(
					collection_type::iterator iter,
					collection_type &collection) :
				d_iter(iter),
				d_collection_ptr(&collection)
			{
				increment_ref_count();
			}

			SharedIterator(
					const SharedIterator &other) :
				d_iter(other.d_iter),
				d_collection_ptr(other.d_collection_ptr)
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
				if (&other != this)
				{
					decrement_ref_count();
					d_iter = other.d_iter;
					d_collection_ptr = other.d_collection_ptr;
					increment_ref_count();
				}
				return *this;
			}

			bool
			operator==(
					const SharedIterator &other) const
			{
				if (d_collection_ptr != other.d_collection_ptr)
				{
					return false;
				}
				// else, the collection-pointers are equal.
				if (d_collection_ptr == NULL)
				{
					// Both collection-pointers are NULL.  This means that both
					// shared iterators were default-constructed.
					//
					// We'll make it so that all default-constructed shared
					// iterators compare equal so that it's possible to tell
					// whether a given shared iterator was default-constructed.
					return true;
				}
				return (d_iter == other.d_iter);
			}

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
			// The iterator is only meaningful if the collection-ptr is non-NULL (which
			// means that the shared iterator was neither default-constructed, nor
			// copy-constructed/copy-assigned from an iterator which was
			// default-constructed).
			collection_type::iterator d_iter;
			collection_type *d_collection_ptr;

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

		StringSet() {  }

		size_type
		size() const
		{
			return d_strings.size();
		}

		bool
		contains(
				const UnicodeString &s) const
		{
			UnicodeStringAndRefCount tmp(s);
			return (d_strings.find(tmp) != d_strings.end());
		}

		SharedIterator
		insert(
				const UnicodeString &s);

	private:
		collection_type d_strings;
	};
}

#endif  // GPLATES_UTIL_STRINGSET_H
