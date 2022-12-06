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

#include "VulkanBuffer.h"

#include "VulkanException.h"


GPlatesOpenGL::VulkanBuffer::VulkanBuffer() :
	d_buffer(VK_NULL_HANDLE),
	d_allocation(VK_NULL_HANDLE),
	d_is_host_visible_and_non_coherent(false)
{
}


GPlatesOpenGL::VulkanBuffer::VulkanBuffer(
		VulkanDevice &vulkan_device,
		const vk::BufferCreateInfo &buffer_create_info,
		const VmaAllocationCreateInfo &allocation_create_info,
		const GPlatesUtils::CallStack::Trace &caller_location) :
	VulkanBuffer()
{
	if (vmaCreateBuffer(
			vulkan_device.get_vma_allocator(),
			&static_cast<const VkBufferCreateInfo &>(buffer_create_info),
			&allocation_create_info,
			&d_buffer,
			&d_allocation,
			nullptr) != VK_SUCCESS)
	{
		throw VulkanException(caller_location, "Failed to create buffer.");
	}

	// See if allocation is host visible and non-coherent.
	VkMemoryPropertyFlags memory_property_flags;
	vmaGetAllocationMemoryProperties(vulkan_device.get_vma_allocator(), d_allocation, &memory_property_flags);
	if ((memory_property_flags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) ==
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
	{
		d_is_host_visible_and_non_coherent = true;
	}
}


void
GPlatesOpenGL::VulkanBuffer::destroy(
		VulkanDevice &vulkan_device,
		VulkanBuffer &buffer)
{
	vmaDestroyBuffer(vulkan_device.get_vma_allocator(), buffer.d_buffer, buffer.d_allocation);

	buffer.d_buffer = VK_NULL_HANDLE;
	buffer.d_allocation = VK_NULL_HANDLE;
	buffer.d_is_host_visible_and_non_coherent = false;
}


void *
GPlatesOpenGL::VulkanBuffer::map_memory(
		VulkanDevice &vulkan_device,
		const GPlatesUtils::CallStack::Trace &caller_location)
{
	void *data;
	if (vmaMapMemory(vulkan_device.get_vma_allocator(), d_allocation, &data) != VK_SUCCESS)
	{
		throw VulkanException(caller_location, "Failed to map buffer memory.");
	}

	return data;
}


void
GPlatesOpenGL::VulkanBuffer::flush_mapped_memory(
		VulkanDevice &vulkan_device,
		vk::DeviceSize offset,
		vk::DeviceSize size,
		const GPlatesUtils::CallStack::Trace &caller_location)
{
	if (d_is_host_visible_and_non_coherent)
	{
		if (vmaFlushAllocation(vulkan_device.get_vma_allocator(), d_allocation, offset, size) != VK_SUCCESS)
		{
			throw VulkanException(caller_location, "Failed to flush mapped buffer memory.");
		}
	}
}


void
GPlatesOpenGL::VulkanBuffer::invalidate_mapped_memory(
		VulkanDevice &vulkan_device,
		vk::DeviceSize offset,
		vk::DeviceSize size,
		const GPlatesUtils::CallStack::Trace &caller_location)
{
	if (d_is_host_visible_and_non_coherent)
	{
		if (vmaInvalidateAllocation(vulkan_device.get_vma_allocator(), d_allocation, offset, size) != VK_SUCCESS)
		{
			throw VulkanException(caller_location, "Failed to invalidate mapped buffer memory.");
		}
	}
}


void
GPlatesOpenGL::VulkanBuffer::unmap_memory(
		VulkanDevice &vulkan_device)
{
	vmaUnmapMemory(vulkan_device.get_vma_allocator(), d_allocation);
}
