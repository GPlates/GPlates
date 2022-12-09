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

#ifndef GPLATES_OPENGL_VULKANBUFFER_H
#define GPLATES_OPENGL_VULKANBUFFER_H

#include "Vulkan.h"

#include "utils/CallStackTracker.h"


namespace GPlatesOpenGL
{
	/**
	 * Convenience wrapper around a vk::Buffer and its associated VmaAllocation (allocated using VmaAllocator).
	 */
	class VulkanBuffer
	{
	public:

		/**
		 * Create a vk:Buffer (and bind it to an allocated VmaAllocation).
		 */
		static
		VulkanBuffer
		create(
				Vulkan &vulkan,
				const vk::BufferCreateInfo &buffer_create_info,
				const VmaAllocationCreateInfo &allocation_create_info,
				const GPlatesUtils::CallStack::Trace &caller_location)
		{
			return VulkanBuffer(vulkan, buffer_create_info, allocation_create_info, caller_location);
		}

		/**
		 * Destroy a vk:Buffer (and free its associated VmaAllocation).
		 *
		 * Note that the buffer and allocation handles can be VK_NULL_HANDLE (in which case nothing happens).
		 */
		static
		void
		destroy(
				Vulkan &vulkan,
				VulkanBuffer &buffer);


		//! Construct a VK_NULL_HANDLE buffer (and associated VK_NULL_HANDLE memory allocation).
		VulkanBuffer();


		void *
		map_memory(
				Vulkan &vulkan,
				const GPlatesUtils::CallStack::Trace &caller_location);

		/**
		 * Flushes the specified range of non-coherent memory from host cache.
		 *
		 * Note that this only flushes if the memory allocation is host visible and non-coherent.
		 *
		 * Also note that @a offset and @a size are internally rounded to VkPhysicalDeviceLimits::nonCoherentAtomSize (by VMA).
		 */
		void
		flush_mapped_memory(
				Vulkan &vulkan,
				vk::DeviceSize offset,
				vk::DeviceSize size,
				const GPlatesUtils::CallStack::Trace &caller_location);

		/**
		 * Invalidates the specified range of non-coherent memory from host cache.
		 *
		 * Note that this only invalidates if the memory allocation is host visible and non-coherent.
		 *
		 * Also note that @a offset and @a size are internally rounded to VkPhysicalDeviceLimits::nonCoherentAtomSize (by VMA).
		 */
		void
		invalidate_mapped_memory(
				Vulkan &vulkan,
				vk::DeviceSize offset,
				vk::DeviceSize size,
				const GPlatesUtils::CallStack::Trace &caller_location);

		void
		unmap_memory(
				Vulkan &vulkan);


		vk::Buffer
		get_buffer()
		{
			return d_buffer;
		}

		VmaAllocation
		get_allocation()
		{
			return d_allocation;
		}

	private:

		VulkanBuffer(
				Vulkan &vulkan,
				const vk::BufferCreateInfo &buffer_create_info,
				const VmaAllocationCreateInfo &allocation_create_info,
				const GPlatesUtils::CallStack::Trace &caller_location);

		VkBuffer d_buffer;
		VmaAllocation d_allocation;

		// True if memory allocation is host visible and non-coherent.
		bool d_is_host_visible_and_non_coherent;
	};
}

#endif // GPLATES_OPENGL_VULKANBUFFER_H
