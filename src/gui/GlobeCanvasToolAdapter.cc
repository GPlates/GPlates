/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007 The University of Sydney, Australia
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

#include "GlobeCanvasToolAdapter.h"

#include "GlobeCanvasTool.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "qt-widgets/GlobeCanvas.h"


GPlatesGui::GlobeCanvasToolAdapter::GlobeCanvasToolAdapter(
		GPlatesQtWidgets::GlobeCanvas &globe_canvas) :
	d_globe_canvas(globe_canvas)
{
}


void
GPlatesGui::GlobeCanvasToolAdapter::activate_canvas_tool(
		GlobeCanvasTool &globe_canvas_tool)
{
	// Make sure we don't have multiple connections if we already have an active canvas tool (and hence connection).
	if (!d_active_globe_canvas_tool)
	{
		connect_to_globe_canvas();
	}

	d_active_globe_canvas_tool = globe_canvas_tool;
}


void
GPlatesGui::GlobeCanvasToolAdapter::deactivate_canvas_tool()
{
	d_active_globe_canvas_tool = boost::none;
	disconnect_from_globe_canvas();
}


void
GPlatesGui::GlobeCanvasToolAdapter::handle_press(
		int screen_width,
		int screen_height,
		double press_screen_x,
		double press_screen_y,
		GPlatesMaths::PointOnSphere press_pos_on_globe,
		bool is_on_globe,
		Qt::MouseButton button,
		Qt::KeyboardModifiers modifiers)
{
	switch (button)
	{
	case Qt::LeftButton:
		switch (modifiers)
		{
		case Qt::NoModifier:
			get_active_globe_canvas_tool().handle_left_press(
					screen_width, screen_height,
					press_screen_x, press_screen_y,
					press_pos_on_globe, is_on_globe);
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
GPlatesGui::GlobeCanvasToolAdapter::handle_click(
		int screen_width,
		int screen_height,
		double click_screen_x,
		double click_screen_y,
		GPlatesMaths::PointOnSphere click_pos_on_globe,
		bool is_on_globe,
		Qt::MouseButton button,
		Qt::KeyboardModifiers modifiers)
{
	switch (button)
	{
	case Qt::LeftButton:
		switch (modifiers)
		{
		case Qt::NoModifier:
			get_active_globe_canvas_tool().handle_left_click(
					screen_width, screen_height,
					click_screen_x, click_screen_y,
					click_pos_on_globe, is_on_globe);
			break;

		case Qt::ShiftModifier:
			get_active_globe_canvas_tool().handle_shift_left_click(
					screen_width, screen_height,
					click_screen_x, click_screen_y,
					click_pos_on_globe, is_on_globe);
			break;

		case Qt::AltModifier:
			get_active_globe_canvas_tool().handle_alt_left_click(
					screen_width, screen_height,
					click_screen_x, click_screen_y,
					click_pos_on_globe, is_on_globe);
			break;

		case Qt::ControlModifier:
			get_active_globe_canvas_tool().handle_ctrl_left_click(
					screen_width, screen_height,
					click_screen_x, click_screen_y,
					click_pos_on_globe, is_on_globe);
			break;

		default:
			// This is an ugly way of getting around the fact that
			// (Qt::ShiftModifier | Qt::ControlModifier) is not a constant-expression,
			// and so cannot be used as a case label.
			if (modifiers == (Qt::ShiftModifier | Qt::ControlModifier))
			{
				// The user was indeed holding the Shift and Control keys.
				get_active_globe_canvas_tool().handle_shift_ctrl_left_click(
						screen_width, screen_height,
						click_screen_x, click_screen_y,
						click_pos_on_globe, is_on_globe);
			}
			else if (modifiers == (Qt::AltModifier | Qt::ControlModifier))
			{
				// The user was indeed holding the Alt and Control keys.
				get_active_globe_canvas_tool().handle_alt_ctrl_left_click(
						screen_width, screen_height,
						click_screen_x, click_screen_y,
						click_pos_on_globe, is_on_globe);
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
GPlatesGui::GlobeCanvasToolAdapter::handle_drag(
		int screen_width,
		int screen_height,
		double initial_screen_x,
		double initial_screen_y,
		GPlatesMaths::PointOnSphere initial_pos_on_globe,
		bool was_on_globe,
		double current_screen_x,
		double current_screen_y,
		GPlatesMaths::PointOnSphere current_pos_on_globe,
		bool is_on_globe,
		GPlatesMaths::PointOnSphere centre_of_viewport,
		Qt::MouseButton button,
		Qt::KeyboardModifiers modifiers)
{
	switch (button)
	{
	case Qt::LeftButton:
		switch (modifiers)
		{
		case Qt::NoModifier:
			get_active_globe_canvas_tool().handle_left_drag(
					screen_width, screen_height,
					initial_screen_x, initial_screen_y,
					initial_pos_on_globe, was_on_globe,
					current_screen_x, current_screen_y,
					current_pos_on_globe, is_on_globe,
					centre_of_viewport);
			break;

		case Qt::ShiftModifier:
			get_active_globe_canvas_tool().handle_shift_left_drag(
					screen_width, screen_height,
					initial_screen_x, initial_screen_y,
					initial_pos_on_globe, was_on_globe,
					current_screen_x, current_screen_y,
					current_pos_on_globe, is_on_globe,
					centre_of_viewport);
			break;

		case Qt::AltModifier:
			get_active_globe_canvas_tool().handle_alt_left_drag(
					screen_width, screen_height,
					initial_screen_x, initial_screen_y,
					initial_pos_on_globe, was_on_globe,
					current_screen_x, current_screen_y,
					current_pos_on_globe, is_on_globe,
					centre_of_viewport);
			break;

		case Qt::ControlModifier:
			get_active_globe_canvas_tool().handle_ctrl_left_drag(
					screen_width, screen_height,
					initial_screen_x, initial_screen_y,
					initial_pos_on_globe, was_on_globe,
					current_screen_x, current_screen_y,
					current_pos_on_globe, is_on_globe,
					centre_of_viewport);
			break;

		default:
			// This is an ugly way of getting around the fact that
			// (Qt::ShiftModifier | Qt::ControlModifier) is not a constant-expression,
			// and so cannot be used as a case label.
			if (modifiers == (Qt::ShiftModifier | Qt::ControlModifier))
			{
				// The user was indeed holding the Shift and Control keys.
				get_active_globe_canvas_tool().handle_shift_ctrl_left_drag(
						screen_width, screen_height,
						initial_screen_x, initial_screen_y,
						initial_pos_on_globe, was_on_globe,
						current_screen_x, current_screen_y,
						current_pos_on_globe, is_on_globe,
						centre_of_viewport);
			}
			else if (modifiers == (Qt::AltModifier | Qt::ControlModifier))
			{
				// The user was indeed holding the Alt and Control keys.
				get_active_globe_canvas_tool().handle_alt_ctrl_left_drag(
						screen_width, screen_height,
						initial_screen_x, initial_screen_y,
						initial_pos_on_globe, was_on_globe,
						current_screen_x, current_screen_y,
						current_pos_on_globe, is_on_globe,
						centre_of_viewport);
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
GPlatesGui::GlobeCanvasToolAdapter::handle_release_after_drag(
		int screen_width,
		int screen_height,
		double initial_screen_x,
		double initial_screen_y,
		GPlatesMaths::PointOnSphere initial_pos_on_globe,
		bool was_on_globe,
		double current_screen_x,
		double current_screen_y,
		GPlatesMaths::PointOnSphere current_pos_on_globe,
		bool is_on_globe,
		GPlatesMaths::PointOnSphere centre_of_viewport,
		Qt::MouseButton button,
		Qt::KeyboardModifiers modifiers)
{
	switch (button)
	{
	case Qt::LeftButton:
		switch (modifiers)
		{
		case Qt::NoModifier:
			get_active_globe_canvas_tool().handle_left_release_after_drag(
					screen_width, screen_height,
					initial_screen_x, initial_screen_y,
					initial_pos_on_globe, was_on_globe,
					current_screen_x, current_screen_y,
					current_pos_on_globe, is_on_globe,
					centre_of_viewport);
			break;

		case Qt::ShiftModifier:
			get_active_globe_canvas_tool().handle_shift_left_release_after_drag(
					screen_width, screen_height,
					initial_screen_x, initial_screen_y,
					initial_pos_on_globe, was_on_globe,
					current_screen_x, current_screen_y,
					current_pos_on_globe, is_on_globe,
					centre_of_viewport);
			break;

		case Qt::AltModifier:
			get_active_globe_canvas_tool().handle_alt_left_release_after_drag(
					screen_width, screen_height,
					initial_screen_x, initial_screen_y,
					initial_pos_on_globe, was_on_globe,
					current_screen_x, current_screen_y,
					current_pos_on_globe, is_on_globe,
					centre_of_viewport);
			break;

		case Qt::ControlModifier:
			get_active_globe_canvas_tool().handle_ctrl_left_release_after_drag(
					screen_width, screen_height,
					initial_screen_x, initial_screen_y,
					initial_pos_on_globe, was_on_globe,
					current_screen_x, current_screen_y,
					current_pos_on_globe, is_on_globe,
					centre_of_viewport);
			break;

		default:
			// This is an ugly way of getting around the fact that
			// (Qt::ShiftModifier | Qt::ControlModifier) is not a constant-expression,
			// and so cannot be used as a case label.
			if (modifiers == (Qt::ShiftModifier | Qt::ControlModifier))
			{
				// The user was indeed holding the Shift and Control keys.
				get_active_globe_canvas_tool().handle_shift_ctrl_left_release_after_drag(
						screen_width, screen_height,
						initial_screen_x, initial_screen_y,
						initial_pos_on_globe, was_on_globe,
						current_screen_x, current_screen_y,
						current_pos_on_globe, is_on_globe,
						centre_of_viewport);
			}
			else if (modifiers == (Qt::AltModifier | Qt::ControlModifier))
			{
				// The user was indeed holding the Alt and Control keys.
				get_active_globe_canvas_tool().handle_alt_ctrl_left_release_after_drag(
						screen_width, screen_height,
						initial_screen_x, initial_screen_y,
						initial_pos_on_globe, was_on_globe,
						current_screen_x, current_screen_y,
						current_pos_on_globe, is_on_globe,
						centre_of_viewport);
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
GPlatesGui::GlobeCanvasToolAdapter::handle_move_without_drag(
		int screen_width,
		int screen_height,
		double current_screen_x,
		double current_screen_y,
		GPlatesMaths::PointOnSphere current_pos_on_globe,
		bool is_on_globe,
		GPlatesMaths::PointOnSphere centre_of_viewport)
{
	get_active_globe_canvas_tool().handle_move_without_drag(
			screen_width, screen_height,
			current_screen_x, current_screen_y,
			current_pos_on_globe, is_on_globe,
			centre_of_viewport);
}


void
GPlatesGui::GlobeCanvasToolAdapter::connect_to_globe_canvas()
{
	QObject::connect(
			&d_globe_canvas,
			SIGNAL(mouse_pressed(
					int,
					int,
					double,
					double,
					GPlatesMaths::PointOnSphere,
					bool,
					Qt::MouseButton,
					Qt::KeyboardModifiers)),
			this,
			SLOT(handle_press(
					int,
					int,
					double,
					double,
					GPlatesMaths::PointOnSphere,
					bool,
					Qt::MouseButton,
					Qt::KeyboardModifiers)));

	QObject::connect(
			&d_globe_canvas,
			SIGNAL(mouse_clicked(
					int,
					int,
					double,
					double,
					GPlatesMaths::PointOnSphere,
					bool,
					Qt::MouseButton,
					Qt::KeyboardModifiers)),
			this,
			SLOT(handle_click(
					int,
					int,
					double,
					double,
					GPlatesMaths::PointOnSphere,
					bool,
					Qt::MouseButton,
					Qt::KeyboardModifiers)));

	QObject::connect(
			&d_globe_canvas,
			SIGNAL(mouse_dragged(
					int,
					int,
					double,
					double,
					GPlatesMaths::PointOnSphere,
					bool,
					double,
					double,
					GPlatesMaths::PointOnSphere,
					bool,
					GPlatesMaths::PointOnSphere,
					Qt::MouseButton,
					Qt::KeyboardModifiers)),
			this,
			SLOT(handle_drag(
					int,
					int,
					double,
					double,
					GPlatesMaths::PointOnSphere,
					bool,
					double,
					double,
					GPlatesMaths::PointOnSphere,
					bool,
					GPlatesMaths::PointOnSphere,
					Qt::MouseButton,
					Qt::KeyboardModifiers)));

	QObject::connect(
			&d_globe_canvas,
			SIGNAL(mouse_released_after_drag(
					int,
					int,
					double,
					double,
					GPlatesMaths::PointOnSphere,
					bool,
					double,
					double,
					GPlatesMaths::PointOnSphere,
					bool,
					GPlatesMaths::PointOnSphere,
					Qt::MouseButton,
					Qt::KeyboardModifiers)),
			this,
			SLOT(handle_release_after_drag(
					int,
					int,
					double,
					double,
					GPlatesMaths::PointOnSphere,
					bool,
					double,
					double,
					GPlatesMaths::PointOnSphere,
					bool,
					GPlatesMaths::PointOnSphere,
					Qt::MouseButton,
					Qt::KeyboardModifiers)));

	QObject::connect(
			&d_globe_canvas,
			SIGNAL(mouse_moved_without_drag(
					int,
					int,
					double,
					double,
					GPlatesMaths::PointOnSphere,
					bool,
					GPlatesMaths::PointOnSphere)),
			this,
			SLOT(handle_move_without_drag(
					int,
					int,
					double,
					double,
					GPlatesMaths::PointOnSphere,
					bool,
					GPlatesMaths::PointOnSphere)));
}


void
GPlatesGui::GlobeCanvasToolAdapter::disconnect_from_globe_canvas()
{
	// Disconnect all signals connected to us.
	QObject::disconnect(&d_globe_canvas, 0, this, 0);
}


GPlatesGui::GlobeCanvasTool &
GPlatesGui::GlobeCanvasToolAdapter::get_active_globe_canvas_tool()
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_active_globe_canvas_tool,
			GPLATES_ASSERTION_SOURCE);

	return *d_active_globe_canvas_tool;
}
