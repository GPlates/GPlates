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

//
// The Vulkan-Hpp C++ interface (around the Vulkan C interface).
//
#include "VulkanHpp.h"


namespace GPlatesOpenGL
{
	/**
	 * Central place for most clients to access the Vulkan (logical) device and asynchronous frames.
	 */
	class Vulkan
	{
	public:

		Vulkan(
				VulkanDevice &vulkan_device,
				VulkanFrame &vulkan_frame);


		//
		// Vulkan instance and physical device.
		//


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


		//
		// Vulkan logical device.
		//


		/**
		 * Return the Vulkan logical device.
		 */
		vk::Device
		get_device()
		{
			return d_vulkan_device.get_device();
		}

		/**
		 * Return the graphics+compute queue family.
		 */
		std::uint32_t
		get_graphics_and_compute_queue_family() const
		{
			return d_vulkan_device.get_graphics_and_compute_queue_family();
		}

		/**
		 * Return the graphics+compute queue.
		 *
		 * Note that this queue can also be used for transfer operations.
		 */
		vk::Queue
		get_graphics_and_compute_queue()
		{
			return d_vulkan_device.get_graphics_and_compute_queue();
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
		static constexpr unsigned int NUM_ASYNC_FRAMES = VulkanFrame::NUM_ASYNC_FRAMES;

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
		next_frame()
		{
			return d_vulkan_frame.next_frame(get_device());
		}

		/**
		 * The current frame *number*.
		 *
		 * If the current frame number is "N" then the device (GPU) has finished rendering frame "N - NUM_ASYNC_FRAMES".
		 *
		 * This means clients should buffer NUM_ASYNC_FRAMES worth of dynamic resources to ensure
		 * they do not modify resources that the device (GPU) is still using.
		 * An example is the host (CPU) recording into command buffers that the device (GPU) is still using.
		 */
		std::int64_t
		get_frame_number() const
		{
			return d_vulkan_frame.get_frame_number();
		}

		/**
		 * The frame *index* is in the range [0, NUM_ASYNC_FRAMES - 1].
		 *
		 * The resources at this index are no longer in use by the device (GPU) and can safely be re-used.
		 *
		 * It's value is "get_frame_number() % NUM_ASYNC_FRAMES" and can be used by clients to index
		 * index their own buffer of resources (eg, an array of size NUM_ASYNC_FRAMES).
		 */
		unsigned int
		get_frame_index() const
		{
			return d_vulkan_frame.get_frame_index();
		}

	private:

		VulkanDevice &d_vulkan_device;
		VulkanFrame &d_vulkan_frame;
	};
}

#endif // GPLATES_OPENGL_VULKAN_H
