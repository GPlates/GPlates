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
 
#ifndef GPLATES_QTWIDGETS_MAPCANVAS_H
#define GPLATES_QTWIDGETS_MAPCANVAS_H

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <QGLWidget>
#include <QImage>
#include <QMouseEvent>
#include <QPaintDevice>
#include <QPoint>
#include <QSize>

#include "SceneView.h"

#include "gui/Map.h"

#include "opengl/GLContext.h"
#include "opengl/GLMatrix.h"
#include "opengl/GLFramebuffer.h"
#include "opengl/GLRenderbuffer.h"
#include "opengl/GLViewProjection.h"
#include "opengl/GLVisualLayers.h"


namespace GPlatesGui
{
	class MapCamera;
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
	class GlobeCanvas;

	/**
	 * Responsible for invoking the functions to paint items onto the map.
	 *
	 * Note: This is analogous to GlobeCanvas.
	 */
	class MapCanvas : 
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


		/**
		 * Constructor.
		 *
		 * @a globe_canvas is provided so we can share OpenGL state with it.
		 * This is state that can be shared across OpenGL contexts (such as texture objects, vertex buffer objects, etc).
		 * This is important since high-resolution rasters can consume a lot of memory and we don't want to double that memory usage.
		 * This is currently used to share textures, etc, with the OpenGL context in GlobeCanvas.
		 */
		MapCanvas(
				GPlatesPresentation::ViewState &view_state,
				GlobeCanvas &globe_canvas,
				QWidget *parent_ = NULL);

		~MapCanvas();

		GPlatesGui::Map &
		map()
		{
			return d_map;
		}

		const GPlatesGui::Map &
		map() const
		{
			return d_map;
		}

		double
		current_proximity_inclusion_threshold(
				const GPlatesMaths::PointOnSphere &click_point) const;

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
				const QSize &image_size_in_device_independent_pixels);

		/**
		 * Paint the scene, as best as possible, by re-directing OpenGL rendering to the specified paint device.
		 */
		void
		render_opengl_feedback_to_paint_device(
				QPaintDevice &feedback_paint_device);

		/**
		 * Returns the LatLonPoint at the centre of the active view, if the central point is on the surface of the earth. 
		 */
		boost::optional<GPlatesMaths::LatLonPoint>
		get_camera_viewpoint() const override;

		/** 
		 * Translates the view so that the LatLonPoint llp is centred on the viewport. 
		 */
		void
		set_camera_viewpoint(
				const GPlatesMaths::LatLonPoint &camera_viewpoint) override;

		void
		set_orientation(
				const GPlatesMaths::Rotation &rotation
				/*bool should_emit_external_signal  = true*/) override;

		boost::optional<GPlatesMaths::Rotation> 
		get_orientation() const override
		{
			return boost::none;
		}

		/**
		 * Returns the OpenGL context associated with our QGLWidget viewport.
		 */
		GPlatesOpenGL::GLContext::non_null_ptr_type
		get_gl_context()
		{
			return d_gl_context;
		}

		/**
		 * Returns the OpenGL layers used to filled polygons, render rasters and scalar fields.
		 */
		GPlatesOpenGL::GLVisualLayers::non_null_ptr_type
		get_gl_visual_layers()
		{
			return d_gl_visual_layers;
		}

	public Q_SLOTS:
		
		void
		update_canvas();

	Q_SIGNALS:

		void
		mouse_pointer_position_changed(
				const boost::optional<GPlatesMaths::LatLonPoint> &,
				bool is_on_surface);
				
		void
		mouse_pressed(
				const QPointF &point_on_scene,
				bool is_on_surface,
				Qt::MouseButton button,
				Qt::KeyboardModifiers modifiers);
				

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
		repainted(
				bool mouse_down);

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

		void 
		mouseMoveEvent(
				QMouseEvent *mouse_event) override;

		void
		mousePressEvent(
				QMouseEvent *mouse_event) override;

		void
		mouseDoubleClickEvent(
				QMouseEvent *mouse_event) override;

		void 
		mouseReleaseEvent(
				QMouseEvent *mouse_event) override;

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

	private:

		/**
		 * Utility class to make the OpenGL context current in @a MapCanvas constructor.
		 *
		 * This is so we can do OpenGL stuff in the @a MapCanvas constructor when normally
		 * we'd have to wait until 'drawBackground()'.
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
		//! Makes the OpenGL context current in @a GlobeCanvas constructor so it can call OpenGL.
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

		//! Paints an optional text overlay onto the map.
		boost::scoped_ptr<GPlatesGui::TextOverlay> d_text_overlay;

		//! Paints an optional velocity legend overlay onto the map.
		boost::scoped_ptr<GPlatesGui::VelocityLegendOverlay> d_velocity_legend_overlay;

		//! Holds the state
		GPlatesGui::Map d_map;

		/**
		 * The position of the mouse pointer in view coordinates.
		 */
		QPoint d_mouse_pointer_screen_pos;

		/**
		 * The last position of the mouse in view (screen) coordinates.
		 */
		QPoint d_last_mouse_view_coords;

		boost::optional<MousePressInfo> d_mouse_press_info;


		//! Dimensions of square render target used for offscreen rendering.
		static const unsigned int OFF_SCREEN_RENDER_TARGET_DIMENSION = 1024;


		//! Calls 'initializeGL()' if it hasn't already been called.
		void
		initializeGL_if_necessary();

		//! Create and initialise the framebuffer and its renderbuffers used for offscreen rendering.
		void
		initialize_off_screen_render_target(
				GPlatesOpenGL::GL &gl);

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
				QImage &image,
				const QPaintDevice &map_canvas_paint_device);

		//! Render onto the canvas.
		cache_handle_type
		render_scene(
				GPlatesOpenGL::GL &gl,
				const GPlatesOpenGL::GLViewProjection &view_projection,
				int paint_device_width_in_device_independent_pixels,
				int paint_device_height_in_device_independent_pixels,
				int map_canvas_paint_device_width_in_device_independent_pixels,
				int map_canvas_paint_device_height_in_device_independent_pixels);

		void
		update_mouse_pointer_pos(
				QMouseEvent *mouse_event);

		void
		handle_mouse_pointer_pos_change();

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
		 * Move camera by @a dx and @a dy, both expressed in window coordinates.
		 */
		void
		move_camera(
				double dx,
				double dy);

		//! Calculate scaling for lines, points and text based on size of view
		float
		calculate_scale(
				int paint_device_width_in_device_independent_pixels,
				int paint_device_height_in_device_independent_pixels,
				int map_canvas_paint_device_width_in_device_independent_pixels,
				int map_canvas_paint_device_height_in_device_independent_pixels);

	};
}

#endif // GPLATES_QTWIDGETS_MAPCANVAS_H
