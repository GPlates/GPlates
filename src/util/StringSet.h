/* $Id: PropertyName.h 880 2006-10-09 08:16:28Z matty $ */

/**
 * \file
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2006-10-09 18:16:28 +1000 (Mon, 09 Oct 2006) $
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
			long d_ref_count;

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

		class SharedIterator
		{
		public:
			SharedIterator() :
					d_collection_ptr(NULL) {  }

			SharedIterator(
					std::set< UnicodeStringAndRefCount >::iterator iter,
					std::set< UnicodeStringAndRefCount > &collection) :
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

			~SharedIterator();

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

			UnicodeString &
			operator*()
			{
				return access_target();
			}

			const UnicodeString *
			operator->() const
			{
				return &(access_target());
			}

			UnicodeString *
			operator->()
			{
				return &(access_target());
			}
		private:
			// The iterator is only meaningful if the pointer is non-NULL (which means
			// that the shared iterator was *not* default-constructed).
			std::set< UnicodeStringAndRefCount >::iterator d_iter;
			std::set< UnicodeStringAndRefCount > *d_collection_ptr;

			const UnicodeString &
			access_target() const
			{
				return d_iter->d_str;
			}

			UnicodeString &
			access_target()
			{
				return d_iter->d_str;
			}

			void
			increment_ref_count()
			{
				++(d_iter->d_ref_count);
			}

			long
			decrement_ref_count()
			{
				return --(d_iter->d_ref_count);
			}
		};

		StringSet() {  }
	};
}

#endif  // GPLATES_UTIL_STRINGSET_H
