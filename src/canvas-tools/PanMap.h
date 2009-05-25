/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C)  2008 Geological Survey of Norway
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

#ifndef GPLATES_CANVASTOOLS_PANMAP_H
#define GPLATES_CANVASTOOLS_PANMAP_H

#include "gui/MapCanvasTool.h"


namespace GPlatesQtWidgets
{
	class MapCanvas;
	class MapView;
	class ViewportWindow;
}

namespace GPlatesCanvasTools
{
	/**
	 * This is the canvas tool used to re-orient the globe by dragging.
	 */
	class PanMap:
			public GPlatesGui::MapCanvasTool
	{
	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<ReorientGlobe,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<PanMap,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		virtual
		~PanMap()
		{  }

		/**
		 * Create a PanMap instance.
		 */
		static
		const non_null_ptr_type
		create(
				GPlatesQtWidgets::MapCanvas &map_canvas_,
				GPlatesQtWidgets::MapView &map_view_,
				const GPlatesQtWidgets::ViewportWindow &view_state_)
		{
			PanMap::non_null_ptr_type ptr(
					new PanMap(map_canvas_, map_view_, view_state_),
					GPlatesUtils::NullIntrusivePointerHandler());
			return ptr;
		}

		virtual
		void
		handle_activation();

		virtual
		void
		handle_deactivation();

		
		virtual
		void
		handle_left_click(
				const QPointF &point_on_scene,
				bool is_on_surface);


		virtual
		void
		handle_left_drag(
				const QPointF &initial_point_on_scene,
				bool was_on_surface,
				const QPointF &current_point_on_scene,
				bool is_on_surface,
				const QPointF &translation
		);

		virtual
		void
		handle_shift_left_click(
				const QPointF &point_on_scene,
				bool is_on_surface);	


		virtual
		void
		handle_shift_left_drag(
				const QPointF &initial_point_on_scene,
				bool was_on_surface,
				const QPointF &current_point_on_scene,
				bool is_on_surface,
				const QPointF &translation);

		virtual
		void
		handle_shift_left_release_after_drag(
				const QPointF &initial_point_on_scene,
				bool was_on_surface,
				const QPointF &current_point_on_scene,
				bool is_on_surface);


	protected:
		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		PanMap(
				GPlatesQtWidgets::MapCanvas &map_canvas_,
				GPlatesQtWidgets::MapView &map_view_,
				const GPlatesQtWidgets::ViewportWindow &view_state_):
			MapCanvasTool(map_canvas_, map_view_),
			d_view_state_ptr(&view_state_)
		{  }

	private:

		/**
		 * This is the View State used to pass messages to the status bar.
		 */
		const GPlatesQtWidgets::ViewportWindow *d_view_state_ptr;

		// This constructor should never be defined, because we don't want/need to allow
		// copy-construction.
		PanMap(
				const PanMap &);

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment.
		PanMap &
		operator=(
				const PanMap &);
	};
}

#endif  // GPLATES_CANVASTOOLS_PANMAP_H
