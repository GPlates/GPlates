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

#include "ReconstructionFeatureProperties.h"
#include "ReconstructionTree.h"
#include "ReconstructionTreeCreator.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "property-values/Enumeration.h"


GPlatesMaths::FiniteRotation
GPlatesAppLogic::RotationUtils::get_half_stage_rotation(
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const double &reconstruction_time,
		GPlatesModel::integer_plate_id_type left_plate_id,
		GPlatesModel::integer_plate_id_type right_plate_id,
		const double &spreading_asymmetry,
		const double &spreading_start_time,
		const double &half_stage_rotation_interval)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			half_stage_rotation_interval > 0,
			GPLATES_ASSERTION_SOURCE);

	//
	// Rotation from present day (0Ma) to current reconstruction time 't' of mid-ocean ridge MOR
	// with left/right plate ids 'L' and 'R':
	//
	// R(0->t,A->MOR)
	// R(0->t,A->L) * R(0->t,L->MOR)
	//
	// ...where 'A' is the anchor plate id.
	// R(0->t,L->MOR) is not directly obtainable from a reconstruction tree since there is no
	// reconstruction plate ID associated with the mid-ocean ridge (which is why we are here).
	// So we need to approximate it by dividing it into 'N' stages (the more stages the better accuracy).
	//
	// The stages are from 'spreading_start_time' to 'reconstruction_time'. When 'reconstruction_time'
	// coincides with 'spreading_start_time' the half-stage rotation includes only the left-plate
	// rotation (with no spreading added).
	// So the finite rotation is (where 'ts' is 'spreading_start_time'):
	//
	// R(0->t,A->MOR)
	// R(0->t,A->L) * R(0->t,L->MOR)
	// R(0->t,A->L) * R(ts->t,L->MOR) * R(0->ts,L->MOR)
	// R(0->t,A->L) * R(ts->t,L->MOR) // since R(0->ts,L->MOR) is zero, due to no spreading from 0 -> ts
	//
	// Another way of looking at this is to look at the entire finite rotation again:
	//
	// R(0->t,A->MOR)
	// R(ts->t,A->MOR) * R(0->ts,A->MOR)
	// R(ts->t,A->MOR) * R(0->ts,A->L) * R(0->ts,L->MOR)
	// R(ts->t,A->MOR) * R(0->ts,A->L)  // where R(0->ts,L->MOR) is zero, due to no spreading from 0 -> ts
	// R(0->t,A->MOR) * R(ts->0,A->MOR) * R(0->ts,A->L)
	// R(0->t,A->MOR) * inverse[R(0->ts,A->MOR)] * R(0->ts,A->L)
	// R(0->t,A->L) * R(0->t,L->MOR) * inverse[R(0->ts,A->L) * R(0->ts,L->MOR)] * R(0->ts,A->L)
	// R(0->t,A->L) * R(0->t,L->MOR) * inverse[R(0->ts,L->MOR)] * inverse[R(0->ts,A->L)] * R(0->v,A->L)
	// R(0->t,A->L) * R(0->t,L->MOR) * R(ts->0,L->MOR) * R(ts->0,A->L) * R(0->ts,A->L)
	// R(0->t,A->L) * R(ts->t,L->MOR)
	//
	// Dividing the spreading component 'R(ts->t,L->MOR)' into 'N' stages gives:
	//
	// R(0->t,A->L) * R(ts->t,L->MOR)
	// R(0->t,A->L) * R(t[N-1]->t[N],L->MOR) * R(t[N-2]->t[N-1],L->MOR) * ... * R(t2->t3,L->MOR) * R(t1->t2,L->MOR) * R(ts->t1,L->MOR)
	//
	// Each stage R(t[i-1]->t[i],L->MOR) can then be calculated as a half-stage rotation of R(t[i-1]->t[i],L->R).

	using namespace GPlatesMaths;

	// Start with the identity rotation and accumulate half-stage rotations over intervals.
	FiniteRotation spreading_rotation = FiniteRotation::create_identity_rotation();
	double prev_time = spreading_start_time;
	bool last_iteration = false;

	bool backward_in_time;
	double time_interval;
	if (reconstruction_time >= spreading_start_time)
	{
		backward_in_time = true;
		time_interval = half_stage_rotation_interval;
	}
	else // forward in time...
	{
		backward_in_time = false;
		time_interval = -half_stage_rotation_interval;
	}

	// Iterate over the half-stage rotation intervals.
	do
	{
		double curr_time = prev_time + time_interval;
		if (backward_in_time)
		{
			if (curr_time >= reconstruction_time)
			{
				curr_time = reconstruction_time;
				last_iteration = true;
			}
		}
		else // forward in time...
		{
			if (curr_time <= reconstruction_time)
			{
				curr_time = reconstruction_time;
				last_iteration = true;
			}
		}

		const FiniteRotation left_to_right_stage =
				get_stage_pole(
						*reconstruction_tree_creator.get_reconstruction_tree(prev_time),
						*reconstruction_tree_creator.get_reconstruction_tree(curr_time),
						right_plate_id,
						left_plate_id);

		if (!represents_identity_rotation(left_to_right_stage.unit_quat()))
		{
			UnitQuaternion3D::RotationParams left_to_right_params =
					left_to_right_stage.unit_quat().get_rotation_params(boost::none);

			// Convert range [-1,1] to range [0,1].
			const double asymmetry_angle_factor = 0.5 * (1 + spreading_asymmetry);
			const real_t asymmetry_angle = asymmetry_angle_factor * left_to_right_params.angle;

			FiniteRotation left_to_right_half_stage = 
					FiniteRotation::create(
							UnitQuaternion3D::create_rotation(
									left_to_right_params.axis, 
									asymmetry_angle),
									left_to_right_stage.axis_hint());

			spreading_rotation = compose(left_to_right_half_stage, spreading_rotation);
		}

		prev_time = curr_time;
	}
	while (!last_iteration);

	const FiniteRotation left_rotation = reconstruction_tree_creator.get_reconstruction_tree(reconstruction_time)
			->get_composed_absolute_rotation(left_plate_id).first;

	return compose(left_rotation, spreading_rotation);
}


GPlatesMaths::FiniteRotation
GPlatesAppLogic::RotationUtils::get_half_stage_rotation(
		const double &reconstruction_time,
		const ReconstructionFeatureProperties &reconstruction_params,
		const ReconstructionTreeCreator &reconstruction_tree_creator)
{
	// Default values for half-stage parameters.
	//
	// If we can't get left/right plate IDs then we'll just use plate id zero (spin axis)
	// which can still give a non-identity rotation if the anchor plate id is non-zero.
	GPlatesModel::integer_plate_id_type left_plate_id = 0;
	GPlatesModel::integer_plate_id_type right_plate_id = 0;
	double spreading_asymmetry = 0.0;
	double spreading_start_time = 0.0;
	// Half-stage version 1 only has a single time interval (from present day to the reconstruction time).
	double half_stage_rotation_interval;
	if (reconstruction_time > 0)
	{
		half_stage_rotation_interval = reconstruction_time;
	}
	else if (reconstruction_time < 0)
	{
		half_stage_rotation_interval = -reconstruction_time;
	}
	else // interval must be non-zero...
	{
		half_stage_rotation_interval = DEFAULT_TIME_INTERVAL_HALF_STAGE_ROTATION;
	}

	if (reconstruction_params.get_left_plate_id())
	{
		left_plate_id = reconstruction_params.get_left_plate_id().get();
	}
	if (reconstruction_params.get_right_plate_id())
	{
		right_plate_id = reconstruction_params.get_right_plate_id().get();
	}

	static GPlatesPropertyValues::EnumerationContent enumeration_half_stage_rotation_version_2 =
			GPlatesPropertyValues::EnumerationContent("HalfStageRotationVersion2");
	static GPlatesPropertyValues::EnumerationContent enumeration_half_stage_rotation_version_3 =
			GPlatesPropertyValues::EnumerationContent("HalfStageRotationVersion3");

	const boost::optional<GPlatesPropertyValues::EnumerationContent> reconstruction_method =
			reconstruction_params.get_reconstruction_method();

	// Get the half-stage rotation.
	if (reconstruction_method == enumeration_half_stage_rotation_version_3 ||
		reconstruction_method == enumeration_half_stage_rotation_version_2)
	{
		/**
		 * The time interval used by version 2 and 3 is 10my.
		 *
		 * NOTE: This value should not be modified otherwise it will cause reconstructions of
		 * half-stage features to be positioned incorrectly (this is because their present day
		 * geometries have been reverse-reconstructed using this value).
		 * If it needs to be modified then a new version should be implemented and associated with
		 * a new enumeration in the 'gpml:ReconstructionMethod' property.
		 */
		half_stage_rotation_interval = 10.0;

		// Version 2 introduced spreading asymmetry.
		if (reconstruction_params.get_spreading_asymmetry())
		{
			spreading_asymmetry = reconstruction_params.get_spreading_asymmetry().get();
		}

		if (reconstruction_method == enumeration_half_stage_rotation_version_3)
		{
			// Version 3 introduced spreading start time (geometry import time).
			const boost::optional<GPlatesPropertyValues::GeoTimeInstant> geometry_import_time =
					reconstruction_params.get_geometry_import_time();

			if (geometry_import_time &&
				geometry_import_time->is_real())
			{
				spreading_start_time = geometry_import_time->value();
			}
		}
	}

	return get_half_stage_rotation(
			reconstruction_tree_creator,
			reconstruction_time,
			left_plate_id,
			right_plate_id,
			spreading_asymmetry,
			spreading_start_time,
			half_stage_rotation_interval);
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

	// NOTE: Since q and -q both rotate a point to the same final position (where 'q' is any quaternion)
	// it's possible that left_q and right_q could be separated by a longer path than are left_q and -right_q
	// (or -left_q and right_q).
	// However we do *not* restrict ourselves to the shortest path (like 'FiniteRotation::interpolate()'
	// does in its SLERP routine). This is because the user (who created the total poles in the
	// rotation file) may want to take the longest path.
	//

	// For t1, get the rotation for plate M w.r.t. anchor	
	const std::pair<GPlatesMaths::FiniteRotation, ReconstructionTree::ReconstructionCircumstance>
			rot_0_to_t1_M = reconstruction_tree_1.get_composed_absolute_rotation(moving_plate_id);
	if (rot_0_to_t1_M.second == ReconstructionTree::NoPlateIdMatchesFound)
	{
		// Unable to calculate stage rotation - return identity rotation.
		return GPlatesMaths::FiniteRotation::create_identity_rotation();
	}
	const GPlatesMaths::FiniteRotation &finite_rot_0_to_t1_M = rot_0_to_t1_M.first;

	// For t1, get the rotation for plate F w.r.t. anchor	
	const std::pair<GPlatesMaths::FiniteRotation, ReconstructionTree::ReconstructionCircumstance>
			rot_0_to_t1_F = reconstruction_tree_1.get_composed_absolute_rotation(fixed_plate_id);
	if (rot_0_to_t1_F.second == ReconstructionTree::NoPlateIdMatchesFound)
	{
		// Unable to calculate stage rotation - return identity rotation.
		return GPlatesMaths::FiniteRotation::create_identity_rotation();
	}
	const GPlatesMaths::FiniteRotation &finite_rot_0_to_t1_F = rot_0_to_t1_F.first;


	// For t2, get the rotation for plate M w.r.t. anchor	
	const std::pair<GPlatesMaths::FiniteRotation, ReconstructionTree::ReconstructionCircumstance>
			rot_0_to_t2_M = reconstruction_tree_2.get_composed_absolute_rotation(moving_plate_id);
	if (rot_0_to_t2_M.second == ReconstructionTree::NoPlateIdMatchesFound)
	{
		// Unable to calculate stage rotation - return identity rotation.
		return GPlatesMaths::FiniteRotation::create_identity_rotation();
	}
	const GPlatesMaths::FiniteRotation &finite_rot_0_to_t2_M = rot_0_to_t2_M.first;

	// For t2, get the rotation for plate F w.r.t. anchor	
	const std::pair<GPlatesMaths::FiniteRotation, ReconstructionTree::ReconstructionCircumstance>
			rot_0_to_t2_F = reconstruction_tree_2.get_composed_absolute_rotation(fixed_plate_id);
	if (rot_0_to_t2_F.second == ReconstructionTree::NoPlateIdMatchesFound)
	{
		// Unable to calculate stage rotation - return identity rotation.
		return GPlatesMaths::FiniteRotation::create_identity_rotation();
	}
	const GPlatesMaths::FiniteRotation &finite_rot_0_to_t2_F = rot_0_to_t2_F.first;

	// Compose these rotations so that we get
	// the stage pole from time t1 to time t2 for plate M w.r.t. plate F.

	GPlatesMaths::FiniteRotation finite_rot_t1 = 
		GPlatesMaths::compose(GPlatesMaths::get_reverse(finite_rot_0_to_t1_F), finite_rot_0_to_t1_M);

	GPlatesMaths::FiniteRotation finite_rot_t2 = 
		GPlatesMaths::compose(GPlatesMaths::get_reverse(finite_rot_0_to_t2_F), finite_rot_0_to_t2_M);	

	GPlatesMaths::FiniteRotation stage_pole = 
		GPlatesMaths::compose(finite_rot_t2,GPlatesMaths::get_reverse(finite_rot_t1));	

	return stage_pole;	
}


boost::optional<GPlatesMaths::FiniteRotation>
GPlatesAppLogic::RotationUtils::calculate_short_path_final_rotation(
		const GPlatesMaths::FiniteRotation &final_rotation,
		const GPlatesMaths::FiniteRotation &initial_rotation)
{
	// Since q and -q both rotate a point to the same final position (where 'q' is any quaternion) it's
	// possible that q1 and q2 could be separated by a longer path than are q1 and -q2 (or -q1 and q2).
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
