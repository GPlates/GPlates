/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2008 The University of Sydney, Australia
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
#include <QDebug>

#include "UnitQuaternion3D.h"
#include "HighPrecision.h"
#include "IndeterminateResultException.h"
#include "ViolatedClassInvariantException.h"

#include "scribe/Scribe.h"


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
#if 0
		qWarning() << "Renormalising unit-quat (current deviation from 1.0 = "
				<< HighPrecision<double>(norm - 1.0) << ")";
#endif

		double one_on_norm = 1.0 / norm;
		m_scalar_part *= one_on_norm;
		m_vector_part = one_on_norm * m_vector_part;

#if 0
		norm = std::sqrt(get_actual_norm_sqrd().dval());
		qWarning() << "After renormalisation, deviation from 1.0 = "
				<< HighPrecision<double>(norm - 1.0);
#endif
	}
}


const GPlatesMaths::UnitQuaternion3D::RotationParams
GPlatesMaths::UnitQuaternion3D::get_rotation_params(
		const boost::optional<UnitVector3D> &axis_hint) const
{
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

		throw IndeterminateResultException(GPLATES_EXCEPTION_SOURCE,
				oss.str().c_str());
	}

	/*
	 * We can now be sure that the angle of rotation ('theta') is not a
	 * multiple of two PI, and the axis of rotation is clearly determined.
	 */
	real_t theta_on_2 = acos(scalar_part());  // not a multiple of PI

	// Previously we defined the axis as:
	//
	//    Vector3D axis_vector = (1 / sin(theta_on_2)) * vector_part();
	//    UnitVector3D axis_unit_vector = axis_vector.get_normalisation();
	//
	// However we don't need the reciprocal sine term since we're normalising anyway and
	// the *sign* of 'sin(theta_on_2)' will always be positive because 'acos()' returns
	// the range [0,PI] and sine of that range is always positive.
	//
	// This is essentially a result of the fact that both (angle, axis) and (-angle, -axis) get mapped
	// onto the exact same quaternion (they're actually the same rotation). So it's not possible to
	// determine, just by looking at the quaternion, which angle/axis variant it was created from.
	// So we always end up returning the positive angle variant.
	// In other words, regardless of whether this quaternion was created with (angle, axis) or (-angle, -axis)
	// we'll always return (angle, axis) unless 'axis_hint' is provided (see below).
	//
	UnitVector3D axis_unit_vector = vector_part().get_normalisation();

	// Now, let's use the axis hint (if provided) to determine whether our rotation axis is
	// pointing in the expected direction (ie, in the direction the user originally specified).
	// FIXME:  Should we assert that the result of the dot product is never approx zero?
	if (axis_hint) {
		if (is_strictly_negative(dot(axis_unit_vector, *axis_hint))) {
			// The calculated axis seems to be pointing in the opposite direction to
			// that which the user would expect.
			axis_unit_vector = -axis_unit_vector;
			theta_on_2 = -theta_on_2;
		}
	}

	return RotationParams(axis_unit_vector, theta_on_2 * 2.0);
}


const GPlatesMaths::UnitQuaternion3D
GPlatesMaths::UnitQuaternion3D::create_rotation(
 const UnitVector3D &axis,
 const real_t &angle) {

	real_t theta_on_two = angle / 2.0;

	//
	// If 'angle' is positive then 'get_rotation_params()' will return the original angle and axis.
	// If 'angle' is negative then 'get_rotation_params()' will return negated versions of the original angle and axis.
	//
	// This is because if 'angle' is negative then it's effectively made positive by the fact that
	// 'cos(-angle) = cos(angle)', and the axis is inverted (in direction) due to the fact that
	// 'sin(-angle) = -sin(angle)'.
	// This is essentially a result of the fact that both (angle, axis) and (-angle, -axis) get mapped
	// onto the exact same quaternion (they're actually the same rotation). So it's not possible to
	// determine, just by looking at the quaternion, which angle/axis variant it was created from.
	// In other words, regardless of whether this quaternion was created with (angle, axis) or (-angle, -axis)
	// 'get_rotation_params()' will always return (angle, axis) unless an axis hint is provided to it.
	//
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

			throw IndeterminateResultException(GPLATES_EXCEPTION_SOURCE,
					oss.str().c_str());
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

		throw ViolatedClassInvariantException(GPLATES_EXCEPTION_SOURCE,
				oss.str().c_str());
	}
}


GPlatesScribe::TranscribeResult
GPlatesMaths::UnitQuaternion3D::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<UnitQuaternion3D> &unit_quaternion)
{
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, unit_quaternion->m_scalar_part, "scalar_part");
		scribe.save(TRANSCRIBE_SOURCE, unit_quaternion->m_vector_part, "vector_part");
	}
	else // loading
	{
		GPlatesScribe::LoadRef<real_t> scalar_part_ = scribe.load<real_t>(TRANSCRIBE_SOURCE, "scalar_part");
		if (!scalar_part_.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		GPlatesScribe::LoadRef<Vector3D> vector_part_ = scribe.load<Vector3D>(TRANSCRIBE_SOURCE, "vector_part");
		if (!vector_part_.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		unit_quaternion.construct_object(scalar_part_, vector_part_);
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesMaths::UnitQuaternion3D::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, m_scalar_part, "scalar_part"))
		{
			return scribe.get_transcribe_result();
		}

		if (!scribe.transcribe(TRANSCRIBE_SOURCE, m_vector_part, "vector_part"))
		{
			return scribe.get_transcribe_result();
		}
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
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
