/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2010 The University of Sydney, Australia
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
#include <boost/optional.hpp>

#include "FiniteRotation.h"

#include "ConstGeometryOnSphereVisitor.h"
#include "GreatCircleArc.h"
#include "GreatCircle.h"
#include "HighPrecision.h"
#include "IndeterminateResultException.h"
#include "InvalidOperationException.h"
#include "LatLonPoint.h"
#include "MathsUtils.h"
#include "MultiPointOnSphere.h"
#include "PointOnSphere.h"
#include "PolygonOnSphere.h"
#include "PolylineOnSphere.h"
#include "SmallCircle.h"
#include "UnitVector3D.h"
#include "Vector3D.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


namespace {

	// This value is calculated to optimise rotation operations.
	inline
	const GPlatesMaths::real_t
	calculate_d_value(
			const GPlatesMaths::UnitQuaternion3D &uq)
	{
		const GPlatesMaths::real_t &s = uq.scalar_part();
		const GPlatesMaths::Vector3D &v = uq.vector_part();

		return ((s * s) - dot(v, v));
	}


	// This value is calculated to optimise rotation operations.
	inline
	const GPlatesMaths::Vector3D
	calculate_e_value(
			const GPlatesMaths::UnitQuaternion3D &uq)
	{
		const GPlatesMaths::real_t &s = uq.scalar_part();
		const GPlatesMaths::Vector3D &v = uq.vector_part();

		return ((2.0 * s) * v);
	}


	/**
	 * Visits a @a GeometryOnSphere, rotates it and returns as a @a GeometryOnSphere.
	 */
	class RotateGeometryOnSphere :
			public GPlatesMaths::ConstGeometryOnSphereVisitor
	{
	public:
		/**
		 * Construct with the @a FiniteRotation to use for rotating.
		 */
		explicit
		RotateGeometryOnSphere(
				const GPlatesMaths::FiniteRotation &finite_rotation) :
			d_finite_rotation(finite_rotation)
		{  }


		/**
		 * Rotates @a geometry using @a FiniteRotation passed into constructor
		 * and returns rotated @a GeometryOnSphere.
		 */
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		rotate(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry)
		{
			d_rotated_geometry = boost::none;

			geometry->accept_visitor(*this);

			// Unless there's a new derived type of GeometryOnSphere we should
			// be able to dereference 'd_rotated_geometry'.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					d_rotated_geometry, GPLATES_ASSERTION_SOURCE);

			return *d_rotated_geometry;
		}

	protected:
		virtual
		void
		visit_multi_point_on_sphere(
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
		{
			d_rotated_geometry = d_finite_rotation * multi_point_on_sphere;
		}

		virtual
		void
		visit_point_on_sphere(
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
		{
			d_rotated_geometry = d_finite_rotation * point_on_sphere;
		}

		virtual
		void
		visit_polygon_on_sphere(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
		{
			d_rotated_geometry = d_finite_rotation * polygon_on_sphere;
		}

		virtual
		void
		visit_polyline_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
		{
			d_rotated_geometry = d_finite_rotation * polyline_on_sphere;
		}

	private:
		const GPlatesMaths::FiniteRotation &d_finite_rotation;
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> d_rotated_geometry;
	};
}


const GPlatesMaths::FiniteRotation
GPlatesMaths::FiniteRotation::create(
		const PointOnSphere &pole,
		const real_t &angle)
{
	const UnitVector3D &axis = pole.position_vector();
	UnitQuaternion3D uq = UnitQuaternion3D::create_rotation(axis, angle);

	return FiniteRotation(uq, axis);
}

const GPlatesMaths::FiniteRotation
GPlatesMaths::FiniteRotation::create(
		const PointOnSphere &from_point,
		const PointOnSphere &to_point)
{
	const Vector3D rotation_axis = cross(from_point.position_vector(), to_point.position_vector());

	// If the points are the same or antipodal then there are an infinite number of rotation axes
	// possible, so we just pick one arbitrarily.
	const PointOnSphere pole = (rotation_axis.magSqrd() == 0)
			? PointOnSphere(generate_perpendicular(from_point.position_vector()))
			: PointOnSphere(rotation_axis.get_normalisation());

	const real_t angle = acos(dot(from_point.position_vector(), to_point.position_vector()));

	return create(pole, angle);
}


const GPlatesMaths::FiniteRotation
GPlatesMaths::FiniteRotation::create(
		const UnitQuaternion3D &uq,
		const boost::optional<UnitVector3D> &axis_hint_)
{
	return FiniteRotation(uq, axis_hint_);
}


const GPlatesMaths::FiniteRotation
GPlatesMaths::FiniteRotation::create_identity_rotation()
{
	return FiniteRotation(UnitQuaternion3D::create_identity_rotation(), boost::none);
}


GPlatesMaths::FiniteRotation::FiniteRotation(
		const UnitQuaternion3D &unit_quat_,
		const boost::optional<UnitVector3D> &axis_hint_):
	d_unit_quat(unit_quat_),
	d_axis_hint(axis_hint_),
	d_d(::calculate_d_value(unit_quat_)),
	d_e(::calculate_e_value(unit_quat_))
{  }


const GPlatesMaths::UnitVector3D
GPlatesMaths::FiniteRotation::operator*(
		const UnitVector3D &unit_vect) const
{
	// Re-use the operator associated with 'Vector3D'.
	Vector3D v_rot = operator*(Vector3D(unit_vect));

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

	// NOTE: We don't check validity because we've already ensured unit magnitude above and
	// avoiding the validity check improves CPU performance quite noticeably.
	// Now the CPU time is spent mostly in the quaternion-vector multiply above instead of being
	// dwarfed by the validity check.
	return UnitVector3D(v_rot.x(), v_rot.y(), v_rot.z(), false/*check_validity*/);
}


const GPlatesMaths::Vector3D
GPlatesMaths::FiniteRotation::operator*(
		const Vector3D &vect) const
{
	const Vector3D &uq_v = d_unit_quat.vector_part();

	return d_d * vect + (2.0 * dot(uq_v, vect)) * uq_v + cross(d_e, vect);
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

		// Since q and -q map to the same rotation (where 'q' is any quaternion) it's possible that
		// q1 and q2 could be separated by a longer path than are q1 and -q2 (or -q1 and q2).
		// So check if we're using the longer path and negate either quaternion in order to
		// take the shorter path.
		//
		// See the "Quaternion Slerp" section of http://en.wikipedia.org/wiki/Slerp
		real_t shortest_path_correction = 1;
		if (cos_theta.is_precisely_less_than(0))
		{
			cos_theta = -cos_theta;

			// NOTE: We really should be negating one of the two quaternions (q1 or q2 - it doesn't
			// matter which one) but it's easier, and faster, to negate one of the interpolation
			// coefficients since the quaternions are multiplied by them (q = c1 * q1 + c2 * q2).
			shortest_path_correction = -1;
		}

		if (cos_theta >= 1.0) {

			/*
			 * The two quaternions are, as far as we're concerned,
			 * identical.  Trying to slerp these suckers will lead
			 * to Infs, NaNs and heart-ache.
			 */
			return q1;
		}

		// Since cos(theta) lies in the range (-1, 1), theta will lie
		// in the range (0, PI).
		const real_t theta = acos(cos_theta);

		// Since theta lies in the range (0, PI), sin(theta) will lie
		// in the range (0, 1].
		//
		// Since |cos(theta)| lies in the range [0, 1), cos^2(theta)
		// will lie in the range [0, 1), so (1 - cos^2(theta)) will lie
		// in the range (0, 1], so sqrt(1 - cos^2(theta)) lie in the
		// range (0, 1], and hence can be used in place of sin(theta)
		// without any sign/quadrant issues.
		//
		// And finally, since sqrt(1 - cos^2(theta)) lies in the range
		// (0, 1], there won't be any division by zero.
		const real_t one_on_sin_theta =
		 1.0 / sqrt(1.0 - cos_theta * cos_theta);

		const real_t c1 = sin((1.0 - t) * theta) * one_on_sin_theta;
		const real_t c2 = sin(t * theta) * one_on_sin_theta;

		return UnitQuaternion3D::create(c1 * q1 + shortest_path_correction * c2 * q2);
	}

}


const GPlatesMaths::FiniteRotation
GPlatesMaths::interpolate(
		const FiniteRotation &r1,
		const FiniteRotation &r2,
		const real_t &t1,
		const real_t &t2,
		const real_t &t_target,
		const boost::optional<UnitVector3D> &axis_hint)
{
	if (t1 == t2) {

		throw IndeterminateResultException(GPLATES_EXCEPTION_SOURCE,
				"Attempted to interpolate "
				"between two finite rotations using a zero-length interval.");
	}
	real_t interpolation_parameter = (t_target - t1) / (t2 - t1);
	UnitQuaternion3D res_uq = ::slerp(r1.unit_quat(), r2.unit_quat(), interpolation_parameter);

	return FiniteRotation::create(res_uq, axis_hint);
}


const GPlatesMaths::FiniteRotation
GPlatesMaths::interpolate(
		const FiniteRotation &r1,
		const FiniteRotation &r2,
		const FiniteRotation &r3,
		const real_t &w1,
		const real_t &w2,
		const real_t &w3)
{
	const real_t w2_plus_w3 = w2 + w3;

	// The weights must sum to 1.0.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			w1 + w2_plus_w3 == 1,
			GPLATES_ASSERTION_SOURCE);

	const UnitQuaternion3D res_uq = 
		::slerp(
			::slerp(r1.unit_quat(), r2.unit_quat(), w2_plus_w3),
			::slerp(r1.unit_quat(), r3.unit_quat(), w2_plus_w3),
			w3 / w2_plus_w3);

	return FiniteRotation::create(res_uq, boost::none);
}

const GPlatesMaths::FiniteRotation
GPlatesMaths::compose(
		const FiniteRotation &r1,
		const FiniteRotation &r2)
{
	UnitQuaternion3D resultant_uq = r1.unit_quat() * r2.unit_quat();

	// If either of the finite rotations has an axis hint, use it.
	boost::optional<GPlatesMaths::UnitVector3D> axis_hint;
	if (r1.axis_hint()) {
		axis_hint = r1.axis_hint();
	} else if (r2.axis_hint()) {
		axis_hint = r2.axis_hint();
	}

	return FiniteRotation::create(resultant_uq, axis_hint);
}


const GPlatesMaths::FiniteRotation
GPlatesMaths::compose(
		const Rotation &r,
		const FiniteRotation &fr)
{
	UnitQuaternion3D resultant_uq = r.quat() * fr.unit_quat();

	// Are we interested in the axis hint of the Rotation?  I think not,
	// since surely it is an arbitrary result of the manipulation...
	// Hence, we're only interested in the axis hint (if there is one) of
	// the FiniteRotation.
	return FiniteRotation::create(resultant_uq, fr.axis_hint());
}


const GPlatesMaths::PointOnSphere
GPlatesMaths::operator*(
		const FiniteRotation &r,
		const PointOnSphere &p)
{

	return PointOnSphere(r * p.position_vector());
}


const GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::PointOnSphere>
GPlatesMaths::operator*(
		const FiniteRotation &r,
		const GPlatesUtils::non_null_intrusive_ptr<const PointOnSphere> &p)
{
	return PointOnSphere::create_on_heap(r * p->position_vector());
}


const GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::MultiPointOnSphere>
GPlatesMaths::operator*(
		const FiniteRotation &r,
		const GPlatesUtils::non_null_intrusive_ptr<const MultiPointOnSphere> &mp)
{
	std::vector<PointOnSphere> rotated_points;
	rotated_points.reserve(mp->number_of_points());

	MultiPointOnSphere::const_iterator iter = mp->begin();
	MultiPointOnSphere::const_iterator end = mp->end();
	for ( ; iter != end; ++iter)
	{
		rotated_points.push_back(PointOnSphere(r * iter->position_vector()));
	}

	return MultiPointOnSphere::create_on_heap(rotated_points);
}


const GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::PolylineOnSphere>
GPlatesMaths::operator*(
		const FiniteRotation &r,
		const GPlatesUtils::non_null_intrusive_ptr<const PolylineOnSphere> &p)
{
	std::vector<PointOnSphere> rotated_points;
	rotated_points.reserve(p->number_of_vertices());

	PolylineOnSphere::vertex_const_iterator iter = p->vertex_begin();
	PolylineOnSphere::vertex_const_iterator end = p->vertex_end();
	for ( ; iter != end; ++iter) {
		rotated_points.push_back(PointOnSphere(r * iter->position_vector()));
	}

	return PolylineOnSphere::create_on_heap(rotated_points);
}


const GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::PolygonOnSphere>
GPlatesMaths::operator*(
		const FiniteRotation &r,
		const GPlatesUtils::non_null_intrusive_ptr<const PolygonOnSphere> &p)
{
	std::vector<PointOnSphere> rotated_points;
	rotated_points.reserve(p->number_of_vertices());

	PolygonOnSphere::vertex_const_iterator iter = p->vertex_begin();
	PolygonOnSphere::vertex_const_iterator end = p->vertex_end();
	for ( ; iter != end; ++iter) {
		rotated_points.push_back(PointOnSphere(r * iter->position_vector()));
	}

	return PolygonOnSphere::create_on_heap(rotated_points);
}


const GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::GeometryOnSphere>
GPlatesMaths::operator*(
		const FiniteRotation &r,
		const GPlatesUtils::non_null_intrusive_ptr<const GeometryOnSphere> &g)
{
	return RotateGeometryOnSphere(r).rotate(g);
}


const GPlatesMaths::GreatCircleArc
GPlatesMaths::operator*(
		const FiniteRotation &r,
		const GreatCircleArc &g)
{
	return GreatCircleArc::create_rotated_arc(r, g);
}


const GPlatesMaths::GreatCircle
GPlatesMaths::operator*(
		const FiniteRotation &r,
		const GreatCircle &g)
{
	UnitVector3D axis = r * g.axis_vector();
	return GreatCircle(axis);
}


const GPlatesMaths::SmallCircle
GPlatesMaths::operator*(
		const FiniteRotation &r,
		const SmallCircle &s)
{
	UnitVector3D axis = r * s.axis_vector();
	return SmallCircle::create_cosine_colatitude(axis, s.cos_colatitude());
}


std::ostream &
GPlatesMaths::operator<<(
		std::ostream &os,
		const FiniteRotation &fr)
{
	os << "(rot = ";

	const UnitQuaternion3D &uq = fr.unit_quat();
	if (represents_identity_rotation(uq)) {

		os << "identity";

	} else {

		UnitQuaternion3D::RotationParams params = uq.get_rotation_params(fr.axis_hint());

		PointOnSphere p(params.axis);  // the point
		PointOnSphere antip( -p.position_vector());  // the antipodal point

		os
		 << "(pole = "
		 << make_lat_lon_point(p)
		 << " (which is antipodal to "
		 << make_lat_lon_point(antip)
		 << "); angle = "
		 << convert_rad_to_deg(params.angle)
		 << " deg)";
	}
	os << ")";

	return os;
}


namespace
{
	template<class T>
	bool
	opt_eq(
			const boost::optional<T> &opt1,
			const boost::optional<T> &opt2)
	{
		if (opt1)
		{
			if (!opt2)
			{
				return false;
			}
			return *opt1 == *opt2;
		}
		else
		{
			return !opt2;
		}
	}
}


bool
GPlatesMaths::FiniteRotation::operator==(
		const FiniteRotation &other) const
{
	return d_unit_quat == other.d_unit_quat &&
		opt_eq(d_axis_hint, other.d_axis_hint);
}

