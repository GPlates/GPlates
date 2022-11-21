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


GPlatesOpenGL::VulkanSwapchain::VulkanSwapchain(
		VulkanDevice &vulkan_device,
		vk::SurfaceKHR surface,
		std::uint32_t present_queue_family,
		const QSize &window_size) :
	d_vulkan_device(vulkan_device),
	d_surface(surface),
	d_present_queue_family(present_queue_family)
{
	recreate_swapchain(window_size);
}


GPlatesOpenGL::VulkanSwapchain::~VulkanSwapchain()
{
	if (d_swapchain)
	{
		// First make sure all commands in all queues have finished.
		// This is in case any commands are still operating on an acquired swapchain image.
		//
		// Note: It's OK to wait here since destroying a swapchain is not a performance-critical part of the code.
		d_vulkan_device.get_device().waitIdle();

		d_vulkan_device.get_device().destroySwapchainKHR(d_swapchain);
	}
}


void
GPlatesOpenGL::VulkanSwapchain::recreate_swapchain(
		const QSize &window_size)
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
		// Use current window size.
		d_swapchain_size = vk::Extent2D(
				static_cast<std::uint32_t>(window_size.width()),
				static_cast<std::uint32_t>(window_size.height()));

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
	vk::SwapchainCreateInfoKHR swapchain_info;
	swapchain_info
			.setSurface(d_surface)
			.setMinImageCount(min_num_swapchain_images)
			.setImageFormat(surface_format.format)
			.setImageColorSpace(surface_format.colorSpace)
			.setImageExtent(d_swapchain_size)
			.setImageUsage(swapchain_usage)
			.setImageArrayLayers(1)
			.setImageSharingMode(vk::SharingMode::eExclusive)
			.setPreTransform(pre_transform)
			.setCompositeAlpha(composite_alpha)
			.setPresentMode(present_mode)
			.setClipped(true)
			.setOldSwapchain(old_swapchain);
	d_swapchain = d_vulkan_device.get_device().createSwapchainKHR(swapchain_info);

	if (old_swapchain)
	{
		// First make sure all commands in all queues have finished.
		// This is in case any commands are still operating on an acquired swapchain image.
		//
		// Note: It's OK to wait here since recreating a swapchain is not a performance-critical part of the code.
		d_vulkan_device.get_device().waitIdle();

		d_vulkan_device.get_device().destroySwapchainKHR(old_swapchain);
	}
}
