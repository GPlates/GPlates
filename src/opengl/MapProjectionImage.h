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
	 * Images mapping latitude and longitude to map projection space (x, y) and partial derivatives (dxdlon, dxdlat, dydlon, dydlat).
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
		 * Note: Both the forward transform and Jacobian matrix images have the same dimensions.
		 */
		unsigned int
		get_image_width() const
		{
			return IMAGE_WIDTH;
		}

		/**
		 * Returns the image height.
		 *
		 * Note: Both the forward transform and Jacobian matrix images have the same dimensions.
		 */
		unsigned int
		get_image_height() const
		{
			return IMAGE_HEIGHT;
		}


		/**
		 * Return the descriptor image info for the forward transform image of the map projection (to be used for shader reads).
		 *
		 * The forward transform maps (longitude, latitude) to map projection space coordinates (x, y).
		 * Each texel in the image contains a vec2 that represents (x, y) at a specific (longitude, latitude) coordinate.
		 *
		 * The image width represents the longitude range [-180, 180] and the image height represents the latitude range [-90, 90].
		 * Note that the ranges start/end at the boundary texel centres to ensure linear interpolation within boundary texels.
		 * For example, the leftmost texels represent the longitude -180 at their texel centres, and so the longitude -180
		 * should have a texture coordinate U of 0.5/image_width (instead of 0.0).
		 *
		 * This is valid as soon as @a initialise_vulkan_resources is called.
		 */
		vk::DescriptorImageInfo
		get_forward_transform_descriptor_image_info() const
		{
			return { d_sampler, d_forward_transform_image_view, vk::ImageLayout::eShaderReadOnlyOptimal };
		}

		/**
		 * Return the descriptor image info for the Jacobian matrix image of the map projection (to be used for shader reads).
		 *
		 * The Jacobian matrix contains the four partial derivatives of map projection output coordinates (x, y) relative to
		 * the input coordinates (longitude, latitude). Each texel in the image contains a vec4 that represents
		 * (dxdlon, dxdlat, dydlon, dydlat) at a specific (longitude, latitude) coordinate.
		 *
		 * The image width represents the longitude range [-180, 180] and the image height represents the latitude range [-90, 90].
		 * Note that the ranges start/end at the boundary texel centres to ensure linear interpolation within boundary texels.
		 * For example, the leftmost texels represent the longitude -180 at their texel centres, and so the longitude -180
		 * should have a texture coordinate U of 0.5/image_width (instead of 0.0).
		 *
		 * This is valid as soon as @a initialise_vulkan_resources is called.
		 */
		vk::DescriptorImageInfo
		get_jacobian_matrix_descriptor_image_info() const
		{
			return { d_sampler, d_jacobian_matrix_image_view, vk::ImageLayout::eShaderReadOnlyOptimal };
		}


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

		/**
		 * Staging buffer for uploading to the images for a specific map projection type.
		 */
		struct StagingBuffer
		{
			//! Contains map projection forward transform image data.
			VulkanBuffer forward_transform_buffer;

			//! Contains map projection Jacobian matrix image data.
			VulkanBuffer jacobian_matrix_buffer;
		};

		/**
		 * Texel containing (x, y) result of the map projection forward transform of a (longitude, latitude) position.
		 */
		struct ForwardTransformTexel
		{
			float x;
			float y;
		};

		/**
		 * Texel containing partial derivatives of map projection forward transform where
		 * the output (x, y) is in map projection space and the input is (longitude, latitude).
		 */
		struct JacobianMatrixTexel
		{
			float  dxdlon;
			float  dxdlat;
			float  dydlon;
			float  dydlat;
		};


		StagingBuffer
		create_staging_buffer(
				Vulkan &vulkan,
				const GPlatesGui::MapProjection &map_projection);


		//! Format for @a ForwardTransformTexel.
		static constexpr vk::Format FORWARD_TRANSFORM_TEXEL_FORMAT = vk::Format::eR32G32Sfloat;
		//! Format for @a JacobianMatrixTexel.
		static constexpr vk::Format JACOBIAN_MATRIX_TEXEL_FORMAT = vk::Format::eR32G32B32A32Sfloat;

		// Parameters common to both the forward transform and Jacobian matrix images.
		//
		// The delay when first switching to a map projection is noticeable when this is above 90.
		static constexpr unsigned int NUM_TEXEL_INTERVALS_PER_90_DEGREES = 90;
		static constexpr double TEXEL_INTERVAL_IN_DEGREES = 90.0 / NUM_TEXEL_INTERVALS_PER_90_DEGREES;
		static constexpr unsigned int IMAGE_WIDTH = 4 * NUM_TEXEL_INTERVALS_PER_90_DEGREES + 1;
		static constexpr unsigned int IMAGE_HEIGHT = 2 * NUM_TEXEL_INTERVALS_PER_90_DEGREES + 1;


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

		// Forward transform image and image view.
		VulkanImage d_forward_transform_image;
		vk::ImageView d_forward_transform_image_view;

		// Jacobian matrix image and image view.
		VulkanImage d_jacobian_matrix_image;
		vk::ImageView d_jacobian_matrix_image_view;
	};
}

#endif // GPLATES_OPENGL_MAP_PROJECTION_IMAGE_H
