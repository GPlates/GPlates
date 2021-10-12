/* $Id$ */

/**
 * @file 
 * Contains the implementation of the GlobeVisibilityTester class.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include "maths/PointOnSphere.h"
#include "maths/LatLonPointConversions.h"
#include "qt-widgets/GlobeCanvas.h"

bool
GPlatesGui::GlobeVisibilityTester::is_point_visible(
		const GPlatesMaths::PointOnSphere &point_on_sphere)
{
	GPlatesMaths::PointOnSphere camera_pos = GPlatesMaths::make_point_on_sphere(
			*(d_globe_canvas_ptr->camera_llp()));
	return calculate_closeness(point_on_sphere, camera_pos) >= 0.0;
}

