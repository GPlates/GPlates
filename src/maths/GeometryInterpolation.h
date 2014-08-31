/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2014 The University of Sydney, Australia
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

#ifndef GPLATES_MATHS_GEOMETRYINTERPOLATION_H
#define GPLATES_MATHS_GEOMETRYINTERPOLATION_H

#include <vector>

#include "PolylineOnSphere.h"
#include "types.h"
#include "UnitVector3D.h"


namespace GPlatesMaths
{
	/**
	 * Interpolates between two polylines along small circle arcs emanating from @a rotation_axis.
	 *
	 * The maximum distance between adjacent interpolated polylines is @a interpolate_resolution_radians.
	 *
	 * NOTE: The original polylines @a from_polyline and @a to_polyline are also returned in
	 * @a interpolated_polylines since the points in each geometry are ordered from closest to
	 * @a rotation_axis to furthest (which may be different than the order in the originals).
	 *
	 * Returns false if the polylines do not overlap in latitude (where North pole is @a rotation_axis).
	 */
	bool
	interpolate(
			std::vector<PolylineOnSphere::non_null_ptr_to_const_type> &interpolated_polylines,
			const PolylineOnSphere::non_null_ptr_to_const_type &from_polyline,
			const PolylineOnSphere::non_null_ptr_to_const_type &to_polyline,
			const UnitVector3D &rotation_axis,
			const double &interpolate_resolution_radians);
}

#endif // GPLATES_MATHS_GEOMETRYINTERPOLATION_H
