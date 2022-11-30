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

#ifndef GPLATES_OPENGL_VULKANSWAPCHAINFRAME_H
#define GPLATES_OPENGL_VULKANSWAPCHAINFRAME_H

#include <cstdint>
#include <vector>

#include "VulkanFrame.h"
#include "VulkanHpp.h"


namespace GPlatesOpenGL
{
	class VulkanDevice;

	/**
	 * Extends @a VulkanFrame with resources specific to a swapchain.
	 */
	class VulkanSwapchainFrame :
			public VulkanFrame
	{
	public:

		/**
		 * By default, this creates a double-buffered set of resources.
		 */
		explicit
		VulkanSwapchainFrame(
				std::uint32_t num_buffered_frames = 2);


		/**
		 * Semaphore to signal when the acquired swapchain image (for the current frame) is ready to be rendered into.
		 */
		vk::Semaphore
		get_swapchain_image_available_semaphore()
		{
			return get_swapchain_buffered_frame().swapchain_image_available_semaphore;
		}


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

	private:

		struct SwapchainBufferedFrame
		{
			vk::Semaphore swapchain_image_available_semaphore;
		};

		std::vector<SwapchainBufferedFrame> d_swapchain_buffered_frames;


		/**
		 * Returns the @a SwapchainBufferedFrame that corresponds to the current frame index.
		 */
		SwapchainBufferedFrame &
		get_swapchain_buffered_frame();
	};
}

#endif // GPLATES_OPENGL_VULKANSWAPCHAINFRAME_H
