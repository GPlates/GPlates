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
#include <boost/operators.hpp>

#include "GpmlTopologicalSection.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "model/FeatureVisitor.h"
#include "model/PropertyValue.h"

#include "property-values/GpmlPropertyDelegate.h"

#include "utils/CopyOnWrite.h"
#include "utils/QtStreamable.h"


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
			public GPlatesModel::PropertyValue
	{

	public:

		//! A convenience typedef for a shared pointer to a non-const @a GpmlTopologicalNetwork.
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlTopologicalNetwork> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GpmlTopologicalNetwork.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlTopologicalNetwork> non_null_ptr_to_const_type;


		/**
		 * Topological reference to a boundary section of the network.
		 */
		class BoundarySection :
				// Gives us "operator<<" for qDebug(), etc and QTextStream, if we provide for std::ostream...
				public GPlatesUtils::QtStreamable<BoundarySection>,
				public boost::equality_comparable<BoundarySection>
		{
		public:

			/**
			 * BoundarySection has value semantics where each @a BoundarySection instance has its own state.
			 * So if you create a copy and modify the copy's state then it will not modify the state
			 * of the original object.
			 *
			 * The constructor first clones the property delegate and then copy-on-write is used to allow
			 * multiple @a BoundarySection objects to share the same state (until the state is modified).
			 */
			BoundarySection(
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
					const BoundarySection &other) const
			{
				return *d_source_section.get_const() == *other.d_source_section.get_const();
			}

		private:

			GPlatesUtils::CopyOnWrite<GpmlTopologicalSection::non_null_ptr_type> d_source_section;
		};

		//! Typedef for a sequence of boundary sections.
		typedef std::vector<BoundarySection> boundary_sections_seq_type;


		/**
		 * Topological reference to an interior geometry of the network.
		 */
		class Interior :
				// Gives us "operator<<" for qDebug(), etc and QTextStream, if we provide for std::ostream...
				public GPlatesUtils::QtStreamable<Interior>,
				public boost::equality_comparable<Interior>
		{
		public:

			/**
			 * Interior has value semantics where each @a Interior instance has its own state.
			 * So if you create a copy and modify the copy's state then it will not modify the state
			 * of the original object.
			 *
			 * The constructor first clones the property delegate and then copy-on-write is used to allow
			 * multiple @a Interior objects to share the same state (until the state is modified).
			 */
			Interior(
					GpmlPropertyDelegate::non_null_ptr_type source_geometry) :
				d_source_geometry(source_geometry)
			{  }

			/**
			 * Returns the 'const' source geometry.
			 */
			GpmlPropertyDelegate::non_null_ptr_to_const_type
			get_source_geometry() const
			{
				return d_source_geometry.get();
			}

			/**
			 * Returns the 'non-const' source geometry.
			 */
			GpmlPropertyDelegate::non_null_ptr_type
			get_source_geometry()
			{
				return d_source_geometry.get();
			}

			/**
			 * Value equality comparison operator.
			 *
			 * Inequality provided by boost equality_comparable.
			 */
			bool
			operator==(
					const Interior &other) const
			{
				return *d_source_geometry.get_const() == *other.d_source_geometry.get_const();
			}

		private:

			GPlatesUtils::CopyOnWrite<GpmlPropertyDelegate::non_null_ptr_type> d_source_geometry;
		};

		//! Typedef for a sequence of interior geometries.
		typedef std::vector<Interior> interior_geometry_seq_type;


		virtual
		~GpmlTopologicalNetwork()
		{  }

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
			return non_null_ptr_type(
					new GpmlTopologicalNetwork(boundary_sections_begin_, boundary_sections_end_));
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
			return non_null_ptr_type(
					new GpmlTopologicalNetwork(
							boundary_sections_begin_, boundary_sections_end_,
							interior_geometries_begin_, interior_geometries_end_));
		}

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GpmlTopologicalNetwork>(clone_impl());
		}


		/**
		 * Returns the boundary sections.
		 *
		 * To modify any boundary sections:
		 * (1) make additions/removals/modifications to a copy of the returned vector, and
		 * (2) use @a set_boundary_sections to set them.
		 *
		 * The returned boundary sections implement copy-on-write to promote resource sharing (until write)
		 * and to ensure our internal state cannot be modified and bypass the revisioning system.
		 */
		const boundary_sections_seq_type &
		get_boundary_sections() const
		{
			return get_current_revision<Revision>().boundary_sections;
		}

		/**
		 * Set the sequence of boundary sections.
		 */
		void
		set_boundary_sections(
				const boundary_sections_seq_type &boundary_sections);


		/**
		 * Returns the interior geometries.
		 *
		 * To modify any interior geometries:
		 * (1) make additions/removals/modifications to a copy of the returned vector, and
		 * (2) use @a set_interior_geometries to set them.
		 *
		 * The returned interior geometries implement copy-on-write to promote resource sharing (until write)
		 * and to ensure our internal state cannot be modified and bypass the revisioning system.
		 */
		const interior_geometry_seq_type &
		get_interior_geometries() const
		{
			return get_current_revision<Revision>().interior_geometries;
		}

		/**
		 * Set the sequence of interior geometries.
		 */
		void
		set_interior_geometries(
				const interior_geometry_seq_type &interior_geometries);


		/**
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gpml("TopologicalNetwork");
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
		template <typename BoundaryTopologicalSectionsIterator>
		GpmlTopologicalNetwork(
				const BoundaryTopologicalSectionsIterator &boundary_sections_begin_,
				const BoundaryTopologicalSectionsIterator &boundary_sections_end_) :
			PropertyValue(
					Revision::non_null_ptr_type(
							new Revision(boundary_sections_begin_, boundary_sections_end_)))
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		template <typename BoundaryTopologicalSectionsIterator, typename InteriorGeometriesIterator>
		GpmlTopologicalNetwork(
				const BoundaryTopologicalSectionsIterator &boundary_sections_begin_,
				const BoundaryTopologicalSectionsIterator &boundary_sections_end_,
				const InteriorGeometriesIterator &interior_geometries_begin_,
				const InteriorGeometriesIterator &interior_geometries_end_) :
			PropertyValue(
					Revision::non_null_ptr_type(
							new Revision(
									boundary_sections_begin_, boundary_sections_end_,
									interior_geometries_begin_, interior_geometries_end_)))
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GpmlTopologicalNetwork(
				const GpmlTopologicalNetwork &other) :
			PropertyValue(other)
		{  }

		virtual
		const GPlatesModel::PropertyValue::non_null_ptr_type
		clone_impl() const
		{
			return non_null_ptr_type(new GpmlTopologicalNetwork(*this));
		}

	private:

		/**
		 * Property value data that is mutable/revisionable.
		 */
		struct Revision :
				public GPlatesModel::PropertyValue::Revision
		{
			template <typename BoundaryTopologicalSectionsIterator>
			Revision(
					const BoundaryTopologicalSectionsIterator &boundary_sections_begin_,
					const BoundaryTopologicalSectionsIterator &boundary_sections_end_) :
				boundary_sections(boundary_sections_begin_, boundary_sections_end_)
			{  }

			template <typename BoundaryTopologicalSectionsIterator, typename InteriorGeometriesIterator>
			Revision(
					const BoundaryTopologicalSectionsIterator &boundary_sections_begin_,
					const BoundaryTopologicalSectionsIterator &boundary_sections_end_,
					const InteriorGeometriesIterator &interior_geometries_begin_,
					const InteriorGeometriesIterator &interior_geometries_end_) :
				boundary_sections(boundary_sections_begin_, boundary_sections_end_),
				interior_geometries(interior_geometries_begin_, interior_geometries_end_)
			{  }

			virtual
			GPlatesModel::PropertyValue::Revision::non_null_ptr_type
			clone() const
			{
				return non_null_ptr_type(new Revision(*this));
			}

			// Don't need 'clone_for_bubble_up_modification()' since we're using CopyOnWrite.

			virtual
			bool
			equality(
					const GPlatesModel::PropertyValue::Revision &other) const;

			boundary_sections_seq_type boundary_sections;
			interior_geometry_seq_type interior_geometries;
		};

	};


	// operator<< for GpmlTopologicalNetwork::BoundarySection.
	std::ostream &
	operator<<(
			std::ostream &os,
			const GpmlTopologicalNetwork::BoundarySection &topological_network_boundary_section);

	// operator<< for GpmlTopologicalNetwork::Interior.
	std::ostream &
	operator<<(
			std::ostream &os,
			const GpmlTopologicalNetwork::Interior &topological_network_interior);
}

#endif // GPLATES_PROPERTY_VALUES_GPMLTOPOLOGICALNETWORK_H
