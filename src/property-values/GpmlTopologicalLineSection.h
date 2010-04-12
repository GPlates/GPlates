/* $Id: GpmlTopologicalLineSection.cc 7836 2010-03-22 00:53:03Z elau $ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2008-07-11 19:36:59 -0700 (Fri, 11 Jul 2008) $
 * 
 * Copyright (C) 2008, 2009, 2010 California Institute of Technology
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

#include "GpmlTopologicalSection.h"
#include "GpmlTopologicalIntersection.h"
#include "GpmlPropertyDelegate.h"


namespace GPlatesPropertyValues
{

	class GpmlTopologicalLineSection:
			public GpmlTopologicalSection
	{

	public:

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<GpmlTopologicalLineSection>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlTopologicalLineSection> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const GpmlTopologicalLineSection>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlTopologicalLineSection> non_null_ptr_to_const_type;

		virtual
		~GpmlTopologicalLineSection()
		{  }

		// This creation function is here purely for the simple, hard-coded construction of
		// features.  It may not be necessary or appropriate later on when we're doing
		// everything properly, so don't look at this function and think "Uh oh, this
		// function doesn't look like it should be here, but I'm sure it's here for a
		// reason..."
		static
		const non_null_ptr_type
		create(
				GpmlPropertyDelegate::non_null_ptr_type source_geometry,
				const boost::optional<GpmlTopologicalIntersection> start_intersection,
				const boost::optional<GpmlTopologicalIntersection> end_intersection,
				const bool reverse_order) 
		{
			non_null_ptr_type ptr(
				new GpmlTopologicalLineSection(
					source_geometry, 
					start_intersection, 
					end_intersection, 
					reverse_order));
			return ptr;
		}

		const GpmlTopologicalLineSection::non_null_ptr_type
		clone() const
		{
			GpmlTopologicalLineSection::non_null_ptr_type dup(
					new GpmlTopologicalLineSection(*this));
			return dup;
		}

		const GpmlTopologicalLineSection::non_null_ptr_type
		deep_clone() const;

		DEFINE_FUNCTION_DEEP_CLONE_AS_PROP_VAL()

		DEFINE_FUNCTION_DEEP_CLONE_AS_TOPO_SECTION()

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



		// access to d_source_geometry
		GpmlPropertyDelegate::non_null_ptr_type
		get_source_geometry() const
		{
			return d_source_geometry;
		}

		void
		set_source_geometry(
				GpmlPropertyDelegate::non_null_ptr_type intersection_geom)
		{
			d_source_geometry = intersection_geom;
			update_instance_id();
		} 


		// access to start intersection
		boost::optional<GpmlTopologicalIntersection>
		get_start_intersection() const
		{
			return d_start_intersection;
		}

		void
		set_start_intersection(
				boost::optional<GpmlTopologicalIntersection> start)
		{
			d_start_intersection = start;
			update_instance_id();
		}

		// access to end intersection
		boost::optional<GpmlTopologicalIntersection>
		get_end_intersection() const
		{
			return d_end_intersection;
		}

		void
		set_end_intersection(
				boost::optional<GpmlTopologicalIntersection> end)
		{
			d_end_intersection = end;
			update_instance_id();
		}

		// access to d_reverse_order
		bool
		get_reverse_order() const
		{
			return d_reverse_order;
		}

		void
		set_reverse_order( bool order)
		{
			d_reverse_order = order;
			update_instance_id();
		}

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GpmlTopologicalLineSection(
				GpmlPropertyDelegate::non_null_ptr_type source_geometry,
				const boost::optional<GpmlTopologicalIntersection> start_intersection,
				const boost::optional<GpmlTopologicalIntersection> end_intersection,
				const bool reverse_order) :
			GpmlTopologicalSection(),
			d_source_geometry( source_geometry ),
			d_start_intersection( start_intersection ),
			d_end_intersection( end_intersection ),
			d_reverse_order( reverse_order ) 
		{  }

#if 0
		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GpmlTopologicalLineSection(
				const GpmlTopologicalLineSection &other) :
			GpmlTopologicalSection(other),
			d_source_geometry(other.d_source_geometry) // will get overwritten in deep_clone() later.
		{  }
#endif

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
		operator=(const GpmlTopologicalLineSection &);

		GpmlPropertyDelegate::non_null_ptr_type d_source_geometry;
		boost::optional<GpmlTopologicalIntersection> d_start_intersection;
		boost::optional<GpmlTopologicalIntersection> d_end_intersection;
		bool d_reverse_order;

	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLTOPOLOGICALSECTION_H
