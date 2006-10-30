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

#ifndef GPLATES_MODEL_TEXTCONTENT_H
#define GPLATES_MODEL_TEXTCONTENT_H

#include "StringSetSingletons.h"


namespace GPlatesModel {

	/**
	 * This class provides an efficient means of containing text content, which is a Unicode
	 * string.
	 *
	 * Since it is anticipated that some text content will be contained within multiple feature
	 * instances (particularly since this class draws from the same "pool of strings" as the
	 * CachedStringRepresentation class), this class minimises memory usage for the storage of
	 * all these duplicate text content instances, by allowing them all to share a single
	 * string; each TextContent instance stores an iterator to the shared string for its text
	 * content.  Accessing the string is as inexpensive as dereferencing the iterator.
	 *
	 * Since the strings are unique in the StringSet, comparison for equality of text content
	 * instances is as simple as comparing a pair of iterators for equality.
	 *
	 * Since StringSet uses a 'std::set' for storage, testing whether an arbitrary Unicode
	 * string is a member of the StringSet has O(log n) cost.  Further, since all loaded text
	 * content instances are stored within the StringSet, it is inexpensive to test whether a
	 * desired text content instance is even loaded, without needing to iterate through all
	 * properties of all features.
	 */
	class TextContent {

	public:

		/**
		 * Determine whether an arbitrary Unicode string is a member of the collection of
		 * loaded text content instances (without inserting the Unicode string into the
		 * collection).
		 */
		static
		bool
		is_loaded(
				const UnicodeString &s);

		/**
		 * Instantiate a new TextContent instance for the given string.
		 */
		explicit
		TextContent(
				const UnicodeString &s) :
			d_ss_iter(StringSetSingletons::text_content_instance().insert(s))
		{ }

		/**
		 * Access the Unicode string of the text content for this instance.
		 */
		const UnicodeString &
		get() const {
			return *d_ss_iter;
		}

		/**
		 * Determine whether another TextContent instance contains the same text content
		 * as this instance.
		 */
		bool
		is_equal_to(
				const TextContent &other) const {
			return d_ss_iter == other.d_ss_iter;
		}

	private:

		GPlatesUtil::StringSet::SharedIterator d_ss_iter;

	};


	inline
	bool
	GPlatesModel::TextContent::is_loaded(
			const UnicodeString &s) {
		return StringSetSingletons::text_content_instance().contains(s);
	}


	inline
	bool
	operator==(
			const TextContent &tc1,
			const TextContent &tc2) {
		return tc1.is_equal_to(tc2);
	}


	inline
	bool
	operator!=(
			const TextContent &tc1,
			const TextContent &tc2) {
		return ! tc1.is_equal_to(tc2);
	}

}

#endif  // GPLATES_MODEL_TEXTCONTENT_H
