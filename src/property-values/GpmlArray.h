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

#include <vector>

#include "StructuralType.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "model/PropertyValue.h"

#include "utils/CopyOnWrite.h"

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

		/**
		 * Typedef for a sequence of property values.
		 *
		 * Get the non_null_intrusive_ptr using 'CopyOnWrite::get_const()' or 'CopyOnWrite::get_non_const()'.
		 */
		typedef std::vector<
				GPlatesUtils::CopyOnWrite<GPlatesModel::PropertyValue::non_null_ptr_type> >
						member_array_type;


		virtual
		~GpmlArray()
		{  }


		static
		const non_null_ptr_type
		create(
			const StructuralType &value_type,		
			const std::vector<GPlatesModel::PropertyValue::non_null_ptr_type> &members)
		{
			return create(value_type, members.begin(), members.end());
		}

		static
		const non_null_ptr_type
		create(
			const StructuralType &value_type,		
			const member_array_type &members)
		{
			return create(value_type, members.begin(), members.end());
		}

		template<typename ForwardIterator>
		static
		const non_null_ptr_type
		create(
				const StructuralType &value_type,		
				ForwardIterator begin,
				ForwardIterator end)
		{
			return non_null_ptr_type(new GpmlArray(value_type, begin, end));
		}

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GpmlArray>(clone_impl());
		}

		/**
		 * Returns the members.
		 *
		 * To modify any members:
		 * (1) make additions/removals/modifications to a copy of the returned vector, and
		 * (2) use @a set_members to set them.
		 *
		 * The returned members implement copy-on-write to promote resource sharing (until write)
		 * and to ensure our internal state cannot be modified and bypass the revisioning system.
		 */
		const member_array_type &
		get_members() const
		{
			return get_current_revision<Revision>().members;
		}

		/**
		 * Sets the internal members.
		 */
		void
		set_members(
				const member_array_type &members);

		// Note that no "setter" is provided:  The value type of a GpmlArray
		// instance should never be changed.
		StructuralType
		get_value_type() const
		{
			return d_value_type;
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

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:


		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		template<typename ForwardIterator>
		GpmlArray(
				const StructuralType &value_type,
				ForwardIterator begin,
				ForwardIterator end) :
			PropertyValue(Revision::non_null_ptr_type(new Revision(begin, end))),
			d_value_type(value_type)
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GpmlArray(
				const GpmlArray &other) :
			PropertyValue(other),
			d_value_type(other.d_value_type)
		{  }

		virtual
		const GPlatesModel::PropertyValue::non_null_ptr_type
		clone_impl() const
		{
			return non_null_ptr_type(new GpmlArray(*this));
		}

		virtual
		bool
		equality(
				const PropertyValue &other) const
		{
			const GpmlArray &other_pv = dynamic_cast<const GpmlArray &>(other);

			return d_value_type == other_pv.d_value_type &&
					// The revisioned data comparisons are handled here...
					GPlatesModel::PropertyValue::equality(other);
		}

	private:

		/**
		 * Property value data that is mutable/revisionable.
		 */
		struct Revision :
				public GPlatesModel::PropertyValue::Revision
		{
			template<typename ForwardIterator>
			Revision(
					ForwardIterator begin_,
					ForwardIterator end_) :
				members(begin_, end_)
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

			std::vector<GPlatesUtils::CopyOnWrite<GPlatesModel::PropertyValue::non_null_ptr_type> > members;
		};

		StructuralType d_value_type;

	};
}

#endif // GPLATES_PROPERTYVALUES_GPMLARRAY_H
