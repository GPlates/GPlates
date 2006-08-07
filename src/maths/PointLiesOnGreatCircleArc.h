/* $Id$ */

/**
 * @file 
 *
 * Most recent change:
 *   $Author$
 *   $Date$
 *
 * Copyright (C) 2003 The GPlates Consortium
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

#ifndef GPLATES_MATHS_POINTLIESONGREATCIRCLEARC_H
#define GPLATES_MATHS_POINTLIESONGREATCIRCLEARC_H

#include <functional>  /* std::unary_function */
#include "GreatCircleArc.h"

namespace GPlatesMaths {

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
	 *   point_list.remove_if(
	 *    std::not1(
	 *     PointIsOnGreatCircleArc(test_arc)));
	 * @endcode
	 * This snippet of code removes all the points in @a point_list which
	 * are not on the @a test_arc.
	 *
	 * See Josuttis99, Chapter 8 "STL Function Objects", and in particular
	 * section 8.2.4 "User-Defined Function Objects for Function Adapters",
	 * for more information about @a std::unary_function.
	 */
	class PointLiesOnGreatCircleArc:
	 public std::unary_function< PointOnSphere, bool > {

	  public:

		/**
		 * Instantiate a function object which determines whether a
		 * given point lies on @a arc.
		 */
		explicit
		PointLiesOnGreatCircleArc(
		 const GreatCircleArc &arc) :
		 d_arc_start(arc.start_point()),
		 d_arc_normal(arc.rotation_axis()),
		 d_arc_dot(arc.dot_of_endpoints()) {  }

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
		 * The normal to the plane which contains the arc.
		 */
		const UnitVector3D d_arc_normal;

		/**
		 * The dot-product of the unit-vectors of the start-point and
		 * end-point of the arc.  Since the angle of the arc must lie
		 * in the open-range (0, PI), the value of the dot-product will
		 * lie in the range (-1, 1).
		 */
		const real_t d_arc_dot;

	 };

}

#endif  // GPLATES_MATHS_POINTLIESONGREATCIRCLEARC_H
