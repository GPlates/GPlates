/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_GPMLCONSTANTVALUE_H
#define GPLATES_MODEL_GPMLCONSTANTVALUE_H

#include "PropertyValue.h"
#include "TemplateTypeParameterType.h"


namespace GPlatesModel {

	class GpmlConstantValue :
			public PropertyValue {

	public:
		/**
		 * A convenience typedef for GPlatesContrib::non_null_intrusive_ptr<GpmlConstantValue>.
		 */
		typedef GPlatesContrib::non_null_intrusive_ptr<GpmlConstantValue> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesContrib::non_null_intrusive_ptr<const GpmlConstantValue>.
		 */
		typedef GPlatesContrib::non_null_intrusive_ptr<const GpmlConstantValue>
				non_null_ptr_to_const_type;


		virtual
		~GpmlConstantValue() {  }

		// This creation function is here purely for the simple, hard-coded construction of
		// features.  It may not be necessary or appropriate later on when we're doing
		// everything properly, so don't look at this function and think "Uh oh, this
		// function doesn't look like it should be here, but I'm sure it's here for a
		// reason..."
		static
		const non_null_ptr_type
		create(
				PropertyValue::non_null_ptr_type value_,
				const TemplateTypeParameterType &value_type_) {
			non_null_ptr_type ptr(*(new GpmlConstantValue(value_, value_type_)));
			return ptr;
		}

		virtual
		const PropertyValue::non_null_ptr_type
		clone() const {
			PropertyValue::non_null_ptr_type dup(*(new GpmlConstantValue(*this)));
			return dup;
		}

		const PropertyValue::non_null_ptr_to_const_type
		value() const {
			return d_value;
		}

		// Note that, because the copy-assignment operator of PropertyValue is private,
		// the PropertyValue referenced by the return-value of this function cannot be
		// assigned-to, which means that this function does not provide a means to directly
		// switch the PropertyValue within this GpmlConstantValue instance.  (This
		// restriction is intentional.)
		//
		// To switch the PropertyValue within this GpmlConstantValue instance, use the
		// function @a set_value below.
		//
		// (This overload is provided to allow the referenced PropertyValue instance to
		// accept a FeatureVisitor instance.)
		const PropertyValue::non_null_ptr_type
		value() {
			return d_value;
		}

		void
		set_value(
				PropertyValue::non_null_ptr_type v) {
			d_value = v;
		}

		// Note that no "setter" is provided:  The value type of a GpmlConstantValue
		// instance should never be changed.
		const TemplateTypeParameterType &
		value_type() const {
			return d_value_type;
		}

		/**
		 * Accept a ConstFeatureVisitor instance.
		 *
		 * See the Visitor pattern (p.331) in Gamma95 for information on the purpose of
		 * this function.
		 */
		virtual
		void
		accept_visitor(
				ConstFeatureVisitor &visitor) const {
			visitor.visit_gpml_constant_value(*this);
		}

		/**
		 * Accept a FeatureVisitor instance.
		 *
		 * See the Visitor pattern (p.331) in Gamma95 for information on the purpose of
		 * this function.
		 */
		virtual
		void
		accept_visitor(
				FeatureVisitor &visitor) {
			visitor.visit_gpml_constant_value(*this);
		}

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GpmlConstantValue(
				PropertyValue::non_null_ptr_type value_,
				const TemplateTypeParameterType &value_type_):
			PropertyValue(),
			d_value(value_),
			d_value_type(value_type_)
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GpmlConstantValue(
				const GpmlConstantValue &other) :
			PropertyValue(),
			d_value(other.d_value),
			d_value_type(other.d_value_type)
		{  }

	private:

		PropertyValue::non_null_ptr_type d_value;
		TemplateTypeParameterType d_value_type;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GpmlConstantValue &
		operator=(const GpmlConstantValue &);

	};

}

#endif  // GPLATES_MODEL_GPMLCONSTANTVALUE_H
