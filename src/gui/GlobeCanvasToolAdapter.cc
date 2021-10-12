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
#include "GlobeCanvasToolChoice.h"

#include "view-operations/RenderedGeometryCollection.h"

namespace
{
	inline
	GPlatesGui::GlobeCanvasTool &
	get_tool(
			const GPlatesGui::GlobeCanvasToolAdapter &adapter)
	{
		return adapter.canvas_tool_choice().tool_choice();
	}
}


void
GPlatesGui::GlobeCanvasToolAdapter::handle_click(
		const GPlatesMaths::PointOnSphere &click_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
		bool is_on_globe,
		Qt::MouseButton button,
		Qt::KeyboardModifiers modifiers)
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block. This is so we can do multiple changes
	// without redrawing canvas after each change.
	// This should ideally be located at the highest level to capture one
	// user GUI interaction - the user performs an action and we update canvas once.
	// But since these guards can be nested it's probably a good idea to have it here too.
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

	switch (button) {
	case Qt::LeftButton:
		switch (modifiers) {
		case Qt::NoModifier:
			get_tool(*this).handle_left_click(click_pos_on_globe,
					oriented_click_pos_on_globe, is_on_globe);
			break;

		case Qt::ShiftModifier:
			get_tool(*this).handle_shift_left_click(click_pos_on_globe,
					oriented_click_pos_on_globe, is_on_globe);
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
GPlatesGui::GlobeCanvasToolAdapter::handle_drag(
		const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
		bool was_on_globe,
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport,
		Qt::MouseButton button,
		Qt::KeyboardModifiers modifiers)
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block. This is so we can do multiple changes
	// without redrawing canvas after each change.
	// This should ideally be located at the highest level to capture one
	// user GUI interaction - the user performs an action and we update canvas once.
	// But since these guards can be nested it's probably a good idea to have it here too.
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

	switch (button) {
	case Qt::LeftButton:
		switch (modifiers) {
		case Qt::NoModifier:
			get_tool(*this).handle_left_drag(initial_pos_on_globe,
					oriented_initial_pos_on_globe, was_on_globe,
					current_pos_on_globe,
					oriented_current_pos_on_globe, is_on_globe,
					oriented_centre_of_viewport);
			break;

		case Qt::ShiftModifier:
			get_tool(*this).handle_shift_left_drag(initial_pos_on_globe,
					oriented_initial_pos_on_globe, was_on_globe,
					current_pos_on_globe,
					oriented_current_pos_on_globe, is_on_globe,
					oriented_centre_of_viewport);
			break;

		case Qt::ControlModifier:
			get_tool(*this).handle_ctrl_left_drag(initial_pos_on_globe,
					oriented_initial_pos_on_globe, was_on_globe,
					current_pos_on_globe,
					oriented_current_pos_on_globe, is_on_globe,
					oriented_centre_of_viewport);
			break;

		default:
			// This is an ugly way of getting around the fact that
			// (Qt::ShiftModifier | Qt::ControlModifier) is not a constant-expression,
			// and so cannot be used as a case label.
			if (modifiers == (Qt::ShiftModifier | Qt::ControlModifier)) {
				// The user was indeed holding the Shift and Control keys.
				get_tool(*this).handle_shift_ctrl_left_drag(initial_pos_on_globe,
						oriented_initial_pos_on_globe, was_on_globe,
						current_pos_on_globe,
						oriented_current_pos_on_globe, is_on_globe,
						oriented_centre_of_viewport);
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
GPlatesGui::GlobeCanvasToolAdapter::handle_release_after_drag(
		const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
		bool was_on_globe,
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport,
		Qt::MouseButton button,
		Qt::KeyboardModifiers modifiers)
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block. This is so we can do multiple changes
	// without redrawing canvas after each change.
	// This should ideally be located at the highest level to capture one
	// user GUI interaction - the user performs an action and we update canvas once.
	// But since these guards can be nested it's probably a good idea to have it here too.
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

	switch (button) {
	case Qt::LeftButton:
		switch (modifiers) {
		case Qt::NoModifier:
			get_tool(*this).handle_left_release_after_drag(initial_pos_on_globe,
					oriented_initial_pos_on_globe, was_on_globe,
					current_pos_on_globe,
					oriented_current_pos_on_globe, is_on_globe,
					oriented_centre_of_viewport);
			break;

		case Qt::ShiftModifier:
			get_tool(*this).handle_shift_left_release_after_drag(initial_pos_on_globe,
					oriented_initial_pos_on_globe, was_on_globe,
					current_pos_on_globe,
					oriented_current_pos_on_globe, is_on_globe,
					oriented_centre_of_viewport);
			break;

		case Qt::ControlModifier:
			get_tool(*this).handle_ctrl_left_release_after_drag(initial_pos_on_globe,
					oriented_initial_pos_on_globe, was_on_globe,
					current_pos_on_globe,
					oriented_current_pos_on_globe, is_on_globe,
					oriented_centre_of_viewport);
			break;

		default:
			// This is an ugly way of getting around the fact that
			// (Qt::ShiftModifier | Qt::ControlModifier) is not a constant-expression,
			// and so cannot be used as a case label.
			if (modifiers == (Qt::ShiftModifier | Qt::ControlModifier)) {
				// The user was indeed holding the Shift and Control keys.
				get_tool(*this).handle_shift_ctrl_left_release_after_drag(initial_pos_on_globe,
						oriented_initial_pos_on_globe, was_on_globe,
						current_pos_on_globe,
						oriented_current_pos_on_globe, is_on_globe,
						oriented_centre_of_viewport);
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
GPlatesGui::GlobeCanvasToolAdapter::handle_move_without_drag(
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block. This is so we can do multiple changes
	// without redrawing canvas after each change.
	// This should ideally be located at the highest level to capture one
	// user GUI interaction - the user performs an action and we update canvas once.
	// But since these guards can be nested it's probably a good idea to have it here too.
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

	get_tool(*this).handle_move_without_drag(
			current_pos_on_globe,
			oriented_current_pos_on_globe, is_on_globe,
			oriented_centre_of_viewport);
}
