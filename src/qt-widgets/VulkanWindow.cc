/**
 * Copyright (C) 2022 The University of Sydney, Australia
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

#include <QPlatformSurfaceEvent>
#include <QVulkanInstance>

#include "VulkanWindow.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "opengl/VulkanDevice.h"
#include "opengl/VulkanException.h"


GPlatesQtWidgets::VulkanWindow::VulkanWindow(
		QVulkanInstance &qvulkan_instance,
		QWindow *parent_) :
	QWindow(parent_),
	d_vulkan_device(qvulkan_instance.vkInstance())
{
	// Set surface type to Vulkan.
	setSurfaceType(QSurface::VulkanSurface);

	// Set the Vulkan instance in this QWindow.
	//
	// We can then subsequently access 'vulkanInstance()' on 'this' QWindow.
	setVulkanInstance(&qvulkan_instance);
}


GPlatesOpenGL::VulkanDevice &
GPlatesQtWidgets::VulkanWindow::get_vulkan_device()
{
	GPlatesGlobal::Assert<GPlatesOpenGL::VulkanException>(
			d_vulkan_device.get_device(),
			GPLATES_ASSERTION_SOURCE,
			"Attempted to query Vulkan device before window first exposed (or lost device).");

	return d_vulkan_device;
}


GPlatesOpenGL::VulkanSwapchain &
GPlatesQtWidgets::VulkanWindow::get_vulkan_swapchain()
{
	GPlatesGlobal::Assert<GPlatesOpenGL::VulkanException>(
			d_vulkan_swapchain.get_swapchain(),
			GPLATES_ASSERTION_SOURCE,
			"Attempted to query Vulkan swapchain before window first exposed (or lost device).");

	return d_vulkan_swapchain;
}


void
GPlatesQtWidgets::VulkanWindow::device_lost()
{
	// Destroy the device (and swapchain).
	// The next window update will create a new device (and a new swapchain).
	if (d_vulkan_device.get_device())
	{
		// Note that this first waits for the GPU to be idle, then notifies subclass to
		// release resources and finally destroys the device/swapchain.
		destroy_vulkan_device_and_swapchain();
	}
}


void
GPlatesQtWidgets::VulkanWindow::exposeEvent(
		QExposeEvent *expose_event)
{
	if (isExposed())
	{
		// Render a frame.
		requestUpdate();
	}
}


bool
GPlatesQtWidgets::VulkanWindow::event(
        QEvent *event)
{
	switch (event->type())
	{
	case QEvent::UpdateRequest:
		update_window();
		break;

	case QEvent::PlatformSurface:
		// Vulkan requires the swapchain be destroyed before the surface.
		if (static_cast<QPlatformSurfaceEvent*>(event)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed)
		{
			if (d_vulkan_device.get_device())
			{
				destroy_vulkan_device_and_swapchain();
			}
		}
		break;

	default:
		break;
	}

	return QWindow::event(event);
}


void
GPlatesQtWidgets::VulkanWindow::update_window()
{
	// Return early (without rendering to window) if the window area is zero (eg, minimized).
	//
	// The Vulkan spec also states that a swapchain cannot be created when the window size is (0, 0),
	// until the size changes.
	if (size().isEmpty())
	{
		return;
	}

	if (!d_vulkan_device.get_device())
	{
		// We don't have a Vulkan device so we shouldn't have a swapchain either.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				!d_vulkan_swapchain.get_swapchain(),
				GPLATES_ASSERTION_SOURCE);

		// We haven't yet created the Vulkan device and swapchain, so do that now.
		create_vulkan_device_and_swapchain();
	}

	// Should now have both a Vulkan device and a swapchain.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_vulkan_device.get_device() && d_vulkan_swapchain.get_swapchain(),
			GPLATES_ASSERTION_SOURCE);

	// If the window size is different than the swapchain size then the window was resized and
	// the swapchain needs to be recreated.
	if (get_window_size_in_device_pixels() != d_vulkan_swapchain.get_swapchain_size())
	{
		d_vulkan_swapchain.recreate(d_vulkan_device, get_window_size_in_device_pixels());
	}

	// Ask subclass to render into this window.
	render_to_window(d_vulkan_device, d_vulkan_swapchain);
}


void
GPlatesQtWidgets::VulkanWindow::create_vulkan_device_and_swapchain()
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			!d_vulkan_device.get_device() && !d_vulkan_swapchain.get_swapchain(),
			GPLATES_ASSERTION_SOURCE);

	// Create (or get if already created) the Vulkan surface for this window.
	vk::SurfaceKHR surface = vulkanInstance()->surfaceForWindow(this);
	GPlatesGlobal::Assert<GPlatesOpenGL::VulkanException>(
			surface,
			GPLATES_ASSERTION_SOURCE,
			"Failed to retrieve Vulkan surface handle from window.");

	// Create the Vulkan device.
	std::uint32_t present_queue_family;
	d_vulkan_device.create_for_surface(
			surface,
			present_queue_family);

	// Use MSAA unless the device-pixel-ratio is greater than 1.0
	// (because then the pixels are so small that we effectively already have anti-aliasing).
	vk::SampleCountFlagBits sample_count = vk::SampleCountFlagBits::e1;
	if (devicePixelRatio() == 1)
	{
		// Use 4xMSAA since Vulkan requires its support for colour and depth/stencil attachments.
		// And 4xMSAA is a good quality/performance trade-off.
		sample_count = vk::SampleCountFlagBits::e4;
	}

	// Create the Vulkan swapchain.
	d_vulkan_swapchain.create(
			d_vulkan_device,
			surface,
			present_queue_family,
			get_window_size_in_device_pixels(),
			sample_count,
			true/*create_depth_stencil_attachment*/);

	// Notify subclass that Vulkan device was created.
	initialise_vulkan_resources(d_vulkan_device, d_vulkan_swapchain);
}


void
GPlatesQtWidgets::VulkanWindow::destroy_vulkan_device_and_swapchain()
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_vulkan_device.get_device() && d_vulkan_swapchain.get_swapchain(),
			GPLATES_ASSERTION_SOURCE);

	// First make sure all commands in all queues have finished before we ask clients to start destroying things.
	//
	// Note: It's OK to wait here since destroying a device/swapchain is not a performance-critical part of the code.
	d_vulkan_device.get_device().waitIdle();

	// Then notify our subclass that the Vulkan device is about to be destroyed.
	release_vulkan_resources(d_vulkan_device, d_vulkan_swapchain);

	// Finally destroy the Vulkan device and swapchain.
	// Note that the swapchain is destroyed first (and then the device).
	d_vulkan_swapchain.destroy(d_vulkan_device);
	d_vulkan_device.destroy();
}
