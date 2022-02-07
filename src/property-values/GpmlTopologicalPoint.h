
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

#ifndef GPLATES_PROPERTYVALUES_GPMLTOPOLOGICALPOINT_H
#define GPLATES_PROPERTYVALUES_GPMLTOPOLOGICALPOINT_H

#include "GpmlPropertyDelegate.h"
#include "GpmlTopologicalSection.h"

#include "model/RevisionContext.h"
#include "model/RevisionedReference.h"


// Enable GPlatesFeatureVisitors::get_revisionable() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlTopologicalPoint, visit_gpml_topological_point)

namespace GPlatesPropertyValues
{

	class GpmlTopologicalPoint:
			public GpmlTopologicalSection,
			public GPlatesModel::RevisionContext
	{

	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GpmlTopologicalPoint>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlTopologicalPoint> non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const GpmlTopologicalPoint>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlTopologicalPoint> non_null_ptr_to_const_type;


		virtual
		~GpmlTopologicalPoint()
		{  }

		static
		const non_null_ptr_type
		create(
				GpmlPropertyDelegate::non_null_ptr_type source_geometry_);

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GpmlTopologicalPoint>(clone_impl());
		}

		/**
		 * Returns the 'const' property delegate.
		 */
		GpmlPropertyDelegate::non_null_ptr_to_const_type
		source_geometry() const
		{
			return get_current_revision<Revision>().source_geometry.get_revisionable();
		}

		/**
		 * Returns the 'non-const' property delegate.
		 */
		GpmlPropertyDelegate::non_null_ptr_type
		source_geometry()
		{
			return get_current_revision<Revision>().source_geometry.get_revisionable();
		}

		/**
		 * Sets the internal property delegate.
		 */
		void
		set_source_geometry(
				GpmlPropertyDelegate::non_null_ptr_type source_geometry);

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
		 * Static access to the structural type as GpmlTopologicalPoint::STRUCTURAL_TYPE.
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
			visitor.visit_gpml_topological_point(*this);
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
			visitor.visit_gpml_topological_point(*this);
		}

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GpmlTopologicalPoint(
				GPlatesModel::ModelTransaction &transaction_,
				GpmlPropertyDelegate::non_null_ptr_type source_geometry_) :
			GpmlTopologicalSection(
					Revision::non_null_ptr_type(
							new Revision(transaction_, *this, source_geometry_)))
		{  }

		//! Constructor used when cloning.
		GpmlTopologicalPoint(
				const GpmlTopologicalPoint &other_,
				boost::optional<RevisionContext &> context_) :
			GpmlTopologicalSection(
					Revision::non_null_ptr_type(
							// Use deep-clone constructor...
							new Revision(other_.get_current_revision<Revision>(), context_, *this)))
		{  }

		virtual
		const Revisionable::non_null_ptr_type
		clone_impl(
				boost::optional<RevisionContext &> context = boost::none) const
		{
			return non_null_ptr_type(new GpmlTopologicalPoint(*this, context));
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
			explicit
			Revision(
					GPlatesModel::ModelTransaction &transaction_,
					RevisionContext &child_context_,
					GpmlPropertyDelegate::non_null_ptr_type source_geometry_) :
				source_geometry(
						GPlatesModel::RevisionedReference<GpmlPropertyDelegate>::attach(
								transaction_, child_context_, source_geometry_))
			{  }

			//! Deep-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<RevisionContext &> context_,
					RevisionContext &child_context_) :
				PropertyValue::Revision(context_),
				source_geometry(other_.source_geometry)
			{
				// Clone data members that were not deep copied.
				source_geometry.clone(child_context_);
			}

			//! Shallow-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<RevisionContext &> context_) :
				PropertyValue::Revision(context_),
				source_geometry(other_.source_geometry)
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

				// Compare property delegate objects not pointers.
				return *source_geometry.get_revisionable() == *other_revision.source_geometry.get_revisionable() &&
						PropertyValue::Revision::equality(other);
			}

			GPlatesModel::RevisionedReference<GpmlPropertyDelegate> source_geometry;
		};

	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLTOPOLOGICALPOINT_H
