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

#ifndef GPLATES_PROPERTYVALUES_GPMLREVISIONID_H
#define GPLATES_PROPERTYVALUES_GPMLREVISIONID_H

#include "feature-visitors/PropertyValueFinder.h"

#include "model/PropertyValue.h"
#include "model/RevisionId.h"

#include "utils/UnicodeStringUtils.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlRevisionId, visit_gpml_revision_id)

namespace GPlatesPropertyValues
{

	class GpmlRevisionId:
			public GPlatesModel::PropertyValue
	{

	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GpmlRevisionId>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlRevisionId> non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const GpmlRevisionId>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlRevisionId> non_null_ptr_to_const_type;


		virtual
		~GpmlRevisionId()
		{  }

		static
		const non_null_ptr_type
		create(
				const GPlatesModel::RevisionId &value_)
		{
			return non_null_ptr_type(new GpmlRevisionId(value_));
		}

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GpmlRevisionId>(clone_impl());
		}

		const GPlatesModel::RevisionId &
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
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gpml("revisionId");
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
			visitor.visit_gpml_revision_id(*this);
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
			visitor.visit_gpml_revision_id(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		GpmlRevisionId(
				const GPlatesModel::RevisionId &value_):
			PropertyValue(Revision::non_null_ptr_type(new Revision())),
			d_value(value_)
		{  }

		//! Constructor used when cloning.
		GpmlRevisionId(
				const GpmlRevisionId &other_,
				boost::optional<GPlatesModel::PropertyValueRevisionContext &> context_) :
			PropertyValue(
					Revision::non_null_ptr_type(
							new Revision(other_.get_current_revision<Revision>(), context_))),
			d_value(other_.d_value)
		{  }

		virtual
		const PropertyValue::non_null_ptr_type
		clone_impl(
				boost::optional<GPlatesModel::PropertyValueRevisionContext &> context = boost::none) const
		{
			return non_null_ptr_type(new GpmlRevisionId(*this, context));
		}

		virtual
		bool
		equality(
				const PropertyValue &other) const
		{
			const GpmlRevisionId &other_pv = dynamic_cast<const GpmlRevisionId &>(other);

			return d_value == other_pv.d_value &&
				PropertyValue::equality(other);
		}

	private:

		/**
		 * Property value data that is mutable/revisionable.
		 */
		struct Revision :
				public GPlatesModel::PropertyValueRevision
		{
			Revision()
			{  }

			//! Clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<GPlatesModel::PropertyValueRevisionContext &> context_) :
				PropertyValueRevision(context_)
			{  }

			virtual
			PropertyValueRevision::non_null_ptr_type
			clone_revision(
					boost::optional<GPlatesModel::PropertyValueRevisionContext &> context) const
			{
				return non_null_ptr_type(new Revision(*this, context));
			}
		};

		GPlatesModel::RevisionId d_value;

	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLREVISIONID_H
