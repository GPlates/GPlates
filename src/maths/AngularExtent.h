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

#ifndef GPLATES_MATHS_ANGULAREXTENT_H
#define GPLATES_MATHS_ANGULAREXTENT_H

#include <boost/operators.hpp>
#include <boost/optional.hpp>

#include "AngularDistance.h"
#include "MathsUtils.h"
#include "types.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


namespace GPlatesMaths
{
	/**
	 * An angular extent stored as cosine and sine instead of the actual angle.
	 *
	 * All comparison operators (<, >, <=, >=, ==, !=) are supported.
	 * In addition all comparison operators (<, >, <=, >=, ==, !=) with @a AngularDistance are supported.
	 *
	 * Note that, as with great circle arcs, the angular extent is limited to the range [0,PI].
	 * So that angular extent only covers up to half the globe (like great circle arcs).
	 * When used as a small circle radius angle this is fine since a small circle with PI radius
	 * angle will cover the entire globe (because radius is half the diameter).
	 *
	 * Use of cosine and sine is more efficient in some situations such as comparing angular
	 * distances (between two unit vectors using a dot product - cosine) and adding two angular
	 * extents (using trigonometric angle sum identities). This can avoid calculating
	 * 'acos', which is generally slower to calculate (about 100 cycles on a circa 2011 CPU).
	 *
	 * Note: This is also useful for region-of-interest queries.
	 * For example, determining which geometries from one spatial partition are within a
	 * specified angular distance of geometries in another spatial partition - this can be
	 * achieved by *extending* the bounds of geometries added to one of the spatial partitions.
	 * Then a simple overlap test becomes a region-of-interest query - for example to perform
	 * a region-of-interest query of 10kms you would extend the bounding circle extent by
	 * the angle subtended by those 10kms.
	 */
	class AngularExtent :
			// NOTE: Base class chaining is used to avoid increasing sizeof(AngularExtent) due to multiple
			// inheritance from several empty base classes...
			public boost::addable<AngularExtent,
					boost::addable<AngularExtent, AngularDistance,
					boost::subtractable<AngularExtent,
					boost::subtractable<AngularExtent, AngularDistance,
					boost::subtractable2_left<AngularExtent, AngularDistance,
					boost::less_than_comparable<AngularExtent,
					boost::less_than_comparable<AngularExtent, AngularDistance,
					boost::equivalent<AngularExtent,
					boost::equivalent<AngularExtent, AngularDistance,
					boost::equality_comparable<AngularExtent,
					boost::equality_comparable<AngularExtent, AngularDistance> > > > > > > > > > >
	{
	public:

		//! Angular extent of zero (radians).
		static const AngularExtent ZERO;

		//! Angular extent of PI/2 radians (90 degrees).
		static const AngularExtent HALF_PI;

		//! Angular extent of PI radians (180 degrees).
		static const AngularExtent PI;


		/**
		 * Create from the cosine of the angular extent - the sine will be calculated when/if needed.
		 *
		 * Note that the cosine can be efficiently calculated as the dot product of two unit vectors.
		 */
		static
		AngularExtent
		create_from_cosine(
				const real_t &cosine)
		{
			return AngularExtent(cosine);
		}

		/**
		 * Create from the cosine and sine of the angular extent.
		 *
		 * This avoids a square root calculation (to get the sine from the cosine) if the sine
		 * is already available. Note that cosine and sine are assumed to refer to the same angle.
		 * This is not checked.
		 *
		 * Note that the cosine can be efficiently calculated as the dot product of two unit vectors.
		 */
		static
		AngularExtent
		create_from_cosine_and_sine(
				const real_t &cosine,
				const real_t &sine)
		{
			return AngularExtent(cosine, sine);
		}

		/**
		 * Create from an angular extent (radians) in the range [0, PI].
		 *
		 * The cosine (and the sine when/if needed) will be calculated.
		 *
		 * @throws PreconditionViolationError exception if @a angle is not in range [0, PI].
		 */
		static
		AngularExtent
		create_from_angle(
				const real_t &angle)
		{
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					0 <= angle && angle <= GPlatesMaths::PI,
					GPLATES_ASSERTION_SOURCE);

			return AngularExtent(cos(angle), boost::none/*sine*/, angle);
		}


		/**
		 * Create from the @a AngularDistance (containing the cosine) - the sine will be calculated when/if needed.
		 */
		explicit
		AngularExtent(
				const AngularDistance &angular_distance) :
			d_cosine(angular_distance.get_cosine())
		{  }


		/**
		 * Returns the cosine of the angular extent (radians).
		 */
		const real_t &
		get_cosine() const
		{
			return d_cosine;
		}


		/**
		 * Returns the sine of the angular extent (radians).
		 */
		const real_t &
		get_sine() const
		{
			if (!d_sine)
			{
				// Real takes care of very slightly negative arguments to 'sqrt'.
				d_sine = sqrt(1 - d_cosine * d_cosine);
			}

			return d_sine.get();
		}


		/**
		 * Returns the angle (in radians).
		 *
		 * NOTE: Although the angle is cached internally (when/if first accessed) that calculation
		 * can be relatively expensive (~100 cycles on a circa 2011 CPU).
		 * So it's better to use cosine unless the angle is actually needed.
		 */
		const real_t &
		get_angle() const
		{
			if (!d_angle)
			{
				d_angle = acos(d_cosine);
			}

			return d_angle.get();
		}


		/**
		 * Convenience method to create a lightweight version of @a AngularExtent
		 * known as @a AngularDistance.
		 *
		 * This is useful for those functions that accept @a AngularDistance as a parameter.
		 * Note that @a AngularDistance does not support addition/subtraction.
		 */
		AngularDistance
		get_angular_distance() const
		{
			return AngularDistance::create_from_cosine(d_cosine);
		}


		//
		// The following operators are provided by boost::addable:
		//
		// AngularExtent + AngularExtent
		//
		// ...based on 'AngularExtent += AngularExtent' which we explicitly provide below.
		//

		/**
		 * A member function for adding an angular extent to 'this' angular extent.
		 *
		 * NOTE: If the sum of the angles exceeds PI then the sum is clamped to PI (cosine set to -1).
		 * This is because cosine repeats itself when its angle exceeds PI and there's no longer
		 * a unique one-to-one mapping from cosine to its angle and vice versa.
		 * Also this clamping works when angular extent is used as a bounding small circle radius
		 * because a radius angle of PI represents a bounding small circle covering the entire globe.
		 *
		 * Even though it works with cosines and sines it effectively adds
		 * the two angular extents as angles.
		 * For example, an extent with 'a' radians plus an extent with 'b' radians
		 * gives an extent with 'a+b' radians.
		 */
		AngularExtent &
		operator+=(
				const AngularExtent &rhs);


		//
		// The following operators are provided by boost::addable (and return AngularExtent):
		//
		// AngularExtent + AngularDistance
		// AngularDistance + AngularExtent
		//
		// ...based on 'AngularExtent += AngularDistance' which we explicitly provide below.
		//

		/**
		 * A member function for adding an angular distance to 'this' angular extent.
		 *
		 * This is a convenient overload to 'AngularExtent += AngularExtent' but it is less efficient
		 * since the addition will require calculation of sine which AngularDistance does not contain.
		 */
		AngularExtent &
		operator+=(
				const AngularDistance &rhs)
		{
			return operator+=(AngularExtent(rhs));
		}


		//
		// The following operators are provided by boost::subtractable:
		//
		// AngularExtent - AngularExtent
		//
		// ...based on 'AngularExtent -= AngularExtent' which we explicitly provide below.
		//

		/**
		 * A member function for subtracting an angular extent from 'this' angular extent.
		 *
		 * NOTE: If the subtraction of the angles is less than zero then it is clamped to zero (cosine set to 1).
		 *
		 * Even though it works with cosines and sines it effectively subtracts
		 * the two angular extents as angles.
		 * For example, an extent with 'a' radians minus an extent with 'b' radians
		 * gives an extent with 'a-b' radians.
		 */
		AngularExtent &
		operator-=(
				const AngularExtent &rhs);


		//
		// The following operators are provided by boost::subtractable and boost::subtractable2_left
		// (and return AngularExtent):
		//
		// AngularExtent - AngularDistance
		// AngularDistance - AngularExtent
		//
		// ...based on 'AngularExtent -= AngularDistance' which we explicitly provide below.
		//
		// Note: Also required by boost::subtractable2_left is a conversion constructor from
		// AngularDistance to AngularExtent (which we provide above) and
		// 'AngularExtent -= AngularExtent' (which we provide above).
		//

		/**
		 * A member function for subtracting an angular distance to 'this' angular extent.
		 *
		 * This is a convenient overload to 'AngularExtent -= AngularExtent' but it is less efficient
		 * since the subtraction will require calculation of sine which AngularDistance does not contain.
		 */
		AngularExtent &
		operator-=(
				const AngularDistance &rhs)
		{
			return operator-=(AngularExtent(rhs));
		}


		//
		// The following operators are provided by boost::less_than_comparable,
		// boost::equivalent and boost::equality_comparable:
		//
		// AngularExtent <= AngularExtent
		// AngularExtent >= AngularExtent
		// AngularExtent > AngularExtent
		// AngularExtent == AngularExtent
		// AngularExtent != AngularExtent
		//
		// ...based on 'AngularExtent < AngularExtent' which we explicitly provide below.
		//

		/**
		 * Less than operator comparison with another @a AngularExtent.
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
		 */
		bool
		operator<(
				const AngularExtent &rhs) const
		{
			// NOTE: We're using 'real_t' which does epsilon test required for boost::equivalent to work.
			// Also note reversal of comparison since comparing cosine(angle) instead of angle.
			return d_cosine > rhs.d_cosine;
		}


		//
		// The following operators are provided by boost::less_than_comparable,
		// boost::equivalent and boost::equality_comparable:
		//
		// AngularExtent <= AngularDistance
		// AngularExtent >= AngularDistance
		// AngularExtent == AngularDistance
		// AngularExtent != AngularDistance
		//
		// AngularDistance <= AngularExtent
		// AngularDistance >= AngularExtent
		// AngularDistance < AngularExtent
		// AngularDistance > AngularExtent
		// AngularDistance == AngularExtent
		// AngularDistance != AngularExtent
		//
		// ...based on 'AngularExtent < AngularDistance' and 'AngularExtent > AngularDistance'
		// which we explicitly provide below.
		//

		/**
		 * Less than operator comparison with @a AngularDistance.
		 */
		bool
		operator<(
				const AngularDistance &rhs) const
		{
			// NOTE: We're using 'real_t' which does epsilon test required for boost::equivalent to work.
			// Also note reversal of comparison since comparing cosine(angle) instead of angle.
			return d_cosine > rhs.get_cosine();
		}

		/**
		 * Greater than operator comparison with @a AngularDistance.
		 */
		bool
		operator>(
				const AngularDistance &rhs) const
		{
			// NOTE: We're using 'real_t' which does epsilon test required for boost::equivalent to work.
			// Also note reversal of comparison since comparing cosine(angle) instead of angle.
			return d_cosine < rhs.get_cosine();
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

		//! Sine of angular extent - only calculated when needed.
		mutable boost::optional<real_t> d_sine;

		//! Angular extent - only calculated when needed.
		mutable boost::optional<real_t> d_angle;


		explicit
		AngularExtent(
				const real_t &cosine,
				boost::optional<real_t> sine = boost::none,
				boost::optional<real_t> angle = boost::none) :
			d_cosine(cosine),
			d_sine(sine),
			d_angle(angle)
		{  }
	};
}

#endif // GPLATES_MATHS_ANGULAREXTENT_H
