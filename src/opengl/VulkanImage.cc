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

#include "VulkanImage.h"

#include "VulkanException.h"


GPlatesOpenGL::VulkanImage::VulkanImage() :
	d_image(VK_NULL_HANDLE),
	d_allocation(VK_NULL_HANDLE),
	d_is_host_visible_and_non_coherent(false)
{
}


GPlatesOpenGL::VulkanImage::VulkanImage(
		VmaAllocator vma_allocator,
		const vk::ImageCreateInfo &image_create_info,
		const VmaAllocationCreateInfo &allocation_create_info,
		const GPlatesUtils::CallStack::Trace &caller_location) :
	VulkanImage()
{
	if (vmaCreateImage(
			vma_allocator,
			&static_cast<const VkImageCreateInfo &>(image_create_info),
			&allocation_create_info,
			&d_image,
			&d_allocation,
			nullptr) != VK_SUCCESS)
	{
		throw VulkanException(caller_location, "Failed to create image.");
	}

	// See if allocation is host visible and non-coherent.
	VkMemoryPropertyFlags memory_property_flags;
	vmaGetAllocationMemoryProperties(vma_allocator, d_allocation, &memory_property_flags);
	if ((memory_property_flags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) ==
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
	{
		d_is_host_visible_and_non_coherent = true;
	}
}


void
GPlatesOpenGL::VulkanImage::destroy(
		VmaAllocator vma_allocator,
		VulkanImage &image)
{
	vmaDestroyImage(vma_allocator, image.d_image, image.d_allocation);

	image.d_image = VK_NULL_HANDLE;
	image.d_allocation = VK_NULL_HANDLE;
	image.d_is_host_visible_and_non_coherent = false;
}


void *
GPlatesOpenGL::VulkanImage::map_memory(
		VmaAllocator vma_allocator,
		const GPlatesUtils::CallStack::Trace &caller_location)
{
	void *data;
	if (vmaMapMemory(vma_allocator, d_allocation, &data) != VK_SUCCESS)
	{
		throw VulkanException(caller_location, "Failed to map image memory.");
	}

	return data;
}


void
GPlatesOpenGL::VulkanImage::flush_mapped_memory(
		VmaAllocator vma_allocator,
		vk::DeviceSize offset,
		vk::DeviceSize size,
		const GPlatesUtils::CallStack::Trace &caller_location)
{
	if (d_is_host_visible_and_non_coherent)
	{
		if (vmaFlushAllocation(vma_allocator, d_allocation, offset, size) != VK_SUCCESS)
		{
			throw VulkanException(caller_location, "Failed to flush mapped image memory.");
		}
	}
}


void
GPlatesOpenGL::VulkanImage::invalidate_mapped_memory(
		VmaAllocator vma_allocator,
		vk::DeviceSize offset,
		vk::DeviceSize size,
		const GPlatesUtils::CallStack::Trace &caller_location)
{
	if (d_is_host_visible_and_non_coherent)
	{
		if (vmaInvalidateAllocation(vma_allocator, d_allocation, offset, size) != VK_SUCCESS)
		{
			throw VulkanException(caller_location, "Failed to invalidate mapped image memory.");
		}
	}
}


void
GPlatesOpenGL::VulkanImage::unmap_memory(
		VmaAllocator vma_allocator)
{
	vmaUnmapMemory(vma_allocator, d_allocation);
}
