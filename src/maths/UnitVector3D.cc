/* $Id$ */

/**
 * \file 
 * File specific comments.
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
 *
 * Authors:
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 *   Dave Symonds <ds@geosci.usyd.edu.au>
 */

#include <sstream>
#include "UnitVector3D.h"
#include "ViolatedUnitVectorInvariantException.h"


void
GPlatesMaths::UnitVector3D::AssertInvariant () const
{
	/*
	 * Calculate magnitude of vector to ensure that it actually _is_ 1.
	 * For efficiency, don't bother sqrting yet.
	 *
	 * On a Pentium IV processor, the calculation of 'mag_sqrd' should
	 * cost about (7 + 2 * 2) + (5 + 1) = 17 clock cycles, while the
	 * FP comparison for unity should cost about 9 clock cycles, making
	 * a total cost of about 26 clock cycles.
	 *
	 * Obviously, if the comparison returns false, performance will go
	 * right out the window.
	 */
	real_t mag_sqrd = (_x * _x) + (_y * _y) + (_z * _z);
	if (mag_sqrd != 1.0) {

		// invariant has been violated
		std::ostringstream oss;
		oss << "UnitVector3D has magnitude " << sqrt(mag_sqrd) << ".";
		throw ViolatedUnitVectorInvariantException(oss.str().c_str());
	}
}


GPlatesMaths::UnitVector3D
GPlatesMaths::generatePerpendicular(const UnitVector3D &u) {

	/*
	 * Let's start with the three Cartesian basis vectors x, y and z.
	 * Take their dot-products with 'u' to test for orthogonality.
	 *
	 * Of course, since x, y and z are the vectors of an orthonormal
	 * basis, their dot-products with 'u' will simply extract individual
	 * components of 'u'.
	 */
	real_t xdot = u.x();
	real_t ydot = u.y();
	real_t zdot = u.z();

	if (xdot == 0.0) {

		// Instant winner!  x is perpendicular to 'u'.
		return UnitVector3D::xBasis();
	}
	if (ydot == 0.0) {

		// Instant winner!  y is perpendicular to 'u'.
		return UnitVector3D::yBasis();
	}
	if (zdot == 0.0) {

		// Instant winner!  z is perpendicular to 'u'.
		return UnitVector3D::zBasis();
	}

	/*
	 * Ok, so none of x, y or z are perpendicular to 'u'.
	 * As a result, we'll have to take one of them and calculate
	 * the cross-product of that vector with 'u'.  Recall that the
	 * result of a cross-product is perpendicular to its arguments.
	 *
	 * The result of a cross-product is perpendicular to its arguments
	 * because it is one of the two normals to the plane defined by the
	 * arguments.  If the arguments are collinear, it is not possible
	 * to determine a plane, hence, we must ensure that the basis vector
	 * we use is not collinear with 'u'.
	 *
	 * Since our three basis vectors are orthonormal, if one of them
	 * were collinear with 'u', the other two would be perpendicular to
	 * 'u'.  Since we have determined that *none* of them are perp. to
	 * 'u', we can deduce that none of them are collinear with 'u'.
	 *
	 * We want to use the basis vector whose dot-product with 'u' is the
	 * smallest: this vector will be the "most perpendicular" to 'u',
	 * and thus, will have the "most clearly-defined" cross-product.
	 *
	 * Since we will be taking the cross-product of non-collinear
	 * unit-vectors, the result will always have non-zero length, and
	 * hence, we can safely normalise it.
	 */
	if (xdot < ydot) {

		// prefer x over y
		if (xdot < zdot) {

			// prefer x over both y and z
			return cross(u, UnitVector3D::xBasis()).normalise();

		} else {

			// prefer x over y, but z over x
			return cross(u, UnitVector3D::zBasis()).normalise();
		}

	} else {

		// prefer y over x
		if (ydot < zdot) {

			// prefer y over both x and z
			return cross(u, UnitVector3D::yBasis()).normalise();

		} else {

			// prefer y over x, but z over y
			return cross(u, UnitVector3D::zBasis()).normalise();
		}
	}
}
