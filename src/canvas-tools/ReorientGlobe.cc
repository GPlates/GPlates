/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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


#include "ReorientGlobe.h"

#include "qt-widgets/GlobeCanvas.h"
#include "qt-widgets/ViewportWindow.h"


void
GPlatesCanvasTools::ReorientGlobe::handle_activation()
{
	// FIXME: Could be pithier.
	// FIXME: May have to adjust message if we are using Map view.
	d_view_state_ptr->status_message(QObject::tr("Drag the globe to reorient."));
}


void
GPlatesCanvasTools::ReorientGlobe::handle_left_drag(
		const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
		bool was_on_globe,
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		bool is_on_globe)
{
	reorient_globe_by_drag_update(initial_pos_on_globe,
			oriented_initial_pos_on_globe, was_on_globe,
			current_pos_on_globe, is_on_globe);
}

void
GPlatesCanvasTools::ReorientGlobe::handle_left_release_after_drag(
		const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
		bool was_on_globe,
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		bool is_on_globe)
{
	reorient_globe_by_drag_release(initial_pos_on_globe,
			oriented_initial_pos_on_globe, was_on_globe,
			current_pos_on_globe, is_on_globe);
}


