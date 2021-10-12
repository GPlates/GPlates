/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTYVALUES_GMLDATABLOCK_H
#define GPLATES_PROPERTYVALUES_GMLDATABLOCK_H

#include <vector>

#include "GmlDataBlockCoordinateList.h"
#include "model/FeatureVisitor.h"
#include "model/PropertyValue.h"


namespace GPlatesPropertyValues
{
	/**
	 * This class implements the PropertyValue which corresponds to "gml:DataBlock".
	 */
	class GmlDataBlock:
			public GPlatesModel::PropertyValue
	{
	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GmlDataBlock>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GmlDataBlock> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const GmlDataBlock>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GmlDataBlock> non_null_ptr_to_const_type;

		/**
		 * The type of the sequence of GmlDataBlockCoordinateList instances.
		 */
		typedef std::vector<GmlDataBlockCoordinateList::non_null_ptr_to_const_type>
				tuple_list_type;

		virtual
		~GmlDataBlock()
		{  }

		static
		const non_null_ptr_type
		create()
		{
			non_null_ptr_type ptr(new GmlDataBlock);
			return ptr;
		}

		const GmlDataBlock::non_null_ptr_type
		clone() const
		{
			GmlDataBlock::non_null_ptr_type dup(new GmlDataBlock(*this));
			return dup;
		}

		const GmlDataBlock::non_null_ptr_type
		deep_clone() const;

		DEFINE_FUNCTION_DEEP_CLONE_AS_PROP_VAL()

		bool
		is_empty() const
		{
			return d_tuple_list.empty();
		}

		tuple_list_type::const_iterator
		tuple_list_begin() const
		{
			return d_tuple_list.begin();
		}

		tuple_list_type::const_iterator
		tuple_list_end() const
		{
			return d_tuple_list.end();
		}

		void
		tuple_list_clear()
		{
			d_tuple_list.clear();
			update_instance_id();
		}

		void
		tuple_list_push_back(
				const GmlDataBlockCoordinateList::non_null_ptr_to_const_type &elem)
		{
			d_tuple_list.push_back(elem);
			update_instance_id();
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
			visitor.visit_gml_data_block(*this);
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
			visitor.visit_gml_data_block(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GmlDataBlock():
			PropertyValue()
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GmlDataBlock(
				const GmlDataBlock &other):
			PropertyValue(other), /* share instance id */
			d_tuple_list(other.d_tuple_list)
		{  }

	private:

		tuple_list_type d_tuple_list;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GmlDataBlock &
		operator=(const GmlDataBlock &);

	};


}

#endif  // GPLATES_PROPERTYVALUES_GMLDATABLOCK_H
