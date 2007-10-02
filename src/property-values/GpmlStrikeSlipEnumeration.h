/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTYVALUES_GPMLSTRIKESLIPENUMERATION_H
#define GPLATES_PROPERTYVALUES_GPMLSTRIKESLIPENUMERATION_H

#include "model/PropertyValue.h"
#include "StrikeSlipEnumerationValue.h"


namespace GPlatesPropertyValues {

	class GpmlStrikeSlipEnumeration :
			public GPlatesModel::PropertyValue {

	public:

		typedef GPlatesContrib::non_null_intrusive_ptr<GpmlStrikeSlipEnumeration> 
				non_null_ptr_type;

		typedef GPlatesContrib::non_null_intrusive_ptr<const GpmlStrikeSlipEnumeration>
				non_null_ptr_to_const_type;

		virtual
		~GpmlStrikeSlipEnumeration() {  }

		static
		const non_null_ptr_type
		create(
				const UnicodeString &s) {
			GpmlStrikeSlipEnumeration::non_null_ptr_type ptr(*(new GpmlStrikeSlipEnumeration(s)));
			return ptr;
		}

		virtual
		const GPlatesModel::PropertyValue::non_null_ptr_type
		clone() const {
			GPlatesModel::PropertyValue::non_null_ptr_type dup(*(new GpmlStrikeSlipEnumeration(*this)));
			return dup;
		}

		const StrikeSlipEnumerationValue &
		value() const {
			return d_value;
		}

		virtual
		void
		accept_visitor(
				GPlatesModel::ConstFeatureVisitor &visitor) const {
			visitor.visit_gpml_strike_slip_enumeration(*this);
		}

		virtual
		void
		accept_visitor(
				GPlatesModel::FeatureVisitor &visitor) {
			visitor.visit_gpml_strike_slip_enumeration(*this);
		}

	protected:

		explicit
		GpmlStrikeSlipEnumeration(
				const UnicodeString &s) :
			PropertyValue(),
			d_value(s)
		{  }

		GpmlStrikeSlipEnumeration(
				const GpmlStrikeSlipEnumeration &other) :
			PropertyValue(other),
			d_value(other.d_value)
		{  }

	private:

		StrikeSlipEnumerationValue d_value;

		GpmlStrikeSlipEnumeration &
		operator=(const GpmlStrikeSlipEnumeration &);

	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLSTRIKESLIPENUMERATION_H
