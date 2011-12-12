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
 
#ifndef GPLATES_MATHS_PROXIMITYHITDETAILVISITOR_H
#define GPLATES_MATHS_PROXIMITYHITDETAILVISITOR_H

#include <boost/intrusive_ptr.hpp>


namespace GPlatesMaths
{
	// Forward declarations for the member functions.
	// Please keep these ordered alphabetically.
	class MultiPointProximityHitDetail;
	class PointProximityHitDetail;
	class PolygonProximityHitDetail;
	class PolylineProximityHitDetail;
	class SmallCircleProximityHitDetail;


	/**
	 * This class defines an abstract interface for a Visitor to visit ProximityHitDetail
	 * instances.
	 *
	 * See the Visitor pattern (p.331) in Gamma95 for more information on the design and
	 * operation of this class.  This class corresponds to the abstract Visitor class in the
	 * pattern structure.
	 *
	 * @par Notes on the class implementation:
	 *  - All the virtual member "visit" functions have (empty) definitions for convenience, so
	 *    that derivations of this abstract base need only override the "visit" functions which
	 *    interest them.
	 *  - The "visit" functions explicitly include the name of the target type in the function
	 *    name, to avoid the problem of name hiding in derived classes.  (If you declare *any*
	 *    functions named 'f' in a derived class, the declaration will hide *all* functions
	 *    named 'f' in the base class; if all the "visit" functions were simply called 'visit'
	 *    and you wanted to override *any* of them in a derived class, you would have to
	 *    override them all.)
	 */
	class ProximityHitDetailVisitor
	{
	public:
		// We'll make this function pure virtual so that the class is abstract.  The class
		// *should* be abstract, but wouldn't be unless we did this, since all the virtual
		// member functions have (empty) definitions.
		virtual
		~ProximityHitDetailVisitor() = 0;

		// Please keep these proximity hit detail types ordered alphabetically.

		virtual
		void
		visit_multi_point_proximity_hit_detail(
				MultiPointProximityHitDetail &multi_point_detail)
		{  }

		virtual
		void
		visit_point_proximity_hit_detail(
				PointProximityHitDetail &point_detail)
		{  }

		virtual
		void
		visit_polygon_proximity_hit_detail(
				PolygonProximityHitDetail &polygon_detail)
		{  }

		virtual
		void
		visit_polyline_proximity_hit_detail(
				PolylineProximityHitDetail &polyline_detail)
		{  }

		virtual
		void
		visit_small_circle_proximity_hit_detail(
				SmallCircleProximityHitDetail &small_circle_detail)
		{  }

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment.
		ProximityHitDetailVisitor &
		operator=(
				const ProximityHitDetailVisitor &);
	};

}

#endif  // GPLATES_MATHS_PROXIMITYHITDETAILVISITOR_H
