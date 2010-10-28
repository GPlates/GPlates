/* $Id$ */

/**
 * @file 
 * Contains implementation of CanvasToolAdapterForGlobe
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

#include "CanvasToolAdapterForGlobe.h"

#include "maths/PointOnSphere.h"

#include "qt-widgets/GlobeCanvas.h"
#include "qt-widgets/ViewportWindow.h"


namespace
{
	const char *STATUS_BAR_MESSAGE_SUFFIX = QT_TR_NOOP("Ctrl+drag to re-orient the globe.");

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


GPlatesCanvasTools::CanvasToolAdapterForGlobe::CanvasToolAdapterForGlobe (
		CanvasTool *canvas_tool_ptr,
		GPlatesGui::Globe &globe_,
		GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
		GPlatesQtWidgets::ViewportWindow &viewport_window_) :
	GlobeCanvasTool(globe_, globe_canvas_),
	d_canvas_tool_ptr(canvas_tool_ptr)
{
	canvas_tool_ptr->set_status_bar_callback(
			boost::bind(
				&::set_status_bar_message,
				boost::ref(viewport_window_),
				_1));
}

const GPlatesCanvasTools::CanvasToolAdapterForGlobe::non_null_ptr_type
GPlatesCanvasTools::CanvasToolAdapterForGlobe::create(
		CanvasTool *canvas_tool_ptr,
		GPlatesGui::Globe &globe_,
		GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
		GPlatesQtWidgets::ViewportWindow &viewport_window) 
{
	CanvasToolAdapterForGlobe::non_null_ptr_type ptr(
			new CanvasToolAdapterForGlobe(
				canvas_tool_ptr,
				globe_,
				globe_canvas_,
				viewport_window),
			GPlatesUtils::NullIntrusivePointerHandler());
	return ptr;
}

void
GPlatesCanvasTools::CanvasToolAdapterForGlobe::handle_activation()
{
	if (globe_canvas().isVisible())
	{
		d_canvas_tool_ptr->handle_activation();
	}
}

void
GPlatesCanvasTools::CanvasToolAdapterForGlobe::handle_deactivation()
{
	d_canvas_tool_ptr->handle_deactivation();
}

void
GPlatesCanvasTools::CanvasToolAdapterForGlobe::handle_left_press(
	const GPlatesMaths::PointOnSphere &click_pos_on_globe,
	const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
	bool is_on_globe)
{
	if (globe_canvas().isVisible())
	{
		d_canvas_tool_ptr->handle_left_press(
			oriented_click_pos_on_globe,
			is_on_globe,
			globe_canvas().current_proximity_inclusion_threshold(
			click_pos_on_globe));
	}
}


void
GPlatesCanvasTools::CanvasToolAdapterForGlobe::handle_left_click(
		const GPlatesMaths::PointOnSphere &click_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
		bool is_on_globe)
{
	if (globe_canvas().isVisible())
	{
		d_canvas_tool_ptr->handle_left_click(
				oriented_click_pos_on_globe,
				is_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					click_pos_on_globe));
	}
}

void
GPlatesCanvasTools::CanvasToolAdapterForGlobe::handle_left_drag(
		const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
		bool was_on_globe,
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
{
	if (globe_canvas().isVisible())
	{
		d_canvas_tool_ptr->handle_left_drag(
				oriented_initial_pos_on_globe,
				was_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					initial_pos_on_globe),
				oriented_current_pos_on_globe,
				is_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					current_pos_on_globe),
				oriented_centre_of_viewport);
	}
}

void
GPlatesCanvasTools::CanvasToolAdapterForGlobe::handle_left_release_after_drag(
		const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
		bool was_on_globe,
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
{
	if (globe_canvas().isVisible())
	{
		d_canvas_tool_ptr->handle_left_release_after_drag(
				oriented_initial_pos_on_globe,
				was_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					initial_pos_on_globe),
				oriented_current_pos_on_globe,
				is_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					current_pos_on_globe),
				oriented_centre_of_viewport);
	}
}

void
GPlatesCanvasTools::CanvasToolAdapterForGlobe::handle_shift_left_click(
		const GPlatesMaths::PointOnSphere &click_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
		bool is_on_globe)
{
	if (globe_canvas().isVisible())
	{
		d_canvas_tool_ptr->handle_shift_left_click(
				oriented_click_pos_on_globe,
				is_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					click_pos_on_globe));
	}
}

void
GPlatesCanvasTools::CanvasToolAdapterForGlobe::handle_shift_left_drag(
		const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
		bool was_on_globe,
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
{
	if (globe_canvas().isVisible())
	{
		d_canvas_tool_ptr->handle_shift_left_drag(
				oriented_initial_pos_on_globe,
				was_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					initial_pos_on_globe),
				oriented_current_pos_on_globe,
				is_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					current_pos_on_globe),
				oriented_centre_of_viewport);
	}
}

void
GPlatesCanvasTools::CanvasToolAdapterForGlobe::handle_shift_left_release_after_drag(
		const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
		bool was_on_globe,
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
{
	if (globe_canvas().isVisible())
	{
		d_canvas_tool_ptr->handle_shift_left_release_after_drag(
				oriented_initial_pos_on_globe,
				was_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					initial_pos_on_globe),
				oriented_current_pos_on_globe,
				is_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					current_pos_on_globe),
				oriented_centre_of_viewport);
	}
}

void
GPlatesCanvasTools::CanvasToolAdapterForGlobe::handle_ctrl_left_click(
		const GPlatesMaths::PointOnSphere &click_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
		bool is_on_globe)
{
	if (globe_canvas().isVisible())
	{
		d_canvas_tool_ptr->handle_ctrl_left_click(
				oriented_click_pos_on_globe,
				is_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					click_pos_on_globe));
	}
}

void
GPlatesCanvasTools::CanvasToolAdapterForGlobe::handle_ctrl_left_drag(
		const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
		bool was_on_globe,
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
{
	if (globe_canvas().isVisible())
	{
		if (d_canvas_tool_ptr->handle_ctrl_left_drag(
				oriented_initial_pos_on_globe,
				was_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					initial_pos_on_globe),
				oriented_current_pos_on_globe,
				is_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					current_pos_on_globe),
				oriented_centre_of_viewport))
		{
			// perform default action
			GlobeCanvasTool::handle_ctrl_left_drag(
					initial_pos_on_globe,
					oriented_initial_pos_on_globe,
					was_on_globe,
					current_pos_on_globe,
					oriented_current_pos_on_globe,
					is_on_globe,
					oriented_centre_of_viewport);
		}
	}
}

void
GPlatesCanvasTools::CanvasToolAdapterForGlobe::handle_ctrl_left_release_after_drag(
		const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
		bool was_on_globe,
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
{
	if (globe_canvas().isVisible())
	{
		if (d_canvas_tool_ptr->handle_ctrl_left_release_after_drag(
				oriented_initial_pos_on_globe,
				was_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					initial_pos_on_globe),
				oriented_current_pos_on_globe,
				is_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					current_pos_on_globe),
				oriented_centre_of_viewport))
		{
			// perform default action
			GlobeCanvasTool::handle_ctrl_left_release_after_drag(
					initial_pos_on_globe,
					oriented_initial_pos_on_globe,
					was_on_globe,
					current_pos_on_globe,
					oriented_current_pos_on_globe,
					is_on_globe,
					oriented_centre_of_viewport);
		}
	}
}

void
GPlatesCanvasTools::CanvasToolAdapterForGlobe::handle_shift_ctrl_left_click(
		const GPlatesMaths::PointOnSphere &click_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
		bool is_on_globe)
{
	if (globe_canvas().isVisible())
	{
		d_canvas_tool_ptr->handle_shift_ctrl_left_click(
				oriented_click_pos_on_globe,
				is_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					click_pos_on_globe));
	}
}

void
GPlatesCanvasTools::CanvasToolAdapterForGlobe::handle_shift_ctrl_left_drag(
		const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
		bool was_on_globe,
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
{
	if (globe_canvas().isVisible())
	{
		if (d_canvas_tool_ptr->handle_shift_ctrl_left_drag(
				oriented_initial_pos_on_globe,
				was_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					initial_pos_on_globe),
				oriented_current_pos_on_globe,
				is_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					current_pos_on_globe),
				oriented_centre_of_viewport))
		{
			// perform default action
			GlobeCanvasTool::handle_shift_ctrl_left_drag(
					initial_pos_on_globe,
					oriented_initial_pos_on_globe,
					was_on_globe,
					current_pos_on_globe,
					oriented_current_pos_on_globe,
					is_on_globe,
					oriented_centre_of_viewport);
		}
	}
}

void
GPlatesCanvasTools::CanvasToolAdapterForGlobe::handle_shift_ctrl_left_release_after_drag(
		const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
		bool was_on_globe,
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
{
	if (globe_canvas().isVisible())
	{
		if (d_canvas_tool_ptr->handle_shift_ctrl_left_release_after_drag(
				oriented_initial_pos_on_globe,
				was_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					initial_pos_on_globe),
				oriented_current_pos_on_globe,
				is_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					current_pos_on_globe),
				oriented_centre_of_viewport))
		{
			// perform default action
			GlobeCanvasTool::handle_shift_ctrl_left_release_after_drag(
					initial_pos_on_globe,
					oriented_initial_pos_on_globe,
					was_on_globe,
					current_pos_on_globe,
					oriented_current_pos_on_globe,
					is_on_globe,
					oriented_centre_of_viewport);
		}
	}
}

void
GPlatesCanvasTools::CanvasToolAdapterForGlobe::handle_move_without_drag(
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
{
	if (globe_canvas().isVisible())
	{
		d_canvas_tool_ptr->handle_move_without_drag(
				oriented_current_pos_on_globe,
				is_on_globe,
				globe_canvas().current_proximity_inclusion_threshold(
					current_pos_on_globe));
	}
}

