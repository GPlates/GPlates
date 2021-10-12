/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C)  2008, 2009 Geological Survey of Norway
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

#ifndef GPLATES_GUI_MAPCANVASTOOL_H
#define GPLATES_GUI_MAPCANVASTOOL_H

#include <QDebug>

#include "qt-widgets/MapView.h"
#include "utils/non_null_intrusive_ptr.h"
#include "utils/NullIntrusivePointerHandler.h"
#include "utils/ReferenceCount.h"


class QPointF;

namespace GPlatesQtWidgets
{
	class MapCanvas;
	class MapView;
}

namespace GPlatesGui
{

	/**
	 * This class is the abstract base of all map canvas tools.
	 *
	 * This serves the role of the abstract State class in the State Pattern in Gamma et al.
	 *
	 * The currently-activated MapCanvasTool is referenced by an instance of MapCanvasToolChoice.
	 */
	class MapCanvasTool :
			public GPlatesUtils::ReferenceCount<MapCanvasTool>
	{
	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<CanvasTool,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<MapCanvasTool,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		/**
		 * Construct a MapCanvasTool instance.
		 *
		 * Since this class is an abstract class, this constructor can never be invoked
		 * other than explicitly in the initialiser lists of derived classes.
		 * Nevertheless, the initialiser lists of derived classes @em do need to invoke it
		 * explicitly, since this class contains members which need to be initialised.
		 */
		explicit
		MapCanvasTool(
			GPlatesQtWidgets::MapCanvas &map_canvas_,
			GPlatesQtWidgets::MapView &map_view_):
			d_map_view_ptr(&map_view_),
			d_map_canvas_ptr(&map_canvas_)

		{  }

		virtual
		~MapCanvasTool() = 0;

		/**
		 * Handle the activation (selection) of this tool.
		 *
		 * This function is a no-op implementation which may be overridden in a derived
		 * class.
		 */
		virtual
		void
		handle_activation()
		{  }

		/**
		 * Handle the deactivation of this tool (a different tool has been selected).
		 *
		 * This function is a no-op implementation which may be overridden in a derived
		 * class.
		 */
		virtual
		void
		handle_deactivation()
		{  }

		/**
		 * Handle a left mouse-button click.
		 *
		 * @a click_point_on_scene is the QPointF containing coordinates of the click point
		 * in the QGraphicsScene. 
		 *
		 * @a is_on_surface is true if the click point is on the surface of the map. 
		 *
		 * This function is a no-op implementation which may be overridden in a derived
		 * class.
		 */
		virtual
		void
		handle_left_click(
				const QPointF &click_point_on_scene,
				bool is_on_surface)
		{  
	
		}

		/**
		 * Handle a mouse drag with the left mouse-button pressed.
		 *
		 *
		 * This function is a no-op implementation which may be overridden in a derived
		 * class.
		 */
		virtual
		void
		handle_left_drag(
				const QPointF &initial_point_on_scene,
				bool was_on_surface,
				const QPointF &current_point_on_scene,
				bool is_on_surface,
				const QPointF &translation)
		{  }

		/**
		 * Handle the release of the left-mouse button after a mouse drag.
		 *
		 *
		 * This function should be invoked in response to the final mouse-pointer position
		 * update (when the mouse-button has just been released).  In response to
		 * intermediate updates of the mouse-pointer position (as the mouse-pointer is
		 * moved about with the mouse-button pressed), invoke the function @a
		 * handle_left_drag instead.
		 *
		 * This function is a no-op implementation which may be overridden in a derived
		 * class.
		 */
		virtual
		void
		handle_left_release_after_drag(
				const QPointF &initial_point_on_scene,
				bool was_on_surface,
				const QPointF &current_point_on_scene,
				bool is_on_surface,
				const QPointF &translation)
		{  }



		/**
		 * Handle a left mouse-button click while a Shift key is held.
		 *
		 * This function is a no-op implementation which may be overridden in a derived
		 * class.
		 */
		virtual
		void
		handle_shift_left_click(
				const QPointF &click_point_on_scene,
				bool is_on_surface)
		{  }

		/**
		 * Handle a mouse drag with the left mouse-button pressed while a Shift key is
		 * held.
		 *
		 * This function should be invoked in response to intermediate updates of the
		 * mouse-pointer position (as the mouse-pointer is moved about with the
		 * mouse-button pressed).  In response to the final update (when the mouse-button
		 * has just been released), invoke the function @a left_release_after_drag instead.
		 *
		 * This function is a no-op implementation which may be overridden in a derived
		 * class.
		 */
		virtual
		void
		handle_shift_left_drag(
				const QPointF &initial_point_on_scene,
				bool was_on_surface,
				const QPointF &current_point_on_scene,
				bool is_on_surface,
				const QPointF &translation)
		{  }

		/**
		 * Handle the release of the left-mouse button after a mouse drag while a Shift
		 * key is held.
		 *
		 * This function should be invoked in response to the final mouse-pointer position
		 * update (when the mouse-button has just been released).  In response to
		 * intermediate updates of the mouse-pointer position (as the mouse-pointer is
		 * moved about with the mouse-button pressed), invoke the function @a
		 * handle_left_drag instead.
		 *
		 * This function is a no-op implementation which may be overridden in a derived
		 * class.
		 */
		virtual
		void
		handle_shift_left_release_after_drag(
				const QPointF &initial_point_on_scene,
				bool was_on_surface,
				const QPointF &current_point_on_scene,
				bool is_on_surface)
		{  }

		/**
		 * Handle a left mouse-button click while a Control key is held.
		 *
		 * The default implementation which may be overridden in a derived
		 * class.
		 */
		virtual
		void
		handle_ctrl_left_click(
				const QPointF &click_point_on_scene,
				bool is_on_surface)
		{  

		}

		/**
		 * Handle a mouse drag with the left mouse-button pressed while a Control key is
		 * held.
		 *
		 * This function should be invoked in response to intermediate updates of the
		 * mouse-pointer position (as the mouse-pointer is moved about with the
		 * mouse-button pressed).  In response to the final update (when the mouse-button
		 * has just been released), invoke the function @a left_release_after_drag instead.
		 *
		 * The default implementation of this function pans the map.  This
		 * implementation may be overridden in a derived class.
		 */
		virtual
		void
		handle_ctrl_left_drag(
				const QPointF &initial_point_on_scene,
				bool was_on_surface,
				const QPointF &current_point_on_scene,
				bool is_on_surface,
				const QPointF &translation)
		{
			map_view().setTransformationAnchor(QGraphicsView::NoAnchor);
			map_view().translate(translation.x(),translation.y());
		}

		/**
		 * Handle the release of the left-mouse button after a mouse drag while a Control
		 * key is held.
		 *
		 * This function should be invoked in response to the final mouse-pointer position
		 * update (when the mouse-button has just been released).  In response to
		 * intermediate updates of the mouse-pointer position (as the mouse-pointer is
		 * moved about with the mouse-button pressed), invoke the function @a
		 * handle_left_drag instead.
		 *
		 * The default implementation of this function switches off panning of the map.  This
		 * implementation may be overridden in a derived class.
		 */
		virtual
		void
		handle_ctrl_left_release_after_drag(
				const QPointF &initial_point_on_scene,
				bool was_on_surface,
				const QPointF &current_point_on_scene,
				bool is_on_surface)
		{

		}

		/**
		 * Handle a left mouse-button click while a Shift key and a Control key are held.
		 *
		 * This function is a no-op implementation which may be overridden in a derived
		 * class.
		 */
		virtual
		void
		handle_shift_ctrl_left_click(
				const QPointF &click_point_on_scene,
				bool is_on_surface)
		{  }

		/**
		 * Handle a mouse drag with the left mouse-button pressed while a Shift key and a
		 * Control key are held.
		 *
		 * This function should be invoked in response to intermediate updates of the
		 * mouse-pointer position (as the mouse-pointer is moved about with the
		 * mouse-button pressed).  In response to the final update (when the mouse-button
		 * has just been released), invoke the function @a left_release_after_drag instead.
		 *
		 * The default implementation of this function pans the map.  This
		 * implementation may be overridden in a derived class.
		 */
		virtual
		void
		handle_shift_ctrl_left_drag(
				const QPointF &initial_point_on_scene,
				bool was_on_surface,
				const QPointF &current_point_on_scene,
				bool is_on_surface,
				const QPointF &translation)
		{
			rotate_map_by_drag(initial_point_on_scene,
					was_on_surface,
					current_point_on_scene,
					is_on_surface,
					translation);
		}
#if 0
		/**
		 * Handle the release of the left-mouse button after a mouse drag while a Shift key
		 * and Control key are held.
		 *
		 * This function should be invoked in response to the final mouse-pointer position
		 * update (when the mouse-button has just been released).  In response to
		 * intermediate updates of the mouse-pointer position (as the mouse-pointer is
		 * moved about with the mouse-button pressed), invoke the function @a
		 * handle_left_drag instead.
		 *
		 * The default implementation of this function rotates the globe.  This
		 * implementation may be overridden in a derived class.
		 */
		virtual
		void
		handle_shift_ctrl_left_release_after_drag(
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				bool was_on_globe,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
		{
			rotate_globe_by_drag_release(initial_pos_on_globe,
					oriented_initial_pos_on_globe, was_on_globe,
					current_pos_on_globe,
					oriented_current_pos_on_globe, is_on_globe,
					oriented_centre_of_viewport);
		}

#endif
		/**
		* Handle a mouse movement when left mouse-button is NOT down.
		*
		* This function should be invoked in response to intermediate updates of the
		* mouse-pointer position (as the mouse-pointer is moved about).
		*
		* This function is a no-op implementation which may be overridden in a derived
		* class.
		*/
		virtual
		void
		handle_move_without_drag(
				const QPointF &current_point_on_scene,
				bool is_on_surface,
				const QPointF &translation)
		{  }

	protected:
		GPlatesQtWidgets::MapView &
		map_view() const
		{
			return *d_map_view_ptr;
		}

		GPlatesQtWidgets::MapCanvas &
		map_canvas() const
		{
			return *d_map_canvas_ptr;
		}

		/**
		 * Re-orient the map by dragging the mouse pointer.
		 *
		 * This function is used by the default implementation of the Ctrl + left-mouse
		 * button drag handler.
		 */
		void
		rotate_map_by_drag(
				const QPointF &initial_point_on_scene,
				bool was_on_surface,
				const QPointF &current_point_on_scene,
				bool is_on_surface,
				const QPointF &translation);

#if 0
		/**
		 * Re-orient the globe by dragging the mouse pointer.
		 *
		 * This function is used by the default implementation of the Ctrl + left-mouse
		 * button drag handler.
		 */
		void
		reorient_globe_by_drag_update(
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				bool was_on_globe,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport);

		/**
		 * Re-orient the globe by dragging the mouse pointer.
		 *
		 * This function is used by the default implementation of the Ctrl + left-mouse
		 * button drag handler.
		 */
		void
		reorient_globe_by_drag_release(
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				bool was_on_globe,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport);

		/**
		 * Rotate the globe around the centre of the viewport by dragging the mouse
		 * pointer.
		 *
		 * This function is used by the default implementation of the Ctrl + Shift +
		 * left-mouse button drag handler.
		 */
		void
		rotate_globe_by_drag_update(
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				bool was_on_globe,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport);

		/**
		 * Rotate the map around the centre of the viewport by dragging the mouse
		 * pointer.
		 *
		 * This function is used by the default implementation of the Ctrl + Shift +
		 * left-mouse button drag handler.
		 */
		void
		rotate_map_by_drag_release(
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				bool was_on_globe,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport);
#endif

	private:

		/**
		 * The map view
		 */
		GPlatesQtWidgets::MapView *d_map_view_ptr;

		/**
		 * The map canvas.
		 */
		GPlatesQtWidgets::MapCanvas *d_map_canvas_ptr;
	};
}

#endif  // GPLATES_GUI_MAPCANVASTOOL_H
