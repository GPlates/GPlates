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

#include "VulkanFrame.h"

#include "VulkanDevice.h"
#include "VulkanException.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


GPlatesOpenGL::VulkanFrame::VulkanFrame() :
	d_frame_number(0)
{
}


vk::Fence
GPlatesOpenGL::VulkanFrame::next_frame(
		vk::Device device)
{
	// First increment the frame number (to move onto the next frame).
	++d_frame_number;

	// Make sure the device (GPU) has finished drawing frame "d_frame_number - NUM_ASYNC_FRAMES"
	// so that clients can use that frame's resources (command buffers, etc) for the current frame.
	vk::Fence async_frame_fence = d_async_frame_fences[get_frame_index()];
	const vk::Result wait_for_frame_result = device.waitForFences(async_frame_fence, true, UINT64_MAX);
	if (wait_for_frame_result != vk::Result::eSuccess)
	{
		if (wait_for_frame_result == vk::Result::eErrorDeviceLost)
		{
			return nullptr;
		}

		throw GPlatesOpenGL::VulkanException(
				GPLATES_EXCEPTION_SOURCE,
				"Failed to wait for next Vulkan asynchronous frame.");
	}

	// Reset to the unsignaled state.
	device.resetFences(async_frame_fence);

	return async_frame_fence;
}


void
GPlatesOpenGL::VulkanFrame::initialise_vulkan_resources(
		GPlatesOpenGL::VulkanDevice &vulkan_device)
{
	// Create a fence for each asynchronous frame.
	for (unsigned int frame_index = 0; frame_index < NUM_ASYNC_FRAMES; ++frame_index)
	{
		// Create a fence in the signaled state (so that the first wait on fence will return immediately).
		vk::FenceCreateInfo fence_create_info(vk::FenceCreateFlagBits::eSignaled);
		d_async_frame_fences[frame_index] = vulkan_device.get_device().createFence(fence_create_info);
	}

	d_frame_number = 0;
}


void
GPlatesOpenGL::VulkanFrame::release_vulkan_resources(
		GPlatesOpenGL::VulkanDevice &vulkan_device)
{
	// Destroy the asynchronous frame fences.
	for (unsigned int frame_index = 0; frame_index < NUM_ASYNC_FRAMES; ++frame_index)
	{
		vulkan_device.get_device().destroyFence(d_async_frame_fences[frame_index]);
		d_async_frame_fences[frame_index] = nullptr;
	}
}
