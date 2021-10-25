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
#include "model/PropertyValue.h"

#include "property-values/GpmlPropertyDelegate.h"

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

		//! Typedef for a sequence of boundary sections.
		typedef std::vector<GpmlTopologicalSection::non_null_ptr_type> boundary_sections_seq_type;

		//! Typedef for a const iterator over the boundary sections.
		typedef boundary_sections_seq_type::const_iterator boundary_sections_const_iterator;


		/**
		 * Topological reference to an interior geometry of the network.
		 */
		class Interior :
				// Gives us "operator<<" for qDebug(), etc and QTextStream, if we provide for std::ostream...
				public GPlatesUtils::QtStreamable<Interior>
		{
		public:

			explicit
			Interior(
					const GpmlPropertyDelegate::non_null_ptr_type &source_geometry) :
				d_source_geometry(source_geometry)
			{  }

			//! Returns the source geometry.
			const GpmlPropertyDelegate::non_null_ptr_type &
			get_source_geometry() const
			{
				return d_source_geometry;
			}

			const Interior
			deep_clone() const;

			bool
			operator==(
					const Interior &other) const;

		private:

			GpmlPropertyDelegate::non_null_ptr_type d_source_geometry;
		};

		//! Typedef for a sequence of interior geometries.
		typedef std::vector<Interior> interior_geometry_seq_type;

		//! Typedef for a const iterator over the interior geometries.
		typedef interior_geometry_seq_type::const_iterator interior_geometries_const_iterator;


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

		const GpmlTopologicalNetwork::non_null_ptr_type
		clone() const
		{
			return non_null_ptr_type(new GpmlTopologicalNetwork(*this));
		}

		const GpmlTopologicalNetwork::non_null_ptr_type
		deep_clone() const;

		DEFINE_FUNCTION_DEEP_CLONE_AS_PROP_VAL()
		

		/**
		 * Return the "begin" const iterator to iterate over the boundary sections.
		 */
		boundary_sections_const_iterator
		boundary_sections_begin() const
		{
			return d_boundary_sections.begin();
		}

		/**
		 * Return the "end" const iterator for iterating over the boundary sections.
		 */
		boundary_sections_const_iterator
		boundary_sections_end() const
		{
			return d_boundary_sections.end();
		}

		
		/**
		 * Return the "begin" const iterator to iterate over the interior geometries.
		 */
		interior_geometries_const_iterator
		interior_geometries_begin() const
		{
			return d_interior_geometries.begin();
		}

		/**
		 * Return the "end" const iterator for iterating over the interior geometries.
		 */
		interior_geometries_const_iterator
		interior_geometries_end() const
		{
			return d_interior_geometries.end();
		}


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
			PropertyValue(), 
			d_boundary_sections(boundary_sections_begin_, boundary_sections_end_)
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		template <typename BoundaryTopologicalSectionsIterator, typename InteriorGeometriesIterator>
		GpmlTopologicalNetwork(
				const BoundaryTopologicalSectionsIterator &boundary_sections_begin_,
				const BoundaryTopologicalSectionsIterator &boundary_sections_end_,
				const InteriorGeometriesIterator &interior_geometries_begin_,
				const InteriorGeometriesIterator &interior_geometries_end_) :
			PropertyValue(), 
			d_boundary_sections(boundary_sections_begin_, boundary_sections_end_),
			d_interior_geometries(interior_geometries_begin_, interior_geometries_end_)
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GpmlTopologicalNetwork(
				const GpmlTopologicalNetwork &other) :
			PropertyValue(other), /* share instance id */
			d_boundary_sections(other.d_boundary_sections),
			d_interior_geometries(other.d_interior_geometries)
		{  }

		/**
		 * Need to compare all data members (recursively) since our boundary sections and interior
		 * geometries are *non-const* non_null_intrusive_ptr and hence can be modified by clients.
		 *
		 * FIXME: Use *const* non_null_intrusive_ptr to avoid this.
		 * Although that means use *const* feature visitors which is currently means changes
		 * will propagate quite far across GPlates - ie, won't be a trivial task to make this change.
		 */
		virtual
		bool
		directly_modifiable_fields_equal(
				const PropertyValue &other) const;

	private:

		boundary_sections_seq_type d_boundary_sections;

		interior_geometry_seq_type d_interior_geometries;


		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GpmlTopologicalNetwork &
		operator=(
				const GpmlTopologicalNetwork &);

	};


	// operator<< for GpmlTopologicalNetwork::Interior.
	std::ostream &
	operator<<(
			std::ostream &os,
			const GpmlTopologicalNetwork::Interior &topological_network_interior);
}

#endif // GPLATES_PROPERTY_VALUES_GPMLTOPOLOGICALNETWORK_H
