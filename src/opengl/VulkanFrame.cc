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


GPlatesOpenGL::VulkanFrame::VulkanFrame(
		std::uint32_t num_buffered_frames) :
	d_num_buffered_frames(num_buffered_frames),
	d_frame_index(0)
{
}


void
GPlatesOpenGL::VulkanFrame::next_frame()
{
	++d_frame_index;
}


void
GPlatesOpenGL::VulkanFrame::begin_command_buffers()
{
	BufferedFrame &buffered_frame = get_buffered_frame();

	// Each command buffer will only be submitted once.
	vk::CommandBufferBeginInfo command_buffer_begin_info;
	command_buffer_begin_info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

	// Begin command buffer for default render pass.
	buffered_frame.default_render_pass_command_buffer.begin(command_buffer_begin_info);
}


void
GPlatesOpenGL::VulkanFrame::end_command_buffers()
{
	BufferedFrame &buffered_frame = get_buffered_frame();

	// End command buffer for default render pass.
	buffered_frame.default_render_pass_command_buffer.end();
}


GPlatesOpenGL::VulkanFrame::BufferedFrame &
GPlatesOpenGL::VulkanFrame::get_buffered_frame()
{
	GPlatesGlobal::Assert<VulkanException>(
			d_buffered_frames.size() == d_num_buffered_frames,
			GPLATES_ASSERTION_SOURCE,
			"Vulkan frame not initialised.");

	return d_buffered_frames[d_frame_index % d_num_buffered_frames];
}


void
GPlatesOpenGL::VulkanFrame::initialise_vulkan_resources(
		GPlatesOpenGL::VulkanDevice &vulkan_device)
{
	// Create the graphics+compute command pool.
	//
	// Note that we not creating a separate command pool for each buffered frame.
	// Each command buffer will tend to be used for a specific purpose (eg, default command buffer
	// used to render into default framebuffer) and so will have consistent memory allocations (from pool).
	vk::CommandPoolCreateInfo graphics_and_compute_command_pool_create_info;
	graphics_and_compute_command_pool_create_info
			// Each command buffer can be reset (eg, implictly by beginning a command buffer).
			// And each command buffer will not be re-used (ie, used once and then reset)...
			.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient)
			.setQueueFamilyIndex(vulkan_device.get_graphics_and_compute_queue_family());
	d_graphics_and_compute_command_pool = vulkan_device.get_device().createCommandPool(graphics_and_compute_command_pool_create_info);

	// Allocate a default graphics+compute command buffer for each buffered frame.
	vk::CommandBufferAllocateInfo default_graphics_and_compute_command_buffer_allocate_info;
	default_graphics_and_compute_command_buffer_allocate_info
			.setCommandPool(d_graphics_and_compute_command_pool)
			.setLevel(vk::CommandBufferLevel::ePrimary)
			.setCommandBufferCount(d_num_buffered_frames);
	const std::vector<vk::CommandBuffer> default_graphics_and_compute_command_buffers =
			vulkan_device.get_device().allocateCommandBuffers(default_graphics_and_compute_command_buffer_allocate_info);
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			default_graphics_and_compute_command_buffers.size() == d_num_buffered_frames,
			GPLATES_ASSERTION_SOURCE);

	// Create/allocate resources for each buffered frame.
	for (std::uint32_t buffered_frame_index = 0; buffered_frame_index < d_num_buffered_frames; ++buffered_frame_index)
	{
		// Get a previously allocated command buffer.
		vk::CommandBuffer default_graphics_and_compute_command_buffer = default_graphics_and_compute_command_buffers[buffered_frame_index];

		// Create a semaphore.
		vk::SemaphoreCreateInfo semaphore_create_info;
		vk::Semaphore rendering_finished_semaphore = vulkan_device.get_device().createSemaphore(semaphore_create_info);

		// Create a fence in the signaled state.
		vk::FenceCreateInfo fence_create_info(vk::FenceCreateFlagBits::eSignaled);
		vk::Fence rendering_finished_fence = vulkan_device.get_device().createFence(fence_create_info);

		BufferedFrame buffered_frame = { default_graphics_and_compute_command_buffer, rendering_finished_semaphore, rendering_finished_fence };
		d_buffered_frames.push_back(buffered_frame);
	}

	d_frame_index = 0;
}


void
GPlatesOpenGL::VulkanFrame::release_vulkan_resources(
		GPlatesOpenGL::VulkanDevice &vulkan_device)
{
	// Destroy/free resources for each buffered frame.
	for (auto &buffered_frame : d_buffered_frames)
	{
		// Free command buffer.
		vulkan_device.get_device().freeCommandBuffers(
				d_graphics_and_compute_command_pool,
				buffered_frame.default_render_pass_command_buffer);

		// Destroy semaphore.
		vulkan_device.get_device().destroySemaphore(buffered_frame.rendering_finished_semaphore);

		// Destroy fence.
		vulkan_device.get_device().destroyFence(buffered_frame.rendering_finished_fence);
	}

	// Destroy the graphics+compute command pool.
	vulkan_device.get_device().destroyCommandPool(d_graphics_and_compute_command_pool);

	d_buffered_frames.clear();
}
