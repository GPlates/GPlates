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

#ifndef GPLATES_MODEL_GMLTIMEPERIOD_H
#define GPLATES_MODEL_GMLTIMEPERIOD_H

#include "PropertyValue.h"
#include "GmlTimeInstant.h"


namespace GPlatesModel {

	class GmlTimePeriod :
			public PropertyValue {

	public:
		/**
		 * A convenience typedef for GPlatesContrib::non_null_intrusive_ptr<GmlTimePeriod>.
		 */
		typedef GPlatesContrib::non_null_intrusive_ptr<GmlTimePeriod> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesContrib::non_null_intrusive_ptr<const GmlTimePeriod>.
		 */
		typedef GPlatesContrib::non_null_intrusive_ptr<const GmlTimePeriod>
				non_null_ptr_to_const_type;


		virtual
		~GmlTimePeriod() {  }

		// This creation function is here purely for the simple, hard-coded construction of
		// features.  It may not be necessary or appropriate later on when we're doing
		// everything properly, so don't look at this function and think "Uh oh, this
		// function doesn't look like it should be here, but I'm sure it's here for a
		// reason..."
		static
		const non_null_ptr_type
		create(
				GmlTimeInstant::non_null_ptr_type begin_,
				GmlTimeInstant::non_null_ptr_type end_) {
			GmlTimePeriod::non_null_ptr_type ptr(*(new GmlTimePeriod(begin_, end_)));
			return ptr;
		}

		virtual
		const PropertyValue::non_null_ptr_type
		clone() const {
			PropertyValue::non_null_ptr_type dup(*(new GmlTimePeriod(*this)));
			return dup;
		}

		const GmlTimeInstant::non_null_ptr_to_const_type
		begin() const {
			return d_begin;
		}

		// Note that, because the copy-assignment operator of GmlTimeInstant is private,
		// the GmlTimeInstant referenced by the return-value of this function cannot be
		// assigned-to, which means that this function does not provide a means to directly
		// switch the GmlTimeInstant within this GmlTimePeriod instance.  (This
		// restriction is intentional.)
		//
		// To switch the GmlTimeInstant within this GmlTimePeriod instance, use the
		// function @a set_begin below.
		//
		// (This overload is provided to allow the referenced GmlTimeInstant instance to
		// accept a FeatureVisitor instance.)
		const GmlTimeInstant::non_null_ptr_type
		begin() {
			return d_begin;
		}

		void
		set_begin(
				GmlTimeInstant::non_null_ptr_type b) {
			d_begin = b;
		}

		const GmlTimeInstant::non_null_ptr_to_const_type
		end() const {
			return d_end;
		}

		// Note that, because the copy-assignment operator of GmlTimeInstant is private,
		// the GmlTimeInstant referenced by the return-value of this function cannot be
		// assigned-to, which means that this function does not provide a means to directly
		// switch the GmlTimeInstant within this GmlTimePeriod instance.  (This
		// restriction is intentional.)
		//
		// To switch the GmlTimeInstant within this GmlTimePeriod instance, use the
		// function @a set_end below.
		//
		// (This overload is provided to allow the referenced GmlTimeInstant instance to
		// accept a FeatureVisitor instance.)
		const GmlTimeInstant::non_null_ptr_type
		end() {
			return d_end;
		}

		void
		set_end(
				GmlTimeInstant::non_null_ptr_type e) {
			d_end = e;
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
				ConstFeatureVisitor &visitor) const {
			visitor.visit_gml_time_period(*this);
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
				FeatureVisitor &visitor) {
			visitor.visit_gml_time_period(*this);
		}

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GmlTimePeriod(
				GmlTimeInstant::non_null_ptr_type begin_,
				GmlTimeInstant::non_null_ptr_type end_):
			PropertyValue(),
			d_begin(begin_),
			d_end(end_)
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GmlTimePeriod(
				const GmlTimePeriod &other) :
			PropertyValue(),
			d_begin(other.d_begin),
			d_end(other.d_end)
		{  }

	private:

		GmlTimeInstant::non_null_ptr_type d_begin;
		GmlTimeInstant::non_null_ptr_type d_end;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GmlTimePeriod &
		operator=(const GmlTimePeriod &);

	};

}

#endif  // GPLATES_MODEL_GMLTIMEPERIOD_H
