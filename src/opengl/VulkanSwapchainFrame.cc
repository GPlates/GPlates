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

#include "VulkanSwapchainFrame.h"

#include "VulkanDevice.h"
#include "VulkanException.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


GPlatesOpenGL::VulkanSwapchainFrame::VulkanSwapchainFrame(
		std::uint32_t num_buffered_frames) :
	VulkanFrame(num_buffered_frames)
{
}


GPlatesOpenGL::VulkanSwapchainFrame::SwapchainBufferedFrame &
GPlatesOpenGL::VulkanSwapchainFrame::get_swapchain_buffered_frame()
{
	GPlatesGlobal::Assert<VulkanException>(
			d_swapchain_buffered_frames.size() == d_num_buffered_frames,
			GPLATES_ASSERTION_SOURCE,
			"Vulkan swapchain frame not initialised.");

	return d_swapchain_buffered_frames[d_frame_index % d_num_buffered_frames];
}


void
GPlatesOpenGL::VulkanSwapchainFrame::initialise_vulkan_resources(
		GPlatesOpenGL::VulkanDevice &vulkan_device)
{
	// Create/allocate resources for each buffered frame.
	for (std::uint32_t buffered_frame_index = 0; buffered_frame_index < d_num_buffered_frames; ++buffered_frame_index)
	{
		// Create a semaphore.
		vk::SemaphoreCreateInfo semaphore_create_info;
		vk::Semaphore swapchain_image_available_semaphore = vulkan_device.get_device().createSemaphore(semaphore_create_info);

		SwapchainBufferedFrame buffered_frame = { swapchain_image_available_semaphore };
		d_swapchain_buffered_frames.push_back(buffered_frame);
	}

	// Call base class method.
	VulkanFrame::initialise_vulkan_resources(vulkan_device);
}


void
GPlatesOpenGL::VulkanSwapchainFrame::release_vulkan_resources(
		GPlatesOpenGL::VulkanDevice &vulkan_device)
{
	// Destroy/free resources for each buffered frame.
	for (auto &buffered_frame : d_swapchain_buffered_frames)
	{
		// Destroy semaphore.
		vulkan_device.get_device().destroySemaphore(buffered_frame.swapchain_image_available_semaphore);
	}

	d_swapchain_buffered_frames.clear();

	// Call base class method.
	VulkanFrame::release_vulkan_resources(vulkan_device);
}
