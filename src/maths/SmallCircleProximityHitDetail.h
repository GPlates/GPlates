/* $Id: PointProximityHitDetail.h 8599 2010-06-02 15:04:27Z rwatson $ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2010-06-02 17:04:27 +0200 (on, 02 jun 2010) $
 * 
 * Copyright (C) 2011 Geological Survey of Norway
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
 
#ifndef GPLATES_MATHS_SMALLCIRCLEPROXIMITYHITDETAIL_H
#define GPLATES_MATHS_SMALLCIRCLEPROXIMITYHITDETAIL_H

#include "ProximityHitDetail.h"
#include "ProximityHitDetailVisitor.h"
#include "PointOnSphere.h"


namespace GPlatesMaths
{
	/**
	 * This contains information about a proximity hit which hit a point.
	 */
	class SmallCircleProximityHitDetail:
			public ProximityHitDetail
	{
	public:

		// FIXME: we should probably add some sort of reference to the small circle in the constructor. 
		static
		const ProximityHitDetail::non_null_ptr_type
		create(
				const double &closeness_)
		{
			ProximityHitDetail::non_null_ptr_type ptr(
					new SmallCircleProximityHitDetail(closeness_),
					GPlatesUtils::NullIntrusivePointerHandler());
			return ptr;
		}

		virtual
		~SmallCircleProximityHitDetail()
		{  }

		// FIXME: not sure if these visitors are required by any of the proximity-hit code at the moment.
		virtual
		void
		accept_visitor(
				ProximityHitDetailVisitor &visitor)
		{
			visitor.visit_small_circle_proximity_hit_detail(*this);
		}

	private:
	

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		SmallCircleProximityHitDetail(
			const double &closeness_):
			ProximityHitDetail(closeness_,boost::none)
		{  }

		// This constructor should never be defined, because we don't want/need to allow
		// copy-construction.
		SmallCircleProximityHitDetail(
				const SmallCircleProximityHitDetail &);

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  There shouldn't be any copying; and all "assignment" should
		// really only be assignment of one intrusive_ptr to another.
		SmallCircleProximityHitDetail &
		operator=(
				const SmallCircleProximityHitDetail &);
	};

}

#endif  // GPLATES_MATHS_SMALLCIRCLEPROXIMITYHITDETAIL_H
