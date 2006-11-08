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

#ifndef GPLATES_MODEL_GMLTIMEINSTANT_H
#define GPLATES_MODEL_GMLTIMEINSTANT_H

#include <boost/intrusive_ptr.hpp>
#include "PropertyValue.h"
#include "GeoTimeInstant.h"
#include "ConstFeatureVisitor.h"


namespace GPlatesModel {

	class GmlTimeInstant :
			public PropertyValue {

	public:

		virtual
		~GmlTimeInstant() {  }

		// This creation function is here purely for the simple, hard-coded construction of
		// features.  It may not be necessary or appropriate later on when we're doing
		// everything properly, so don't look at this function and think "Uh oh, this
		// function doesn't look like it should be here, but I'm sure it's here for a
		// reason..."
		static
		boost::intrusive_ptr<GmlTimeInstant>
		create(
				const GeoTimeInstant &geo_time_instant) {
			boost::intrusive_ptr<GmlTimeInstant> ptr(new GmlTimeInstant(geo_time_instant));
			return ptr;
		}

		virtual
		boost::intrusive_ptr<PropertyValue>
		clone() const {
			boost::intrusive_ptr<PropertyValue> dup(new GmlTimeInstant(*this));
			return dup;
		}

		virtual
		void
		accept_visitor(
				ConstFeatureVisitor &visitor) const {
			visitor.visit_gml_time_instant(*this);
		}

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		GmlTimeInstant(
				const GeoTimeInstant &geo_time_instant):
			PropertyValue(),
			d_geo_time_instant(geo_time_instant)
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GmlTimeInstant(
				const GmlTimeInstant &other) :
			PropertyValue(),
			d_geo_time_instant(other.d_geo_time_instant)
		{  }

	private:

		GeoTimeInstant d_geo_time_instant;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GmlTimeInstant &
		operator=(const GmlTimeInstant &);

	};

}

#endif  // GPLATES_MODEL_GMLTIMEINSTANT_H
