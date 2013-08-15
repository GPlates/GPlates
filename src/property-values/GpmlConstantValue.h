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
#include "model/PropertyValueRevisionContext.h"
#include "model/PropertyValueRevisionedReference.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlConstantValue, visit_gpml_constant_value)

namespace GPlatesPropertyValues
{

	class GpmlConstantValue :
			public GPlatesModel::PropertyValue,
			public GPlatesModel::PropertyValueRevisionContext
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
				const StructuralType &value_type_,
				const GPlatesUtils::UnicodeString &description_ = "");

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GpmlConstantValue>(clone_impl());
		}

		/**
		 * Returns the 'const' property value.
		 */
		const GPlatesModel::PropertyValue::non_null_ptr_to_const_type
		value() const
		{
			return get_current_revision<Revision>().value.get_property_value();
		}

		/**
		 * Returns the 'non-const' property value.
		 */
		const GPlatesModel::PropertyValue::non_null_ptr_type
		value()
		{
			return get_current_revision<Revision>().value.get_property_value();
		}

		/**
		 * Sets the internal property value to a clone of @a v.
		 */
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
				GPlatesModel::ModelTransaction &transaction_,
				GPlatesModel::PropertyValue::non_null_ptr_type value_,
				const StructuralType &value_type_,
				const GPlatesUtils::UnicodeString &description_) :
			PropertyValue(
					Revision::non_null_ptr_type(
							new Revision(transaction_, *this, value_, description_))),
			d_value_type(value_type_)
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
			const GpmlConstantValue &other_pv = dynamic_cast<const GpmlConstantValue &>(other);

			return d_value_type == other_pv.d_value_type &&
					// The revisioned data comparisons are handled here...
					GPlatesModel::PropertyValue::equality(other);
		}

	private:

		/**
		 * Used when modifications bubble up to us.
		 *
		 * Inherited from @a PropertyValueRevisionContext.
		 */
		virtual
		GPlatesModel::PropertyValueRevision::non_null_ptr_type
		bubble_up(
				GPlatesModel::ModelTransaction &transaction,
				const GPlatesModel::PropertyValue::non_null_ptr_to_const_type &child_property_value);

		/**
		 * Inherited from @a PropertyValueRevisionContext.
		 */
		virtual
		boost::optional<GPlatesModel::Model &>
		get_model()
		{
			return PropertyValue::get_model();
		}


		/**
		 * Property value data that is mutable/revisionable.
		 */
		struct Revision :
				public GPlatesModel::PropertyValueRevision
		{
			typedef GPlatesUtils::non_null_intrusive_ptr<Revision> non_null_ptr_type;
			typedef GPlatesUtils::non_null_intrusive_ptr<const Revision> non_null_ptr_to_const_type;

			Revision(
					GPlatesModel::ModelTransaction &transaction_,
					GPlatesModel::PropertyValueRevisionContext &revision_context_,
					const GPlatesModel::PropertyValue::non_null_ptr_type value_,
					const GPlatesUtils::UnicodeString &description_) :
				value(
						GPlatesModel::PropertyValueRevisionedReference<GPlatesModel::PropertyValue>::attach(
								transaction_, revision_context_, value_)),
				description(description_)
			{  }

			virtual
			GPlatesModel::PropertyValueRevision::non_null_ptr_type
			clone_impl() const
			{
				// Copy constructor followed by cloning data members that were not deep copied.
				non_null_ptr_type dup(new Revision(*this));
				dup->value.clone();
				return dup;
			}

			virtual
			GPlatesModel::PropertyValueRevision::non_null_ptr_type
			clone_revision_impl() const
			{
				// Copy constructor is sufficient - deep copies all but revision references.
				return non_null_ptr_type(new Revision(*this));
			}

			virtual
			bool
			equality(
					const GPlatesModel::PropertyValueRevision &other) const
			{
				const Revision &other_revision = dynamic_cast<const Revision &>(other);

				// Note that we compare the property value contents (and not pointers).
				return *value.get_property_value() == *other_revision.value.get_property_value() &&
						description == other_revision.description &&
						GPlatesModel::PropertyValueRevision::equality(other);
			}

			GPlatesModel::PropertyValueRevisionedReference<GPlatesModel::PropertyValue> value;
			GPlatesUtils::UnicodeString description;
		};


		// Immutable, so doesn't need revisioning.
		StructuralType d_value_type;

	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLCONSTANTVALUE_H
