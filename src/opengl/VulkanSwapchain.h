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
#include <boost/optional.hpp>

#include "VulkanHpp.h"
#include "VulkanImage.h"


namespace GPlatesOpenGL
{
	class VulkanDevice;

	/**
	 * Vulkan swapchain and the presentation queue.
	 */
	class VulkanSwapchain
	{
	public:

		/**
		 * Construct a @a VulkanSwapchain.
		 *
		 * Note: This does not actually create a vk::SwapchainKHR, that happens in @a create (and @a recreate).
		 */
		VulkanSwapchain();


		/**
		 * Create a Vulkan swapchain (and its associated image views and framebuffers) supporting
		 * presentation to the specified Vulkan surface.
		 *
		 * The present queue family is specified with @a present_queue_family.
		 * This may or may not be the same as the graphics+compute queue family in @a VulkanDevice.
		 *
		 * If @a sample_count is greater than one then each single-sample swapchain colour image also gets an
		 * associated multi-sample image that is rendered into during a render pass and resolved into the
		 * single-sample swapchain image at the end of the render pass.
		 *
		 * If @a create_depth_stencil_attachment is true then a single depth/stencil image is created and
		 * added to each swapchain image framebuffer.
		 *
		 * Note: Before this, @a get_swapchain() will be equal to nullptr.
		 *       This can be used to test whether the swapchain has not yet been created, or has been destroyed.
		 *
		 * NOTE: VulkanHpp::initialise() must have been called first.
		 */
		void
		create(
				VulkanDevice &vulkan_device,
				vk::SurfaceKHR surface,
				std::uint32_t present_queue_family,
				const vk::Extent2D &swapchain_size,
				vk::SampleCountFlagBits sample_count,
				bool create_depth_stencil_attachment);

		/**
		 * Re-create the swapchain (and its associated image views and framebuffers).
		 *
		 * This is useful when the surface window is resized.
		 *
		 * Note: This should be called between @a create and @a destroy.
		 */
		void
		recreate(
				VulkanDevice &vulkan_device,
				const vk::Extent2D &swapchain_size);

		/**
		 * Destroy the Vulkan swapchain (and its associated image views and framebuffers).
		 *
		 * Note: After this, @a get_swapchain() will be equal to nullptr.
		 *       This can be used to test whether the swapchain has not yet been created, or has been destroyed.
		 */
		void
		destroy(
				VulkanDevice &vulkan_device);


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
		 * Returns the render pass (containing a single subpass) to use when rendering to a swapchain image.
		 *
		 * Pipeline barriers can be used inside the (sole) subpass for non-attachment reads/writes using fragment shader.
		 * For example, writing to non-attachment storage images/buffers using fragment shader, then emitting a barrier,
		 * then reading from them using another fragment shader (all done within subpass).
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
			return d_swapchain_images.size();
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
		 * Returns the sample count used for the render pass and colour/depth-stencil attachments in the framebuffer.
		 *
		 * If this is greater than one then rendering will be into a MSAA colour image (instead of
		 * into the single-sample swapchain image) and the MSAA image will be resolved into the
		 * single-sample swapchain image at the end of the render pass.
		 */
		vk::SampleCountFlagBits
		get_sample_count() const
		{
			return d_sample_count;
		}

		/**
		 * Returns the framebuffer at specified swapchain image index.
		 */
		vk::Framebuffer
		get_framebuffer(
				std::uint32_t swapchain_image_index) const;

		/**
		 * Returns the framebuffer clear values.
		 */
		std::vector<vk::ClearValue>
		get_framebuffer_clear_values() const;

	private:

		/**
		 * Optional MSAA colour image (only used when the sample count is greater than one).
		 *
		 * Each swapchain image will (optionally) have an associated MSAA image.
		 */
		struct MSAAImage
		{
			VulkanImage image;
			vk::ImageView image_view;
		};

		/**
		 * An optional single depth/stencil image shared by all swapchain images/framebuffers.
		 */
		struct DepthStencilImage
		{
			vk::Format format;
			VulkanImage image;
			vk::ImageView image_view;
		};

		vk::SurfaceKHR d_surface;
		std::uint32_t d_present_queue_family;
		vk::Queue d_present_queue;

		vk::SampleCountFlagBits d_sample_count;
		bool d_using_msaa;

		vk::SwapchainKHR d_swapchain;
		vk::Format d_swapchain_image_format;
		vk::Extent2D d_swapchain_size;

		vk::RenderPass d_render_pass;

		//! Swapchain images.
		std::vector<vk::Image> d_swapchain_images;
		//! Swapchain image views.
		std::vector<vk::ImageView> d_swapchain_image_views;

		//! Optionally, each swapchain image has an associated MSAA image (only used when sample count is greater than one).
		boost::optional<std::vector<MSAAImage>> d_msaa_images;

		//! Optional depth/stencil image shared by all swapchain images/framebuffers.
		boost::optional<DepthStencilImage> d_depth_stencil_image;
		bool d_using_depth_stencil;

		//! Each swapchain image has an associated framebuffer.
		std::vector<vk::Framebuffer> d_framebuffers;


		void
		create_swapchain(
				VulkanDevice &vulkan_device,
				const vk::Extent2D &swapchain_size);

		void
		destroy_swapchain(
				VulkanDevice &vulkan_device);


		void
		create_swapchain_image_views(
				VulkanDevice &vulkan_device);
		
		void
		destroy_swapchain_image_views(
				VulkanDevice &vulkan_device);


		void
		create_msaa_images(
				VulkanDevice &vulkan_device);

		void
		destroy_msaa_images(
				VulkanDevice &vulkan_device);


		void
		create_depth_stencil_image(
				VulkanDevice &vulkan_device);

		void
		destroy_depth_stencil_image(
				VulkanDevice &vulkan_device);


		void
		create_render_pass(
				VulkanDevice &vulkan_device);

		void
		destroy_render_pass(
				VulkanDevice &vulkan_device);


		void
		create_framebuffers(
				VulkanDevice &vulkan_device);

		void
		destroy_framebuffers(
				VulkanDevice &vulkan_device);
	};
}

#endif // GPLATES_OPENGL_VULKANSWAPCHAIN_H
