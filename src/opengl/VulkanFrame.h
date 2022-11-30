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

#ifndef GPLATES_OPENGL_VULKANFRAME_H
#define GPLATES_OPENGL_VULKANFRAME_H

#include <cstdint>
#include <vector>

#include "VulkanDeviceLifetime.h"
#include "VulkanHpp.h"


namespace GPlatesOpenGL
{
	class VulkanDevice;

	/**
	 * Manages asynchronous frame rendering by buffering resources over two or more frames.
	 *
	 * For example, this enables the CPU to record command buffers for frame N while the GPU is
	 * executing command buffers from the previous frame N-1.
	 */
	class VulkanFrame :
			public VulkanDeviceLifetime
	{
	public:

		/**
		 * By default, this creates a double-buffered set of resources.
		 */
		explicit
		VulkanFrame(
				std::uint32_t num_buffered_frames = 2);


		/**
		 * Increment the frame index to the next frame.
		 *
		 * This will result in accessing a different set of buffered resources.
		 * For example, this will increment frame N-1 to become frame N, and for double-buffered resources
		 * this will now access buffer resources at index 'N % 2' (for frame N).
		 */
		virtual
		void
		next_frame();


		/**
		 * Begin recording into all command buffers.
		 *
		 * This implicitly resets each command buffer (ie, command pool was created with 'eResetCommandBuffer' flag).
		 * And each command buffer can only be submitted once (ie, each command buffer must be re-recorded each frame).
		 */
		virtual
		void
		begin_command_buffers();

		/**
		 * End recording into all command buffers.
		 */
		virtual
		void
		end_command_buffers();


		/**
		 * Access the command buffer for rendering into the default framebuffer (eg, swapchain)
		 * using the default render pass (with commands allowed within a render pass).
		 */
		vk::CommandBuffer
		get_default_render_pass_command_buffer();


		/**
		 * Semaphore to signal when the GPU has finished rendering a frame.
		 */
		vk::Semaphore
		get_rendering_finished_semaphore();

		/**
		 * Fence to signal when the GPU has finished rendering a frame.
		 *
		 * Note: For double-buffered resources, waiting on this fence will wait for the GPU to
		 *       finish drawing frame N-2 (for current frame N). And if N < 2 then waiting will
		 *       return immediately since fence is initially created in the signaled state.
		 *       When the wait returns the buffered resources are then available for use by the CPU.
		 */
		vk::Fence
		get_rendering_finished_fence();


		/**
		 * Vulkan device was just created.
		 */
		void
		initialise_vulkan_resources(
				VulkanDevice &vulkan_device) override;

		/**
		 * Vulkan device is about to be destroyed.
		 */
		void
		release_vulkan_resources(
				VulkanDevice &vulkan_device) override;

	protected:

		std::uint32_t d_num_buffered_frames;
		std::uint32_t d_frame_index;

	private:

		struct BufferedFrame
		{
			vk::CommandBuffer default_render_pass_command_buffer;
			vk::Semaphore rendering_finished_semaphore;
			vk::Fence rendering_finished_fence;
		};

		vk::CommandPool d_graphics_and_compute_command_pool;

		std::vector<BufferedFrame> d_buffered_frames;
	};
}

#endif // GPLATES_OPENGL_VULKANFRAME_H
