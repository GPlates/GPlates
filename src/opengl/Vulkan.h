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

#ifndef GPLATES_OPENGL_VULKAN_H
#define GPLATES_OPENGL_VULKAN_H

#include "VulkanDevice.h"
#include "VulkanFrame.h"
#include "VulkanHpp.h"


namespace GPlatesOpenGL
{
	/**
	 * Central place to access Vulkan device and command buffers for use by most clients needing access to Vulkan.
	 */
	class Vulkan
	{
	public:

		Vulkan(
				VulkanDevice &vulkan_device,
				VulkanFrame &vulkan_frame,
				vk::RenderPass default_render_pass);


		/**
		 * Return the Vulkan instance.
		 */
		vk::Instance
		get_instance()
		{
			return d_vulkan_device.get_instance();
		}

		/**
		 * Return the Vulkan physical device (that the logical device was created from).
		 */
		vk::PhysicalDevice
		get_physical_device()
		{
			return d_vulkan_device.get_physical_device();
		}

		/**
		 * Return the properties of the Vulkan physical device (that the logical device was created from).
		 */
		const vk::PhysicalDeviceProperties &
		get_physical_device_properties()
		{
			return d_vulkan_device.get_physical_device_properties();
		}

		/**
		 * Return the enabled features of the Vulkan physical device (that the logical device was created from).
		 */
		const vk::PhysicalDeviceFeatures &
		get_physical_device_features()
		{
			return d_vulkan_device.get_physical_device_features();
		}

		/**
		 * Return the Vulkan logical device.
		 */
		vk::Device
		get_device()
		{
			return d_vulkan_device.get_device();
		}

		/**
		 * Return the VMA allocator.
		 *
		 * Buffer and image allocations can go through this.
		 */
		VmaAllocator
		get_vma_allocator()
		{
			return d_vulkan_device.get_vma_allocator();
		}

		/**
		 * Return the render pass used for rendering into the default framebuffer (eg, swapchain).
		 */
		vk::RenderPass
		get_default_render_pass()
		{
			return d_default_render_pass;
		}

		/**
		 * Access the command buffer for rendering into the default framebuffer (eg, swapchain)
		 * using the default render pass (with commands allowed within a render pass).
		 */
		vk::CommandBuffer
		get_default_render_pass_command_buffer()
		{
			return d_vulkan_frame.get_default_render_pass_command_buffer();
		}

	private:

		VulkanDevice &d_vulkan_device;
		VulkanFrame &d_vulkan_frame;
		vk::RenderPass d_default_render_pass;
	};
}

#endif // GPLATES_OPENGL_VULKAN_H
