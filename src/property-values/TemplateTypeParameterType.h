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

#ifndef GPLATES_PROPERTYVALUES_TEMPLATETYPEPARAMETERTYPE_H
#define GPLATES_PROPERTYVALUES_TEMPLATETYPEPARAMETERTYPE_H

#include "model/StringSetSingletons.h"


namespace GPlatesPropertyValues {

	/**
	 * This class provides an efficient means of containing the type of a template
	 * type-parameter, which is a Unicode string.
	 *
	 * Since many type-parameters share the same type, this class minimises memory usage for
	 * the storage of all these type-parameter types, by allowing them all to share a single
	 * string; each TemplateTypeParameterType instance stores an iterator to the shared string
	 * for its type-parameter type.  Accessing the string is as inexpensive as dereferencing
	 * the iterator.
	 *
	 * Since the strings are unique in the StringSet, comparison for equality of type-parameter
	 * types is as simple as comparing a pair of iterators for equality.
	 *
	 * Since StringSet uses a 'std::set' for storage, testing whether an arbitrary Unicode
	 * string is a member of the StringSet has O(log n) cost.  Further, since all loaded
	 * type-parameter types are stored within the StringSet, it is inexpensive to test
	 * whether a desired type-parameter type is even loaded, without needing to iterate through
	 * all properties of all features.
	 */
	class TemplateTypeParameterType {

	public:

		/**
		 * Determine whether an arbitrary Unicode string is a member of the collection of
		 * loaded type-parameter types (without inserting the Unicode string into the
		 * collection).
		 */
		static
		bool
		is_loaded(
				const UnicodeString &s);

		/**
		 * Instantiate a new TemplateTypeParameterType instance for the given type.
		 */
		explicit
		TemplateTypeParameterType(
				const UnicodeString &type) :
			d_ss_iter(GPlatesModel::StringSetSingletons::template_type_parameter_type_instance().insert(type))
		{ }

		/**
		 * Access the Unicode string of the type-parameter type for this instance.
		 */
		const UnicodeString &
		get() const {
			return *d_ss_iter;
		}

		/**
		 * Determine whether another TemplateTypeParameterType instance contains the same
		 * type-parameter type as this instance.
		 */
		bool
		is_equal_to(
				const TemplateTypeParameterType &other) const {
			return d_ss_iter == other.d_ss_iter;
		}

	private:

		GPlatesUtils::StringSet::SharedIterator d_ss_iter;

	};


	inline
	bool
	GPlatesPropertyValues::TemplateTypeParameterType::is_loaded(
			const UnicodeString &s) {
		return GPlatesModel::StringSetSingletons::feature_type_instance().contains(s);
	}


	inline
	bool
	operator==(
			const TemplateTypeParameterType &ttpt1,
			const TemplateTypeParameterType &ttpt2) {
		return ttpt1.is_equal_to(ttpt2);
	}


	inline
	bool
	operator!=(
			const TemplateTypeParameterType &ttpt1,
			const TemplateTypeParameterType &ttpt2) {
		return ! ttpt1.is_equal_to(ttpt2);
	}

}

#endif  // GPLATES_PROPERTYVALUES_TEMPLATETYPEPARAMETERTYPE_H
