/* $Id: GpmlArray.h 10193 2010-11-11 16:12:10Z rwatson $ */

/**
 * @file 
 *
 * Most recent change:
 *   $Date: 2010-11-12 03:12:10 +1100 (Fri, 12 Nov 2010) $
 * 
 * Copyright (C) 2010 Geological Survey of Norway
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

#ifndef GPLATES_PROPERTYVALUES_GPMLARRAY_H
#define GPLATES_PROPERTYVALUES_GPMLARRAY_H

#include <vector>

#include "StructuralType.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "model/FeatureVisitor.h"
#include "model/ModelTransaction.h"
#include "model/PropertyValue.h"
#include "model/RevisionContext.h"
#include "model/RevisionedReference.h"
#include "model/RevisionedVector.h"

#include "utils/QtStreamable.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlArray, visit_gpml_array)

namespace GPlatesPropertyValues
{

	class GpmlArray:
			public GPlatesModel::PropertyValue,
			public GPlatesModel::RevisionContext
	{

	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GpmlArray>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlArray> non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const GpmlArray>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlArray> non_null_ptr_to_const_type;


		virtual
		~GpmlArray()
		{  }


		static
		const non_null_ptr_type
		create(
				const std::vector<GPlatesModel::PropertyValue::non_null_ptr_type> &members_,
				const StructuralType &value_type_)
		{
			return create(members_.begin(), members_.end(), value_type_);
		}

		template<typename PropertyValueIter>
		static
		const non_null_ptr_type
		create(
				PropertyValueIter members_begin,
				PropertyValueIter members_end,
				const StructuralType &value_type)
		{
			GPlatesModel::ModelTransaction transaction;
			non_null_ptr_type ptr(
					new GpmlArray(
							transaction,
							GPlatesModel::RevisionedVector<GPlatesModel::PropertyValue>::create(
									members_begin,
									members_end),
							value_type));
			transaction.commit();
			return ptr;
		}

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GpmlArray>(clone_impl());
		}

		/**
		 * Returns the 'const' vector of members.
		 */
		const GPlatesModel::RevisionedVector<GPlatesModel::PropertyValue> &
		members() const
		{
			return *get_current_revision<Revision>().members.get_revisionable();
		}

		/**
		 * Returns the 'non-const' vector of members.
		 */
		GPlatesModel::RevisionedVector<GPlatesModel::PropertyValue> &
		members()
		{
			return *get_current_revision<Revision>().members.get_revisionable();
		}

		// Note that no "setter" is provided:  The value type of a GpmlArray
		// instance should never be changed.
		StructuralType
		get_value_type() const
		{
			return d_value_type;
		}

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
		 * Static access to the structural type as GpmlArray::STRUCTURAL_TYPE.
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
			visitor.visit_gpml_array(*this);
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
			visitor.visit_gpml_array(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:


		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GpmlArray(
				GPlatesModel::ModelTransaction &transaction_,
				GPlatesModel::RevisionedVector<GPlatesModel::PropertyValue>::non_null_ptr_type members_,
				const StructuralType &value_type) :
			PropertyValue(Revision::non_null_ptr_type(new Revision(transaction_, *this, members_))),
			d_value_type(value_type)
		{  }

		//! Constructor used when cloning.
		GpmlArray(
				const GpmlArray &other_,
				boost::optional<RevisionContext &> context_) :
			PropertyValue(
					Revision::non_null_ptr_type(
							// Use deep-clone constructor...
							new Revision(other_.get_current_revision<Revision>(), context_, *this))),
			d_value_type(other_.d_value_type)
		{  }

		virtual
		const Revisionable::non_null_ptr_type
		clone_impl(
				boost::optional<RevisionContext &> context = boost::none) const
		{
			return non_null_ptr_type(new GpmlArray(*this, context));
		}

		virtual
		bool
		equality(
				const Revisionable &other) const
		{
			const GpmlArray &other_pv = dynamic_cast<const GpmlArray &>(other);

			return d_value_type == other_pv.d_value_type &&
					// The revisioned data comparisons are handled here...
					Revisionable::equality(other);
		}

	private:

		/**
		 * Used when modifications bubble up to us.
		 *
		 * Inherited from @a RevisionContext.
		 */
		virtual
		GPlatesModel::Revision::non_null_ptr_type
		bubble_up(
				GPlatesModel::ModelTransaction &transaction,
				const Revisionable::non_null_ptr_to_const_type &child_revisionable);

		/**
		 * Inherited from @a RevisionContext.
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
				public PropertyValue::Revision
		{
			Revision(
					GPlatesModel::ModelTransaction &transaction_,
					RevisionContext &child_context_,
					GPlatesModel::RevisionedVector<GPlatesModel::PropertyValue>::non_null_ptr_type members_) :
				members(
						GPlatesModel::RevisionedReference<
								GPlatesModel::RevisionedVector<GPlatesModel::PropertyValue> >::attach(
										transaction_, child_context_, members_))
			{  }

			//! Deep-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<RevisionContext &> context_,
					RevisionContext &child_context_) :
				PropertyValue::Revision(context_),
				members(other_.members)
			{
				// Clone data members that were not deep copied.
				members.clone(child_context_);
			}

			//! Shallow-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<RevisionContext &> context_) :
				PropertyValue::Revision(context_),
				members(other_.members)
			{  }

			virtual
			GPlatesModel::Revision::non_null_ptr_type
			clone_revision(
					boost::optional<RevisionContext &> context) const
			{
				// Use shallow-clone constructor.
				return non_null_ptr_type(new Revision(*this, context));
			}

			virtual
			bool
			equality(
					const GPlatesModel::Revision &other) const
			{
				const Revision &other_revision = dynamic_cast<const Revision &>(other);

				return *members.get_revisionable() == *other_revision.members.get_revisionable() &&
						PropertyValue::Revision::equality(other);
			}

			GPlatesModel::RevisionedReference<GPlatesModel::RevisionedVector<GPlatesModel::PropertyValue> > members;
		};

		StructuralType d_value_type;

	};
}

#endif // GPLATES_PROPERTYVALUES_GPMLARRAY_H
