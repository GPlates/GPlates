/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTYVALUES_XSINTEGER_H
#define GPLATES_PROPERTYVALUES_XSINTEGER_H

#include "feature-visitors/PropertyValueFinder.h"
#include "model/PropertyValue.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::XsInteger, visit_xs_integer)

namespace GPlatesPropertyValues
{

	class XsInteger :
			public GPlatesModel::PropertyValue
	{

	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<XsIntger>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<XsInteger> non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const XsInteger>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const XsInteger> non_null_ptr_to_const_type;


		virtual
		~XsInteger()
		{  }

		static
		const non_null_ptr_type
		create(
				int value)
		{
			return non_null_ptr_type(new XsInteger(value));
		}

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<XsInteger>(clone_impl());
		}

		/**
		 * Accesses the int contained within this XsInteger.
		 */
		int
		get_value() const
		{
			return get_current_revision<Revision>().value;
		}

		/**
		 * Set the int value contained within this XsInteger to @a i.
		 */
		void
		set_value(
				int i);


		/**
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			return STRUCTURAL_TYPE;
		}

		/**
		 * Static access to the structural type as XsInteger::STRUCTURAL_TYPE.
		 */
		static const StructuralType STRUCTURAL_TYPE;


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
			visitor.visit_xs_integer(*this);
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
			visitor.visit_xs_integer(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		XsInteger(
				int value_) :
			PropertyValue(Revision::non_null_ptr_type(new Revision(value_)))
		{  }

		//! Constructor used when cloning.
		XsInteger(
				const XsInteger &other_,
				boost::optional<GPlatesModel::RevisionContext &> context_) :
			PropertyValue(
					Revision::non_null_ptr_type(
							new Revision(other_.get_current_revision<Revision>(), context_)))
		{  }

		virtual
		const Revisionable::non_null_ptr_type
		clone_impl(
				boost::optional<GPlatesModel::RevisionContext &> context = boost::none) const
		{
			return non_null_ptr_type(new XsInteger(*this, context));
		}

	private:

		/**
		 * Property value data that is mutable/revisionable.
		 */
		struct Revision :
				public PropertyValue::Revision
		{
			explicit
			Revision(
					int value_) :
				value(value_)
			{  }

			//! Clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<GPlatesModel::RevisionContext &> context_) :
				PropertyValue::Revision(context_),
				value(other_.value)
			{  }

			virtual
			GPlatesModel::Revision::non_null_ptr_type
			clone_revision(
					boost::optional<GPlatesModel::RevisionContext &> context) const
			{
				return non_null_ptr_type(new Revision(*this, context));
			}

			virtual
			bool
			equality(
					const GPlatesModel::Revision &other) const
			{
				const Revision &other_revision = dynamic_cast<const Revision &>(other);

				return value == other_revision.value &&
						PropertyValue::Revision::equality(other);
			}

			int value;
		};

	};

}

#endif  // GPLATES_PROPERTYVALUES_XSINTEGER_H
