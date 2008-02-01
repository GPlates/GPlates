/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006 The University of Sydney, Australia
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

#include <iostream>
#include <sstream>
#include <cmath>

#include "UnitQuaternion3D.h"
#include "HighPrecision.h"
#include "IndeterminateResultException.h"
#include "ViolatedClassInvariantException.h"


void
GPlatesMaths::UnitQuaternion3D::renormalise_if_necessary() {

	// Unit-quaternions require renormalisation sometimes, due to the accumulation of
	// floating-point error (which can occur when unit-quaternions are composed, for example).
	//
	// From testing and observation, it seems that, very soon after the actual-norm-squared
	// deviates from 1.0 by more than 2.0e-14, using the unit-quaternion to rotate a
	// unit-vector will result in a unit-vector whose actual-magnitude-squared deviates from
	// 1.0 by 5.0e-14.  (Note that these numbers are fairly arbitrary; a crude justification
	// for the significance of 1.0e-14 is given in "src/maths/Real.cc".)
	//
	// So, to avoid this, let's renormalise this unit-quaternion if its actual-norm-squared
	// deviates from 1.0 by more than 2.0e-14.

	double norm_sqrd = get_actual_norm_sqrd().dval();
	if (std::fabs(norm_sqrd - 1.0) > 2.0e-14) {
		double norm = std::sqrt(norm_sqrd);
		std::cerr << "Renormalising unit-quat (current deviation from 1.0 = "
				<< HighPrecision<double>(norm - 1.0) << ")" << std::endl;

		double one_on_norm = 1.0 / norm;
		m_scalar_part *= one_on_norm;
		m_vector_part = one_on_norm * m_vector_part;

		norm = std::sqrt(get_actual_norm_sqrd().dval());
		std::cerr << "After renormalisation, deviation from 1.0 = "
				<< HighPrecision<double>(norm - 1.0) << std::endl;
	}
}


const GPlatesMaths::UnitQuaternion3D::RotationParams
GPlatesMaths::UnitQuaternion3D::get_rotation_params() const {

	/*
	 * Ensure that the quaternion does not represent an identity rotation.
	 *
	 * In an identity rotation, the angle of rotation is (2 * n * PI),
	 * for some integer 'n':  this would later result in an evaluation of
	 * the sine of some (n * PI), which is always zero.  This, in turn,
	 * would result in a division by zero when attempting to calculate
	 * the rotation axis, which is geometrically equivalent to the fact
	 * that, in an identity rotation, the axis is indeterminate.
	 */
	if (represents_identity_rotation(*this)) {

		std::ostringstream oss;

		oss
		 << "Attempted to calculate the rotation parameters\n"
		 << "of a quaternion which represents the identity rotation:\n"
		 << (*this);

		throw IndeterminateResultException(oss.str().c_str());
	}

	/*
	 * We can now be sure that the angle of rotation ('theta') is not a
	 * multiple of two PI, and the axis of rotation is clearly determined.
	 */
	real_t theta_on_2 = acos(scalar_part());  // not a multiple of PI
	real_t sin_of_theta_on_2 = sin(theta_on_2);  // not zero

	Vector3D axis_vector = (1 / sin_of_theta_on_2) * vector_part();
	UnitVector3D axis_unit_vector = axis_vector.get_normalisation();

	return RotationParams(axis_unit_vector, theta_on_2 * 2.0);
}


const GPlatesMaths::UnitQuaternion3D
GPlatesMaths::UnitQuaternion3D::create_rotation(
 const UnitVector3D &axis,
 const real_t &angle) {

	real_t theta_on_two = angle / 2.0;

	real_t   scalar_part = cos(theta_on_two);
	Vector3D vector_part = sin(theta_on_two) * Vector3D(axis);

	return UnitQuaternion3D(scalar_part, vector_part);
}


const GPlatesMaths::UnitQuaternion3D
GPlatesMaths::UnitQuaternion3D::create_identity_rotation()
{
	/*
	 * A unit quaternion which encodes an identity rotation is composed of a scalar part which
	 * is equal to one, and a vector part which is the zero vector.
	 *
	 * (For the proof of this statement, read the comment in the function
	 * '::GPlatesMaths::represents_identity_rotation(const UnitQuaternion3D &)'.)
	 */
	real_t   scalar_part = 1.0;
	Vector3D vector_part = Vector3D(0.0, 0.0, 0.0);

	return UnitQuaternion3D(scalar_part, vector_part);
}


const GPlatesMaths::UnitQuaternion3D
GPlatesMaths::UnitQuaternion3D::create(
 const NonUnitQuaternion &q) {

	// Assert the invariant.
#if 0  // Until precision suckiness is fixed.  (Hint, sucka: FIXME.)
	real_t norm_sqrd = dot(q, q);
	if (norm_sqrd != 1.0) {  // FIXME: adjust precision of F.P.-comparison.

		std::ostringstream oss;

		oss
		 << "Attempted to create a unit quaternion from a quaternion "
		 << "of magnitude\n"
		 << sqrt(norm_sqrd)  // FIXME: use HighPrecision ?
		 << ": "
		 << q
		 << ".";

		throw ViolatedClassInvariantException(oss.str().c_str());
	}
#else  // FIXME: this sucks.  There should be two thresholds (strict & relaxed)
	real_t norm = sqrt(dot(q, q));
	if (norm != 1.0) {  // FIXME: adjust precision of F.P.-comparison.

		// Just to be on the safe side...
		if (norm == 0.0) {

			std::ostringstream oss;

			oss
			 << "Unable to renormalise the non-unit-quaternion "
			 << q
			 << " because its norm is 0.";

			throw IndeterminateResultException(oss.str().c_str());
		}
		real_t one_on_norm = 1.0 / norm;

		return
		 UnitQuaternion3D(
		  one_on_norm * q.d_scalar_part,
		  one_on_norm * q.d_vector_part);
	}
#endif
	return UnitQuaternion3D(q.d_scalar_part, q.d_vector_part);
}


GPlatesMaths::UnitQuaternion3D::UnitQuaternion3D(
		const real_t &s,
		const Vector3D &v):
	m_scalar_part(s),
	m_vector_part(v) {

	renormalise_if_necessary();
}


void
GPlatesMaths::UnitQuaternion3D::assert_invariant() const {

	/*
	 * FIXME: (1) implement loose/tight comparisons policy, with automatic
	 * self-correction in the case of natural drift.
	 *
	 * FIXME: (2) once (1) is implemented, invoke this in the ctor.
	 */

	/*
	 * Calculate norm of quaternion to ensure that it actually _is_ 1.
	 * For efficiency, don't bother sqrting yet.
	 */
	real_t norm_sqrd = get_actual_norm_sqrd();
	if (norm_sqrd != 1.0) {

		// invariant has been violated
		std::ostringstream oss;

		oss
		 << "UnitQuaternion3D has magnitude "
		 << sqrt(norm_sqrd)
		 << ".";

		throw ViolatedClassInvariantException(oss.str().c_str());
	}
}


const GPlatesMaths::UnitQuaternion3D
GPlatesMaths::operator*(
 const UnitQuaternion3D &q1,
 const UnitQuaternion3D &q2) {

	const real_t &s1 = q1.scalar_part();
	const real_t &s2 = q2.scalar_part();

	const Vector3D &v1 = q1.vector_part();
	const Vector3D &v2 = q2.vector_part();

	real_t   res_scalar_part = s1 * s2 - dot(v1, v2);
	Vector3D res_vector_part = s1 * v2 + s2 * v1 + cross(v1, v2);

	return UnitQuaternion3D(res_scalar_part, res_vector_part);
}


std::ostream &
GPlatesMaths::operator<<(
 std::ostream &os,
 const UnitQuaternion3D &q) {

	os
	 << "("
	 << q.w() << ", "
	 << q.x() << ", "
	 << q.y() << ", "
	 << q.z() << ")";

	return os;
}


std::ostream &
GPlatesMaths::operator<<(
 std::ostream &os,
 const UnitQuaternion3D::NonUnitQuaternion &q) {

	os
	 << "["
	 << q.d_scalar_part
	 << ", "
	 << q.d_vector_part
	 << "]";

	return os;
}
