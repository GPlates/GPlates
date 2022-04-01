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
 * Copyright (C) 2006, 2007, 2010, 2011 The University of Sydney, Australia
 *  (under the name "GlobeCanvas.h")
 * Copyright (C) 2011 Geological Survey of Norway
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

#include <utility>  // std::pair
#include <vector>
#include <boost/optional.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <QGLWidget>
#include <QImage>
#include <QPaintDevice>
#include <QPainter>
#include <QtGlobal>
#include <QtOpenGL/qgl.h>

#include "SceneView.h"

#include "gui/Globe.h"

#include "maths/PointOnSphere.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"

#include "opengl/GLContext.h"
#include "opengl/GLFramebuffer.h"
#include "opengl/GLMatrix.h"
#include "opengl/GLRenderbuffer.h"
#include "opengl/GLViewProjection.h"
#include "opengl/GLVisualLayers.h"

#include "view-operations/RenderedGeometryFactory.h"


namespace GPlatesGui
{
	class GlobeCamera;
	class TextOverlay;
	class VelocityLegendOverlay;
}

namespace GPlatesOpenGL
{
	class GL;
	class GLTileRender;
}

namespace GPlatesPresentation
{
	class ViewState;
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
					qreal mouse_pointer_screen_pos_x,
					qreal mouse_pointer_screen_pos_y,
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

			qreal d_mouse_pointer_screen_pos_x;
			qreal d_mouse_pointer_screen_pos_y;
			GPlatesMaths::PointOnSphere d_mouse_pointer_pos;
			bool d_is_on_globe;
			Qt::MouseButton d_button;
			Qt::KeyboardModifiers d_modifiers;
			bool d_is_mouse_drag;
		};


		explicit
		GlobeCanvas(
				GPlatesPresentation::ViewState &view_state,
				QWidget *parent_ = 0);

		~GlobeCanvas();


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

		/**
		 * The point which corresponds to the centre of the viewport.
		 */
		GPlatesMaths::PointOnSphere
		centre_of_viewport() const;

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

		double
		mouse_pointer_screen_pos_x() const
		{
			return d_mouse_pointer_screen_pos_x;
		}

		double
		mouse_pointer_screen_pos_y() const
		{
			return d_mouse_pointer_screen_pos_y;
		}

		/**
		 * If the mouse pointer is on the globe, return the position of the mouse pointer
		 * on the globe.
		 *
		 * Otherwise, return the closest position on the globe to the position of the
		 * mouse pointer in the 3-D "universe".
		 */
		const GPlatesMaths::PointOnSphere &
		mouse_pointer_pos_on_globe() const
		{
			return d_mouse_pointer_pos_on_globe;
		}

		/**
		 * Return whether the mouse pointer is on the globe.
		 */
		bool
		mouse_pointer_is_on_globe() const
		{
			return d_mouse_pointer_is_on_globe;
		}

		/**
		 * Returns the dimensions of the viewport in device *independent* pixels (ie, widget size).
		 *
		 * Device-independent pixels (widget size) differ from device pixels (OpenGL size).
		 * Widget dimensions are device independent whereas OpenGL uses device pixels
		 * (differing by the device pixel ratio).
		 */
		QSize
		get_viewport_size() const override;

		/**
		 * Renders the scene to a QImage of the dimensions specified by @a image_size.
		 *
		 * The specified image size should be in device *independent* pixels (eg, widget dimensions).
		 * The returned image will be a high-DPI image if this canvas has a device pixel ratio greater than 1.0
		 * (in which case the returned QImage will have the same device pixel ratio).
		 *
		 * Returns a null QImage if unable to allocate enough memory for the image data.
		 */
		QImage
		render_to_qimage(
				const QSize &image_size_in_device_independent_pixels) override;

		/**
		 * Paint the scene, as best as possible, by re-directing OpenGL rendering to the specified paint device.
		 */
		void
		render_opengl_feedback_to_paint_device(
				QPaintDevice &feedback_paint_device) override;

		void
		set_camera_viewpoint(
				const GPlatesMaths::LatLonPoint &camera_viewpoint) override;

		boost::optional<GPlatesMaths::LatLonPoint>
		get_camera_viewpoint() const override;

		boost::optional<GPlatesMaths::Rotation>
		get_orientation() const override;

		void
		set_orientation(
				const GPlatesMaths::Rotation &rotation
				/*bool should_emit_external_signal = true */) override;

		/**
		 * Returns the OpenGL context associated with this QGLWidget.
		 *
		 * This also enables it to be shared across widgets.
		 */
		GPlatesOpenGL::GLContext::non_null_ptr_type
		get_gl_context()
		{
			return d_gl_context;
		}

		//! Returns the persistent OpenGL objects associated with this widget's OpenGL context so it can be shared across widgets.
		GPlatesOpenGL::GLVisualLayers::non_null_ptr_type
		get_gl_visual_layers()
		{
			return d_gl_visual_layers;
		}

	public Q_SLOTS:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		void
		update_canvas() override;

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
		 * This function should set up any required OpenGL context state.
		 *
		 * There is no need to call makeCurrent() because this has already been done when
		 * this function is called.
		 */
		void 
		initializeGL() override;

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
		void 
		resizeGL(
				int width, 
				int height) override;

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
		void
		paintGL() override;

		void
		paintEvent(
				QPaintEvent *paint_event) override;

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
		void
		mousePressEvent(
				QMouseEvent *event) override;

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
		void
		mouseMoveEvent(
				QMouseEvent *event) override;

		/**
		 * This is a virtual override of the function in QWidget.
		 *
		 * To quote the QWidget documentation:
		 *
		 * This event handler, for event event, can be reimplemented in a subclass to
		 * receive mouse release events for the widget.
		 */
		void 
		mouseReleaseEvent(
				QMouseEvent *event) override;

		void
		keyPressEvent(
				QKeyEvent *key_event) override;

		void
		move_camera_up() override;

		void
		move_camera_down() override;

		void
		move_camera_left() override;

		void
		move_camera_right() override;

		void
		rotate_camera_clockwise() override;

		void
		rotate_camera_anticlockwise() override;

		void
		reset_camera_orientation() override;

	Q_SIGNALS:

		void
		mouse_pointer_position_changed(
				int screen_width,
				int screen_height,
				double screen_x,
				double screen_y,
				const GPlatesMaths::PointOnSphere &new_pos,
				bool is_on_globe);
				
				
		void
		mouse_pressed(
				int screen_width,
				int screen_height,
				double press_screen_x,
				double press_screen_y,
				const GPlatesMaths::PointOnSphere &press_pos_on_globe,
				bool is_on_globe,
				Qt::MouseButton button,
				Qt::KeyboardModifiers modifiers);

		void
		mouse_clicked(
				int screen_width,
				int screen_height,
				double click_screen_x,
				double click_screen_y,
				const GPlatesMaths::PointOnSphere &click_pos_on_globe,
				bool is_on_globe,
				Qt::MouseButton button,
				Qt::KeyboardModifiers modifiers);

		void
		mouse_dragged(
				int screen_width,
				int screen_height,
				double initial_screen_x,
				double initial_screen_y,
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				bool was_on_globe,
				double current_screen_x,
				double current_screen_y,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport,
				Qt::MouseButton button,
				Qt::KeyboardModifiers modifiers);

		void
		mouse_released_after_drag(
				int screen_width,
				int screen_height,
				double initial_screen_x,
				double initial_screen_y,
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				bool was_on_globe,
				double current_screen_x,
				double current_screen_y,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport,
				Qt::MouseButton button,
				Qt::KeyboardModifiers modifiers);

		/**
		 * The mouse position moved but the left mouse button is NOT down.
		 */
		void
		mouse_moved_without_drag(
				int screen_width,
				int screen_height,
				double current_screen_x,
				double current_screen_y,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe,
				const GPlatesMaths::PointOnSphere &centre_of_viewport);

		void
		repainted(
				bool mouse_down);

	public Q_SLOTS:

		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		void
		handle_camera_change();

	private:
		/**
		 * Utility class to make the QGLWidget's OpenGL context current in @a GlobeCanvas constructor.
		 *
		 * This is so we can do OpenGL stuff in the @a GlobeCanvas constructor when normally
		 * we'd have to wait until 'initializeGL()'.
		 */
		struct MakeGLContextCurrent
		{
			explicit
			MakeGLContextCurrent(
					GPlatesOpenGL::GLContext &gl_context)
			{
				gl_context.make_current();
			}
		};

		/**
		 * Typedef for an opaque object that caches a particular painting.
		 */
		typedef boost::shared_ptr<void> cache_handle_type;


		GPlatesPresentation::ViewState &d_view_state;

		//! Mirrors an OpenGL context and provides a central place to manage low-level OpenGL objects.
		GPlatesOpenGL::GLContext::non_null_ptr_type d_gl_context;
		//! Makes the QGLWidget's OpenGL context current in @a GlobeCanvas constructor so it can call OpenGL.
		MakeGLContextCurrent d_make_context_current;

		//! Is true if OpenGL has been initialised for this canvas.
		bool d_initialisedGL;

		/**
		 * The current view-projection transform and viewport.
		 */
		GPlatesOpenGL::GLViewProjection d_view_projection;

		//! Colour renderbuffer object used for offscreen rendering.
		GPlatesOpenGL::GLRenderbuffer::shared_ptr_type d_off_screen_colour_renderbuffer;

		//! Depth/stencil renderbuffer object used for offscreen rendering.
		GPlatesOpenGL::GLRenderbuffer::shared_ptr_type d_off_screen_depth_stencil_renderbuffer;

		//! Framebuffer object used for offscreen rendering.
		GPlatesOpenGL::GLFramebuffer::shared_ptr_type d_off_screen_framebuffer;

		//! Dimensions of square render target used for offscreen rendering.
		unsigned int d_off_screen_render_target_dimension;

		//! Keeps track of OpenGL objects that persist from one render to another.
		GPlatesOpenGL::GLVisualLayers::non_null_ptr_type d_gl_visual_layers;

		/**
		 * Enables frame-to-frame caching of persistent OpenGL resources.
		 *
		 * There is a certain amount of caching without this already.
		 * This just prevents a render frame from invalidating cached resources of the previous frame
		 * in order to avoid regenerating the same cached resources unnecessarily each frame.
		 * We hold onto the previous frame's cached resources *while* generating the current frame and
		 * then release our hold on the previous frame (and continue this pattern each new frame).
		 */
		cache_handle_type d_gl_frame_cache_handle;

		/**
		 * If the mouse pointer is on the globe, this is the position of the mouse pointer
		 * on the globe.
		 *
		 * Otherwise, this is the closest position on the globe to the position of the
		 * mouse pointer in the 3-D "universe".
		 */
		GPlatesMaths::PointOnSphere d_mouse_pointer_pos_on_globe;

		//! Whether the mouse pointer is on the globe.
		bool d_mouse_pointer_is_on_globe;

		//! The x-coord of the mouse pointer position on the screen.
		qreal d_mouse_pointer_screen_pos_x;

		//! The y-coord of the mouse pointer position on the screen.
		qreal d_mouse_pointer_screen_pos_y;

		boost::optional<MousePressInfo> d_mouse_press_info;

		GPlatesGui::Globe d_globe;
		
		GPlatesGui::GlobeCamera &d_globe_camera;

		//! Paints an optional text overlay onto the globe.
		boost::scoped_ptr<GPlatesGui::TextOverlay> d_text_overlay;

		//! Paints an optional velocity legend overlay onto the globe.
		boost::scoped_ptr<GPlatesGui::VelocityLegendOverlay> d_velocity_legend_overlay;



		/**
		 * How far to nudge or rotate the camera when incrementally moving the camera, in degrees.
		 */
		static const double NUDGE_CAMERA_DEGREES;

		//! Dimensions of square render target used for offscreen rendering.
		static const unsigned int OFF_SCREEN_RENDER_TARGET_DIMENSION = 1024;


		//! Calls 'initializeGL()' if it hasn't already been called.
		void
		initializeGL_if_necessary();

		//! Create and initialise the framebuffer and its renderbuffers used for offscreen rendering.
		void
		initialize_off_screen_render_target(
				GPlatesOpenGL::GL &gl);

		void
		set_view();

		/**
		 * Render one tile of the scene (as specified by @a image_tile_render).
		 *
		 * The sub-rect of @a image to render into is determined by @a image_tile_render.
		 */
		cache_handle_type
		render_scene_tile_into_image(
				GPlatesOpenGL::GL &gl,
				const GPlatesOpenGL::GLMatrix &image_view_transform,
				const GPlatesOpenGL::GLMatrix &image_projection_transform,
				const GPlatesOpenGL::GLTileRender &image_tile_render,
				QImage &image);

		/**
		 * Render the scene.
		 */
		cache_handle_type
		render_scene(
				GPlatesOpenGL::GL &gl,
				const GPlatesOpenGL::GLViewProjection &view_projection,
				int paint_device_width_in_device_independent_pixels,
				int paint_device_height_in_device_independent_pixels);

		void
		update_mouse_pointer_pos(
				QMouseEvent *mouse_event);

		/**
		 * Given the screen coordinates, calculate and return a position which is on
		 * the globe (a unit sphere).
		 *
		 * This position might be the position determined by @a y and @a z, or the closest position
		 * on the globe to the position determined by @a y and @a z.
		 */
		std::pair<bool, GPlatesMaths::PointOnSphere>
		calc_globe_position(
				qreal screen_x,
				qreal screen_y) const;

		//! Calculates scaling for lines, points and text based on size of the paint device.
		float
		calculate_scale(
				int paint_device_width_in_device_independent_pixels,
				int paint_device_height_in_device_independent_pixels) const;

	};

}

#endif  // GPLATES_QTWIDGETS_GLOBECANVAS_H
