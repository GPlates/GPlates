/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_GMLLINESTRING_H
#define GPLATES_MODEL_GMLLINESTRING_H

#include <vector>
#include "PropertyValue.h"


// Forward declaration for intrusive-pointer.
// Note that, because we want to avoid the inclusion of "maths/PolylineOnSphere.h" into this header
// file (which is why we have this forward declaration), the constructors and destructor of
// GmlLineString cannot be defined in this header file, since the constructors and destructor
// require the ability to invoke 'intrusive_ptr_add_ref' and 'intrusive_ptr_release', which are
// only defined in "maths/PolylineOnSphere.h".
namespace GPlatesMaths {

	class PolylineOnSphere;

	void
	intrusive_ptr_add_ref(
			const PolylineOnSphere *p);

	void
	intrusive_ptr_release(
			const PolylineOnSphere *p);

}

namespace GPlatesModel {

	class GmlLineString :
			public PropertyValue {

	public:

		virtual
		~GmlLineString()
		{  }

		// This creation function is here purely for the simple, hard-coded construction of
		// features.  It may not be necessary or appropriate later on when we're doing
		// everything properly, so don't look at this function and think "Uh oh, this
		// function doesn't look like it should be here, but I'm sure it's here for a
		// reason..."
		static
		boost::intrusive_ptr<GmlLineString>
		create(
				const std::vector<double> &gml_pos_list);

		virtual
		boost::intrusive_ptr<PropertyValue>
		clone() const {
			boost::intrusive_ptr<PropertyValue> dup(new GmlLineString(*this));
			return dup;
		}

		boost::intrusive_ptr<const GPlatesMaths::PolylineOnSphere>
		polyline() const {
			return d_polyline;
		}

		void
		set_polyline(
				boost::intrusive_ptr<GPlatesMaths::PolylineOnSphere> p) {
			d_polyline = p;
		}

		virtual
		void
		accept_visitor(
				ConstFeatureVisitor &visitor) const {
			visitor.visit_gml_line_string(*this);
		}

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		GmlLineString(
				boost::intrusive_ptr<GPlatesMaths::PolylineOnSphere> polyline_):
			PropertyValue(),
			d_polyline(polyline_)
		{  }


		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GmlLineString(
				const GmlLineString &other):
			PropertyValue(),
			d_polyline(other.d_polyline)
		{  }

	private:

		boost::intrusive_ptr<GPlatesMaths::PolylineOnSphere> d_polyline;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GmlLineString &
		operator=(
				const GmlLineString &);

	};

}

#endif  // GPLATES_MODEL_GMLLINESTRING_H
