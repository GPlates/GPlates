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

#ifndef GPLATES_PROPERTYVALUES_GPMLTOPOLOGICALPOLYGON_H
#define GPLATES_PROPERTYVALUES_GPMLTOPOLOGICALPOLYGON_H

#include <vector>
#include <boost/intrusive_ptr.hpp>

#include "model/PropertyValue.h"
#include "GpmlTopologicalLineSection.h"

namespace GPlatesPropertyValues
{

	class GpmlTopologicalPolygon:
			public GPlatesModel::PropertyValue
	{

	public:

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<GpmlTopologicalPolygon>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlTopologicalPolygon> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const GpmlTopologicalPolygon>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlTopologicalPolygon> non_null_ptr_to_const_type;

		virtual
		~GpmlTopologicalPolygon()
		{  }

		static
		const non_null_ptr_type
		create( 
			const GpmlTopologicalSection::non_null_ptr_type &first_section)
		{
			non_null_ptr_type ptr(
					new GpmlTopologicalPolygon( first_section ));
			return ptr;
		}

		// This creation function is here purely for the simple, hard-coded construction of
		// features.  It may not be necessary or appropriate later on when we're doing
		// everything properly, so don't look at this function and think "Uh oh, this
		// function doesn't look like it should be here, but I'm sure it's here for a
		// reason..."
		static
		const non_null_ptr_type
		create( const std::vector<GpmlTopologicalSection::non_null_ptr_type> &sections_ )
		{
			non_null_ptr_type ptr(
					new GpmlTopologicalPolygon( sections_ ));
			return ptr;
		}

		const GpmlTopologicalPolygon::non_null_ptr_type
		clone() const
		{
			GpmlTopologicalPolygon::non_null_ptr_type dup(
					new GpmlTopologicalPolygon(*this));
			return dup;
		}

		const GpmlTopologicalPolygon::non_null_ptr_type
		deep_clone() const;

		DEFINE_FUNCTION_DEEP_CLONE_AS_PROP_VAL()

		// @b FIXME:  Should this function be replaced with per-index const-access to
		// elements of the time sample vector?  (For consistency with the non-const
		// overload...)
		const std::vector<GpmlTopologicalSection::non_null_ptr_type> &
		sections() const
		{
			return d_sections;
		}

		// @b FIXME:  Should this function be replaced with per-index const-access to
		// elements of the time sample vector, well as per-index assignment (setter) and
		// removal operations?  This would ensure that revisioning is correctly handled...
		std::vector<GpmlTopologicalSection::non_null_ptr_type> &
		sections()
		{
			return d_sections;
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
		GpmlTopologicalPolygon(
				const GpmlTopologicalSection::non_null_ptr_type &first_section):
			PropertyValue(), 
			d_sections()
		{
			d_sections.push_back(first_section);
		}

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GpmlTopologicalPolygon(
				const std::vector<GpmlTopologicalSection::non_null_ptr_type> &s):
			PropertyValue(), 
			d_sections(s)
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GpmlTopologicalPolygon(
				const GpmlTopologicalPolygon &other) :
			PropertyValue(other), /* share instance id */
			d_sections(other.d_sections)
		{  }

		virtual
		bool
		directly_modifiable_fields_equal(
				const PropertyValue &other) const;

	private:

		/** Because GpmlTopologicalSection is abstract we use non_null_ptr_type */
		std::vector<GpmlTopologicalSection::non_null_ptr_type> d_sections;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GpmlTopologicalPolygon &
		operator=(const GpmlTopologicalPolygon &);

	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLTOPOLOGICALPOLYGON_H
