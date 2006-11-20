/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_XMLATTRIBUTEVALUE_H
#define GPLATES_MODEL_XMLATTRIBUTEVALUE_H

#include "StringSetSingletons.h"


namespace GPlatesModel {

	/**
	 * This class provides an efficient means of containing an XML attribute value, which is a
	 * Unicode string.
	 *
	 * Since many XML attributes share the same value, this class minimises memory usage for
	 * the storage of all these attribute values, by allowing them all to share a single
	 * string; each XmlAttributeValue instance stores an iterator to the shared string for its
	 * attribute value.  Accessing the string is as inexpensive as dereferencing the iterator.
	 *
	 * Since the strings are unique in the StringSet, comparison for equality of attribute
	 * values is as simple as comparing a pair of iterators for equality.
	 *
	 * Since StringSet uses a 'std::set' for storage, testing whether an arbitrary Unicode
	 * string is a member of the StringSet has O(log n) cost.  Further, since all loaded
	 * attribute values are stored within the StringSet, it is inexpensive to test whether a
	 * desired attribute values is even loaded, without needing to iterate through all
	 * properties of all features.
	 */
	class XmlAttributeValue {

	public:

		/**
		 * Determine whether an arbitrary Unicode string is a member of the collection of
		 * loaded attribute values (without inserting the Unicode string into the
		 * collection).
		 */
		static
		bool
		is_loaded(
				const UnicodeString &s);

		/**
		 * Instantiate a new XmlAttributeValue instance for the given value.
		 */
		explicit
		XmlAttributeValue(
				const UnicodeString &value) :
			d_ss_iter(StringSetSingletons::xml_attribute_value_instance().insert(value))
		{ }

		/**
		 * Access the Unicode string of the attribute value for this instance.
		 */
		const UnicodeString &
		get() const {
			return *d_ss_iter;
		}

		/**
		 * Determine whether another XmlAttributeValue instance contains the same attribute
		 * value as this instance.
		 */
		bool
		is_equal_to(
				const XmlAttributeValue &other) const {
			return d_ss_iter == other.d_ss_iter;
		}

	private:

		GPlatesUtil::StringSet::SharedIterator d_ss_iter;

	};


	inline
	bool
	GPlatesModel::XmlAttributeValue::is_loaded(
			const UnicodeString &s) {
		return StringSetSingletons::xml_attribute_value_instance().contains(s);
	}


	inline
	bool
	operator==(
			const XmlAttributeValue &xav1,
			const XmlAttributeValue &xav2) {
		return xav1.is_equal_to(xav2);
	}


	inline
	bool
	operator!=(
			const XmlAttributeValue &xav1,
			const XmlAttributeValue &xav2) {
		return ! xav1.is_equal_to(xav2);
	}

}

#endif  // GPLATES_MODEL_XMLATTRIBUTEVALUE_H
