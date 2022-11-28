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
		QWindow *parent_) :
	QWindow(parent_)
{
	setSurfaceType(QSurface::VulkanSurface);
}


GPlatesOpenGL::VulkanDevice &
GPlatesQtWidgets::VulkanWindow::get_vulkan_device()
{
	GPlatesGlobal::Assert<GPlatesOpenGL::VulkanException>(
			d_vulkan_device_and_swapchain,
			GPLATES_ASSERTION_SOURCE,
			"Attempted to query Vulkan device before window first exposed (or lost device).");

	return *d_vulkan_device_and_swapchain->device;
}


GPlatesOpenGL::VulkanSwapchain &
GPlatesQtWidgets::VulkanWindow::get_vulkan_swapchain()
{
	GPlatesGlobal::Assert<GPlatesOpenGL::VulkanException>(
			d_vulkan_device_and_swapchain,
			GPLATES_ASSERTION_SOURCE,
			"Attempted to query Vulkan device before window first exposed (or lost device).");

	return *d_vulkan_device_and_swapchain->swapchain;
}


void
GPlatesQtWidgets::VulkanWindow::device_lost()
{
	// Destroy the device (and swapchain).
	// The next window update will create a new device (and a new swapchain).
	if (d_vulkan_device_and_swapchain)
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
			if (d_vulkan_device_and_swapchain)
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

	if (d_vulkan_device_and_swapchain)
	{
		// If the window size is different than the swapchain size then the window was resized and
		// the swapchain needs to be recreated.
		if (get_swapchain_size() != d_vulkan_device_and_swapchain->swapchain->get_swapchain_size())
		{
			d_vulkan_device_and_swapchain->swapchain->recreate_swapchain(get_swapchain_size());
		}
	}
	else
	{
		// We haven't yet created the Vulkan device and swapchain, so do that now.
		create_vulkan_device_and_swapchain();
	}

	// Ask subclass to render into this window.
	render_to_window(
			*d_vulkan_device_and_swapchain->device,
			*d_vulkan_device_and_swapchain->swapchain);
}


void
GPlatesQtWidgets::VulkanWindow::create_vulkan_device_and_swapchain()
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			!d_vulkan_device_and_swapchain,
			GPLATES_ASSERTION_SOURCE);

	QVulkanInstance *qvulkan_instance = vulkanInstance();

	// Vulkan instance.
	vk::Instance instance = qvulkan_instance->vkInstance();
	// Create (or get if already created) the Vulkan surface for this window.
	vk::SurfaceKHR surface = qvulkan_instance->surfaceForWindow(this);
	GPlatesGlobal::Assert<GPlatesOpenGL::VulkanException>(
			surface,
			GPLATES_ASSERTION_SOURCE,
			"Failed to retrieve Vulkan surface handle from window.");

	// Create the Vulkan device.
	std::uint32_t present_queue_family;
	GPlatesOpenGL::VulkanDevice::non_null_ptr_type vulkan_device =
			GPlatesOpenGL::VulkanDevice::create_for_surface(instance, surface, present_queue_family);

	// Create the Vulkan swapchain.
	GPlatesOpenGL::VulkanSwapchain::non_null_ptr_type vulkan_swapchain =
			GPlatesOpenGL::VulkanSwapchain::create(*vulkan_device, surface, present_queue_family, get_swapchain_size());

	// Keep a record of the Vulkan device and swapchain.
	d_vulkan_device_and_swapchain = VulkanDeviceAndSwapChain{ vulkan_device, vulkan_swapchain };

	// Notify subclass that Vulkan device was created.
	initialise_vulkan_resources(*vulkan_device);
}


void
GPlatesQtWidgets::VulkanWindow::destroy_vulkan_device_and_swapchain()
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_vulkan_device_and_swapchain,
			GPLATES_ASSERTION_SOURCE);

	// First make sure all commands in all queues have finished before we start destroying things.
	//
	// Note: It's OK to wait here since destroying a device/swapchain is not a performance-critical part of the code.
	d_vulkan_device_and_swapchain->device->get_device().waitIdle();

	// Then notify our subclass that the Vulkan device is about to be destroyed.
	release_vulkan_resources(*d_vulkan_device_and_swapchain->device);

	// Finally destroy the Vulkan device and swapchain.
	// Note that the swapchain is destroyed first (and then the device).
	d_vulkan_device_and_swapchain = boost::none;
}
