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
 
#ifndef GPLATES_MATHS_MULTIPOINTPROXIMITYHITDETAIL_H
#define GPLATES_MATHS_MULTIPOINTPROXIMITYHITDETAIL_H

#include "ProximityHitDetail.h"
#include "ProximityHitDetailVisitor.h"
#include "MultiPointOnSphere.h"


namespace GPlatesMaths
{
	/**
	 * This contains information about a proximity hit which hit a multi-point.
	 *
	 * There is no extra information about which point was hit.
	 */
	class MultiPointProximityHitDetail:
			public ProximityHitDetail
	{
	public:
		static
		const ProximityHitDetail::non_null_ptr_type
		create(
				MultiPointOnSphere::non_null_ptr_to_const_type multi_point_,
				const double &closeness_)
		{
			ProximityHitDetail::non_null_ptr_type ptr(
					new MultiPointProximityHitDetail(multi_point_, closeness_),
					GPlatesUtils::NullIntrusivePointerHandler());
			return ptr;
		}

		virtual
		~MultiPointProximityHitDetail()
		{  }

		virtual
		void
		accept_visitor(
				ProximityHitDetailVisitor &visitor)
		{
			visitor.visit_multi_point_proximity_hit_detail(*this);
		}

	private:
		MultiPointOnSphere::non_null_ptr_to_const_type d_multi_point;

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		MultiPointProximityHitDetail(
				MultiPointOnSphere::non_null_ptr_to_const_type multi_point_,
				const double &closeness_):
			ProximityHitDetail(closeness_),
			d_multi_point(multi_point_)
		{  }

		// This constructor should never be defined, because we don't want/need to allow
		// copy-construction.
		MultiPointProximityHitDetail(
				const MultiPointProximityHitDetail &);

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  There shouldn't be any copying; and all "assignment" should
		// really only be assignment of one intrusive_ptr to another.
		MultiPointProximityHitDetail &
		operator=(
				const MultiPointProximityHitDetail &);
	};

}

#endif  // GPLATES_MATHS_MULTIPOINTPROXIMITYHITDETAIL_H
