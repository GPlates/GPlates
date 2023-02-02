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

#ifndef GPLATES_OPENGL_MAP_PROJECTION_IMAGE_H
#define GPLATES_OPENGL_MAP_PROJECTION_IMAGE_H

#include <cstdint>
#include <map>
#include <vector>
#include <boost/optional.hpp>

#include "Vulkan.h"
#include "VulkanBuffer.h"
#include "VulkanImage.h"

#include "gui/MapProjection.h"


namespace GPlatesOpenGL
{
	/**
	 * Images mapping latitude and longitude to:
	 * - map projection space (x, y),
	 * - first-order partial derivatives dx/dlon, dx/dlat, dy/dlon, dy/dlat, and
	 * - second-order partial derivatives:
	 *   + d(dx/dlon)/dlon, d(dx/dlat)/dlat, d(dx/dlon)/dlat (for 'x') and
	 *   + d(dy/dlon)/dlon, d(dy/dlat)/dlat, d(dy/dlon)/dlat (for 'y').
	 */
	class MapProjectionImage
	{
	public:

		/**
		 * The Vulkan device was just created.
		 */
		void
		initialise_vulkan_resources(
				Vulkan &vulkan,
				vk::CommandBuffer initialisation_command_buffer,
				vk::Fence initialisation_submit_fence);

		/**
		 * The Vulkan device is about to be destroyed.
		 */
		void
		release_vulkan_resources(
				Vulkan &vulkan);


		/**
		 * Returns the image width.
		 *
		 * Note: All images have the same dimensions.
		 */
		unsigned int
		get_image_width() const
		{
			return IMAGE_WIDTH;
		}

		/**
		 * Returns the image height.
		 *
		 * Note: All images have the same dimensions.
		 */
		unsigned int
		get_image_height() const
		{
			return IMAGE_HEIGHT;
		}


		/**
		 * Return the descriptor image infos for the three images containing map projection data (to be used for shader reads).
		 *
		 * Each texel in each image represents data at a specific (longitude, latitude) coordinate.
		 *
		 * The forward transform data maps (longitude, latitude) to map projection space coordinates (x, y).
		 *
		 * The Jacobian matrix data contains the four 1st order partial derivatives of map projection output coordinates (x, y)
		 * relative to the input coordinates (longitude, latitude).
		 * These are dx/dlon, dx/dlat, dy/dlon, dy/dlat.
		 *
		 * The Hessian matrix data contains the eight 2nd order partial derivatives of map projection output coordinates (x, y)
		 * relative to the input coordinates (longitude, latitude).
		 * The Hessian matrix of 'x' has 4 components and Hessian matrix of 'y' has 4 components.
		 * However, since Hessian matrices are symmetric we only store 3 components each.
		 * These are d(dx/dlon)/dlon, d(dx/dlat)/dlat, d(dx/dlon)/dlat (for 'x') and d(dy/dlon)/dlon, d(dy/dlat)/dlat, d(dy/dlon)/dlat (for 'y').
		 * The off-diagonal elements are stored in the first image and the diagonal elements in the third image.
		 *
		 * Each texel in the FIRST image contains 4 components:
		 *   0: forward transformed x coordinate
		 *   1: forward transformed y coordinate
		 *   2: Hessian matrix d(dx/dlon)/dlat (same as d(dx/dlat)/dlon since Hessian matrix is symmetric)
		 *   3: Hessian matrix d(dy/dlon)/dlat (same as d(dy/dlat)/dlon since Hessian matrix is symmetric)
		 *
		 * Each texel in the SECOND image contains 4 components:
		 *   0: Jacobian matrix dx/dlon
		 *   1: Jacobian matrix dx/dlat
		 *   2: Jacobian matrix dy/dlon
		 *   3: Jacobian matrix dy/dlat
		 *
		 * Each texel in the THIRD image contains 4 components:
		 *   0: Hessian matrix d(dx/dlon)/dlon
		 *   1: Hessian matrix d(dx/dlat)/dlat
		 *   2: Hessian matrix d(dy/dlon)/dlon
		 *   3: Hessian matrix d(dy/dlat)/dlat
		 *
		 * The image width represents the longitude range [-180, 180] and the image height represents the latitude range [-90, 90].
		 * Note that the ranges start/end at the boundary texel centres to ensure linear interpolation within boundary texels.
		 * For example, the leftmost texels represent the longitude -180 at their texel centres, and so the longitude -180
		 * should have a texture coordinate S of 0.5/image_width (instead of 0.0).
		 *
		 * This method can be called once @a initialise_vulkan_resources has been called (and before @a update is first called).
		 */
		std::vector<vk::DescriptorImageInfo>
		get_descriptor_image_infos() const;


		/**
		 * Update the map projection forward transform and Jacobian matrix images
		 * (if the map projection *type* changed since the last call to this method).
		 */
		void
		update(
				Vulkan &vulkan,
				vk::CommandBuffer preprocess_command_buffer,
				const GPlatesGui::MapProjection &d_map_projection);

	private:

		//! Three images contain all the map projection data.
		static constexpr unsigned int NUM_IMAGES = 3;

		//! Format for all images (all images contains texels with 4 floating-point values).
		static constexpr vk::Format TEXEL_FORMAT = vk::Format::eR32G32B32A32Sfloat;

		// Parameters common to both the forward transform and Jacobian matrix images.
		//
		// The delay when first switching to a map projection is significant when this is above 180 (on a circa 2013 CPU).
		static constexpr unsigned int NUM_TEXEL_INTERVALS_PER_180_DEGREES = 180;
		static constexpr double TEXEL_INTERVAL_IN_DEGREES = 180.0 / NUM_TEXEL_INTERVALS_PER_180_DEGREES;
		static constexpr unsigned int IMAGE_WIDTH = 2 * NUM_TEXEL_INTERVALS_PER_180_DEGREES + 1;
		static constexpr unsigned int IMAGE_HEIGHT = NUM_TEXEL_INTERVALS_PER_180_DEGREES + 1;


		/**
		 * Staging buffer for uploading to the images for a specific map projection type.
		 */
		struct StagingBuffer
		{
			//! Contains image data for map projection forward transform, and its Jacobian and Hessian matrices.
			VulkanBuffer buffers[NUM_IMAGES];
		};

		/**
		 * Texel containing 4 floating-point values of map projection data at a (longitude, latitude) position.
		 */
		struct Texel
		{
			float values[4];
		};


		StagingBuffer
		create_staging_buffer(
				Vulkan &vulkan,
				const GPlatesGui::MapProjection &map_projection);


		//! The map projection type updated in the last call to @a update (or not if none yet updated).
		boost::optional<GPlatesGui::MapProjection::Type> d_last_updated_map_projection_type;

		/**
		 * Each map projection type caches its own staging buffer.
		 *
		 * This enables us to switch between them to upload to image when switching map projections.
		 * They only need to calculate map projection when first used.
		 */
		std::map<GPlatesGui::MapProjection::Type, StagingBuffer> d_staging_buffers;

		vk::Sampler d_sampler;

		// Image and image view for containing data for map projection forward transform, and its Jacobian and Hessian matrices.
		VulkanImage d_images[NUM_IMAGES];
		vk::ImageView d_image_views[NUM_IMAGES];
	};
}

#endif // GPLATES_OPENGL_MAP_PROJECTION_IMAGE_H
