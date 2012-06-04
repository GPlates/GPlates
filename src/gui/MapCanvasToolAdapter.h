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


class QPointF;

namespace GPlatesQtWidgets
{
	class MapView;
}

namespace GPlatesGui
{
	class MapCanvasTool;

	/**
	 * This class adapts the interface of MapCanvasTool to the interface expected by the mouse-click
	 * and mouse-drag signals of MapView and directs them to the activate canvas tool.
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
				GPlatesQtWidgets::MapView &map_view);

		~MapCanvasToolAdapter()
		{  }

		/**
		 * Connects mouse signals from @a MapView to the specified canvas tool.
		 */
		void
		activate_canvas_tool(
				MapCanvasTool &map_canvas_tool);

		/**
		 * Disconnects mouse signals from @a MapView to the currently active canvas tool.
		 */
		void
		deactivate_canvas_tool();

	private slots:
	
		void
		handle_press(
				const QPointF &clicked_point_on_scene,
				bool is_on_surface,
				Qt::MouseButton button,
				Qt::KeyboardModifiers modifiers);
	
	
		void
		handle_click(
				const QPointF &clicked_point_on_scene,
				bool is_on_surface,
				Qt::MouseButton button,
				Qt::KeyboardModifiers modifiers);

		void
		handle_drag(
				const QPointF &initial_point_on_scene,
				bool was_on_surface,
				const QPointF &current_point_on_scene,
				bool is_on_surface,
				Qt::MouseButton button,
				Qt::KeyboardModifiers modifiers,
				const QPointF &translation);

		void
		handle_release_after_drag(
				const QPointF &initial_point_on_scene,
				bool was_on_surface,
				const QPointF &current_point_on_scene,
				bool is_on_surface,
				const QPointF &translation,
				Qt::MouseButton button,
				Qt::KeyboardModifiers modifiers);

		/**
		* The mouse position moved but the left mouse button is NOT down.
		*/
		void
		handle_move_without_drag(
				const QPointF &current_point_on_scene,
				bool is_on_surface,
				const QPointF &translation);

	private:

		GPlatesQtWidgets::MapView &d_map_view;

		boost::optional<MapCanvasTool &> d_active_map_canvas_tool;


		//! Connects to mouse signals from the map view.
		void
		connect_to_map_view();

		//! Disconnects from mouse signals from the map view.
		void
		disconnect_from_map_view();

		MapCanvasTool &
		get_active_map_canvas_tool();
	};
}

#endif  // GPLATES_GUI_MAPCANVASTOOLADAPTER_H
