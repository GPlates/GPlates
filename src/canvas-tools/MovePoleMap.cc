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

#include "MovePoleMap.h"

#include "presentation/ViewState.h"

#include "qt-widgets/MapCanvas.h"
#include "qt-widgets/MapView.h"
#include "qt-widgets/ViewportWindow.h"


GPlatesCanvasTools::MovePoleMap::MovePoleMap(
		const GPlatesViewOperations::MovePoleOperation::non_null_ptr_type &move_pole_operation,
		GPlatesQtWidgets::MapCanvas &map_canvas_,
		GPlatesQtWidgets::MapView &map_view_,
		GPlatesQtWidgets::ViewportWindow &viewport_window_,
		GPlatesPresentation::ViewState &view_state_) :
	MapCanvasTool(map_canvas_, map_view_, view_state_.get_map_transform()),
	d_viewport_window_ptr(&viewport_window_),
	d_move_pole_operation(move_pole_operation),
	d_is_in_drag(false)
{
}


void
GPlatesCanvasTools::MovePoleMap::handle_activation()
{
	if (map_view().isVisible())
	{
		// Activate our MovePoleOperation.
		d_move_pole_operation->activate();

		d_viewport_window_ptr->status_message(QObject::tr(
				"Drag pole to move its location."));
	}
}


void
GPlatesCanvasTools::MovePoleMap::handle_deactivation()
{
	if (map_view().isVisible())
	{
		// Deactivate our MovePoleOperation.
		d_move_pole_operation->deactivate();
	}
}


void
GPlatesCanvasTools::MovePoleMap::handle_left_drag(
		const QPointF &initial_point_on_scene,
		bool was_on_surface,
		const QPointF &current_point_on_scene,
		bool is_on_surface,
		const QPointF &translation)
{
	if (!map_view().isVisible())
	{
		return;
	}

	if (!is_on_surface)
	{
		// We currently can't do anything sensible with map view when off map.
		return;
	}

	const GPlatesGui::MapProjection &projection = map_canvas().map().projection();

	boost::optional<GPlatesMaths::PointOnSphere> initial_point_on_sphere =
			qpointf_to_point_on_sphere(initial_point_on_scene, projection);
	if (!initial_point_on_sphere)
	{
		return;
	}

	boost::optional<GPlatesMaths::PointOnSphere> current_point_on_sphere =
			qpointf_to_point_on_sphere(current_point_on_scene, projection);
	if (!current_point_on_sphere)
	{
		return;
	}

	if (!d_is_in_drag)
	{
		d_move_pole_operation->start_drag_on_map(
				initial_point_on_scene,
				initial_point_on_sphere.get(),
				projection);

		d_is_in_drag = true;
	}

	d_move_pole_operation->update_drag(current_point_on_sphere.get());
}


void
GPlatesCanvasTools::MovePoleMap::handle_left_release_after_drag(
		const QPointF &initial_point_on_scene,
		bool was_on_surface,
		const QPointF &current_point_on_scene,
		bool is_on_surface,
		const QPointF &translation)
{
	if (!map_view().isVisible())
	{
		return;
	}

	if (!is_on_surface)
	{
		// We currently can't do anything sensible with map view when off map.
		return;
	}

	const GPlatesGui::MapProjection &projection = map_canvas().map().projection();

	boost::optional<GPlatesMaths::PointOnSphere> initial_point_on_sphere =
			qpointf_to_point_on_sphere(initial_point_on_scene, projection);
	if (!initial_point_on_sphere)
	{
		return;
	}

	boost::optional<GPlatesMaths::PointOnSphere> current_point_on_sphere =
			qpointf_to_point_on_sphere(current_point_on_scene, projection);
	if (!current_point_on_sphere)
	{
		return;
	}

	// In case clicked and released at same time.
	if (!d_is_in_drag)
	{
		d_move_pole_operation->start_drag_on_map(
				initial_point_on_scene,
				initial_point_on_sphere.get(),
				projection);

		d_is_in_drag = true;
	}

	d_move_pole_operation->update_drag(current_point_on_sphere.get());

	d_move_pole_operation->end_drag(current_point_on_sphere.get());
	d_is_in_drag = false;
}


void
GPlatesCanvasTools::MovePoleMap::handle_move_without_drag(
		const QPointF &current_point_on_scene,
		bool is_on_surface,
		const QPointF &translation)
{
	if (!map_view().isVisible())
	{
		return;
	}

	if (!is_on_surface)
	{
		// We currently can't do anything sensible with map view when off map.
		return;
	}

	const GPlatesGui::MapProjection &projection = map_canvas().map().projection();

	boost::optional<GPlatesMaths::PointOnSphere> current_point_on_sphere =
			qpointf_to_point_on_sphere(current_point_on_scene, projection);
	if (!current_point_on_sphere)
	{
		return;
	}

	d_move_pole_operation->mouse_move_on_map(
			current_point_on_scene,
			current_point_on_sphere.get(),
			projection);
}
