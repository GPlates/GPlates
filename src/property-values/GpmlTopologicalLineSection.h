/* $Id: GpmlTopologicalLineSection.cc 7836 2010-03-22 00:53:03Z elau $ */

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

#ifndef GPLATES_PROPERTYVALUES_GPMLTOPOLOGICALLINESECTION_H
#define GPLATES_PROPERTYVALUES_GPMLTOPOLOGICALLINESECTION_H

#include "GpmlPropertyDelegate.h"
#include "GpmlTopologicalSection.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlTopologicalLineSection, visit_gpml_topological_line_section)

namespace GPlatesPropertyValues
{
	/**
	 * This class implements the PropertyValue which corresponds to "gpml:TopologicalLineSection".
	 */
	class GpmlTopologicalLineSection:
			public GpmlTopologicalSection
	{

	public:

		//! A convenience typedef for a shared pointer to a non-const @a GpmlTopologicalLineSection.
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlTopologicalLineSection> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GpmlTopologicalLineSection.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlTopologicalLineSection> non_null_ptr_to_const_type;

		virtual
		~GpmlTopologicalLineSection()
		{  }

		static
		const non_null_ptr_type
		create(
				GpmlPropertyDelegate::non_null_ptr_type source_geometry,
				const bool reverse_order) 
		{
			return non_null_ptr_type(
					new GpmlTopologicalLineSection(
							source_geometry, 
							reverse_order));
		}

		const GpmlTopologicalLineSection::non_null_ptr_type
		clone() const
		{
			return non_null_ptr_type(new GpmlTopologicalLineSection(*this));
		}

		const GpmlTopologicalLineSection::non_null_ptr_type
		deep_clone() const;

		DEFINE_FUNCTION_DEEP_CLONE_AS_PROP_VAL()

		DEFINE_FUNCTION_DEEP_CLONE_AS_TOPO_SECTION()

		/**
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gpml("TopologicalLineSection");
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
			visitor.visit_gpml_topological_line_section(*this);
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
			visitor.visit_gpml_topological_line_section(*this);
		}



		//! Returns the source geometry.
		GpmlPropertyDelegate::non_null_ptr_type
		get_source_geometry() const
		{
			return d_source_geometry;
		}

		//! Sets the source geometry.
		void
		set_source_geometry(
				const GpmlPropertyDelegate::non_null_ptr_type &source_geometry)
		{
			d_source_geometry = source_geometry;
			update_instance_id();
		} 

		//! Returns the reverse order.
		bool
		get_reverse_order() const
		{
			return d_reverse_order;
		}

		//! Sets the reverse order.
		void
		set_reverse_order(
				bool reverse_order)
		{
			d_reverse_order = reverse_order;
			update_instance_id();
		}

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GpmlTopologicalLineSection(
				GpmlPropertyDelegate::non_null_ptr_type source_geometry,
				const bool reverse_order) :
			GpmlTopologicalSection(),
			d_source_geometry( source_geometry ),
			d_reverse_order( reverse_order ) 
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GpmlTopologicalLineSection(
				const GpmlTopologicalLineSection &other) :
			GpmlTopologicalSection(other),
			d_source_geometry(other.d_source_geometry),
			d_reverse_order(other.d_reverse_order)
		{  }

		virtual
		bool
		directly_modifiable_fields_equal(
				const PropertyValue &other) const;

	private:

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GpmlTopologicalLineSection &
		operator=(
				const GpmlTopologicalLineSection &);

		GpmlPropertyDelegate::non_null_ptr_type d_source_geometry;
		bool d_reverse_order;

	};
}

#endif  // GPLATES_PROPERTYVALUES_GPMLTOPOLOGICALSECTION_H
