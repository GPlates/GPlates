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
#include "global/config.h" // For GPLATES_USE_VULKAN_BACKEND
#if defined(GPLATES_USE_VULKAN_BACKEND)
#	include <QVulkanFunctions>
#	include <QVulkanInstance>
#endif

#include "GlobeAndMapCanvas.h"

#include "app-logic/ApplicationState.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/GPlatesException.h"

#include "gui/Colour.h"
#include "gui/GlobeCamera.h"
#include "gui/MapCamera.h"
#include "gui/MapProjection.h"
#include "gui/ViewportZoom.h"

#include "maths/MathsUtils.h"

#include "opengl/GLContext.h"
#include "opengl/GLViewport.h"
#if defined(GPLATES_USE_VULKAN_BACKEND)
#	include "opengl/Vulkan.h"
#	include "opengl/VulkanException.h"
#endif

#include "presentation/ViewState.h"


GPlatesQtWidgets::GlobeAndMapCanvas::GlobeAndMapCanvas(
		GPlatesPresentation::ViewState &view_state) :
#if defined(GPLATES_USE_VULKAN_BACKEND)
	QVulkanWindow(),
#else
	QOpenGLWindow(),
#endif
	d_gl_context(GPlatesOpenGL::GLContext::create()),
	d_initialised_gl(false),
#if defined(GPLATES_USE_VULKAN_BACKEND)
	d_vulkan_compute_queue_family_index(0),
#endif
	d_scene(GPlatesGui::Scene::create(view_state, devicePixelRatio())),
	d_scene_view(GPlatesGui::SceneView::create(view_state)),
	d_scene_overlays(GPlatesGui::SceneOverlays::create(view_state)),
	d_scene_renderer(GPlatesGui::SceneRenderer::create(view_state)),
	// The following unit-vector initialisation value is arbitrary.
	d_mouse_position_on_globe(GPlatesMaths::UnitVector3D(1, 0, 0)),
	d_mouse_is_on_globe(false)
{
#if defined(GPLATES_USE_VULKAN_BACKEND)

	using namespace boost::placeholders;  // For _1, _2, etc

	// Set the Vulkan instance in this QVulkanWindow.
	//
	// We do this first so that we then subsequently access 'vulkanInstance()' on 'this' QVulkanWindow.
	setVulkanInstance(&view_state.get_application_state().get_vulkan_instance());

	// Set PersistentResources flag on our QVulkanWindow.
	//
	// This is needed for off-screen rendering (eg, 'render_to_image()').
	// If disabled then we won't be able to use any Vulkan resources at all when the window is un-exposed.
	// Because when window is un-exposed it will call QVulkanWindowRenderer::releaseResources() which means,
	// among other things, we can't even call QVulkanWindow::device() to get the VkDevice to do offscreen rendering.
	setFlags(PersistentResources);

	// Choose the index of the VkPhysicalDevice that has a queue family supporting graphics and compute.
	set_vulkan_physical_device_index();

	// Ensure that either this QVulkanWindow's 'graphics' queue (yet to be created) supports 'compute' or
	// request creation of an extra queue from a 'compute' queue family (when the logical device is created).
	setQueueCreateInfoModifier(boost::bind(&GlobeAndMapCanvas::vulkan_queue_create_info_modifier, this, _1, _2, _3));

#endif

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
#if !defined(GPLATES_USE_VULKAN_BACKEND)
	// Make sure the OpenGL context is current, and then initialise OpenGL if we haven't already.
	//
	// Note: We're not called from 'paintEvent()' so we can't be sure the OpenGL context is current.
	//       And we also don't know if 'initializeGL()' has been called yet.
	makeCurrent();
#endif
	if (!d_initialised_gl)
	{
		initialize_gl();
	}

	// Start a render scope (all GL calls should be done inside this scope).
	//
	// NOTE: Before calling this, OpenGL should be in the default OpenGL state.
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
#if defined(GPLATES_USE_VULKAN_BACKEND)
	requestUpdate();
#else
	update();
#endif
}


#if !defined(GPLATES_USE_VULKAN_BACKEND)

void
GPlatesQtWidgets::GlobeAndMapCanvas::initializeGL() 
{
	// Initialise OpenGL.
	//
	// Note: The OpenGL context is already current.
	initialize_gl();
}

#endif


void 
GPlatesQtWidgets::GlobeAndMapCanvas::initialize_gl()
{
#if !defined(GPLATES_USE_VULKAN_BACKEND)
	// Call 'shutdown_gl()' when the QOpenGLContext is about to be destroyed.
	QObject::connect(
			context(), SIGNAL(aboutToBeDestroyed()),
			this, SLOT(shutdown_gl()));
#endif

	// A new VkDevice just got created, so create a new Vulkan object used to render.
	d_vulkan = GPlatesOpenGL::Vulkan::create(
			vulkanInstance()->vkInstance(),
			physicalDevice(),
			device());

	// Initialise our context-like object first.
	d_gl_context->initialise_gl(*this);

#if !defined(GPLATES_USE_VULKAN_BACKEND)
	// Start a render scope (all GL calls should be done inside this scope).
	//
	// NOTE: Before calling this, OpenGL should be in the default OpenGL state.
	GPlatesOpenGL::GL::non_null_ptr_type gl = d_gl_context->access_opengl();

	// NOTE: We should not perform any operation that affects the default framebuffer (such as 'gl.Clear()')
	//       because it's possible the default framebuffer (associated with this QOpenGLWindow) is not yet
	//       set up correctly despite its OpenGL context being the current rendering context.

	// Initialise the scene.
	d_scene->initialise_gl(*gl);

	// Initialise the scene renderer.
	d_scene_renderer->initialise_gl(*gl);
#endif

	// 'initializeGL()' should only be called once.
	d_initialised_gl = true;
}


void 
GPlatesQtWidgets::GlobeAndMapCanvas::shutdown_gl() 
{
#if !defined(GPLATES_USE_VULKAN_BACKEND)
	// Start a render scope (all GL calls should be done inside this scope).
	//
	// NOTE: Before calling this, OpenGL should be in the default OpenGL state.
	{
		GPlatesOpenGL::GL::non_null_ptr_type gl = d_gl_context->access_opengl();

		// Shutdown the scene.
		d_scene->shutdown_gl(*gl);

		// Shutdown the scene renderer.
		d_scene_renderer->shutdown_gl(*gl);
	}
#endif

	// Shutdown our context-like object last.
	d_gl_context->shutdown_gl();

	// Our VkDevice is about to be destroyed, so destroy our Vulkan object used to render.
	d_vulkan = boost::none;

	d_initialised_gl = false;

#if !defined(GPLATES_USE_VULKAN_BACKEND)
	// Disconnect 'shutdown_gl()' now that the QOpenGLContext is about to be destroyed.
	QObject::disconnect(
			context(), SIGNAL(aboutToBeDestroyed()),
			this, SLOT(shutdown_gl()));
#endif
}


#if defined(GPLATES_USE_VULKAN_BACKEND)

void
GPlatesQtWidgets::GlobeAndMapCanvas::set_vulkan_physical_device_index()
{
	// Get the Vulkan instance.
	//
	// Note: We don't access 'd_vulkan' since it has not been initialised yet.
	vk::Instance vulkan_instance = vulkanInstance()->vkInstance();

	// Get the physical devices.
	const std::vector<vk::PhysicalDevice> physical_devices = vulkan_instance.enumeratePhysicalDevices();

	// Find candidate physical devices with a queue family supporting both graphics and compute.
	//
	// According to the Vulkan spec...
	//   "If an implementation exposes any queue family that supports graphics operations, at least one queue family of
	//    at least one physical device exposed by the implementation must support both graphics and compute operations."
	//
	std::vector<uint32_t> candidate_physical_device_indices;
	for (uint32_t physical_device_index = 0; physical_device_index < physical_devices.size(); ++physical_device_index)
	{
		// Get the queue family properties.
		const std::vector<vk::QueueFamilyProperties> queue_family_properties_seq =
				physical_devices[physical_device_index].getQueueFamilyProperties();

		// See if any queue family supports both graphics and compute.
		for (const auto &queue_family_properties : queue_family_properties_seq)
		{
			if ((queue_family_properties.queueFlags & vk::QueueFlagBits::eGraphics) &&
				(queue_family_properties.queueFlags & vk::QueueFlagBits::eCompute))
			{
				candidate_physical_device_indices.push_back(physical_device_index);
				break;
			}
		}
	}

	// Vulkan implementation should have a physical device with a queue family supporting graphics and compute.
	GPlatesGlobal::Assert<GPlatesOpenGL::VulkanException>(
			!candidate_physical_device_indices.empty(),
			GPLATES_ASSERTION_SOURCE,
			"Failed to find a physical device with a queue family supporting both graphics and compute.");

	// Choose a 'discrete' GPU if found.
	boost::optional<uint32_t> selected_physical_device_index;
	for (uint32_t physical_device_index : candidate_physical_device_indices)
	{
		const vk::PhysicalDeviceProperties physical_device_properties =
				physical_devices[physical_device_index].getProperties();

		if (physical_device_properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
		{
			selected_physical_device_index = physical_device_index;
			break;
		}
	}

	// Discrete GPU not found, so just choose the first candidate physical device.
	if (!selected_physical_device_index)
	{
		selected_physical_device_index = candidate_physical_device_indices.front();
	}

	// Set the desired physical device index in 'this' QVulkanWindow.
	setPhysicalDeviceIndex(selected_physical_device_index.get());
}


void
GPlatesQtWidgets::GlobeAndMapCanvas::vulkan_queue_create_info_modifier(
		const VkQueueFamilyProperties *queue_family_properties_seq,
		uint32_t queue_count,
		vulkan_queue_create_info_seq_type &queue_create_infos)
{
	// First search the existing queue creation infos to see if one of the queues being created by
	// 'this' QVulkanWindow supports 'compute' (it's likely the 'graphics' queue also supports 'compute').
	for (const VkDeviceQueueCreateInfo &queue_create_info : queue_create_infos)
	{
		if ((queue_family_properties_seq[queue_create_info.queueFamilyIndex].queueFlags & VK_QUEUE_COMPUTE_BIT))
		{
			// The current queue (to be created) supports 'compute'.
			// So we don't need to create an extra 'compute' queue.
			//
			// Specify which queue family to obtain (yet to be created) 'compute' queue from.
			d_vulkan_compute_queue_family_index = queue_create_info.queueFamilyIndex;
			return;
		}
	}

	// Next search the queue families for a family that supports 'compute'.
	boost::optional<uint32_t> compute_queue_family_index;
	for (uint32_t queue_family_index = 0; queue_family_index < queue_count; ++queue_family_index)
	{
		const VkQueueFamilyProperties &queue_family_properties = queue_family_properties_seq[queue_family_index];

		if ((queue_family_properties.queueFlags & VK_QUEUE_COMPUTE_BIT))
		{
			// The current queue family supports 'compute'.
			compute_queue_family_index = queue_family_index;
			break;
		}
	}

	// We previously selected the physical device with a queue family supporting graphics and compute.
	// So we should be able to find a queue family supporting compute.
	GPlatesGlobal::Assert<GPlatesOpenGL::VulkanException>(
			compute_queue_family_index,
			GPLATES_ASSERTION_SOURCE,
			"Failed to find a queue family supporting compute.");

	// Specify the compute queue create info.
	const vk::DeviceQueueCreateInfo compute_queue_create_info =
	{
		{}, // flags
		compute_queue_family_index.get(), // queueFamilyIndex
		1, // queueCount
		{ 0 } // pQueuePriorities
	};

	// Add the compute queue create info to the caller's list.
	// The compute queue will get created when the logical device (VkDevice) is created.
	queue_create_infos.append(
			static_cast<const VkDeviceQueueCreateInfo &>(compute_queue_create_info));

	// Specify which queue family to obtain (yet to be created) 'compute' queue from.
	d_vulkan_compute_queue_family_index = compute_queue_family_index.get();
}

#endif


void
#if defined(GPLATES_USE_VULKAN_BACKEND)
GPlatesQtWidgets::GlobeAndMapCanvas::paint_gl()
#else
GPlatesQtWidgets::GlobeAndMapCanvas::paintGL()
#endif
{
#if !defined(GPLATES_USE_VULKAN_BACKEND)

	// Start a render scope (all GL calls should be done inside this scope).
	//
	// NOTE: Before calling this, OpenGL should be in the default OpenGL state.
	GPlatesOpenGL::GL::non_null_ptr_type gl = d_gl_context->access_opengl();

	// The viewport is in device pixels since that is what OpenGL will use to render the scene.
	const GPlatesOpenGL::GLViewport viewport(0, 0,
			width() * devicePixelRatio(),
			height() * devicePixelRatio());

	// Clear colour buffer of the main framebuffer.
	//
	// Note that we clear the colour to (0,0,0,1) and not (0,0,0,0) because we want any parts of
	// the scene, that are not rendered, to have *opaque* alpha (=1). This appears to be needed on
	// Mac with Qt5 (alpha=0 is fine on Qt5 Windows/Ubuntu, and on Qt4 for all platforms). Perhaps because
	// QGLWidget rendering (on Qt5 Mac) is first done to a framebuffer object which is then blended into the
	// window framebuffer (where having a source alpha of zero would result in the black background not showing).
	// Or, more likely, maybe a framebuffer object is used on all platforms but the window framebuffer is white on Mac
	// but already black on Windows/Ubuntu. That was using a QOpenGLWidget and we turned off background rendering with
	// "setAutoFillBackground(false)" and "setAttribute(Qt::WA_NoSystemBackground)"). Now we use QOpenGLWindow
	// which does not support those features, so it may no longer be an issue.
	const GPlatesGui::Colour clear_colour(0, 0, 0, 1);


	// Render the scene into the canvas.
	d_scene_renderer->render(*gl, *d_scene, *d_scene_overlays, *d_scene_view, viewport, clear_colour, devicePixelRatio());

#else

	QVulkanDeviceFunctions *device_functions = vulkanInstance()->deviceFunctions(device());

	VkClearColorValue clearColor = { { 1.0f, 0.0f, 0.0f, 1.0f } };
	VkClearDepthStencilValue clearDS = { 1.0f, 0 };
	VkClearValue clearValues[3];
	memset(clearValues, 0, sizeof(clearValues));
	clearValues[0].color = clearValues[2].color = clearColor;
	clearValues[1].depthStencil = clearDS;

	VkRenderPassBeginInfo rpBeginInfo;
	memset(&rpBeginInfo, 0, sizeof(rpBeginInfo));
	rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpBeginInfo.renderPass = defaultRenderPass();
	rpBeginInfo.framebuffer = currentFramebuffer();
	rpBeginInfo.renderArea.extent.width = swapChainImageSize().width();
	rpBeginInfo.renderArea.extent.height = swapChainImageSize().height();
	rpBeginInfo.clearValueCount = sampleCountFlagBits() > VK_SAMPLE_COUNT_1_BIT ? 3 : 2;
	rpBeginInfo.pClearValues = clearValues;
	device_functions->vkCmdBeginRenderPass(currentCommandBuffer(), &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	device_functions->vkCmdEndRenderPass(currentCommandBuffer());

#endif
}


#if !defined(GPLATES_USE_VULKAN_BACKEND)

void
GPlatesQtWidgets::GlobeAndMapCanvas::paintEvent(
		QPaintEvent *paint_event)
{
	QOpenGLWindow::paintEvent(paint_event);

	emit_repainted();
}

#endif


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
#if defined(GPLATES_USE_VULKAN_BACKEND)
			QVulkanWindow
#else
			QOpenGLWindow
#endif
				::keyPressEvent(key_event);
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
