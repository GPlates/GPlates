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

#include <array>

#include "VulkanSwapchain.h"

#include "VulkanDevice.h"
#include "VulkanException.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


GPlatesOpenGL::VulkanSwapchain::VulkanSwapchain() :
	d_present_queue_family(UINT32_MAX),
	d_sample_count(vk::SampleCountFlagBits::e1),
	d_using_msaa(false),
	d_swapchain_image_format(vk::Format::eUndefined),
	d_using_depth_stencil(false)
{
}


void
GPlatesOpenGL::VulkanSwapchain::create(
		VulkanDevice &vulkan_device,
		vk::SurfaceKHR surface,
		std::uint32_t present_queue_family,
		const vk::Extent2D &swapchain_size,
		vk::SampleCountFlagBits sample_count,
		bool create_depth_stencil_attachment)
{
	GPlatesGlobal::Assert<VulkanException>(
			!d_swapchain,
			GPLATES_ASSERTION_SOURCE,
			"Attempted to create Vulkan swapchain that has already been created.");

	d_surface = surface;
	d_present_queue_family = present_queue_family;

	// Get the present queue from the logical device.
	// Note: This may or may not be the same as the graphics+compute queue in VulkanDevice.
	d_present_queue = vulkan_device.get_device().getQueue(d_present_queue_family, 0/*queueIndex*/);

	// Optional MSAA and depth/stencil.
	d_sample_count = sample_count;
	d_using_msaa = sample_count != vk::SampleCountFlagBits::e1;  // use MSAA if sample count > 1
	d_using_depth_stencil = create_depth_stencil_attachment;

	// Create swapchain first, then swapchain image views (from swapchain images), then create optional
	// MSAA images, then create optional depth/stencil image, then render pass and finally the framebuffers.
	create_swapchain(vulkan_device, swapchain_size);
	create_swapchain_image_views(vulkan_device);
	if (d_using_msaa)
	{
		create_msaa_images(vulkan_device);
	}
	if (d_using_depth_stencil)
	{
		create_depth_stencil_image(vulkan_device);
	}
	create_render_pass(vulkan_device);
	create_framebuffers(vulkan_device);
}


void
GPlatesOpenGL::VulkanSwapchain::recreate(
		VulkanDevice &vulkan_device,
		const vk::Extent2D &swapchain_size)
{
	GPlatesGlobal::Assert<VulkanException>(
			d_swapchain,
			GPLATES_ASSERTION_SOURCE,
			"Attempted to recreate Vulkan swapchain without first creating it.");

	// First make sure all commands in all queues have finished.
	// This is in case any commands are still operating on an acquired swapchain image.
	//
	// Note: It's OK to wait here since destroying a swapchain is not a performance-critical part of the code.
	vulkan_device.get_device().waitIdle();

	// Destroy framebuffers first, then render pass, then depth/stencil image (if using one), then
	// MSAA images (if using them), then swapchain image views (from swapchain images).
	destroy_framebuffers(vulkan_device);
	destroy_render_pass(vulkan_device);
	if (d_using_depth_stencil)
	{
		destroy_depth_stencil_image(vulkan_device);
	}
	if (d_using_msaa)
	{
		destroy_msaa_images(vulkan_device);
	}
	destroy_swapchain_image_views(vulkan_device);

	// Recreate the swapchain, which passes in the old swapchain (to aid in presentation resource reuse),
	// so we'll keep track of the old swapchain and destroy it after the new swapchain is created.
	vk::SwapchainKHR old_swapchain = d_swapchain;
	create_swapchain(vulkan_device, swapchain_size);
	// Destroy the *old* swapchain (not the current swapchain just created, which 'destroy()' would do).
	if (old_swapchain)
	{
		vulkan_device.get_device().destroySwapchainKHR(old_swapchain);
	}

	// Create the swapchain image views (from swapchain images), then create MSAA images (if using them),
	// then create depth/stencil image (if using one), then render pass and finally the framebuffers.
	create_swapchain_image_views(vulkan_device);
	if (d_using_msaa)
	{
		create_msaa_images(vulkan_device);
	}
	if (d_using_depth_stencil)
	{
		create_depth_stencil_image(vulkan_device);
	}
	create_render_pass(vulkan_device);
	create_framebuffers(vulkan_device);
}


void
GPlatesOpenGL::VulkanSwapchain::destroy(
		VulkanDevice &vulkan_device)
{
	GPlatesGlobal::Assert<VulkanException>(
			d_swapchain,
			GPLATES_ASSERTION_SOURCE,
			"Attempted to destroy Vulkan swapchain without first creating it.");

	// First make sure all commands in all queues have finished.
	// This is in case any commands are still operating on an acquired swapchain image.
	//
	// Note: It's OK to wait here since destroying a swapchain is not a performance-critical part of the code.
	vulkan_device.get_device().waitIdle();

	// Destroy framebuffers first, then render pass, then depth/stencil image (if using one), then
	// MSAA images (if using them), then swapchain image views (from swapchain images), and finally the swapchain.
	destroy_framebuffers(vulkan_device);
	destroy_render_pass(vulkan_device);
	if (d_using_depth_stencil)
	{
		destroy_depth_stencil_image(vulkan_device);
	}
	if (d_using_msaa)
	{
		destroy_msaa_images(vulkan_device);
	}
	destroy_swapchain_image_views(vulkan_device);
	destroy_swapchain(vulkan_device);
}


vk::Image
GPlatesOpenGL::VulkanSwapchain::get_swapchain_image(
		std::uint32_t swapchain_image_index) const
{
	GPlatesGlobal::Assert<VulkanException>(
			swapchain_image_index < d_swapchain_images.size(),
			GPLATES_ASSERTION_SOURCE,
			"Swapchain image index >= number of images");

	return d_swapchain_images[swapchain_image_index];
}


vk::ImageView
GPlatesOpenGL::VulkanSwapchain::get_swapchain_image_view(
		std::uint32_t swapchain_image_index) const
{
	GPlatesGlobal::Assert<VulkanException>(
			swapchain_image_index < d_swapchain_image_views.size(),
			GPLATES_ASSERTION_SOURCE,
			"Swapchain image index >= number of images");

	return d_swapchain_image_views[swapchain_image_index];
}


vk::Framebuffer
GPlatesOpenGL::VulkanSwapchain::get_framebuffer(
		std::uint32_t swapchain_image_index) const
{
	GPlatesGlobal::Assert<VulkanException>(
			swapchain_image_index < d_framebuffers.size(),
			GPLATES_ASSERTION_SOURCE,
			"Swapchain image index >= number of images");

	return d_framebuffers[swapchain_image_index];
}


std::vector<vk::ClearValue>
GPlatesOpenGL::VulkanSwapchain::get_framebuffer_clear_values() const
{
	std::vector<vk::ClearValue> clear_values;

	// Clear colour of the framebuffer.
	//
	// Note that we clear the colour to (0,0,0,1) and not (0,0,0,0) because we want any parts of
	// the scene, that are not rendered, to have *opaque* alpha (=1).
	// This ensures that if there is any further alpha composition of the framebuffer
	// (we attempt to set vk::CompositeAlphaFlagBitsKHR::eOpaque, if it's supported, to avoid further composition)
	// then it will have no effect.
	// For example, we don't want our black background converted to white if the underlying surface happens to be white.
	const vk::ClearColorValue clear_colour = std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f };

	// First clear value is for swapchain image attachment.
	clear_values.push_back(clear_colour);

	// Next clear value is for optional MSAA image attachment.
	if (d_using_msaa)
	{
		clear_values.push_back(clear_colour);
	}

	// Last clear value is for optional depth/stencil image attachment.
	if (d_using_depth_stencil)
	{
		const vk::ClearDepthStencilValue clear_depth_stencil = { 1.0f, 0 };
		clear_values.push_back(clear_depth_stencil);
	}

	return clear_values;
}


void
GPlatesOpenGL::VulkanSwapchain::create_swapchain(
		VulkanDevice &vulkan_device,
		const vk::Extent2D &swapchain_size)
{
	// Get the supported surface formats.
	const std::vector<vk::SurfaceFormatKHR> supported_surface_formats =
			vulkan_device.get_physical_device().getSurfaceFormatsKHR(d_surface);

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
			vulkan_device.get_physical_device().getSurfaceCapabilitiesKHR(d_surface);

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

	// Swapchain image sharing mode.
	//
	// Exclusive means if the present queue family is not the graphics+compute queue family then we'll need
	// to transfer image ownership from the graphics+compute queue to the present queue before displaying it.
	const vk::SharingMode swapchain_sharing_mode = vk::SharingMode::eExclusive;

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
			.setImageSharingMode(swapchain_sharing_mode)
			.setPreTransform(pre_transform)
			.setCompositeAlpha(composite_alpha)
			.setPresentMode(present_mode)
			.setClipped(true)
			.setOldSwapchain(old_swapchain);
	d_swapchain = vulkan_device.get_device().createSwapchainKHR(swapchain_create_info);

	// Get the swapchain images (from the swapchain).
	d_swapchain_images = vulkan_device.get_device().getSwapchainImagesKHR(d_swapchain);
}


void
GPlatesOpenGL::VulkanSwapchain::destroy_swapchain(
		VulkanDevice &vulkan_device)
{
	vulkan_device.get_device().destroySwapchainKHR(d_swapchain);
	d_swapchain = nullptr;

	// Reset some members.
	d_swapchain_image_format = vk::Format::eUndefined;
	d_swapchain_size = vk::Extent2D{};  // Zero extent.
	// Note that the swapchain images are owned by the swapchain (ie, we don't destroy them).
	d_swapchain_images.clear();  // Set size back to zero.
}


void
GPlatesOpenGL::VulkanSwapchain::create_swapchain_image_views(
		VulkanDevice &vulkan_device)
{
	// For each swapchain image, create an image view.
	for (vk::Image swapchain_image : d_swapchain_images)
	{
		// Create swapchain image view.
		vk::ImageViewCreateInfo swapchain_image_view_create_info;
		swapchain_image_view_create_info
				.setImage(swapchain_image)
				.setViewType(vk::ImageViewType::e2D)
				.setFormat(d_swapchain_image_format)
				.setComponents({})  // identity swizzle
				.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } );
		vk::ImageView swapchain_image_view = vulkan_device.get_device().createImageView(swapchain_image_view_create_info);

		d_swapchain_image_views.push_back(swapchain_image_view);
	}
}


void
GPlatesOpenGL::VulkanSwapchain::destroy_swapchain_image_views(
		VulkanDevice &vulkan_device)
{
	// For each swapchain image, destroy the swapchain image view.
	for (vk::ImageView swapchain_image_view : d_swapchain_image_views)
	{
		// Destroy swapchain image *view* but not swapchain image itself (it belongs to the swapchain).
		vulkan_device.get_device().destroyImageView(swapchain_image_view);
	}

	d_swapchain_image_views.clear();  // Set size back to zero.
}


void
GPlatesOpenGL::VulkanSwapchain::create_msaa_images(
		VulkanDevice &vulkan_device)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_using_msaa && !d_msaa_images,
			GPLATES_ASSERTION_SOURCE);

	d_msaa_images = std::vector<MSAAImage>();

	// For each swapchain image, create a MSAA image (and image view).
	for (std::uint32_t swapchain_image_index = 0;
		swapchain_image_index < d_swapchain_images.size();
		++swapchain_image_index)
	{
		// Create an image for use as a MSAA colour attachment.
		vk::ImageCreateInfo msaa_image_create_info;
		msaa_image_create_info
				.setImageType(vk::ImageType::e2D)
				.setFormat(d_swapchain_image_format)
				.setExtent({ d_swapchain_size.width, d_swapchain_size.height, 1 })
				.setMipLevels(1)
				.setArrayLayers(1)
				.setSamples(d_sample_count)
				.setTiling(vk::ImageTiling::eOptimal)
				// Specifying transient attachment enables image to be allocated using VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT...
				.setUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransientAttachment)
				.setSharingMode(vk::SharingMode::eExclusive)
				.setInitialLayout(vk::ImageLayout::eUndefined);
		// Allocation for MSAA image.
		VmaAllocationCreateInfo msaa_allocation_create_info = {};
		msaa_allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;
		msaa_allocation_create_info.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
		msaa_allocation_create_info.preferredFlags = VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
		VulkanImage msaa_image = VulkanImage::create(
				vulkan_device.get_vma_allocator(),
				msaa_image_create_info,
				msaa_allocation_create_info,
				GPLATES_EXCEPTION_SOURCE);

		// Create MSAA image view.
		vk::ImageViewCreateInfo msaa_image_view_create_info;
		msaa_image_view_create_info
				.setImage(msaa_image.get_image())
				.setViewType(vk::ImageViewType::e2D)
				.setFormat(d_swapchain_image_format)
				.setComponents({})  // identity swizzle
				.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } );
		vk::ImageView msaa_image_view = vulkan_device.get_device().createImageView(msaa_image_view_create_info);

		// MSAA image and image view.
		d_msaa_images->push_back({ msaa_image, msaa_image_view });
	}
}


void
GPlatesOpenGL::VulkanSwapchain::destroy_msaa_images(
		VulkanDevice &vulkan_device)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_using_msaa && d_msaa_images,
			GPLATES_ASSERTION_SOURCE);

	for (MSAAImage &msaa_image : d_msaa_images.get())
	{
		vulkan_device.get_device().destroyImageView(msaa_image.image_view);
		VulkanImage::destroy(vulkan_device.get_vma_allocator(), msaa_image.image);
	}

	d_msaa_images = boost::none;
}


void
GPlatesOpenGL::VulkanSwapchain::create_depth_stencil_image(
		VulkanDevice &vulkan_device)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_using_depth_stencil && !d_depth_stencil_image,
			GPLATES_ASSERTION_SOURCE);

	// Determine depth/stencil format.
	//
	// The Vulkan spec states that one of the following depth/stencil formats must be supported (as a depth/stencil attachment):
	// - VK_FORMAT_D24_UNORM_S8_UINT
	// - VK_FORMAT_D32_SFLOAT_S8_UINT
	// ...so we'll prefer VK_FORMAT_D24_UNORM_S8_UINT if it's supported, otherwise VK_FORMAT_D32_SFLOAT_S8_UINT.
	vk::Format depth_stencil_format = vk::Format::eUndefined;
	if (vulkan_device.get_physical_device().getFormatProperties(vk::Format::eD24UnormS8Uint).optimalTilingFeatures &
		vk::FormatFeatureFlagBits::eDepthStencilAttachment)
	{
		depth_stencil_format = vk::Format::eD24UnormS8Uint;
	}
	else
	{
		GPlatesGlobal::Assert<VulkanException>(
				vulkan_device.get_physical_device().getFormatProperties(vk::Format::eD32SfloatS8Uint).optimalTilingFeatures &
					vk::FormatFeatureFlagBits::eDepthStencilAttachment,
				GPLATES_ASSERTION_SOURCE,
				"Vulkan should support either VK_FORMAT_D24_UNORM_S8_UINT or VK_FORMAT_D32_SFLOAT_S8_UINT as a depth/stencil attachment");

		depth_stencil_format = vk::Format::eD32SfloatS8Uint;
	}

	// Create an image for use as a depth/stencil attachment.
	vk::ImageCreateInfo depth_stencil_image_create_info;
	depth_stencil_image_create_info
			.setImageType(vk::ImageType::e2D)
			.setFormat(depth_stencil_format)
			.setExtent({ d_swapchain_size.width, d_swapchain_size.height, 1 })
			.setMipLevels(1)
			.setArrayLayers(1)
			.setSamples(d_sample_count)
			.setTiling(vk::ImageTiling::eOptimal)
			// Specifying transient attachment enables image to be allocated using VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT...
			.setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransientAttachment)
			.setSharingMode(vk::SharingMode::eExclusive)
			.setInitialLayout(vk::ImageLayout::eUndefined);
	// Allocation for depth/stencil image.
	VmaAllocationCreateInfo depth_stencil_allocation_create_info = {};
	depth_stencil_allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;
	depth_stencil_allocation_create_info.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
	depth_stencil_allocation_create_info.preferredFlags = VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
	VulkanImage depth_stencil_image = VulkanImage::create(
			vulkan_device.get_vma_allocator(),
			depth_stencil_image_create_info,
			depth_stencil_allocation_create_info,
			GPLATES_EXCEPTION_SOURCE);

	// Create depth/stencil image view.
	vk::ImageViewCreateInfo depth_stencil_image_view_create_info;
	depth_stencil_image_view_create_info
			.setImage(depth_stencil_image.get_image())
			.setViewType(vk::ImageViewType::e2D)
			.setFormat(depth_stencil_format)
			.setComponents({})  // identity swizzle
			.setSubresourceRange({ vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, 0, 1, 0, 1 } );
	vk::ImageView depth_stencil_image_view = vulkan_device.get_device().createImageView(depth_stencil_image_view_create_info);

	d_depth_stencil_image = DepthStencilImage{ depth_stencil_format, depth_stencil_image, depth_stencil_image_view };
}


void
GPlatesOpenGL::VulkanSwapchain::destroy_depth_stencil_image(
		VulkanDevice &vulkan_device)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_using_depth_stencil && d_depth_stencil_image,
			GPLATES_ASSERTION_SOURCE);

	vulkan_device.get_device().destroyImageView(d_depth_stencil_image->image_view);
	VulkanImage::destroy(vulkan_device.get_vma_allocator(), d_depth_stencil_image->image);

	d_depth_stencil_image = boost::none;
}


void
GPlatesOpenGL::VulkanSwapchain::create_render_pass(
		VulkanDevice &vulkan_device)
{
	std::vector<vk::AttachmentDescription> attachment_descriptions;

	// Swapchain image attachment.
	vk::AttachmentDescription swapchain_image_attachment_description;
	swapchain_image_attachment_description
			.setFormat(d_swapchain_image_format)
			.setSamples(vk::SampleCountFlagBits::e1)  // swapchain image is always single-sample
			// We'll clear the colour attachment (except for MSAA since this will be target of MSAA resolve)...
			.setLoadOp(d_using_msaa ? vk::AttachmentLoadOp::eDontCare : vk::AttachmentLoadOp::eClear)
			// We'll keep the colour attachment contents after the render pass...
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

	// Swapchain image is the first attachment (and only attachment if not using MSAA or depth/stencil).
	const std::uint32_t swapchain_image_attachment_index = attachment_descriptions.size();
	attachment_descriptions.push_back(swapchain_image_attachment_description);

	// Colour attachment references swapchain image attachment (unless using MSAA - see below).
	vk::AttachmentReference colour_attachment_reference;
	colour_attachment_reference
			.setAttachment(swapchain_image_attachment_index)
			// Subpass renders to attachment, so it should be in an optimal image layout...
			.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

	// If we're using MSAA then a resolve attachment references the swapchain image attachment and
	// the colour attachment references a new MSAA image attachment.
	vk::AttachmentReference resolve_attachment_reference;
	if (d_using_msaa)
	{
		// MSAA image attachment.
		vk::AttachmentDescription msaa_image_attachment_description;
		msaa_image_attachment_description
				.setFormat(d_swapchain_image_format)
				.setSamples(d_sample_count)
				// We'll clear the MSAA attachment on load (since we'll be rendering to this attachment after that)...
				.setLoadOp(vk::AttachmentLoadOp::eClear)
				// Not keeping the MSAA attachment contents after the render pass (used only to resolve into swapchain image)...
				.setStoreOp(vk::AttachmentStoreOp::eDontCare)
				// Don't care about stencil load/store...
				.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
				.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
				// Not preserving initial contents of MSAA image.
				.setInitialLayout(vk::ImageLayout::eUndefined)
				// Final layout leave as optimal (from subpass)...
				.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);

		// MSAA image is the next attachment.
		const std::uint32_t msaa_attachment_index = attachment_descriptions.size();
		attachment_descriptions.push_back(msaa_image_attachment_description);

		// The colour attachment now references the MSAA image attachment.
		colour_attachment_reference.setAttachment(msaa_attachment_index);

		// The resolve attachment now references the swapchain image attachment.
		resolve_attachment_reference
				.setAttachment(swapchain_image_attachment_index)
				// Subpass resolves MSAA attachment to resolve attachment, so optimal image layout is fine...
				.setLayout(vk::ImageLayout::eColorAttachmentOptimal);
	}

	// Optional depth/stencil attachment references optional depth/stencil image attachment.
	vk::AttachmentReference depth_stencil_attachment_reference;
	if (d_using_depth_stencil)
	{
		// Depth/stencil image attachment.
		vk::AttachmentDescription depth_stencil_image_attachment_description;
		depth_stencil_image_attachment_description
				.setFormat(d_depth_stencil_image->format)
				.setSamples(d_sample_count)
				// We'll clear the depth attachment on load...
				.setLoadOp(vk::AttachmentLoadOp::eClear)
				// Don't care about depth store...
				.setStoreOp(vk::AttachmentStoreOp::eDontCare)
				// We'll clear the stencil attachment on load...
				.setStencilLoadOp(vk::AttachmentLoadOp::eClear)
				// Don't care about stencil store...
				.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
				// Not preserving initial contents of depth/stencil image.
				.setInitialLayout(vk::ImageLayout::eUndefined)
				// Final layout leave as optimal (from subpass)...
				.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

		// Depth/stencil image is the last attachment.
		const std::uint32_t depth_stencil_attachment_index = attachment_descriptions.size();
		attachment_descriptions.push_back(depth_stencil_image_attachment_description);

		// Depth/stencil attachment references depth/stencil image attachment.
		depth_stencil_attachment_reference
				.setAttachment(depth_stencil_attachment_index)
				// Subpass uses attachment as a depth/stencil buffer, so it should be in an optimal image layout...
				.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
	}

	// One subpass using a single colour attachment, and optional resolve attachment, and optional depth/stencil attachment.
	vk::SubpassDescription subpass_description;
	subpass_description
			.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
			.setColorAttachments(colour_attachment_reference);
	if (d_using_msaa)
	{
		subpass_description.setPResolveAttachments(&resolve_attachment_reference);
	}
	if (d_using_depth_stencil)
	{
		subpass_description.setPDepthStencilAttachment(&depth_stencil_attachment_reference);
	}

	std::vector<vk::SubpassDependency> subpass_dependencies;

	// One subpass self-dependency to allow users to use a pipeline barrier inside (the subpass of) this render pass.
	// This is configured such that any pipeline barriers must be for non-attachment reads/writes using fragment shader.
	// For example, writing to non-attachment storage images/buffers using fragment shader, then a emitting barrier,
	// then reading from them using another fragment shader (all done within subpass).
	//
	// Note: Unlike subpass dependencies that are not a self-dependency, a self-dependency does not actually
	//       create a memory dependency (it only places constraints on pipeline barriers in the subpass, and
	//       the barriers must be explicitly created by the user).
	vk::SubpassDependency non_attachment_subpass_self_dependency;
	non_attachment_subpass_self_dependency
			// Self-dependency (on sole subpass) uses same src/dst subpass indices...
			.setSrcSubpass(0)
			.setDstSubpass(0)
			// Fragment shader reads/writes before a pipeline barrier will be available/visible before
			// fragment shader reads/writes after the pipeline barrier...
			.setSrcStageMask(vk::PipelineStageFlagBits::eFragmentShader)
			.setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
			.setSrcAccessMask(vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite)
			.setDstAccessMask(vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite)
			// Required for a subpass self-dependency when it specifies framebuffer-space stages (as we have)...
			.setDependencyFlags(vk::DependencyFlagBits::eByRegion);
	subpass_dependencies.push_back(non_attachment_subpass_self_dependency);

	// One subpass external dependency to ensure swapchain image layout transition (from eUndefined to
	// eColorAttachmentOptimal) happens *after* image is acquired (which happens before stage
	// eColorAttachmentOutput since that is the wait stage on the image acquire semaphore).
	vk::SubpassDependency swapchain_image_acquire_subpass_dependency;
	swapchain_image_acquire_subpass_dependency
			// Synchronise with commands before the render pass...
			.setSrcSubpass(VK_SUBPASS_EXTERNAL)
			// Only one subpass...
			.setDstSubpass(0)
			// Chain dependency with the wait stage of the image acquire semaphore.
			// This means this dependency will also wait for the swapchain image to be acquired.
			// The layout transition (from eUndefined to eColorAttachmentOptimal) will then happen after that...
			.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
			// Block colour attachment writes by the subpass (but stages before that are not blocked, eg, vertex shader).
			// The layout transition (from eUndefined to eColorAttachmentOptimal) will happen before that...
			.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
			// Swapchain image acquire signals a semaphore, which ensures all writes are made available, so
			// no need to specify a src access mask. In any case, there's no writes in the presentation engine...
			.setSrcAccessMask({})
			// Colour attachment clear (in render pass) is a write operation...
			.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);
	subpass_dependencies.push_back(swapchain_image_acquire_subpass_dependency);

	// And one subpass external dependency to ensure swapchain image layout transition (from eColorAttachmentOptimal
	// to ePresentSrcKHR) happens *before* image queue ownership is (potentially) released from graphics+compute queue
	// (if present queue in a different queue family than graphics+compute queue).
	vk::SubpassDependency swapchain_image_release_subpass_dependency;
	swapchain_image_release_subpass_dependency
			// Only one subpass...
			.setSrcSubpass(0)
			// Synchronise with commands after the render pass...
			.setDstSubpass(VK_SUBPASS_EXTERNAL)
			// Wait for the swapchain image to be rendered to (in colour attachment stage).
			// The layout transition (from eColorAttachmentOptimal to ePresentSrcKHR) will then happen after that...
			.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
			// Chain dependency with the subsequent (potential) queue ownership image release pipeline barrier.
			// The layout transition (from eColorAttachmentOptimal to ePresentSrcKHR) will happen before that...
			.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
			// Colour attachment writes should be made available (before subsequent queue ownership release)...
			.setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
			// Colour attachment reads/writes should be made visible (before subsequent queue ownership release)...
			.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);
	subpass_dependencies.push_back(swapchain_image_release_subpass_dependency);

	if (d_using_depth_stencil)
	{
		// One subpass external dependency to ensure depth/stencil writes to shared depth/stencil attachment in a previous render pass
		// (which used the *same* shared depth/stencil image, but with a different swapchain colour image) do not overlap with
		// depth/stencil writes in the next render pass...
		vk::SubpassDependency shared_depth_stencil_subpass_dependency;
		shared_depth_stencil_subpass_dependency
				// Synchronise with commands before the render pass...
				.setSrcSubpass(VK_SUBPASS_EXTERNAL)
				// Only one subpass...
				.setDstSubpass(0)
				// Wait for any depth/stencil writes to shared depth/stencil attachment from previous render pass
				// (which used the *same* shared depth/stencil image, but with a different swapchain colour image).
				// The layout transition (from eUndefined to eDepthStencilAttachmentOptimal) will then happen after that.
				// Note: The depth/stencil store op happens in the LATE_FRAGMENT_TESTS stage...
				.setSrcStageMask(vk::PipelineStageFlagBits::eLateFragmentTests)
				// Block depth/stencil attachment access in the subpass (but stages before that are not blocked, eg, vertex shader).
				// The layout transition (from eUndefined to eDepthStencilAttachmentOptimal) will happen before that.
				// Note: The depth/stencil load op (clear) happens in the EARLY_FRAGMENT_TESTS stage...
				.setDstStageMask(vk::PipelineStageFlagBits::eEarlyFragmentTests)
				// Make *available* any depth/stencil writes to shared depth/stencil attachment from previous render pass
				// (which used the *same* shared depth/stencil image, but with a different swapchain colour image)...
				.setSrcAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite)
				// Make *visible* any depth/stencil writes to shared depth/stencil attachment from previous render pass
				// (which used the *same* shared depth/stencil image, but with a different swapchain colour image).
				// Note: Depth/stencil attachment clear (in render pass) is a write operation...
				.setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite);
		subpass_dependencies.push_back(shared_depth_stencil_subpass_dependency);
	}

	// Create the render pass.
	vk::RenderPassCreateInfo render_pass_create_info;
	render_pass_create_info
			.setAttachments(attachment_descriptions)
			.setSubpasses(subpass_description)
			.setDependencies(subpass_dependencies);
	d_render_pass = vulkan_device.get_device().createRenderPass(render_pass_create_info);
}


void
GPlatesOpenGL::VulkanSwapchain::destroy_render_pass(
		VulkanDevice &vulkan_device)
{
	vulkan_device.get_device().destroyRenderPass(d_render_pass);
	d_render_pass = nullptr;
}


void
GPlatesOpenGL::VulkanSwapchain::create_framebuffers(
		VulkanDevice &vulkan_device)
{
	// For each swapchain image view, create a framebuffer.
	for (std::uint32_t swapchain_image_index = 0;
		swapchain_image_index < d_swapchain_images.size();
		++swapchain_image_index)
	{
		// Framebuffer attachments.
		std::vector<vk::ImageView> attachments;

		// The swapchain image view is first.
		attachments.push_back(d_swapchain_image_views[swapchain_image_index]);

		// Optional MSAA image (if using MSAA) is next.
		if (d_using_msaa)
		{
			attachments.push_back(d_msaa_images.get()[swapchain_image_index].image_view);
		}

		// Optional depth/stencil image is last.
		if (d_using_depth_stencil)
		{
			attachments.push_back(d_depth_stencil_image->image_view);
		}

		// Create framebuffer.
		vk::FramebufferCreateInfo framebuffer_create_info;
		framebuffer_create_info
				.setRenderPass(d_render_pass)
				.setAttachments(attachments)
				.setWidth(d_swapchain_size.width)
				.setHeight(d_swapchain_size.height)
				.setLayers(1);
		vk::Framebuffer framebuffer = vulkan_device.get_device().createFramebuffer(framebuffer_create_info);

		d_framebuffers.push_back(framebuffer);
	}
}


void
GPlatesOpenGL::VulkanSwapchain::destroy_framebuffers(
		VulkanDevice &vulkan_device)
{
	// For each swapchain image, destroy the framebuffer referencing its image view.
	for (vk::Framebuffer framebuffer : d_framebuffers)
	{
		vulkan_device.get_device().destroyFramebuffer(framebuffer);
	}

	d_framebuffers.clear();  // Set size back to zero.
}
