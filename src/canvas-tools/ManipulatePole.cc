/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2010 The University of Sydney, Australia
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

#include "maths/LatLonPoint.h"

#include "qt-widgets/GlobeCanvas.h"
#include "qt-widgets/ViewportWindow.h"

#include "view-operations/RenderedGeometryCollection.h"


GPlatesCanvasTools::ManipulatePole::ManipulatePole(
		const status_bar_callback_type &status_bar_callback,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		GPlatesQtWidgets::ModifyReconstructionPoleWidget &pole_widget) :
	CanvasTool(status_bar_callback),
	d_rendered_geom_collection(&rendered_geom_collection),
	d_pole_widget_ptr(&pole_widget),
	d_is_in_drag(false)
{  }


void
GPlatesCanvasTools::ManipulatePole::handle_activation()
{
	set_status_bar_message(QT_TR_NOOP("Drag or Shift+drag the current geometry to modify its reconstruction pole."));

	// Activate the pole manipulation rendered layer.
	d_rendered_geom_collection->set_main_layer_active(
		GPlatesViewOperations::RenderedGeometryCollection::POLE_MANIPULATION_LAYER);

	d_pole_widget_ptr->activate();
}


void
GPlatesCanvasTools::ManipulatePole::handle_deactivation()
{
	d_pole_widget_ptr->deactivate();
}


void
GPlatesCanvasTools::ManipulatePole::handle_left_drag(
		const GPlatesMaths::PointOnSphere &initial_point_on_sphere,
		bool was_on_earth,
		double initial_proximity_inclusion_threshold,
		const GPlatesMaths::PointOnSphere &current_point_on_sphere,
		bool is_on_earth,
		double current_proximity_inclussion_threshold,
		const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport)
{
	if ( ! d_is_in_drag)
	{
		d_pole_widget_ptr->start_new_drag(initial_point_on_sphere);
		d_is_in_drag = true;
	}
	d_pole_widget_ptr->update_drag_position(current_point_on_sphere);
}


void
GPlatesCanvasTools::ManipulatePole::handle_left_release_after_drag(
		const GPlatesMaths::PointOnSphere &initial_point_on_sphere,
		bool was_on_earth,
		double initial_proximity_inclusion_threshold,
		const GPlatesMaths::PointOnSphere &current_point_on_sphere,
		bool is_on_earth,
		double current_proximity_inclussion_threshold,
		const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport)
{
	if ( ! d_is_in_drag)
	{
		d_pole_widget_ptr->start_new_drag(initial_point_on_sphere);
		d_is_in_drag = true;
	}
	d_pole_widget_ptr->update_drag_position(current_point_on_sphere);
	d_pole_widget_ptr->end_drag();
	d_is_in_drag = false;
}


void
GPlatesCanvasTools::ManipulatePole::handle_shift_left_drag(
		const GPlatesMaths::PointOnSphere &initial_point_on_sphere,
		bool was_on_earth,
		double initial_proximity_inclusion_threshold,
		const GPlatesMaths::PointOnSphere &current_point_on_sphere,
		bool is_on_earth,
		double current_proximity_inclussion_threshold,
		const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport)
{
	if ( ! centre_of_viewport)
	{
		return;
	}

	if ( ! d_is_in_drag)
	{
		d_pole_widget_ptr->start_new_rotation_drag(
				initial_point_on_sphere,
				*centre_of_viewport);
		d_is_in_drag = true;
	}
	d_pole_widget_ptr->update_rotation_drag_position(
			current_point_on_sphere,
			*centre_of_viewport);
}


void
GPlatesCanvasTools::ManipulatePole::handle_shift_left_release_after_drag(
		const GPlatesMaths::PointOnSphere &initial_point_on_sphere,
		bool was_on_earth,
		double initial_proximity_inclusion_threshold,
		const GPlatesMaths::PointOnSphere &current_point_on_sphere,
		bool is_on_earth,
		double current_proximity_inclussion_threshold,
		const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport)
{
	if ( ! centre_of_viewport)
	{
		return;
	}

	if ( ! d_is_in_drag)
	{
		d_pole_widget_ptr->start_new_rotation_drag(
				initial_point_on_sphere,
				*centre_of_viewport);
		d_is_in_drag = true;
	}
	d_pole_widget_ptr->update_rotation_drag_position(
			current_point_on_sphere,
			*centre_of_viewport);
	d_pole_widget_ptr->end_drag();
	d_is_in_drag = false;
}

