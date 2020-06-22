/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, 2008 The University of Sydney, Australia
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

#include "GlobeCanvasTool.h"

#include "Globe.h"
#include "GlobeCamera.h"

#include "maths/PointOnSphere.h"

#include "qt-widgets/GlobeCanvas.h"


GPlatesGui::GlobeCanvasTool::~GlobeCanvasTool()
{  }


void
GPlatesGui::GlobeCanvasTool::reorient_globe_by_drag_update(
		const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
		bool was_on_globe,
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
{
	if (!d_is_in_reorientation_op)
	{
		d_globe_ptr->get_globe_camera().start_drag(GlobeCamera::DRAG_NORMAL, initial_pos_on_globe);
		d_is_in_reorientation_op = true;
	}

	d_globe_ptr->get_globe_camera().update_drag(current_pos_on_globe);
}


void
GPlatesGui::GlobeCanvasTool::reorient_globe_by_drag_release(
		const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
		bool was_on_globe,
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
{
	if (!d_is_in_reorientation_op)
	{
		d_globe_ptr->get_globe_camera().start_drag(GlobeCamera::DRAG_NORMAL, initial_pos_on_globe);
		d_is_in_reorientation_op = true;
	}

	d_globe_ptr->get_globe_camera().update_drag(current_pos_on_globe);
	d_globe_ptr->get_globe_camera().end_drag();
	d_is_in_reorientation_op = false;
}


void
GPlatesGui::GlobeCanvasTool::rotate_globe_by_drag_update(
		const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
		bool was_on_globe,
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
{
	if ( ! d_is_in_reorientation_op)
	{
		d_globe_ptr->get_globe_camera().start_drag(GlobeCamera::DRAG_ROTATE, initial_pos_on_globe);
		d_is_in_reorientation_op = true;
	}

	d_globe_ptr->get_globe_camera().update_drag(current_pos_on_globe);
}


void
GPlatesGui::GlobeCanvasTool::rotate_globe_by_drag_release(
		const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
		bool was_on_globe,
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
{
	if ( ! d_is_in_reorientation_op)
	{
		d_globe_ptr->get_globe_camera().start_drag(GlobeCamera::DRAG_ROTATE, initial_pos_on_globe);
		d_is_in_reorientation_op = true;
	}

	d_globe_ptr->get_globe_camera().update_drag(current_pos_on_globe);
	d_globe_ptr->get_globe_camera().end_drag();
	d_is_in_reorientation_op = false;
}
