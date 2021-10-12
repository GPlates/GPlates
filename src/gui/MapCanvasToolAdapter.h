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

#include <QObject>

class QPointF;

namespace GPlatesGui
{
	class MapCanvasTool;
	class MapCanvasToolChoice;


	/**
	 * This class adapts the interface of MapCanvasTool to the interface expected by the
	 * mouse-click and mouse-drag signals of MapView.
	 *
	 * This class provides slots to be connected to the signals emitted by MapView.  It
	 * invokes the appropriate handler function of the current choice of MapCanvasTool.
	 */
	class MapCanvasToolAdapter:
			public QObject
	{
		Q_OBJECT

	public:
		/**
		 * Construct a MapCanvasToolAdapter instance.
		 */
		MapCanvasToolAdapter(
				const MapCanvasToolChoice &map_canvas_tool_choice_):
			d_map_canvas_tool_choice_ptr(&map_canvas_tool_choice_)
		{  }

		~MapCanvasToolAdapter()
		{  }

		const MapCanvasToolChoice &
		map_canvas_tool_choice() const
		{
			return *d_map_canvas_tool_choice_ptr;
		}

	public slots:
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
		const MapCanvasToolChoice *d_map_canvas_tool_choice_ptr;

		// This constructor should never be defined, because we don't want/need to allow
		// copy-construction.
		MapCanvasToolAdapter(
				const MapCanvasToolAdapter &);

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment.
		MapCanvasToolAdapter &
		operator=(
				const MapCanvasToolAdapter &);
	};
}

#endif  // GPLATES_GUI_MAPCANVASTOOLADAPTER_H
