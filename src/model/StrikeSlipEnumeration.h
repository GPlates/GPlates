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

#ifndef GPLATES_MODEL_STRIKESLIPENUMERATION_H
#define GPLATES_MODEL_STRIKESLIPENUMERATION_H

#include "PropertyValue.h"
#include "StrikeSlipEnumerationValue.h"


namespace GPlatesModel {

	class StrikeSlipEnumeration :
			public PropertyValue {

	public:

		typedef GPlatesContrib::non_null_intrusive_ptr<StrikeSlipEnumeration> non_null_ptr_type;

		typedef GPlatesContrib::non_null_intrusive_ptr<const StrikeSlipEnumeration>
				non_null_ptr_to_const_type;

		virtual
		~StrikeSlipEnumeration() {  }

		static
		const non_null_ptr_type
		create(
				const UnicodeString &s) {
			StrikeSlipEnumeration::non_null_ptr_type ptr(*(new StrikeSlipEnumeration(s)));
			return ptr;
		}

		virtual
		const PropertyValue::non_null_ptr_type
		clone() const {
			PropertyValue::non_null_ptr_type dup(*(new StrikeSlipEnumeration(*this)));
			return dup;
		}

		const StrikeSlipEnumerationValue &
		value() const {
			return d_value;
		}

		virtual
		void
		accept_visitor(
				ConstFeatureVisitor &visitor) const {
			visitor.visit_strike_slip_enumeration(*this);
		}

		virtual
		void
		accept_visitor(
				FeatureVisitor &visitor) {
			visitor.visit_strike_slip_enumeration(*this);
		}

	protected:

		explicit
		StrikeSlipEnumeration(
				const UnicodeString &s) :
			PropertyValue(),
			d_value(s)
		{  }

		StrikeSlipEnumeration(
				const StrikeSlipEnumeration &other) :
			PropertyValue(other),
			d_value(other.d_value)
		{  }

	private:

		StrikeSlipEnumerationValue d_value;

		StrikeSlipEnumeration &
		operator=(const StrikeSlipEnumeration &);

	};

}

#endif  // GPLATES_MODEL_STRIKESLIPENUMERATION_H
