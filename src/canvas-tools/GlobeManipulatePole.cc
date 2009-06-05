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

#include "GlobeManipulatePole.h"
#include "maths/LatLonPointConversions.h"
#include "qt-widgets/GlobeCanvas.h"
#include "qt-widgets/ViewportWindow.h"
#include "view-operations/RenderedGeometryCollection.h"


GPlatesCanvasTools::GlobeManipulatePole::GlobeManipulatePole(
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		GPlatesGui::Globe &globe_,
		GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
		const GPlatesQtWidgets::ViewportWindow &view_state_,
		GPlatesQtWidgets::ReconstructionPoleWidget &pole_widget):
	GlobeCanvasTool(globe_, globe_canvas_),
	d_rendered_geom_collection(&rendered_geom_collection),
	d_view_state_ptr(&view_state_),
	d_pole_widget_ptr(&pole_widget),
	d_is_in_drag(false)
{
}


void
GPlatesCanvasTools::GlobeManipulatePole::handle_activation()
{
	if (globe_canvas().isVisible())
	{
	
		d_view_state_ptr->status_message(QObject::tr(
			"Drag or Shift+drag the current geometry to modify its reconstruction pole."
			" Ctrl+drag to re-orient the globe."));

		// Activate the pole manipulation rendered layer.
		d_rendered_geom_collection->set_main_layer_active(
			GPlatesViewOperations::RenderedGeometryCollection::POLE_MANIPULATION_LAYER);

		d_pole_widget_ptr->activate();
	}
}


void
GPlatesCanvasTools::GlobeManipulatePole::handle_deactivation()
{
	d_pole_widget_ptr->deactivate();
}


void
GPlatesCanvasTools::GlobeManipulatePole::handle_left_drag(
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
GPlatesCanvasTools::GlobeManipulatePole::handle_left_release_after_drag(
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
GPlatesCanvasTools::GlobeManipulatePole::handle_shift_left_drag(
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
GPlatesCanvasTools::GlobeManipulatePole::handle_shift_left_release_after_drag(
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
