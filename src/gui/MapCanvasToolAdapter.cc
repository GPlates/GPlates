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

#include "qt-widgets/MapCanvas.h"


GPlatesGui::MapCanvasToolAdapter::MapCanvasToolAdapter(
		GPlatesQtWidgets::MapCanvas &map_canvas) :
	d_map_canvas(map_canvas)
{
}


void
GPlatesGui::MapCanvasToolAdapter::activate_canvas_tool(
		MapCanvasTool &map_canvas_tool)
{
	// Make sure we don't have multiple connections if we already have an active canvas tool (and hence connection).
	if (!d_active_map_canvas_tool)
	{
		connect_to_map_canvas();
	}

	d_active_map_canvas_tool = map_canvas_tool;
}


void
GPlatesGui::MapCanvasToolAdapter::deactivate_canvas_tool()
{
	d_active_map_canvas_tool = boost::none;
	disconnect_from_map_canvas();
}


void
GPlatesGui::MapCanvasToolAdapter::handle_press(
		int screen_width,
		int screen_height,
		const QPointF &press_screen_position,
		const boost::optional<QPointF> &press_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &press_position_on_globe,
		Qt::MouseButton button,
		Qt::KeyboardModifiers modifiers)
{
	switch (button)
	{
	case Qt::LeftButton:
		switch (modifiers)
		{
		case Qt::NoModifier:
			get_active_map_canvas_tool().handle_left_press(
					screen_width, screen_height,
					press_screen_position, press_map_position, press_position_on_globe);
			break;

		case Qt::ShiftModifier:
		case Qt::AltModifier:
		case Qt::ControlModifier:
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
		int screen_width,
		int screen_height,
		const QPointF &click_screen_position,
		const boost::optional<QPointF> &click_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &click_position_on_globe,
		Qt::MouseButton button,
		Qt::KeyboardModifiers modifiers)
{
	switch (button)
	{
	case Qt::LeftButton:
		switch (modifiers)
		{
		case Qt::NoModifier:
			get_active_map_canvas_tool().handle_left_click(
					screen_width, screen_height,
					click_screen_position, click_map_position, click_position_on_globe);
			break;

		case Qt::ShiftModifier:
			get_active_map_canvas_tool().handle_shift_left_click(
					screen_width, screen_height,
					click_screen_position, click_map_position, click_position_on_globe);
			break;

		case Qt::AltModifier:
			get_active_map_canvas_tool().handle_alt_left_click(
					screen_width, screen_height,
					click_screen_position, click_map_position, click_position_on_globe);
			break;

		case Qt::ControlModifier:
			get_active_map_canvas_tool().handle_ctrl_left_click(
					screen_width, screen_height,
					click_screen_position, click_map_position, click_position_on_globe);
			break;

		default:
			// This is an ugly way of getting around the fact that
			// (Qt::ShiftModifier | Qt::ControlModifier) is not a constant-expression,
			// and so cannot be used as a case label.
			if (modifiers == (Qt::ShiftModifier | Qt::ControlModifier))
			{
				// The user was indeed holding the Shift and Control keys.
				get_active_map_canvas_tool().handle_shift_ctrl_left_click(
						screen_width, screen_height,
						click_screen_position, click_map_position, click_position_on_globe);
			}
			else if (modifiers == (Qt::AltModifier | Qt::ControlModifier))
			{
				// The user was indeed holding the Alt and Control keys.
				get_active_map_canvas_tool().handle_alt_ctrl_left_click(
						screen_width, screen_height,
						click_screen_position, click_map_position, click_position_on_globe);
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
GPlatesGui::MapCanvasToolAdapter::handle_drag(
		int screen_width,
		int screen_height,
		const QPointF &initial_screen_position,
		const boost::optional<QPointF> &initial_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &initial_position_on_globe,
		const QPointF &current_screen_position,
		const boost::optional<QPointF> &current_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &current_position_on_globe,
		const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe,
		Qt::MouseButton button,
		Qt::KeyboardModifiers modifiers)
{
	//qDebug() << "Handling drag on the map...";
	switch (button)
	{
	case Qt::LeftButton:
		switch (modifiers)
		{
		case Qt::NoModifier:
			get_active_map_canvas_tool().handle_left_drag(
					screen_width, screen_height,
					initial_screen_position, initial_map_position, initial_position_on_globe,
					current_screen_position, current_map_position, current_position_on_globe,
					centre_of_viewport_on_globe);
			break;

		case Qt::ShiftModifier:
			get_active_map_canvas_tool().handle_shift_left_drag(
					screen_width, screen_height,
					initial_screen_position, initial_map_position, initial_position_on_globe,
					current_screen_position, current_map_position, current_position_on_globe,
					centre_of_viewport_on_globe);
			break;

		case Qt::AltModifier:
			get_active_map_canvas_tool().handle_alt_left_drag(
					screen_width, screen_height,
					initial_screen_position, initial_map_position, initial_position_on_globe,
					current_screen_position, current_map_position, current_position_on_globe,
					centre_of_viewport_on_globe);
			break;

		case Qt::ControlModifier:
			get_active_map_canvas_tool().handle_ctrl_left_drag(
					screen_width, screen_height,
					initial_screen_position, initial_map_position, initial_position_on_globe,
					current_screen_position, current_map_position, current_position_on_globe,
					centre_of_viewport_on_globe);
			break;

		default:
			// This is an ugly way of getting around the fact that
			// (Qt::ShiftModifier | Qt::ControlModifier) is not a constant-expression,
			// and so cannot be used as a case label.
			if (modifiers == (Qt::ShiftModifier | Qt::ControlModifier))
			{
				// The user was indeed holding the Shift and Control keys.
				get_active_map_canvas_tool().handle_shift_ctrl_left_drag(
						screen_width, screen_height,
						initial_screen_position, initial_map_position, initial_position_on_globe,
						current_screen_position, current_map_position, current_position_on_globe,
						centre_of_viewport_on_globe);
			}
			else if (modifiers == (Qt::AltModifier | Qt::ControlModifier))
			{
				// The user was indeed holding the Alt and Control keys.
				get_active_map_canvas_tool().handle_alt_ctrl_left_drag(
						screen_width, screen_height,
						initial_screen_position, initial_map_position, initial_position_on_globe,
						current_screen_position, current_map_position, current_position_on_globe,
						centre_of_viewport_on_globe);
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
		int screen_width,
		int screen_height,
		const QPointF &initial_screen_position,
		const boost::optional<QPointF> &initial_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &initial_position_on_globe,
		const QPointF &current_screen_position,
		const boost::optional<QPointF> &current_map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &current_position_on_globe,
		const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe,
		Qt::MouseButton button,
		Qt::KeyboardModifiers modifiers)
{
	switch (button)
	{
	case Qt::LeftButton:
		switch (modifiers)
		{
		case Qt::NoModifier:
			get_active_map_canvas_tool().handle_left_release_after_drag(
						screen_width, screen_height,
						initial_screen_position, initial_map_position, initial_position_on_globe,
						current_screen_position, current_map_position, current_position_on_globe,
						centre_of_viewport_on_globe);
			break;

		case Qt::ShiftModifier:
			get_active_map_canvas_tool().handle_shift_left_release_after_drag(
						screen_width, screen_height,
						initial_screen_position, initial_map_position, initial_position_on_globe,
						current_screen_position, current_map_position, current_position_on_globe,
						centre_of_viewport_on_globe);
			break;

		case Qt::AltModifier:
			get_active_map_canvas_tool().handle_alt_left_release_after_drag(
						screen_width, screen_height,
						initial_screen_position, initial_map_position, initial_position_on_globe,
						current_screen_position, current_map_position, current_position_on_globe,
						centre_of_viewport_on_globe);
			break;

		case Qt::ControlModifier:
			get_active_map_canvas_tool().handle_ctrl_left_release_after_drag(
						screen_width, screen_height,
						initial_screen_position, initial_map_position, initial_position_on_globe,
						current_screen_position, current_map_position, current_position_on_globe,
						centre_of_viewport_on_globe);
			break;

		default:
			// This is an ugly way of getting around the fact that
			// (Qt::ShiftModifier | Qt::ControlModifier) is not a constant-expression,
			// and so cannot be used as a case label.
			if (modifiers == (Qt::ShiftModifier | Qt::ControlModifier))
			{
				// The user was indeed holding the Shift and Control keys.
				get_active_map_canvas_tool().handle_shift_ctrl_left_release_after_drag(
						screen_width, screen_height,
						initial_screen_position, initial_map_position, initial_position_on_globe,
						current_screen_position, current_map_position, current_position_on_globe,
						centre_of_viewport_on_globe);
			}
			else if (modifiers == (Qt::AltModifier | Qt::ControlModifier))
			{
				// The user was indeed holding the Alt and Control keys.
				get_active_map_canvas_tool().handle_alt_ctrl_left_release_after_drag(
						screen_width, screen_height,
						initial_screen_position, initial_map_position, initial_position_on_globe,
						current_screen_position, current_map_position, current_position_on_globe,
						centre_of_viewport_on_globe);
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
GPlatesGui::MapCanvasToolAdapter::handle_move_without_drag(
		int screen_width,
		int screen_height,
		const QPointF &screen_position,
		const boost::optional<QPointF> &map_position,
		const boost::optional<GPlatesMaths::PointOnSphere> &position_on_globe,
		const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport_on_globe)
{
	get_active_map_canvas_tool().handle_move_without_drag(
			screen_width, screen_height,
			screen_position, map_position, position_on_globe,
			centre_of_viewport_on_globe);
}


void
GPlatesGui::MapCanvasToolAdapter::connect_to_map_canvas()
{
	QObject::connect(
			&d_map_canvas,
			SIGNAL(mouse_pressed(
					int,
					int,
					const QPointF &,
					const boost::optional<QPointF> &,
					const boost::optional<GPlatesMaths::PointOnSphere> &,
					Qt::MouseButton,
					Qt::KeyboardModifiers)),
			this,
			SLOT(handle_press(
					int,
					int,
					const QPointF &,
					const boost::optional<QPointF> &,
					const boost::optional<GPlatesMaths::PointOnSphere> &,
					Qt::MouseButton,
					Qt::KeyboardModifiers)));

	QObject::connect(
			&d_map_canvas,
			SIGNAL(mouse_clicked(
					int,
					int,
					const QPointF &,
					const boost::optional<QPointF> &,
					const boost::optional<GPlatesMaths::PointOnSphere> &,
					Qt::MouseButton,
					Qt::KeyboardModifiers)),
			this,
			SLOT(handle_click(
					int,
					int,
					const QPointF &,
					const boost::optional<QPointF> &,
					const boost::optional<GPlatesMaths::PointOnSphere> &,
					Qt::MouseButton,
					Qt::KeyboardModifiers)));

	QObject::connect(
			&d_map_canvas,
			SIGNAL(mouse_dragged(
					int,
					int,
					const QPointF &,
					const boost::optional<QPointF> &,
					const boost::optional<GPlatesMaths::PointOnSphere> &,
					const QPointF &,
					const boost::optional<QPointF> &,
					const boost::optional<GPlatesMaths::PointOnSphere> &,
					const boost::optional<GPlatesMaths::PointOnSphere> &,
					Qt::MouseButton,
					Qt::KeyboardModifiers)),
			this,
			SLOT(handle_drag(
					int,
					int,
					const QPointF &,
					const boost::optional<QPointF> &,
					const boost::optional<GPlatesMaths::PointOnSphere> &,
					const QPointF &,
					const boost::optional<QPointF> &,
					const boost::optional<GPlatesMaths::PointOnSphere> &,
					const boost::optional<GPlatesMaths::PointOnSphere> &,
					Qt::MouseButton,
					Qt::KeyboardModifiers)));

	QObject::connect(
			&d_map_canvas,
			SIGNAL(mouse_released_after_drag(
					int,
					int,
					const QPointF &,
					const boost::optional<QPointF> &,
					const boost::optional<GPlatesMaths::PointOnSphere> &,
					const QPointF &,
					const boost::optional<QPointF> &,
					const boost::optional<GPlatesMaths::PointOnSphere> &,
					const boost::optional<GPlatesMaths::PointOnSphere> &,
					Qt::MouseButton,
					Qt::KeyboardModifiers)),
			this,
			SLOT(handle_release_after_drag(
					int,
					int,
					const QPointF &,
					const boost::optional<QPointF> &,
					const boost::optional<GPlatesMaths::PointOnSphere> &,
					const QPointF &,
					const boost::optional<QPointF> &,
					const boost::optional<GPlatesMaths::PointOnSphere> &,
					const boost::optional<GPlatesMaths::PointOnSphere> &,
					Qt::MouseButton,
					Qt::KeyboardModifiers)));


	QObject::connect(
			&d_map_canvas,
			SIGNAL(mouse_moved_without_drag(
					int,
					int,
					const QPointF &,
					const boost::optional<QPointF> &,
					const boost::optional<GPlatesMaths::PointOnSphere> &,
					const boost::optional<GPlatesMaths::PointOnSphere> &)),
			this,
			SLOT(handle_move_without_drag(
					int,
					int,
					const QPointF &,
					const boost::optional<QPointF> &,
					const boost::optional<GPlatesMaths::PointOnSphere> &,
					const boost::optional<GPlatesMaths::PointOnSphere> &)));
}


void
GPlatesGui::MapCanvasToolAdapter::disconnect_from_map_canvas()
{
	// Disconnect all signals connected to us.
	QObject::disconnect(&d_map_canvas, 0, this, 0);
}


GPlatesGui::MapCanvasTool &
GPlatesGui::MapCanvasToolAdapter::get_active_map_canvas_tool()
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_active_map_canvas_tool,
			GPLATES_ASSERTION_SOURCE);

	return *d_active_map_canvas_tool;
}
