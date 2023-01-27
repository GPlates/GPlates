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

	// Image and allocation create info parameters common to both images (forward transform and Jacobian matrix).
	vk::ImageCreateInfo image_create_info;
	image_create_info
			.setImageType(vk::ImageType::e2D)
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

	// Create the forward transform image.
	image_create_info.setFormat(FORWARD_TRANSFORM_TEXEL_FORMAT);
	d_forward_transform_image = VulkanImage::create(
			vulkan.get_vma_allocator(),
			image_create_info,
			image_allocation_create_info,
			GPLATES_EXCEPTION_SOURCE);

	// Create the Jacobian matrix image.
	image_create_info.setFormat(JACOBIAN_MATRIX_TEXEL_FORMAT);
	d_jacobian_matrix_image = VulkanImage::create(
			vulkan.get_vma_allocator(),
			image_create_info,
			image_allocation_create_info,
			GPLATES_EXCEPTION_SOURCE);

	// Image view create info parameters common to both image views (forward transform and Jacobian matrix).
	vk::ImageViewCreateInfo image_view_create_info;
	image_view_create_info
			.setViewType(vk::ImageViewType::e2D)
			.setComponents({})  // identity swizzle
			.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } );

	// Create forward transform image view.
	image_view_create_info
			.setImage(d_forward_transform_image.get_image())
			.setFormat(FORWARD_TRANSFORM_TEXEL_FORMAT);
	d_forward_transform_image_view = vulkan.get_device().createImageView(image_view_create_info);

	// Create Jacobian matrix image view.
	image_view_create_info
			.setImage(d_jacobian_matrix_image.get_image())
			.setFormat(JACOBIAN_MATRIX_TEXEL_FORMAT);
	d_jacobian_matrix_image_view = vulkan.get_device().createImageView(image_view_create_info);


	//
	// Transition the both image layouts (forward transform and Jacobian matrix) for optimal shader reads.
	//
	// This is only to avoid the validation layers complaining since an image can be bound for optimal shader reads
	// via a descriptor set (by a client) when the globe is active (ie, when not rendering to a map view).
	// It needs to be bound because it's "statically" used, even though the shader will not actually sample the image (since not in map view).
	//

	// Begin recording into the initialisation command buffer.
	vk::CommandBufferBeginInfo initialisation_command_buffer_begin_info;
	// Command buffer will only be submitted once.
	initialisation_command_buffer_begin_info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	initialisation_command_buffer.begin(initialisation_command_buffer_begin_info);

	//
	// Pipeline barrier to transition both images to an image layout suitable for a transfer destination.
	//

	vk::ImageMemoryBarrier pre_clear_image_memory_barrier;
	pre_clear_image_memory_barrier
			.setSrcAccessMask({})
			.setDstAccessMask(vk::AccessFlagBits::eTransferWrite)
			.setOldLayout(vk::ImageLayout::eUndefined)
			.setNewLayout(vk::ImageLayout::eTransferDstOptimal)
			.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

	vk::ImageMemoryBarrier pre_clear_forward_transform_image_memory_barrier(pre_clear_image_memory_barrier);
	pre_clear_forward_transform_image_memory_barrier.setImage(d_forward_transform_image.get_image());

	vk::ImageMemoryBarrier pre_clear_jacobian_matrix_image_memory_barrier(pre_clear_image_memory_barrier);
	pre_clear_jacobian_matrix_image_memory_barrier.setImage(d_jacobian_matrix_image.get_image());

	initialisation_command_buffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eTopOfPipe,  // don't need to wait to access freshly allocated memory
			vk::PipelineStageFlagBits::eTransfer,
			{},  // dependencyFlags
			{},  // memoryBarriers
			{},  // bufferMemoryBarriers
			{ pre_clear_forward_transform_image_memory_barrier, pre_clear_jacobian_matrix_image_memory_barrier });  // imageMemoryBarriers

	//
	// Clear both images.
	//
	// This is not really necessary since a call to 'update()' will overwrite these images (when switching map projections).
	// But we have to transition to an image layout for optimal shader reads anyway, so might as well clear the images also.
	const vk::ClearColorValue clear_value = std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f };
	const vk::ImageSubresourceRange sub_resource_range = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
	initialisation_command_buffer.clearColorImage(
			d_forward_transform_image.get_image(),
			vk::ImageLayout::eTransferDstOptimal,
			clear_value,
			sub_resource_range);
	initialisation_command_buffer.clearColorImage(
			d_jacobian_matrix_image.get_image(),
			vk::ImageLayout::eTransferDstOptimal,
			clear_value,
			sub_resource_range);

	//
	// Pipeline barrier to transition both images to an image layout suitable for optimal shader reads.
	//

	vk::ImageMemoryBarrier post_clear_image_memory_barrier;
	post_clear_image_memory_barrier
			.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
			.setDstAccessMask({})
			.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
			.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
			.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

	vk::ImageMemoryBarrier post_clear_forward_transform_image_memory_barrier(post_clear_image_memory_barrier);
	post_clear_forward_transform_image_memory_barrier.setImage(d_forward_transform_image.get_image());

	vk::ImageMemoryBarrier post_clear_jacobian_matrix_image_memory_barrier(post_clear_image_memory_barrier);
	post_clear_jacobian_matrix_image_memory_barrier.setImage(d_jacobian_matrix_image.get_image());

	initialisation_command_buffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer,
			// Don't need to wait since image won't be used until 'update()' is called (and that will wait)...
			vk::PipelineStageFlagBits::eBottomOfPipe,
			{},  // dependencyFlags
			{},  // memoryBarriers
			{},  // bufferMemoryBarriers
			{ post_clear_forward_transform_image_memory_barrier, post_clear_jacobian_matrix_image_memory_barrier });  // imageMemoryBarriers

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

	vulkan.get_device().destroyImageView(d_forward_transform_image_view);
	VulkanImage::destroy(vma_allocator, d_forward_transform_image);

	vulkan.get_device().destroyImageView(d_jacobian_matrix_image_view);
	VulkanImage::destroy(vma_allocator, d_jacobian_matrix_image);

	vulkan.get_device().destroySampler(d_sampler);

	for (auto staging_buffer_entry : d_staging_buffers)
	{
		StagingBuffer &staging_buffer = staging_buffer_entry.second;

		VulkanBuffer::destroy(vma_allocator, staging_buffer.forward_transform_buffer);
		VulkanBuffer::destroy(vma_allocator, staging_buffer.jacobian_matrix_buffer);
	}

	// Clear all our staging buffers so that they'll get recreated and repopulated
	// (that is, if 'initialise_vulkan_resources()' is called again, eg, due to a lost device).
	d_staging_buffers.clear();
}


void
GPlatesOpenGL::MapProjectionImage::update(
		Vulkan &vulkan,
		vk::CommandBuffer preprocess_command_buffer,
		const GPlatesGui::MapProjection &map_projection)
{
	const auto map_projection_type = map_projection.get_projection_settings().get_projection_type();

	// Only need to update the map projection images if the map projection *type* changed.
	//
	// Note: Changing the central meridian does not change the staging buffer contents (see 'create_staging_buffer()' for details).
	if (d_last_updated_map_projection_type == map_projection_type)
	{
		return;
	}
	d_last_updated_map_projection_type = map_projection_type;

	// If first time visiting the current map projection *type* then create, fill and cache a staging buffer for it.
	if (d_staging_buffers.find(map_projection_type) == d_staging_buffers.end())
	{
		d_staging_buffers[map_projection_type] = create_staging_buffer(vulkan, map_projection);
	}

	// Get the staging buffer associated with the map projection *type*.
	StagingBuffer &staging_buffer = d_staging_buffers[map_projection_type];

	//
	// Pipeline barrier to wait for any commands that read from the images before we copy new data into them.
	// And also transition both images to an image layout suitable for a transfer destination.
	//

	vk::ImageMemoryBarrier pre_update_image_memory_barrier;
	pre_update_image_memory_barrier
			.setSrcAccessMask({})  // no writes to make available (image is only read from)
			.setDstAccessMask(vk::AccessFlagBits::eTransferWrite)
			.setOldLayout(vk::ImageLayout::eUndefined)  // not interested in retaining contents of image
			.setNewLayout(vk::ImageLayout::eTransferDstOptimal)
			.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

	vk::ImageMemoryBarrier pre_update_forward_transform_image_memory_barrier(pre_update_image_memory_barrier);
	pre_update_forward_transform_image_memory_barrier.setImage(d_forward_transform_image.get_image());

	vk::ImageMemoryBarrier pre_update_jacobian_matrix_image_memory_barrier(pre_update_image_memory_barrier);
	pre_update_jacobian_matrix_image_memory_barrier.setImage(d_jacobian_matrix_image.get_image());

	preprocess_command_buffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eAllCommands,
			vk::PipelineStageFlagBits::eTransfer,
			{},  // dependencyFlags
			{},  // memoryBarriers
			{},  // bufferMemoryBarriers
			{ pre_update_forward_transform_image_memory_barrier, pre_update_jacobian_matrix_image_memory_barrier });  // imageMemoryBarriers

	//
	// Copy image data from staging buffers to images.
	//

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
			d_forward_transform_image.get_image(),
			vk::ImageLayout::eTransferDstOptimal,
			buffer_image_copy);
	preprocess_command_buffer.copyBufferToImage(
			staging_buffer.jacobian_matrix_buffer.get_buffer(),
			d_jacobian_matrix_image.get_image(),
			vk::ImageLayout::eTransferDstOptimal,
			buffer_image_copy);

	//
	// Pipeline barrier to wait for staging transfer writes to be made visible for image reads from any shader.
	// And also transition both images to an image layout suitable for shader reads.
	//

	vk::ImageMemoryBarrier post_update_image_memory_barrier;
	post_update_image_memory_barrier
			.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
			.setDstAccessMask(vk::AccessFlagBits::eShaderRead)
			.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
			.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
			.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

	vk::ImageMemoryBarrier post_update_forward_transform_image_memory_barrier(post_update_image_memory_barrier);
	post_update_forward_transform_image_memory_barrier.setImage(d_forward_transform_image.get_image());

	vk::ImageMemoryBarrier post_update_jacobian_matrix_image_memory_barrier(post_update_image_memory_barrier);
	post_update_jacobian_matrix_image_memory_barrier.setImage(d_jacobian_matrix_image.get_image());

	preprocess_command_buffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eAllCommands,
			{},  // dependencyFlags
			{},  // memoryBarriers
			{},  // bufferMemoryBarriers
			{ post_update_forward_transform_image_memory_barrier, post_update_jacobian_matrix_image_memory_barrier });  // imageMemoryBarriers
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

	// Buffer and allocation create info parameters common to both buffers (forward transform and Jacobian matrix).
	vk::BufferCreateInfo staging_buffer_create_info;
	staging_buffer_create_info
			.setUsage(vk::BufferUsageFlagBits::eTransferSrc)
			.setSharingMode(vk::SharingMode::eExclusive);
	VmaAllocationCreateInfo staging_buffer_allocation_create_info = {};
	staging_buffer_allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;
	staging_buffer_allocation_create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT; // host mappable

	// Create the forward transform buffer.
	staging_buffer_create_info.setSize(IMAGE_WIDTH * IMAGE_HEIGHT * sizeof(ForwardTransformTexel));
	staging_buffer.forward_transform_buffer = VulkanBuffer::create(
			vulkan.get_vma_allocator(),
			staging_buffer_create_info,
			staging_buffer_allocation_create_info,
			GPLATES_EXCEPTION_SOURCE);

	// Create the Jacobian matrix buffer.
	staging_buffer_create_info.setSize(IMAGE_WIDTH * IMAGE_HEIGHT * sizeof(JacobianMatrixTexel));
	staging_buffer.jacobian_matrix_buffer = VulkanBuffer::create(
			vulkan.get_vma_allocator(),
			staging_buffer_create_info,
			staging_buffer_allocation_create_info,
			GPLATES_EXCEPTION_SOURCE);

	//
	// Update the staging buffer.
	//
	// Note: The central meridian does not affect the staging buffer contents because the buffer contains
	//       map projected values (output of the map projection forward transform and Jacobian matrix) and
	//       these values are independent of the central meridian (see below).
	//

	// Map forward transform and Jacobian matrix buffers.
	void *const staging_buffer_forward_transform_mapped_pointer =
			staging_buffer.forward_transform_buffer.map_memory(vulkan.get_vma_allocator(), GPLATES_EXCEPTION_SOURCE);
	void *const staging_buffer_jacobian_matrix_mapped_pointer =
			staging_buffer.jacobian_matrix_buffer.map_memory(vulkan.get_vma_allocator(), GPLATES_EXCEPTION_SOURCE);

	std::vector<ForwardTransformTexel> row_of_forward_transform_texels;
	row_of_forward_transform_texels.resize(IMAGE_WIDTH);
	std::vector<JacobianMatrixTexel> row_of_jacobian_matrix_texels;
	row_of_jacobian_matrix_texels.resize(IMAGE_WIDTH);
	for (unsigned int row = 0; row < IMAGE_HEIGHT; ++row)
	{
		for (unsigned int column = 0; column < IMAGE_WIDTH; ++column)
		{
			// Make sure the longitude is within [-180+epsilon, 180-epsilon] around the central meridian longitude.
			const double longitude_epsilon = 1e-6/*approx 0.1 metres at equator*/;
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
				longitude += -180 + longitude_epsilon;
			}
			else if (column == IMAGE_WIDTH - 1)
			{
				longitude += 180 - longitude_epsilon;
			}
			else
			{
				longitude += -180 + column * TEXEL_INTERVAL_IN_DEGREES;
			}

			// Make sure the latitude is within the clamped range accepted by MapProjection which is slightly inside [-90, 90].
			double latitude;
			if (row == 0)
			{
				latitude = GPlatesGui::MapProjection::MIN_LATITUDE;
			}
			else if (row == IMAGE_HEIGHT - 1)
			{
				latitude = GPlatesGui::MapProjection::MAX_LATITUDE;
			}
			else
			{
				// Note: Our 'TEXEL_INTERVAL_IN_DEGREES' is much larger than 'MapProjection::CLAMP_LATITUDE_NEAR_POLES_EPSILON'.
				//       So we don't need to check our non-boundary latitudes.
				latitude = -90 + row * TEXEL_INTERVAL_IN_DEGREES;
			}

			//
			// Map (longitude, latitude) to map projection space (x, y).
			//

			double x = longitude;
			double y = latitude;
			map_projection.forward_transform(x, y);

			row_of_forward_transform_texels[column] = { static_cast<float>(x), static_cast<float>(y) };

			//
			// Map (longitude, latitude) to map projection space partial derivatives (dxdlon, dxdlat, dydlon, dydlat).
			//

			// Derivative delta in (longitude, latitude) space.
			// It should be small enough to get good accuracy for the derivatives,
			// but not too small that we run into numerical precision issues with the Proj library.
			//
			// The following are error measurements between the actual and computed (in compute shader) map projected positions and unit-vectors (directions)
			// of 50,000 arrows for different derivative delta values. The position error is distance in (x,y) map projection space between the two positions
			// (noting that each map projection is approximately 360 in width), and vector error is distance between the two unit vectors.
			// In the following, the order of map projections is Rectangular, Mercator, Mollweide and Robinson.
			//
			//   delta_lon_lat_for_derivs = 1e-5:
			//   RMS position error:  1.23108e-05 , Max position error: 5.06184e-05 , RMS vector error:  3.00003e-06 , Max vector error: 4.97866e-06
			//   RMS position error:  0.0142877 , Max position error: 0.211496 , RMS vector error:  0.000262877 , Max vector error: 0.00415843
			//   RMS position error:  0.0031077 , Max position error: 0.0346373 , RMS vector error:  0.000144097 , Max vector error: 0.00478129
			//   RMS position error:  0.00189213 , Max position error: 0.00946791 , RMS vector error:  0.0859115 , Max vector error: 1.01257
			//
			//   delta_lon_lat_for_derivs = 1e-4:
			//   RMS position error:  1.23108e-05 , Max position error: 5.06184e-05 , RMS vector error:  3.00003e-06 , Max vector error: 4.97866e-06
			//   RMS position error:  0.0142877 , Max position error: 0.211496 , RMS vector error:  0.000262877 , Max vector error: 0.00415843
			//   RMS position error:  0.0031077 , Max position error: 0.0346373 , RMS vector error:  0.000144097 , Max vector error: 0.00478129
			//   RMS position error:  0.00149643 , Max position error: 0.0077787 , RMS vector error:  0.00797961 , Max vector error: 0.0612052
			//
			//   delta_lon_lat_for_derivs = 1e-3:
			//   RMS position error:  1.23108e-05 , Max position error: 5.06184e-05 , RMS vector error:  3.00003e-06 , Max vector error: 4.97866e-06
			//   RMS position error:  0.0142877 , Max position error: 0.211496 , RMS vector error:  0.000262878 , Max vector error: 0.00415843
			//   RMS position error:  0.0031077 , Max position error: 0.0346373 , RMS vector error:  0.000144096 , Max vector error: 0.00478129
			//   RMS position error:  0.00149328 , Max position error: 0.00811773 , RMS vector error:  0.000809906 , Max vector error: 0.00961482
			//
			//   delta_lon_lat_for_derivs = 5e-3:
			//   RMS position error:  1.23108e-05 , Max position error: 5.06184e-05 , RMS vector error:  3.00003e-06 , Max vector error: 4.97866e-06
			//   RMS position error:  0.0142877 , Max position error: 0.211496 , RMS vector error:  0.000262886 , Max vector error: 0.00415855
			//   RMS position error:  0.0031077 , Max position error: 0.0346373 , RMS vector error:  0.000144096 , Max vector error: 0.00478144
			//   RMS position error:  0.00149336 , Max position error: 0.00815432 , RMS vector error:  0.000218747 , Max vector error: 0.0127383
			//
			//   delta_lon_lat_for_derivs = 1e-2:
			//   RMS position error:  1.23108e-05 , Max position error: 5.06184e-05 , RMS vector error:  3.00003e-06 , Max vector error: 4.97866e-06
			//   RMS position error:  0.0142877 , Max position error: 0.211496 , RMS vector error:  0.000262921 , Max vector error: 0.00415902
			//   RMS position error:  0.0031077 , Max position error: 0.0346373 , RMS vector error:  0.000144102 , Max vector error: 0.00478155
			//   RMS position error:  0.00149336 , Max position error: 0.00815432 , RMS vector error:  0.000170958 , Max vector error: 0.01313
			//
			// The errors for most projections are (mostly) unchanged with varying derivative delta values.
			// Except the Robinson projection which has very large vector (direction) errors for 1e-5.
			// It's only when derivative delta is increased to 1e-3 that the error becomes acceptable.
			// Further increases to the derivative delta progressively increase the errors again, so it seems 1-e3 is the sweet spot.
			//
			// The Robinson projection is the only projection that uses a lookup table, and the Proj library encodes the table as single precision,
			// and there's a rounding of latitude to integer (in order to index the table). So having a derivative delta that is too small must somehow
			// be interacting with the lookup table unfavourably. Also it appears to happen only for specific latitudes - it was noticed for latitudes around
			// 35 +/- 0.5 degrees (positive or negative latitude) and also 65 +/- 0.5 degrees (positive only it seems, but might have had too few samples there).
			// For example:
			//
			//     lon/lat: 74.1903 35.3408 , position error: 0.00404441 , vector error: 0.526679 , original vector: "(0.635099, 0.772431, -4.06282e-09)" , compute shader vector: "(0.939477, 0.342611, 1.90348e-08)"
			//
			// Interestingly, looking at the Robinson lookup table (https://en.wikipedia.org/wiki/Robinson_projection), the latitudes 35 and 65
			// fall right on the lookup table's 5-degree interval boundaries. So I wouldn't be surprised if the polynomial interpolation coefficients don't
			// match up on either for the adjacent intervals at 35 and 65 degrees as well as for other 5-degree latitudes in the table.
			// Also a derivative delta of 1e-5 is in degrees, which for 35 degrees is a factor of ~3e-7 (1e-5 / 35) which is getting pretty close to
			// single-precision floating-point accuracy (which is the accuracy of the polynomial interpolation coefficients in the Proj library).
			// Increasing the derivative delta to 1e-3 is an increase by a factor of 100 and appears to significantly reduce the error.
			// Note: The Proj source code is at https://github.com/OSGeo/PROJ/blob/master/src/projections/robin.cpp
			//
			const double delta_lon_lat_for_derivs = 1e-3;

			// Move the first and last columns slightly inward (by the delta) so the derivative calculations
			// don't sample outside our longitude range.
			double longitude_for_derivs = longitude;
			if (column == 0)
			{
				longitude_for_derivs += delta_lon_lat_for_derivs;
			}
			else if (column == IMAGE_WIDTH - 1)
			{
				longitude_for_derivs -= delta_lon_lat_for_derivs;
			}

			// Move the first and last rows slightly inward (by the delta) so the derivative calculations
			// don't sample outside our latitude range.
			double latitude_for_derivs = latitude;
			if (row == 0)
			{
				latitude_for_derivs += delta_lon_lat_for_derivs;
			}
			else if (row == IMAGE_HEIGHT - 1)
			{
				latitude_for_derivs -= delta_lon_lat_for_derivs;
			}

			// Sample map projection at (longitude + delta, latitude).
			double x_at_lon_plus_delta = longitude_for_derivs + delta_lon_lat_for_derivs;
			double y_at_lon_plus_delta = latitude_for_derivs;
			map_projection.forward_transform(x_at_lon_plus_delta, y_at_lon_plus_delta);

			// Sample map projection at (longitude - delta, latitude).
			double x_at_lon_minus_delta = longitude_for_derivs - delta_lon_lat_for_derivs;
			double y_at_lon_minus_delta = latitude_for_derivs;
			map_projection.forward_transform(x_at_lon_minus_delta, y_at_lon_minus_delta);

			// Sample map projection at (longitude, latitude + delta).
			double x_at_lat_plus_delta = longitude_for_derivs;
			double y_at_lat_plus_delta = latitude_for_derivs + delta_lon_lat_for_derivs;
			map_projection.forward_transform(x_at_lat_plus_delta, y_at_lat_plus_delta);

			// Sample map projection at (longitude, latitude - delta).
			double x_at_lat_minus_delta = longitude_for_derivs;
			double y_at_lat_minus_delta = latitude_for_derivs - delta_lon_lat_for_derivs;
			map_projection.forward_transform(x_at_lat_minus_delta, y_at_lat_minus_delta);

			// Jacobian matrix.
			const double dxdlon = (x_at_lon_plus_delta - x_at_lon_minus_delta) / (2 * delta_lon_lat_for_derivs);
			const double dxdlat = (x_at_lat_plus_delta - x_at_lat_minus_delta) / (2 * delta_lon_lat_for_derivs);
			const double dydlon = (y_at_lon_plus_delta - y_at_lon_minus_delta) / (2 * delta_lon_lat_for_derivs);
			const double dydlat = (y_at_lat_plus_delta - y_at_lat_minus_delta) / (2 * delta_lon_lat_for_derivs);

			row_of_jacobian_matrix_texels[column] = {
					static_cast<float>(dxdlon), static_cast<float>(dxdlat),
					static_cast<float>(dydlon), static_cast<float>(dydlat)};
		}

		// Copy the row into the forward transform buffer.
		std::memcpy(
				// Pointer to row within mapped forward transform buffer...
				static_cast<ForwardTransformTexel *>(staging_buffer_forward_transform_mapped_pointer) + row * IMAGE_WIDTH,
				row_of_forward_transform_texels.data(),
				IMAGE_WIDTH * sizeof(ForwardTransformTexel));

		// Copy the row into the Jacobian matrix buffer.
		std::memcpy(
				// Pointer to row within mapped Jacobian matrix buffer...
				static_cast<JacobianMatrixTexel *>(staging_buffer_jacobian_matrix_mapped_pointer) + row * IMAGE_WIDTH,
				row_of_jacobian_matrix_texels.data(),
				IMAGE_WIDTH * sizeof(JacobianMatrixTexel));
	}

	// Flush and unmap forward transform and Jacobian matrix buffers.
	staging_buffer.forward_transform_buffer.flush_mapped_memory(vulkan.get_vma_allocator(), 0, VK_WHOLE_SIZE, GPLATES_EXCEPTION_SOURCE);
	staging_buffer.jacobian_matrix_buffer.flush_mapped_memory(vulkan.get_vma_allocator(), 0, VK_WHOLE_SIZE, GPLATES_EXCEPTION_SOURCE);
	staging_buffer.forward_transform_buffer.unmap_memory(vulkan.get_vma_allocator());
	staging_buffer.jacobian_matrix_buffer.unmap_memory(vulkan.get_vma_allocator());

	return staging_buffer;
}
