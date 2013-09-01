/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTYVALUES_UNINTERPRETEDPROPERTYVALUE_H
#define GPLATES_PROPERTYVALUES_UNINTERPRETEDPROPERTYVALUE_H

#include "feature-visitors/PropertyValueFinder.h"

#include "model/PropertyValue.h"
#include "model/XmlNode.h"

#include "utils/UnicodeStringUtils.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::UninterpretedPropertyValue, visit_uninterpreted_property_value)

namespace GPlatesPropertyValues
{
	/**
	 * This class implements an uninterpreted PropertyValue.
	 */
	class UninterpretedPropertyValue:
			public GPlatesModel::PropertyValue
	{

	public:

		//! A convenience typedef for a shared pointer to a non-const @a UninterpretedPropertyValue.
		typedef GPlatesUtils::non_null_intrusive_ptr<UninterpretedPropertyValue> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a UninterpretedPropertyValue.
		typedef GPlatesUtils::non_null_intrusive_ptr<const UninterpretedPropertyValue> non_null_ptr_to_const_type;


		virtual
		~UninterpretedPropertyValue()
		{  }

		static
		const non_null_ptr_type
		create(
				const GPlatesModel::XmlElementNode::non_null_ptr_to_const_type &value)
		{
			return non_null_ptr_type(new UninterpretedPropertyValue(value));
		}

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<UninterpretedPropertyValue>(clone_impl());
		}

		const GPlatesModel::XmlElementNode::non_null_ptr_to_const_type &
		get_value() const
		{
			return d_value;
		}

		/**
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gpml("UninterpretedPropertyValue");
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
			visitor.visit_uninterpreted_property_value(*this);
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
			visitor.visit_uninterpreted_property_value(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		UninterpretedPropertyValue(
				const GPlatesModel::XmlElementNode::non_null_ptr_to_const_type &value_) :
			PropertyValue(Revision::non_null_ptr_type(new Revision())),
			// TODO: Should probably deep copy so caller cannot modify our value later...
			d_value(value_)
		{  }

		//! Constructor used when cloning.
		UninterpretedPropertyValue(
				const UninterpretedPropertyValue &other_,
				boost::optional<GPlatesModel::RevisionContext &> context_) :
			PropertyValue(
					Revision::non_null_ptr_type(
							new Revision(other_.get_current_revision<Revision>(), context_))),
			d_value(other_.d_value)
		{  }

		virtual
		const Revisionable::non_null_ptr_type
		clone_impl(
				boost::optional<GPlatesModel::RevisionContext &> context = boost::none) const
		{
			return non_null_ptr_type(new UninterpretedPropertyValue(*this, context));
		}

		virtual
		bool
		equality(
				const Revisionable &other) const
		{
			const UninterpretedPropertyValue &other_pv = dynamic_cast<const UninterpretedPropertyValue &>(other);

			// TODO: Compare XML element nodes instead of pointers.
			return d_value == other_pv.d_value &&
				Revisionable::equality(other);
		}

	private:

		/**
		 * Property value data that is mutable/revisionable.
		 */
		struct Revision :
				public PropertyValue::Revision
		{
			Revision()
			{  }

			//! Clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<GPlatesModel::RevisionContext &> context_) :
				PropertyValue::Revision(context_)
			{  }

			virtual
			GPlatesModel::Revision::non_null_ptr_type
			clone_revision(
					boost::optional<GPlatesModel::RevisionContext &> context) const
			{
				return non_null_ptr_type(new Revision(*this, context));
			}
		};

		GPlatesModel::XmlElementNode::non_null_ptr_to_const_type d_value;

	};

}

#endif  // GPLATES_PROPERTYVALUES_UNINTERPRETEDPROPERTYVALUE_H
