/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTYVALUES_GMLGRIDENVELOPE_H
#define GPLATES_PROPERTYVALUES_GMLGRIDENVELOPE_H

#include <vector>

#include "feature-visitors/PropertyValueFinder.h"

#include "model/PropertyValue.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GmlGridEnvelope, visit_gml_grid_envelope)

namespace GPlatesPropertyValues
{
	/**
	 * This class implements the PropertyValue which corresponds to "gml:GridEnvelope".
	 */
	class GmlGridEnvelope:
			public GPlatesModel::PropertyValue
	{
	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GmlGridEnvelope>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GmlGridEnvelope> non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const GmlGridEnvelope>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GmlGridEnvelope> non_null_ptr_to_const_type;


		typedef std::vector<int> integer_list_type;


		virtual
		~GmlGridEnvelope()
		{  }

		/**
		 * Create a GmlGridEnvelope instance from @a low_ and @a high_ positions.
		 *
		 * The number of dimensions in @a low_ and @a high_ must be the same.
		 */
		static
		const non_null_ptr_type
		create(
				const integer_list_type &low_,
				const integer_list_type &high_);

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GmlGridEnvelope>(clone_impl());
		}

		const integer_list_type &
		get_low() const
		{
			return get_current_revision<Revision>().low;
		}

		const integer_list_type &
		get_high() const
		{
			return get_current_revision<Revision>().high;
		}

		void
		set_low_and_high(
				const integer_list_type &low_,
				const integer_list_type &high_);

		/**
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gml("GridEnvelope");
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
			visitor.visit_gml_grid_envelope(*this);
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
			visitor.visit_gml_grid_envelope(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		GmlGridEnvelope(
				const integer_list_type &low_,
				const integer_list_type &high_) :
			PropertyValue(Revision::non_null_ptr_type(new Revision(low_, high_)))
		{  }

		//! Constructor used when cloning.
		GmlGridEnvelope(
				const GmlGridEnvelope &other_,
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
			return non_null_ptr_type(new GmlGridEnvelope(*this, context));
		}

	private:

		/**
		 * Property value data that is mutable/revisionable.
		 */
		struct Revision :
				public PropertyValue::Revision
		{
			Revision(
					const integer_list_type &low_,
					const integer_list_type &high_) :
				low(low_),
				high(high_)
			{  }

			//! Clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<GPlatesModel::RevisionContext &> context_) :
				PropertyValue::Revision(context_),
				low(other_.low),
				high(other_.high)
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

				return low == other_revision.low &&
						high == other_revision.high &&
						PropertyValue::Revision::equality(other);
			}

			integer_list_type low;
			integer_list_type high;
		};

	};

}

#endif  // GPLATES_PROPERTYVALUES_GMLGRIDENVELOPE_H
