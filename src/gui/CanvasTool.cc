/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007 The University of Sydney, Australia
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

#include "CanvasTool.h"
#include "Globe.h"
#include "maths/PointOnSphere.h"


GPlatesGui::CanvasTool::~CanvasTool()
{  }


void
GPlatesGui::CanvasTool::reorient_globe_by_drag_update(
		const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
		bool was_on_globe,
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		bool is_on_globe)
{
	if ( ! d_is_in_reorientation_op) {
		d_globe_ptr->SetNewHandlePos(initial_pos_on_globe);
		d_is_in_reorientation_op = true;
	}
	d_globe_ptr->UpdateHandlePos(current_pos_on_globe);
}


void
GPlatesGui::CanvasTool::reorient_globe_by_drag_release(
		const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
		bool was_on_globe,
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		bool is_on_globe)
{
	if ( ! d_is_in_reorientation_op) {
		d_globe_ptr->SetNewHandlePos(initial_pos_on_globe);
		d_is_in_reorientation_op = true;
	}
	d_globe_ptr->UpdateHandlePos(current_pos_on_globe);
	d_is_in_reorientation_op = false;
}
