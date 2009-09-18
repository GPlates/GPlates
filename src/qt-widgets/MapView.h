/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009 Geological Survey of Norway.
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
 
#ifndef GPLATES_QTWIDGETS_MAPVIEW_H
#define GPLATES_QTWIDGETS_MAPVIEW_H

#include <QGraphicsView>
#include <QGLWidget>

#include <boost/optional.hpp>

#include "gui/ViewportZoom.h"
#include "maths/LatLonPointConversions.h"
#include "qt-widgets/SceneView.h"
#include "view-operations/QueryProximityThreshold.h"
#include "view-operations/ViewState.h"

namespace GPlatesQtWidgets
{

	class MapCanvas;
	class ViewportWindow;

	class MapView: 
		public QGraphicsView, 
		public GPlatesViewOperations::QueryProximityThreshold,
		public SceneView
	{
	Q_OBJECT
	public:

		struct MousePressInfo
		{
			MousePressInfo(
					int mouse_pointer_screen_pos_x,
					int mouse_pointer_screen_pos_y,
					const QPointF &mouse_pointer_scene_coords,
					const boost::optional<GPlatesMaths::LatLonPoint> &mouse_pointer_llp,
					bool is_on_surface,
					Qt::MouseButton button,
					Qt::KeyboardModifiers modifiers):
				d_mouse_pointer_screen_pos_x(mouse_pointer_screen_pos_x),
				d_mouse_pointer_screen_pos_y(mouse_pointer_screen_pos_y),
				d_mouse_pointer_scene_coords(mouse_pointer_scene_coords),
				d_mouse_pointer_llp(mouse_pointer_llp),
				d_is_on_surface(is_on_surface),
				d_button(button),
				d_modifiers(modifiers),
				d_is_mouse_drag(false)
			{  }

			int d_mouse_pointer_screen_pos_x;
			int d_mouse_pointer_screen_pos_y;
			QPointF d_mouse_pointer_scene_coords;
			boost::optional<GPlatesMaths::LatLonPoint> d_mouse_pointer_llp;
			bool d_is_on_surface;
			Qt::MouseButton d_button;
			Qt::KeyboardModifiers d_modifiers;
			bool d_is_mouse_drag;
		};

		MapView(
			GPlatesViewOperations::ViewState &view_state,
			QWidget *parent,
			MapCanvas *map_canvas);

		/** 
		 * Translates the view so that the LatLonPoint llp is centred on the viewport. 
		 */
		virtual
		void
		set_camera_viewpoint(
			const GPlatesMaths::LatLonPoint &llp);

		virtual
		void
		move_camera_up();

		virtual
		void
		move_camera_down();

		virtual
		void
		move_camera_left();

		virtual
		void
		move_camera_right();

		virtual
		void
		rotate_camera_clockwise();

		virtual
		void
		rotate_camera_anticlockwise();

		virtual
		void
		reset_camera_orientation();

		/**
		 * Returns the LatLonPoint at the centre of the active view, if the central point is on the surface
		 * of the earth. 
		 */
		virtual
		boost::optional<GPlatesMaths::LatLonPoint>
		camera_llp();
				
		virtual
		void
		enable_raster_display();

		virtual
		void
		disable_raster_display();

		/**
		 * Rescales the view of the scene after zoom or resize events. 
		 */
		void
		set_view();

		void
		update_mouse_pointer_pos(
			QMouseEvent *mouse_event);

		virtual
		void
		handle_mouse_pointer_pos_change();

		/**
		 * Create an svg file representing the current state of the viewport window.
		 *
		 * This function passes control to the SvgExport class, which uses the MapView's
		 * draw_svg_output to do the actual vector drawing.
		 */
		virtual
		void
		create_svg_output(
			QString filename);

		/**
		 * Draw the reconstruction geometries on the screen.
		 */
		virtual
		void
		draw_svg_output();

		MapCanvas &
		map_canvas() const;

		/**
		 * Updates the member variable d_centre_of_viewport by translating the center of the viewport
		 * to scene coordinates. 
		 */
		void
		update_centre_of_viewport();


		/**
		 * Return a pointer to the viewport zoom, which is really the GlobeCanvas' viewport zoom. 
		 */ 
		GPlatesGui::ViewportZoom &
		viewport_zoom()
		{
			return *d_viewport_zoom;
		}

		/**
		 * Redraw geometries on the canvas associated with this view.
		 */
		void
		update_canvas();

		double
		current_proximity_inclusion_threshold(
				const GPlatesMaths::PointOnSphere &click_point) const;

	public slots:

		void
		handle_zoom_change();


	protected:



		virtual 
		void 
		mouseMoveEvent(
				QMouseEvent *mouse_event);

		virtual
		void
		mousePressEvent(
				QMouseEvent *mouse_event);

		virtual 
		void 
		mouseReleaseEvent(
				QMouseEvent *mouse_event);	


		virtual
		void
		wheelEvent(
			QWheelEvent *wheel_event);


		virtual
		void
		resizeEvent(
			QResizeEvent* resize_event);

	signals:

		void
		mouse_pointer_position_changed(
				const boost::optional<GPlatesMaths::LatLonPoint> &,
				bool is_on_globe);

		void
		mouse_clicked(
				const QPointF &point_on_scene,
				bool is_on_surface,
				Qt::MouseButton button,
				Qt::KeyboardModifiers modifiers);

		void
		mouse_dragged(
				const QPointF &initial_point_on_scene,
				bool was_on_surface,
				const QPointF &current_point_on_scene,
				bool is_on_surface,
				Qt::MouseButton button,
				Qt::KeyboardModifiers modifiers,
				const QPointF &translation);

		void
		mouse_released_after_drag(
				const QPointF &initial_point_on_scene,
				bool was_on_surface,
				const QPointF &current_point_on_scene,
				bool is_on_surface,
				const QPointF &translation,
				Qt::MouseButton button,
				Qt::KeyboardModifiers modifiers);

		void
		mouse_moved_without_drag(
				const QPointF &current_point_on_scene,
				bool is_on_surface,
				const QPointF &translation);


		void
		view_changed();


	private:

		MapView(
			const MapView &);

		MapView &
		operator=(
			MapView &other);

		void
		handle_wheel_rotation(
				int delta);

		/**
		 * Returns the llp of the mouse position, if the mouse is on the surface. 
		 */ 
		boost::optional<GPlatesMaths::LatLonPoint>
		mouse_pointer_llp();

		/**
		 * Returns the scene coords of the mouse position. 
		 */ 
		QPointF
		mouse_pointer_scene_coords();

		/**
		 * Returns true if the mouse is over the surface of the earth. 
		 */ 
		bool
		mouse_pointer_is_on_surface();


		/**
		 * A pointer to the GlobeCanvas' ViewportZoom. 
		 */
		GPlatesGui::ViewportZoom *d_viewport_zoom;

		/**
		 * A pointer to the map canvas that this view is associated with. 
		 */
		MapCanvas *d_map_canvas_ptr;

		/**
		 * Whether the mouse pointer is on the surface of the earth.
		 */
		bool d_mouse_pointer_is_on_surface;

		/**
		 * The position of the mouse pointer in view coordinates.
		 */
		QPoint d_mouse_pointer_screen_pos;

		/**
		 * The last position of the mouse in view (screen) coordinates.
		 */
		QPoint d_last_mouse_view_coords;

		boost::optional<MousePressInfo> d_mouse_press_info;

		/**
		 * The coordinates (in scene coords) of the centre of the visible viewport.
		 */
		QPointF d_centre_of_viewport;

		/**
		 * The visible area of the MapView will initially be set to cover this area of the QGraphicsScene.
		 */
		QRectF d_scene_rect;

		/**
		 * The QGLWidget that we use for this widget's viewport
		 */
		QGLWidget *d_gl_widget_ptr;
	};

}


#endif // GPLATES_QTWIDGETS_MAPVIEW_H
