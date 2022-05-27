/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2009 Geological Survey of Norway
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

#ifndef GPLATES_GUI_MAPCANVASTOOLADAPTER_H
#define GPLATES_GUI_MAPCANVASTOOLADAPTER_H

#include <boost/optional.hpp>
#include <QObject>
#include <QPointF>

#include "maths/PointOnSphere.h"


namespace GPlatesQtWidgets
{
	class MapCanvas;
}

namespace GPlatesGui
{
	class MapCanvasTool;

	/**
	 * This class adapts the interface of MapCanvasTool to the interface expected by the mouse-click
	 * and mouse-drag signals of MapCanvas and directs them to the activate canvas tool.
	 */
	class MapCanvasToolAdapter:
			public QObject
	{
		Q_OBJECT

	public:
		/**
		 * Construct a MapCanvasToolAdapter instance.
		 */
		explicit
		MapCanvasToolAdapter(
				GPlatesQtWidgets::MapCanvas &map_canvas);

		~MapCanvasToolAdapter()
		{  }

		/**
		 * Connects mouse signals from @a MapCanvas to the specified canvas tool.
		 */
		void
		activate_canvas_tool(
				MapCanvasTool &map_canvas_tool);

		/**
		 * Disconnects mouse signals from @a MapCanvas to the currently active canvas tool.
		 */
		void
		deactivate_canvas_tool();

	private Q_SLOTS:
	
		void
		handle_press(
				int screen_width,
				int screen_height,
				const QPointF &press_screen_position,
				const boost::optional<QPointF> &press_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &press_position_on_globe,
				Qt::MouseButton button,
				Qt::KeyboardModifiers modifiers);
	
	
		void
		handle_click(
				int screen_width,
				int screen_height,
				const QPointF &click_screen_position,
				const boost::optional<QPointF> &click_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &click_position_on_globe,
				Qt::MouseButton button,
				Qt::KeyboardModifiers modifiers);

		void
		handle_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const boost::optional<QPointF> &initial_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &initial_position_on_globe,
				const QPointF &current_screen_position,
				const boost::optional<QPointF> &current_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &current_position_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe,
				Qt::MouseButton button,
				Qt::KeyboardModifiers modifiers);

		void
		handle_release_after_drag(
				int screen_width,
				int screen_height,
				const QPointF &initial_screen_position,
				const boost::optional<QPointF> &initial_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &initial_position_on_globe,
				const QPointF &current_screen_position,
				const boost::optional<QPointF> &current_map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &current_position_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe,
				Qt::MouseButton button,
				Qt::KeyboardModifiers modifiers);

		/**
		* The mouse position moved but the left mouse button is NOT down.
		*/
		void
		handle_move_without_drag(
				int screen_width,
				int screen_height,
				const QPointF &screen_position,
				const boost::optional<QPointF> &map_position,
				const boost::optional<GPlatesMaths::PointOnSphere> &position_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport_on_globe);

	private:

		GPlatesQtWidgets::MapCanvas &d_map_canvas;

		boost::optional<MapCanvasTool &> d_active_map_canvas_tool;


		//! Connects to mouse signals from the map canvas.
		void
		connect_to_map_canvas();

		//! Disconnects from mouse signals from the map canvas.
		void
		disconnect_from_map_canvas();

		MapCanvasTool &
		get_active_map_canvas_tool();
	};
}

#endif  // GPLATES_GUI_MAPCANVASTOOLADAPTER_H
