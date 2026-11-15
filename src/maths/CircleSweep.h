/* $Id$ */

/**
 * \file
 * $Revision$
 * $Date$
 *
 * Copyright (C) 2026 The University of Sydney, Australia
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

#ifndef GPLATES_MATHS_CIRCLESWEEP_H
#define GPLATES_MATHS_CIRCLESWEEP_H

#include <utility>
#include <vector>

#include "UnitVector3D.h"


namespace GPlatesMaths
{
	/**
	 * The pairs of circles on the sphere that meet, visited in the order of a sweep along an
	 * axis (the polygon overlay pairs its rings by their bounding circles with it: the grouping,
	 * the containment test and the touch search, spherical-geometry.md section 5.2).
	 *
	 * The order is that of a sweep of the circles' intervals of angle from one axis (each a
	 * circle's centre's angle plus and minus its radius): by the beginnings of the first circle's
	 * interval and then the second's, the second's interval beginning within the first's. The axis
	 * is whichever of x, y and z gives the fewest such pairs, so that rings along a meridian or a
	 * parallel spread out along it. Of those pairs only the ones whose circles meet, within a
	 * slack far below the overlay's thickness, are visited. So every pair whose circles meet
	 * without the slack is visited, since such circles overlap on every axis (two points' angles
	 * from an axis differ by no more than the angle between them).
	 *
	 * The pairs are found once, when constructed, and stored: their number, not the circles',
	 * decides the memory (as many as the square of the circles where nearly all meet).
	 *
	 * The order is part of the result: the overlay's touch search keeps the first of two touches
	 * at almost one point, as the pairs found them. So the pairs are visited in the sweep's order
	 * however they are found.
	 *
	 * Where the sweep has few pairs per circle, they are its own. A sweep along one axis, though,
	 * pairs every circle with every other in a band across the area they spread over: about
	 * n^1.5 pairs for n circles on a grid, most of whose circles do not meet. Beyond a number of
	 * pairs per circle, those that meet are found instead by a tree of the circles joined with
	 * itself, in time in proportion to n log n and to their number, and sorted into the sweep's
	 * order (which adds the logarithm of their number).
	 */
	class CircleSweep
	{
	public:

		//! A circle: its centre, and its radius in radians (with any margin added).
		class Circle
		{
		public:
			UnitVector3D centre;
			double radius;
		};


		/**
		 * The default for @a max_sweep_pairs_per_circle: about where building and joining the
		 * tree starts to cost less than the sweep's own pairs (measured on the overlay's cases).
		 */
		static const unsigned int DEFAULT_MAX_SWEEP_PAIRS_PER_CIRCLE = 64;


		/**
		 * The pairs of @a circles that meet. While the sweep has at most
		 * @a max_sweep_pairs_per_circle pairs per circle (of circles whose intervals overlap,
		 * whether they meet or not), they are found by the sweep itself, whose pairs are already
		 * in order; beyond it, by the tree. Either way the pairs visited are the same.
		 */
		explicit
		CircleSweep(
				const std::vector<Circle> &circles,
				unsigned int max_sweep_pairs_per_circle = DEFAULT_MAX_SWEEP_PAIRS_PER_CIRCLE);


		/**
		 * Calls @a visit(a, b), with the indices of two circles, for each pair of circles that
		 * meet, once, in the sweep's order (see the class comment); @a a is the circle whose
		 * interval begins first.
		 */
		template <typename VisitType>
		void
		visit_pairs(
				VisitType visit) const
		{
			for (const std::pair<unsigned int, unsigned int> &pair : d_pairs)
			{
				visit(pair.first, pair.second);
			}
		}


		/**
		 * The slack, in radians, within which two circles are taken to meet: their centres are at
		 * most the sum of their radii and this apart. Far above the rounding of the angles, far
		 * below the overlay's thickness.
		 */
		static
		double
		get_meeting_slack()
		{
			return 1e-9;
		}

	private:

		//! The pairs of circles that meet, in the sweep's order.
		std::vector<std::pair<unsigned int, unsigned int>> d_pairs;
	};
}

#endif // GPLATES_MATHS_CIRCLESWEEP_H
