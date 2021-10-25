/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2008, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTYVALUES_GPMLCONSTANTVALUE_H
#define GPLATES_PROPERTYVALUES_GPMLCONSTANTVALUE_H

#include "StructuralType.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "model/FeatureVisitor.h"
#include "model/PropertyValue.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlConstantValue, visit_gpml_constant_value)

namespace GPlatesPropertyValues
{

	class GpmlConstantValue :
			public GPlatesModel::PropertyValue
	{

	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GpmlConstantValue>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlConstantValue> non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const GpmlConstantValue>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlConstantValue> non_null_ptr_to_const_type;


		virtual
		~GpmlConstantValue()
		{  }

		static
		const non_null_ptr_type
		create(
				GPlatesModel::PropertyValue::non_null_ptr_type value_,
				const StructuralType &value_type_)
		{
			return non_null_ptr_type(new GpmlConstantValue(value_, value_type_));
		}

		static
		const non_null_ptr_type
		create(
				GPlatesModel::PropertyValue::non_null_ptr_type value_,
				const StructuralType &value_type_,
				const GPlatesUtils::UnicodeString &description_)
		{
			return non_null_ptr_type(new GpmlConstantValue(value_, value_type_, description_));
		}

		const non_null_ptr_type
		clone() const
		{
			return non_null_ptr_type(new GpmlConstantValue(*this));
		}

		const non_null_ptr_type
		deep_clone() const;

		DEFINE_FUNCTION_DEEP_CLONE_AS_PROP_VAL()

		const GPlatesModel::PropertyValue::non_null_ptr_to_const_type
		value() const
		{
			return d_value;
		}

		/**
		 * Returns the 'non-const' property value.
		 */
		const GPlatesModel::PropertyValue::non_null_ptr_type
		value()
		{
			return d_value;
		}

		/**
		 * Sets the internal property value.
		 */
		void
		set_value(
				GPlatesModel::PropertyValue::non_null_ptr_type v)
		{
			d_value = v;
		}

		// Note that no "setter" is provided:  The value type of a GpmlConstantValue
		// instance should never be changed.
		const StructuralType &
		value_type() const
		{
			return d_value_type;
		}

		const GPlatesUtils::UnicodeString &
		description() const
		{
			return d_description;
		}

		void
		set_description(
				const GPlatesUtils::UnicodeString &new_description)
		{
			d_description = new_description;
			update_instance_id();
		}

		/**
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gpml("ConstantValue");
			return STRUCTURAL_TYPE;
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
				GPlatesModel::ConstFeatureVisitor &visitor) const
		{
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
				GPlatesModel::FeatureVisitor &visitor)
		{
			visitor.visit_gpml_constant_value(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GpmlConstantValue(
				GPlatesModel::PropertyValue::non_null_ptr_type value_,
				const StructuralType &value_type_):
			PropertyValue(),
			d_value(value_),
			d_value_type(value_type_),
			d_description("")
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GpmlConstantValue(
				GPlatesModel::PropertyValue::non_null_ptr_type value_,
				const StructuralType &value_type_,
				const GPlatesUtils::UnicodeString &description_):
			PropertyValue(),
			d_value(value_),
			d_value_type(value_type_),
			d_description(description_)
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GpmlConstantValue(
				const GpmlConstantValue &other) :
			PropertyValue(other), /* share instance id */
			d_value(other.d_value),
			d_value_type(other.d_value_type),
			d_description(other.d_description)
		{  }

		virtual
		bool
		directly_modifiable_fields_equal(
				const PropertyValue &other) const;

	private:

		GPlatesModel::PropertyValue::non_null_ptr_type d_value;
		StructuralType d_value_type;
		GPlatesUtils::UnicodeString d_description;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GpmlConstantValue &
		operator=(const GpmlConstantValue &);

	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLCONSTANTVALUE_H
