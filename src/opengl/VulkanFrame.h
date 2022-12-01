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

#include "VulkanHpp.h"


namespace GPlatesOpenGL
{
	class VulkanDevice;

	/**
	 * Manages asynchronous frame rendering used by clients to buffer dynamic resources over two or more frames.
	 *
	 * For example, this enables the host (CPU) to record command buffers for frame N while the device (GPU)
	 * is executing command buffers from the previous frame N-1.
	 */
	class VulkanFrame
	{
	public:

		VulkanFrame();


		/**
		 * Vulkan device was just created.
		 */
		void
		initialise_vulkan_resources(
				VulkanDevice &vulkan_device);

		/**
		 * Vulkan device is about to be destroyed.
		 */
		void
		release_vulkan_resources(
				VulkanDevice &vulkan_device);


		//
		// Asynchronous frame rendering.
		//

		/**
		 * The maximum number of frames that the host (CPU) can record/queue commands ahead of the device (GPU).
		 *
		 * For example, when this value is 2 then the host can record command buffers for frames N-1 and N
		 * while the device is still executing command buffers for frame N-2.
		 *
		 * Note: Each "frame" is determined by a call to @a next_frame.
		 */
		static constexpr unsigned int NUM_ASYNC_FRAMES = 2;

		/**
		 * Increment the frame number and wait for the device (GPU) to finish rendering the frame from
		 * NUM_ASYNC_FRAMES frames ago, or return nullptr if device lost (vk::Result::eErrorDeviceLost).
		 *
		 * For example, if calling @a next_frame increments the frame number to "N" then we wait for
		 * the device (GPU) to finish rendering frame "N - NUM_ASYNC_FRAMES".
		 *
		 * This means clients should buffer NUM_ASYNC_FRAMES worth of dynamic resources to ensure
		 * they do not modify resources that the device (GPU) is still using.
		 * An example is the host (CPU) recording into command buffers that the device (GPU) is still using.
		 *
		 * NOTE: The caller should signal the returned fence when rendering for the frame (N) has finished.
		 *       This can be done by passing it to the final queue submission for the frame (N).
		 */
		vk::Fence
		next_frame(
				vk::Device device);

		/**
		 * The frame *number* is simply incremented at each call to @a next_frame.
		 */
		std::int64_t
		get_frame_number() const
		{
			return d_frame_number;
		}

		/**
		 * The frame *index* is in the range [0, NUM_ASYNC_FRAMES - 1].
		 *
		 * Due to the wait in @a next_frame, the resources at this index are no longer in use
		 * by the device (GPU) and can safely be re-used.
		 *
		 * It's value is "get_frame_number() % NUM_ASYNC_FRAMES" and can be used by clients to index
		 * index their own buffer of resources (eg, an array of size NUM_ASYNC_FRAMES).
		 */
		unsigned int
		get_frame_index() const
		{
			return static_cast<unsigned int>(d_frame_number % NUM_ASYNC_FRAMES);
		}

	private:

		/**
		 * Fences for asynchronous frames.
		 */
		vk::Fence d_async_frame_fences[NUM_ASYNC_FRAMES];
		std::int64_t d_frame_number;
	};
}

#endif // GPLATES_OPENGL_VULKANFRAME_H
