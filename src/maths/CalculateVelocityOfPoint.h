/* $Id: CalculateVelocityOfPoint.h,v 1.1.4.5 2006/03/14 00:29:01 mturner Exp $ */

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


#ifndef _GPLATES_MATHS_CVOP_H_
#define _GPLATES_MATHS_CVOP_H_

#include <map>
#include <utility>  /* std::pair */

#include "global/types.h"
#include "maths/FiniteRotation.h"
#include "maths/types.h"


namespace GPlatesMaths
{
	typedef
	 std::map< GPlatesGlobal::rid_t, FiniteRotation >
	  RotationsByPlate;


	/**
	 * Given an empty @a rot_cache (which will come to contain the
	 * collection of all plates which can be rotated), populate it 
	 * with the finite rotations which will rotate the plates to 
	 * their positions at time @a t.
	 *
	 * This function is a non-recursive "wrapper" around the recursive
	 * function @a CheckRotation.
	 */
	void
	PopulateRotatableData(RotationsByPlate &rot_cache, const real_t &t);


	/**
	 * Calculate the colatitudinal and longitudinal components of the
	 * velocity of a PointOnSphere @a point undergoing rotation.
	 * Dimensions are centimetres per year.  
	 * The velocity will be returned
	 * in the std::pair<,> (colat_comp, lon_comp).  If, for whatever
	 * reason, the velocity cannot be calculated, return (0, 0).
	 *
	 * In general, time 1 should be more recent than time 2.
	 *
	 * That is, t1 should be less than t2 in GPlates age based system.
	 * For example: t1 = 10 Ma, t2 = 11 Ma.
	 */
	std::pair< real_t, real_t >
	CalculateVelocityOfPoint(
		const PointOnSphere &point, 
		const rid_t &plate_id, 
		const RotationsByPlate &rotations_for_time_t1,
		const RotationsByPlate &rotations_for_time_t2,
		PointOnSphere &rotated_point 
		);

}

#endif  // _GPLATES_MATHS_CVOP_H_
