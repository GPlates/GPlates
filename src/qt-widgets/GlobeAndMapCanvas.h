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
 
#ifndef GPLATES_QTWIDGETS_GLOBEANDMAPCANVAS_H
#define GPLATES_QTWIDGETS_GLOBEANDMAPCANVAS_H

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <QImage>
#include <QMouseEvent>
#include <QOpenGLWidget>
#include <QPaintDevice>
#include <QPointF>
#include <QSize>
#include <QtGlobal>

#include "gui/Globe.h"
#include "gui/Map.h"
#include "gui/Projection.h"

#include "maths/LatLonPoint.h"
#include "maths/PointOnSphere.h"

#include "opengl/GLContext.h"
#include "opengl/GLMatrix.h"
#include "opengl/GLFramebuffer.h"
#include "opengl/GLRenderbuffer.h"
#include "opengl/GLViewProjection.h"
#include "opengl/GLVisualLayers.h"
#include "opengl/OpenGL.h"  // For Class GL and the OpenGL constants/typedefs


namespace GPlatesGui
{
	class Camera;
	class TextOverlay;
	class VelocityLegendOverlay;
}

namespace GPlatesOpenGL
{
	class GLTileRender;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	/**
	 * Paint the globe and map views.
	 */
	class GlobeAndMapCanvas : 
			public QOpenGLWidget
	{
		Q_OBJECT

	public:

		GlobeAndMapCanvas(
				GPlatesPresentation::ViewState &view_state,
				QWidget *parent_ = nullptr);

		~GlobeAndMapCanvas();


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
		get_viewport_size() const;

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
		 * Return the camera controlling the current view (globe or map camera).
		 */
		const GPlatesGui::Camera &
		get_active_camera() const;

		/**
		 * Return the camera controlling the current view (globe or map camera).
		 */
		GPlatesGui::Camera &
		get_active_camera();


		/**
		 * Returns true if the globe view is currently active.
		 */
		bool
		is_globe_active() const;

		/**
		 * Returns true if the map view is currently active.
		 */
		bool
		is_map_active() const
		{
			return !is_globe_active();
		}


		/**
		 * Returns the OpenGL context associated with our QOpenGLWidget viewport.
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
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		void
		update_canvas();

	Q_SIGNALS:

		//
		// NOTE: These signals do NOT pass by reference (const) since a reference points to an underlying data member and
		//       that data member can change within the duration of the slot being signaled.
		//       For example, changing the map camera causes it to emit a signal that we (GlobeAndMapCanvas) use to update the
		//       current map position under the mouse cursor. So if a signaled slot updates the camera and then continues
		//       to use the reference after that then it will find the referenced variable has unexpectedly been updated.
		//

		void
		mouse_position_on_globe_changed(
				GPlatesMaths::PointOnSphere position_on_globe,
				bool is_on_globe);
			
		// Mouse pressed when globe is active.
		void
		mouse_pressed_when_globe_active(
				int screen_width,
				int screen_height,
				QPointF press_screen_position,
				GPlatesMaths::PointOnSphere press_position_on_globe,
				bool is_on_globe,
				Qt::MouseButton button,
				Qt::KeyboardModifiers modifiers);

		// Mouse pressed when map is active.
		void
		mouse_pressed_when_map_active(
				int screen_width,
				int screen_height,
				QPointF press_screen_position,
				boost::optional<QPointF> press_map_position,
				GPlatesMaths::PointOnSphere press_position_on_globe,
				bool is_on_globe,
				Qt::MouseButton button,
				Qt::KeyboardModifiers modifiers);
				

		// Mouse clicked when globe is active.
		void
		mouse_clicked_when_globe_active(
				int screen_width,
				int screen_height,
				QPointF click_screen_position,
				GPlatesMaths::PointOnSphere click_position_on_globe,
				bool is_on_globe,
				Qt::MouseButton button,
				Qt::KeyboardModifiers modifiers);

		// Mouse clicked when map is active.
		void
		mouse_clicked_when_map_active(
				int screen_width,
				int screen_height,
				QPointF click_screen_position,
				boost::optional<QPointF> click_map_position,
				GPlatesMaths::PointOnSphere click_position_on_globe,
				bool is_on_globe,
				Qt::MouseButton button,
				Qt::KeyboardModifiers modifiers);


		// Mouse dragged when globe is active.
		void
		mouse_dragged_when_globe_active(
				int screen_width,
				int screen_height,
				QPointF initial_screen_position,
				GPlatesMaths::PointOnSphere initial_position_on_globe,
				bool was_on_globe,
				QPointF current_screen_position,
				GPlatesMaths::PointOnSphere current_position_on_globe,
				bool is_on_globe,
				GPlatesMaths::PointOnSphere centre_of_viewport_on_globe,
				Qt::MouseButton button,
				Qt::KeyboardModifiers modifiers);

		// Mouse dragged when map is active.
		void
		mouse_dragged_when_map_active(
				int screen_width,
				int screen_height,
				QPointF initial_screen_position,
				boost::optional<QPointF> initial_map_position,
				GPlatesMaths::PointOnSphere initial_position_on_globe,
				bool was_on_globe,
				QPointF current_screen_position,
				boost::optional<QPointF> current_map_position,
				GPlatesMaths::PointOnSphere current_position_on_globe,
				bool is_on_globe,
				GPlatesMaths::PointOnSphere centre_of_viewport_on_globe,
				Qt::MouseButton button,
				Qt::KeyboardModifiers modifiers);


		// Mouse released after drag when globe is active.
		void
		mouse_released_after_drag_when_globe_active(
				int screen_width,
				int screen_height,
				QPointF initial_screen_position,
				GPlatesMaths::PointOnSphere initial_position_on_globe,
				bool was_on_globe,
				QPointF current_screen_position,
				GPlatesMaths::PointOnSphere current_position_on_globe,
				bool is_on_globe,
				GPlatesMaths::PointOnSphere centre_of_viewport_on_globe,
				Qt::MouseButton button,
				Qt::KeyboardModifiers modifiers);

		// Mouse released after drag when map is active.
		void
		mouse_released_after_drag_when_map_active(
				int screen_width,
				int screen_height,
				QPointF initial_screen_position,
				boost::optional<QPointF> initial_map_position,
				GPlatesMaths::PointOnSphere initial_position_on_globe,
				bool was_on_globe,
				QPointF current_screen_position,
				boost::optional<QPointF> current_map_position,
				GPlatesMaths::PointOnSphere current_position_on_globe,
				bool is_on_globe,
				GPlatesMaths::PointOnSphere centre_of_viewport_on_globe,
				Qt::MouseButton button,
				Qt::KeyboardModifiers modifiers);


		// Mouse moved without drag when globe is active.
		void
		mouse_moved_without_drag_when_globe_active(
				int screen_width,
				int screen_height,
				QPointF screen_position,
				GPlatesMaths::PointOnSphere position_on_globe,
				bool is_on_globe,
				GPlatesMaths::PointOnSphere centre_of_viewport_on_globe);

		// Mouse moved without drag when map is active.
		void
		mouse_moved_without_drag_when_map_active(
				int screen_width,
				int screen_height,
				QPointF screen_position,
				boost::optional<QPointF> map_position,
				GPlatesMaths::PointOnSphere position_on_globe,
				bool is_on_globe,
				GPlatesMaths::PointOnSphere centre_of_viewport_on_globe);


		void
		repainted(
				bool mouse_down);

	protected:

		/**
		 * This is a virtual override of the function in QOpenGLWidget.
		 */
		void 
		initializeGL() override;

		/**
		 * This is a virtual override of the function in QOpenGLWidget.
		 */
		void 
		resizeGL(
				int width, 
				int height) override;

		/**
		 * This is a virtual override of the function in QOpenGLWidget.
		 */
		void
		paintGL() override;


		void
		paintEvent(
				QPaintEvent *paint_event) override;

		void
		mousePressEvent(
				QMouseEvent *mouse_event) override;

		void 
		mouseMoveEvent(
				QMouseEvent *mouse_event) override;

		void 
		mouseReleaseEvent(
				QMouseEvent *mouse_event) override;

		void
		keyPressEvent(
				QKeyEvent *key_event) override;

	private Q_SLOTS:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		void
		handle_camera_change();

		void
		handle_globe_map_projection_changed(
				const GPlatesGui::Projection::globe_map_projection_type &old_globe_map_projection,
				const GPlatesGui::Projection::globe_map_projection_type &globe_map_projection);

		void
		handle_viewport_projection_changed(
				GPlatesGui::Projection::viewport_projection_type old_viewport_projection,
				GPlatesGui::Projection::viewport_projection_type viewport_projection);

	private:

		struct MousePressInfo
		{
			MousePressInfo(
					const QPointF &mouse_screen_position,
					const boost::optional<QPointF> &mouse_map_position,
					const GPlatesMaths::PointOnSphere &mouse_position_on_globe,
					bool mouse_is_on_globe,
					Qt::MouseButton button,
					Qt::KeyboardModifiers modifiers):
				d_mouse_screen_position(mouse_screen_position),
				d_mouse_map_position(mouse_map_position),
				d_mouse_position_on_globe(mouse_position_on_globe),
				d_mouse_is_on_globe(mouse_is_on_globe),
				d_button(button),
				d_modifiers(modifiers),
				d_is_mouse_drag(false)
			{  }

			QPointF d_mouse_screen_position;
			boost::optional<QPointF> d_mouse_map_position;  // Only used when map is active (ie, when globe is inactive).
			GPlatesMaths::PointOnSphere d_mouse_position_on_globe;
			bool d_mouse_is_on_globe;
			Qt::MouseButton d_button;
			Qt::KeyboardModifiers d_modifiers;
			bool d_is_mouse_drag;
		};


		/**
		 * Typedef for an opaque object that caches a particular painting.
		 */
		typedef boost::shared_ptr<void> cache_handle_type;


		GPlatesPresentation::ViewState &d_view_state;

		//! Mirrors an OpenGL context and provides a central place to manage low-level OpenGL objects.
		GPlatesOpenGL::GLContext::non_null_ptr_type d_gl_context;

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

		//! The mouse pointer position on the *screen*.
		QPointF d_mouse_screen_position;

		/**
		 * The mouse pointer position on the map *plane* (2D plane with z=0), or none if screen view ray
		 * at the mouse pointer *screen* position does not intersect the map plane.
		 *
		 * Note: This is only used when the map is active (ie, when globe is inactive).
		 */
		boost::optional<QPointF> d_mouse_position_on_map_plane;

		/**
		 * If the mouse pointer is on the globe, this is the position of the mouse pointer on the globe.
		 *
		 * Otherwise, this is the closest position on the globe to the position of the mouse pointer.
		 *
		 * When the map is active (ie, when globe is inactive) the mouse pointer is considered to intersect
		 * the globe if it intersects the map plane at a position that is inside the map projection boundary.
		 */
		GPlatesMaths::PointOnSphere d_mouse_position_on_globe;

		/**
		 * Whether the mouse pointer is on the globe.
		 *
		 * When the globe is active (ie, when map is inactive) this is true if the mouse pointer is
		 * on the globe's sphere.
		 *
		 * When the map is active (ie, when globe is inactive) this is true if the mouse pointer is
		 * on the map plane and inside the map projection boundary.
		 */
		bool d_mouse_is_on_globe;

		boost::optional<MousePressInfo> d_mouse_press_info;

		/**
		 * The projection determines whether the globe view or map view is currently active
		 * (as well as whether the view is orthographic or perspective).
		 */
		GPlatesGui::Projection &d_projection;

		//! Holds the globe state.
		GPlatesGui::Globe d_globe;

		//! Holds the map state.
		GPlatesGui::Map d_map;

		//! Paints an optional text overlay onto the globe/map.
		boost::scoped_ptr<GPlatesGui::TextOverlay> d_text_overlay;

		//! Paints an optional velocity legend overlay onto the globe/map.
		boost::scoped_ptr<GPlatesGui::VelocityLegendOverlay> d_velocity_legend_overlay;


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

		//! Render onto the canvas.
		cache_handle_type
		render_scene(
				GPlatesOpenGL::GL &gl,
				const GPlatesOpenGL::GLViewProjection &view_projection,
				int paint_device_width_in_device_independent_pixels,
				int paint_device_height_in_device_independent_pixels);

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
		 * The point on the globe which corresponds to the centre of the viewport.
		 */
		GPlatesMaths::PointOnSphere
		centre_of_viewport() const;

		/**
		 * Returns true if the distance from the current mouse screen position to the position
		 * when the mouse was first pressed is greater than a threshold.
		 */
		bool
		is_mouse_in_drag() const;

		void
		update_mouse_screen_position(
				QMouseEvent *mouse_event);

		void
		update_mouse_position_on_globe_or_map();

		/**
		 * Update mouse position on globe when globe is active (ie, when map is inactive).
		 */
		void
		update_mouse_position_on_globe(
				const GPlatesOpenGL::GLIntersect::Ray &camera_ray);

		/**
		 * Update mouse position on globe when map is active (ie, when globe is inactive).
		 */
		void
		update_mouse_position_on_map(
				const GPlatesOpenGL::GLIntersect::Ray &camera_ray);


		//! Calculate scaling for lines, points and text based on size of the view.
		float
		calculate_scale(
				int paint_device_width_in_device_independent_pixels,
				int paint_device_height_in_device_independent_pixels) const;

	};
}

#endif // GPLATES_QTWIDGETS_GLOBEANDMAPCANVAS_H
