/* $Id$ */

/**
 * \file 
 * Interface for choosing canvas tools.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_CHOOSECANVASTOOL_H
#define GPLATES_GUI_CHOOSECANVASTOOL_H

#include <QObject>
#include <QUndoCommand>


namespace GPlatesQtWidgets
{
	class ViewportWindow;
}

namespace GPlatesGui
{
	class ChooseCanvasTool;

	/**
	 * Undo/redo command for selecting a canvas tool.
	 */
	class ChooseCanvasToolUndoCommand :
		public QUndoCommand
	{
	public:
		/**
		 * @a ChooseCanvasTool method to call.
		 */
		typedef void (ChooseCanvasTool:: *choose_canvas_tool_method_type)();

		/**
		 * @param choose_canvas_tool @a ChooseCanvasTool object on which to call method.
		 * @param choose_canvas_tool_method method to call on @a choose_canvas_tool.
		 */
		ChooseCanvasToolUndoCommand(
				GPlatesGui::ChooseCanvasTool *choose_canvas_tool,
				choose_canvas_tool_method_type choose_canvas_tool_method,
				QUndoCommand *parent = 0);

		virtual
		void
		redo();

		virtual
		void
		undo();

	private:
		GPlatesGui::ChooseCanvasTool *d_choose_canvas_tool;
		choose_canvas_tool_method_type d_choose_canvas_tool_method;
		bool d_first_redo;
	};


	/**
	 * Used for choosing a canvas tool.
	 * You can register to receive signals whenever a canvas tool is chosen.
	 */
	class ChooseCanvasTool :
		public QObject
	{
		Q_OBJECT

	public:
		/**
		 * Type of canvas tool.
		 */
		enum ToolType
		{
			NONE,

			DRAG_GLOBE,
			ZOOM_GLOBE,
			CLICK_GEOMETRY,
			DIGITISE_POLYLINE,
			DIGITISE_MULTIPOINT,
			DIGITISE_POLYGON,
			MOVE_GEOMETRY,
			MOVE_VERTEX,
			MANIPULATE_POLE,
			CREATE_TOPOLOGY
		};

		ChooseCanvasTool(
			GPlatesQtWidgets::ViewportWindow &);

		/**
		 * Choose the most recently selected of the three digitise geometry
		 * tools (polyline, multipoint and polygon).
		 */
		void
		choose_most_recent_digitise_geometry_tool();

	public slots:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		void
		choose_drag_globe_tool();

		void
		choose_zoom_globe_tool();

		void
		choose_click_geometry_tool();

		void
		choose_digitise_polyline_tool();

		void
		choose_digitise_multipoint_tool();

		void
		choose_digitise_polygon_tool();

		void
		choose_move_geometry_tool();

		void
		choose_move_vertex_tool();

		void
		choose_manipulate_pole_tool();

		void
		choose_create_topology_tool();

	signals:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		/**
		 * This signal is emitted when one the above slot methods is called.
		 */
		void
		chose_canvas_tool(
				GPlatesGui::ChooseCanvasTool &,
				GPlatesGui::ChooseCanvasTool::ToolType);

	private:
		/**
		 * Most recent tool type chosen.
		 */
		ToolType d_most_recent_tool_type;

		/**
		 * Most recent of the three digitise geometry tools chosen.
		 */
		ToolType d_most_recent_digitise_geom_tool_type;

		/**
		 * Used to do the actual choosing of canvas tool.
		 * Ultimately that functionality could be moved into this class.
		 */
		GPlatesQtWidgets::ViewportWindow *d_viewport_window;
	};
}

#endif // GPLATES_GUI_CHOOSECANVASTOOL_H
