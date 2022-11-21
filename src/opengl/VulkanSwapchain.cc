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

#include "VulkanSwapchain.h"

#include "VulkanDevice.h"


GPlatesOpenGL::VulkanSwapchain::VulkanSwapchain(
		VulkanDevice &vulkan_device,
		vk::SurfaceKHR surface,
		std::uint32_t present_queue_family,
		const QSize &window_size) :
	d_vulkan_device(vulkan_device),
	d_surface(surface),
	d_present_queue_family(present_queue_family),
	d_swapchain_size(window_size)
{
	recreate_swapchain(window_size);
}


GPlatesOpenGL::VulkanSwapchain::~VulkanSwapchain()
{
	if (d_swapchain)
	{
		// First make sure all commands in all queues have finished.
		// This is in case any commands are still operating on an acquired swapchain image.
		//
		// Note: It's OK to wait here since destroying a swapchain is not a performance-critical part of the code.
		d_vulkan_device.get_device().waitIdle();

		d_vulkan_device.get_device().destroySwapchainKHR(d_swapchain);
	}
}


void
GPlatesOpenGL::VulkanSwapchain::recreate_swapchain(
		const QSize &window_size)
{
	// Get the surface capabilities.
	const vk::SurfaceCapabilitiesKHR surface_capabilities =
			d_vulkan_device.get_physical_device().getSurfaceCapabilitiesKHR(d_surface);

	// Get the supported surface formats.
	const std::vector<vk::SurfaceFormatKHR> surface_formats =
			d_vulkan_device.get_physical_device().getSurfaceFormatsKHR(d_surface);

	d_swapchain_size = window_size;

	// The current swapchain.
	//
	// Note: When the swapchain is first created this will be null.
	vk::SwapchainKHR old_swapchain = d_swapchain;

	// Create the new swapchain.
	vk::SwapchainCreateInfoKHR swapchain_info;
	swapchain_info.setOldSwapchain(old_swapchain);
	d_swapchain = d_vulkan_device.get_device().createSwapchainKHR(swapchain_info);

	if (old_swapchain)
	{
		// First make sure all commands in all queues have finished.
		// This is in case any commands are still operating on an acquired swapchain image.
		//
		// Note: It's OK to wait here since recreating a swapchain is not a performance-critical part of the code.
		d_vulkan_device.get_device().waitIdle();

		d_vulkan_device.get_device().destroySwapchainKHR(old_swapchain);
	}
}
