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
			return GPlatesUtils::dynamic_pointer_cast<GpmlConstantValue>(clone_impl());
		}

		const GPlatesModel::PropertyValue::non_null_ptr_to_const_type
		get_value() const
		{
			return get_current_revision<Revision>().value;
		}

		// Note that, because the copy-assignment operator of PropertyValue is private,
		// the PropertyValue referenced by the return-value of this function cannot be
		// assigned-to, which means that this function does not provide a means to directly
		// switch the PropertyValue within this GpmlConstantValue instance.  (This restriction
		// is intentional.).
		// However the property value contained within can be modified, and this causes no
		// revisioning problems because all property values take care of their own revisioning.
		//
		// To switch the PropertyValue within this GpmlConstantValue instance, use the
		// function @a set_value below.
		//
		// (This overload is provided to allow the referenced PropertyValue instance to
		// accept a FeatureVisitor instance.)
		const GPlatesModel::PropertyValue::non_null_ptr_type
		get_value()
		{
			return get_current_revision<Revision>().value;
		}

		void
		set_value(
				GPlatesModel::PropertyValue::non_null_ptr_type v);

		// Note that no "setter" is provided:  The value type of a GpmlConstantValue
		// instance should never be changed.
		const StructuralType &
		get_value_type() const
		{
			return d_value_type;
		}

		const GPlatesUtils::UnicodeString &
		get_description() const
		{
			return get_current_revision<Revision>().description;
		}

		void
		set_description(
				const GPlatesUtils::UnicodeString &new_description);

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
			PropertyValue(Revision::non_null_ptr_type(new Revision(value_, ""))),
			d_value_type(value_type_)
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GpmlConstantValue(
				GPlatesModel::PropertyValue::non_null_ptr_type value_,
				const StructuralType &value_type_,
				const GPlatesUtils::UnicodeString &description_):
			PropertyValue(Revision::non_null_ptr_type(new Revision(value_, description_))),
			d_value_type(value_type_)
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GpmlConstantValue(
				const GpmlConstantValue &other) :
			PropertyValue(other),
			d_value_type(other.d_value_type)
		{  }

		virtual
		const GPlatesModel::PropertyValue::non_null_ptr_type
		clone_impl() const
		{
			return non_null_ptr_type(new GpmlConstantValue(*this));
		}

		virtual
		bool
		equality(
				const PropertyValue &other) const
		{
			const GpmlConstantValue &other_pv = static_cast<const GpmlConstantValue &>(other);

			return d_value_type == other_pv.d_value_type &&
					// The revisioned data comparisons are handled here...
					GPlatesModel::PropertyValue::equality(other);
		}

	private:

		/**
		 * Property value data that is mutable/revisionable.
		 */
		struct Revision :
				public GPlatesModel::PropertyValue::Revision
		{
			Revision(
					const GPlatesModel::PropertyValue::non_null_ptr_type value_,
					const GPlatesUtils::UnicodeString &description_) :
				value(value_),
				description(description_)
			{  }

			Revision(
					const Revision &other) :
				value(other.value->clone()),
				description(other.description)
			{  }

			virtual
			GPlatesModel::PropertyValue::Revision::non_null_ptr_type
			clone() const
			{
				return non_null_ptr_type(new Revision(*this));
			}

			virtual
			GPlatesModel::PropertyValue::Revision::non_null_ptr_type
			clone_for_bubble_up_modification() const
			{
				// Don't clone the property value - share it instead.
				return non_null_ptr_type(new Revision(value, description));
			}

			virtual
			bool
			equality(
					const GPlatesModel::PropertyValue::Revision &other) const
			{
				const Revision &other_revision = static_cast<const Revision &>(other);

				// Note that we compare the property value contents (and not pointers).
				return *value == *other_revision.value &&
						description == other_revision.description &&
						GPlatesModel::PropertyValue::Revision::equality(other);
			}

			GPlatesModel::PropertyValue::non_null_ptr_type value;
			GPlatesUtils::UnicodeString description;
		};


		// Immutable, so doesn't need revisioning.
		StructuralType d_value_type;


		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GpmlConstantValue &
		operator=(const GpmlConstantValue &);

	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLCONSTANTVALUE_H
