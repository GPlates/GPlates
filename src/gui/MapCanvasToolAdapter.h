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

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <QObject>
#include <QPointF>
#include <Qt>
// Fix Qt6 metatype for boost::optional<QPointF> (used in signal/slots)...
#include <QTypeInfo_fix.h>

#include "maths/PointOnSphere.h"


namespace GPlatesQtWidgets
{
	class GlobeAndMapCanvas;
}

namespace GPlatesGui
{
	class MapCanvasTool;

	/**
	 * This class adapts the interface of MapCanvasTool to the interface expected by the mouse-click
	 * and mouse-drag signals of GlobeAndMapCanvas and directs them to the activate canvas tool.
	 */
	class MapCanvasToolAdapter:
			public QObject,
			private boost::noncopyable
	{
		Q_OBJECT

	public:
		/**
		 * Construct a MapCanvasToolAdapter instance.
		 */
		explicit
		MapCanvasToolAdapter(
				GPlatesQtWidgets::GlobeAndMapCanvas &map_canvas);

		~MapCanvasToolAdapter()
		{  }

		/**
		 * Connects mouse signals from @a GlobeAndMapCanvas to the specified canvas tool.
		 */
		void
		activate_canvas_tool(
				MapCanvasTool &map_canvas_tool);

		/**
		 * Disconnects mouse signals from @a GlobeAndMapCanvas to the currently active canvas tool.
		 */
		void
		deactivate_canvas_tool();

	private Q_SLOTS:
	
		void
		handle_press(
				int screen_width,
				int screen_height,
				QPointF press_screen_position,
				boost::optional<QPointF> press_map_position,
				GPlatesMaths::PointOnSphere press_position_on_globe,
				bool is_on_globe,
				Qt::MouseButton button,
				Qt::KeyboardModifiers modifiers);
	
	
		void
		handle_click(
				int screen_width,
				int screen_height,
				QPointF click_screen_position,
				boost::optional<QPointF> click_map_position,
				GPlatesMaths::PointOnSphere click_position_on_globe,
				bool is_on_globe,
				Qt::MouseButton button,
				Qt::KeyboardModifiers modifiers);

		void
		handle_drag(
				int screen_width,
				int screen_height,
				QPointF initial_screen_position,
				boost::optional<QPointF> initial_map_position,
				GPlatesMaths::PointOnSphere initial_position_on_globe,
				bool was_on_globe,
				QPointF current_screen_position,
				boost::optional<QPointF> current_map_position,
				GPlatesMaths::PointOnSphere current_position_on_globe,
				bool is_on_globe,
				GPlatesMaths::PointOnSphere centre_of_viewport_on_globe,
				Qt::MouseButton button,
				Qt::KeyboardModifiers modifiers);

		void
		handle_release_after_drag(
				int screen_width,
				int screen_height,
				QPointF initial_screen_position,
				boost::optional<QPointF> initial_map_position,
				GPlatesMaths::PointOnSphere initial_position_on_globe,
				bool was_on_globe,
				QPointF current_screen_position,
				boost::optional<QPointF> current_map_position,
				GPlatesMaths::PointOnSphere current_position_on_globe,
				bool is_on_globe,
				GPlatesMaths::PointOnSphere centre_of_viewport_on_globe,
				Qt::MouseButton button,
				Qt::KeyboardModifiers modifiers);

		/**
		 * The mouse position moved but the left mouse button is NOT down.
		 */
		void
		handle_move_without_drag(
				int screen_width,
				int screen_height,
				QPointF screen_position,
				boost::optional<QPointF> map_position,
				GPlatesMaths::PointOnSphere position_on_globe,
				bool is_on_globe,
				GPlatesMaths::PointOnSphere centre_of_viewport_on_globe);

	private:

		GPlatesQtWidgets::GlobeAndMapCanvas &d_map_canvas;

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
