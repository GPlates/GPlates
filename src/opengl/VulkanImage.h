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

#ifndef GPLATES_OPENGL_VULKANIMAGE_H
#define GPLATES_OPENGL_VULKANIMAGE_H

#include "VulkanHpp.h"
#include "VulkanMemoryAllocator.h"

#include "utils/CallStackTracker.h"


namespace GPlatesOpenGL
{
	/**
	 * Convenience wrapper around a vk::Image and its associated VmaAllocation (allocated using VmaAllocator).
	 */
	class VulkanImage
	{
	public:

		/**
		 * Create a vk:Image (and bind it to an allocated VmaAllocation).
		 */
		static
		VulkanImage
		create(
				VmaAllocator vma_allocator,
				const vk::ImageCreateInfo &image_create_info,
				const VmaAllocationCreateInfo &allocation_create_info,
				const GPlatesUtils::CallStack::Trace &caller_location)
		{
			return VulkanImage(vma_allocator, image_create_info, allocation_create_info, caller_location);
		}

		/**
		 * Destroy a vk:Image (and free its associated VmaAllocation).
		 *
		 * Note that the image and allocation handles can be VK_NULL_HANDLE (in which case nothing happens).
		 */
		static
		void
		destroy(
				VmaAllocator vma_allocator,
				VulkanImage &image);


		//! Construct a VK_NULL_HANDLE image (and associated VK_NULL_HANDLE memory allocation).
		VulkanImage();


		void *
		map_memory(
				VmaAllocator vma_allocator,
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
				VmaAllocator vma_allocator,
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
				VmaAllocator vma_allocator,
				vk::DeviceSize offset,
				vk::DeviceSize size,
				const GPlatesUtils::CallStack::Trace &caller_location);

		void
		unmap_memory(
				VmaAllocator vma_allocator);


		vk::Image
		get_image()
		{
			return d_image;
		}

		VmaAllocation
		get_allocation()
		{
			return d_allocation;
		}

	private:

		VulkanImage(
				VmaAllocator vma_allocator,
				const vk::ImageCreateInfo &image_create_info,
				const VmaAllocationCreateInfo &allocation_create_info,
				const GPlatesUtils::CallStack::Trace &caller_location);

		VkImage d_image;
		VmaAllocation d_allocation;

		// True if memory allocation is host visible and non-coherent.
		bool d_is_host_visible_and_non_coherent;
	};
}

#endif // GPLATES_OPENGL_VULKANIMAGE_H
