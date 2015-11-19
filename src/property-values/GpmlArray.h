/* $Id: GpmlArray.h 10193 2010-11-11 16:12:10Z rwatson $ */

/**
 * @file 
 *
 * Most recent change:
 *   $Date: 2010-11-12 03:12:10 +1100 (Fri, 12 Nov 2010) $
 * 
 * Copyright (C) 2010 Geological Survey of Norway
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

#ifndef GPLATES_PROPERTYVALUES_GPMLARRAY_H
#define GPLATES_PROPERTYVALUES_GPMLARRAY_H

#include "StructuralType.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "model/PropertyValue.h"

// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlArray, visit_gpml_array)

namespace GPlatesPropertyValues
{

	class GpmlArray:
		public GPlatesModel::PropertyValue
	{

	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GpmlArray>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlArray> non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const GpmlArray>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlArray> non_null_ptr_to_const_type;


		virtual
		~GpmlArray()
		{  }


		static
		const non_null_ptr_type
		create(
			const StructuralType &value_type_,		
                        const std::vector<GPlatesModel::PropertyValue::non_null_ptr_type> &members)
		{
			return non_null_ptr_type(new GpmlArray(
					value_type_,
					members));
		}

		const non_null_ptr_type
		clone() const
		{
			return non_null_ptr_type(new GpmlArray(*this));
		}

		const non_null_ptr_type
		deep_clone() const;

		DEFINE_FUNCTION_DEEP_CLONE_AS_PROP_VAL()

		const std::vector<GPlatesModel::PropertyValue::non_null_ptr_type> &
		members() const
		{
			return d_members;
		}

		std::vector<GPlatesModel::PropertyValue::non_null_ptr_type> &
		members()
		{
			return d_members;
		}

		const StructuralType &
		type() const
		{
			return d_type;
		}

		/**
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gpml("Array");
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
			visitor.visit_gpml_array(*this);
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
			visitor.visit_gpml_array(*this);
		}

		bool
		is_empty() const
		{
			return d_members.empty();
		}

		std::vector<GPlatesModel::PropertyValue::non_null_ptr_type>::size_type
		num_elements() const
		{
		    return d_members.size();
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;


		virtual
		bool
		directly_modifiable_fields_equal(
			const PropertyValue &other) const;

	protected:


		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GpmlArray(
			const StructuralType &value_type_,
			const std::vector<GPlatesModel::PropertyValue::non_null_ptr_type> &members_):
				PropertyValue(),
				d_type(value_type_),
				d_members(members_)
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GpmlArray(
			const GpmlArray &other) :
				PropertyValue(other),
				d_type(other.d_type),
				d_members(other.d_members)
		{  }


	private:

		StructuralType d_type;

		std::vector<GPlatesModel::PropertyValue::non_null_ptr_type> d_members;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GpmlArray &
			operator=(const GpmlArray &);
	};
}

#endif // GPLATES_PROPERTYVALUES_GPMLARRAY_H
