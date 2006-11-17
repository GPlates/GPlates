/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
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

#include <iostream>
#include <sstream>

#include "FiniteRotation.h"
#include "HighPrecision.h"
#include "Vector3D.h"
#include "PolylineOnSphere.h"
#include "PolygonOnSphere.h"
#include "LatLonPointConversions.h"
#include "InvalidOperationException.h"
#include "IndeterminateResultException.h"


namespace {

	inline
	const GPlatesMaths::real_t
	calculate_d_value(
	 const GPlatesMaths::UnitQuaternion3D &uq) {

		const GPlatesMaths::real_t &s = uq.scalar_part();
		const GPlatesMaths::Vector3D &v = uq.vector_part();

		return ((s * s) - dot(v, v));
	}


	inline
	const GPlatesMaths::Vector3D
	calculate_e_value(
	 const GPlatesMaths::UnitQuaternion3D &uq) {

		const GPlatesMaths::real_t &s = uq.scalar_part();
		const GPlatesMaths::Vector3D &v = uq.vector_part();

		return ((2.0 * s) * v);
	}

}


const GPlatesMaths::FiniteRotation
GPlatesMaths::FiniteRotation::create(
 const PointOnSphere &euler_pole,
 const real_t &rotation_angle,
 const real_t &point_in_time) {

	const UnitVector3D &rotation_axis = euler_pole.position_vector();
	UnitQuaternion3D uq =
	 UnitQuaternion3D::create_rotation(rotation_axis, rotation_angle);

	// These values are used to optimise rotation of points on the sphere.
	real_t d = calculate_d_value(uq);
	Vector3D e = calculate_e_value(uq);

	return FiniteRotation(uq, point_in_time, d, e);
}


const GPlatesMaths::FiniteRotation
GPlatesMaths::FiniteRotation::create(
 const UnitQuaternion3D &uq,
 const real_t &point_in_time) {

	// These values are used to optimise rotation of points on the sphere.
	real_t d = calculate_d_value(uq);
	Vector3D e = calculate_e_value(uq);

	return FiniteRotation(uq, point_in_time, d, e);
}


const GPlatesMaths::UnitVector3D
GPlatesMaths::FiniteRotation::operator*(
 const UnitVector3D &uv) const {

	Vector3D v(uv);
	const Vector3D &uq_v = m_unit_quat.vector_part();

	Vector3D v_rot =
	 m_d * v +
	 (2.0 * dot(uq_v, v)) * uq_v +
	 cross(m_e, v);

	// FIXME: This both sucks *and* blows.  All this stuff needs a cleanup.
	real_t mag_sqrd = v_rot.magSqrd();
	if ( ! are_slightly_more_strictly_equal(mag_sqrd, 1.0)) {

		// Renormalise it, before the UV3D ctor has a hissy fit.
#if 0
		real_t old_mag = sqrt(mag_sqrd);
#endif
		v_rot = (1.0 / sqrt(mag_sqrd)) * v_rot;
#if 0
		std::cerr
		 << "Useful analysis info: just renormalised Vector3D:\n"
		 << v_rot
		 << "\nMagnitude was "
		 << HighPrecision< real_t >(old_mag)
		 << "; is now "
		 << HighPrecision< real_t >(sqrt(v_rot.magnitude()))
		 << std::endl;
#endif
	}

	return UnitVector3D(v_rot.x(), v_rot.y(), v_rot.z());
}


const GPlatesMaths::FiniteRotation
GPlatesMaths::operator*(
 const FiniteRotation &r1,
 const FiniteRotation &r2) {

	if (r1.time() != r2.time()) {

		// these FiniteRotations are not of the same point in time.
		std::ostringstream oss;
		oss
		 << "Mismatched times in composition of FiniteRotations:\n"
		 << r1.time()
		 << " vs. "
		 << r2.time()
		 << ".";

		throw InvalidOperationException(oss.str().c_str());
	}

	UnitQuaternion3D resultant_uq = r1.unit_quat() * r2.unit_quat();
	return FiniteRotation::create(resultant_uq, r1.time());
}


const GPlatesMaths::PolylineOnSphere
GPlatesMaths::operator*(
 const FiniteRotation &r,
 const PolylineOnSphere &polyline) {

	std::list< PointOnSphere > rotated_points;

	PolylineOnSphere::vertex_const_iterator
	 iter = polyline.vertex_begin(),
	 end = polyline.vertex_end();

	for ( ; iter != end; ++iter) rotated_points.push_back(r * (*iter));

	return PolylineOnSphere::create(rotated_points);
}


const GPlatesMaths::PolygonOnSphere
GPlatesMaths::operator*(
 const FiniteRotation &r,
 const PolygonOnSphere &polygon) {

	std::list< PointOnSphere > rotated_points;

	PolygonOnSphere::vertex_const_iterator
	 iter = polygon.vertex_begin(),
	 end = polygon.vertex_end();

	for ( ; iter != end; ++iter) rotated_points.push_back(r * (*iter));

	return PolygonOnSphere::create(rotated_points);
}


namespace {

	const GPlatesMaths::UnitQuaternion3D
	slerp(
	 const GPlatesMaths::UnitQuaternion3D &q1,
	 const GPlatesMaths::UnitQuaternion3D &q2,
	 const GPlatesMaths::real_t &t) {

		using namespace GPlatesMaths;

		/*
		 * This algorithm based upon the method described in Burger89.
		 */

		real_t cos_theta = dot(q1, q2);

		if (cos_theta >= 1.0) {

			/*
			 * The two quaternions are, as far as we're concerned,
			 * identical.  Trying to slerp these suckers will lead
			 * to Infs, NaNs and heart-ache.
			 */
			return q1;
		}
		if (cos_theta <= -1.0) {

			/*
			 * FIXME:  The two quaternions are pointing in opposite
			 * directions.  In 4D hypersphere space.  What the hell
			 * am I supposed to do now?  How the hell do I
			 * interpolate from one to the other when there's not
			 * even a unique 4D great-circle from one to the other? 
			 *
			 * Perhaps *more* concerningly, how the **HELL** do I
			 * explain this to the user??  "Hello, your finite
			 * rotation quaternions are pointing in opposite
			 * directions in four-dimensional hyperspace, even
			 * though you don't understand how four-dimensional
			 * hyperspace has anything to do with plate rotations
			 * and you probably don't even know what quaternions
			 * are!  This operation will now terminate.  Have a
			 * nice day!"
			 *
			 * Argh!
			 */
			std::ostringstream oss;

			oss
			 << "Unable to interpolate between quaternions which "
			    "are pointing in opposite directions\n"
			    "in 4D hypersphere space: "
			 << q1
			 << "\nand "
			 << q2
			 << ".\nNot quite sure what to make of this?  Neither "
			    "are we.  You should probably contact us (the "
			    "developers).";

			throw IndeterminateResultException(oss.str().c_str());
		}

		// Since cos(theta) lies in the range (-1, 1), theta will lie
		// in the range (0, PI).
		real_t theta = acos(cos_theta);

		// Since theta lies in the range (0, PI), sin(theta) will lie
		// in the range (0, 1].
		//
		// Since cos(theta) lies in the range (-1, 1), cos^2(theta)
		// will lie in the range [0, 1), so (1 - cos^2(theta)) will lie
		// in the range (0, 1], so sqrt(1 - cos^2(theta)) lie in the
		// range (0, 1], and hence can be used in place of sin(theta)
		// without any sign/quadrant issues.
		//
		// And finally, since sqrt(1 - cos^2(theta)) lies in the range
		// (0, 1], there won't be any division by zero.
		real_t one_on_sin_theta =
		 1.0 / sqrt(1.0 - cos_theta * cos_theta);

		real_t
		 c1 = sin((1.0 - t) * theta) * one_on_sin_theta,
		 c2 = sin(t * theta) * one_on_sin_theta;

		return UnitQuaternion3D::create(c1 * q1 + c2 * q2);
	}

}


const GPlatesMaths::FiniteRotation
GPlatesMaths::interpolate(
 const FiniteRotation &r1,
 const FiniteRotation &r2,
 const real_t &t) {

	if (r1.time() == r2.time()) {

		throw IndeterminateResultException("Attempted to interpolate "
		 "between two finite rotations of the same age.");
	}
	const real_t &t1 = r1.time(), &t2 = r2.time();
	real_t interpolation_parameter = (t - t1) / (t2 - t1);

	UnitQuaternion3D q =
	 slerp(r1.unit_quat(), r2.unit_quat(), interpolation_parameter);

	return FiniteRotation::create(q, t);
}


std::ostream &
GPlatesMaths::operator<<(
 std::ostream &os,
 const FiniteRotation &fr) {

	os << "(time = " << fr.time() << "; rot = ";

	UnitQuaternion3D uq = fr.unit_quat();
	if (represents_identity_rotation(uq)) {

		os << "identity";

	} else {

		using LatLonPointConversions::convertPointOnSphereToLatLonPoint;

		UnitQuaternion3D::RotationParams params =
		 uq.get_rotation_params();

		PointOnSphere p(params.axis);  // the point
		PointOnSphere antip( -p.position_vector());  // the antipodal point

		os
		 << "(pole = "
		 << convertPointOnSphereToLatLonPoint(p)
		 << " (which is antipodal to "
		 << convertPointOnSphereToLatLonPoint(antip)
		 << "); angle = "
		 << radiansToDegrees(params.angle)
		 << " deg)";
	}
	os << ")";

	return os;
}
