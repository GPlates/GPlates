/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006 The University of Sydney, Australia
 *  (under the name "GLCanvas.h")
 * Copyright (C) 2006, 2007, 2010 The University of Sydney, Australia
 *  (under the name "GlobeCanvas.h")
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
 
#ifndef GPLATES_QTWIDGETS_GLOBECANVAS_H
#define GPLATES_QTWIDGETS_GLOBECANVAS_H

#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>
#include <opengl/OpenGL.h>
#include <QtOpenGL/qgl.h>

#include "gui/ColourScheme.h"
#include "gui/Globe.h"
#include "gui/PersistentOpenGLObjects.h"
#include "gui/ViewportZoom.h"

#include "maths/MultiPointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"

#include "opengl/GLClearBuffers.h"
#include "opengl/GLClearBuffersState.h"
#include "opengl/GLContext.h"
#include "opengl/GLRenderGraph.h"
#include "opengl/GLRenderTargetManager.h"
#include "opengl/GLTransform.h"
#include "opengl/GLViewport.h"

#include "qt-widgets/SceneView.h"

#include "view-operations/RenderedGeometryFactory.h"


namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesViewOperations
{
	class RenderedGeometryCollection;
}

namespace GPlatesQtWidgets 
{
	class GlobeCanvas:
			public QGLWidget,
			public SceneView
	{
		Q_OBJECT

	public:
		struct MousePressInfo
		{
			MousePressInfo(
					int mouse_pointer_screen_pos_x,
					int mouse_pointer_screen_pos_y,
					const GPlatesMaths::PointOnSphere &mouse_pointer_pos,
					bool is_on_globe,
					Qt::MouseButton button,
					Qt::KeyboardModifiers modifiers):
				d_mouse_pointer_screen_pos_x(mouse_pointer_screen_pos_x),
				d_mouse_pointer_screen_pos_y(mouse_pointer_screen_pos_y),
				d_mouse_pointer_pos(mouse_pointer_pos),
				d_is_on_globe(is_on_globe),
				d_button(button),
				d_modifiers(modifiers),
				d_is_mouse_drag(false)
			{  }

			int d_mouse_pointer_screen_pos_x;
			int d_mouse_pointer_screen_pos_y;
			GPlatesMaths::PointOnSphere d_mouse_pointer_pos;
			bool d_is_on_globe;
			Qt::MouseButton d_button;
			Qt::KeyboardModifiers d_modifiers;
			bool d_is_mouse_drag;
		};

		/**
		 * The point which corresponds to the centre of the viewport.
		 *
		 * (I'm not expecting that this will change, but I'm creating this accessor as an
		 * alternative to littering lots of equivalent PointOnSphere definitions throughout
		 * the code.
		 */
		static
		const GPlatesMaths::PointOnSphere &
		centre_of_viewport();

		explicit
		GlobeCanvas(
				GPlatesPresentation::ViewState &view_state,
				GPlatesGui::ColourScheme::non_null_ptr_type colour_scheme,
				QWidget *parent_ = 0);

	private:

		//! Private constructor for use by clone()
		explicit
		GlobeCanvas(
				GlobeCanvas *existing_globe_canvas,
				GPlatesPresentation::ViewState &view_state_,
				GPlatesMaths::PointOnSphere &virtual_mouse_pointer_pos_on_globe_,
				bool mouse_pointer_is_on_globe_,
				GPlatesGui::Globe &existing_globe_,
				GPlatesGui::ColourScheme::non_null_ptr_type colour_scheme_,
				QWidget *parent_ = 0);

		//! Common code for both constructors
		void
		init();

	public:

		GlobeCanvas *
		clone(
				GPlatesGui::ColourScheme::non_null_ptr_type colour_scheme,
				QWidget *parent_ = 0);

		/**
		 * The proximity inclusion threshold is a measure of how close a geometry must be
		 * to a click-point be considered "hit" by the click.
		 *
		 * The proximity inclusion threshold varies with the canvas size and zoom level. 
		 * It is also dependent upon the position of @a click_point on the globe (since
		 * positions at the edge of the globe in the current projection are harder to
		 * target than positions in the centre of the globe).
		 *
		 * The fundamental reason for this function derives from the observation that the
		 * mouse-pointer position on-screen is described by integer coordinates, but the
		 * geometries on the globe in the 3-D "universe" are described by floating-point
		 * coordinates which can lie "between" the universe coordinates which correspond to
		 * the discrete on-screen coordinates.
		 *
		 * Hence, even ignoring the issues of usability (ie, whether you want to make the
		 * user click on some *exact pixel* on-screen) and floating-point comparisons, we
		 * can't just convert the integer on-screen coordinates of the click-point to
		 * floating-point universe coordinates and look for hits.
		 *
		 * The approach taken by the GPlates GUI is to pick an "epsilon radius" around the
		 * click-point, then determine which geometries on the globe are "close enough" to
		 * the click-point (by determining the geometries whose shortest distance from the
		 * click-point is less than the epsilon radius).  This may be visualised as drawing
		 * a circle with radius "epsilon" around the click-point on the surface of the
		 * globe, then determining which geometries pass through the circle.
		 */
		double
		current_proximity_inclusion_threshold(
				const GPlatesMaths::PointOnSphere &click_point) const;

		GPlatesGui::Globe &
		globe()
		{
			return d_globe;
		}

		const GPlatesGui::Globe &
		globe() const
		{
			return d_globe;
		}

		/**
		 * If the mouse pointer is on the globe, return the position of the mouse pointer
		 * on the globe.
		 *
		 * Otherwise, return the closest position on the globe to the position of the
		 * mouse pointer in the 3-D "universe".
		 */
		const GPlatesMaths::PointOnSphere &
		virtual_mouse_pointer_pos_on_globe() const
		{
			return d_virtual_mouse_pointer_pos_on_globe;
		}

		/**
		 * Return whether the mouse pointer is on the globe.
		 */
		bool
		mouse_pointer_is_on_globe() const
		{
			return d_mouse_pointer_is_on_globe;
		}

		virtual
		void
		create_svg_output(
			QString filename);

		void
		repaint_canvas()
		{
			paintGL();
		}

		/**
		*   Draw the relevant objects for svg output. This is slightly different from 
		*	the usual paintGL method, because we don't want to draw the sphere 
		*   (otherwise we end up with loads of little polygons);
		*	Instead we'll use the nurbs routines to draw a circle around the circumference
		*	of the sphere (prior to the sphere orientation).
		*/
		virtual
		void
		draw_svg_output();

		virtual
		boost::optional<GPlatesMaths::LatLonPoint>
		camera_llp() const;

		virtual
		void
		set_camera_viewpoint(
			const GPlatesMaths::LatLonPoint &llp);

		QImage
		grab_frame_buffer();

	public slots:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		virtual
		void
		update_canvas();

		void
		notify_of_orientation_change();

		void
		handle_mouse_pointer_pos_change();

		void
		force_mouse_pointer_pos_change();


	protected:
		/**
		 * This is a virtual override of the function in QGLWidget.
		 *
		 * To quote the QGLWidget documentation:
		 *
		 * This virtual function is called once before the first call to paintGL() or
		 * resizeGL(), and then once whenever the widget has been assigned a new
		 * QGLContext.  Reimplement it in a subclass.
		 *
		 * This function should set up any required OpenGL context rendering flags,
		 * defining display lists, etc.
		 *
		 * There is no need to call makeCurrent() because this has already been done when
		 * this function is called.
		 */
		virtual 
		void 
		initializeGL();

		/**
		 * This is a virtual override of the function in QGLWidget.
		 *
		 * To quote the QGLWidget documentation:
		 *
		 * This virtual function is called whenever the widget has been resized.  The new
		 * size is passed in width and height.  Reimplement it in a subclass.
		 *
		 * There is no need to call makeCurrent() because this has already been done when
		 * this function is called.
		 */
		virtual
		void 
		resizeGL(
				int width, 
				int height);

		/**
		 * This is a virtual override of the function in QGLWidget.
		 *
		 * To quote the QGLWidget documentation:
		 *
		 * This virtual function is called whenever the widget needs to be painted.
		 * Reimplement it in a subclass.
		 *
		 * There is no need to call makeCurrent() because this has already been done when
		 * this function is called.
		 */
		virtual
		void
		paintGL();

		/**
		 * This is a virtual override of the function in QWidget.
		 *
		 * To quote the QWidget documentation:
		 *
		 * This event handler, for event event, can be reimplemented in a subclass to
		 * receive mouse press events for the widget.
		 *
		 * If you create new widgets in the mousePressEvent() the mouseReleaseEvent() may
		 * not end up where you expect, depending on the underlying window system (or X11
		 * window manager), the widgets' location and maybe more.
		 *
		 * The default implementation implements the closing of popup widgets when you
		 * click outside the window.  For other widget types it does nothing.
		 */
		virtual
		void
		mousePressEvent(
				QMouseEvent *event);

		/**
		 * This is a virtual override of the function in QWidget.
		 *
		 * To quote the QWidget documentation:
		 *
		 * This event handler, for event event, can be reimplemented in a subclass to
		 * receive mouse move events for the widget.
		 *
		 * If mouse tracking is switched off, mouse move events only occur if a mouse
		 * button is pressed while the mouse is being moved.  If mouse tracking is switched
		 * on, mouse move events occur even if no mouse button is pressed.
		 *
		 * QMouseEvent::pos() reports the position of the mouse cursor, relative to this
		 * widget.  For press and release events, the position is usually the same as the
		 * position of the last mouse move event, but it might be different if the user's
		 * hand shakes.  This is a feature of the underlying window system, not Qt.
		 */
		virtual 
		void 
		mouseMoveEvent(
				QMouseEvent *event);

		/**
		 * This is a virtual override of the function in QWidget.
		 *
		 * To quote the QWidget documentation:
		 *
		 * This event handler, for event event, can be reimplemented in a subclass to
		 * receive mouse release events for the widget.
		 */
		virtual 
		void 
		mouseReleaseEvent(
				QMouseEvent *event);

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

#if 0
		virtual
		void
		paintEvent(
			QPaintEvent *paint_event);
#endif

	signals:

		void
		mouse_pointer_position_changed(
				const GPlatesMaths::PointOnSphere &new_virtual_pos,
				bool is_on_globe);
				
				
		void
		mouse_pressed(
			const GPlatesMaths::PointOnSphere &press_pos_on_globe,
			const GPlatesMaths::PointOnSphere &oriented_press_pos_on_globe,
			bool is_on_globe,
			Qt::MouseButton button,
			Qt::KeyboardModifiers modifiers);

		void
		mouse_clicked(
				const GPlatesMaths::PointOnSphere &click_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
				bool is_on_globe,
				Qt::MouseButton button,
				Qt::KeyboardModifiers modifiers);

		void
		mouse_dragged(
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
		mouse_released_after_drag(
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
		mouse_moved_without_drag(
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport);

		void
		repainted(
				bool mouse_down);

	public slots:

		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		void
		handle_zoom_change();

	private:

		GPlatesPresentation::ViewState &d_view_state;

		/**
		 * Shadows some OpenGL state to allow faster OpenGL queries and
		 * to minimise state changes.
		 */
		GPlatesOpenGL::GLContext::non_null_ptr_type d_gl_context;

		/**
		 * Manages render targets and creates them as needed.
		 */
		GPlatesOpenGL::GLRenderTargetManager::non_null_ptr_type d_gl_render_target_manager;

		//! The OpenGL frame buffer clear values (colour, depth, etc).
		GPlatesOpenGL::GLClearBuffersState::non_null_ptr_type d_gl_clear_buffers_state;

		//! An OpenGL drawable to clear the frame buffers (colour, depth, etc).
		GPlatesOpenGL::GLClearBuffers::non_null_ptr_type d_gl_clear_buffers;

		//! The OpenGL viewport used to render the main scene into this canvas.
		GPlatesOpenGL::GLViewport d_gl_viewport;

		//! The current projection transform for regular OpenGL rendering.
		GPlatesOpenGL::GLTransform::non_null_ptr_type d_gl_projection_transform;

		/**
		 * The current projection transform for OpenGL rendering for SVG output.
		 *
		 * This is different than the regular OpenGL rendering transform because the
		 * far clip plane differs (because SVG output ignores depth buffering - it uses
		 * the OpenGL feedback mechanism which bypasses rasterisation - and hence the
		 * opaque disc through the centre of the globe does not occlude vector geometry
		 * on the back side of the globe and hence is visible in the SVG output).
		 * The solution is to set the far clip plane to pass through the globe centre
		 * (effectively doing the equivalent of the opaque disc but in the transformation
		 * pipeline instead of the rasterisation pipeline).
		 */
		GPlatesOpenGL::GLTransform::non_null_ptr_type d_gl_projection_transform_svg;

		//! Keeps track of OpenGL objects that persist from one render to another.
		GPlatesGui::PersistentOpenGLObjects::non_null_ptr_type d_gl_persistent_objects;


		/**
		 * If the mouse pointer is on the globe, this is the position of the mouse pointer
		 * on the globe.
		 *
		 * Otherwise, this is the closest position on the globe to the position of the
		 * mouse pointer in the 3-D "universe".
		 */
		GPlatesMaths::PointOnSphere d_virtual_mouse_pointer_pos_on_globe;

		//! Whether the mouse pointer is on the globe.
		bool d_mouse_pointer_is_on_globe;

		//! The x-coord of the mouse pointer position on the screen.
		int d_mouse_pointer_screen_pos_x;

		//! The y-coord of the mouse pointer position on the screen.
		int d_mouse_pointer_screen_pos_y;

		//! The width of the canvas in integer screen coordinates.
		int d_canvas_screen_width;

		//! The height of the canvas in integer screen coordinates.
		int d_canvas_screen_height;

		//! The smaller of the dimensions (width/height) of the screen.
		double d_smaller_dim;

		//! The larger of the dimensions (width/height) of the screen.
		double d_larger_dim;

		boost::optional<MousePressInfo> d_mouse_press_info;

		GPlatesGui::Globe d_globe;

		void
		set_view();

		void
		update_mouse_pointer_pos(
				QMouseEvent *mouse_event);

		void
		update_dimensions();

		/**
		 * Get the "universe" y-coordinate of the current mouse pointer position.
		 *
		 * Note that this function makes no statement about whether the current mouse
		 * pointer position is on the globe or not.
		 */
		inline
		double
		get_universe_coord_y_of_mouse() const
		{
			return get_universe_coord_y(d_mouse_pointer_screen_pos_x);
		}

		/**
		 * Get the "universe" z-coordinate of the current mouse pointer position.
		 *
		 * Note that this function makes no statement about whether the current mouse
		 * pointer position is on the globe or not.
		 */
		inline
		double
		get_universe_coord_z_of_mouse() const
		{
			return get_universe_coord_z(d_mouse_pointer_screen_pos_y);
		}

		/**
		 * Translate the screen x-coordinate @a screen_x to the corresponding "universe"
		 * y-coordinate.
		 *
		 * Note that this function makes no statement about whether the screen position is
		 * on the globe or not.
		 */
		double
		get_universe_coord_y(
				int screen_x) const;

		/**
		 * Translate the screen y-coordinate @a screen_y to the corresponding "universe"
		 * z-coordinate.
		 *
		 * Note that this function makes no statement about whether the screen position is
		 * on the globe or not.
		 */
		double
		get_universe_coord_z(
				int screen_y) const;

		void
		clear_canvas(
				const QColor& color = Qt::black);

		/**
		 * Creates a render graph and sets up some state in preparation for globe drawing.
		 */
		GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type
		initialise_render_graph(
				GPlatesOpenGL::GLRenderGraph &render_graph,
				const GPlatesOpenGL::GLTransform::non_null_ptr_to_const_type &projection_transform);

		void
		draw_render_graph(
				GPlatesOpenGL::GLRenderGraph &render_graph);

		//! Calculates scaling for lines, points and text based on size of canvas
		float
		calculate_scale();

	};

}

#endif  // GPLATES_QTWIDGETS_GLOBECANVAS_H
