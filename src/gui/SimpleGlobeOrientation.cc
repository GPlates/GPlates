/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004, 2005, 2006, 2007 The University of Sydney, Australia
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

#include "SimpleGlobeOrientation.h"


void
GPlatesGui::SimpleGlobeOrientation::move_handle_to_pos(
		const GPlatesMaths::PointOnSphere &pos)
{
	if (d_handle_pos == pos) {
		// There's no difference between the positions, so nothing to do.
		return;
	}
	GPlatesMaths::Rotation rot = GPlatesMaths::Rotation::create(d_handle_pos, pos);

	d_accum_rot = rot * d_accum_rot;
	d_rev_accum_rot = d_accum_rot.get_reverse();
	d_handle_pos = pos;

	emit orientation_changed();
}
