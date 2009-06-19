
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


#ifndef _GPLATES_MATHS_CV_H_
#define _GPLATES_MATHS_CV_H_

#include <map>
#include <utility>  /* std::pair */
#include <boost/optional.hpp>

#include "global/types.h"
#include "maths/FiniteRotation.h"
#include "maths/types.h"


namespace GPlatesMaths
{
	/**
	 * Calculate the velocity of a PointOnSphere @a point undergoing rotation.
	 * Dimensions are centimetres per year.  
	 * The velocity will be returned as an X Y Z vector
	 * If, for whatever reason, the velocity cannot be calculated, 
	 * return Vector3D(0, 0, 0).
	 *
	 * In general, time 1 should be more recent than time 2.
	 * That is, t1 should be less than t2 in GPlates age based system.
	 * For example: t1 = 10 Ma, t2 = 11 Ma.
	 *
	 */
	Vector3D
	calculate_velocity_vector(
		const PointOnSphere &point, 
		FiniteRotation &fr_t1,
	    FiniteRotation &fr_t2);

	/**
	 * Convert a vector from X Y Z space to North East Down space and 
	 * return Colatitudinal and Longitudinal components of the vector
	 * ( Colat is -North , and Lon is East )
	 */
	std::pair< real_t, real_t >
	calculate_vector_components_colat_lon(
		const PointOnSphere &point, 
		Vector3D &vector_xyz);

}

#endif  // _GPLATES_MATHS_CV_H_
