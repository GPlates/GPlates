
/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Author: mturner $
 *   $Date: 2006/03/14 00:29:01 $
 * 
 * Copyright (C) 2003, 2005, 2006 The University of Sydney, Australia.
 * Copyright (C) 2008, 2009 California Institute of Technology 
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


#include <list>
#include <algorithm>  /* std::find */
#include <sstream>

#include "CalculateVelocity.h"

#include "global/IllegalParametersException.h"

#include "maths/CartesianConvMatrix3D.h"
#include "maths/MathsUtils.h"
#include "maths/UnitQuaternion3D.h"

#include "utils/Earth.h"


using namespace GPlatesGlobal;

GPlatesMaths::Vector3D
GPlatesMaths::calculate_velocity_vector(
		const PointOnSphere &point, 
		const FiniteRotation &fr_t1,
		const FiniteRotation &fr_t2)
{
	const UnitQuaternion3D &q1 = fr_t1.unit_quat();
	const UnitQuaternion3D &q2 = fr_t2.unit_quat();

	// This quaternion represents a rotation from t2 to t1.
	//
	// Note the t1 is a more recent time (closer to present day) than t2.
	//
	// R(t2->t1,A->P)
	//    = R(0->t1,A->P) * R(t2->0,A->P)
	//    = R(0->t1,A->P) * inverse[R(0->t2,A->P)]
	//
	// ...where 'A' is the anchor plate and 'P' is the plate the point is in.
	//
	//
	// NOTE: Since q and -q map to the same rotation (where 'q' is any quaternion) it's possible
	// that q1 and q2 could be separated by a longer path than are q1 and -q2 (or -q1 and q2).
	// So check if we're using the longer path and negate either quaternion in order to
	// take the shorter path. It actually doesn't matter which one we negate.
	// We don't normally make this correction because it limits the user's (who creates total poles
	// in the rotation file) ability to select the short or the long path. However since the velocity
	// calculation uses two adjacent times (separated by 1Ma usually) then the shortest path should
	// be fine. And also the SLERP used in 'FiniteRotation::interpolate()' chooses the shortest path
	// between two adjacent total poles (two different times for the same plate) so the calculated
	// velocities should follow that interpolated motion anyway.
	//
	const UnitQuaternion3D q = dot(q1, q2).is_precisely_less_than(0)
			? q1 * (-q2).get_inverse()
			: q1 * q2.get_inverse();

	if ( represents_identity_rotation( q ) ) 
	{
		// The finite rotations must be identical.
		return Vector3D(0, 0, 0);
	}

	// The axis hint does not affect our results because, in our velocity calculation, the signs of
	// the axis and angle cancel each other out so it doesn't matter if axis/angle or -axis/-angle.
	const UnitQuaternion3D::RotationParams params = q.get_rotation_params(boost::none);

	// Angular velocity of rotation (radians per million years).
	real_t omega = params.angle;

	// Axis of roation 
	UnitVector3D rotation_axis = params.axis;

	// Cartesian (x, y, z) velocity (cm/yr).
	const Vector3D velocity_xyz =
			omega *
				(GPlatesUtils::Earth::EQUATORIAL_RADIUS_KMS * 1e-1/* kms/my -> cm/yr */) *
					cross(rotation_axis, point.position_vector() );

	return velocity_xyz;
}


GPlatesMaths::VectorColatitudeLongitude
GPlatesMaths::convert_vector_from_xyz_to_colat_lon(
		const GPlatesMaths::PointOnSphere &point, 
		const Vector3D &vector_xyz)
{
	// Matrix to convert between different Cartesian representations.
	CartesianConvMatrix3D ccm(point);

	// Cartesian (n, e, d) 
	Vector3D vector_ned = convert_from_geocentric_to_north_east_down(ccm, vector_xyz);

	real_t colat = -vector_ned.x();
	real_t lon =    vector_ned.y();

	return VectorColatitudeLongitude(colat, lon);
}


GPlatesMaths::Vector3D
GPlatesMaths::convert_vector_from_colat_lon_to_xyz(
		const PointOnSphere &point, 
		const VectorColatitudeLongitude &vector_colat_lon)
{
	// Create a new vector 3d from the components.
	GPlatesMaths::CartesianConvMatrix3D ccm(point);
	GPlatesMaths::Vector3D vector_ned(
			-vector_colat_lon.get_vector_colatitude(),
			vector_colat_lon.get_vector_longitude(),
			0);
	return convert_from_north_east_down_to_geocentric(ccm , vector_ned);
}


std::pair< GPlatesMaths::real_t, GPlatesMaths::real_t >
GPlatesMaths::calculate_vector_components_magnitude_angle(
		const GPlatesMaths::PointOnSphere &point, 
		const Vector3D &vector_xyz)
{
	// Matrix to convert between different Cartesian representations.
	CartesianConvMatrix3D ccm(point);

	// Cartesian (n, e, d) 
	Vector3D vector_ned = convert_from_geocentric_to_north_east_down(ccm, vector_xyz);

	real_t lat   =  vector_ned.x();
	real_t lon   =  vector_ned.y();

	// Note that this goes in the opposite direction from 'azimuth' and is -180/180 at West and is
	// counter-clockwise (South-wise), whereas 'azimuth' is 0/360 at North and is clockwise (East-wise).
	real_t angle = atan2( lat , lon );

	real_t magnitude = vector_ned.magnitude();

	return std::make_pair(magnitude, angle);
}


std::pair< GPlatesMaths::real_t, GPlatesMaths::real_t >
GPlatesMaths::calculate_vector_components_magnitude_and_azimuth(
		const GPlatesMaths::PointOnSphere &point, 
		const Vector3D &vector_xyz)
{
	const boost::tuple<real_t, real_t, real_t> magnitude_azimuth_inclination =
			convert_from_geocentric_to_magnitude_azimuth_inclination(
					CartesianConvMatrix3D(point),
					vector_xyz);

	return std::make_pair(
			boost::get<0>(magnitude_azimuth_inclination)/*magnitude*/,
			boost::get<1>(magnitude_azimuth_inclination)/*azimuth*/);
}


std::pair<GPlatesMaths::Vector3D, GPlatesMaths::real_t>
GPlatesMaths::calculate_velocity_vector_and_omega(
		const GPlatesMaths::PointOnSphere &point,
		const GPlatesMaths::FiniteRotation &fr_t1,
		const GPlatesMaths::FiniteRotation &fr_t2,
		const boost::optional<GPlatesMaths::UnitVector3D> &axis_hint)
{
	const UnitQuaternion3D &q1 = fr_t1.unit_quat();
	const UnitQuaternion3D &q2 = fr_t2.unit_quat();

	// This quaternion represents a rotation from t2 to t1.
	//
	// Note the t1 is a more recent time (closer to present day) than t2.
	//
	// R(t2->t1,A->P)
	//    = R(0->t1,A->P) * R(t2->0,A->P)
	//    = R(0->t1,A->P) * inverse[R(0->t2,A->P)]
	//
	// ...where 'A' is the anchor plate and 'P' is the plate the point is in.
	//
	//
	// NOTE: Since q and -q map to the same rotation (where 'q' is any quaternion) it's possible
	// that q1 and q2 could be separated by a longer path than are q1 and -q2 (or -q1 and q2).
	// So check if we're using the longer path and negate either quaternion in order to
	// take the shorter path. It actually doesn't matter which one we negate.
	// We don't normally make this correction because it limits the user's (who creates total poles
	// in the rotation file) ability to select the short or the long path. However since the velocity
	// calculation uses two adjacent times (separated by 1Ma usually) then the shortest path should
	// be fine. And also the SLERP used in 'FiniteRotation::interpolate()' chooses the shortest path
	// between two adjacent total poles (two different times for the same plate) so the calculated
	// velocities should follow that interpolated motion anyway.
	//
	const UnitQuaternion3D q = dot(q1, q2).is_precisely_less_than(0)
			? q1 * (-q2).get_inverse()
			: q1 * q2.get_inverse();

	if ( represents_identity_rotation( q ) )
	{
		// The finite rotations must be identical.
		return std::make_pair(Vector3D(0, 0, 0),0.);
	}

	// The axis hint does not affect our results because, in our velocity calculation, the signs of
	// the axis and angle cancel each other out so it doesn't matter if axis/angle or -axis/-angle.
	const UnitQuaternion3D::RotationParams params = q.get_rotation_params(axis_hint);

	// Angular velocity of rotation (radians per million years).
	real_t omega = params.angle;

	// Axis of roation
	UnitVector3D rotation_axis = params.axis;

	// Cartesian (x, y, z) velocity (cm/yr).
	const Vector3D velocity_xyz =
			omega *
				(GPlatesUtils::Earth::EQUATORIAL_RADIUS_KMS * 1e-1/* kms/my -> cm/yr */) *
					cross(rotation_axis, point.position_vector());

	return std::make_pair(velocity_xyz,omega);
}
