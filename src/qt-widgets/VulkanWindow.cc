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
#include "opengl/VulkanHpp.h"


GPlatesQtWidgets::VulkanWindow::VulkanWindow(
		QWindow *parent_) :
	QWindow(parent_)
{
	setSurfaceType(QSurface::VulkanSurface);
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
		// Create the Vulkan device (if we haven't already).
		if (!d_vulkan_device)
		{
			create_vulkan_device();
		}

		// Create the Vulkan swapchain (if we haven't already).
		if (!d_vulkan_swapchain)
		{
			create_vulkan_swapchain();
		}

		render_to_window(*d_vulkan_device.get());
		break;

	case QEvent::PlatformSurface:
		// Vulkan requires the swapchain be destroyed before the surface.
		if (static_cast<QPlatformSurfaceEvent*>(event)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed)
		{
			destroy_vulkan_swapchain();
			destroy_vulkan_device();
		}
		break;

	default:
		break;
	}

	return QWindow::event(event);
}


void
GPlatesQtWidgets::VulkanWindow::create_vulkan_device()
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			!d_vulkan_device,
			GPLATES_ASSERTION_SOURCE);

	QVulkanInstance *qvulkan_instance = vulkanInstance();

	// Vulkan instance.
	vk::Instance instance = qvulkan_instance->vkInstance();
	// Create or get the Vulkan surface for this window.
	vk::SurfaceKHR surface = qvulkan_instance->surfaceForWindow(this);

	// Create the Vulkan device.
	std::uint32_t present_queue_family;
	d_vulkan_device = GPlatesOpenGL::VulkanDevice::create_for_surface(instance, surface, present_queue_family);

	// Notify subclass that Vulkan device was created.
	initialise_vulkan_resources(*d_vulkan_device.get());
}


void
GPlatesQtWidgets::VulkanWindow::destroy_vulkan_device()
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_vulkan_device,
			GPLATES_ASSERTION_SOURCE);

	// Notify subclass that Vulkan device is about to be destroyed.
	release_vulkan_resources(*d_vulkan_device.get());

	d_vulkan_device = boost::none;
}


void
GPlatesQtWidgets::VulkanWindow::create_vulkan_swapchain()
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			!d_vulkan_swapchain,
			GPLATES_ASSERTION_SOURCE);

	d_vulkan_swapchain = GPlatesOpenGL::VulkanSwapchain::create();
}


void
GPlatesQtWidgets::VulkanWindow::destroy_vulkan_swapchain()
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_vulkan_swapchain,
			GPLATES_ASSERTION_SOURCE);

	d_vulkan_swapchain = boost::none;
}
