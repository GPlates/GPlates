/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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


namespace GPlatesPropertyValues {

	class GpmlFiniteRotationSlerp :
			public GpmlInterpolationFunction {

	public:

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<GpmlFiniteRotationSlerp>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlFiniteRotationSlerp>
				non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const GpmlFiniteRotationSlerp>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlFiniteRotationSlerp>
				non_null_ptr_to_const_type;

		virtual
		~GpmlFiniteRotationSlerp() {  }

		// This creation function is here purely for the simple, hard-coded construction of
		// features.  It may not be necessary or appropriate later on when we're doing
		// everything properly, so don't look at this function and think "Uh oh, this
		// function doesn't look like it should be here, but I'm sure it's here for a
		// reason..."
		static
		const non_null_ptr_type
		create(
				const TemplateTypeParameterType &value_type_) {
			non_null_ptr_type ptr(*(new GpmlFiniteRotationSlerp(value_type_)));
			return ptr;
		}

		virtual
		const GPlatesModel::PropertyValue::non_null_ptr_type
		clone() const {
			GPlatesModel::PropertyValue::non_null_ptr_type dup(*(new GpmlFiniteRotationSlerp(*this)));
			return dup;
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
				GPlatesModel::ConstFeatureVisitor &visitor) const {
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
				GPlatesModel::FeatureVisitor &visitor) {
			visitor.visit_gpml_finite_rotation_slerp(*this);
		}

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		GpmlFiniteRotationSlerp(
				const TemplateTypeParameterType &value_type_):
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
