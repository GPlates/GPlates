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
#include <QPointF>
#include <QSize>
#include <QtGlobal>

#include "global/config.h" // For GPLATES_USE_VULKAN_BACKEND
#if defined(GPLATES_USE_VULKAN_BACKEND)
#	include <QVulkanWindow>
#else
#	include <QOpenGLWindow>
#endif

#include "gui/Scene.h"
#include "gui/SceneOverlays.h"
#include "gui/SceneRenderer.h"
#include "gui/SceneView.h"

#include "maths/PointOnSphere.h"

#include "opengl/GLContext.h"
#include "opengl/GLMatrix.h"
#include "opengl/GLVisualLayers.h"
#include "opengl/OpenGL.h"  // For Class GL and the OpenGL constants/typedefs


namespace GPlatesGui
{
	class Camera;
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
#if defined(GPLATES_USE_VULKAN_BACKEND)
			public QVulkanWindow
#else
			public QOpenGLWindow
#endif
	{
		Q_OBJECT

	public:

		explicit
		GlobeAndMapCanvas(
				GPlatesPresentation::ViewState &view_state);

		~GlobeAndMapCanvas();


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
		 * The image is cleared to @a image_clear_colour prior to rendering.
		 * This colour will show through for pixels not rendered (or pixel rendered with transparent alpha).
		 *
		 * Returns a null QImage if unable to allocate enough memory for the image data.
		 */
		QImage
		render_to_image(
				const QSize &image_size_in_device_independent_pixels,
				const GPlatesGui::Colour &image_clear_colour);


		/**
		 * Return the camera controlling the current view (globe or map camera).
		 */
		const GPlatesGui::Camera &
		get_active_camera() const
		{
			return d_scene_view->get_active_camera();
		}

		/**
		 * Return the camera controlling the current view (globe or map camera).
		 */
		GPlatesGui::Camera &
		get_active_camera()
		{
			return d_scene_view->get_active_camera();
		}


		/**
		 * Returns true if the globe view is currently active.
		 */
		bool
		is_globe_active() const
		{
			return d_scene_view->is_globe_active();
		}

		/**
		 * Returns true if the map view is currently active.
		 */
		bool
		is_map_active() const
		{
			return d_scene_view->is_map_active();
		}


		/**
		 * Returns the OpenGL context representing our QVulkanWindow (or QOpenGLWindow).
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
			return d_scene->get_gl_visual_layers();
		}

		/**
		 * The proximity inclusion threshold is a dot product (cosine) measure of how close a geometry
		 * must be to a click-point be considered "hit" by the click.
		 *
		 * This will depend on the projection of the globe/map.
		 * For 3D projections the horizon of the globe will need a larger threshold than the centre of the globe.
		 * For 2D projections the threshold will vary with the 'stretch' around the clicked-point.
		 */
		double
		current_proximity_inclusion_threshold(
				const GPlatesMaths::PointOnSphere &click_point) const;

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

#if defined(GPLATES_USE_VULKAN_BACKEND)

		/**
		 * Choose the index of the VkPhysicalDevice that has a queue family supporting graphics and compute.
		 */
		void
		set_vulkan_physical_device_index();

		/**
		 * Callbacks when VkDevice is created and destroyed, and when need to render to canvas.
		 */
		class VulkanWindowRenderer :
				public QVulkanWindowRenderer
		{
		public:
			explicit
			VulkanWindowRenderer(
					GlobeAndMapCanvas *canvas) :
				d_canvas(canvas)
			{  }

			void
			initResources() override
			{
				d_canvas->initialize_gl();
			}

			void
			releaseResources() override
			{
				d_canvas->shutdown_gl();
			}

			void
			startNextFrame() override
			{
				// Render the scene.
				d_canvas->paint_gl();

				// We've finished rendering, so present the rendered frame.
				d_canvas->frameReady();

				// Emit 'repainted' signal.
				d_canvas->emit_repainted();
			}

		private:
			GlobeAndMapCanvas *d_canvas;
		};

		/**
		 * This is a virtual override of the function in QVulkanWindow.
		 */
		QVulkanWindowRenderer *
		createRenderer() override
		{
			// This QVulkanWindow takes ownership of returned pointer.
			return new VulkanWindowRenderer(this);
		}

		void
		paint_gl();

#else

		/**
		 * This is a virtual override of the function in QOpenGLWindow.
		 */
		void 
		initializeGL() override;

		/**
		 * This is a virtual override of the function in QOpenGLWindow.
		 */
		void
		paintGL() override;


		void
		paintEvent(
				QPaintEvent *paint_event) override;

#endif

		//! Emit 'repainted' signal when finished rendering a frame to the canvas.
		void
		emit_repainted();


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

		void
		wheelEvent(
				QWheelEvent *wheel_event) override;

	private Q_SLOTS:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		void 
		initialize_gl();

		void 
		shutdown_gl() ;

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


		//! Mirrors an OpenGL context and provides a central place to manage low-level OpenGL objects.
		GPlatesOpenGL::GLContext::non_null_ptr_type d_gl_context;

		//! Is true if OpenGL has been initialised for this canvas.
		bool d_initialised_gl;


		/**
		 * The scene contains the globe and map.
		 */
		GPlatesGui::Scene::non_null_ptr_type d_scene;

		/**
		 * The view (including projection) of the scene (globe and map).
		 */
		GPlatesGui::SceneView::non_null_ptr_type d_scene_view;

		/**
		 * Any overlays that get rendered in 2D, on top of the 3D scene (globe and map).
		 */
		GPlatesGui::SceneOverlays::non_null_ptr_type d_scene_overlays;

		/**
		 * The scene renderer.
		 *
		 * Can render scene into this canvas or into an arbitrary-size QImage.
		 */
		GPlatesGui::SceneRenderer::non_null_ptr_type d_scene_renderer;


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

		/**
		 * Information of the current mouse left button press (or none is not currently pressed).
		 */
		boost::optional<MousePressInfo> d_mouse_press_info;


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

		/**
		 * Update the mouse screen position and associated position on globe/map (from mouse event).
		 */
		void
		update_mouse_position(
				QMouseEvent *mouse_event);
	};
}

#endif // GPLATES_QTWIDGETS_GLOBEANDMAPCANVAS_H
