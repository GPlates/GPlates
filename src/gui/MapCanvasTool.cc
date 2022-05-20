/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2009 Geological Survey of Norway
 * Copyright (C) 2010 The University of Sydney, Australia
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

#include "MapCanvasTool.h"

#include "view-operations/MapViewOperation.h"


GPlatesGui::MapCanvasTool::MapCanvasTool(
		GPlatesQtWidgets::MapCanvas &map_canvas_,
		GPlatesViewOperations::MapViewOperation &map_view_operation_) :
	d_map_canvas(map_canvas_),
	d_map_view_operation(map_view_operation_)
{  }


GPlatesGui::MapCanvasTool::~MapCanvasTool()
{  }


void
GPlatesGui::MapCanvasTool::pan_map_by_drag_update(
		int screen_width,
		int screen_height,
		const QPointF &initial_screen_position,
		const boost::optional<QPointF> &initial_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &initial_position_on_globe,
		const QPointF &current_screen_position,
		const boost::optional<QPointF> &current_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &current_position_on_globe,
		const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe)
{
	if (!d_map_view_operation.in_drag())
	{
		d_map_view_operation.start_drag(
				GPlatesViewOperations::MapViewOperation::DRAG_NORMAL,
				initial_screen_position,
				initial_map_position,
				screen_width, screen_height);
	}

	d_map_view_operation.update_drag(
			current_screen_position,
			current_map_position,
			screen_width, screen_height,
			false/*end_of_drag*/);
}


void
GPlatesGui::MapCanvasTool::pan_map_by_drag_release(
		int screen_width,
		int screen_height,
		const QPointF &initial_screen_position,
		const boost::optional<QPointF> &initial_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &initial_position_on_globe,
		const QPointF &current_screen_position,
		const boost::optional<QPointF> &current_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &current_position_on_globe,
		const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe)
{
	if (!d_map_view_operation.in_drag())
	{
		d_map_view_operation.start_drag(
				GPlatesViewOperations::MapViewOperation::DRAG_NORMAL,
				initial_screen_position,
				initial_map_position,
				screen_width, screen_height);
	}

	d_map_view_operation.update_drag(
			current_screen_position,
			current_map_position,
			screen_width, screen_height,
			true/*end_of_drag*/);
}


void
GPlatesGui::MapCanvasTool::rotate_map_by_drag_update(
		int screen_width,
		int screen_height,
		const QPointF &initial_screen_position,
		const boost::optional<QPointF> &initial_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &initial_position_on_globe,
		const QPointF &current_screen_position,
		const boost::optional<QPointF> &current_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &current_position_on_globe,
		const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe)
{
	if (!d_map_view_operation.in_drag())
	{
		d_map_view_operation.start_drag(
				GPlatesViewOperations::MapViewOperation::DRAG_ROTATE_AND_TILT,
				initial_screen_position,
				initial_map_position,
				screen_width, screen_height);
	}

	d_map_view_operation.update_drag(
			current_screen_position,
			current_map_position,
			screen_width, screen_height,
			false/*end_of_drag*/);
}


void
GPlatesGui::MapCanvasTool::rotate_map_by_drag_release(
		int screen_width,
		int screen_height,
		const QPointF &initial_screen_position,
		const boost::optional<QPointF> &initial_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &initial_position_on_globe,
		const QPointF &current_screen_position,
		const boost::optional<QPointF> &current_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &current_position_on_globe,
		const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe)
{
	if (!d_map_view_operation.in_drag())
	{
		d_map_view_operation.start_drag(
				GPlatesViewOperations::MapViewOperation::DRAG_ROTATE_AND_TILT,
				initial_screen_position,
				initial_map_position,
				screen_width, screen_height);
	}

	d_map_view_operation.update_drag(
			current_screen_position,
			current_map_position,
			screen_width, screen_height,
			true/*end_of_drag*/);
}


void
GPlatesGui::MapCanvasTool::tilt_map_by_drag_update(
		int screen_width,
		int screen_height,
		const QPointF &initial_screen_position,
		const boost::optional<QPointF> &initial_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &initial_position_on_globe,
		const QPointF &current_screen_position,
		const boost::optional<QPointF> &current_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &current_position_on_globe,
		const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe)
{
	if (!d_map_view_operation.in_drag())
	{
		d_map_view_operation.start_drag(
				GPlatesViewOperations::MapViewOperation::DRAG_TILT,
				initial_screen_position,
				initial_map_position,
				screen_width, screen_height);
	}

	d_map_view_operation.update_drag(
			current_screen_position,
			current_map_position,
			screen_width, screen_height,
			false/*end_of_drag*/);
}


void
GPlatesGui::MapCanvasTool::tilt_map_by_drag_release(
		int screen_width,
		int screen_height,
		const QPointF &initial_screen_position,
		const boost::optional<QPointF> &initial_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &initial_position_on_globe,
		const QPointF &current_screen_position,
		const boost::optional<QPointF> &current_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &current_position_on_globe,
		const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe)
{
	if (!d_map_view_operation.in_drag())
	{
		d_map_view_operation.start_drag(
				GPlatesViewOperations::MapViewOperation::DRAG_TILT,
				initial_screen_position,
				initial_map_position,
				screen_width, screen_height);
	}

	d_map_view_operation.update_drag(
			current_screen_position,
			current_map_position,
			screen_width, screen_height,
			true/*end_of_drag*/);
}
