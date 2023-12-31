/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTYVALUES_GPMLFINITEROTATIONSLERP_H
#define GPLATES_PROPERTYVALUES_GPMLFINITEROTATIONSLERP_H

#include "GpmlInterpolationFunction.h"
#include "feature-visitors/PropertyValueFinder.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlFiniteRotationSlerp, visit_gpml_finite_rotation_slerp)

namespace GPlatesPropertyValues {

	class GpmlFiniteRotationSlerp:
			public GpmlInterpolationFunction
	{

	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GpmlFiniteRotationSlerp>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlFiniteRotationSlerp> non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const GpmlFiniteRotationSlerp>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlFiniteRotationSlerp> non_null_ptr_to_const_type;


		virtual
		~GpmlFiniteRotationSlerp()
		{  }

		static
		const non_null_ptr_type
		create(
				const StructuralType &value_type_)
		{
			return non_null_ptr_type(new GpmlFiniteRotationSlerp(value_type_));
		}

		const non_null_ptr_type
		clone() const
		{
			return non_null_ptr_type(new GpmlFiniteRotationSlerp(*this));
		}

		const non_null_ptr_type
		deep_clone() const
		{
			// This class doesn't reference any mutable objects by pointer, so there's
			// no need for any recursive cloning.  Hence, regular clone will suffice.
			return clone();
		}

		DEFINE_FUNCTION_DEEP_CLONE_AS_PROP_VAL()

		DEFINE_FUNCTION_DEEP_CLONE_AS_INTERP_FUNC()

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
			visitor.visit_gpml_finite_rotation_slerp(*this);
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
			visitor.visit_gpml_finite_rotation_slerp(*this);
		}

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		GpmlFiniteRotationSlerp(
				const StructuralType &value_type_):
			GpmlInterpolationFunction(value_type_)
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GpmlFiniteRotationSlerp(
				const GpmlFiniteRotationSlerp &other) :
			GpmlInterpolationFunction(other)
		{  }

	private:

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GpmlFiniteRotationSlerp &
		operator=(const GpmlFiniteRotationSlerp &);

	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLFINITEROTATIONSLERP_H
