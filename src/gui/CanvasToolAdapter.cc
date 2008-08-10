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

#include "CanvasToolAdapter.h"
#include "CanvasToolChoice.h"


namespace
{
	inline
	GPlatesGui::CanvasTool &
	get_tool(
			const GPlatesGui::CanvasToolAdapter &adapter)
	{
		return adapter.canvas_tool_choice().tool_choice();
	}
}


void
GPlatesGui::CanvasToolAdapter::handle_click(
		const GPlatesMaths::PointOnSphere &click_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
		bool is_on_globe,
		Qt::MouseButton button,
		Qt::KeyboardModifiers modifiers)
{
	switch (button) {
	case Qt::LeftButton:
		switch (modifiers) {
		case Qt::NoModifier:
			get_tool(*this).handle_left_click(click_pos_on_globe,
					oriented_click_pos_on_globe, is_on_globe);
			break;

		case Qt::ShiftModifier:
			break;

		case Qt::ControlModifier:
			get_tool(*this).handle_ctrl_left_click(click_pos_on_globe,
					oriented_click_pos_on_globe, is_on_globe);
			break;

		default:
			// This is an ugly way of getting around the fact that
			// (Qt::ShiftModifier | Qt::ControlModifier) is not a constant-expression,
			// and so cannot be used as a case label.
			if (modifiers == (Qt::ShiftModifier | Qt::ControlModifier)) {
			}
			else {
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
GPlatesGui::CanvasToolAdapter::handle_drag(
		const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
		bool was_on_globe,
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
		bool is_on_globe,
		Qt::MouseButton button,
		Qt::KeyboardModifiers modifiers)
{
	switch (button) {
	case Qt::LeftButton:
		switch (modifiers) {
		case Qt::NoModifier:
			get_tool(*this).handle_left_drag(initial_pos_on_globe,
					oriented_initial_pos_on_globe, was_on_globe,
					current_pos_on_globe,
					oriented_current_pos_on_globe, is_on_globe);
			break;

		case Qt::ShiftModifier:
			break;

		case Qt::ControlModifier:
			get_tool(*this).handle_ctrl_left_drag(initial_pos_on_globe,
					oriented_initial_pos_on_globe, was_on_globe,
					current_pos_on_globe,
					oriented_current_pos_on_globe, is_on_globe);
			break;

		default:
			// This is an ugly way of getting around the fact that
			// (Qt::ShiftModifier | Qt::ControlModifier) is not a constant-expression,
			// and so cannot be used as a case label.
			if (modifiers == (Qt::ShiftModifier | Qt::ControlModifier)) {
			}
			else {
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
GPlatesGui::CanvasToolAdapter::handle_release_after_drag(
		const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
		bool was_on_globe,
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
		bool is_on_globe,
		Qt::MouseButton button,
		Qt::KeyboardModifiers modifiers)
{
	switch (button) {
	case Qt::LeftButton:
		switch (modifiers) {
		case Qt::NoModifier:
			get_tool(*this).handle_left_release_after_drag(initial_pos_on_globe,
					oriented_initial_pos_on_globe, was_on_globe,
					current_pos_on_globe,
					oriented_current_pos_on_globe, is_on_globe);
			break;

		case Qt::ShiftModifier:
			break;

		case Qt::ControlModifier:
			get_tool(*this).handle_ctrl_left_release_after_drag(initial_pos_on_globe,
					oriented_initial_pos_on_globe, was_on_globe,
					current_pos_on_globe,
					oriented_current_pos_on_globe, is_on_globe);
			break;

		default:
			// This is an ugly way of getting around the fact that
			// (Qt::ShiftModifier | Qt::ControlModifier) is not a constant-expression,
			// and so cannot be used as a case label.
			if (modifiers == (Qt::ShiftModifier | Qt::ControlModifier)) {
			}
			else {
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
