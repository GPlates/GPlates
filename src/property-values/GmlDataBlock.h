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
#include "utils/CopyOnWrite.h"


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
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const GmlDataBlock>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GmlDataBlock> non_null_ptr_to_const_type;


		/**
		 * A list of coordinates for a specific coordinate in the tuple list.
		 */
		class CoordinateList :
				public boost::equality_comparable<CoordinateList>
		{
		public:

			/**
			 * CoordinateList has value semantics where each @a CoordinateList instance has its own state.
			 * So if you create a copy and modify the copy's state then it will not modify the state
			 * of the original object.
			 *
			 * The constructor first clones the property value and then copy-on-write is used to allow
			 * multiple @a CoordinateList objects to share the same state (until the state is modified).
			 */
			CoordinateList(
					GmlDataBlockCoordinateList::non_null_ptr_type value) :
				d_value(value)
			{  }

			/**
			 * Returns the 'const' coordinate list.
			 */
			const GmlDataBlockCoordinateList::non_null_ptr_to_const_type
			get_value() const
			{
				return d_value.get();
			}

			/**
			 * Returns the 'non-const' coordinate list.
			 */
			const GmlDataBlockCoordinateList::non_null_ptr_type
			get_value()
			{
				return d_value.get();
			}

			void
			set_value(
					GmlDataBlockCoordinateList::non_null_ptr_type value)
			{
				d_value = value;
			}

			/**
			 * Value equality comparison operator.
			 *
			 * Inequality provided by boost equality_comparable.
			 */
			bool
			operator==(
					const CoordinateList &other) const
			{
				return *d_value.get_const() == *other.d_value.get_const();
			}

		private:
			GPlatesUtils::CopyOnWrite<GmlDataBlockCoordinateList::non_null_ptr_type> d_value;
		};

		//! Typedef for a sequence of coordinate lists.
		typedef std::vector<CoordinateList> tuple_list_type;


		virtual
		~GmlDataBlock()
		{  }

		static
		const non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new GmlDataBlock(tuple_list_type()));
		}

		static
		const non_null_ptr_type
		create(
				const tuple_list_type &tuple_list)
		{
			return non_null_ptr_type(new GmlDataBlock(tuple_list));
		}

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GmlDataBlock>(clone_impl());
		}

		/**
		 * Returns the tuple list.
		 *
		 * To modify any tuples:
		 * (1) make a copy of the returned vector,
		 * (2) make additions/removals/modifications to the returned vector, and
		 * (3) use @a set_tuple_list to set them.
		 */
		const tuple_list_type &
		get_tuple_list() const
		{
			return get_current_revision<Revision>().tuple_list;
		}

		void
		set_tuple_list(
				const tuple_list_type &tuple_list);

		/**
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gml("DataBlock");
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
		explicit
		GmlDataBlock(
				const tuple_list_type &tuple_list) :
			PropertyValue(Revision::non_null_ptr_type(new Revision(tuple_list)))
		{  }

		virtual
		const GPlatesModel::PropertyValue::non_null_ptr_type
		clone_impl() const
		{
			return non_null_ptr_type(new GmlDataBlock(*this));
		}

	private:

		/**
		 * Property value data that is mutable/revisionable.
		 */
		struct Revision :
				public GPlatesModel::PropertyValue::Revision
		{
			explicit
			Revision(
					const tuple_list_type &tuple_list_) :
				tuple_list(tuple_list_)
			{  }

			virtual
			GPlatesModel::PropertyValue::Revision::non_null_ptr_type
			clone() const
			{
				// The default copy constructor is fine since we use CopyOnWrite.
				return non_null_ptr_type(new Revision(*this));
			}

			virtual
			bool
			equality(
					const GPlatesModel::PropertyValue::Revision &other) const
			{
				const Revision &other_revision = dynamic_cast<const Revision &>(other);

				return tuple_list == other_revision.tuple_list &&
					GPlatesModel::PropertyValue::Revision::equality(other);
			}

			tuple_list_type tuple_list;
		};

	};


}

#endif  // GPLATES_PROPERTYVALUES_GMLDATABLOCK_H
