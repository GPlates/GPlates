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

#include "GpmlTopologicalSection.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "model/FeatureVisitor.h"
#include "model/PropertyValue.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlTopologicalLine, visit_gpml_topological_line)

namespace GPlatesPropertyValues
{	/**
	 * This class implements the PropertyValue which corresponds to "gpml:TopologicalLine".
	 */
	class GpmlTopologicalLine:
			public GPlatesModel::PropertyValue
	{

	public:

		//! A convenience typedef for a shared pointer to a non-const @a GpmlTopologicalLine.
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlTopologicalLine> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GpmlTopologicalLine.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlTopologicalLine> non_null_ptr_to_const_type;

		//! Typedef for a sequence of topological sections.
		typedef std::vector<GpmlTopologicalSection::non_null_ptr_type> sections_seq_type;

		//! Typedef for a const iterator over the topological sections.
		typedef sections_seq_type::const_iterator sections_const_iterator;


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
			return non_null_ptr_type(
					new GpmlTopologicalLine(sections_begin_, sections_end_));
		}

		const GpmlTopologicalLine::non_null_ptr_type
		clone() const
		{
			return non_null_ptr_type(new GpmlTopologicalLine(*this));
		}

		const GpmlTopologicalLine::non_null_ptr_type
		deep_clone() const;

		DEFINE_FUNCTION_DEEP_CLONE_AS_PROP_VAL()
		

		/**
		 * Return the "begin" const iterator to iterate over the topological sections.
		 */
		sections_const_iterator
		sections_begin() const
		{
			return d_sections.begin();
		}

		/**
		 * Return the "end" const iterator for iterating over the topological sections.
		 */
		sections_const_iterator
		sections_end() const
		{
			return d_sections.end();
		}


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
				const TopologicalSectionsIterator &sections_begin_,
				const TopologicalSectionsIterator &sections_end_) :
			PropertyValue(), 
			d_sections(sections_begin_, sections_end_)
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

		/**
		 * Need to compare all data members (recursively) since our sections are
		 * *non-const* non_null_intrusive_ptr and hence can be modified by clients.
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

		sections_seq_type d_sections;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GpmlTopologicalLine &
		operator=(
				const GpmlTopologicalLine &);

	};
}

#endif  // GPLATES_PROPERTYVALUES_GPMLTOPOLOGICALLINE_H
