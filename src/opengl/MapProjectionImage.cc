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

#include <cmath>    // fmod
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

	// Image and allocation create info parameters common to all images.
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

	// Create the images.
	for (unsigned int n = 0; n < NUM_IMAGES; ++n)
	{
		d_images[n] = VulkanImage::create(
				vulkan.get_vma_allocator(),
				image_create_info,
				image_allocation_create_info,
				GPLATES_EXCEPTION_SOURCE);
	}

	// Image view create info parameters common to all image views.
	vk::ImageViewCreateInfo image_view_create_info;
	image_view_create_info
			.setViewType(vk::ImageViewType::e2D)
			.setFormat(TEXEL_FORMAT)
			.setComponents({})  // identity swizzle
			.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

	// Create the image views.
	for (unsigned int n = 0; n < NUM_IMAGES; ++n)
	{
		image_view_create_info.setImage(d_images[n].get_image());

		d_image_views[n] = vulkan.get_device().createImageView(image_view_create_info);
	}


	//
	// Transition the all image layouts for optimal shader reads.
	//
	// This is only to avoid the validation layers complaining since an image can be bound for optimal shader reads
	// via a descriptor set (by a client) when the globe is active (ie, when not rendering to a map view).
	// It needs to be bound because it's "statically" used, even though the shader will not actually sample the image (if not in map view).
	//

	// Begin recording into the initialisation command buffer.
	vk::CommandBufferBeginInfo initialisation_command_buffer_begin_info;
	// Command buffer will only be submitted once.
	initialisation_command_buffer_begin_info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	initialisation_command_buffer.begin(initialisation_command_buffer_begin_info);

	//
	// Pipeline barrier to transition all images to an image layout suitable for a transfer destination.
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

	vk::ImageMemoryBarrier pre_clear_image_memory_barriers[NUM_IMAGES];
	for (unsigned int n = 0; n < NUM_IMAGES; ++n)
	{
		pre_clear_image_memory_barriers[n] = pre_clear_image_memory_barrier;
		pre_clear_image_memory_barriers[n].setImage(d_images[n].get_image());
	}

	initialisation_command_buffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eTopOfPipe,  // don't need to wait to access freshly allocated memory
			vk::PipelineStageFlagBits::eTransfer,
			{},  // dependencyFlags
			{},  // memoryBarriers
			{},  // bufferMemoryBarriers
			pre_clear_image_memory_barriers);  // imageMemoryBarriers

	//
	// Clear all images.
	//
	// This is not really necessary since a call to 'update()' will overwrite these images (when switching map projections).
	// But we have to transition to an image layout for optimal shader reads anyway, so might as well clear the images also.
	const vk::ClearColorValue clear_value = std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f };
	const vk::ImageSubresourceRange sub_resource_range = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
	for (unsigned int n = 0; n < NUM_IMAGES; ++n)
	{
		initialisation_command_buffer.clearColorImage(
				d_images[n].get_image(),
				vk::ImageLayout::eTransferDstOptimal,
				clear_value,
				sub_resource_range);
	}

	//
	// Pipeline barrier to transition all images to an image layout suitable for optimal shader reads.
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

	vk::ImageMemoryBarrier post_clear_image_memory_barriers[NUM_IMAGES];
	for (unsigned int n = 0; n < NUM_IMAGES; ++n)
	{
		post_clear_image_memory_barriers[n] = post_clear_image_memory_barrier;
		post_clear_image_memory_barriers[n].setImage(d_images[n].get_image());
	}

	initialisation_command_buffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer,
			// Don't need to wait since images won't be used until 'update()' is called (and that will wait)...
			vk::PipelineStageFlagBits::eBottomOfPipe,
			{},  // dependencyFlags
			{},  // memoryBarriers
			{},  // bufferMemoryBarriers
			post_clear_image_memory_barriers);  // imageMemoryBarriers

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

	for (unsigned int n = 0; n < NUM_IMAGES; ++n)
	{
		vulkan.get_device().destroyImageView(d_image_views[n]);
		VulkanImage::destroy(vma_allocator, d_images[n]);
	}

	vulkan.get_device().destroySampler(d_sampler);

	for (auto staging_buffer_entry : d_staging_buffers)
	{
		StagingBuffer &staging_buffer = staging_buffer_entry.second;

		for (unsigned int n = 0; n < NUM_IMAGES; ++n)
		{
			VulkanBuffer::destroy(vma_allocator, staging_buffer.buffers[n]);
		}
	}

	// Clear all our staging buffers so that they'll get recreated and repopulated
	// (that is, if 'initialise_vulkan_resources()' is called again, eg, due to a lost device).
	d_staging_buffers.clear();
}


std::vector<vk::DescriptorImageInfo>
GPlatesOpenGL::MapProjectionImage::get_descriptor_image_infos() const
{
	std::vector<vk::DescriptorImageInfo> descriptor_image_infos;
	for (unsigned int n = 0; n < NUM_IMAGES; ++n)
	{
		descriptor_image_infos.push_back({ d_sampler, d_image_views[n], vk::ImageLayout::eShaderReadOnlyOptimal });
	}

	return descriptor_image_infos;
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
			.setSrcAccessMask({})  // no writes to make available (images are only read from)
			.setDstAccessMask(vk::AccessFlagBits::eTransferWrite)
			.setOldLayout(vk::ImageLayout::eUndefined)  // not interested in retaining contents of images
			.setNewLayout(vk::ImageLayout::eTransferDstOptimal)
			.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

	vk::ImageMemoryBarrier pre_update_image_memory_barriers[NUM_IMAGES];
	for (unsigned int n = 0; n < NUM_IMAGES; ++n)
	{
		pre_update_image_memory_barriers[n] = pre_update_image_memory_barrier;
		pre_update_image_memory_barriers[n].setImage(d_images[n].get_image());
	}

	preprocess_command_buffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eAllCommands,
			vk::PipelineStageFlagBits::eTransfer,
			{},  // dependencyFlags
			{},  // memoryBarriers
			{},  // bufferMemoryBarriers
			pre_update_image_memory_barriers);  // imageMemoryBarriers

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

	for (unsigned int n = 0; n < NUM_IMAGES; ++n)
	{
		preprocess_command_buffer.copyBufferToImage(
				staging_buffer.buffers[n].get_buffer(),
				d_images[n].get_image(),
				vk::ImageLayout::eTransferDstOptimal,
				buffer_image_copy);
	}

	//
	// Pipeline barrier to wait for staging transfer writes to be made visible for image reads from any shader.
	// And also transition all images to an image layout suitable for shader reads.
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

	vk::ImageMemoryBarrier post_update_image_memory_barriers[NUM_IMAGES];
	for (unsigned int n = 0; n < NUM_IMAGES; ++n)
	{
		post_update_image_memory_barriers[n] = post_update_image_memory_barrier;
		post_update_image_memory_barriers[n].setImage(d_images[n].get_image());
	}

	preprocess_command_buffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eAllCommands,
			{},  // dependencyFlags
			{},  // memoryBarriers
			{},  // bufferMemoryBarriers
			post_update_image_memory_barriers);  // imageMemoryBarriers
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

	// Buffer and allocation create info parameters common to all buffers.
	vk::BufferCreateInfo staging_buffer_create_info;
	staging_buffer_create_info
			.setUsage(vk::BufferUsageFlagBits::eTransferSrc)
			.setSharingMode(vk::SharingMode::eExclusive);
	VmaAllocationCreateInfo staging_buffer_allocation_create_info = {};
	staging_buffer_allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;
	staging_buffer_allocation_create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT; // host mappable

	// Create the buffers.
	for (unsigned int n = 0; n < NUM_IMAGES; ++n)
	{
		staging_buffer_create_info.setSize(IMAGE_WIDTH * IMAGE_HEIGHT * sizeof(Texel)/*all images have same-size texels*/);
		staging_buffer.buffers[n] = VulkanBuffer::create(
				vulkan.get_vma_allocator(),
				staging_buffer_create_info,
				staging_buffer_allocation_create_info,
				GPLATES_EXCEPTION_SOURCE);
	}

	//
	// Update the staging buffer.
	//
	// Note: The central meridian does not affect the staging buffer contents because the buffer contains
	//       map projected values (output of the map projection forward transform and Jacobian matrix) and
	//       these values are independent of the central meridian (see below).
	//

	// Map forward transform and Jacobian matrix buffers.
	void *staging_buffer_mapped_pointer[NUM_IMAGES];
	for (unsigned int n = 0; n < NUM_IMAGES; ++n)
	{
		staging_buffer_mapped_pointer[n] = staging_buffer.buffers[n].map_memory(vulkan.get_vma_allocator(), GPLATES_EXCEPTION_SOURCE);
	}

	// Allocate a single row of texels for each image.
	std::vector<Texel> row_of_texels[NUM_IMAGES];
	for (unsigned int n = 0; n < NUM_IMAGES; ++n)
	{
		row_of_texels[n].resize(IMAGE_WIDTH);
	}

	// Fill the buffers with map projection data.
	for (unsigned int row = 0; row < IMAGE_HEIGHT; ++row)
	{
		for (unsigned int column = 0; column < IMAGE_WIDTH; ++column)
		{
			// Current texel in each image.
			Texel &texel1 = row_of_texels[0][column];
			Texel &texel2 = row_of_texels[1][column];
			Texel &texel3 = row_of_texels[2][column];

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
			// The delta used to calculate derivatives in (longitude, latitude) space.
			//
			// It should be small enough to get good accuracy for the derivatives, but not too small that we run into
			// numerical precision issues with the Proj library (see Robinson projection comment further below).
			//
			// The following are error measurements between the actual and computed (in compute shader) map projected positions and unit-vectors (directions)
			// of 50,000 arrows for different derivative delta values. The position error is distance in (x,y) map projection space between the two positions
			// (noting that each map projection is approximately 360 in width), and vector error is distance between the two unit vectors.
			// In the following, the order of map projections is Rectangular, Mercator, Mollweide and Robinson.
			//
			//   delta_lon_lat_for_derivs = 1e-5:
			//   RMS position error:  1.22747e-05 , Max position error: 5.09968e-05 , RMS vector error:  2.99995e-06 , Max vector error: 4.97652e-06
			//   RMS position error:  0.000555428 , Max position error: 0.0201331   , RMS vector error:  0.000262882 , Max vector error: 0.00415851
			//   RMS position error:  7.85416e-05 , Max position error: 0.00190795  , RMS vector error:  0.00014409  , Max vector error: 0.00478119
			//   RMS position error:  4.44738e-05 , Max position error: 0.000328331 , RMS vector error:  0.000152836 , Max vector error: 0.0135238
			//
			//   delta_lon_lat_for_derivs = 1e-4:
			//   RMS position error:  1.27192e-05 , Max position error: 0.000127291 , RMS vector error:  2.99995e-06 , Max vector error: 4.97652e-06
			//   RMS position error:  0.000555943 , Max position error: 0.0202552   , RMS vector error:  0.000262881 , Max vector error: 0.00415851
			//   RMS position error:  5.21176e-05 , Max position error: 0.00187159  , RMS vector error:  0.00014409  , Max vector error: 0.00478119
			//   RMS position error:  9.24784e-05 , Max position error: 0.000512773 , RMS vector error:  0.000152816 , Max vector error: 0.013525
			//
			//   delta_lon_lat_for_derivs = 1e-3:
			//   RMS position error:  3.33461e-05 , Max position error: 0.00100724  , RMS vector error:  2.99995e-06 , Max vector error: 4.97652e-06
			//   RMS position error:  0.000556458 , Max position error: 0.0202552   , RMS vector error:  0.000262883 , Max vector error: 0.00415851
			//   RMS position error:  5.62128e-05 , Max position error: 0.00186778  , RMS vector error:  0.000144089 , Max vector error: 0.00478119
			//   RMS position error:  0.000825928 , Max position error: 0.00326381  , RMS vector error:  0.000152677 , Max vector error: 0.0135361
			//
			//   delta_lon_lat_for_derivs = 1e-2:
			//   RMS position error:  0.000306855 , Max position error: 0.00999452  , RMS vector error:  2.99995e-06 , Max vector error: 4.97652e-06
			//   RMS position error:  0.000605247 , Max position error: 0.0202552   , RMS vector error:  0.000262928 , Max vector error: 0.00415902
			//   RMS position error:  0.000214083 , Max position error: 0.00928417  , RMS vector error:  0.000144097 , Max vector error: 0.00478178
			//   RMS position error:  0.00823971  , Max position error: 0.0326414   , RMS vector error:  0.000157338 , Max vector error: 0.0136478
			//
			const double delta_lon_lat_for_derivs = 1e-5;  // degrees
			const double delta_lon_lat_for_derivs_radians = GPlatesMaths::convert_deg_to_rad(delta_lon_lat_for_derivs);

			// Move the first and last columns slightly inward (by the delta) so the derivative calculations don't sample outside our longitude range.
			//
			// Note: We do this even for the mapping of (longitude, latitude) to (x, y) since that (x, y) value is also used in the 2nd order derivatives.
			if (column == 0)
			{
				longitude += delta_lon_lat_for_derivs;
			}
			else if (column == IMAGE_WIDTH - 1)
			{
				longitude -= delta_lon_lat_for_derivs;
			}

			// Move the first and last rows slightly inward (by the delta) so the derivative calculations don't sample outside our latitude range.
			//
			// Note: We do this even for the mapping of (longitude, latitude) to (x, y) since that (x, y) value is also used in the 2nd order derivatives.
			if (row == 0)
			{
				latitude += delta_lon_lat_for_derivs;
			}
			else if (row == IMAGE_HEIGHT - 1)
			{
				latitude -= delta_lon_lat_for_derivs;
			}

			//
			// Handle the Robinson map projection as a special case.
			//
			// The errors for most projections are (mostly) unchanged with varying derivative delta values.
			// However the Robinson projection has very large errors for a derivative delta of 1e-5 and
			// it's only when the derivative delta is increased to 1e-3 that the error becomes acceptable.
			//
			//   RMS position error:  34.5264 , Max position error: 352.087       // delta_lon_lat_for_derivs = 1e-5
			//   RMS position error:  0.0036003 , Max position error: 0.022695    // delta_lon_lat_for_derivs = 1e-2
			//
			// This is because the Robinson projection is the only projection we use that uses a lookup table and there's a
			// rounding of latitude to integer (in order to index the table). This rounding happens every 5 degrees of latitude.
			// You can see the Robinson 5-degree intervals here: https://en.wikipedia.org/wiki/Robinson_projection and
			// the associated interpolation polynomial coefficients for these intervals in the Proj library source code
			// here: https://github.com/OSGeo/PROJ/blob/master/src/projections/robin.cpp
			//
			// Our errors only happen near these interval boundaries. The Proj library interpolation polynomials produce results
			// that appear to be accurate to single-precision floating-point, a ratio of ~1e-7, which is pretty good really
			// (on the order of 1 metre accuracy on the Earth's surface). However we're using double precision and a
			// derivative delta of 1e-5 (in degrees) which, for reasonable latitudes (eg, 45 degrees), is ~2e-7 (1e-5 / 45)
			// and that is getting pretty close to single-precision accuracy, so we're going to notice some errors.
			// This can be seen by considering 45 degrees to have both:
			// - a lookup table index of 9 and a fractional remainder of 0.0 degrees, and
			// - a lookup table index of 8 and a fractional remainder of 5.0 degrees.
			// Manually plugging both these into the Proj library source code (and multiplying by ~60 to get output 'y' to roughly match latitude)
			// gives projected 'y' values of:
			//
			//   45.2019798         (for i=9, dphi=0)
			//   45.20199437441325  (for i=8, dphi=5)
			//
			// ...ideally both values should be equal but the relative error between them is 3.22e-7 (which is basically single precision).
			// And the absolute error is 1.45e-5 (45.20199437441325 - 45.2019798) which is even larger than our derivative delta of 1e-5.
			// So any derivatives we perform that cross these boundaries will have serious errors.
			// This also explains why increasing the derivative delta to 1e-3 reduces the error significantly (by a factor of 100 for 1st-order derivatives).
			//
			// So, for the Robinson projection, we'll avoid calculating derivatives across latitudes that are a multiple of 5 degrees.
			// And we do this by shifting those latitudes slightly so that this doesn't happen.
			// This allows us to retain a small derivative delta (eg, 1e-5).
			//
			if (map_projection.projection_type() == GPlatesGui::MapProjection::ROBINSON)
			{
				// Get remainder of latitude divided by 5.0.
				const double latitude_mod_5 = std::fmod(latitude, 5.0);       // in range [-5, 5]
				const double abs_latitude_mod_5 = std::fabs(latitude_mod_5);  // in range [0, 5]

				// Detect latitudes within a threshold distance to a multiple of 5.0.
				// The threshold distance is the derivative delta plus a little extra.
				const double latitude_closeness_to_multiple_of_5 = delta_lon_lat_for_derivs + 1e-10;
				if (abs_latitude_mod_5 < latitude_closeness_to_multiple_of_5 ||      // close to 0.0
					abs_latitude_mod_5 > 5.0 - latitude_closeness_to_multiple_of_5)  // close to 5.0
				{
					// Shift the latitude slightly towards zero such that the derivative filter width
					// does not cross the multiple-of-five latitude.
					if (latitude >= 0)
					{
						latitude -= 2 * latitude_closeness_to_multiple_of_5;
					}
					else
					{
						latitude += 2 * latitude_closeness_to_multiple_of_5;
					}
				}
			}

			//
			// Map (longitude, latitude) to map projection space (x, y).
			//

			double x = longitude;
			double y = latitude;
			map_projection.forward_transform(x, y);

			// Store map projection (x, y) in first image.
			texel1.values[0] = static_cast<float>(x);
			texel1.values[1] = static_cast<float>(y);

			//
			// Map (longitude, latitude) to map projection space first-order partial derivatives dx/dlon, dx/dlat, dy/dlon, dy/dlat.
			//

			// Sample map projection at (longitude + delta, latitude).
			double x_at_lon_plus_delta = longitude + delta_lon_lat_for_derivs;
			double y_at_lon_plus_delta = latitude;
			map_projection.forward_transform(x_at_lon_plus_delta, y_at_lon_plus_delta);

			// Sample map projection at (longitude - delta, latitude).
			double x_at_lon_minus_delta = longitude - delta_lon_lat_for_derivs;
			double y_at_lon_minus_delta = latitude;
			map_projection.forward_transform(x_at_lon_minus_delta, y_at_lon_minus_delta);

			// Sample map projection at (longitude, latitude + delta).
			double x_at_lat_plus_delta = longitude;
			double y_at_lat_plus_delta = latitude + delta_lon_lat_for_derivs;
			map_projection.forward_transform(x_at_lat_plus_delta, y_at_lat_plus_delta);

			// Sample map projection at (longitude, latitude - delta).
			double x_at_lat_minus_delta = longitude;
			double y_at_lat_minus_delta = latitude - delta_lon_lat_for_derivs;
			map_projection.forward_transform(x_at_lat_minus_delta, y_at_lat_minus_delta);

			// Jacobian matrix.
			const double dx_dlon_radians = (x_at_lon_plus_delta - x_at_lon_minus_delta) / (2 * delta_lon_lat_for_derivs_radians);
			const double dx_dlat_radians = (x_at_lat_plus_delta - x_at_lat_minus_delta) / (2 * delta_lon_lat_for_derivs_radians);
			const double dy_dlon_radians = (y_at_lon_plus_delta - y_at_lon_minus_delta) / (2 * delta_lon_lat_for_derivs_radians);
			const double dy_dlat_radians = (y_at_lat_plus_delta - y_at_lat_minus_delta) / (2 * delta_lon_lat_for_derivs_radians);

			// Store map projection Jacobian matrix in second image.
			//
			// Note: Using radians for longitude and latitude since Vulkan shaders will use GLSL atan() and asin() functions
			//       to get longitude and latitude (and those functions returns results in radians).
			texel2.values[0] = static_cast<float>(dx_dlon_radians);
			texel2.values[1] = static_cast<float>(dx_dlat_radians);
			texel2.values[2] = static_cast<float>(dy_dlon_radians);
			texel2.values[3] = static_cast<float>(dy_dlat_radians);

			//
			// Map (longitude, latitude) to map projection space first-order partial derivatives.
			//
			// The 2nd order partial derivatives are:
			//   d(dx/dlon)/dlon, d(dx/dlat)/dlat, d(dx/dlon)/dlat (for 'x') and
			//   d(dy/dlon)/dlon, d(dy/dlat)/dlat, d(dy/dlon)/dlat (for 'y').
			//
			// Note: d(dx/dlon)/dlat and d(dx/dlat)/dlon are the same (so we only need to calculate one of them).
			//       d(dy/dlon)/dlat and d(dy/dlat)/dlon are also the same.
			//

			// Sample map projection at (longitude + delta, latitude + delta).
			double x_at_lon_plus_delta_lat_plus_delta = longitude + delta_lon_lat_for_derivs;
			double y_at_lon_plus_delta_lat_plus_delta = latitude + delta_lon_lat_for_derivs;
			map_projection.forward_transform(x_at_lon_plus_delta_lat_plus_delta, y_at_lon_plus_delta_lat_plus_delta);

			// Sample map projection at (longitude + delta, latitude - delta).
			double x_at_lon_plus_delta_lat_minus_delta = longitude + delta_lon_lat_for_derivs;
			double y_at_lon_plus_delta_lat_minus_delta = latitude - delta_lon_lat_for_derivs;
			map_projection.forward_transform(x_at_lon_plus_delta_lat_minus_delta, y_at_lon_plus_delta_lat_minus_delta);

			// Sample map projection at (longitude - delta, latitude + delta).
			double x_at_lon_minus_delta_lat_plus_delta = longitude - delta_lon_lat_for_derivs;
			double y_at_lon_minus_delta_lat_plus_delta = latitude + delta_lon_lat_for_derivs;
			map_projection.forward_transform(x_at_lon_minus_delta_lat_plus_delta, y_at_lon_minus_delta_lat_plus_delta);

			// Sample map projection at (longitude - delta, latitude - delta).
			double x_at_lon_minus_delta_lat_minus_delta = longitude - delta_lon_lat_for_derivs;
			double y_at_lon_minus_delta_lat_minus_delta = latitude - delta_lon_lat_for_derivs;
			map_projection.forward_transform(x_at_lon_minus_delta_lat_minus_delta, y_at_lon_minus_delta_lat_minus_delta);

			// Hessian matrix (for 'x').
			const double ddx_dlon_dlon_radians = (x_at_lon_plus_delta - 2 * x + x_at_lon_minus_delta) /
					(delta_lon_lat_for_derivs_radians * delta_lon_lat_for_derivs_radians);
			const double ddx_dlat_dlat_radians = (x_at_lat_plus_delta - 2 * x + x_at_lat_minus_delta) /
					(delta_lon_lat_for_derivs_radians * delta_lon_lat_for_derivs_radians);
			const double ddx_dlon_dlat_radians = (x_at_lon_plus_delta_lat_plus_delta - x_at_lon_plus_delta_lat_minus_delta -
							x_at_lon_minus_delta_lat_plus_delta + x_at_lon_minus_delta_lat_minus_delta) /
					(4 * delta_lon_lat_for_derivs_radians * delta_lon_lat_for_derivs_radians);

			// Hessian matrix (for 'y1').
			const double ddy_dlon_dlon_radians = (y_at_lon_plus_delta - 2 * y + y_at_lon_minus_delta) /
					(delta_lon_lat_for_derivs_radians * delta_lon_lat_for_derivs_radians);
			const double ddy_dlat_dlat_radians = (y_at_lat_plus_delta - 2 * y + y_at_lat_minus_delta) /
					(delta_lon_lat_for_derivs_radians * delta_lon_lat_for_derivs_radians);
			const double ddy_dlon_dlat_radians = (y_at_lon_plus_delta_lat_plus_delta - y_at_lon_plus_delta_lat_minus_delta -
							y_at_lon_minus_delta_lat_plus_delta + y_at_lon_minus_delta_lat_minus_delta) /
					(4 * delta_lon_lat_for_derivs_radians * delta_lon_lat_for_derivs_radians);

			// Store the diagonal Hessian matrix elements in the third image.
			//
			// Note: Using radians for longitude and latitude since Vulkan shaders will use GLSL atan() and asin() functions
			//       to get longitude and latitude (and those functions returns results in radians).
			texel3.values[0] = static_cast<float>(ddx_dlon_dlon_radians);
			texel3.values[1] = static_cast<float>(ddx_dlat_dlat_radians);
			texel3.values[2] = static_cast<float>(ddy_dlon_dlon_radians);
			texel3.values[3] = static_cast<float>(ddy_dlat_dlat_radians);

			// Store the off-diagonal symmetric Hessian matrix elements in the first image.
			//
			// Note: Using radians for longitude and latitude since Vulkan shaders will use GLSL atan() and asin() functions
			//       to get longitude and latitude (and those functions returns results in radians).
			texel1.values[2] = static_cast<float>(ddx_dlon_dlat_radians);
			texel1.values[3] = static_cast<float>(ddy_dlon_dlat_radians);
		}

		// Copy a row into each buffer.
		for (unsigned int n = 0; n < NUM_IMAGES; ++n)
		{
			std::memcpy(
					// Pointer to row within mapped buffer...
					static_cast<Texel *>(staging_buffer_mapped_pointer[n]) + row * IMAGE_WIDTH,
					row_of_texels[n].data(),
					IMAGE_WIDTH * sizeof(Texel));
		}
	}

	// Flush and unmap forward transform and Jacobian matrix buffers.
	for (unsigned int n = 0; n < NUM_IMAGES; ++n)
	{
		staging_buffer.buffers[n].flush_mapped_memory(vulkan.get_vma_allocator(), 0, VK_WHOLE_SIZE, GPLATES_EXCEPTION_SOURCE);
		staging_buffer.buffers[n].unmap_memory(vulkan.get_vma_allocator());
	}

	return staging_buffer;
}
