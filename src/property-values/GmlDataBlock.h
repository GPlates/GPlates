/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTYVALUES_GMLDATABLOCK_H
#define GPLATES_PROPERTYVALUES_GMLDATABLOCK_H

#include <vector>

#include "GmlDataBlockCoordinateList.h"
#include "StructuralType.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "model/ModelTransaction.h"
#include "model/PropertyValue.h"
#include "model/RevisionContext.h"
#include "model/RevisionedReference.h"
#include "model/RevisionedVector.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GmlDataBlock, visit_gml_data_block)

namespace GPlatesPropertyValues
{
	/**
	 * This class implements the PropertyValue which corresponds to "gml:DataBlock".
	 */
	class GmlDataBlock :
			public GPlatesModel::PropertyValue,
			public GPlatesModel::RevisionContext
	{
	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GmlDataBlock>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GmlDataBlock> non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const GmlDataBlock>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GmlDataBlock> non_null_ptr_to_const_type;


		virtual
		~GmlDataBlock()
		{  }


		static
		const non_null_ptr_type
		create(
				const std::vector<GmlDataBlockCoordinateList::non_null_ptr_type> &tuple_list_)
		{
			return create(tuple_list_.begin(), tuple_list_.end());
		}

		template <typename GmlDataBlockCoordinateListIter>
		static
		const non_null_ptr_type
		create(
				GmlDataBlockCoordinateListIter tuple_list_begin,
				GmlDataBlockCoordinateListIter tuple_list_end)
		{
			GPlatesModel::ModelTransaction transaction;
			non_null_ptr_type ptr(
					new GmlDataBlock(
							transaction,
							GPlatesModel::RevisionedVector<GmlDataBlockCoordinateList>::create(
									tuple_list_begin,
									tuple_list_end)));
			transaction.commit();
			return ptr;
		}

		static
		const non_null_ptr_type
		create(
				const GmlDataBlockCoordinateList::non_null_ptr_type &list_)
		{
			std::vector<GmlDataBlockCoordinateList::non_null_ptr_type> tuple_list_(1, list_);
			return create(tuple_list_.begin(), tuple_list_.end());
		}

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GmlDataBlock>(clone_impl());
		}

		/**
		 * Returns the 'const' vector of members.
		 */
		const GPlatesModel::RevisionedVector<GmlDataBlockCoordinateList> &
		tuple_list() const
		{
			return *get_current_revision<Revision>().tuple_list.get_revisionable();
		}

		/**
		 * Returns the 'non-const' vector of members.
		 */
		GPlatesModel::RevisionedVector<GmlDataBlockCoordinateList> &
		tuple_list()
		{
			return *get_current_revision<Revision>().tuple_list.get_revisionable();
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
		 * Static access to the structural type as GmlDataBlock::STRUCTURAL_TYPE.
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
			visitor.visit_gml_data_block(*this);
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
			visitor.visit_gml_data_block(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:


		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GmlDataBlock(
				GPlatesModel::ModelTransaction &transaction_,
				GPlatesModel::RevisionedVector<GmlDataBlockCoordinateList>::non_null_ptr_type tuple_list_) :
			PropertyValue(Revision::non_null_ptr_type(new Revision(transaction_, *this, tuple_list_)))
		{  }

		//! Constructor used when cloning.
		GmlDataBlock(
				const GmlDataBlock &other_,
				boost::optional<RevisionContext &> context_) :
			PropertyValue(
					Revision::non_null_ptr_type(
							// Use deep-clone constructor...
							new Revision(other_.get_current_revision<Revision>(), context_, *this)))
		{  }

		virtual
		const Revisionable::non_null_ptr_type
		clone_impl(
				boost::optional<RevisionContext &> context = boost::none) const
		{
			return non_null_ptr_type(new GmlDataBlock(*this, context));
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
					GPlatesModel::RevisionedVector<GmlDataBlockCoordinateList>::non_null_ptr_type tuple_list_) :
				tuple_list(
						GPlatesModel::RevisionedReference<
								GPlatesModel::RevisionedVector<GmlDataBlockCoordinateList> >::attach(
										transaction_, child_context_, tuple_list_))
			{  }

			//! Deep-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<RevisionContext &> context_,
					RevisionContext &child_context_) :
				PropertyValue::Revision(context_),
				tuple_list(other_.tuple_list)
			{
				// Clone data members that were not deep copied.
				tuple_list.clone(child_context_);
			}

			//! Shallow-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<RevisionContext &> context_) :
				PropertyValue::Revision(context_),
				tuple_list(other_.tuple_list)
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

				return *tuple_list.get_revisionable() == *other_revision.tuple_list.get_revisionable() &&
						PropertyValue::Revision::equality(other);
			}

			GPlatesModel::RevisionedReference<GPlatesModel::RevisionedVector<GmlDataBlockCoordinateList> > tuple_list;
		};

	};


}

#endif  // GPLATES_PROPERTYVALUES_GMLDATABLOCK_H
