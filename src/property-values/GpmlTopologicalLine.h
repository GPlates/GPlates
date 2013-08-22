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
#include "model/PropertyValueRevisionContext.h"
#include "model/PropertyValueRevisionedReference.h"


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
			public GPlatesModel::PropertyValueRevisionContext
	{
	public:

		//! A convenience typedef for a shared pointer to a non-const @a GpmlTopologicalLine.
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlTopologicalLine> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GpmlTopologicalLine.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlTopologicalLine> non_null_ptr_to_const_type;


		/**
		 * Topological reference to a section of the topological line.
		 */
		class Section :
				// Gives us "operator<<" for qDebug(), etc and QTextStream, if we provide for std::ostream...
				public GPlatesUtils::QtStreamable<Section>,
				public boost::equality_comparable<Section>
		{
		public:

			/**
			 * Section has value semantics where each @a Section instance has its own state.
			 * So if you create a copy and modify the copy's state then it will not modify the state
			 * of the original object.
			 *
			 * The constructor first clones the property delegate and then copy-on-write is used to allow
			 * multiple @a Section objects to share the same state (until the state is modified).
			 */
			Section(
					GpmlTopologicalSection::non_null_ptr_type source_section) :
				d_source_section(source_section)
			{  }

			/**
			 * Returns the 'const' source section.
			 */
			GpmlTopologicalSection::non_null_ptr_to_const_type
			get_source_section() const
			{
				return d_source_section.get();
			}

			/**
			 * Returns the 'non-const' source section.
			 */
			GpmlTopologicalSection::non_null_ptr_type
			get_source_section()
			{
				return d_source_section.get();
			}

			/**
			 * Value equality comparison operator.
			 *
			 * Inequality provided by boost equality_comparable.
			 */
			bool
			operator==(
					const Section &other) const
			{
				return *d_source_section == *other.d_source_section;
			}

		private:

			GpmlTopologicalSection::non_null_ptr_type d_source_section;
		};

		//! Typedef for a sequence of topological sections.
		typedef std::vector<Section> sections_seq_type;


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
			GPlatesModel::ModelTransaction transaction;
			non_null_ptr_type ptr(new GpmlTopologicalLine(transaction, sections_begin_, sections_end_));
			transaction.commit();
			return ptr;
		}

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GpmlTopologicalLine>(clone_impl());
		}

		/**
		 * Returns the topological sections.
		 *
		 * To modify any topological sections:
		 * (1) make additions/removals/modifications to a copy of the returned vector, and
		 * (2) use @a set_sections to set them.
		 *
		 * The returned topological sections implement copy-on-write to promote resource sharing (until write)
		 * and to ensure our internal state cannot be modified and bypass the revisioning system.
		 */
		const sections_seq_type &
		get_sections() const
		{
			return get_current_revision<Revision>().sections;
		}

		/**
		 * Set the sequence of topological sections.
		 */
		void
		set_sections(
				const sections_seq_type &sections);


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
				GPlatesModel::ModelTransaction &transaction_,
				const TopologicalSectionsIterator &sections_begin_,
				const TopologicalSectionsIterator &sections_end_) :
			PropertyValue(
					Revision::non_null_ptr_type(
							new Revision(transaction_, *this, sections_begin_, sections_end_)))
		{  }

		//! Constructor used when cloning.
		GpmlTopologicalLine(
				const GpmlTopologicalLine &other_,
				boost::optional<PropertyValueRevisionContext &> context_) :
			PropertyValue(
					Revision::non_null_ptr_type(
							// Use deep-clone constructor...
							new Revision(other_.get_current_revision<Revision>(), context_, *this)))
		{  }

		virtual
		const PropertyValue::non_null_ptr_type
		clone_impl(
				boost::optional<PropertyValueRevisionContext &> context = boost::none) const
		{
			return non_null_ptr_type(new GpmlTopologicalLine(*this, context));
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
				const PropertyValue::non_null_ptr_to_const_type &child_property_value);

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
			template <typename TopologicalSectionsIterator>
			Revision(
					GPlatesModel::ModelTransaction &transaction_,
					PropertyValueRevisionContext &child_context_,
					const TopologicalSectionsIterator &sections_begin_,
					const TopologicalSectionsIterator &sections_end_) :
				sections(sections_begin_, sections_end_)
			{  }

			//! Deep-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<PropertyValueRevisionContext &> context_,
					PropertyValueRevisionContext &child_context_) :
				PropertyValueRevision(context_),
				sections(other_.sections)
			{
				// Clone data members that were not deep copied.
			}

			//! Shallow-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<PropertyValueRevisionContext &> context_) :
				PropertyValueRevision(context_),
				sections(other_.sections)
			{  }

			virtual
			PropertyValueRevision::non_null_ptr_type
			clone_revision(
					boost::optional<PropertyValueRevisionContext &> context) const
			{
				// Use shallow-clone constructor.
				return non_null_ptr_type(new Revision(*this, context));
			}

			virtual
			bool
			equality(
					const PropertyValueRevision &other) const;

			sections_seq_type sections;
		};

	};


	// operator<< for GpmlTopologicalLine::Section.
	std::ostream &
	operator<<(
			std::ostream &os,
			const GpmlTopologicalLine::Section &topological_line_section);
}

#endif  // GPLATES_PROPERTYVALUES_GPMLTOPOLOGICALLINE_H
