/* $Id: CalculateVelocityOfPoint.cc,v 1.1.4.6 2006/03/14 00:29:01 mturner Exp $ */

/**
 * @file 
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


#include <list>
#include <algorithm>  /* std::find */
#include <sstream>

#include "CalculateVelocityOfPoint.h"
#include "global/IllegalParametersException.h"
#include "maths/CartesianConvMatrix3D.h"
#include "maths/UnitQuaternion3D.h"


using namespace GPlatesGlobal;

std::pair< GPlatesMaths::real_t, GPlatesMaths::real_t >
GPlatesMaths::CalculateVelocityOfPoint(
	const PointOnSphere &point, 
	FiniteRotation fr_t1,
	FiniteRotation fr_t2)
{

	static const real_t radius_of_earth = 6.378e8;  // in centimetres.

	// This quaternion represents a rotation between t1 and t2
	UnitQuaternion3D q = fr_t2.unit_quat().get_inverse() * fr_t1.unit_quat();
	
	if ( represents_identity_rotation( q ) ) 
	{
		// The finite rotations must be identical.
		return std::make_pair(0, 0);
	}

	const UnitQuaternion3D::RotationParams params = q.get_rotation_params( fr_t1.axis_hint() );

	// Angular velocity of rotation (radians per million years).
	real_t omega = params.angle;

	// Axis of roation 
	UnitVector3D rotation_axis = params.axis;

	// Cartesian (x, y, z) velocity (cm/yr).
	Vector3D velocity_xyz = omega * (radius_of_earth * 1.0e-6) *
		cross(rotation_axis, point.position_vector() );

	// Matrix to convert between different Cartesian representations.
	CartesianConvMatrix3D ccm(point);

	// Cartesian (n, e, d) velocity (cm/yr).
	Vector3D velocity_ned = ccm * velocity_xyz;

	real_t colat_v = -velocity_ned.x();
	real_t lon_v =    velocity_ned.y();

#if 0
// FIX : remove this diagnostic output
std::cout << std::endl;
std::cout << "CALC: omega        = " << omega << std::endl;
std::cout << "CALC: rotation_axis= " << rotation_axis << std::endl;
std::cout << "CALC: point_vector = " << point << std::endl;
std::cout << "CALC: velocity_xyz = " << velocity_xyz << std::endl;
std::cout << "CALC: velocity_ned = " << velocity_ned << std::endl;
std::cout << "CALC: colat_v      = " << colat_v << std::endl;
std::cout << "CALC: lon_v        = " << lon_v << std::endl;
std::cout << std::endl;
#endif

	return std::make_pair(colat_v, lon_v);
}



