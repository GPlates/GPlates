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

#include "scribe/Scribe.h"


namespace
{
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
				GPlatesMaths::PointGeometryOnSphere::non_null_ptr_to_const_type point_on_sphere)
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
GPlatesMaths::FiniteRotation::create_great_circle_point_rotation(
		const PointOnSphere &from_point,
		const PointOnSphere &to_point)
{
	const Vector3D rotation_axis = cross(from_point.position_vector(), to_point.position_vector());

	// If the points are the same or antipodal then there are an infinite number of rotation axes
	// possible, so we just pick one arbitrarily.
	const PointOnSphere pole = rotation_axis.is_zero_magnitude()
			? PointOnSphere(generate_perpendicular(from_point.position_vector()))
			: PointOnSphere(rotation_axis.get_normalisation());

	const real_t angle = acos(dot(from_point.position_vector(), to_point.position_vector()));

	return create(pole, angle);
}


const GPlatesMaths::FiniteRotation
GPlatesMaths::FiniteRotation::create_small_circle_point_rotation(
		const PointOnSphere &rotation_pole,
		const PointOnSphere &from_point,
		const PointOnSphere &to_point)
{
	// Get the rotation axes of the arcs from the rotation pole to the 'from' and 'to' points.
	const Vector3D from_rotation_axis = cross(rotation_pole.position_vector(), from_point.position_vector());
	const Vector3D to_rotation_axis = cross(rotation_pole.position_vector(), to_point.position_vector());

	// If either rotation axis is zero magnitude then we cannot determine both 'from' and 'to' point
	// orientations relative to the rotation pole, so just return the identity rotation.
	// This means one or both points lie on the rotation pole.
	if (from_rotation_axis.is_zero_magnitude() ||
		to_rotation_axis.is_zero_magnitude())
	{
		return create_identity_rotation();
	}

	// The angle between the rotation axes is the angle we need to rotate.
	// This is the orientation of the 'to' point relative to the 'from' point with respect to the rotation pole.
	real_t angle = acos(dot(from_rotation_axis.get_normalisation(), to_rotation_axis.get_normalisation()));

	// Positive rotation angles rotate counter-clockwise so if we need to rotate clockwise then negate angle.
	if (dot(from_rotation_axis, to_point.position_vector()).dval() < 0)
	{
		angle = -angle;
	}

	return create(rotation_pole, angle);
}


const GPlatesMaths::FiniteRotation
GPlatesMaths::FiniteRotation::create_segment_rotation(
		const PointOnSphere &from_segment_start,
		const PointOnSphere &from_segment_end,
		const PointOnSphere &to_segment_start,
		const PointOnSphere &to_segment_end)
{
	// First rotate the start point of the 'from' segment to the start point of the 'to' segment.
	//
	// There are an infinite number of possible rotations (all with rotation poles on the great circle
	// that separates the two points). We can pick any, so the easiest is the rotation that moves
	// along the great circle arc between the two points.
	const FiniteRotation rotate_from_segment_start_to_segment_start =
			create_great_circle_point_rotation(from_segment_start, to_segment_start);

    // So far we can rotate the start point of the 'from' segment onto the start point of the 'to' segment.
    // However if we use that rotation to rotate the end point of the 'from' segment then it will not land
    // on the end point of the 'to' segment.
    // So we need to further rotate it by another rotation to get to the end point of the 'to' segment.
    // That extra rotation rotates around the start point of the 'to' segment until the result lands on
    // the end point of the 'to' segment.
    //
    // Note that since it's a rotation around the start point of the 'to' segment it doesn't affect
    // the start point of the 'to' segment and so it won't mess up our final composed rotation of the
    // start point of the 'from' segment onto the start point of the 'to' segment.
	const FiniteRotation rotate_rotated_from_segment_end_to_segment_end =
			create_small_circle_point_rotation(
					to_segment_start/*rotation_pole*/,
					rotate_from_segment_start_to_segment_start * from_segment_end/*from_point*/,
					to_segment_end/*to_point*/);

	return compose(rotate_rotated_from_segment_end_to_segment_end, rotate_from_segment_start_to_segment_start);
}


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
	const GPlatesMaths::real_t &uq_s = d_unit_quat.scalar_part();
	const Vector3D &uq_v = d_unit_quat.vector_part();

	//
	// Quaternion (uq_s, uq_v) rotates vector v to v' as:
	//
	//   v' = v + 2 * uq_v x (uq_s * v + uq_v x v)
	//
	// ...and using the vector triple product rule:
	//
	//   a x (b x c) = (a.c)b - (a.b)c
	//
	// ...we get:
	//
	//   v' = v + 2 * uq_s * uq_v x v + 2 * uq_v x (uq_v x v)
	//      = v + 2 * uq_s * uq_v x v + 2 * (uq_v . v) * uq_v - 2 * (uq_v . uq_v) * v
	//      = (1 - 2 * (uq_v . uq_v)) * v + 2 * uq_s * uq_v x v + 2 * (uq_v . v) * uq_v
	//
	// ...and using the norm of a unit quaternion:
	//
	//   uq_s * uq_s + uq_v . uq_v = 1
	//                 uq_v . uq_v = 1 - uq_s * uq_s
	//       1 - 2 * (uq_v . uq_v) = 1 - 2 * (1 - uq_s * uq_s)
	//                             = 2 * uq_s * uq_s - 1
	//
	// ...we get:
	//
	//   v' = (1 - 2 * (uq_v . uq_v)) * v + 2 * uq_s * uq_v x v + 2 * (uq_v . v) * uq_v
	//      = (2 * uq_s * uq_s - 1) * v + 2 * uq_s * uq_v x v + 2 * (uq_v . v) * uq_v
	//      = (2 * uq_s * uq_s - 1) * v + 2 * [uq_s * uq_v x v + (uq_v . v) * uq_v]
	//
	return (2.0 * uq_s * uq_s - 1.0) * vect + 2.0 * (cross(uq_s * uq_v, vect) + dot(uq_v, vect) * uq_v);
}


GPlatesScribe::TranscribeResult
GPlatesMaths::FiniteRotation::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<FiniteRotation> &finite_rotation)
{
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, finite_rotation->d_unit_quat, "unit_quat");
		scribe.save(TRANSCRIBE_SOURCE, finite_rotation->d_axis_hint, "axis_hint");
	}
	else // loading
	{
		GPlatesScribe::LoadRef<UnitQuaternion3D> unit_quat_ = scribe.load<UnitQuaternion3D>(TRANSCRIBE_SOURCE, "unit_quat");
		if (!unit_quat_.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		GPlatesScribe::LoadRef<boost::optional<UnitVector3D>> axis_hint_ = scribe.load<boost::optional<UnitVector3D>>(TRANSCRIBE_SOURCE, "axis_hint");
		if (!axis_hint_.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		finite_rotation.construct_object(unit_quat_, axis_hint_);
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesMaths::FiniteRotation::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_unit_quat, "unit_quat"))
		{
			return scribe.get_transcribe_result();
		}

		if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_axis_hint, "axis_hint"))
		{
			return scribe.get_transcribe_result();
		}
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
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

		// Since q and -q both rotate a point to the same final position (where 'q' is any quaternion)
		// it's possible that q1 and q2 could be separated by a longer path than are q1 and -q2 (or -q1 and q2).
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
		const real_t &interpolate_ratio)
{
	UnitQuaternion3D res_uq = ::slerp(r1.unit_quat(), r2.unit_quat(), interpolate_ratio);

	return FiniteRotation::create(res_uq, boost::none);
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


const GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::MultiPointOnSphere>
GPlatesMaths::operator*(
		const FiniteRotation &r,
		const GPlatesUtils::non_null_intrusive_ptr<const MultiPointOnSphere> &mp)
{
	// TODO: If there are enough points, convert quaternion to 3x3 matrix and
	//       use that to rotate all points (since it's cheaper). There needs to
	//       be enough points to cover the cost of converting quaternion to a matrix.

	std::vector<PointOnSphere> rotated_points;
	rotated_points.reserve(mp->number_of_points());

	MultiPointOnSphere::const_iterator iter = mp->begin();
	MultiPointOnSphere::const_iterator end = mp->end();
	for ( ; iter != end; ++iter)
	{
		rotated_points.push_back(PointOnSphere(r * iter->position_vector()));
	}

	return MultiPointOnSphere::create(rotated_points);
}


const GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::PolylineOnSphere>
GPlatesMaths::operator*(
		const FiniteRotation &r,
		const GPlatesUtils::non_null_intrusive_ptr<const PolylineOnSphere> &p)
{
	// TODO: If there are enough points, convert quaternion to 3x3 matrix and
	//       use that to rotate all points (since it's cheaper). There needs to
	//       be enough points to cover the cost of converting quaternion to a matrix.

	std::vector<PointOnSphere> rotated_points;
	rotated_points.reserve(p->number_of_vertices());

	PolylineOnSphere::vertex_const_iterator iter = p->vertex_begin();
	PolylineOnSphere::vertex_const_iterator end = p->vertex_end();
	for ( ; iter != end; ++iter) {
		rotated_points.push_back(PointOnSphere(r * iter->position_vector()));
	}

	return PolylineOnSphere::create(rotated_points);
}


const GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::PolygonOnSphere>
GPlatesMaths::operator*(
		const FiniteRotation &r,
		const GPlatesUtils::non_null_intrusive_ptr<const PolygonOnSphere> &p)
{
	// TODO: If there are enough points, convert quaternion to 3x3 matrix and
	//       use that to rotate all points (since it's cheaper). There needs to
	//       be enough points to cover the cost of converting quaternion to a matrix.

	std::vector<PointOnSphere> rotated_exterior_ring;
	rotated_exterior_ring.reserve(p->number_of_vertices_in_exterior_ring());

	// Rotate the exterior ring.
	PolygonOnSphere::ring_vertex_const_iterator exterior_iter = p->exterior_ring_vertex_begin();
	PolygonOnSphere::ring_vertex_const_iterator exterior_end = p->exterior_ring_vertex_end();
	for ( ; exterior_iter != exterior_end; ++exterior_iter)
	{
		rotated_exterior_ring.push_back(PointOnSphere(r * exterior_iter->position_vector()));
	}

	const unsigned int num_interior_rings = p->number_of_interior_rings();
	if (num_interior_rings == 0)
	{
		return PolygonOnSphere::create(rotated_exterior_ring);
	}

	std::vector< std::vector<PointOnSphere> > rotated_interior_rings(num_interior_rings);

	// Rotate the interior rings.
	for (unsigned int interior_ring_index = 0; interior_ring_index < num_interior_rings; interior_ring_index++)
	{
		rotated_interior_rings[interior_ring_index].reserve(p->number_of_vertices_in_interior_ring(interior_ring_index));

		PolygonOnSphere::ring_vertex_const_iterator interior_iter = p->interior_ring_vertex_begin(interior_ring_index);
		PolygonOnSphere::ring_vertex_const_iterator interior_end = p->interior_ring_vertex_end(interior_ring_index);
		for ( ; interior_iter != interior_end; ++interior_iter)
		{
			rotated_interior_rings[interior_ring_index].push_back(PointOnSphere(r * interior_iter->position_vector()));
		}
	} // loop over interior rings

	return PolygonOnSphere::create(rotated_exterior_ring, rotated_interior_rings);
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

