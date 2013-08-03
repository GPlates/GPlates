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

#include <vector>

#include "GpmlTopologicalSection.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "model/FeatureVisitor.h"
#include "model/PropertyValue.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlTopologicalLine, visit_gpml_topological_line)

namespace GPlatesPropertyValues
{	/**
	 * This class implements the PropertyValue which corresponds to "gpml:TopologicalLine".
	 */
	class GpmlTopologicalLine:
			public GPlatesModel::PropertyValue
	{
	public:

		//! A convenience typedef for a shared pointer to a non-const @a GpmlTopologicalLine.
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlTopologicalLine> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GpmlTopologicalLine.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlTopologicalLine> non_null_ptr_to_const_type;


		//! Typedef for a sequence of topological sections.
		typedef std::vector<GpmlTopologicalSection::non_null_ptr_to_const_type> sections_seq_type;


		virtual
		~GpmlTopologicalLine()
		{  }


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
			return non_null_ptr_type(new GpmlTopologicalLine(sections_begin_, sections_end_));
		}

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GpmlTopologicalLine>(clone_impl());
		}

		/**
		 * Returns the topological sections.
		 *
		 * The returned sections are each 'const' objects so that they cannot be modified and
		 * bypass the revisioning system.
		 */
		const sections_seq_type &
		get_sections() const
		{
			return get_current_revision<Revision>().sections;
		}

		/**
		 * Set the sequence of topological sections.
		 */
		template <typename TopologicalSectionsIterator>
		void
		set_sections(
				const TopologicalSectionsIterator &sections_begin_,
				const TopologicalSectionsIterator &sections_end_)
		{
			MutableRevisionHandler revision_handler(this);
			revision_handler.get_mutable_revision<Revision>().set_sections(sections_begin_, sections_end_);
			revision_handler.handle_revision_modification();
		}


		/**
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gpml("TopologicalLine");
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
		template <typename TopologicalSectionsIterator>
		GpmlTopologicalLine(
				const TopologicalSectionsIterator &sections_begin_,
				const TopologicalSectionsIterator &sections_end_) :
			PropertyValue(Revision::non_null_ptr_type(new Revision(sections_begin_, sections_end_)))
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GpmlTopologicalLine(
				const GpmlTopologicalLine &other) :
			PropertyValue(other)
		{  }

		virtual
		const GPlatesModel::PropertyValue::non_null_ptr_type
		clone_impl() const
		{
			return non_null_ptr_type(new GpmlTopologicalLine(*this));
		}

	private:

		/**
		 * Property value data that is mutable/revisionable.
		 */
		struct Revision :
				public GPlatesModel::PropertyValue::Revision
		{
			template <typename TopologicalSectionsIterator>
			Revision(
					const TopologicalSectionsIterator &sections_begin_,
					const TopologicalSectionsIterator &sections_end_)
			{
				set_sections(sections_begin_, sections_end_);
			}

			// This constructor used only by @a clone_for_bubble_up_modification - it does not clone.
			explicit
			Revision(
					const sections_seq_type &sections_) :
				sections(sections_)
			{  }

			Revision(
					const Revision &other);

			// To keep our revision state immutable we clone the sections so that the client
			// can no longer modify them indirectly...
			template <typename TopologicalSectionsIterator>
			void
			set_sections(
					const TopologicalSectionsIterator &sections_begin_,
					const TopologicalSectionsIterator &sections_end_) :
			{
				sections.clear();
				TopologicalSectionsIterator sections_iter_ = sections_begin_;
				for ( ; sections_iter_ != sections_end_; ++sections_iter_)
				{
					GpmlTopologicalSection::non_null_ptr_to_const_type section_ = *sections_iter_;
					sections.push_back(section_->clone());
				}
			}

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
				// Don't clone the sections - share them instead.
				return non_null_ptr_type(new Revision(sections));
			}

			virtual
			bool
			equality(
					const GPlatesModel::PropertyValue::Revision &other) const;

			sections_seq_type sections;
		};


		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GpmlTopologicalLine &
		operator=(
				const GpmlTopologicalLine &);

	};
}

#endif  // GPLATES_PROPERTYVALUES_GPMLTOPOLOGICALLINE_H
