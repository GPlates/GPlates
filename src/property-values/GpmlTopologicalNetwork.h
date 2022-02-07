/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTY_VALUES_GPMLTOPOLOGICALNETWORK_H
#define GPLATES_PROPERTY_VALUES_GPMLTOPOLOGICALNETWORK_H

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

#include "property-values/GpmlPropertyDelegate.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlTopologicalNetwork, visit_gpml_topological_network)

namespace GPlatesPropertyValues
{
	/**
	 * This class implements the PropertyValue which corresponds to "gpml:TopologicalNetwork".
	 */
	class GpmlTopologicalNetwork :
			public GPlatesModel::PropertyValue,
			public GPlatesModel::RevisionContext
	{

	public:

		//! A convenience typedef for a shared pointer to a non-const @a GpmlTopologicalNetwork.
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlTopologicalNetwork> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GpmlTopologicalNetwork.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlTopologicalNetwork> non_null_ptr_to_const_type;


		virtual
		~GpmlTopologicalNetwork()
		{  }


		/**
		 * Create a @a GpmlTopologicalNetwork instance which contains a boundary only (no interior geometries).
		 */
		static
		const non_null_ptr_type
		create(
				const std::vector<GpmlTopologicalSection::non_null_ptr_type> &boundary_sections_)
		{
			return create(boundary_sections_.begin(), boundary_sections_.end());
		}

		/**
		 * Create a @a GpmlTopologicalNetwork instance which contains a boundary and interior geometries.
		 */
		static
		const non_null_ptr_type
		create(
				const std::vector<GpmlTopologicalSection::non_null_ptr_type> &boundary_sections_,
				const std::vector<GpmlPropertyDelegate::non_null_ptr_type> &interior_geometries_)
		{
			return create(
				boundary_sections_.begin(), boundary_sections_.end(),
				interior_geometries_.begin(), interior_geometries_.end());
		}


		/**
		 * Create a @a GpmlTopologicalNetwork instance which contains a boundary only (no interior geometries).
		 */
		template <typename BoundaryTopologicalSectionsIterator>
		static
		const non_null_ptr_type
		create(
				const BoundaryTopologicalSectionsIterator &boundary_sections_begin_,
				const BoundaryTopologicalSectionsIterator &boundary_sections_end_)
		{
			GPlatesModel::ModelTransaction transaction;
			non_null_ptr_type ptr(
					new GpmlTopologicalNetwork(
							transaction,
							GPlatesModel::RevisionedVector<GpmlTopologicalSection>::create(
									boundary_sections_begin_,
									boundary_sections_end_)));
			transaction.commit();
			return ptr;
		}

		/**
		 * Create a @a GpmlTopologicalNetwork instance which contains a boundary and interior geometries.
		 */
		template <typename BoundaryTopologicalSectionsIterator, typename InteriorGeometriesIterator>
		static
		const non_null_ptr_type
		create(
				const BoundaryTopologicalSectionsIterator &boundary_sections_begin_,
				const BoundaryTopologicalSectionsIterator &boundary_sections_end_,
				const InteriorGeometriesIterator &interior_geometries_begin_,
				const InteriorGeometriesIterator &interior_geometries_end_)
		{
			GPlatesModel::ModelTransaction transaction;
			non_null_ptr_type ptr(
					new GpmlTopologicalNetwork(
							transaction,
							GPlatesModel::RevisionedVector<GpmlTopologicalSection>::create(
									boundary_sections_begin_,
									boundary_sections_end_),
							GPlatesModel::RevisionedVector<GpmlPropertyDelegate>::create(
									interior_geometries_begin_,
									interior_geometries_end_)));
			transaction.commit();
			return ptr;
		}

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GpmlTopologicalNetwork>(clone_impl());
		}

		/**
		 * Returns the 'const' vector of members.
		 */
		const GPlatesModel::RevisionedVector<GpmlTopologicalSection> &
		boundary_sections() const
		{
			return *get_current_revision<Revision>().boundary_sections.get_revisionable();
		}

		/**
		 * Returns the 'non-const' vector of members.
		 */
		GPlatesModel::RevisionedVector<GpmlTopologicalSection> &
		boundary_sections()
		{
			return *get_current_revision<Revision>().boundary_sections.get_revisionable();
		}


		/**
		 * Returns the 'const' vector of members.
		 */
		const GPlatesModel::RevisionedVector<GpmlPropertyDelegate> &
		interior_geometries() const
		{
			return *get_current_revision<Revision>().interior_geometries.get_revisionable();
		}

		/**
		 * Returns the 'non-const' vector of members.
		 */
		GPlatesModel::RevisionedVector<GpmlPropertyDelegate> &
		interior_geometries()
		{
			return *get_current_revision<Revision>().interior_geometries.get_revisionable();
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
		 * Static access to the structural type as GpmlTopologicalNetwork::STRUCTURAL_TYPE.
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
			visitor.visit_gpml_topological_network(*this);
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
			visitor.visit_gpml_topological_network(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GpmlTopologicalNetwork(
				GPlatesModel::ModelTransaction &transaction_,
				GPlatesModel::RevisionedVector<GpmlTopologicalSection>::non_null_ptr_type boundary_sections_) :
			PropertyValue(Revision::non_null_ptr_type(new Revision(transaction_, *this, boundary_sections_)))
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GpmlTopologicalNetwork(
				GPlatesModel::ModelTransaction &transaction_,
				GPlatesModel::RevisionedVector<GpmlTopologicalSection>::non_null_ptr_type boundary_sections_,
				GPlatesModel::RevisionedVector<GpmlPropertyDelegate>::non_null_ptr_type interior_geometries_) :
			PropertyValue(Revision::non_null_ptr_type(new Revision(transaction_, *this, boundary_sections_, interior_geometries_)))
		{  }

		//! Constructor used when cloning.
		GpmlTopologicalNetwork(
				const GpmlTopologicalNetwork &other_,
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
			return non_null_ptr_type(new GpmlTopologicalNetwork(*this, context));
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
					GPlatesModel::RevisionedVector<GpmlTopologicalSection>::non_null_ptr_type boundary_sections_) :
				boundary_sections(
						GPlatesModel::RevisionedReference<
								GPlatesModel::RevisionedVector<GpmlTopologicalSection> >::attach(
										transaction_, child_context_, boundary_sections_)),
				interior_geometries(
						GPlatesModel::RevisionedReference<
								GPlatesModel::RevisionedVector<GpmlPropertyDelegate> >::attach(
										transaction_, child_context_, GPlatesModel::RevisionedVector<GpmlPropertyDelegate>::create()))
			{  }

			Revision(
					GPlatesModel::ModelTransaction &transaction_,
					RevisionContext &child_context_,
					GPlatesModel::RevisionedVector<GpmlTopologicalSection>::non_null_ptr_type boundary_sections_,
					GPlatesModel::RevisionedVector<GpmlPropertyDelegate>::non_null_ptr_type interior_geometries_) :
				boundary_sections(
						GPlatesModel::RevisionedReference<
								GPlatesModel::RevisionedVector<GpmlTopologicalSection> >::attach(
										transaction_, child_context_, boundary_sections_)),
				interior_geometries(
						GPlatesModel::RevisionedReference<
								GPlatesModel::RevisionedVector<GpmlPropertyDelegate> >::attach(
										transaction_, child_context_, interior_geometries_))
			{  }

			//! Deep-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<RevisionContext &> context_,
					RevisionContext &child_context_) :
				PropertyValue::Revision(context_),
				boundary_sections(other_.boundary_sections),
				interior_geometries(other_.interior_geometries)
			{
				// Clone data members that were not deep copied.
				boundary_sections.clone(child_context_);
				interior_geometries.clone(child_context_);
			}

			//! Shallow-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<RevisionContext &> context_) :
				PropertyValue::Revision(context_),
				boundary_sections(other_.boundary_sections),
				interior_geometries(other_.interior_geometries)
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

				return *boundary_sections.get_revisionable() == *other_revision.boundary_sections.get_revisionable() &&
						*interior_geometries.get_revisionable() == *other_revision.interior_geometries.get_revisionable() &&
						PropertyValue::Revision::equality(other);
			}

			GPlatesModel::RevisionedReference<GPlatesModel::RevisionedVector<GpmlTopologicalSection> > boundary_sections;
			GPlatesModel::RevisionedReference<GPlatesModel::RevisionedVector<GpmlPropertyDelegate> > interior_geometries;
		};

	};
}

#endif // GPLATES_PROPERTY_VALUES_GPMLTOPOLOGICALNETWORK_H
