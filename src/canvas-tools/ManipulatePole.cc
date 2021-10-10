/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
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

#include "ManipulatePole.h"
#include "qt-widgets/GlobeCanvas.h"
#include "qt-widgets/ViewportWindow.h"
#include "gui/RenderedGeometryLayers.h"
#include "maths/LatLonPointConversions.h"


GPlatesCanvasTools::ManipulatePole::ManipulatePole(
		GPlatesGui::Globe &globe_,
		GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
		GPlatesGui::RenderedGeometryLayers &layers,
		const GPlatesQtWidgets::ViewportWindow &view_state_,
		GPlatesQtWidgets::ReconstructionPoleWidget &pole_widget_):
	CanvasTool(globe_, globe_canvas_),
	d_layers_ptr(&layers),
	d_view_state_ptr(&view_state_),
	d_pole_widget_ptr(&pole_widget_),
	d_is_in_drag(false)
{  }


void
GPlatesCanvasTools::ManipulatePole::handle_activation()
{
	// FIXME:  We may have to adjust the message if we are using a Map View.
	d_view_state_ptr->status_message(QObject::tr(
			"Drag or Shift+drag the current geometry to modify its reconstruction pole."
			" Ctrl+drag to re-orient the globe."));

	d_pole_widget_ptr->activate();
	d_layers_ptr->show_only_pole_manipulation_layer();
	globe_canvas().update_canvas();
}


void
GPlatesCanvasTools::ManipulatePole::handle_deactivation()
{
	d_pole_widget_ptr->deactivate();
}


void
GPlatesCanvasTools::ManipulatePole::handle_left_drag(
		const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
		bool was_on_globe,
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
{
	if ( ! d_is_in_drag) {
		d_pole_widget_ptr->start_new_drag(oriented_initial_pos_on_globe);
		d_is_in_drag = true;
	}
	d_pole_widget_ptr->update_drag_position(oriented_current_pos_on_globe);
}


void
GPlatesCanvasTools::ManipulatePole::handle_left_release_after_drag(
		const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
		bool was_on_globe,
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
{
	if ( ! d_is_in_drag) {
		d_pole_widget_ptr->start_new_drag(oriented_initial_pos_on_globe);
		d_is_in_drag = true;
	}
	d_pole_widget_ptr->update_drag_position(oriented_current_pos_on_globe);
	d_pole_widget_ptr->end_drag();
	d_is_in_drag = false;
}


void
GPlatesCanvasTools::ManipulatePole::handle_shift_left_drag(
		const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
		bool was_on_globe,
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
{
	if ( ! d_is_in_drag) {
		d_pole_widget_ptr->start_new_rotation_drag(oriented_initial_pos_on_globe,
				oriented_centre_of_viewport);
		d_is_in_drag = true;
	}
	d_pole_widget_ptr->update_rotation_drag_position(oriented_current_pos_on_globe,
			oriented_centre_of_viewport);
}


void
GPlatesCanvasTools::ManipulatePole::handle_shift_left_release_after_drag(
		const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
		bool was_on_globe,
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
{
	if ( ! d_is_in_drag) {
		d_pole_widget_ptr->start_new_rotation_drag(oriented_initial_pos_on_globe,
				oriented_centre_of_viewport);
		d_is_in_drag = true;
	}
	d_pole_widget_ptr->update_rotation_drag_position(oriented_current_pos_on_globe,
			oriented_centre_of_viewport);
	d_pole_widget_ptr->end_drag();
	d_is_in_drag = false;
}
