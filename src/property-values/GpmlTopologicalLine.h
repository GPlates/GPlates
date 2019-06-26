/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2008-07-11 19:36:59 -0700 (Fri, 11 Jul 2008) $
 * 
 * Copyright (C) 2008, 2009, 2010 California Institute of Technology
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

#ifndef GPLATES_PROPERTYVALUES_GPMLTOPOLOGICALLINE_H
#define GPLATES_PROPERTYVALUES_GPMLTOPOLOGICALLINE_H

#include <iosfwd>
#include <vector>

#include "GpmlTopologicalSection.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "model/FeatureVisitor.h"
#include "model/ModelTransaction.h"
#include "model/PropertyValue.h"
#include "model/RevisionContext.h"
#include "model/RevisionedReference.h"
#include "model/RevisionedVector.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlTopologicalLine, visit_gpml_topological_line)

namespace GPlatesPropertyValues
{	/**
	 * This class implements the PropertyValue which corresponds to "gpml:TopologicalLine".
	 */
	class GpmlTopologicalLine:
			public GPlatesModel::PropertyValue,
			public GPlatesModel::RevisionContext
	{
	public:

		//! A convenience typedef for a shared pointer to a non-const @a GpmlTopologicalLine.
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlTopologicalLine> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GpmlTopologicalLine.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlTopologicalLine> non_null_ptr_to_const_type;


		virtual
		~GpmlTopologicalLine()
		{  }


		/**
		 * Create a @a GpmlTopologicalLine instance from the specified sequence of topological sections.
		 */
		static
		const non_null_ptr_type
		create(
				const std::vector<GpmlTopologicalSection::non_null_ptr_type> &topological_sections_)
		{
			return create(topological_sections_.begin(), topological_sections_.end());
		}

		/**
		 * Create a @a GpmlTopologicalLine instance from the specified sequence of topological sections.
		 */
		template <typename TopologicalSectionsIterator>
		static
		const non_null_ptr_type
		create(
				const TopologicalSectionsIterator &sections_begin_,
				const TopologicalSectionsIterator &sections_end_)
		{
			GPlatesModel::ModelTransaction transaction;
			non_null_ptr_type ptr(
					new GpmlTopologicalLine(
							transaction,
							GPlatesModel::RevisionedVector<GpmlTopologicalSection>::create(
									sections_begin_,
									sections_end_)));
			transaction.commit();
			return ptr;
		}

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GpmlTopologicalLine>(clone_impl());
		}

		/**
		 * Returns the 'const' vector of members.
		 */
		const GPlatesModel::RevisionedVector<GpmlTopologicalSection> &
		sections() const
		{
			return *get_current_revision<Revision>().sections.get_revisionable();
		}

		/**
		 * Returns the 'non-const' vector of members.
		 */
		GPlatesModel::RevisionedVector<GpmlTopologicalSection> &
		sections()
		{
			return *get_current_revision<Revision>().sections.get_revisionable();
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
		 * Static access to the structural type as GpmlTopologicalLine::STRUCTURAL_TYPE.
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
			visitor.visit_gpml_topological_line(*this);
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
			visitor.visit_gpml_topological_line(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GpmlTopologicalLine(
				GPlatesModel::ModelTransaction &transaction_,
				GPlatesModel::RevisionedVector<GpmlTopologicalSection>::non_null_ptr_type sections_):
			PropertyValue(Revision::non_null_ptr_type(new Revision(transaction_, *this, sections_)))
		{  }

		//! Constructor used when cloning.
		GpmlTopologicalLine(
				const GpmlTopologicalLine &other_,
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
			return non_null_ptr_type(new GpmlTopologicalLine(*this, context));
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
					GPlatesModel::RevisionedVector<GpmlTopologicalSection>::non_null_ptr_type sections_) :
				sections(
						GPlatesModel::RevisionedReference<
								GPlatesModel::RevisionedVector<GpmlTopologicalSection> >::attach(
										transaction_, child_context_, sections_))
			{  }

			//! Deep-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<RevisionContext &> context_,
					RevisionContext &child_context_) :
				PropertyValue::Revision(context_),
				sections(other_.sections)
			{
				// Clone data members that were not deep copied.
				sections.clone(child_context_);
			}

			//! Shallow-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<RevisionContext &> context_) :
				PropertyValue::Revision(context_),
				sections(other_.sections)
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

				return *sections.get_revisionable() == *other_revision.sections.get_revisionable() &&
						PropertyValue::Revision::equality(other);
			}

			GPlatesModel::RevisionedReference<GPlatesModel::RevisionedVector<GpmlTopologicalSection> > sections;
		};

	};
}

#endif  // GPLATES_PROPERTYVALUES_GPMLTOPOLOGICALLINE_H
