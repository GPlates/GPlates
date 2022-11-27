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
#include "VulkanException.h"

#include "global/GPlatesAssert.h"


GPlatesOpenGL::VulkanSwapchain::VulkanSwapchain(
		VulkanDevice &vulkan_device,
		vk::SurfaceKHR surface,
		std::uint32_t present_queue_family,
		const vk::Extent2D &swapchain_size) :
	d_vulkan_device(vulkan_device),
	d_surface(surface),
	d_present_queue_family(present_queue_family)
{
	// Get the present queue from the logical device.
	// Note: This may or may not be the same as the graphics+compute queue in VulkanDevice.
	d_present_queue = vulkan_device.get_device().getQueue(d_present_queue_family, 0/*queueIndex*/);

	// Create swapchain first, then render pass and finally the render targets (image views and framebuffers).
	create_swapchain(swapchain_size);
	create_render_pass();
	create_render_targets();
}


GPlatesOpenGL::VulkanSwapchain::~VulkanSwapchain()
{
	// First make sure all commands in all queues have finished.
	// This is in case any commands are still operating on an acquired swapchain image.
	//
	// Note: It's OK to wait here since destroying a swapchain is not a performance-critical part of the code.
	d_vulkan_device.get_device().waitIdle();

	// Destroy the render targets (image views and framebuffers), then render pass and finally the swapchain.
	destroy_render_targets();
	destroy_render_pass();
	destroy_swapchain();
}


vk::Image
GPlatesOpenGL::VulkanSwapchain::get_swapchain_image(
		std::uint32_t swapchain_image_index) const
{
	GPlatesGlobal::Assert<VulkanException>(
			swapchain_image_index < d_render_targets.size(),
			GPLATES_ASSERTION_SOURCE,
			"Swapchain image index >= number of images");

	return d_render_targets[swapchain_image_index].image;
}


vk::ImageView
GPlatesOpenGL::VulkanSwapchain::get_swapchain_image_view(
		std::uint32_t swapchain_image_index) const
{
	GPlatesGlobal::Assert<VulkanException>(
			swapchain_image_index < d_render_targets.size(),
			GPLATES_ASSERTION_SOURCE,
			"Swapchain image index >= number of images");

	return d_render_targets[swapchain_image_index].image_view;
}


vk::Framebuffer
GPlatesOpenGL::VulkanSwapchain::get_swapchain_framebuffer(
		std::uint32_t swapchain_image_index) const
{
	GPlatesGlobal::Assert<VulkanException>(
			swapchain_image_index < d_render_targets.size(),
			GPLATES_ASSERTION_SOURCE,
			"Swapchain image index >= number of images");

	return d_render_targets[swapchain_image_index].framebuffer;
}


void
GPlatesOpenGL::VulkanSwapchain::recreate_swapchain(
		const vk::Extent2D &swapchain_size)
{
	// First make sure all commands in all queues have finished.
	// This is in case any commands are still operating on an acquired swapchain image.
	//
	// Note: It's OK to wait here since recreating a swapchain is not a performance-critical part of the code.
	d_vulkan_device.get_device().waitIdle();

	// Destroy render targets (an image view and framebuffer associated with each swapchain image).
	destroy_render_targets();
	// Destroy render pass (shouldn't really need to, but do anyway since it depends on swapchain image format
	// determined in new swapchain).
	destroy_render_pass();

	// Recreate the swapchain, which passes in the old swapchain (to aid in presentation resource reuse),
	// so we'll keep track of the old swapchain and destroy it after the new swapchain is created.
	vk::SwapchainKHR old_swapchain = d_swapchain;
	create_swapchain(swapchain_size);
	// Destroy the *old* swapchain (not the current swapchain just created, which 'destroy_swapchain()' would do).
	if (old_swapchain)
	{
		d_vulkan_device.get_device().destroySwapchainKHR(old_swapchain);
	}

	// Create the render pass and the render targets (image views and framebuffers).
	create_render_pass();
	create_render_targets();
}


void
GPlatesOpenGL::VulkanSwapchain::create_swapchain(
		const vk::Extent2D &swapchain_size)
{
	// Get the supported surface formats.
	const std::vector<vk::SurfaceFormatKHR> supported_surface_formats =
			d_vulkan_device.get_physical_device().getSurfaceFormatsKHR(d_surface);

	// There is at least one supported supported surface format.
	// We'll use it if we can't find our desired format.
	vk::SurfaceFormatKHR surface_format = supported_surface_formats[0];
	// Search for our desired format.
	const vk::Format desired_format = vk::Format::eR8G8B8A8Unorm;
	for (const auto &supported_surface_format : supported_surface_formats)
	{
		if (supported_surface_format.format == desired_format)
		{
			surface_format = supported_surface_format;
			break;
		}
	}
	d_swapchain_image_format = surface_format.format;
	const vk::ColorSpaceKHR swapchain_image_color_space = surface_format.colorSpace;


	// Get the surface capabilities.
	const vk::SurfaceCapabilitiesKHR surface_capabilities =
			d_vulkan_device.get_physical_device().getSurfaceCapabilitiesKHR(d_surface);

	// Number of swapchain images.
	std::uint32_t min_num_swapchain_images = 2;  // Double buffer
	if (min_num_swapchain_images < surface_capabilities.minImageCount)
	{
		min_num_swapchain_images = surface_capabilities.minImageCount;
	}
	if (surface_capabilities.maxImageCount > 0 &&  // 'maxImageCount == 0' means unlimited images
		min_num_swapchain_images > surface_capabilities.maxImageCount)
	{
		min_num_swapchain_images = surface_capabilities.maxImageCount;
	}

	// Size of the swapchain.
	if (surface_capabilities.currentExtent.width == std::uint32_t(-1) &&
		surface_capabilities.currentExtent.height == std::uint32_t(-1))
	{
		// Use the requested swapchain size.
		d_swapchain_size = swapchain_size;

		// Respect min limits.
		if (d_swapchain_size.width < surface_capabilities.minImageExtent.width)
		{
			d_swapchain_size.width = surface_capabilities.minImageExtent.width;
		}
		if (d_swapchain_size.height < surface_capabilities.minImageExtent.height)
		{
			d_swapchain_size.height = surface_capabilities.minImageExtent.height;
		}

		// Respect max limits.
		if (d_swapchain_size.width > surface_capabilities.maxImageExtent.width)
		{
			d_swapchain_size.width = surface_capabilities.maxImageExtent.width;
		}
		if (d_swapchain_size.height > surface_capabilities.maxImageExtent.height)
		{
			d_swapchain_size.height = surface_capabilities.maxImageExtent.height;
		}
	}
	else
	{
		// Use current size of surface.
		d_swapchain_size = surface_capabilities.currentExtent;
	}

	// Swapchain image usage.
	//
	// Note that color attachment usage is always supported.
	// Currently we only render to the swapchain using it as a framebuffer colour attachment.
	const vk::ImageUsageFlags swapchain_usage = vk::ImageUsageFlagBits::eColorAttachment;

	// Surface pre-transform.
	//
	// We don't want any transformations to occur, so if the identity transform is supported then use it,
	// otherwise just use the current transform.
	const vk::SurfaceTransformFlagBitsKHR pre_transform =
			(surface_capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity)
			? vk::SurfaceTransformFlagBitsKHR::eIdentity
			: surface_capabilities.currentTransform;

	// Composite alpha.
	//
	// We don't want to blend our rendered surface into other surfaces so use opaque compositing if supported,
	// otherwise try the other supported values.
	const vk::CompositeAlphaFlagBitsKHR composite_alpha =
		(surface_capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eOpaque)
			? vk::CompositeAlphaFlagBitsKHR::eOpaque
			: (surface_capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit)
				? vk::CompositeAlphaFlagBitsKHR::eInherit
				: (surface_capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied)
					? vk::CompositeAlphaFlagBitsKHR::ePreMultiplied
					: vk::CompositeAlphaFlagBitsKHR::ePostMultiplied;

	// Present mode.
	//
	// Use FIFO mode to avoid image tearing.
	// Note that FIFO mode is always supported.
	const vk::PresentModeKHR present_mode = vk::PresentModeKHR::eFifo;


	// The current swapchain.
	//
	// Note: When the swapchain is first created this will be null.
	vk::SwapchainKHR old_swapchain = d_swapchain;

	// Create the new swapchain.
	vk::SwapchainCreateInfoKHR swapchain_create_info;
	swapchain_create_info
			.setSurface(d_surface)
			.setMinImageCount(min_num_swapchain_images)
			.setImageFormat(d_swapchain_image_format)
			.setImageColorSpace(swapchain_image_color_space)
			.setImageExtent(d_swapchain_size)
			.setImageUsage(swapchain_usage)
			.setImageArrayLayers(1)
			.setImageSharingMode(vk::SharingMode::eExclusive)
			.setPreTransform(pre_transform)
			.setCompositeAlpha(composite_alpha)
			.setPresentMode(present_mode)
			.setClipped(true)
			.setOldSwapchain(old_swapchain);
	d_swapchain = d_vulkan_device.get_device().createSwapchainKHR(swapchain_create_info);
}


void
GPlatesOpenGL::VulkanSwapchain::destroy_swapchain()
{
	if (d_swapchain)
	{
		d_vulkan_device.get_device().destroySwapchainKHR(d_swapchain);
		d_swapchain = nullptr;
	}
}


void
GPlatesOpenGL::VulkanSwapchain::create_render_pass()
{
	// Swapchain image attachment.
	vk::AttachmentDescription swapchain_image_attachment_description;
	swapchain_image_attachment_description
			.setFormat(d_swapchain_image_format)
			.setSamples(vk::SampleCountFlagBits::e1)
			// We'll clear the colour attachment on input...
			.setLoadOp(vk::AttachmentLoadOp::eClear)
			// We'll keep the colour attachment output...
			.setStoreOp(vk::AttachmentStoreOp::eStore)
			// Don't care about stencil load/store...
			.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
			.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
			// Not preserving initial contents of acquired swapchain image.
			// Also has advantage that we don't need to do a queue ownership transfer from
			// present queue to graphics+compute queue (if they're from different queue families)...
			.setInitialLayout(vk::ImageLayout::eUndefined)
			// Final layout should be presentable (usable by presentation engine)...
			.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

	// Colour attachment references swapchain image attachment.
	vk::AttachmentReference colour_attachment_reference;
	colour_attachment_reference
			.setAttachment(0)
			// Subpass renders to attachment, so it should be in an optimal image layout...
			.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

	// One subpass using a single colour attachment.
	vk::SubpassDescription subpass_description;
	subpass_description
			.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
			.setColorAttachments(colour_attachment_reference);

	// One subpass external dependency to ensure swapchain image layout transition, from eUndefined to
	// eColorAttachmentOptimal, happens *after* image is acquired.
	vk::SubpassDependency subpass_dependency;
	subpass_dependency
			// Synchronise with commands before the render pass...
			.setSrcSubpass(VK_SUBPASS_EXTERNAL)
			// Only one subpass...
			.setDstSubpass(0)
			// Chain dependency with the wait stage of the image acquire semaphore.
			// This means this dependency will also wait for the swapchain image to be acquired.
			// The layout transition will then happen after that...
			.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
			// Block colour attachment writes by the subpass (but stages before that are not blocked, eg, vertex shader)...
			.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
			// Swapchain image acquire signals a semaphore, which ensures all writes are made available, so
			// no need to specify a src access mask. In any case, there's no writes in the presentation engine...
			.setSrcAccessMask({})
			// Colour attachment clear is a write operation...
			.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);

	vk::RenderPassCreateInfo render_pass_create_info;
	render_pass_create_info
			.setAttachments(swapchain_image_attachment_description)
			.setSubpasses(subpass_description)
			.setDependencies(subpass_dependency);
	d_render_pass = d_vulkan_device.get_device().createRenderPass(render_pass_create_info);
}


void
GPlatesOpenGL::VulkanSwapchain::destroy_render_pass()
{
	if (d_render_pass)
	{
		d_vulkan_device.get_device().destroyRenderPass(d_render_pass);
		d_render_pass = nullptr;
	}
}


void
GPlatesOpenGL::VulkanSwapchain::create_render_targets()
{
	// Get the swapchain images.
	const std::vector<vk::Image> swapchain_images = d_vulkan_device.get_device().getSwapchainImagesKHR(d_swapchain);

	// For each swapchain image, create an image view and a framebuffer.
	for (vk::Image image : swapchain_images)
	{
		// Create image view.
		vk::ImageViewCreateInfo image_view_create_info;
		image_view_create_info
				.setImage(image)
				.setViewType(vk::ImageViewType::e2D)
				.setFormat(d_swapchain_image_format)
				.setComponents({})  // identity swizzle
				.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } );
		vk::ImageView image_view = d_vulkan_device.get_device().createImageView(image_view_create_info);

		// Create framebuffer.
		vk::FramebufferCreateInfo framebuffer_create_info;
		framebuffer_create_info
				.setRenderPass(d_render_pass)
				.setAttachments(image_view)
				.setWidth(d_swapchain_size.width)
				.setHeight(d_swapchain_size.height)
				.setLayers(1);
		vk::Framebuffer framebuffer = d_vulkan_device.get_device().createFramebuffer(framebuffer_create_info);

		const RenderTarget render_target = { image, image_view, framebuffer };
		d_render_targets.push_back(render_target);
	}
}


void
GPlatesOpenGL::VulkanSwapchain::destroy_render_targets()
{
	// For each swapchain image, destroy the image view and framebuffer referencing it.
	for (const RenderTarget &render_target : d_render_targets)
	{
		// Destroy image view and framebuffer but not swapchain image itself (it belongs to the swapchain).
		if (render_target.image_view)
		{
			d_vulkan_device.get_device().destroyImageView(render_target.image_view);
		}
		if (render_target.framebuffer)
		{
			d_vulkan_device.get_device().destroyFramebuffer(render_target.framebuffer);
		}
	}

	d_render_targets.clear();  // Set size back to zero.
}
