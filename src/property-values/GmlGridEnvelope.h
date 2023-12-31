/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
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

#ifndef GPLATES_PROPERTYVALUES_GMLGRIDENVELOPE_H
#define GPLATES_PROPERTYVALUES_GMLGRIDENVELOPE_H

#include <vector>

#include "feature-visitors/PropertyValueFinder.h"

#include "model/PropertyValue.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GmlGridEnvelope, visit_gml_grid_envelope)

namespace GPlatesPropertyValues
{
	/**
	 * This class implements the PropertyValue which corresponds to "gml:GridEnvelope".
	 */
	class GmlGridEnvelope:
			public GPlatesModel::PropertyValue
	{
	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GmlGridEnvelope>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GmlGridEnvelope> non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const GmlGridEnvelope>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GmlGridEnvelope> non_null_ptr_to_const_type;


		virtual
		~GmlGridEnvelope()
		{  }

		typedef std::vector<int> integer_list_type;

		/**
		 * Create a GmlGridEnvelope instance from @a low_ and @a high_ positions.
		 *
		 * The number of dimensions in @a low_ and @a high_ must be the same.
		 */
		static
		const non_null_ptr_type
		create(
				const integer_list_type &low_,
				const integer_list_type &high_);

		const non_null_ptr_type
		clone() const
		{
			non_null_ptr_type dup(new GmlGridEnvelope(*this));
			return dup;
		}

		const non_null_ptr_type
		deep_clone() const
		{
			// This class doesn't reference any mutable objects by pointer, so there's
			// no need for any recursive cloning.  Hence, regular clone will suffice.
			return clone();
		}

		DEFINE_FUNCTION_DEEP_CLONE_AS_PROP_VAL()

		const integer_list_type &
		low() const
		{
			return d_low;
		}

		const integer_list_type &
		high() const
		{
			return d_high;
		}

		void
		set_low_and_high(
				const integer_list_type &low_,
				const integer_list_type &high_);

		/**
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gml("GridEnvelope");
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
			visitor.visit_gml_grid_envelope(*this);
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
			visitor.visit_gml_grid_envelope(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		GmlGridEnvelope(
				const integer_list_type &low_,
				const integer_list_type &high_) :
			PropertyValue(),
			d_low(low_),
			d_high(high_)
		{  }


		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GmlGridEnvelope(
				const GmlGridEnvelope &other) :
			PropertyValue(other), /* share instance id */
			d_low(other.d_low),
			d_high(other.d_high)
		{  }

	private:

		integer_list_type d_low;
		integer_list_type d_high;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GmlGridEnvelope &
		operator=(
				const GmlGridEnvelope &);

	};

}

#endif  // GPLATES_PROPERTYVALUES_GMLGRIDENVELOPE_H
