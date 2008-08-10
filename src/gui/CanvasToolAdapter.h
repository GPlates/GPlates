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

#ifndef GPLATES_GUI_CANVASTOOLADAPTER_H
#define GPLATES_GUI_CANVASTOOLADAPTER_H

#include <QObject>
#include <Qt>


namespace GPlatesMaths
{
	class PointOnSphere;
}

namespace GPlatesGui
{
	class CanvasTool;
	class CanvasToolChoice;


	/**
	 * This class adapts the interface of CanvasTool to the interface expected by the
	 * mouse-click and mouse-drag signals of GlobeCanvas.
	 *
	 * This class provides slots to be connected to the signals emitted by GlobeCanvas.  It
	 * invokes the appropriate handler function of the current choice of CanvasTool.
	 */
	class CanvasToolAdapter:
			public QObject
	{
		Q_OBJECT

	public:
		/**
		 * Construct a CanvasToolAdapter instance.
		 */
		CanvasToolAdapter(
				const CanvasToolChoice &canvas_tool_choice_):
			d_canvas_tool_choice_ptr(&canvas_tool_choice_)
		{  }

		~CanvasToolAdapter()
		{  }

		const CanvasToolChoice &
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
				Qt::MouseButton button,
				Qt::KeyboardModifiers modifiers);

	private:
		const CanvasToolChoice *d_canvas_tool_choice_ptr;

		// This constructor should never be defined, because we don't want/need to allow
		// copy-construction.
		CanvasToolAdapter(
				const CanvasToolAdapter &);

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment.
		CanvasToolAdapter &
		operator=(
				const CanvasToolAdapter &);
	};
}

#endif  // GPLATES_GUI_CANVASTOOLADAPTER_H
