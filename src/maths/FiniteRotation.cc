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
#include "HighPrecision.h"
#include "UnitVector3D.h"
#include "Vector3D.h"
#include "MathsUtils.h"
#include "MultiPointOnSphere.h"
#include "PointOnSphere.h"
#include "PolylineOnSphere.h"
#include "PolygonOnSphere.h"
#include "GridOnSphere.h"
#include "GreatCircleArc.h"
#include "GreatCircle.h"
#include "SmallCircle.h"
#include "LatLonPoint.h"
#include "InvalidOperationException.h"
#include "IndeterminateResultException.h"
#include "ConstGeometryOnSphereVisitor.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


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
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
				d_rotated_geometry;
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
		const UnitQuaternion3D &uq,
		const boost::optional<UnitVector3D> &axis_hint_)
{
	return FiniteRotation(uq, axis_hint_);
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
	Vector3D v(unit_vect);
	const Vector3D &uq_v = d_unit_quat.vector_part();

	Vector3D v_rot =
	 d_d * v +
	 (2.0 * dot(uq_v, v)) * uq_v +
	 cross(d_e, v);

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

			throw IndeterminateResultException(GPLATES_EXCEPTION_SOURCE,
					oss.str().c_str());
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
		const PointOnSphere &p) {

	UnitVector3D rotated_position_vector = r * p.position_vector();
	return PointOnSphere(rotated_position_vector);
}


const GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::PointOnSphere,
		GPlatesUtils::NullIntrusivePointerHandler>
GPlatesMaths::operator*(
		const FiniteRotation &r,
		GPlatesUtils::non_null_intrusive_ptr<const PointOnSphere,
				GPlatesUtils::NullIntrusivePointerHandler> p)
{
	UnitVector3D rotated_position_vector = r * p->position_vector();
	GPlatesUtils::non_null_intrusive_ptr<const PointOnSphere,
			GPlatesUtils::NullIntrusivePointerHandler> rotated_point(
			PointOnSphere::create_on_heap(rotated_position_vector));
	return rotated_point;
}


const GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::MultiPointOnSphere,
		GPlatesUtils::NullIntrusivePointerHandler>
GPlatesMaths::operator*(
		const FiniteRotation &r,
		GPlatesUtils::non_null_intrusive_ptr<const MultiPointOnSphere,
				GPlatesUtils::NullIntrusivePointerHandler> mp)
{
	std::vector<PointOnSphere> rotated_points;
	rotated_points.reserve(mp->number_of_points());

	MultiPointOnSphere::const_iterator iter = mp->begin();
	MultiPointOnSphere::const_iterator end = mp->end();
	for ( ; iter != end; ++iter) {
		rotated_points.push_back(r * (*iter));
	}

	GPlatesUtils::non_null_intrusive_ptr<const MultiPointOnSphere,
			GPlatesUtils::NullIntrusivePointerHandler> rotated_multipoint(
			MultiPointOnSphere::create_on_heap(rotated_points));
	return rotated_multipoint;
}


const GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::PolylineOnSphere,
		GPlatesUtils::NullIntrusivePointerHandler>
GPlatesMaths::operator*(
		const FiniteRotation &r,
		GPlatesUtils::non_null_intrusive_ptr<const PolylineOnSphere,
				GPlatesUtils::NullIntrusivePointerHandler> p)
{
	std::vector<PointOnSphere> rotated_points;
	rotated_points.reserve(p->number_of_vertices());

	PolylineOnSphere::vertex_const_iterator iter = p->vertex_begin();
	PolylineOnSphere::vertex_const_iterator end = p->vertex_end();
	for ( ; iter != end; ++iter) {
		rotated_points.push_back(r * (*iter));
	}

	GPlatesUtils::non_null_intrusive_ptr<const PolylineOnSphere,
			GPlatesUtils::NullIntrusivePointerHandler> rotated_polyline(
			PolylineOnSphere::create_on_heap(rotated_points));
	return rotated_polyline;
}


const GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::PolygonOnSphere,
		GPlatesUtils::NullIntrusivePointerHandler>
GPlatesMaths::operator*(
		const FiniteRotation &r,
		GPlatesUtils::non_null_intrusive_ptr<const PolygonOnSphere,
				GPlatesUtils::NullIntrusivePointerHandler> p)
{
	std::vector<PointOnSphere> rotated_points;
	rotated_points.reserve(p->number_of_vertices());

	PolygonOnSphere::vertex_const_iterator iter = p->vertex_begin();
	PolygonOnSphere::vertex_const_iterator end = p->vertex_end();
	for ( ; iter != end; ++iter) {
		rotated_points.push_back(r * (*iter));
	}

	GPlatesUtils::non_null_intrusive_ptr<const PolygonOnSphere,
			GPlatesUtils::NullIntrusivePointerHandler> rotated_polygon(
			PolygonOnSphere::create_on_heap(rotated_points));
	return rotated_polygon;
}


const GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::GeometryOnSphere,
		GPlatesUtils::NullIntrusivePointerHandler>
GPlatesMaths::operator*(
		const FiniteRotation &r,
		GPlatesUtils::non_null_intrusive_ptr<const GeometryOnSphere,
				GPlatesUtils::NullIntrusivePointerHandler> g)
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


const GPlatesMaths::GridOnSphere
GPlatesMaths::operator*(
		const FiniteRotation &r,
		const GridOnSphere &g)
{
	SmallCircle sc = r * g.lineOfLat();
	GreatCircle gc = r * g.lineOfLon();
	PointOnSphere p = r * g.origin();

	return GridOnSphere(sc, gc, p, g.deltaAlongLat(), g.deltaAlongLon());
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

