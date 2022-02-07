/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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
 
#ifndef GPLATES_MATHS_POLYLINEPROXIMITYHITDETAIL_H
#define GPLATES_MATHS_POLYLINEPROXIMITYHITDETAIL_H

#include "ProximityHitDetail.h"
#include "ProximityHitDetailVisitor.h"
#include "PolylineOnSphere.h"


namespace GPlatesMaths
{
	/**
	 * This contains information about a proximity hit which hit a polyline.
	 *
	 * There is no extra information about whether the proximity hit was on a vertex or a
	 * segment.
	 */
	class PolylineProximityHitDetail:
			public ProximityHitDetail
	{
	public:
		static
		const ProximityHitDetail::non_null_ptr_type
		create(
				PolylineOnSphere::non_null_ptr_to_const_type polyline_,
				const double &closeness_,
				const boost::optional<unsigned int> &index_ = boost::none)
		{
			ProximityHitDetail::non_null_ptr_type ptr(
					new PolylineProximityHitDetail(polyline_, closeness_,index_),
					GPlatesUtils::NullIntrusivePointerHandler());
			return ptr;
		}

		virtual
		~PolylineProximityHitDetail()
		{  }

		virtual
		void
		accept_visitor(
				ProximityHitDetailVisitor &visitor)
		{
			visitor.visit_polyline_proximity_hit_detail(*this);
		}

	private:
		PolylineOnSphere::non_null_ptr_to_const_type d_polyline;

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		PolylineProximityHitDetail(
				PolylineOnSphere::non_null_ptr_to_const_type polyline_,
				const double &closeness_,
				const boost::optional<unsigned int> &index_):
			ProximityHitDetail(closeness_,index_),
			d_polyline(polyline_)
		{  }

		// This constructor should never be defined, because we don't want/need to allow
		// copy-construction.
		PolylineProximityHitDetail(
				const PolylineProximityHitDetail &);

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  There shouldn't be any copying; and all "assignment" should
		// really only be assignment of one intrusive_ptr to another.
		PolylineProximityHitDetail &
		operator=(
				const PolylineProximityHitDetail &);
	};

}

#endif  // GPLATES_MATHS_POLYLINEPROXIMITYHITDETAIL_H
