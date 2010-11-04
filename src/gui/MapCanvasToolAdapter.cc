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
#include "MapCanvasToolChoice.h"

#include "view-operations/RenderedGeometryCollection.h"

namespace
{
	inline
	GPlatesGui::MapCanvasTool &
	get_tool(
		const GPlatesGui::MapCanvasToolAdapter &adapter)
	{
		return adapter.map_canvas_tool_choice().tool_choice();
	}
}

void
GPlatesGui::MapCanvasToolAdapter::handle_press(
	const QPointF &point_on_scene,
	bool is_on_surface,
	Qt::MouseButton button,
	Qt::KeyboardModifiers modifiers)
{
	switch (button) {
	case Qt::LeftButton:
		switch (modifiers) {
	case Qt::NoModifier:
		get_tool(*this).handle_left_press(point_on_scene, is_on_surface);
		break;
	case Qt::ShiftModifier:
		break;
	case Qt::ControlModifier:
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
GPlatesGui::MapCanvasToolAdapter::handle_click(
	const QPointF &point_on_scene,
	bool is_on_surface,
	Qt::MouseButton button,
	Qt::KeyboardModifiers modifiers)
{
	switch (button) {
	case Qt::LeftButton:
		switch (modifiers) {
	case Qt::NoModifier:
		get_tool(*this).handle_left_click(point_on_scene, is_on_surface);
		break;
	case Qt::ShiftModifier:
		get_tool(*this).handle_shift_left_click(point_on_scene, is_on_surface);
		break;
	case Qt::ControlModifier:
		get_tool(*this).handle_ctrl_left_click(point_on_scene, is_on_surface);
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
	switch (button) {
	case Qt::LeftButton:
		switch (modifiers) {
		case Qt::NoModifier:
			get_tool(*this).handle_left_drag(initial_point_on_scene,
					was_on_surface,
					current_point_on_scene,
					is_on_surface,
					translation);
			break;
		case Qt::ShiftModifier:
			get_tool(*this).handle_shift_left_drag(initial_point_on_scene,
					was_on_surface,
					current_point_on_scene,
					is_on_surface,
					translation);
			break;
		case Qt::ControlModifier:
			get_tool(*this).handle_ctrl_left_drag(initial_point_on_scene,
					was_on_surface,
					current_point_on_scene,
					is_on_surface,
					translation);
			break;
		default:
			// This is an ugly way of getting around the fact that
			// (Qt::ShiftModifier | Qt::ControlModifier) is not a constant-expression,
			// and so cannot be used as a case label.
			if (modifiers == (Qt::ShiftModifier | Qt::ControlModifier)) {
				// The user was indeed holding the Shift and Control keys.
				get_tool(*this).handle_shift_ctrl_left_drag(initial_point_on_scene,
					was_on_surface,
					current_point_on_scene,
					is_on_surface,
					translation);
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
GPlatesGui::MapCanvasToolAdapter::handle_release_after_drag(
		const QPointF &initial_point_on_scene,
		bool was_on_surface,
		const QPointF &current_point_on_scene,
		bool is_on_surface,
		const QPointF &translation,
		Qt::MouseButton button,
		Qt::KeyboardModifiers modifiers)
{
	switch (button) {
	case Qt::LeftButton:
		switch (modifiers) {
		case Qt::NoModifier:
			get_tool(*this).handle_left_release_after_drag(initial_point_on_scene,
					was_on_surface,
					current_point_on_scene,
					is_on_surface,
					translation
					);
			break;
		case Qt::ShiftModifier:
			get_tool(*this).handle_shift_left_release_after_drag(initial_point_on_scene,
					was_on_surface,
					current_point_on_scene,
					is_on_surface);
		case Qt::ControlModifier:
			get_tool(*this).handle_ctrl_left_release_after_drag(initial_point_on_scene,
					was_on_surface,
					current_point_on_scene,
					is_on_surface);
			break;
		default:
			// This is an ugly way of getting around the fact that
			// (Qt::ShiftModifier | Qt::ControlModifier) is not a constant-expression,
			// and so cannot be used as a case label.
			if (modifiers == (Qt::ShiftModifier | Qt::ControlModifier)) {
				// The user was indeed holding the Shift and Control keys.
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
GPlatesGui::MapCanvasToolAdapter::handle_move_without_drag(
	const QPointF &current_point_on_scene,
	bool is_on_surface,
	const QPointF &translation)
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block. This is so we can do multiple changes
	// without redrawing canvas after each change.
	// This should ideally be located at the highest level to capture one
	// user GUI interaction - the user performs an action and we update canvas once.
	// But since these guards can be nested it's probably a good idea to have it here too.
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

	get_tool(*this).handle_move_without_drag(
		current_point_on_scene,
		is_on_surface,
		translation);
}

