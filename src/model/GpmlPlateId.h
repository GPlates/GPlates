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

#ifndef GPLATES_MODEL_GPMLPLATEID_H
#define GPLATES_MODEL_GPMLPLATEID_H

#include <boost/intrusive_ptr.hpp>
#include "PropertyValue.h"
#include "PlateId.h"


namespace GPlatesModel {

	class GpmlPlateId :
			public PropertyValue {

	public:

		virtual
		~GpmlPlateId() {  }

		static
		boost::intrusive_ptr<GpmlPlateId>
		create(
				const PlateId &plate_id) {
			boost::intrusive_ptr<GpmlPlateId> ptr(new GpmlPlateId(plate_id));
			return ptr;
		}

		virtual
		boost::intrusive_ptr<PropertyValue>
		clone() const {
			boost::intrusive_ptr<PropertyValue> dup(new GpmlPlateId(*this));
			return dup;
		}

		// FIXME: visitor accept method

	protected:

		// This operator should not be public, because we don't want to allow instantiation
		// of this type on the stack.
		explicit
		GpmlPlateId(
				const PlateId &plate_id):
			PropertyValue(),
			d_plate_id(plate_id)
		{  }

	private:

		PlateId d_plate_id;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GpmlPlateId &
		operator=(const GpmlPlateId &);

	};

}

#endif  // GPLATES_MODEL_GPMLPLATEID_H
