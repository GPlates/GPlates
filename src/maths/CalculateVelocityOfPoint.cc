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
#if 0
// GPlates 8
#include "state/Data.h"
#include "state/Layout.h"
#endif
#include "maths/StageRotation.h"
#include "maths/CartesianConvMatrix3D.h"


using namespace GPlatesGlobal;
#if 0
// GPlates 8
using namespace GPlatesState;
#endif


namespace
{
	enum status { SUCCESSFUL, CANNOT_BE_ROTATED };

	/**
	 * FIXME: this should be placed somewhere "more official".
	 */
	static const rid_t RIDOfGlobe = 0;

	/**
	 * A convenient alias for the operation of searching through a list
	 * for a particular element.
	 */
	template< typename T >
	inline bool
	ListContainsElem(const std::list< T > &l, const T &e) {

		return (std::find(l.begin(), l.end(), e) != l.end());
	}
}


#if 0
// GPlates 8
// GPlates 8
/**
 * Check whether the plate described by @a plate_id can be rotated to 
 * time @a t.
 *
 * If a plate can be rotated, the function will return 'SUCCESSFUL'.
 * Otherwise, it will return 'CANNOT_BE_ROTATED'.
 *
 * Additionally, when it is verified that the plate can be rotated to 
 * time @a t, the finite rotation to perform this rotation is 
 * calculated and stored in the map @a rot_cache.
 * 
 * This function operates recursively.  This is due to the fact that 
 * a plate depends on all the plates in the rotation hierarchy between 
 * itself and the globe.  Thus, in checking a plate X, all plates 
 * between X and the globe will also be checked.
 *
 * The operation of this function is sped up by caching negative 
 * results in the list @a cannot_be_rotated.  The map @a rot_cache 
 * serves as a cache of positive results.  This caching ensures that 
 * it is not necessary to traverse the hierarchy all the way to the 
 * globe every single time.
 */
static enum status
CheckRotation(
	rid_t plate_id, 
	GPlatesMaths::real_t t,
	GPlatesMaths::RotationsByPlate &rot_cache, 
	std::list< rid_t > &cannot_be_rotated, 
    const Data::RotationMap_type &histories) 
{

	using namespace GPlatesMaths;

	/*
	 * Firstly, check whether this plate can be rotated to time 't'.
	 *
	 * See if it can be found in the collection of rotation histories
	 * for each plate 
	 * (a map: plate id -> rotation history for that plate).
	 *
	 * If there is no match for the given plate id, this plate has no
	 * rotation history, and hence there is no way it can be rotated.
	 *
	 * If there IS a match, query the rotation history as to whether
	 * it is defined at time 't'.  If not, there is no rotation sequence
	 * defined at 't', and hence there is no way the plate can be
	 * rotated to a position at 't'.
	 */
	Data::RotationMap_type::const_iterator i = histories.find(plate_id);

	if (i == histories.end()) 
	{
		// this plate has no rotation history, so cannot rotate it
		cannot_be_rotated.push_back(plate_id);
		return CANNOT_BE_ROTATED;
	}
	if ( ! i->second.isDefinedAtTime(t)) 
	{
		// this plate's rotation history is not defined at this time
		cannot_be_rotated.push_back(plate_id);
		return CANNOT_BE_ROTATED;
	}

	/*
	 * Otherwise, we know that this plate has a rotation history defined
	 * at time 't', which means there must exist a finite rotation 
	 * for 't'.
	 *
	 * Now we need to check upwards in the rotation hierarchy, to ensure
	 * that those plates are also rotatable to time 't'.
	 */
	RotationHistory::const_iterator rot_seq = i->second.findAtTime(t);

	rid_t fixed_plate_id = (*rot_seq).fixedPlate();

	// Base case of recursion: 
	// if this plate is moving relative to globe.

	if (fixed_plate_id == RIDOfGlobe) 
	{
		/*
		 * The fixed plate is the globe, which is always defined,
		 * so the rotation of our plate is defined.
		 */
		FiniteRotation rot = (*rot_seq).finiteRotationAtTime(t);
		rot_cache.insert(std::make_pair(plate_id, rot));
		return SUCCESSFUL;
	}

	/*
	 * Now, check in caches to see if the fixed plate has already been
	 * dealt with.
	 */
	RotationsByPlate::const_iterator j = rot_cache.find(fixed_plate_id);

	if (j != rot_cache.end()) 
	{
		/*
		 * The fixed plate has already been dealt with, and is known
		 * to be rotatable, so the rotation of our plate is defined.
		 */
		FiniteRotation rot = (*rot_seq).finiteRotationAtTime(t);
		rot_cache.insert(std::make_pair(plate_id, j->second * rot));
		return SUCCESSFUL;
	}
	if (ListContainsElem(cannot_be_rotated, fixed_plate_id)) 
	{
		/*
		 * The fixed plate (or some plate in the hierarchy above it)
		 * is known not to be rotatable.
		 */
		cannot_be_rotated.push_back(plate_id);
		return CANNOT_BE_ROTATED;
	}

	/*
	 * There is no cached result, so we must query this the hard way
	 * (recursively).
	 */
	enum status query_status = 
		CheckRotation(
			fixed_plate_id, 
			t, 
			rot_cache, 
			cannot_be_rotated, 
			histories);

	if (query_status == CANNOT_BE_ROTATED) 
	{
		/*
		 * The fixed plate (or some plate in the hierarchy above it)
		 * has been determined to be not rotatable.
		 */
		cannot_be_rotated.push_back(plate_id);
		return CANNOT_BE_ROTATED;
	}
	// else, the fixed plate has been determined to be rotatable

	/*
	 * Since the fixed plate is rotatable, it must be in the 
	 * rotation cache, so this search should not fail.
	 */

	RotationsByPlate::const_iterator k = rot_cache.find(fixed_plate_id);

	// FIXME: check that (k != rot_cache.end()), and if so, throw an
	// exception for an internal error.

	FiniteRotation rot = (*rot_seq).finiteRotationAtTime(t);

	rot_cache.insert(std::make_pair(plate_id, k->second * rot));

	return SUCCESSFUL;
}

#endif 

#if 0
// GPlates 8
// GPlates 8

void
GPlatesMaths::PopulateRotatableData(
	RotationsByPlate &rot_cache, const real_t &t) 
{
	std::list< rid_t > cannot_be_rotated;

	/*
	 * If Data::GetRotationHistories() is NULL, 
	 * then there are no rotations to populate with.
	 */
	Data::RotationMap_type *histories = Data::GetRotationHistories();

	if (histories == NULL) 
	{
		// no rotations
		return;
	}

	Data::RotationMap_type::const_iterator i = histories->begin();

	for (; i != histories->end(); i++) 
	{
		// get the plate id
		rid_t plate_id = i->first;

		// Check whether this plate can be rotated to time 't'
		CheckRotation(plate_id, t, rot_cache, cannot_be_rotated, 
			*histories);
	}
}

#endif


std::pair< GPlatesMaths::real_t, GPlatesMaths::real_t >
GPlatesMaths::CalculateVelocityOfPoint(
	const PointOnSphere &point, 
	const rid_t &plate_id, 
	const RotationsByPlate &rotations_for_time_t1,
	const RotationsByPlate &rotations_for_time_t2, 
	PointOnSphere &rotated_point)
{

	static const real_t radius_of_earth = 6.378e8;  // in centimetres.

	RotationsByPlate::const_iterator iter_t1 =
	 rotations_for_time_t1.find(plate_id);

	RotationsByPlate::const_iterator iter_t2 =
	 rotations_for_time_t2.find(plate_id);

	if (iter_t1 == rotations_for_time_t1.end() ||
	    iter_t2 == rotations_for_time_t2.end() ) 
	{
		// Rotation undefined at one (or both) of the times.
		return std::make_pair(0, 0);
	}

	// Finite Rotations for each time
	FiniteRotation fr_t1 = iter_t1->second;
	FiniteRotation fr_t2 = iter_t2->second;

	
	// Get the StageRotation for time 1 - time 2
	//
	// This will give correct velocities for moving FORWARD in time,
	// For example: t1 = 10 Ma, t2 = 11 Ma, 
	// sr.timeDelta == -1 M years in age
	// or +1 M yr forward in time

	StageRotation sr = subtractFiniteRots(fr_t1, fr_t2);

//
// FIX : the delta is usually - 1.0 M years in age
// FIX : will be variable in the future 
//

	// Double-check that time-delta is one million years.
	if (sr.timeDelta() != -1.0) 
	{
		std::ostringstream oss;
		oss << "Time-delta was "
		 << sr.timeDelta()
		 << " million years, expected one million years.";
		throw IllegalParametersException(oss.str().c_str());
	}
	// Else, the finite rotations are 1M years apart (as expected).

	// This quaternion represents a rotation over a million years.
	UnitQuaternion3D q = sr.unit_quat();

	if ( represents_identity_rotation( q ) ) 
	{
		// The finite rotations must be identical.
		return std::make_pair(0, 0);
	}

	UnitQuaternion3D::RotationParams params = q.get_rotation_params();

	// Angular velocity of rotation (radians per million years).
	real_t omega = params.angle;

	// Axis of roation 
	UnitVector3D rotation_axis = params.axis;


	// Get the vector scale factor
	real_t factor( Data::GetVelocityVectorsScaleFactor() );

	// Build a new scaled stage rotation for 2 Ma 
	StageRotation sr_scaled = scaleToNewTimeDelta(sr, factor);
	
	// Build a finite rotatation from the stage rotation values
	GPlatesMaths::FiniteRotation fr_from_sr = 
		GPlatesMaths::FiniteRotation::create( 
			sr_scaled.unit_quat(), 0.0);

	// Get the reverse rotation
	GPlatesMaths::FiniteRotation rev_rot =
		GPlatesMaths::get_reverse( fr_from_sr );
	
	// Rotate the point in question with a Finite Rotation 
	// built from scaled Stage Rotation 
	rotated_point = rev_rot * point;

	// Cartesian (x, y, z) velocity (cm/yr).
	Vector3D velocity_xyz = omega * (radius_of_earth * 1.0e-6) *
		cross(rotation_axis, point.unitvector());


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
std::cout << "CALC: rot_pnt_vect = " << rotated_point << std::endl;
std::cout << "CALC: velocity_xyz = " << velocity_xyz << std::endl;
std::cout << "CALC: velocity_ned = " << velocity_ned << std::endl;
std::cout << std::endl;
#endif

	return std::make_pair(colat_v, lon_v);
}


