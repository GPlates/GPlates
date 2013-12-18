/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#include "RotationUtils.h"

#include "ReconstructionTree.h"


GPlatesMaths::FiniteRotation
GPlatesAppLogic::RotationUtils::get_half_stage_rotation(
		const ReconstructionTree &reconstruction_tree,
		GPlatesModel::integer_plate_id_type left_plate_id,
		GPlatesModel::integer_plate_id_type right_plate_id)
{
	//
	// Rotation from present day (0Ma) to current reconstruction time 't' of mid-ocean ridge MOR
	// with left/right plate ids 'L' and 'R':
	//
	// R(0->t,A->MOR)
	// R(0->t,A->L) * R(0->t,L->MOR)
	// R(0->t,A->L) * Half[R(0->t,L->R)] // Assumes L->R spreading from 0->t1 *and* t1->t2
	// R(0->t,A->L) * Half[R(0->t,L->A) * R(0->t,A->R)]
	// R(0->t,A->L) * Half[inverse[R(0->t,A->L)] * R(0->t,A->R)]
	//
	// ...where 'A' is the anchor plate id.
	//

	using namespace GPlatesMaths;

	FiniteRotation left_rotation = reconstruction_tree.get_composed_absolute_rotation(left_plate_id).first;
	FiniteRotation right_rotation = reconstruction_tree.get_composed_absolute_rotation(right_plate_id).first;

	// NOTE: Since q and -q map to the same rotation (where 'q' is any quaternion) it's possible
	// that left_q and right_q could be separated by a longer path than are left_q and -right_q
	// (or -left_q and right_q).
	// However we do *not* restrict ourselves to the shortest path (like 'FiniteRotation::interpolate()'
	// does in its SLERP routine). This is because the user (who created the total poles in the
	// rotation file) may want to take the longest path.
	//
	const FiniteRotation left_to_right = compose(get_reverse(left_rotation), right_rotation);

	if (represents_identity_rotation(left_to_right.unit_quat()))
	{
		return left_rotation;
	}

	UnitQuaternion3D::RotationParams left_to_right_params =
			left_to_right.unit_quat().get_rotation_params(boost::none);
	const real_t half_angle = 0.5 * left_to_right_params.angle;

	FiniteRotation half_rotation = 
			FiniteRotation::create(
					UnitQuaternion3D::create_rotation(
							left_to_right_params.axis, 
							half_angle),
							left_to_right.axis_hint());

	return compose(left_rotation, half_rotation);

	//
	// NOTE: The above algorithm works only if there is no motion of the right plate relative to
	// the left plate outside time intervals when ridge spreading is occurring because the algorithm
	// does not know when spreading is not occurring and just calculates the half-stage rotation
	// from the current reconstruction time back to present day (0Ma).
	//
	// The following example is for a mid-ocean ridge that only spreads between t1->t2:
	//
	// This assumes no spreading from 0->t1 and spreading from t1->t2...
	// R(0->t2,A->MOR)
	// R(0->t2,A->L) * R(0->t2,L->MOR)
	// R(0->t2,A->L) * R(t1->t2,L->MOR) * R(0->t1,L->MOR)
	// R(0->t2,A->L) * Half[R(t1->t2,L->R)] * R(0->t1,L->R) // No L->R spreading from 0->t1
	//
	// This (incorrectly) assumes spreading from 0->t2 (ie, both 0->t1 and t1->t2)...
	// R(0->t2,A->MOR)
	// R(0->t2,A->L) * R(0->t2,L->MOR)
	// R(0->t2,A->L) * Half[R(0->t2,L->R)] // Assumes L->R spreading from 0->t1 *and* t1->t2
	// R(0->t2,A->L) * Half[R(t1->t2,L->R) * R(0->t1,L->R)]
	//
	// Both equations above are only equivalent if there is no rotation between L and R from 0->t1.
	// Which happens if R(0->t1,L->R) is the identity rotation.
	// Note that the second equation is the one we actually use so it only works if the rotation
	// file has identity stage rotations between poles (of the MOR moving/fixed plate pair) where
	// no ridge spreading is occurring (ie, the two poles around an identity stage rotation are equal).
	//
}


GPlatesMaths::FiniteRotation
GPlatesAppLogic::RotationUtils::get_stage_pole(
	const GPlatesAppLogic::ReconstructionTree &reconstruction_tree_1,
	const GPlatesAppLogic::ReconstructionTree &reconstruction_tree_2,
	const GPlatesModel::integer_plate_id_type &moving_plate_id,
	const GPlatesModel::integer_plate_id_type &fixed_plate_id)
{
	//
	// Rotation from present day (0Ma) to time 't2' (via time 't1'):
	//
	// R(0->t2)  = R(t1->t2) * R(0->t1)
	// ...or by post-multiplying both sides by R(t1->0), and then swapping sides, this becomes...
	// R(t1->t2) = R(0->t2) * R(t1->0)
	//
	// Rotation from anchor plate 'A' to moving plate 'M' (via fixed plate 'F'):
	//
	// R(A->M) = R(A->F) * R(F->M)
	// ...or by pre-multiplying both sides by R(F->A) this becomes...
	// R(F->M) = R(F->A) * R(A->M)
	//
	// NOTE: The rotations for relative times and for relative plates have the opposite order of each other !
	// In other words:
	//   * For times 0->t1->t2 you apply the '0->t1' rotation first followed by the 't1->t2' rotation:
	//     R(0->t2)  = R(t1->t2) * R(0->t1)
	//   * For plate circuit A->F->M you apply the 'F->M' rotation first followed by the 'A->F' rotation:
	//     R(A->M) = R(A->F) * R(F->M)
	//     Note that this is not 'A->F' followed by 'F->M' as you might expect (looking at the time example).
	// This is probably best explained by the difference between thinking in terms of the grand fixed
	// coordinate system and local coordinate system (see http://glprogramming.com/red/chapter03.html#name2).
	// Essentially, in the plate circuit A->F->M, the 'F->M' rotation can be thought of as a rotation
	// within the local coordinate system of 'A->F'. In other words 'F->M' is not a rotation that
	// occurs relative to the global spin axis but a rotation relative to the local coordinate system
	// of plate 'F' *after* it has been rotated relative to the anchor plate 'A'.
	// For the times 0->t1->t2 this local/relative coordinate system concept does not apply.
	//
	// NOTE: A rotation must be relative to present day (0Ma) before it can be separated into
	// a (plate circuit) chain of moving/fixed plate pairs.
	// For example, the following is correct...
	//
	//    R(t1->t2,A->C)
	//       = R(0->t2,A->C) * R(t1->0,A->C)
	//       = R(0->t2,A->C) * inverse[R(0->t1,A->C)]
	//       // Now that all times are relative to 0Ma we can split A->C into A->B->C...
	//       = R(0->t2,A->B) * R(0->t2,B->C) * inverse[R(0->t1,A->B) * R(0->t1,B->C)]
	//       = R(0->t2,A->B) * R(0->t2,B->C) * inverse[R(0->t1,B->C)] * inverse[R(0->t1,A->B)]
	//
	// ...but the following is *incorrect*...
	//
	//    R(t1->t2,A->C)
	//       = R(t1->t2,A->B) * R(t1->t2,B->C)   // <-- This line is *incorrect*
	//       = R(0->t2,A->B) * R(t1->0,A->B) * R(0->t2,B->C) * R(t1->0,B->C)
	//       = R(0->t2,A->B) * inverse[R(0->t1,A->B)] * R(0->t2,B->C) * inverse[R(0->t1,B->C)]
	//
	// ...as can be seen above this gives two different results - the same four rotations are
	// present in each result but in a different order.
	// A->B->C means B->C is the rotation of C relative to B and A->B is the rotation of B relative to A.
	// The need for rotation A->C to be relative to present day (0Ma) before it can be split into
	// A->B and B->C is because A->B and B->C are defined (in the rotation file) as total reconstruction
	// poles which are always relative to present day.
	//
	//
	// So the stage rotation of moving plate relative to fixed plate and from time 't1' to time 't2':
	//
	// R(t1->t2,F->M)
	//    = R(0->t2,F->M) * R(t1->0,F->M)
	//    = R(0->t2,F->M) * inverse[R(0->t1,F->M)]
	//    = R(0->t2,F->A) * R(0->t2,A->M) * inverse[R(0->t1,F->A) * R(0->t1,A->M)]
	//    = inverse[R(0->t2,A->F)] * R(0->t2,A->M) * inverse[inverse[R(0->t1,A->F)] * R(0->t1,A->M)]
	//    = inverse[R(0->t2,A->F)] * R(0->t2,A->M) * inverse[R(0->t1,A->M)] * R(0->t1,A->F)
	//
	// ...where 'A' is the anchor plate, 'F' is the fixed plate and 'M' is the moving plate.

	// NOTE: Since q and -q map to the same rotation (where 'q' is any quaternion) it's possible
	// that left_q and right_q could be separated by a longer path than are left_q and -right_q
	// (or -left_q and right_q).
	// However we do *not* restrict ourselves to the shortest path (like 'FiniteRotation::interpolate()'
	// does in its SLERP routine). This is because the user (who created the total poles in the
	// rotation file) may want to take the longest path.
	//

	// For t1, get the rotation for plate M w.r.t. anchor	
	GPlatesMaths::FiniteRotation rot_0_to_t1_M = 
		reconstruction_tree_1.get_composed_absolute_rotation(moving_plate_id).first;

	// For t1, get the rotation for plate F w.r.t. anchor	
	GPlatesMaths::FiniteRotation rot_0_to_t1_F = 
		reconstruction_tree_1.get_composed_absolute_rotation(fixed_plate_id).first;


	// For t2, get the rotation for plate M w.r.t. anchor	
	GPlatesMaths::FiniteRotation rot_0_to_t2_M = 
		reconstruction_tree_2.get_composed_absolute_rotation(moving_plate_id).first;

	// For t2, get the rotation for plate F w.r.t. anchor	
	GPlatesMaths::FiniteRotation rot_0_to_t2_F = 
		reconstruction_tree_2.get_composed_absolute_rotation(fixed_plate_id).first;

	// Compose these rotations so that we get
	// the stage pole from time t1 to time t2 for plate M w.r.t. plate F.

	GPlatesMaths::FiniteRotation rot_t1 = 
		GPlatesMaths::compose(GPlatesMaths::get_reverse(rot_0_to_t1_F), rot_0_to_t1_M);

	GPlatesMaths::FiniteRotation rot_t2 = 
		GPlatesMaths::compose(GPlatesMaths::get_reverse(rot_0_to_t2_F), rot_0_to_t2_M);	

	GPlatesMaths::FiniteRotation stage_pole = 
		GPlatesMaths::compose(rot_t2,GPlatesMaths::get_reverse(rot_t1));	

	return stage_pole;	
}


boost::optional<GPlatesMaths::FiniteRotation>
GPlatesAppLogic::RotationUtils::calculate_short_path_final_rotation(
		const GPlatesMaths::FiniteRotation &final_rotation,
		const GPlatesMaths::FiniteRotation &initial_rotation)
{
	// Since q and -q map to the same rotation (where 'q' is any quaternion) it's possible
	// that q1 and q2 could be separated by a longer path than are q1 and -q2 (or -q1 and q2).
	// So check if we're using the longer path and negate the quaternion (of the
	// to-rotation) in order to take the shorter path.
	if (dot(initial_rotation.unit_quat(), final_rotation.unit_quat()).is_precisely_less_than(0))
	{
		// Note: We use the axis hint of the initial rotation (for the adjusted final rotation)
		// since, although this does not affect the quaternion at all, it does mean
		// the rotation parameters (lat/lon axis and angle) of the from and to rotations
		// come out consistent. For example when saving rotations back out to a PLATES
		// rotation format we would get something like...
		//
		// 902 190.0  51.75  112.91 -179.05  901
		// 902 200.0  48.81  113.3 -184.9  901
		//
		// ...instead of...
		//
		// 902 190.0  51.75  112.91 -179.05  901
		// 902 200.0 -48.81  -66.7  184.9  901
		//
		// ...both results are fine though, it's just that the first result has
		// continuous rotation angles (ie, no abrupt jump by ~360 degrees) and the
		// pole axes are roughly aligned (ie, not roughly antipodal) so its easier
		// for human readers to see the continuity.
		// However, both results still map to the exact same quaternions because
		// (axis, angle) and (-axis, -angle) map to the exact same quaternion.
		// So either result is perfectly fine as far as GPlates is concerned.
		// 
		return GPlatesMaths::FiniteRotation::create(
				-final_rotation.unit_quat(),
				initial_rotation.axis_hint());
	}

	// No adjustment was necessary.
	return boost::none;
}
