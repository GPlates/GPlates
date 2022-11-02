/* $Id$ */

/**
 * @file 
 *
 * Most recent change:
 *   $Author$
 *   $Date$
 *
 * Copyright (C) 2005, 2006, 2008 The University of Sydney, Australia
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

#ifndef GPLATES_MATHS_POINTLIESONGREATCIRCLEARC_H
#define GPLATES_MATHS_POINTLIESONGREATCIRCLEARC_H

#include "GreatCircleArc.h"

namespace GPlatesMaths
{
	/**
	 * This class instantiates to a function object which determines
	 * whether a point lies on a given GreatCircleArc.
	 *
	 * This is particularly useful when used in conjunction with STL
	 * algorithms.  For example:
	 * @code
	 *   std::list< PointOnSphere > point_list = ...;
	 *   GreatCircleArc test_arc = ...;
	 *
	 *   PointIsOnGreatCircleArc point_is_on_arc(test_arc);
	 *   point_list.remove_if(
	 *       [&](const PointOnSphere &point) { return !point_is_on_arc(point); });
	 * @endcode
	 * This snippet of code removes all the points in @a point_list which
	 * are not on the @a test_arc.
	 */
	class PointLiesOnGreatCircleArc
	{

	  public:

		/**
		 * Instantiate a function object which determines whether a
		 * given point lies on @a arc.
		 */
		explicit
		PointLiesOnGreatCircleArc(
				const GreatCircleArc &arc) :
			d_arc_start(arc.start_point()),
			d_arc_end(arc.end_point())
		{
			if (!arc.is_zero_length())
			{
				d_arc_normal = arc.rotation_axis();
			}
		}

		/**
		 * Test whether @a test_point lies on the arc supplied to the
		 * constructor.
		 */
		bool
		operator()(
				const PointOnSphere &test_point) const;

	  private:

		/**
		 * The start-point of the arc.
		 */
		const PointOnSphere d_arc_start;

		/**
		 * The end-point of the arc.
		 */
		const PointOnSphere d_arc_end;

		/**
		 * The normal to the plane which contains the arc.
		 *
		 * Note that this might be an undefined boost::optional.
		 */
		boost::optional<UnitVector3D> d_arc_normal;

	 };

}

#endif  // GPLATES_MATHS_POINTLIESONGREATCIRCLEARC_H
