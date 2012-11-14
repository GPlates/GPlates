
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
#include "maths/UnitQuaternion3D.h"


using namespace GPlatesGlobal;

GPlatesMaths::Vector3D
GPlatesMaths::calculate_velocity_vector(
	const PointOnSphere &point, 
	const FiniteRotation &fr_t1,
	const FiniteRotation &fr_t2)
{

	static const real_t radius_of_earth = 6.378e8;  // in centimetres.

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
	UnitQuaternion3D q = fr_t1.unit_quat() * fr_t2.unit_quat().get_inverse();
	
	if ( represents_identity_rotation( q ) ) 
	{
		// The finite rotations must be identical.
		return Vector3D(0, 0, 0);
	}

	const UnitQuaternion3D::RotationParams params = q.get_rotation_params( fr_t1.axis_hint() );

	// Angular velocity of rotation (radians per million years).
	real_t omega = params.angle;

	// Axis of roation 
	UnitVector3D rotation_axis = params.axis;

	// Cartesian (x, y, z) velocity (cm/yr).
	Vector3D velocity_xyz = omega * (radius_of_earth * 1.0e-6) *
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
	Vector3D vector_ned = ccm * vector_xyz;

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
	return inverse_multiply( ccm , vector_ned );
}


std::pair< GPlatesMaths::real_t, GPlatesMaths::real_t >
GPlatesMaths::calculate_vector_components_magnitude_angle(
	const GPlatesMaths::PointOnSphere &point, 
	const Vector3D &vector_xyz)
{
	// Matrix to convert between different Cartesian representations.
	CartesianConvMatrix3D ccm(point);

	// Cartesian (n, e, d) 
	Vector3D vector_ned = ccm * vector_xyz;

	// real_t colat = -vector_ned.x();
	real_t lat   =  vector_ned.x();
	real_t lon   =  vector_ned.y();

	real_t angle = atan2( lat , lon );

	real_t magnitude = vector_ned.magnitude();

	return std::make_pair(magnitude, angle);
}






