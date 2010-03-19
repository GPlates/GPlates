/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, 2008, 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTYVALUES_ENUMERATION_H
#define GPLATES_PROPERTYVALUES_ENUMERATION_H

#include "feature-visitors/PropertyValueFinder.h"
#include "model/PropertyValue.h"
#include "EnumerationContent.h"
#include "EnumerationType.h"


// Enable GPlatesFeatureVisitors::getPropertyValue() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::Enumeration, visit_enumeration)

namespace GPlatesPropertyValues {

	class Enumeration :
			public GPlatesModel::PropertyValue {

	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<Enumeration,
				GPlatesUtils::NullIntrusivePointerHandler> 
				non_null_ptr_type;

		typedef GPlatesUtils::non_null_intrusive_ptr<const Enumeration,
				GPlatesUtils::NullIntrusivePointerHandler>
				non_null_ptr_to_const_type;

		virtual
		~Enumeration() {  }

		// FIXME: enum_type should probably be a PropertyName.
		static
		const non_null_ptr_type
		create(
				const UnicodeString &enum_type,
				const UnicodeString &enum_content) {
			Enumeration::non_null_ptr_type ptr(new Enumeration(enum_type, enum_content),
					GPlatesUtils::NullIntrusivePointerHandler());
			return ptr;
		}

		const Enumeration::non_null_ptr_type
		clone() const {
			Enumeration::non_null_ptr_type dup(new Enumeration(*this),
					GPlatesUtils::NullIntrusivePointerHandler());
			return dup;
		}

		const Enumeration::non_null_ptr_type
		deep_clone() const
		{
			// This class doesn't reference any mutable objects by pointer, so there's
			// no need for any recursive cloning.  Hence, regular 'clone' will suffice.
			return clone();
		}

		DEFINE_FUNCTION_DEEP_CLONE_AS_PROP_VAL()

		const EnumerationContent &
		value() const {
			return d_value;
		}
		
		/**
		 * Set the content of this enumeration to @a new_value.
		 * EnumerationContent can be created by passing a UnicodeString in to
		 * EnumerationContent's constructor.
		 *
		 * FIXME: when we have undo/redo, this act should cause
		 * a new revision to be propagated up to the Feature which
		 * contains this PropertyValue.
		 */
		void
		set_value(
				const EnumerationContent &new_value) {
			d_value = new_value;
		}

		const EnumerationType &
		type() const {
			return d_type;
		}

		virtual
		void
		accept_visitor(
				GPlatesModel::ConstFeatureVisitor &visitor) const {
			visitor.visit_enumeration(*this);
		}

		virtual
		void
		accept_visitor(
				GPlatesModel::FeatureVisitor &visitor) {
			visitor.visit_enumeration(*this);
		}

	protected:

		explicit
		Enumeration(
				const UnicodeString &enum_type,
				const UnicodeString &enum_content) :
			PropertyValue(),
			d_type(enum_type),
			d_value(enum_content)
		{  }

		Enumeration(
				const Enumeration &other) :
			PropertyValue(other),
			d_type(other.d_type),
			d_value(other.d_value)
		{  }

	private:

		EnumerationType d_type;
		EnumerationContent d_value;

		Enumeration &
		operator=(const Enumeration &);

	};

}

#endif  // GPLATES_PROPERTYVALUES_ENUMERATION_H
