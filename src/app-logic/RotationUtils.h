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

#ifndef GPLATES_APP_LOGIC_ROTATIONUTILS_H
#define GPLATES_APP_LOGIC_ROTATIONUTILS_H

#include <boost/optional.hpp>

#include "maths/FiniteRotation.h"

#include "model/types.h"


namespace GPlatesAppLogic
{
	class ReconstructionTree;
	class ReconstructionTreeCreator;

	/**
	 * Utilities related to rotation calculations.
	 *
	 * Utilities dealing with geometry reconstruction can go in "ReconstructUtils.h".
	 */
	namespace RotationUtils
	{
		/**
		 * The default time interval for calculating half-stage rotations (see @a get_half_stage_rotation).
		 *
		 * If present day to reconstruction time is greater than this interval then it will be
		 * divided into multiple half-stage intervals of this size.
		 *
		 * This is small enough to get good accuracy but not too small that it requires an excessive
		 * number of reconstruction trees to be calculated (one at end of each time interval).
		 */
		const double DEFAULT_TIME_INTERVAL_HALF_STAGE_ROTATION = 10.0;


		/**
		 * Returns the half-stage rotation between @a left_plate_id and @a right_plate_id at the
		 * reconstruction time of the specified reconstruction tree.
		 *
		 * @a spreading_asymmetry is in the range [-1,1] where the value 0 represents half-stage
		 * rotation, the value 1 represents full-stage rotation (right plate) and the value -1
		 * represents zero stage rotation (left plate).
		 *
		 * If present day to reconstruction time is greater than @a half_stage_rotation_interval
		 * then it will be divided into multiple half-stage intervals of this size (except for
		 * the last interval that ends at the reconstruction time).
		 *
		 * @throws PreconditionViolationError if @a reconstruction_time is negative or
		 * if @a half_stage_rotation_interval is not greater than zero.
		 */
		GPlatesMaths::FiniteRotation
		get_half_stage_rotation(
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				const double &reconstruction_time,
				GPlatesModel::integer_plate_id_type left_plate_id,
				GPlatesModel::integer_plate_id_type right_plate_id,
				const double &spreading_asymmetry = 0.0,
				const double &half_stage_rotation_interval = DEFAULT_TIME_INTERVAL_HALF_STAGE_ROTATION);


		/**
		 * Returns the stage-pole for @a moving_plate_id wrt @a fixed_plate_id, between
		 * the times represented by @a reconstruction_tree_ptr_1 and 
		 * @a reconstruction_tree_ptr_2
		 */
		GPlatesMaths::FiniteRotation
        get_stage_pole(
				const ReconstructionTree &reconstruction_tree_ptr_1, 
				const ReconstructionTree &reconstruction_tree_ptr_2, 
				const GPlatesModel::integer_plate_id_type &moving_plate_id, 
				const GPlatesModel::integer_plate_id_type &fixed_plate_id);	


		/**
		 * Returns an adjusted version of @a final_rotation such that the relative rotation
		 * from @a initial_rotation to @a final_rotation takes the short path around the globe
		 * (instead of long path), or returns none if it's already the short path.
		 *
		 * Although this adjustment is not necessary for interpolating between adjacent total
		 * poles (because the SLERP interpolation checks for short/long paths) it does appear
		 * necessary for half-stage rotations. An interpolated total rotation is fine because it
		 * rotates along the short path *between* the two total rotations it's interpolating.
		 * In other words there's two possible stage rotations between these two total rotations -
		 * the long way around and the short way - and the SLERP interpolation follows the short way.
		 * However a particular interpolated total rotation only needs to rotate such that a geometry
		 * (at present day) ends up at the correct location (at a palaeo time). The *total* rotation
		 * could have gone the short or long way around the globe to get there - it doesn't really matter.
		 * Note that the *stage* rotation, in contrast, always follows the short way as noted above -
		 * it's a bit confusing.
		 * However if you were to calculate the stage rotation between those two total poles then
		 * it might actually be the long path. That's where this function comes in handy - it can be
		 * used to adjust one of the two total poles such that, if a stage pole was calculated, it
		 * would take the short path.
		 * This could also apply to other types of relative rotations (besides stage poles) such as
		 * the relative rotation of one plate relative to any another plate at a fixed time.
		 */
		boost::optional<GPlatesMaths::FiniteRotation>
		calculate_short_path_final_rotation(
				const GPlatesMaths::FiniteRotation &final_rotation,
				const GPlatesMaths::FiniteRotation &initial_rotation);
	}
}

#endif // GPLATES_APP_LOGIC_ROTATIONUTILS_H
