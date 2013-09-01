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

#ifndef GPLATES_PROPERTYVALUES_GPMLTOPOLOGICALPOLYGON_H
#define GPLATES_PROPERTYVALUES_GPMLTOPOLOGICALPOLYGON_H

#include <iosfwd>
#include <vector>
#include <boost/intrusive_ptr.hpp>

#include "GpmlTopologicalSection.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "model/FeatureVisitor.h"
#include "model/ModelTransaction.h"
#include "model/PropertyValue.h"
#include "model/RevisionContext.h"
#include "model/RevisionedReference.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlTopologicalPolygon, visit_gpml_topological_polygon)

namespace GPlatesPropertyValues
{
	/**
	 * This class implements the PropertyValue which corresponds to "gpml:TopologicalPolygon".
	 */
	class GpmlTopologicalPolygon:
			public GPlatesModel::PropertyValue,
			public GPlatesModel::RevisionContext
	{

	public:

		//! A convenience typedef for a shared pointer to a non-const @a GpmlTopologicalPolygon.
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlTopologicalPolygon> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GpmlTopologicalPolygon.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlTopologicalPolygon> non_null_ptr_to_const_type;


		/**
		 * Topological reference to a section of the topological polygon.
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

		//! Typedef for a sequence of sections.
		typedef std::vector<Section> sections_seq_type;


		virtual
		~GpmlTopologicalPolygon()
		{  }


		/**
		 * Create a @a GpmlTopologicalPolygon instance from the specified sequence of
		 * topological sections representing the exterior of the topological polygon.
		 *
		 * TODO: Add support for topological interiors where each interior is a reference
		 * to a topological polygon exterior and represents an interior hole region.
		 */
		template <typename TopologicalSectionsIterator>
		static
		const non_null_ptr_type
		create(
				const TopologicalSectionsIterator &exterior_sections_begin_,
				const TopologicalSectionsIterator &exterior_sections_end_)
		{
			GPlatesModel::ModelTransaction transaction;
			non_null_ptr_type ptr(
					new GpmlTopologicalPolygon(
							transaction, exterior_sections_begin_, exterior_sections_end_));
			transaction.commit();
			return ptr;
		}

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GpmlTopologicalPolygon>(clone_impl());
		}

		/**
		 * Returns the exterior topological sections.
		 *
		 * To modify any exterior topological sections:
		 * (1) make additions/removals/modifications to a copy of the returned vector, and
		 * (2) use @a get_exterior_sections to set them.
		 *
		 * The returned exterior topological sections implement copy-on-write to promote resource sharing (until write)
		 * and to ensure our internal state cannot be modified and bypass the revisioning system.
		 */
		const sections_seq_type &
		get_exterior_sections() const
		{
			return get_current_revision<Revision>().exterior_sections;
		}

		/**
		 * Set the sequence of exterior topological sections.
		 */
		void
		set_exterior_sections(
				const sections_seq_type &exterior_sections);

		/**
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gpml("TopologicalPolygon");
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
			visitor.visit_gpml_topological_polygon(*this);
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
			visitor.visit_gpml_topological_polygon(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		template <typename TopologicalSectionsIterator>
		GpmlTopologicalPolygon(
				GPlatesModel::ModelTransaction &transaction_,
				const TopologicalSectionsIterator &exterior_sections_begin_,
				const TopologicalSectionsIterator &exterior_sections_end_) :
			PropertyValue(
					Revision::non_null_ptr_type(
							new Revision(transaction_, *this, exterior_sections_begin_, exterior_sections_end_)))
		{  }

		//! Constructor used when cloning.
		GpmlTopologicalPolygon(
				const GpmlTopologicalPolygon &other_,
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
			return non_null_ptr_type(new GpmlTopologicalPolygon(*this, context));
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
			template <typename TopologicalSectionsIterator>
			Revision(
					GPlatesModel::ModelTransaction &transaction_,
					RevisionContext &child_context_,
					const TopologicalSectionsIterator &exterior_sections_begin_,
					const TopologicalSectionsIterator &exterior_sections_end_) :
				exterior_sections(exterior_sections_begin_, exterior_sections_end_)
			{  }

			//! Deep-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<RevisionContext &> context_,
					RevisionContext &child_context_) :
				PropertyValue::Revision(context_),
				exterior_sections(other_.exterior_sections)
			{
				// Clone data members that were not deep copied.
			}

			//! Shallow-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<RevisionContext &> context_) :
				PropertyValue::Revision(context_),
				exterior_sections(other_.exterior_sections)
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
					const GPlatesModel::Revision &other) const;

			sections_seq_type exterior_sections;
		};

	};


	// operator<< for GpmlTopologicalPolygon::Section.
	std::ostream &
	operator<<(
			std::ostream &os,
			const GpmlTopologicalPolygon::Section &topological_polygon_section);
}

#endif  // GPLATES_PROPERTYVALUES_GPMLTOPOLOGICALPOLYGON_H
