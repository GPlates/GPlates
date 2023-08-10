/**
 * Copyright (C) 2023 The University of Sydney, Australia
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

#include "SceneTile.h"
#include "VulkanException.h"

#include "utils/CallStackTracker.h"


namespace
{
	//
	// The allowed number of scene fragments that can be rendered per pixel on average.
	//
	// Some pixels will have more fragments that this and most will have less.
	// As long as the average over all screen pixels is less than this then we have allocated enough storage.
	//
	const unsigned int AVERAGE_FRAGMENTS_PER_PIXEL = 4;
}


GPlatesOpenGL::SceneTile::SceneTile() :
	d_num_fragments_in_storage(0)
{
}


void
GPlatesOpenGL::SceneTile::initialise_vulkan_resources(
		Vulkan &vulkan,
		vk::CommandBuffer initialisation_command_buffer,
		vk::Fence initialisation_submit_fence)
{
	// Add this scope to the call stack trace printed if exception thrown in this scope.
	TRACK_CALL_STACK();

	// Image and allocation create info parameters for fragment list head image.
	vk::ImageCreateInfo fragment_list_head_image_create_info;
	fragment_list_head_image_create_info
			.setImageType(vk::ImageType::e2D)
			.setFormat(FRAGMENT_LIST_HEAD_IMAGE_TEXEL_FORMAT)
			.setExtent({ FRAGMENT_TILE_DIMENSION, FRAGMENT_TILE_DIMENSION, 1 })
			.setMipLevels(1)
			.setArrayLayers(1)
			.setSamples(vk::SampleCountFlagBits::e1)
			.setTiling(vk::ImageTiling::eOptimal)
			.setUsage(vk::ImageUsageFlagBits::eStorage)
			.setSharingMode(vk::SharingMode::eExclusive)
			.setInitialLayout(vk::ImageLayout::eUndefined);
	// Image allocation.
	VmaAllocationCreateInfo fragment_list_head_image_allocation_create_info = {};
	fragment_list_head_image_allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;
	fragment_list_head_image_allocation_create_info.flags = 0;  // device local memory

	// Create the fragment list head image.
	d_fragment_list_head_image = VulkanImage::create(
			vulkan.get_vma_allocator(),
			fragment_list_head_image_create_info,
			fragment_list_head_image_allocation_create_info,
			GPLATES_EXCEPTION_SOURCE);

	// Image view create info parameters for fragment list head image.
	vk::ImageViewCreateInfo fragment_list_head_image_view_create_info;
	fragment_list_head_image_view_create_info
			.setImage(d_fragment_list_head_image.get_image())
			.setViewType(vk::ImageViewType::e2D)
			.setFormat(FRAGMENT_LIST_HEAD_IMAGE_TEXEL_FORMAT)
			.setComponents({})  // identity swizzle
			.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

	// Create the fragment list head image view.
	d_fragment_list_head_image_view = vulkan.get_device().createImageView(fragment_list_head_image_view_create_info);


	// Determine size of fragment storage buffer and ensure it doesn't exceed maximum storage buffer range.
	//
	// Note: Vulkan guarantees support of at least 128MB for the maximum storage buffer range.
	d_num_fragments_in_storage = FRAGMENT_TILE_DIMENSION * FRAGMENT_TILE_DIMENSION * AVERAGE_FRAGMENTS_PER_PIXEL;
	if (d_num_fragments_in_storage * NUM_BYTES_PER_FRAGMENT > vulkan.get_physical_device_properties().limits.maxStorageBufferRange)
	{
		d_num_fragments_in_storage = vulkan.get_physical_device_properties().limits.maxStorageBufferRange / NUM_BYTES_PER_FRAGMENT;
	}

	// Buffer and allocation create info parameters for fragment storage buffer.
	vk::BufferCreateInfo fragment_storage_buffer_create_info;
	fragment_storage_buffer_create_info
			.setSize(vk::DeviceSize(d_num_fragments_in_storage) * NUM_BYTES_PER_FRAGMENT)
			.setUsage(vk::BufferUsageFlagBits::eStorageBuffer)
			.setSharingMode(vk::SharingMode::eExclusive);
	VmaAllocationCreateInfo fragment_storage_buffer_allocation_create_info = {};
	fragment_storage_buffer_allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;
	fragment_storage_buffer_allocation_create_info.flags = 0;  // device local memory

	// Create the fragment storage buffer.
	d_fragment_storage_buffer = VulkanBuffer::create(
			vulkan.get_vma_allocator(),
			fragment_storage_buffer_create_info,
			fragment_storage_buffer_allocation_create_info,
			GPLATES_EXCEPTION_SOURCE);


	// Buffer and allocation create info parameters for fragment allocator buffer.
	vk::BufferCreateInfo fragment_allocator_buffer_create_info;
	fragment_allocator_buffer_create_info
			.setSize(4)  // buffer contains a single uint
			.setUsage(vk::BufferUsageFlagBits::eStorageBuffer)
			.setSharingMode(vk::SharingMode::eExclusive);
	VmaAllocationCreateInfo fragment_allocator_buffer_allocation_create_info = {};
	fragment_allocator_buffer_allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;
	fragment_allocator_buffer_allocation_create_info.flags = 0;  // device local memory

	// Create the fragment allocator buffer.
	d_fragment_allocator_buffer = VulkanBuffer::create(
			vulkan.get_vma_allocator(),
			fragment_allocator_buffer_create_info,
			fragment_allocator_buffer_allocation_create_info,
			GPLATES_EXCEPTION_SOURCE);
}


void
GPlatesOpenGL::SceneTile::release_vulkan_resources(
		Vulkan &vulkan)
{
	// Vulkan memory allocator.
	VmaAllocator vma_allocator = vulkan.get_vma_allocator();

	vulkan.get_device().destroyImageView(d_fragment_list_head_image_view);
	VulkanImage::destroy(vma_allocator, d_fragment_list_head_image);

	VulkanBuffer::destroy(vma_allocator, d_fragment_storage_buffer);
	VulkanBuffer::destroy(vma_allocator, d_fragment_allocator_buffer);

	d_num_fragments_in_storage = 0;
}


std::vector<vk::WriteDescriptorSet>
GPlatesOpenGL::SceneTile::get_write_descriptor_sets(
		vk::DescriptorSet descriptor_set,
		std::uint32_t binding) const
{
	// Fragment list head image.
	vk::DescriptorImageInfo fragment_list_head_descriptor_image_info;
	fragment_list_head_descriptor_image_info
			.setImageView(d_fragment_list_head_image_view)
			.setImageLayout(vk::ImageLayout::eGeneral);
	vk::WriteDescriptorSet fragment_list_head_write_descriptor_set;
	fragment_list_head_write_descriptor_set
			.setDstSet(descriptor_set)
			.setDstBinding(binding)
			.setDstArrayElement(0)
			.setDescriptorType(vk::DescriptorType::eStorageImage)
			.setImageInfo(fragment_list_head_descriptor_image_info);

	// Fragment storage buffer.
	vk::DescriptorBufferInfo fragment_storage_descriptor_buffer_info;
	fragment_storage_descriptor_buffer_info
			.setBuffer(d_fragment_storage_buffer.get_buffer())
			.setOffset(vk::DeviceSize(0))
			.setRange(VK_WHOLE_SIZE);
	vk::WriteDescriptorSet fragment_storage_write_descriptor_set;
	fragment_storage_write_descriptor_set
			.setDstSet(descriptor_set)
			.setDstBinding(binding + 1)
			.setDstArrayElement(0)
			.setDescriptorType(vk::DescriptorType::eStorageBuffer)
			.setBufferInfo(fragment_storage_descriptor_buffer_info);

	// Fragment allocator buffer.
	vk::DescriptorBufferInfo fragment_allocator_descriptor_buffer_info;
	fragment_allocator_descriptor_buffer_info
			.setBuffer(d_fragment_allocator_buffer.get_buffer())
			.setOffset(vk::DeviceSize(0))
			.setRange(VK_WHOLE_SIZE);
	vk::WriteDescriptorSet fragment_allocator_write_descriptor_set;
	fragment_allocator_write_descriptor_set
			.setDstSet(descriptor_set)
			.setDstBinding(binding + 2)
			.setDstArrayElement(0)
			.setDescriptorType(vk::DescriptorType::eStorageBuffer)
			.setBufferInfo(fragment_allocator_descriptor_buffer_info);

	return {
			fragment_list_head_write_descriptor_set,
			fragment_storage_write_descriptor_set,
			fragment_allocator_write_descriptor_set };
}


std::vector<vk::DescriptorSetLayoutBinding>
GPlatesOpenGL::SceneTile::get_descriptor_set_layout_bindings(
		std::uint32_t binding,
		vk::ShaderStageFlags shader_stage_flags) const
{
	// Fragment list head image.
	vk::DescriptorSetLayoutBinding fragment_list_head_descriptor_set_layout_binding;
	fragment_list_head_descriptor_set_layout_binding
			.setBinding(binding)
			.setDescriptorType(vk::DescriptorType::eStorageImage)
			.setDescriptorCount(1)
			.setStageFlags(shader_stage_flags);

	// Fragment storage buffer.
	vk::DescriptorSetLayoutBinding fragment_storage_descriptor_set_layout_binding;
	fragment_storage_descriptor_set_layout_binding
			.setBinding(binding + 1)
			.setDescriptorType(vk::DescriptorType::eStorageBuffer)
			.setDescriptorCount(1)
			.setStageFlags(shader_stage_flags);

	// Fragment allocator buffer.
	vk::DescriptorSetLayoutBinding fragment_allocator_descriptor_set_layout_binding;
	fragment_allocator_descriptor_set_layout_binding
			.setBinding(binding + 2)
			.setDescriptorType(vk::DescriptorType::eStorageBuffer)
			.setDescriptorCount(1)
			.setStageFlags(shader_stage_flags);

	return {
			fragment_list_head_descriptor_set_layout_binding,
			fragment_storage_descriptor_set_layout_binding,
			fragment_allocator_descriptor_set_layout_binding };
}


std::vector<vk::DescriptorPoolSize>
GPlatesOpenGL::SceneTile::get_descriptor_pool_sizes() const
{
	// Fragment list head image.
	vk::DescriptorPoolSize image_descriptor_pool_size;
	image_descriptor_pool_size
			.setType(vk::DescriptorType::eStorageImage)
			.setDescriptorCount(1);

	// Fragment storage buffer and fragment allocator buffer.
	vk::DescriptorPoolSize buffer_descriptor_pool_size;
	buffer_descriptor_pool_size
			.setType(vk::DescriptorType::eStorageBuffer)
			.setDescriptorCount(2);

	return { image_descriptor_pool_size, buffer_descriptor_pool_size };
}


void
GPlatesOpenGL::SceneTile::clear(
		Vulkan &vulkan,
		vk::CommandBuffer preprocess_command_buffer)
{
}
