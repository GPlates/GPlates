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

#include <cmath>
#include <iostream>
#include <vector>
#include <boost/bind/bind.hpp>
#include <boost/optional.hpp>
#include <QtGlobal>
#include <QDebug>
#include <QString>
#include <QVulkanInstance>

#include "GlobeAndMapCanvas.h"

#include "app-logic/ApplicationState.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/GPlatesException.h"
#include "global/PreconditionViolationError.h"

#include "gui/Colour.h"
#include "gui/GlobeCamera.h"
#include "gui/MapCamera.h"
#include "gui/MapProjection.h"
#include "gui/ViewportZoom.h"

#include "maths/MathsUtils.h"

#include "opengl/GLContext.h"
#include "opengl/GLViewport.h"
#include "opengl/VulkanDevice.h"
#include "opengl/VulkanException.h"
#include "opengl/VulkanSwapchain.h"

#include "presentation/ViewState.h"


GPlatesQtWidgets::GlobeAndMapCanvas::GlobeAndMapCanvas(
		GPlatesPresentation::ViewState &view_state) :
	d_gl_context(GPlatesOpenGL::GLContext::create()),
	d_vulkan_compute_queue_family_index(0),
	d_scene(GPlatesGui::Scene::create(view_state, devicePixelRatio())),
	d_scene_view(GPlatesGui::SceneView::create(view_state)),
	d_scene_overlays(GPlatesGui::SceneOverlays::create(view_state)),
	d_scene_renderer(GPlatesGui::SceneRenderer::create(view_state)),
	// The following unit-vector initialisation value is arbitrary.
	d_mouse_position_on_globe(GPlatesMaths::UnitVector3D(1, 0, 0)),
	d_mouse_is_on_globe(false)
{
	// Set the Vulkan instance in this QWindow.
	//
	// We do this first so that we then subsequently access 'vulkanInstance()' on 'this' QWindow.
	setVulkanInstance(&view_state.get_application_state().get_vulkan_instance());


	// Update our canvas whenever the RenderedGeometryCollection gets updated.
	// This will cause 'paintGL()' to be called which will visit the rendered
	// geometry collection and redraw it.
	QObject::connect(
			&view_state.get_rendered_geometry_collection(),
			SIGNAL(collection_was_updated(
					GPlatesViewOperations::RenderedGeometryCollection &,
					GPlatesViewOperations::RenderedGeometryCollection::main_layers_update_type)),
			this,
			SLOT(update_canvas()));

	// Update our canvas whenever the scene view changes.
	//
	// Note that the scene view changes when the globe/map cameras change.
	QObject::connect(
			&*d_scene_view, SIGNAL(view_changed()),
			this, SLOT(update_canvas()));

	//
	// NOTE: This window is not yet initialised (eg, calling width() and height() might return undefined values).
	//       This might be due to it being managed by "QWidget::createWindowContainer()" (see GlobeAndMapWidget).
	// 
}


GPlatesQtWidgets::GlobeAndMapCanvas::~GlobeAndMapCanvas()
{
}


double
GPlatesQtWidgets::GlobeAndMapCanvas::current_proximity_inclusion_threshold(
		const GPlatesMaths::PointOnSphere &click_point) const
{
	// The viewport should be in device *independent* coordinates.
	// This way if a user has a high DPI display (like Apple Retina) the higher pixel resolution
	// does not force them to have more accurate mouse clicks.
	const GPlatesOpenGL::GLViewport viewport(0, 0, width(), height());

	return d_scene_view->current_proximity_inclusion_threshold(click_point, viewport);
}


QSize
GPlatesQtWidgets::GlobeAndMapCanvas::get_viewport_size() const
{
	return QSize(width(), height());
}


QImage
GPlatesQtWidgets::GlobeAndMapCanvas::render_to_image(
		const QSize &image_size_in_device_independent_pixels,
		const GPlatesGui::Colour &image_clear_colour)
{
	// We need a Vulkan device (VkDevice) which means this VulkanWindow must be exposed/shown.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_vulkan,
			GPLATES_ASSERTION_SOURCE);

	// Start a render scope (all GL calls should be done inside this scope).
	GPlatesOpenGL::GL::non_null_ptr_type gl = d_gl_context->access_opengl();

	// The image to render/copy the scene into.
	//
	// Handle high DPI displays (eg, Apple Retina) by rendering image in high-res device pixels.
	// The image will still be it's original size in device *independent* pixels.
	//
	// TODO: We're using the device pixel ratio of current canvas since we're rendering into that and
	// then copying into image. This might not be ideal if this canvas is displayed on one monitor and
	// the QImage (eg, Colouring previews) will be displayed on another with a different device pixel ratio.
	const QSize image_size_in_device_pixels(
			image_size_in_device_independent_pixels.width() * devicePixelRatio(),
			image_size_in_device_independent_pixels.height() * devicePixelRatio());
	QImage image(image_size_in_device_pixels, QImage::Format_ARGB32);
	image.setDevicePixelRatio(devicePixelRatio());

	if (image.isNull())
	{
		// Most likely a memory allocation failure - return the null image.
		return QImage();
	}

	// Fill the image with the clear colour in case there's an exception during rendering
	// of one of the tiles and the image is incomplete.
	image.fill(QColor(image_clear_colour).rgba());

	// Render the scene into the image.
	d_scene_renderer->render_to_image(image, *gl, *d_scene, *d_scene_overlays, *d_scene_view, image_clear_colour);

	return image;
}


void
GPlatesQtWidgets::GlobeAndMapCanvas::update_canvas()
{
	requestUpdate();
}


void
GPlatesQtWidgets::GlobeAndMapCanvas::initialise_vulkan_resources(
		GPlatesOpenGL::VulkanDevice &vulkan_device)
{
	d_gl_context->initialise_gl(vulkan_device);

	// Start a render scope (all GL calls should be done inside this scope).
	GPlatesOpenGL::GL::non_null_ptr_type gl = d_gl_context->access_opengl();

#if 0
	// Initialise the scene.
	d_scene->initialise_gl(*gl);

	// Initialise the scene renderer.
	d_scene_renderer->initialise_gl(*gl);
#endif
}


void
GPlatesQtWidgets::GlobeAndMapCanvas::release_vulkan_resources(
		GPlatesOpenGL::VulkanDevice &vulkan_device) 
{
	// Start a render scope (all GL calls should be done inside this scope).
	GPlatesOpenGL::GL::non_null_ptr_type gl = d_gl_context->access_opengl();

#if 0
	// Shutdown the scene.
	d_scene->shutdown_gl(*gl);

	// Shutdown the scene renderer.
	d_scene_renderer->shutdown_gl(*gl);
#endif

	d_gl_context->shutdown_gl();
}


void
GPlatesQtWidgets::GlobeAndMapCanvas::render_to_window(
		GPlatesOpenGL::VulkanDevice &vulkan_device,
		GPlatesOpenGL::VulkanSwapchain &vulkan_swapchain)
{
	// Size of the current window/swapchain in device pixels.
	const vk::Extent2D swapchain_size = vulkan_swapchain.get_swapchain_size();
	// The viewport is in device pixels since that is what Vulkan will use to render the scene.
	const GPlatesOpenGL::GLViewport viewport(0, 0, swapchain_size.width, swapchain_size.height);

	//
	// Acquire the next available swapchain image.
	//

	vk::Semaphore swapchain_image_available_semaphore;
	vk::Fence frame_fence;

	vulkan_device.get_device().waitForFences(frame_fence, true, UINT64_MAX);
	vulkan_device.get_device().resetFences(frame_fence);

	std::uint32_t swapchain_image_index;
	const vk::Result acquire_next_image_result = vulkan_device.get_device().acquireNextImageKHR(
			vulkan_swapchain.get_swapchain(),
			UINT64_MAX,
			swapchain_image_available_semaphore,
			nullptr,  // fence
			&swapchain_image_index);
	if (acquire_next_image_result != vk::Result::eSuccess)
	{
		if (acquire_next_image_result == vk::Result::eErrorOutOfDateKHR ||
			acquire_next_image_result == vk::Result::eSuboptimalKHR)
		{
			// Either surface is no longer compatible with the swapchain (vk::Result::eErrorOutOfDateKHR) or
			// swapchain no longer matches but we can still present to the surface (vk::Result::eSuboptimalKHR).
			//
			// In either case we'll recreate the swapchain and render again.
			vulkan_swapchain.recreate_swapchain(swapchain_size);
			update_canvas();
			return;
		}

		throw GPlatesOpenGL::VulkanException(
				GPLATES_EXCEPTION_SOURCE,
				"Failed to acquire next Vulkan swapchain image.");
	}

	//
	// Swapchain render pass (for rendering to swapchain image).
	//

	vk::CommandBuffer swapchain_command_buffer;

	const vk::RenderPass swapchain_render_pass = vulkan_swapchain.get_swapchain_render_pass();
	const vk::Framebuffer swapchain_framebuffer = vulkan_swapchain.get_swapchain_framebuffer(swapchain_image_index);

	// Clear colour of the swapchain framebuffer.
	//
	// Note that we clear the colour to (0,0,0,1) and not (0,0,0,0) because we want any parts of
	// the scene, that are not rendered, to have *opaque* alpha (=1). This ensures that if there
	// is any further alpha composition of the framebuffer (see VkCompositeAlphaFlagBitsKHR) then
	// it will have no effect. For example, we won't get our black background converted to white
	// if the window background colour happens to be white.
	const vk::ClearColorValue swapchain_clear_colour = std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f };
	// Clear depth/stencil of the swapchain framebuffer.
	const vk::ClearDepthStencilValue swapchain_clear_depth_stencil = { 1.0f, 0 };

	// Begin swapchain render pass to the current swapchain framebuffer.
	const vk::Rect2D swapchain_render_area = { {0, 0}, swapchain_size };
	const vk::ClearValue swapchain_clear_values[2] = { swapchain_clear_colour, swapchain_clear_depth_stencil };
	vk::RenderPassBeginInfo swapchain_render_pass_begin_info;
	swapchain_render_pass_begin_info
			.setRenderPass(swapchain_render_pass)
			.setFramebuffer(swapchain_framebuffer)
			.setRenderArea(swapchain_render_area)
			.setClearValues(swapchain_clear_values);
	swapchain_command_buffer.beginRenderPass(swapchain_render_pass_begin_info, vk::SubpassContents::eInline);

	// Render the scene.
	//
	// This records Vulkan commands into the swapchain command buffer, and also into other command buffers
	// (eg, that render into textures that are in turn used to render into the swapchain framebuffer).
	d_scene_renderer->render(vulkan_device, *d_scene, *d_scene_overlays, *d_scene_view, viewport, devicePixelRatio());

	// End swapchain render pass.
	swapchain_command_buffer.endRenderPass();


	//
	// Submit our graphics+compute command buffers.
	//

	// Two submission batches.
	vk::SubmitInfo graphics_and_compute_submit_infos[2];

	// Swapchain command buffer submission batch happens next.
	//
	// This is the only batch that writes to the swapchain image.
	// And so it must not write until the swapchain image is ready to render into.
	vk::SubmitInfo &swapchain_command_buffer_submit_info = graphics_and_compute_submit_infos[1];
	// This should be the same as the srcStageMask in the render pass subpass dependency in order to form a dependency *chain*.
	const vk::PipelineStageFlags swapchain_command_buffer_wait_dst_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	swapchain_command_buffer_submit_info
			.setWaitSemaphoreCount(1)
			// Wait until the acquired swapchain image is ready to be rendered into...
			.setPWaitSemaphores(&swapchain_image_available_semaphore)
			// ...and wait at the colour attachment output stage (which writes to the swapchain image)...
			.setPWaitDstStageMask(&swapchain_command_buffer_wait_dst_stage)
			.setCommandBuffers(swapchain_command_buffer);

	// Submit the two submission batches to the graphics+compute queue.
	//
	// And signal the current frame fence when completed
	// (so the command buffers, etc, can be recorded again in a later frame).
	vulkan_device.get_graphics_and_compute_queue().submit(graphics_and_compute_submit_infos, frame_fence);


	//
	// Queue the current swapchain image for presentation.
	//

#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
	// Called *before* queuing a present operation.
	vulkanInstance()->presentAboutToBeQueued(this);
#endif

	// Submit the present operation to the present queue.
	vk::SwapchainKHR swapchain = vulkan_swapchain.get_swapchain();
	vk::PresentInfoKHR present_info;
	present_info
			.setPImageIndices(&swapchain_image_index)
			.setSwapchains(swapchain);
	const vk::Result present_result = vulkan_swapchain.get_present_queue().presentKHR(&present_info);
	if (present_result != vk::Result::eSuccess &&
		// Swapchain no longer matches but we can still present to the surface, so might as well
		// since we've already generated and submitted commands for the current frame...
		present_result != vk::Result::eSuboptimalKHR)
	{
		if (present_result == vk::Result::eErrorOutOfDateKHR)
		{
			// Surface is no longer compatible with the swapchain and we cannot present.
			// So recreate the swapchain and render again.
			vulkan_swapchain.recreate_swapchain(swapchain_size);
			update_canvas();
			return;
		}

		throw GPlatesOpenGL::VulkanException(
				GPLATES_EXCEPTION_SOURCE,
				"Failed to present Vulkan swapchain image.");
	}

	// Called *after* queuing a present operation.
	vulkanInstance()->presentQueued(this);


	//
	// Emit 'repainted' signal.
	//
	emit_repainted();
}


void
GPlatesQtWidgets::GlobeAndMapCanvas::emit_repainted()
{
	const bool mouse_is_down = static_cast<bool>(d_mouse_press_info);
	Q_EMIT repainted(mouse_is_down);
}


void
GPlatesQtWidgets::GlobeAndMapCanvas::mousePressEvent(
		QMouseEvent *press_event) 
{
	// Let's ignore all mouse buttons except the left mouse button.
	if (press_event->button() != Qt::LeftButton)
	{
		return;
	}

	update_mouse_position(press_event);

	d_mouse_press_info =
			MousePressInfo(
					d_mouse_screen_position,
					d_mouse_position_on_map_plane,
					d_mouse_position_on_globe,
					d_mouse_is_on_globe,
					press_event->button(),
					press_event->modifiers());

	if (is_globe_active())
	{
		Q_EMIT mouse_pressed_when_globe_active(
				width(),
				height(),
				d_mouse_press_info->d_mouse_screen_position,
				d_mouse_press_info->d_mouse_position_on_globe,
				d_mouse_press_info->d_mouse_is_on_globe,
				d_mouse_press_info->d_button,
				d_mouse_press_info->d_modifiers);
	}
	else
	{
		Q_EMIT mouse_pressed_when_map_active(
				width(),
				height(),
				d_mouse_press_info->d_mouse_screen_position,
				d_mouse_press_info->d_mouse_map_position,
				d_mouse_press_info->d_mouse_position_on_globe,
				d_mouse_press_info->d_mouse_is_on_globe,
				d_mouse_press_info->d_button,
				d_mouse_press_info->d_modifiers);
	}
}


void
GPlatesQtWidgets::GlobeAndMapCanvas::mouseMoveEvent(
	QMouseEvent *move_event)
{
	update_mouse_position(move_event);

	if (d_mouse_press_info)
	{
		if (is_mouse_in_drag())
		{
			d_mouse_press_info->d_is_mouse_drag = true;
		}

		if (d_mouse_press_info->d_is_mouse_drag)
		{
			if (is_globe_active())
			{
				Q_EMIT mouse_dragged_when_globe_active(
						width(),
						height(),
						d_mouse_press_info->d_mouse_screen_position,
						d_mouse_press_info->d_mouse_position_on_globe,
						d_mouse_press_info->d_mouse_is_on_globe,
						d_mouse_screen_position,
						d_mouse_position_on_globe,
						d_mouse_is_on_globe,
						centre_of_viewport(),
						d_mouse_press_info->d_button,
						d_mouse_press_info->d_modifiers);
			}
			else
			{
				Q_EMIT mouse_dragged_when_map_active(
						width(),
						height(),
						d_mouse_press_info->d_mouse_screen_position,
						d_mouse_press_info->d_mouse_map_position,
						d_mouse_press_info->d_mouse_position_on_globe,
						d_mouse_press_info->d_mouse_is_on_globe,
						d_mouse_screen_position,
						d_mouse_position_on_map_plane,
						d_mouse_position_on_globe,
						d_mouse_is_on_globe,
						centre_of_viewport(),
						d_mouse_press_info->d_button,
						d_mouse_press_info->d_modifiers);
			}
		}
	}
	else
	{
		//
		// The mouse has moved but the left mouse button is not currently pressed.
		// This could mean no mouse buttons are currently pressed or it could mean a
		// button other than the left mouse button is currently pressed.
		// Either way it is an mouse movement that is not currently invoking a
		// canvas tool operation.
		//
		if (is_globe_active())
		{
			Q_EMIT mouse_moved_without_drag_when_globe_active(
					width(),
					height(),
					d_mouse_screen_position,
					d_mouse_position_on_globe,
					d_mouse_is_on_globe,
					centre_of_viewport());
		}
		else
		{
			Q_EMIT mouse_moved_without_drag_when_map_active(
					width(),
					height(),
					d_mouse_screen_position,
					d_mouse_position_on_map_plane,
					d_mouse_position_on_globe,
					d_mouse_is_on_globe,
					centre_of_viewport());
		}
	}
}


void 
GPlatesQtWidgets::GlobeAndMapCanvas::mouseReleaseEvent(
		QMouseEvent *release_event)
{
	// Let's ignore all mouse buttons except the left mouse button.
	if (release_event->button() != Qt::LeftButton)
	{
		return;
	}

	if (!d_mouse_press_info)
	{
		// Somehow we received this left-mouse release event without having first received the
		// corresponding left-mouse press event.
		//
		// Note: With the map view (in older versions of GPlates) a reasonably fast double left mouse click
		//       on the canvas resulted in this (for some reason). However, according to the Qt docs, a
		//       double-click should still produce a mouse press, then release, then a second press
		//       and then a second release.
		return;
	}

	update_mouse_position(release_event);

	if (is_mouse_in_drag())
	{
		d_mouse_press_info->d_is_mouse_drag = true;
	}

	if (d_mouse_press_info->d_is_mouse_drag)
	{
		if (is_globe_active())
		{
			Q_EMIT mouse_released_after_drag_when_globe_active(
					width(),
					height(),
					d_mouse_press_info->d_mouse_screen_position,
					d_mouse_press_info->d_mouse_position_on_globe,
					d_mouse_press_info->d_mouse_is_on_globe,
					d_mouse_screen_position,
					d_mouse_position_on_globe,
					d_mouse_is_on_globe,
					centre_of_viewport(),
					d_mouse_press_info->d_button,
					d_mouse_press_info->d_modifiers);
		}
		else
		{
			Q_EMIT mouse_released_after_drag_when_map_active(
					width(),
					height(),
					d_mouse_press_info->d_mouse_screen_position,
					d_mouse_press_info->d_mouse_map_position,
					d_mouse_press_info->d_mouse_position_on_globe,
					d_mouse_press_info->d_mouse_is_on_globe,
					d_mouse_screen_position,
					d_mouse_position_on_map_plane,
					d_mouse_position_on_globe,
					d_mouse_is_on_globe,
					centre_of_viewport(),
					d_mouse_press_info->d_button,
					d_mouse_press_info->d_modifiers);
		}
	}
	else
	{
		if (is_globe_active())
		{
			Q_EMIT mouse_clicked_when_globe_active(
					width(),
					height(),
					d_mouse_press_info->d_mouse_screen_position,
					d_mouse_press_info->d_mouse_position_on_globe,
					d_mouse_press_info->d_mouse_is_on_globe,
					d_mouse_press_info->d_button,
					d_mouse_press_info->d_modifiers);
		}
		else
		{
			Q_EMIT mouse_clicked_when_map_active(
					width(),
					height(),
					d_mouse_press_info->d_mouse_screen_position,
					d_mouse_press_info->d_mouse_map_position,
					d_mouse_press_info->d_mouse_position_on_globe,
					d_mouse_press_info->d_mouse_is_on_globe,
					d_mouse_press_info->d_button,
					d_mouse_press_info->d_modifiers);
		}
	}

	d_mouse_press_info = boost::none;

	// Emit repainted signal with mouse_down = false so that those listeners who
	// didn't care about intermediate repaints can now deal with the repaint.
	Q_EMIT repainted(false);
}


void
GPlatesQtWidgets::GlobeAndMapCanvas::keyPressEvent(
		QKeyEvent *key_event)
{
	// Note that the arrow keys are handled here instead of being set as shortcuts
	// to the corresponding actions in ViewportWindow because when they were set as
	// shortcuts, they were interfering with the arrow keys on other widgets.
	switch (key_event->key())
	{
		case Qt::Key_Up:
			get_active_camera().pan_up();
			break;

		case Qt::Key_Down:
			get_active_camera().pan_down();
			break;

		case Qt::Key_Left:
			get_active_camera().pan_left();
			break;

		case Qt::Key_Right:
			get_active_camera().pan_right();
			break;

		default:
			VulkanWindow::keyPressEvent(key_event);
	}
}


void
GPlatesQtWidgets::GlobeAndMapCanvas::wheelEvent(
		QWheelEvent *wheel_event)
{
	int delta = wheel_event->angleDelta().y();
	if (delta == 0)
	{
		return;
	}

	// The number 120 is derived from the Qt docs for QWheelEvent:
	// http://doc.trolltech.com/4.3/qwheelevent.html#delta
	static const int NUM_UNITS_PER_STEP = 120;

	double num_levels = static_cast<double>(std::abs(delta)) / NUM_UNITS_PER_STEP;
	if (delta > 0)
	{
		d_scene_view->get_viewport_zoom().zoom_in(num_levels);
	}
	else
	{
		d_scene_view->get_viewport_zoom().zoom_out(num_levels);
	}
}


GPlatesMaths::PointOnSphere
GPlatesQtWidgets::GlobeAndMapCanvas::centre_of_viewport() const
{
	// The point on the globe which corresponds to the centre of the viewport.
	//
	// Note that, for the map view, the map camera look-at position (on map plane) is restricted
	// to be inside the map projection boundary, so this always returns a valid position on the globe.
	return get_active_camera().get_look_at_position_on_globe();
}


bool
GPlatesQtWidgets::GlobeAndMapCanvas::is_mouse_in_drag() const
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_mouse_press_info,
			GPLATES_ASSERTION_SOURCE);

	// Call it a drag if the mouse moved at least 4 pixels in any direction.
	//
	// Otherwise, the user just has shaky hands or a very high-res screen.
	const qreal x_dist = d_mouse_screen_position.x() - d_mouse_press_info->d_mouse_screen_position.x();
	const qreal y_dist = d_mouse_screen_position.y() - d_mouse_press_info->d_mouse_screen_position.y();

	return x_dist*x_dist + y_dist*y_dist > 4;
}


void
GPlatesQtWidgets::GlobeAndMapCanvas::update_mouse_position(
		QMouseEvent *mouse_event)
{
	d_mouse_screen_position = mouse_event->
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
			position()
#else
			localPos()
#endif
			;

	// Note that OpenGL and Qt y-axes are the reverse of each other.
	const double mouse_window_y = height() - d_mouse_screen_position.y();
	const double mouse_window_x = d_mouse_screen_position.x();

	// Get the position on the globe (in the current globe or map view) at the specified window coordinate.
	//
	// If misses the globe (or is outside map boundary) then get the nearest point on the globe horizon
	// visible circumference in globe view (or nearest point on the map projection boundary in map view).
	//
	// Note: 'd_mouse_position_on_map_plane' is set to none when the globe view is active.
	bool is_now_on_globe;
	const GPlatesMaths::PointOnSphere new_position_on_globe =
			d_scene_view->get_position_on_globe_at_window_coord(
					mouse_window_x, mouse_window_y, width(), height(),
					is_now_on_globe, d_mouse_position_on_map_plane);

	// Update if changed.
	if (new_position_on_globe != d_mouse_position_on_globe ||
		is_now_on_globe != d_mouse_is_on_globe)
	{
		d_mouse_position_on_globe = new_position_on_globe;
		d_mouse_is_on_globe = is_now_on_globe;

		Q_EMIT mouse_position_on_globe_changed(d_mouse_position_on_globe, d_mouse_is_on_globe);
	}
}
