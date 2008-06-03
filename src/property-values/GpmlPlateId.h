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

#ifndef GPLATES_PROPERTYVALUES_GPMLPLATEID_H
#define GPLATES_PROPERTYVALUES_GPMLPLATEID_H

#include "model/PropertyValue.h"
#include "model/types.h"


namespace GPlatesPropertyValues {

	class GpmlPlateId :
			public GPlatesModel::PropertyValue {

	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GpmlPlateId>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlPlateId> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const GpmlPlateId>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlPlateId>
				non_null_ptr_to_const_type;

		virtual
		~GpmlPlateId() {  }

		// This creation function is here purely for the simple, hard-coded construction of
		// features.  It may not be necessary or appropriate later on when we're doing
		// everything properly, so don't look at this function and think "Uh oh, this
		// function doesn't look like it should be here, but I'm sure it's here for a
		// reason..."
		static
		const non_null_ptr_type
		create(
				const GPlatesModel::integer_plate_id_type &value_) {
			non_null_ptr_type ptr(*(new GpmlPlateId(value_)));
			return ptr;
		}

		virtual
		const GPlatesModel::PropertyValue::non_null_ptr_type
		clone() const {
			GPlatesModel::PropertyValue::non_null_ptr_type dup(*(new GpmlPlateId(*this)));
			return dup;
		}

		/**
		 * Access the integer_plate_id_type contained within this GpmlPlateId.
		 *
		 * Note that this does not allow you to directly modify the plate id
		 * inside this GpmlPlateId. For that, you should use @a set_value.
		 */
		const GPlatesModel::integer_plate_id_type &
		value() const {
			return d_value;
		}
		
		/**
		 * Set the plate id contained within this GpmlPlateId to @a p.
		 *
		 * FIXME: when we have undo/redo, this act should cause
		 * a new revision to be propagated up to the Feature which
		 * contains this PropertyValue.
		 */
		void
		set_value(
				const GPlatesModel::integer_plate_id_type &p)
		{
			d_value = p;
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
			visitor.visit_gpml_plate_id(*this);
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
			visitor.visit_gpml_plate_id(*this);
		}

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		GpmlPlateId(
				const GPlatesModel::integer_plate_id_type &value_):
			PropertyValue(),
			d_value(value_)
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GpmlPlateId(
				const GpmlPlateId &other) :
			PropertyValue(),
			d_value(other.d_value)
		{  }

	private:

		GPlatesModel::integer_plate_id_type d_value;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GpmlPlateId &
		operator=(const GpmlPlateId &);

	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLPLATEID_H
