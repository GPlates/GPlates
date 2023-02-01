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


#define V(C,z) (C.c0 + z * (C.c1 + z * (C.c2 + z * C.c3)))

namespace { // anonymous namespace
struct COEFS {
    double c0, c1, c2, c3;
};
} // anonymous namespace

static const struct COEFS X[] = {
    {1.0, 2.2199e-17, -7.15515e-05, 3.1103e-06},
    {0.9986, -0.000482243, -2.4897e-05, -1.3309e-06},
    {0.9954, -0.00083103, -4.48605e-05, -9.86701e-07},
    {0.99, -0.00135364, -5.9661e-05, 3.6777e-06},
    {0.9822, -0.00167442, -4.49547e-06, -5.72411e-06},
    {0.973, -0.00214868, -9.03571e-05, 1.8736e-08},
    {0.96, -0.00305085, -9.00761e-05, 1.64917e-06},
    {0.9427, -0.00382792, -6.53386e-05, -2.6154e-06},
    {0.9216, -0.00467746, -0.00010457, 4.81243e-06},
    {0.8962, -0.00536223, -3.23831e-05, -5.43432e-06},
    {0.8679, -0.00609363, -0.000113898, 3.32484e-06},
    {0.835, -0.00698325, -6.40253e-05, 9.34959e-07},
    {0.7986, -0.00755338, -5.00009e-05, 9.35324e-07},
    {0.7597, -0.00798324, -3.5971e-05, -2.27626e-06},
    {0.7186, -0.00851367, -7.01149e-05, -8.6303e-06},
    {0.6732, -0.00986209, -0.000199569, 1.91974e-05},
    {0.6213, -0.010418, 8.83923e-05, 6.24051e-06},
    {0.5722, -0.00906601, 0.000182, 6.24051e-06},
    {0.5322, -0.00677797, 0.000275608, 6.24051e-06}
};

static const struct COEFS Y[] = {
    {-5.20417e-18, 0.0124, 1.21431e-18, -8.45284e-11},
    {0.062, 0.0124, -1.26793e-09, 4.22642e-10},
    {0.124, 0.0124, 5.07171e-09, -1.60604e-09},
    {0.186, 0.0123999, -1.90189e-08, 6.00152e-09},
    {0.248, 0.0124002, 7.10039e-08, -2.24e-08},
    {0.31, 0.0123992, -2.64997e-07, 8.35986e-08},
    {0.372, 0.0124029, 9.88983e-07, -3.11994e-07},
    {0.434, 0.0123893, -3.69093e-06, -4.35621e-07},

    {0.4958, 0.0123198, -1.02252e-05, -3.45523e-07},
    {0.5571, 0.0121916, -1.54081e-05, -5.82288e-07},
    {0.6176, 0.0119938, -2.41424e-05, -5.25327e-07},
    {0.6769, 0.011713, -3.20223e-05, -5.16405e-07},
    {0.7346, 0.0113541, -3.97684e-05, -6.09052e-07},
    {0.7903, 0.0109107, -4.89042e-05, -1.04739e-06},
    {0.8435, 0.0103431, -6.4615e-05, -1.40374e-09},
    {0.8936, 0.00969686, -6.4636e-05, -8.547e-06},
    {0.9394, 0.00840947, -0.000192841, -4.2106e-06},
    {0.9761, 0.00616527, -0.000256, -4.2106e-06},
    {1.0, 0.00328947, -0.000319159, -4.2106e-06}
};

#define FXC 0.8487
#define FYC 1.3523
#define C1  11.45915590261646417544
#define RC1 0.08726646259971647884
#define NODES   18
#define ONEEPS  1.000001
#define EPS 1e-10
/* Not sure at all of the appropriate number for MAX_ITER... */
#define MAX_ITER 100

bool robin_debug = false;

void
robin_forward_transform(
		double &longitude,
		double &latitude)
{           /* Spheroidal, forward */

	longitude = GPlatesMaths::convert_deg_to_rad(longitude);
	latitude = GPlatesMaths::convert_deg_to_rad(latitude);

	double x = 0;
	double y = 0;
    long i;
    double dphi;

    dphi = fabs(latitude);
    i = isnan(latitude) ? -1 : lround(floor(dphi * C1 + 1e-15));
    if( i < 0 )
	{
        qWarning() << "Outside domain";
		longitude = x;
		latitude = y;
        return;
    }
    if (i >= NODES) i = NODES;
    dphi = GPlatesMaths::convert_rad_to_deg(dphi - RC1 * i);
    x = V(X[i], dphi) * FXC * longitude;
    y = V(Y[i], dphi) * FYC;
    if (latitude < 0.) y = -y;

	if (robin_debug)
	{
		qDebug()
				<< qSetRealNumberPrecision(12)
				<< "  i" << i << ", dphi" << dphi
				<< qSetRealNumberPrecision(6);
	}

	x *= 60;
	y *= 60;

	longitude = x;
	latitude = y;
}


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
			// match up on either side for the adjacent intervals at 35 and 65 degrees as well as for other 5-degree latitudes in the table.
			// Increasing the derivative delta to 1e-3 is an increase by a factor of 100 and appears to significantly reduce the error
			// (which makes sense, since this possible misalignment offset would be reduced by 100 when calculating derivatives).
			//
			// Also a derivative delta of 1e-5 is in degrees, which for 35 degrees is a factor of ~3e-7 (1e-5 / 35) which is getting pretty close to
			// single-precision floating-point accuracy (which is the accuracy of the polynomial interpolation coefficients in the Proj library).
			// UPDATE: Temporarily copying the Proj code (version 8.0.1) into our code and converting their float coefficients to double
			//         doesn't significantly reduce the error. So the problem is more likely the misalignment at 35 and 65 degrees mentioned above.
			// Note: The Proj source code is at https://github.com/OSGeo/PROJ/blob/master/src/projections/robin.cpp
			//   Robinson projection only:
			//   // delta_lon_lat_for_derivs = 1e-2 ...
			//   RMS position error:  0.0036003 , Max position error: 0.022695    // using Proj library
			//   RMS position error:  0.00356503 , Max position error: 0.0224696  // using our impl of Robin (float coeffs initialised from double)
			//   RMS position error:  0.00343327 , Max position error: 0.0204588  // using our impl of Robin (double coeffs)
			//   // delta_lon_lat_for_derivs = 1e-5 ...
			//   RMS position error:  34.5264 , Max position error: 352.087       // using Proj library
			//   RMS position error:  34.5317 , Max position error: 348.645       // using our impl of Robin (float coeffs initialised from double)
			//   RMS position error:  29.9408 , Max position error: 335.406       // using our impl of Robin (double coeffs)
			//
			const double delta_lon_lat_for_derivs = (map_projection.projection_type() == GPlatesGui::MapProjection::ROBINSON) ? 1e-2 : 1e-5;
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
			// Map (longitude, latitude) to map projection space (x, y).
			//

			double x = longitude;
			double y = latitude;
			if (map_projection.projection_type() == GPlatesGui::MapProjection::NUM_PROJECTIONS)
			{
				if (GPlatesMaths::are_almost_exactly_equal(latitude, 45.0))
				{
					robin_debug = true;
					qDebug() << "Robin: y";
				}
				robin_forward_transform(x, y);
				if (GPlatesMaths::are_almost_exactly_equal(latitude, 45.0))
				{
					robin_debug = false;
				}
			}
			else
			{
				map_projection.forward_transform(x, y);
			}
			//map_projection.forward_transform(x, y);

			// Store map projection (x, y) in first image.
			texel1.values[0] = static_cast<float>(x);
			texel1.values[1] = static_cast<float>(y);

			//
			// Map (longitude, latitude) to map projection space first-order partial derivatives dx/dlon, dx/dlat, dy/dlon, dy/dlat.
			//

			// Sample map projection at (longitude + delta, latitude).
			double x_at_lon_plus_delta = longitude + delta_lon_lat_for_derivs;
			double y_at_lon_plus_delta = latitude;
			if (map_projection.projection_type() == GPlatesGui::MapProjection::NUM_PROJECTIONS)
			{
				robin_forward_transform(x_at_lon_plus_delta, y_at_lon_plus_delta);
			}
			else
			{
				map_projection.forward_transform(x_at_lon_plus_delta, y_at_lon_plus_delta);
			}
			//map_projection.forward_transform(x_at_lon_plus_delta, y_at_lon_plus_delta);

			// Sample map projection at (longitude - delta, latitude).
			double x_at_lon_minus_delta = longitude - delta_lon_lat_for_derivs;
			double y_at_lon_minus_delta = latitude;
			if (map_projection.projection_type() == GPlatesGui::MapProjection::NUM_PROJECTIONS)
			{
				robin_forward_transform(x_at_lon_minus_delta, y_at_lon_minus_delta);
			}
			else
			{
				map_projection.forward_transform(x_at_lon_minus_delta, y_at_lon_minus_delta);
			}
			//map_projection.forward_transform(x_at_lon_minus_delta, y_at_lon_minus_delta);

			// Sample map projection at (longitude, latitude + delta).
			double x_at_lat_plus_delta = longitude;
			double y_at_lat_plus_delta = latitude + delta_lon_lat_for_derivs;
			if (map_projection.projection_type() == GPlatesGui::MapProjection::NUM_PROJECTIONS)
			{
				if (GPlatesMaths::are_almost_exactly_equal(latitude, 45.0))
				{
					robin_debug = true;
					qDebug() << "Robin: y_at_lat_plus_delta";
				}
				robin_forward_transform(x_at_lat_plus_delta, y_at_lat_plus_delta);
				if (GPlatesMaths::are_almost_exactly_equal(latitude, 45.0))
				{
					robin_debug = false;
				}
			}
			else
			{
				map_projection.forward_transform(x_at_lat_plus_delta, y_at_lat_plus_delta);
			}
			//map_projection.forward_transform(x_at_lat_plus_delta, y_at_lat_plus_delta);

			// Sample map projection at (longitude, latitude - delta).
			double x_at_lat_minus_delta = longitude;
			double y_at_lat_minus_delta = latitude - delta_lon_lat_for_derivs;
			if (map_projection.projection_type() == GPlatesGui::MapProjection::NUM_PROJECTIONS)
			{
				if (GPlatesMaths::are_almost_exactly_equal(latitude, 45.0))
				{
					robin_debug = true;
					qDebug() << "Robin: y_at_lat_minus_delta";
				}
				robin_forward_transform(x_at_lat_minus_delta, y_at_lat_minus_delta);
				if (GPlatesMaths::are_almost_exactly_equal(latitude, 45.0))
				{
					robin_debug = false;
				}
			}
			else
			{
				map_projection.forward_transform(x_at_lat_minus_delta, y_at_lat_minus_delta);
			}
			//map_projection.forward_transform(x_at_lat_minus_delta, y_at_lat_minus_delta);

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
			if (map_projection.projection_type() == GPlatesGui::MapProjection::NUM_PROJECTIONS)
			{
				robin_forward_transform(x_at_lon_plus_delta_lat_plus_delta, y_at_lon_plus_delta_lat_plus_delta);
			}
			else
			{
				map_projection.forward_transform(x_at_lon_plus_delta_lat_plus_delta, y_at_lon_plus_delta_lat_plus_delta);
			}
			//map_projection.forward_transform(x_at_lon_plus_delta_lat_plus_delta, y_at_lon_plus_delta_lat_plus_delta);

			// Sample map projection at (longitude + delta, latitude - delta).
			double x_at_lon_plus_delta_lat_minus_delta = longitude + delta_lon_lat_for_derivs;
			double y_at_lon_plus_delta_lat_minus_delta = latitude - delta_lon_lat_for_derivs;
			if (map_projection.projection_type() == GPlatesGui::MapProjection::NUM_PROJECTIONS)
			{
				robin_forward_transform(x_at_lon_plus_delta_lat_minus_delta, y_at_lon_plus_delta_lat_minus_delta);
			}
			else
			{
				map_projection.forward_transform(x_at_lon_plus_delta_lat_minus_delta, y_at_lon_plus_delta_lat_minus_delta);
			}
			//map_projection.forward_transform(x_at_lon_plus_delta_lat_minus_delta, y_at_lon_plus_delta_lat_minus_delta);

			// Sample map projection at (longitude - delta, latitude + delta).
			double x_at_lon_minus_delta_lat_plus_delta = longitude - delta_lon_lat_for_derivs;
			double y_at_lon_minus_delta_lat_plus_delta = latitude + delta_lon_lat_for_derivs;
			if (map_projection.projection_type() == GPlatesGui::MapProjection::NUM_PROJECTIONS)
			{
				robin_forward_transform(x_at_lon_minus_delta_lat_plus_delta, y_at_lon_minus_delta_lat_plus_delta);
			}
			else
			{
				map_projection.forward_transform(x_at_lon_minus_delta_lat_plus_delta, y_at_lon_minus_delta_lat_plus_delta);
			}
			//map_projection.forward_transform(x_at_lon_minus_delta_lat_plus_delta, y_at_lon_minus_delta_lat_plus_delta);

			// Sample map projection at (longitude - delta, latitude - delta).
			double x_at_lon_minus_delta_lat_minus_delta = longitude - delta_lon_lat_for_derivs;
			double y_at_lon_minus_delta_lat_minus_delta = latitude - delta_lon_lat_for_derivs;
			if (map_projection.projection_type() == GPlatesGui::MapProjection::NUM_PROJECTIONS)
			{
				robin_forward_transform(x_at_lon_minus_delta_lat_minus_delta, y_at_lon_minus_delta_lat_minus_delta);
			}
			else
			{
				map_projection.forward_transform(x_at_lon_minus_delta_lat_minus_delta, y_at_lon_minus_delta_lat_minus_delta);
			}
			//map_projection.forward_transform(x_at_lon_minus_delta_lat_minus_delta, y_at_lon_minus_delta_lat_minus_delta);

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

			if (GPlatesMaths::are_almost_exactly_equal(latitude, 45.0))
			{
				qDebug()
						<< qSetRealNumberPrecision(12)
						<< "(lon, lat):" << longitude << latitude
						<< ", x" << x
						<< ", y" << y
						<< ", y_at_lat_plus_delta" << y_at_lat_plus_delta
						<< ", y_at_lat_minus_delta" << y_at_lat_minus_delta
						<< ", dx_dlon" << dx_dlon_radians
						<< ", dx_dlat" << dx_dlat_radians
						<< ", dy_dlon" << dy_dlon_radians
						<< ", dy_dlat" << dy_dlat_radians
						<< ", ddx_dlon_dlon:" << ddx_dlon_dlon_radians
						<< ", ddx_dlat_dlat:" << ddx_dlat_dlat_radians
						<< ", ddx_dlon_dlat:" << ddx_dlon_dlat_radians
						<< ", ddy_dlon_dlon:" << ddy_dlon_dlon_radians
						<< ", ddy_dlat_dlat:" << ddy_dlat_dlat_radians
						<< ", ddy_dlon_dlat:" << ddy_dlon_dlat_radians
						<< qSetRealNumberPrecision(6);
			}
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
