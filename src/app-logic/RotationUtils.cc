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


boost::optional<GPlatesMaths::FiniteRotation>
GPlatesAppLogic::RotationUtils::take_short_relative_rotation_path(
		const GPlatesMaths::FiniteRotation &to_rotation,
		const GPlatesMaths::FiniteRotation &from_rotation)
{
	// Since q and -q map to the same rotation (where 'q' is any quaternion) it's possible
	// that q1 and q2 could be separated by a longer path than are q1 and -q2 (or -q1 and q2).
	// So check if we're using the longer path and negate the quaternion (of the
	// to-rotation) in order to take the shorter path.
	if (dot(from_rotation.unit_quat(), to_rotation.unit_quat()).is_precisely_less_than(0))
	{
		// Note: We use the axis hint of the from-rotation (for the adjusted to-rotation)
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
				-to_rotation.unit_quat(),
				from_rotation.axis_hint());
	}

	// No adjustment was necessary.
	return boost::none;
}
