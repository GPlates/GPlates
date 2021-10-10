/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, 2008 The University of Sydney, Australia
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

#include "model/PropertyValue.h"
#include "EnumerationContent.h"
#include "EnumerationType.h"


namespace GPlatesPropertyValues {

	class Enumeration :
			public GPlatesModel::PropertyValue {

	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<Enumeration> 
				non_null_ptr_type;

		typedef GPlatesUtils::non_null_intrusive_ptr<const Enumeration>
				non_null_ptr_to_const_type;

		virtual
		~Enumeration() {  }

		static
		const non_null_ptr_type
		create(
				const UnicodeString &enum_type,
				const UnicodeString &enum_content) {
			Enumeration::non_null_ptr_type ptr(*(new Enumeration(enum_type, enum_content)));
			return ptr;
		}

		virtual
		const GPlatesModel::PropertyValue::non_null_ptr_type
		clone() const {
			GPlatesModel::PropertyValue::non_null_ptr_type dup(*(new Enumeration(*this)));
			return dup;
		}

		const EnumerationContent &
		value() const {
			return d_value;
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
