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

#include <cmath>
#include <boost/optional.hpp>


namespace GPlatesMaths
{
	class BoundingSmallCircle;

	/**
	 * An angular extent stored as cosine and sine instead of the actual angle.
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
	class AngularExtent
	{
	public:

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
				const double &cosine_angle,
				const double &sine_angle)
		{
			return AngularExtent(cosine_angle, sine_angle);
		}


		/**
		 * Create from only the cosine of the angular extent - the sine will be calculated.
		 *
		 * Note that the cosine can be efficiently calculated as the dot product of two unit vectors.
		 */
		static
		AngularExtent
		create_from_cosine(
				const double &cosine_angle);


		/**
		 * Create from only the angular extent (radians) - both the cosine and the sine will be calculated.
		 */
		static
		AngularExtent
		create_from_angle(
				const double &angle)
		{
			return create_from_cosine(std::cos(angle));
		}


		/**
		 * Create from only the extents are specified by @a bounding_small_circle.
		 *
		 * NOTE: The small circle centre is not needed or used here (only the extents are used).
		 */
		static
		AngularExtent
		create(
				const BoundingSmallCircle &bounding_small_circle);


		/**
		 * Returns the cosine of the angular extent (radians).
		 */
		const double &
		get_cosine() const
		{
			return d_cosine;
		}


		/**
		 * Returns the sine of the angular extent (radians).
		 */
		const double &
		get_sine() const
		{
			return d_sine;
		}


		/**
		 * A member function for adding two angular extents such that the returned
		 * angular extent is the sum of the angles.
		 *
		 * Even though it works with cosines and sines it effectively adds
		 * the two angular extents as angles.
		 * For example, an extent with 'a' radians plus an extent with 'b' radians
		 * gives an extent with 'a+b' radians.
		 */
		AngularExtent
		operator+(
				const AngularExtent &rhs) const
		{
			return AngularExtent(
					// cos(a+b) = cos(a)cos(b) - sin(a)sin(b)
					d_cosine * rhs.d_cosine - d_sine * rhs.d_sine,
					// sin(a+b) = sin(a)cos(b) + cos(a)sin(b)
					d_sine * rhs.d_cosine + d_cosine * rhs.d_sine);
		}

	private:

		AngularExtent(
				const double &cosine,
				const double &sine) :
			d_cosine(cosine),
			d_sine(sine)
		{  }

		double d_cosine;
		double d_sine;
	};
}

#endif // GPLATES_MATHS_ANGULAREXTENT_H
