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
#include <boost/intrusive_ptr.hpp>

#include "model/PropertyValue.h"
#include "GpmlTopologicalLineSection.h"

namespace GPlatesPropertyValues
{

	class GpmlTopologicalLine:
			public GPlatesModel::PropertyValue
	{

	public:

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<GpmlTopologicalLine>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlTopologicalLine> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const GpmlTopologicalLine>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlTopologicalLine> non_null_ptr_to_const_type;

		virtual
		~GpmlTopologicalLine()
		{  }

		static
		const non_null_ptr_type
		create( 
			const GpmlTopologicalSection::non_null_ptr_type &first_section)
		{
			non_null_ptr_type ptr(
					new GpmlTopologicalLine( first_section ));
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
					new GpmlTopologicalLine( sections_ ));
			return ptr;
		}

		const GpmlTopologicalLine::non_null_ptr_type
		clone() const
		{
			GpmlTopologicalLine::non_null_ptr_type dup(
					new GpmlTopologicalLine(*this));
			return dup;
		}

		const GpmlTopologicalLine::non_null_ptr_type
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
		GpmlTopologicalLine(
				const GpmlTopologicalSection::non_null_ptr_type &first_section):
			PropertyValue(), 
			d_sections()
		{
			d_sections.push_back(first_section);
		}

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GpmlTopologicalLine(
				const std::vector<GpmlTopologicalSection::non_null_ptr_type> &s):
			PropertyValue(), 
			d_sections(s)
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GpmlTopologicalLine(
				const GpmlTopologicalLine &other) :
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
		GpmlTopologicalLine &
		operator=(const GpmlTopologicalLine &);

	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLTOPOLOGICALLINE_H
