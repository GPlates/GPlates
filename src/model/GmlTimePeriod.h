/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef GPLATES_MODEL_GMLTIMEPERIOD_H
#define GPLATES_MODEL_GMLTIMEPERIOD_H

#include <boost/intrusive_ptr.hpp>
#include "PropertyValue.h"
#include "GmlTimeInstant.h"


namespace GPlatesModel {

	class GmlTimePeriod :
			public PropertyValue {

	public:

		virtual
		~GmlTimePeriod() {  }

		// This creation function is here purely for the simple, hard-coded construction of
		// features.  It may not be necessary or appropriate later on when we're doing
		// everything properly, so don't look at this function and think "Uh oh, this
		// function doesn't look like it should be here, but I'm sure it's here for a
		// reason..."
		static
		boost::intrusive_ptr<GmlTimePeriod>
		create(
				boost::intrusive_ptr<GmlTimeInstant> begin,
				boost::intrusive_ptr<GmlTimeInstant> end) {
			boost::intrusive_ptr<GmlTimePeriod> ptr(new GmlTimePeriod(begin, end));
			return ptr;
		}

		virtual
		boost::intrusive_ptr<PropertyValue>
		clone() const {
			boost::intrusive_ptr<PropertyValue> dup(new GmlTimePeriod(*this));
			return dup;
		}

		// FIXME: visitor accept method

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		GmlTimePeriod(
				boost::intrusive_ptr<GmlTimeInstant> begin,
				boost::intrusive_ptr<GmlTimeInstant> end):
			PropertyValue(),
			d_begin(begin),
			d_end(end)
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

		boost::intrusive_ptr<GmlTimeInstant> d_begin;
		boost::intrusive_ptr<GmlTimeInstant> d_end;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GmlTimePeriod &
		operator=(const GmlTimePeriod &);

	};

}

#endif  // GPLATES_MODEL_GMLTIMEPERIOD_H
