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
 
#ifndef GPLATES_MATHS_POLYGONPROXIMITYHITDETAIL_H
#define GPLATES_MATHS_POLYGONPROXIMITYHITDETAIL_H

#include "ProximityHitDetail.h"
#include "ProximityHitDetailVisitor.h"
#include "PolygonOnSphere.h"


namespace GPlatesMaths
{
	/**
	 * This contains information about a proximity hit which hit a polygon.
	 *
	 * There is no extra information about whether the proximity hit was on a vertex or a
	 * segment, or on the interior of the polygon.
	 */
	class PolygonProximityHitDetail:
			public ProximityHitDetail
	{
	public:
		static
		const ProximityHitDetail::non_null_ptr_type
		create(
				PolygonOnSphere::non_null_ptr_to_const_type polygon_,
				const double &closeness_)
		{
			ProximityHitDetail::non_null_ptr_type ptr(
					new PolygonProximityHitDetail(polygon_, closeness_),
					GPlatesUtils::NullIntrusivePointerHandler());
			return ptr;
		}

		virtual
		~PolygonProximityHitDetail()
		{  }

		virtual
		void
		accept_visitor(
				ProximityHitDetailVisitor &visitor)
		{
			visitor.visit_polygon_proximity_hit_detail(*this);
		}

	private:
		PolygonOnSphere::non_null_ptr_to_const_type d_polygon;

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		PolygonProximityHitDetail(
				PolygonOnSphere::non_null_ptr_to_const_type polygon_,
				const double &closeness_):
			ProximityHitDetail(closeness_),
			d_polygon(polygon_)
		{  }

		// This constructor should never be defined, because we don't want/need to allow
		// copy-construction.
		PolygonProximityHitDetail(
				const PolygonProximityHitDetail &);

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  There shouldn't be any copying; and all "assignment" should
		// really only be assignment of one intrusive_ptr to another.
		PolygonProximityHitDetail &
		operator=(
				const PolygonProximityHitDetail &);
	};

}

#endif  // GPLATES_MATHS_POLYGONPROXIMITYHITDETAIL_H
