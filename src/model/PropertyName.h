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

#ifndef GPLATES_MODEL_PROPERTYNAME_H
#define GPLATES_MODEL_PROPERTYNAME_H

#include "StringSetSingletons.h"


namespace GPlatesModel {

	/**
	 * This class provides an efficient means of containing the name of a property, which is a
	 * Unicode string.
	 *
	 * Since many properties share the same name, this class minimises memory usage for the
	 * storage of all these property names, by allowing them all to share a single string; each
	 * PropertyName instance stores an iterator to the shared string for its property name.
	 * Accessing the string is as inexpensive as dereferencing the iterator.
	 *
	 * Since the strings are unique in the StringSet, comparison for equality of property names
	 * is as simple as comparing a pair of iterators for equality.
	 *
	 * Since StringSet uses a 'std::set' for storage, testing whether an arbitrary Unicode
	 * string is a member of the StringSet has O(log n) cost.  Further, since all loaded
	 * property names are stored within the StringSet, it is inexpensive to test whether a
	 * desired property name is even loaded, without needing to iterate through all properties
	 * of all features.
	 */
	class PropertyName {

	public:

		/**
		 * Determine whether an arbitrary Unicode string is a member of the collection of
		 * loaded property names (without inserting the Unicode string into the
		 * collection).
		 */
		static
		bool
		is_loaded(
				const UnicodeString &s);

		/**
		 * Instantiate a new PropertyName instance for the given name.
		 */
		explicit
		PropertyName(
				const UnicodeString &name) :
			d_ss_iter(StringSetSingletons::property_name_instance().insert(name))
		{ }

		/**
		 * Access the Unicode string of the property name for this instance.
		 */
		const UnicodeString &
		get() const {
			return *d_ss_iter;
		}

		/**
		 * Determine whether another PropertyName instance contains the same property name
		 * as this instance.
		 */
		bool
		is_equal_to(
				const PropertyName &other) const {
			return d_ss_iter == other.d_ss_iter;
		}

	private:

		GPlatesUtil::StringSet::SharedIterator d_ss_iter;

	};


	inline
	bool
	GPlatesModel::PropertyName::is_loaded(
			const UnicodeString &s) {
		return StringSetSingletons::property_name_instance().contains(s);
	}


	inline
	bool
	operator==(
			const PropertyName &pn1,
			const PropertyName &pn2) {
		return pn1.is_equal_to(pn2);
	}


	inline
	bool
	operator!=(
			const PropertyName &pn1,
			const PropertyName &pn2) {
		return ! pn1.is_equal_to(pn2);
	}

}

#endif  // GPLATES_MODEL_PROPERTYNAME_H
