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

#include "Vulkan.h"
#include "VulkanBuffer.h"
#include "VulkanImage.h"

#include "gui/MapProjection.h"


namespace GPlatesOpenGL
{
	/**
	 * Image mapping latitude and longitude to a map projection space.
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
		 * Return the descriptor image info for the map projection image (to be used for shader reads).
		 *
		 * This is valid as soon as @a initialise_vulkan_resources is called.
		 */
		vk::DescriptorImageInfo
		get_descriptor_image_info() const;

		/**
		 * Update the map projection image (if the map projection changed in any way since the last call to this method).
		 */
		void
		update(
				Vulkan &vulkan,
				vk::CommandBuffer preprocess_command_buffer,
				const GPlatesGui::MapProjection &d_map_projection);

	private:

		/**
		 * Staging buffer for uploading to an image of a specific map projection type.
		 */
		struct StagingBuffer
		{
			//! Contains map projection forward transform image data.
			VulkanBuffer forward_transform_buffer;
		};

		/**
		 * Texel containing (x, y) result of the map projection of a (longitude, latitude) position.
		 */
		struct Texel
		{
			float x;
			float y;
		};


		StagingBuffer
		create_staging_buffer(
				Vulkan &vulkan,
				const GPlatesGui::MapProjection &map_projection);


		static constexpr vk::Format TEXEL_FORMAT = vk::Format::eR32G32Sfloat;  // format for struct Texel
		static constexpr unsigned int NUM_TEXEL_INTERVALS_PER_90_DEGREES = 90;
		static constexpr double TEXEL_INTERVAL_IN_DEGREES = 90.0 / NUM_TEXEL_INTERVALS_PER_90_DEGREES;
		static constexpr unsigned int IMAGE_WIDTH = 4 * NUM_TEXEL_INTERVALS_PER_90_DEGREES + 1;
		static constexpr unsigned int IMAGE_HEIGHT = 2 * NUM_TEXEL_INTERVALS_PER_90_DEGREES + 1;

		/**
		 * Each map projection type caches its own staging buffer.
		 *
		 * This enables us to switch between them to upload to image when switching map projections.
		 * They only need to calculate map projection when first used.
		 */
		std::map<GPlatesGui::MapProjection::Type, StagingBuffer> d_staging_buffers;

		vk::Sampler d_sampler;
		VulkanImage d_image;
		vk::ImageView d_image_view;
	};
}

#endif // GPLATES_OPENGL_MAP_PROJECTION_IMAGE_H
