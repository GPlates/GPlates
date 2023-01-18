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

#include <cstring>  // memcpy
#include <vector>

#include "MapProjectionImage.h"
#include "VulkanException.h"

#include "gui/MapProjection.h"

#include "utils/CallStackTracker.h"


void
GPlatesOpenGL::MapProjectionImage::initialise_vulkan_resources(
		Vulkan &vulkan,
		vk::CommandBuffer initialisation_command_buffer,
		vk::Fence initialisation_submit_fence)
{
	// Add this scope to the call stack trace printed if exception thrown in this scope.
	TRACK_CALL_STACK();

	// Create the sampler.
	vk::SamplerCreateInfo sampler_create_info;
	sampler_create_info
			.setMagFilter(vk::Filter::eLinear)
			.setMinFilter(vk::Filter::eLinear)
			.setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
			.setAddressModeV(vk::SamplerAddressMode::eClampToEdge);
	d_sampler = vulkan.get_device().createSampler(sampler_create_info);

	// Create the image.
	vk::ImageCreateInfo image_create_info;
	image_create_info
			.setImageType(vk::ImageType::e2D)
			.setFormat(TEXEL_FORMAT)
			.setExtent({ IMAGE_WIDTH, IMAGE_HEIGHT, 1 })
			.setMipLevels(1)
			.setArrayLayers(1)
			.setSamples(vk::SampleCountFlagBits::e1)
			.setTiling(vk::ImageTiling::eOptimal)
			.setUsage(vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst)
			.setSharingMode(vk::SharingMode::eExclusive)
			.setInitialLayout(vk::ImageLayout::eUndefined);
	// Image allocation.
	VmaAllocationCreateInfo image_allocation_create_info = {};
	image_allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;
	d_image = VulkanImage::create(
			vulkan.get_vma_allocator(),
			image_create_info,
			image_allocation_create_info,
			GPLATES_EXCEPTION_SOURCE);

	// Create image view.
	vk::ImageViewCreateInfo image_view_create_info;
	image_view_create_info
			.setImage(d_image.get_image())
			.setViewType(vk::ImageViewType::e2D)
			.setFormat(TEXEL_FORMAT)
			.setComponents({})  // identity swizzle
			.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } );
	d_image_view = vulkan.get_device().createImageView(image_view_create_info);


	//
	// Transition the image layout for optimal shader reads.
	//
	// This is only to avoid the validation layers complaining since the image can be bound for optimal shader reads
	// via a descriptor set (by a client) when the globe is active (ie, when not rendering to a map view).
	// It needs to be bound because it's "statically" used, even though the shader will not actually sample the image (since not in map view).
	//

	// Begin recording into the initialisation command buffer.
	vk::CommandBufferBeginInfo initialisation_command_buffer_begin_info;
	// Command buffer will only be submitted once.
	initialisation_command_buffer_begin_info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	initialisation_command_buffer.begin(initialisation_command_buffer_begin_info);

	// Pipeline barrier to transition to an image layout suitable for a transfer destination.
	vk::ImageMemoryBarrier pre_clear_image_memory_barrier;
	pre_clear_image_memory_barrier
			.setSrcAccessMask({})
			.setDstAccessMask(vk::AccessFlagBits::eTransferWrite)
			.setOldLayout(vk::ImageLayout::eUndefined)
			.setNewLayout(vk::ImageLayout::eTransferDstOptimal)
			.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setImage(d_image.get_image())
			.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });
	initialisation_command_buffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eTopOfPipe,  // don't need to wait to access freshly allocated memory
			vk::PipelineStageFlagBits::eTransfer,
			{},  // dependencyFlags
			{},  // memoryBarriers
			{},  // bufferMemoryBarriers
			pre_clear_image_memory_barrier);  // imageMemoryBarriers

	// Clear the image.
	//
	// This is not really necessary since a call to 'update()' will overwrite the image (when switching map projections).
	// But we have to transition the image layout for optimal shader reads anyway, so might as well clear the image also.
	const vk::ClearColorValue clear_value = std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f };
	const vk::ImageSubresourceRange sub_resource_range = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
	initialisation_command_buffer.clearColorImage(
			d_image.get_image(),
			vk::ImageLayout::eTransferDstOptimal,
			clear_value,
			sub_resource_range);

	// Pipeline barrier to transition to an image layout suitable for optimal shader reads.
	vk::ImageMemoryBarrier post_clear_image_memory_barrier;
	post_clear_image_memory_barrier
			.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
			.setDstAccessMask({})
			.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
			.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
			.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setImage(d_image.get_image())
			.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });
	initialisation_command_buffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer,
			// Don't need to wait since image won't be used until 'update()' is called (and that will wait)...
			vk::PipelineStageFlagBits::eBottomOfPipe,
			{},  // dependencyFlags
			{},  // memoryBarriers
			{},  // bufferMemoryBarriers
			post_clear_image_memory_barrier);  // imageMemoryBarriers

	// End recording into the initialisation command buffer.
	initialisation_command_buffer.end();

	// Submit the initialisation command buffer.
	vk::SubmitInfo initialisation_command_buffer_submit_info;
	initialisation_command_buffer_submit_info.setCommandBuffers(initialisation_command_buffer);
	vulkan.get_graphics_and_compute_queue().submit(initialisation_command_buffer_submit_info, initialisation_submit_fence);

	// Wait for the copy commands to finish.
	// Note: It's OK to wait since initialisation is not a performance-critical part of the code.
	if (vulkan.get_device().waitForFences(initialisation_submit_fence, true, UINT64_MAX) != vk::Result::eSuccess)
	{
		throw VulkanException(GPLATES_EXCEPTION_SOURCE, "Error waiting for initialisation of map projection image.");
	}
	vulkan.get_device().resetFences(initialisation_submit_fence);
}


void
GPlatesOpenGL::MapProjectionImage::release_vulkan_resources(
		Vulkan &vulkan)
{
	// Vulkan memory allocator.
	VmaAllocator vma_allocator = vulkan.get_vma_allocator();

	vulkan.get_device().destroyImageView(d_image_view);
	VulkanImage::destroy(vma_allocator, d_image);

	vulkan.get_device().destroySampler(d_sampler);

	for (auto staging_buffer_entry : d_staging_buffers)
	{
		StagingBuffer &staging_buffer = staging_buffer_entry.second;

		VulkanBuffer::destroy(vma_allocator, staging_buffer.forward_transform_buffer);
	}

	// Clear all our staging buffers so that they'll get recreated and repopulated
	// (that is, if 'initialise_vulkan_resources()' is called again, eg, due to a lost device).
	d_staging_buffers.clear();
}


vk::DescriptorImageInfo
GPlatesOpenGL::MapProjectionImage::get_descriptor_image_info() const
{
	return { d_sampler, d_image_view, vk::ImageLayout::eShaderReadOnlyOptimal };
}


void
GPlatesOpenGL::MapProjectionImage::update(
		Vulkan &vulkan,
		vk::CommandBuffer preprocess_command_buffer,
		const GPlatesGui::MapProjection &map_projection)
{
	// If first time visiting current map projection *type* then create and fill a staging buffer for it.
	//
	// Note: changing the central meridian does not change the staging buffer contents (see 'create_staging_buffer()' for details).
	const auto map_projection_type = map_projection.get_projection_settings().get_projection_type();
	if (d_staging_buffers.find(map_projection_type) == d_staging_buffers.end())
	{
		d_staging_buffers[map_projection_type] = create_staging_buffer(vulkan, map_projection);
	}

	// Get the staging buffer associated with the map projection *type*.
	StagingBuffer &staging_buffer = d_staging_buffers[map_projection_type];

	// Pipeline barrier to wait for any commands that read from the image before we copy new data into it.
	// And also transition to an image layout suitable for a transfer destination.
	vk::ImageMemoryBarrier pre_update_image_memory_barrier;
	pre_update_image_memory_barrier
			.setSrcAccessMask({})  // no writes to make available (image is only read from)
			.setDstAccessMask(vk::AccessFlagBits::eTransferWrite)
			.setOldLayout(vk::ImageLayout::eUndefined)  // not interested in retaining contents of image
			.setNewLayout(vk::ImageLayout::eTransferDstOptimal)
			.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setImage(d_image.get_image())
			.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });
	preprocess_command_buffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eAllCommands,
			vk::PipelineStageFlagBits::eTransfer,
			{},  // dependencyFlags
			{},  // memoryBarriers
			{},  // bufferMemoryBarriers
			pre_update_image_memory_barrier);  // imageMemoryBarriers

	// Copy image data from staging buffer to image.
	vk::BufferImageCopy buffer_image_copy;
	buffer_image_copy
			.setBufferOffset(0)
			.setBufferRowLength(0)
			.setBufferImageHeight(0)
			.setImageSubresource({vk::ImageAspectFlagBits::eColor, 0, 0, 1})
			.setImageOffset({0, 0, 0})
			.setImageExtent({IMAGE_WIDTH, IMAGE_HEIGHT, 1});
	preprocess_command_buffer.copyBufferToImage(
			staging_buffer.forward_transform_buffer.get_buffer(),
			d_image.get_image(),
			vk::ImageLayout::eTransferDstOptimal,
			buffer_image_copy);

	// Pipeline barrier to wait for staging transfer writes to be made visible for image reads from any shader.
	// And also transition to an image layout suitable for shader reads.
	vk::ImageMemoryBarrier post_update_image_memory_barrier;
	post_update_image_memory_barrier
			.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
			.setDstAccessMask(vk::AccessFlagBits::eShaderRead)
			.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
			.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
			.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setImage(d_image.get_image())
			.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });
	preprocess_command_buffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eAllCommands,
			{},  // dependencyFlags
			{},  // memoryBarriers
			{},  // bufferMemoryBarriers
			post_update_image_memory_barrier);  // imageMemoryBarriers
}


GPlatesOpenGL::MapProjectionImage::StagingBuffer
GPlatesOpenGL::MapProjectionImage::create_staging_buffer(
		Vulkan &vulkan,
		const GPlatesGui::MapProjection &map_projection)
{
	//
	// Create a staging buffer for the current map projection *type*.
	//

	StagingBuffer staging_buffer = {};

	vk::BufferCreateInfo staging_buffer_create_info;
	staging_buffer_create_info
			.setSize(IMAGE_WIDTH * IMAGE_WIDTH * sizeof(Texel))
			.setUsage(vk::BufferUsageFlagBits::eTransferSrc)
			.setSharingMode(vk::SharingMode::eExclusive);
	VmaAllocationCreateInfo staging_buffer_allocation_create_info = {};
	staging_buffer_allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;
	staging_buffer_allocation_create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT; // host mappable
	staging_buffer.forward_transform_buffer = VulkanBuffer::create(
			vulkan.get_vma_allocator(),
			staging_buffer_create_info,
			staging_buffer_allocation_create_info,
			GPLATES_EXCEPTION_SOURCE);

	//
	// Update the staging buffer.
	//
	// Note: The central meridian does not affect the staging buffer contents because the buffer contains
	//       map projected values (output of the map projection forward transform) and these values
	//       are independent of the central meridian (see below).
	//

	// Map staging buffer.
	void *const staging_buffer_forward_transform_mapped_pointer =
			staging_buffer.forward_transform_buffer.map_memory(vulkan.get_vma_allocator(), GPLATES_EXCEPTION_SOURCE);

	std::vector<Texel> row_of_texels;
	row_of_texels.reserve(IMAGE_WIDTH);
	for (unsigned int row = 0; row < IMAGE_HEIGHT; ++row)
	{
		const double latitude = -90 + row * TEXEL_INTERVAL_IN_DEGREES;

		for (unsigned int column = 0; column < IMAGE_WIDTH; ++column)
		{
			// Make sure the longitude is within [-180+epsilon, 180-epsilon] around the central meridian longitude.
			const double epsilon = 1e-6;
			//
			// This is to prevent subsequent map projection from wrapping (-180 -> +180 or vice versa) due to
			// the map projection code receiving a longitude value slightly outside that range or the map
			// projection code itself having numerical precision issues.
			//
			// NOTE: Even though we are specifying the central meridian here, it does not affect the map projection output
			//       (of the forward transform). In other words, the map projection could have any central meridian
			//       and the output would be the same. This is because the map projection output (x, y) is centred
			//       such that longitude=central_meridian maps to x=0. Essentially we're adding the central meridian
			//       here and then the map projection forward transform is subtracting it (essentially removing it).
			double longitude = map_projection.central_meridian();
			if (column == 0)
			{
				longitude += -180 + epsilon;
			}
			else if (column == IMAGE_WIDTH - 1)
			{
				longitude += 180 - epsilon;
			}
			else
			{
				longitude += -180 + column * TEXEL_INTERVAL_IN_DEGREES;
			}

			double x = longitude;
			double y = latitude;
			map_projection.forward_transform(x, y);

			row_of_texels[column] = { static_cast<float>(x), static_cast<float>(y) };
		}

		// Copy the row into the staging buffer.
		std::memcpy(
				// Pointer to row within mapped staging buffer...
				static_cast<Texel *>(staging_buffer_forward_transform_mapped_pointer) + row * IMAGE_WIDTH,
				row_of_texels.data(),
				IMAGE_WIDTH * sizeof(Texel));
	}

	// Flush and unmap staging buffer.
	staging_buffer.forward_transform_buffer.flush_mapped_memory(vulkan.get_vma_allocator(), 0, VK_WHOLE_SIZE, GPLATES_EXCEPTION_SOURCE);
	staging_buffer.forward_transform_buffer.unmap_memory(vulkan.get_vma_allocator());

	return staging_buffer;
}
