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
#include <QObject>
#include <Qt>

#include <boost/optional.hpp>

namespace GPlatesMaths
{
	class PointOnSphere;
	class LatLonPoint;
}

namespace GPlatesViewOperations
{
	class RenderedGeometryCollection;
}

namespace GPlatesGui
{
	class GlobeCanvasTool;
	class GlobeCanvasToolChoice;


	/**
	 * This class adapts the interface of GlobeCanvasTool to the interface expected by the
	 * mouse-click and mouse-drag signals of GlobeCanvas.
	 *
	 * This class provides slots to be connected to the signals emitted by GlobeCanvas.  It
	 * invokes the appropriate handler function of the current choice of GlobeCanvasTool.
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
		GlobeCanvasToolAdapter(
				const GlobeCanvasToolChoice &canvas_tool_choice_):
		d_canvas_tool_choice_ptr(&canvas_tool_choice_)
		{  }

		~GlobeCanvasToolAdapter()
		{  }

		const GlobeCanvasToolChoice &
		canvas_tool_choice() const
		{
			return *d_canvas_tool_choice_ptr;
		}

	public slots:
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
		const GlobeCanvasToolChoice *d_canvas_tool_choice_ptr;
	};
}

#endif  // GPLATES_GUI_GLOBECANVASTOOLADAPTER_H
