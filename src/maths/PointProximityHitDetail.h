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
 
#ifndef GPLATES_MATHS_POINTPROXIMITYHITDETAIL_H
#define GPLATES_MATHS_POINTPROXIMITYHITDETAIL_H

#include "ProximityHitDetail.h"
#include "ProximityHitDetailVisitor.h"
#include "PointOnSphere.h"


namespace GPlatesMaths
{
	/**
	 * This contains information about a proximity hit which hit a point.
	 */
	class PointProximityHitDetail:
			public ProximityHitDetail
	{
	public:
		static
		const ProximityHitDetail::non_null_ptr_type
		create(
				const PointOnSphere::non_null_ptr_to_const_type point_,
				const double &closeness_)
		{
			ProximityHitDetail::non_null_ptr_type ptr(
					*(new PointProximityHitDetail(point_, closeness_)));
			return ptr;
		}

		virtual
		~PointProximityHitDetail()
		{  }

		virtual
		void
		accept_visitor(
				ProximityHitDetailVisitor &visitor)
		{
			visitor.visit_point_proximity_hit_detail(*this);
		}

	private:
		PointOnSphere::non_null_ptr_to_const_type d_point;

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		PointProximityHitDetail(
				const PointOnSphere::non_null_ptr_to_const_type point_,
				const double &closeness_):
			ProximityHitDetail(closeness_),
			d_point(point_)
		{  }

		// This constructor should never be defined, because we don't want/need to allow
		// copy-construction.
		PointProximityHitDetail(
				const PointProximityHitDetail &);

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  There shouldn't be any copying; and all "assignment" should
		// really only be assignment of one intrusive_ptr to another.
		PointProximityHitDetail &
		operator=(
				const PointProximityHitDetail &);
	};

}

#endif  // GPLATES_MATHS_POINTPROXIMITYHITDETAIL_H
