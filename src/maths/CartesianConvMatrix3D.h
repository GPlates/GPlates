/* $Id: CartesianConvMatrix3D.h,v 1.1.4.2 2006/03/14 00:29:01 mturner Exp $ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Author: mturner $
 *   $Date: 2006/03/14 00:29:01 $
 * 
 * Copyright (C) 2003, 2005, 2006 The University of Sydney, Australia.
 *
 * --------------------------------------------------------------------
 *
 * This file is part of GPlates.  GPlates is free software; you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License, version 2, as published by the Free Software
 * Foundation.
 *
 * GPlates is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to 
 * Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 */


#ifndef _GPLATES_MATHS_CARTESIANCONVMATRIX3D_H_
#define _GPLATES_MATHS_CARTESIANCONVMATRIX3D_H_

#include <ostream>
#include <boost/tuple/tuple.hpp>

#include "types.h"  /* real_t */
#include "PointOnSphere.h"
#include "Vector3D.h"


namespace GPlatesMaths
{
	class CartesianConvMatrix3D;


	/**
	 * Converts a 3D vector in the global geocentric coordinate system to a 3D vector
	 * in a local North/East/Down coordinate frame (determined by @a ccm).
	 */
	Vector3D
	convert_from_geocentric_to_north_east_down(
			const CartesianConvMatrix3D &ccm,
			const Vector3D &geocentric_vec);

	/**
	 * Converts a 3D vector in a local North/East/Down coordinate frame (determined by @a ccm)
	 * to a 3D vector in the global geocentric coordinate system.
	 */
	Vector3D
	convert_from_north_east_down_to_geocentric(
			const CartesianConvMatrix3D &ccm,
			const Vector3D &north_east_down_vec);

	//
	// The following applies to subsequent functions...
	//
	// Magnitude, azimuth and inclination are related to the North/East/Down coordinate frame
	// in the following way.
	// For a 3D vector in the North/East/Down coordinate frame:
	//  - magnitude is the length of the 3D vector,
	//  - azimuth is the angle (in radians) clockwise (East-wise) from North (from 0 to 2*PI),
	//  - inclination is the angle (in radians) in the downward direction (eg, PI/2 if vector aligned
	//    with Down axis, -PI/2 if aligned with up direction and 0 if vector in tangent plane).
	//


	/**
	 * Converts a 3D vector in the global geocentric coordinate system to a tuple of
	 * (magnitude, azimuth, inclination) coordinates (in a local North/East/Down coordinate frame
	 * determined by @a ccm).
	 */
	boost::tuple<real_t/*magnitude*/, real_t/*azimuth*/, real_t/*inclination*/>
	convert_from_geocentric_to_magnitude_azimuth_inclination(
			const CartesianConvMatrix3D &ccm,
			const Vector3D &geocentric_vec);

	/**
	 * Converts a tuple of (magnitude, azimuth, inclination) coordinates (in a local North/East/Down
	 * coordinate frame determined by @a ccm) to a 3D vector in the global geocentric coordinate system.
	 */
	Vector3D
	convert_from_magnitude_azimuth_inclination_to_geocentric(
			const CartesianConvMatrix3D &ccm,
			const boost::tuple<real_t/*magnitude*/, real_t/*azimuth*/, real_t/*inclination*/> &
					magnitude_azimuth_inclination);


	/**
	 * Converts a 3D vector in a local North/East/Down coordinate frame to a tuple of
	 * (magnitude, azimuth, inclination) coordinates in the same coordinate frame.
	 */
	boost::tuple<real_t/*magnitude*/, real_t/*azimuth*/, real_t/*inclination*/>
	convert_from_north_east_down_to_magnitude_azimuth_inclination(
			const Vector3D &north_east_down_vec);

	/**
	 * Converts a tuple of (magnitude, azimuth, inclination) coordinates in a local North/East/Down
	 * coordinate frame to a 3D vector in the coordinate frame.
	 */
	Vector3D
	convert_from_magnitude_azimuth_inclination_to_north_east_down(
			const boost::tuple<real_t/*magnitude*/, real_t/*azimuth*/, real_t/*inclination*/> &
					magnitude_azimuth_inclination);


	/** 
	 * A 3x3 matrix used to convert the components of a global geocentric Cartesian
	 * Vector3D (x, y, z) into the components of a local Cartesian Vector3D
	 * (north, east, down) at a given PointOnSphere.
	 */
	class CartesianConvMatrix3D
	{
		public:

			/**
			 * Create a cartesian conversion matrix to operate at
			 * the PointOnSphere @a pos.
			 */
			explicit 
			CartesianConvMatrix3D(
					const PointOnSphere &pos);


			const Vector3D &
			north() const
			{
				return d_north;
			}


			const Vector3D &
			east() const
			{
				return d_east;
			}


			const Vector3D &
			down() const
			{
				return d_down;
			}

		private:
			Vector3D d_north;
			Vector3D d_east;
			Vector3D d_down;
	};


	inline
	bool
	operator==(
			const CartesianConvMatrix3D &ccm1,
			const CartesianConvMatrix3D &ccm2)
	{
		return
			ccm1.north() == ccm2.north() &&
			ccm1.east() == ccm2.east() &&
			ccm1.down() == ccm2.down();
	}


	inline
	bool
	operator!=(
			const CartesianConvMatrix3D &ccm1,
			const CartesianConvMatrix3D &ccm2)
	{
		return !operator==(ccm1, ccm2);
	}
}

#endif  // _GPLATES_MATHS_CARTESIANCONVMATRIX3D_H_


