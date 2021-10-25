/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 Geological Survey of Norway
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

#include <QDebug>

#include "MapCanvasToolAdapter.h"

#include "MapCanvasTool.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "qt-widgets/MapView.h"


GPlatesGui::MapCanvasToolAdapter::MapCanvasToolAdapter(
		GPlatesQtWidgets::MapView &map_view) :
	d_map_view(map_view)
{
}


void
GPlatesGui::MapCanvasToolAdapter::activate_canvas_tool(
		MapCanvasTool &map_canvas_tool)
{
	// Make sure we don't have multiple connections if we already have an active canvas tool (and hence connection).
	if (!d_active_map_canvas_tool)
	{
		connect_to_map_view();
	}

	d_active_map_canvas_tool = map_canvas_tool;
}


void
GPlatesGui::MapCanvasToolAdapter::deactivate_canvas_tool()
{
	d_active_map_canvas_tool = boost::none;
	disconnect_from_map_view();
}


void
GPlatesGui::MapCanvasToolAdapter::handle_press(
		const QPointF &point_on_scene,
		bool is_on_surface,
		Qt::MouseButton button,
		Qt::KeyboardModifiers modifiers)
{
	switch (button)
	{
	case Qt::LeftButton:
		switch (modifiers)
		{
		case Qt::NoModifier:
			get_active_map_canvas_tool().handle_left_press(point_on_scene, is_on_surface);
			break;

		case Qt::ShiftModifier:
			break;

		case Qt::ControlModifier:
			break;

		default:
			break;
		}
		break;

	case Qt::RightButton:
		break;

	default:
		break;
	}
}


void
GPlatesGui::MapCanvasToolAdapter::handle_click(
		const QPointF &point_on_scene,
		bool is_on_surface,
		Qt::MouseButton button,
		Qt::KeyboardModifiers modifiers)
{
	switch (button)
	{
	case Qt::LeftButton:
		switch (modifiers)
		{
		case Qt::NoModifier:
			get_active_map_canvas_tool().handle_left_click(point_on_scene, is_on_surface);
			break;

		case Qt::ShiftModifier:
			get_active_map_canvas_tool().handle_shift_left_click(point_on_scene, is_on_surface);
			break;

		case Qt::ControlModifier:
			get_active_map_canvas_tool().handle_ctrl_left_click(point_on_scene, is_on_surface);
			break;

		default:
			break;
		}
		break;

	case Qt::RightButton:
		break;

	default:
		break;
	}
}





void
GPlatesGui::MapCanvasToolAdapter::handle_drag(
		const QPointF &initial_point_on_scene,
		bool was_on_surface,
		const QPointF &current_point_on_scene,
		bool is_on_surface,
		Qt::MouseButton button,
		Qt::KeyboardModifiers modifiers,
		const QPointF &translation)
{
	//qDebug() << "Handling drag on the map...";
	switch (button)
	{
	case Qt::LeftButton:
		switch (modifiers)
		{
		case Qt::NoModifier:
			get_active_map_canvas_tool().handle_left_drag(initial_point_on_scene,
					was_on_surface,
					current_point_on_scene,
					is_on_surface,
					translation);
			break;

		case Qt::ShiftModifier:
			get_active_map_canvas_tool().handle_shift_left_drag(initial_point_on_scene,
					was_on_surface,
					current_point_on_scene,
					is_on_surface,
					translation);
			break;

		case Qt::ControlModifier:
			get_active_map_canvas_tool().handle_ctrl_left_drag(initial_point_on_scene,
					was_on_surface,
					current_point_on_scene,
					is_on_surface,
					translation);
			break;

		default:
			// This is an ugly way of getting around the fact that
			// (Qt::ShiftModifier | Qt::ControlModifier) is not a constant-expression,
			// and so cannot be used as a case label.
			if (modifiers == (Qt::ShiftModifier | Qt::ControlModifier))
			{
				// The user was indeed holding the Shift and Control keys.
				get_active_map_canvas_tool().handle_shift_ctrl_left_drag(initial_point_on_scene,
					was_on_surface,
					current_point_on_scene,
					is_on_surface,
					translation);
			}
			break;
		}
		break;

	case Qt::RightButton:
		break;

	default:
		break;
	}
}

void
GPlatesGui::MapCanvasToolAdapter::handle_release_after_drag(
		const QPointF &initial_point_on_scene,
		bool was_on_surface,
		const QPointF &current_point_on_scene,
		bool is_on_surface,
		const QPointF &translation,
		Qt::MouseButton button,
		Qt::KeyboardModifiers modifiers)
{
	switch (button)
	{
	case Qt::LeftButton:
		switch (modifiers)
		{
		case Qt::NoModifier:
			get_active_map_canvas_tool().handle_left_release_after_drag(initial_point_on_scene,
					was_on_surface,
					current_point_on_scene,
					is_on_surface,
					translation);
			break;

		case Qt::ShiftModifier:
			get_active_map_canvas_tool().handle_shift_left_release_after_drag(initial_point_on_scene,
					was_on_surface,
					current_point_on_scene,
					is_on_surface);
			break;

		case Qt::ControlModifier:
			get_active_map_canvas_tool().handle_ctrl_left_release_after_drag(initial_point_on_scene,
					was_on_surface,
					current_point_on_scene,
					is_on_surface);
			break;

		default:
			break;
		}
		break;

	case Qt::RightButton:
		break;

	default:
		break;
	}
}

void
GPlatesGui::MapCanvasToolAdapter::handle_move_without_drag(
		const QPointF &current_point_on_scene,
		bool is_on_surface,
		const QPointF &translation)
{
	get_active_map_canvas_tool().handle_move_without_drag(
		current_point_on_scene,
		is_on_surface,
		translation);
}


void
GPlatesGui::MapCanvasToolAdapter::connect_to_map_view()
{
	QObject::connect(
			&d_map_view,
			SIGNAL(mouse_pressed(
					const QPointF &,
					bool,
					Qt::MouseButton,
					Qt::KeyboardModifiers)),
			this,
			SLOT(handle_press(
					const QPointF &,
					bool,
					Qt::MouseButton,
					Qt::KeyboardModifiers)));

	QObject::connect(
			&d_map_view,
			SIGNAL(mouse_clicked(
					const QPointF &,
					bool,
					Qt::MouseButton,
					Qt::KeyboardModifiers)),
			this,
			SLOT(handle_click(
					const QPointF &,
				    bool,
					Qt::MouseButton,
					Qt::KeyboardModifiers)));

	QObject::connect(
			&d_map_view,
			SIGNAL(mouse_dragged(
					const QPointF &,
					bool,
					const QPointF &,
					bool,
					Qt::MouseButton,
					Qt::KeyboardModifiers,
					const QPointF &)),
			this,
			SLOT(handle_drag(
					const QPointF &,
					bool,
					const QPointF &,
					bool,
					Qt::MouseButton,
					Qt::KeyboardModifiers,
					const QPointF &)));

	QObject::connect(
			&d_map_view,
			SIGNAL(mouse_released_after_drag(
					const QPointF &,
					bool,
					const QPointF &,
					bool,
					const QPointF &,
					Qt::MouseButton,
					Qt::KeyboardModifiers)),
			this,
			SLOT(handle_release_after_drag(
					const QPointF &,
					bool,
					const QPointF &,
					bool,
					const QPointF &,
					Qt::MouseButton,
					Qt::KeyboardModifiers)));


	QObject::connect(
			&d_map_view,
			SIGNAL(mouse_moved_without_drag(
					const QPointF &,
					bool,
					const QPointF &)),
			this,
			SLOT(handle_move_without_drag(
					const QPointF &,
					bool,
					const QPointF &)));
}


void
GPlatesGui::MapCanvasToolAdapter::disconnect_from_map_view()
{
	// Disconnect all signals connected to us.
	QObject::disconnect(&d_map_view, 0, this, 0);
}


GPlatesGui::MapCanvasTool &
GPlatesGui::MapCanvasToolAdapter::get_active_map_canvas_tool()
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_active_map_canvas_tool,
			GPLATES_ASSERTION_SOURCE);

	return *d_active_map_canvas_tool;
}
