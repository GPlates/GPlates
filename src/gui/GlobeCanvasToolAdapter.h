/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
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

#ifndef GPLATES_GUI_GLOBECANVASTOOLADAPTER_H
#define GPLATES_GUI_GLOBECANVASTOOLADAPTER_H

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <QObject>
#include <Qt>


namespace GPlatesMaths
{
	class PointOnSphere;
	class LatLonPoint;
}

namespace GPlatesQtWidgets
{
	class GlobeCanvas;
}

namespace GPlatesGui
{
	class GlobeCanvasTool;

	/**
	 * This class adapts the interface of GlobeCanvasTool to the interface expected by the mouse-click
	 * and mouse-drag signals of GlobeCanvas and directs them to the activate canvas tool.
	 */
	class GlobeCanvasToolAdapter:
			public QObject,
			private boost::noncopyable
	{
		Q_OBJECT

	public:
		/**
		 * Construct a GlobeCanvasToolAdapter instance.
		 */
		explicit
		GlobeCanvasToolAdapter(
				GPlatesQtWidgets::GlobeCanvas &globe_canvas);

		~GlobeCanvasToolAdapter()
		{  }

		/**
		 * Connects mouse signals from @a GlobeCanvas to the specified canvas tool.
		 */
		void
		activate_canvas_tool(
				GlobeCanvasTool &globe_canvas_tool);

		/**
		 * Disconnects mouse signals from @a GlobeCanvas to the currently active canvas tool.
		 */
		void
		deactivate_canvas_tool();

	private slots:
	
		void
		handle_press(
				const GPlatesMaths::PointOnSphere &press_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_press_pos_on_globe,
				bool is_on_globe,
				Qt::MouseButton button,
				Qt::KeyboardModifiers modifiers);	
	
		void
		handle_click(
				const GPlatesMaths::PointOnSphere &click_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
				bool is_on_globe,
				Qt::MouseButton button,
				Qt::KeyboardModifiers modifiers);

		void
		handle_drag(
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				bool was_on_globe,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport,
				Qt::MouseButton button,
				Qt::KeyboardModifiers modifiers);

		void
		handle_release_after_drag(
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				bool was_on_globe,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport,
				Qt::MouseButton button,
				Qt::KeyboardModifiers modifiers);

		/**
		 * The mouse position moved but the left mouse button is NOT down.
		 */
		void
		handle_move_without_drag(
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport);

	private:

		GPlatesQtWidgets::GlobeCanvas &d_globe_canvas;

		boost::optional<GlobeCanvasTool &> d_active_globe_canvas_tool;


		//! Connects to mouse signals from the globe canvas.
		void
		connect_to_globe_canvas();

		//! Disconnects from mouse signals from the globe canvas.
		void
		disconnect_from_globe_canvas();

		GlobeCanvasTool &
		get_active_globe_canvas_tool();
	};
}

#endif  // GPLATES_GUI_GLOBECANVASTOOLADAPTER_H
