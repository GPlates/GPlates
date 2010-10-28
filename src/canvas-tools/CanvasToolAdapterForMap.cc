/* $Id$ */

/**
 * @file 
 * Contains implementation of CanvasToolAdapterForMap
 *
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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

#include <boost/bind.hpp>

#include "CanvasToolAdapterForMap.h"

#include "gui/MapProjection.h"

#include "maths/PointOnSphere.h"

#include "qt-widgets/ViewportWindow.h"
#include "qt-widgets/MapCanvas.h"


namespace
{
	/**
	 * Converts a QPointF to a PointOnSphere
	 */
	boost::optional<GPlatesMaths::PointOnSphere>
	qpointf_to_point_on_sphere(const QPointF &point, const GPlatesGui::MapProjection &projection)
	{
		double x = point.x();
		double y = point.y();

		boost::optional<GPlatesMaths::LatLonPoint> llp =
			projection.inverse_transform(x, y);
		if (llp)
		{
			GPlatesMaths::PointOnSphere pos = GPlatesMaths::make_point_on_sphere(*llp);
			return boost::optional<GPlatesMaths::PointOnSphere>(pos);
		}
		else
		{
			return boost::none;
		}
	}

	const char *STATUS_BAR_MESSAGE_SUFFIX = QT_TR_NOOP("Ctrl+drag to pan the map.");

	/**
	 * Helper function that sets the status bar message of @a viewport_window
	 * to be @a message with @a STATUS_BAR_MESSAGE_SUFFIX appended.
	 *
	 * Note that @a message should *not* have been translated, i.e. passed
	 * through QObject::tr().
	 */
	void
	set_status_bar_message(
			GPlatesQtWidgets::ViewportWindow &viewport_window,
			const char *message)
	{
		viewport_window.status_message(
				GPlatesQtWidgets::ViewportWindow::tr(message) + " " +
				GPlatesQtWidgets::ViewportWindow::tr(STATUS_BAR_MESSAGE_SUFFIX));
	}
}


GPlatesCanvasTools::CanvasToolAdapterForMap::CanvasToolAdapterForMap (
		CanvasTool *canvas_tool_ptr,
		GPlatesQtWidgets::MapCanvas &map_canvas_,
		GPlatesQtWidgets::MapView &map_view_,
		GPlatesQtWidgets::ViewportWindow &view_state_,
		GPlatesGui::MapTransform &map_transform_) :
	MapCanvasTool(
			map_canvas_,
			map_view_,
			map_transform_),
	d_canvas_tool_ptr(canvas_tool_ptr)
{
	canvas_tool_ptr->set_status_bar_callback(
			boost::bind(
				&::set_status_bar_message,
				boost::ref(view_state_),
				_1));
}


const GPlatesCanvasTools::CanvasToolAdapterForMap::non_null_ptr_type
GPlatesCanvasTools::CanvasToolAdapterForMap::create(
		CanvasTool *canvas_tool_ptr,
		GPlatesQtWidgets::MapCanvas &map_canvas_,
		GPlatesQtWidgets::MapView &map_view_,
		GPlatesQtWidgets::ViewportWindow &view_state,
		GPlatesGui::MapTransform &map_transform_) 
{
	CanvasToolAdapterForMap::non_null_ptr_type ptr(
			new CanvasToolAdapterForMap(
				canvas_tool_ptr,
				map_canvas_,
				map_view_,
				view_state,
				map_transform_),
			GPlatesUtils::NullIntrusivePointerHandler());
	return ptr;
}


void
GPlatesCanvasTools::CanvasToolAdapterForMap::handle_activation()
{
	if (map_view().isVisible())
	{
		d_canvas_tool_ptr->handle_activation();
	}
}


void
GPlatesCanvasTools::CanvasToolAdapterForMap::handle_deactivation()
{
	d_canvas_tool_ptr->handle_deactivation();
}


void
GPlatesCanvasTools::CanvasToolAdapterForMap::invoke_canvas_tool_func(
		const QPointF &click_point_on_scene,
		bool is_on_surface,
		const canvas_tool_click_func &func)
{
	if (!map_view().isVisible())
	{
		return;
	}

	if (!is_on_surface)
	{
		// We currently can't do anything sensible with map view when off map.
		// This can be removed when we have the ability to make mouse-clicks snap
		// to the edge of the map, much like how it snaps to the horizon of the
		// globe if you click outside of the globe.
		return;
	}

	boost::optional<GPlatesMaths::PointOnSphere> point_on_sphere =
		qpointf_to_point_on_sphere(click_point_on_scene, map_canvas().map().projection());
	if (!point_on_sphere)
	{
		return;
	}

	((d_canvas_tool_ptr.get())->*func)(
			*point_on_sphere,
			is_on_surface,
			map_view().current_proximity_inclusion_threshold(
				*point_on_sphere));
}

void
GPlatesCanvasTools::CanvasToolAdapterForMap::invoke_canvas_tool_func(
		const QPointF &initial_point_on_scene,
		bool was_on_surface,
		const QPointF &current_point_on_scene,
		bool is_on_surface,
		const canvas_tool_drag_func_without_default &func)
{
	if ( ! map_view().isVisible())
	{
		return;
	}

	if ( ! is_on_surface)
	{
		// We currently can't do anything sensible with map view when off map.
		return;
	}

	const GPlatesGui::MapProjection &projection = map_canvas().map().projection();

	boost::optional<GPlatesMaths::PointOnSphere> initial_point_on_sphere =
		qpointf_to_point_on_sphere(initial_point_on_scene, projection);
	if ( ! initial_point_on_sphere)
	{
		return;
	}

	boost::optional<GPlatesMaths::PointOnSphere> current_point_on_sphere =
		qpointf_to_point_on_sphere(current_point_on_scene, projection);
	if ( ! current_point_on_sphere)
	{
		return;
	}

	((d_canvas_tool_ptr.get())->*func)(
			*initial_point_on_sphere,
			was_on_surface,
			map_view().current_proximity_inclusion_threshold(
				*initial_point_on_sphere),
			*current_point_on_sphere,
			is_on_surface,
			map_view().current_proximity_inclusion_threshold(
				*current_point_on_sphere),
			qpointf_to_point_on_sphere(map_transform().get_centre_of_viewport(), projection));
}


bool
GPlatesCanvasTools::CanvasToolAdapterForMap::invoke_canvas_tool_func(
		const QPointF &initial_point_on_scene,
		bool was_on_surface,
		const QPointF &current_point_on_scene,
		bool is_on_surface,
		const canvas_tool_drag_func_with_default &func)
{
	if ( ! map_view().isVisible())
	{
		return false;
	}

	if ( ! is_on_surface)
	{
		// We currently can't do anything sensible with map view when off map.
		return false;
	}

	const GPlatesGui::MapProjection &projection = map_canvas().map().projection();

	boost::optional<GPlatesMaths::PointOnSphere> initial_point_on_sphere =
		qpointf_to_point_on_sphere(initial_point_on_scene, projection);
	if ( ! initial_point_on_sphere)
	{
		return false;
	}

	boost::optional<GPlatesMaths::PointOnSphere> current_point_on_sphere =
		qpointf_to_point_on_sphere(current_point_on_scene, projection);
	if ( ! current_point_on_sphere)
	{
		return false;
	}

	return ((d_canvas_tool_ptr.get())->*func)(
			*initial_point_on_sphere,
			was_on_surface,
			map_view().current_proximity_inclusion_threshold(
				*initial_point_on_sphere),
			*current_point_on_sphere,
			is_on_surface,
			map_view().current_proximity_inclusion_threshold(
				*current_point_on_sphere),
			qpointf_to_point_on_sphere(map_transform().get_centre_of_viewport(), projection));
}


void
GPlatesCanvasTools::CanvasToolAdapterForMap::handle_left_press(
		const QPointF &click_point_on_scene,
		bool is_on_surface)
{
	invoke_canvas_tool_func(
			click_point_on_scene,
			is_on_surface,
			&CanvasTool::handle_left_press);
}


void
GPlatesCanvasTools::CanvasToolAdapterForMap::handle_left_click(
		const QPointF &click_point_on_scene,
		bool is_on_surface)
{
	invoke_canvas_tool_func(
			click_point_on_scene,
			is_on_surface,
			&CanvasTool::handle_left_click);
}


void
GPlatesCanvasTools::CanvasToolAdapterForMap::handle_left_drag(
		const QPointF &initial_point_on_scene,
		bool was_on_surface,
		const QPointF &current_point_on_scene,
		bool is_on_surface,
		const QPointF &translation)
{
	invoke_canvas_tool_func(
			initial_point_on_scene,
			was_on_surface,
			current_point_on_scene,
			is_on_surface,
			&CanvasTool::handle_left_drag);
}


void
GPlatesCanvasTools::CanvasToolAdapterForMap::handle_left_release_after_drag(
		const QPointF &initial_point_on_scene,
		bool was_on_surface,
		const QPointF &current_point_on_scene,
		bool is_on_surface,
		const QPointF &translation)
{
	invoke_canvas_tool_func(
			initial_point_on_scene,
			was_on_surface,
			current_point_on_scene,
			is_on_surface,
			&CanvasTool::handle_left_release_after_drag);
}


void
GPlatesCanvasTools::CanvasToolAdapterForMap::handle_shift_left_click(
		const QPointF &click_point_on_scene,
		bool is_on_surface)
{
	invoke_canvas_tool_func(
			click_point_on_scene,
			is_on_surface,
			&CanvasTool::handle_shift_left_click);
}


void
GPlatesCanvasTools::CanvasToolAdapterForMap::handle_shift_left_drag(
		const QPointF &initial_point_on_scene,
		bool was_on_surface,
		const QPointF &current_point_on_scene,
		bool is_on_surface,
		const QPointF &translation)
{
	invoke_canvas_tool_func(
			initial_point_on_scene,
			was_on_surface,
			current_point_on_scene,
			is_on_surface,
			&CanvasTool::handle_shift_left_drag);
}


void
GPlatesCanvasTools::CanvasToolAdapterForMap::handle_shift_left_release_after_drag(
		const QPointF &initial_point_on_scene,
		bool was_on_surface,
		const QPointF &current_point_on_scene,
		bool is_on_surface)
{
	invoke_canvas_tool_func(
			initial_point_on_scene,
			was_on_surface,
			current_point_on_scene,
			is_on_surface,
			&CanvasTool::handle_shift_left_release_after_drag);
}


void
GPlatesCanvasTools::CanvasToolAdapterForMap::handle_ctrl_left_click(
		const QPointF &click_point_on_scene,
		bool is_on_surface)
{
	invoke_canvas_tool_func(
			click_point_on_scene,
			is_on_surface,
			&CanvasTool::handle_ctrl_left_click);
}


void
GPlatesCanvasTools::CanvasToolAdapterForMap::handle_ctrl_left_drag(
		const QPointF &initial_point_on_scene,
		bool was_on_surface,
		const QPointF &current_point_on_scene,
		bool is_on_surface,
		const QPointF &translation)
{
	if (invoke_canvas_tool_func(
			initial_point_on_scene,
			was_on_surface,
			current_point_on_scene,
			is_on_surface,
			&CanvasTool::handle_ctrl_left_drag))
	{
		// Perform default action.
		MapCanvasTool::handle_ctrl_left_drag(
				initial_point_on_scene,
				was_on_surface,
				current_point_on_scene,
				is_on_surface,
				translation);
	}
}


void
GPlatesCanvasTools::CanvasToolAdapterForMap::handle_ctrl_left_release_after_drag(
		const QPointF &initial_point_on_scene,
		bool was_on_surface,
		const QPointF &current_point_on_scene,
		bool is_on_surface)
{
	if (invoke_canvas_tool_func(
			initial_point_on_scene,
			was_on_surface,
			current_point_on_scene,
			is_on_surface,
			&CanvasTool::handle_ctrl_left_release_after_drag))
	{
		// Perform default action.
		MapCanvasTool::handle_ctrl_left_release_after_drag(
				initial_point_on_scene,
				was_on_surface,
				current_point_on_scene,
				is_on_surface);
	}
}


void
GPlatesCanvasTools::CanvasToolAdapterForMap::handle_shift_ctrl_left_click(
		const QPointF &click_point_on_scene,
		bool is_on_surface)
{
	invoke_canvas_tool_func(
			click_point_on_scene,
			is_on_surface,
			&CanvasTool::handle_shift_ctrl_left_click);
}


void
GPlatesCanvasTools::CanvasToolAdapterForMap::handle_shift_ctrl_left_drag(
		const QPointF &initial_point_on_scene,
		bool was_on_surface,
		const QPointF &current_point_on_scene,
		bool is_on_surface,
		const QPointF &translation)
{
	if (invoke_canvas_tool_func(
			initial_point_on_scene,
			was_on_surface,
			current_point_on_scene,
			is_on_surface,
			&CanvasTool::handle_ctrl_left_drag))
	{
		// Perform default action.
		MapCanvasTool::handle_ctrl_left_drag(
				initial_point_on_scene,
				was_on_surface,
				current_point_on_scene,
				is_on_surface,
				translation);
	}
}


void
GPlatesCanvasTools::CanvasToolAdapterForMap::handle_move_without_drag(
		const QPointF &current_point_on_scene,
		bool is_on_surface,
		const QPointF &translation)
{
	invoke_canvas_tool_func(
			current_point_on_scene,
			is_on_surface,
			&CanvasTool::handle_move_without_drag);
}

