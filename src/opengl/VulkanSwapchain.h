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

#ifndef GPLATES_OPENGL_VULKANSWAPCHAIN_H
#define GPLATES_OPENGL_VULKANSWAPCHAIN_H

#include <cstdint>
#include <vector>

#include "VulkanHpp.h"

#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	class VulkanDevice;

	/**
	 * Vulkan swapchain and the presentation queue.
	 */
	class VulkanSwapchain :
			public GPlatesUtils::ReferenceCount<VulkanSwapchain>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<VulkanSwapchain> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const VulkanSwapchain> non_null_ptr_to_const_type;


		/**
		 * Create a Vulkan swapchain.
		 *
		 * NOTE: The lifetime of the returned @a VulkanSwapchain must not be longer
		 *       than the lifetime of @a vulkan_device (because it's used in our destructor).
		 */
		static
		non_null_ptr_type
		create(
				VulkanDevice &vulkan_device,
				vk::SurfaceKHR surface,
				std::uint32_t present_queue_family,
				const vk::Extent2D &swapchain_size)
		{
			return non_null_ptr_type(new VulkanSwapchain(vulkan_device, surface, present_queue_family, swapchain_size));
		}


		~VulkanSwapchain();


		/**
		 * Return the present queue family.
		 *
		 * This may or may not be the same as the graphics+compute queue family in @a VulkanDevice.
		 */
		std::uint32_t
		get_present_queue_family() const
		{
			return d_present_queue_family;
		}

		/**
		 * Return the present queue.
		 *
		 * This may or may not be the same as the graphics+compute queue in @a VulkanDevice.
		 * It is the same only if the present and graphics+compute queue *family* are the same.
		 */
		vk::Queue
		get_present_queue()
		{
			return d_present_queue;
		}


		/**
		 * Returns the swapchain.
		 */
		vk::SwapchainKHR
		get_swapchain() const
		{
			return d_swapchain;
		}

		/**
		 * Returns the image format of the swapchain images.
		 */
		vk::Format
		get_swapchain_image_format() const
		{
			return d_swapchain_image_format;
		}

		/**
		 * Returns the current swapchain size (in device pixels).
		 */
		vk::Extent2D
		get_swapchain_size() const
		{
			return d_swapchain_size;
		}

		/**
		 * Returns the render pass to use when rendering to a swapchain image.
		 */
		vk::RenderPass
		get_swapchain_render_pass() const
		{
			return d_render_pass;
		}

		/**
		 * Returns the number of swapchain images.
		 */
		std::uint32_t
		get_num_swapchain_images() const
		{
			return d_render_targets.size();
		}

		/**
		 * Returns the swapchain image at specified swapchain image index.
		 */
		vk::Image
		get_swapchain_image(
				std::uint32_t swapchain_image_index) const;

		/**
		 * Returns the swapchain image view at specified swapchain image index.
		 */
		vk::ImageView
		get_swapchain_image_view(
				std::uint32_t swapchain_image_index) const;

		/**
		 * Returns the swapchain framebuffer at specified swapchain image index.
		 */
		vk::Framebuffer
		get_swapchain_framebuffer(
				std::uint32_t swapchain_image_index) const;

		/**
		 * Re-create the vk::SwapchainKHR, and its associate image views and framebuffers.
		 *
		 * This is useful when the surface window is resized.
		 */
		void
		recreate_swapchain(
				const vk::Extent2D &swapchain_size);

	private:

		struct RenderTarget
		{
			vk::Image image;
			vk::ImageView image_view;
			vk::Framebuffer framebuffer;
		};

		VulkanDevice &d_vulkan_device;

		vk::SurfaceKHR d_surface;
		std::uint32_t d_present_queue_family;
		vk::Queue d_present_queue;

		vk::SwapchainKHR d_swapchain;
		vk::Format d_swapchain_image_format;
		vk::Extent2D d_swapchain_size;

		vk::RenderPass d_render_pass;
		//! Each swapchain image has an associated framebuffer.
		std::vector<RenderTarget> d_render_targets;

		VulkanSwapchain(
				VulkanDevice &vulkan_device,
				vk::SurfaceKHR surface,
				std::uint32_t present_queue_family,
				const vk::Extent2D &swapchain_size);

		void
		create_swapchain(
				const vk::Extent2D &swapchain_size);

		void
		destroy_swapchain();


		void
		create_render_pass();

		void
		destroy_render_pass();


		void
		create_render_targets();

		void
		destroy_render_targets();
	};
}

#endif // GPLATES_OPENGL_VULKANSWAPCHAIN_H
