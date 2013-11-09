/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#ifndef GPLATES_MATHS_ANGULARDISTANCE_H
#define GPLATES_MATHS_ANGULARDISTANCE_H

#include "MathsUtils.h"
#include "types.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


namespace GPlatesMaths
{
	/**
	 * An angular distance stored as cosine instead of the actual angle.
	 *
	 * This class is very similar to @a AngularExtent, but is more lightweight and doesn't
	 * support addition/subtraction and angular distances. This class, @a AngularDistance, is
	 * used when only angular distance comparisons are require and it is also the same size as a
	 * 'double' so it's useful for returning from distance calculations. @a AngularExtent is more
	 * useful as an angular threshold or angular bounds where the bounds can be extended/contracted
	 * (using addition/subtraction).
	 *
	 * All comparison operators (<, >, <=, >=, ==, !=) are supported.
	 *
	 * Note that, as with great circle arcs, the angular distance is limited to the range [0,PI].
	 * So that angular distance only covers up to half the globe (like great circle arcs).
	 *
	 * Use of cosine is more efficient in some situations such as comparing angular distances
	 * (between two unit vectors using a dot product - cosine). This can avoid calculating
	 * 'acos', which is generally slower to calculate (about 100 cycles on a circa 2011 CPU).
	 */
	class AngularDistance :
			// NOTE: Base class chaining is used to avoid increasing sizeof(AngularDistance) due to multiple
			// inheritance from several empty base classes - this reduces sizeof(AngularDistance) from 16 to 8...
			public boost::less_than_comparable<AngularDistance,
					boost::equivalent<AngularDistance,
					boost::equality_comparable<AngularDistance> > >
	{
	public:

		//! Angular distance of zero (radians).
		static const AngularDistance ZERO;

		//! Angular distance of PI/2 radians (90 degrees).
		static const AngularDistance HALF_PI;

		//! Angular distance of PI radians (180 degrees).
		static const AngularDistance PI;


		/**
		 * Create from the cosine of the angular distance.
		 *
		 * @a cosine_colatitude is the cosine of the "colatitude" of the small circle around the
		 * "North Pole" of its axis (from the small circle centre to the
		 * boundary of the small circle - the radius angle).
		 *
		 * Note that the cosine can be efficiently calculated as the dot product of two unit vectors.
		 */
		static
		AngularDistance
		create_from_cosine(
				const real_t &cosine_colatitude)
		{
			return AngularDistance(cosine_colatitude);
		}

		/**
		 * Create from an angular distance (radians) in the range [0, PI].
		 *
		 * The cosine will be calculated.
		 *
		 * @throws PreconditionViolationError exception if @a angle is not in range [0, PI].
		 */
		static
		AngularDistance
		create_from_angle(
				const real_t &colatitude)
		{
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					0 <= colatitude && colatitude <= GPlatesMaths::PI,
					GPLATES_ASSERTION_SOURCE);

			return create_from_cosine(cos(colatitude));
		}


		/**
		 * Returns the cosine of the angular distance (radians).
		 */
		const real_t &
		get_cosine() const
		{
			return d_cosine;
		}


		/**
		 * Calculates the angular distance (radians) from the cosine of the angular distance.
		 *
		 * Note: The angle is not cached internally and so must be calculated each time.
		 * This calculation can be relatively expensive (~100 cycles on a circa 2011 CPU) 
		 * which is the main reason for this class (to use cosine until/if angle is actually needed).
		 * The angle is not cached in order to keep this class the lightweight (about the same
		 * size as a 'double').
		 */
		real_t
		calculate_angle() const
		{
			return acos(d_cosine);
		}


		/**
		 * Less than operator - all other operators (<=, >, >=, ==, !=) provided by
		 * boost::less_than_comparable, boost::equivalent and boost::equality_comparable.
		 *
		 * This comparison can be done cheaply using cosines as opposed to using inverse cosine (acos)
		 * to get the angles (inverse cosine is quite expensive even on modern CPUs).
		 * So instead of testing...
		 *
		 * angular_extent_1 < angular_extent_2
		 *
		 * ...we can test...
		 *
		 * cos(angular_extent_1) > cos(angular_extent_2)
		 *
		 * Whereas using angles would require calculating:
		 *
		 * angular_extent = acos(dot(start_point_angular_extent, end_point_angular_extent))
		 *
		 * Note that 'dot' is significantly cheaper than 'acos'.
		 *
		 */
		bool
		operator<(
				const AngularDistance &rhs) const
		{
			// NOTE: We're using 'real_t' which does epsilon test required for boost::equivalent to work.
			// Also note reversal of comparison since comparing cosine(angle) instead of angle.
			return d_cosine > rhs.d_cosine;
		}


		/**
		 * Similar to 'operator<' except does not have an epsilon test.
		 *
		 * AngularExtentOrDistance can be @a AngularExtent or @a AngularDistance.
		 */
		template <class AngularExtentOrDistance>
		bool
		is_precisely_less_than(
				const AngularExtentOrDistance &rhs) const
		{
			// Also note reversal of comparison since comparing cosine(angle) instead of angle.
			return d_cosine.dval() > rhs.get_cosine().dval();
		}

		/**
		 * Similar to 'operator>' except does not have an epsilon test.
		 *
		 * AngularExtentOrDistance can be @a AngularExtent or @a AngularDistance.
		 */
		template <class AngularExtentOrDistance>
		bool
		is_precisely_greater_than(
				const AngularExtentOrDistance &rhs) const
		{
			// Also note reversal of comparison since comparing cosine(angle) instead of angle.
			return d_cosine.dval() < rhs.get_cosine().dval();
		}

	private:

		real_t d_cosine;


		explicit
		AngularDistance(
				const real_t &cosine) :
			d_cosine(cosine)
		{  }
	};
}

#endif // GPLATES_MATHS_ANGULARDISTANCE_H
