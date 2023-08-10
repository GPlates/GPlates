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

#ifndef GPLATES_OPENGL_SCENETILE_H
#define GPLATES_OPENGL_SCENETILE_H

#include <cstdint>
#include <vector>

#include "Vulkan.h"
#include "VulkanBuffer.h"
#include "VulkanImage.h"


namespace GPlatesOpenGL
{
	/**
	 * Provides order-independent-transparency of 3D scene objects using per-pixel fragment lists (sorted by depth).
	 *
	 * Various 3D objects will render their primitives and add the resultant fragments to our per-pixel lists.
	 * We will then blend these per-pixel fragments (in depth order) into the final scene framebuffer.
	 *
	 * This is limited to a tile rather than being the size of the render target (eg, swapchain image) in order to
	 * limit graphics memory usage. Vulkan guarantees support of at least 128MB for the maximum storage buffer size,
	 * but we can fairly easily exceed this. For example, full-screen on a 4K monitor with an average of 8 fragments overlapping
	 * each pixel (at 16 bytes per fragment) consumes about 1GB of graphics memory.
	 *
	 * A 2D storage image contains the per-pixel list head pointers and a storage buffer contains the linked-list fragments.
	 */
	class SceneTile
	{
	public:

		SceneTile();

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
		 * Return the dimension of the square tile that scene (tiled) rendering should be limited to.
		 */
		std::uint32_t
		get_tile_dimension() const
		{
			return FRAGMENT_TILE_DIMENSION;
		}


		/**
		 * Return the structures used to write to the (specified) descriptor set and binding (used in shader).
		 *
		 * This writes the fragment list head image, and fragment storage and allocator buffers.
		 *
		 * This method can be called once @a initialise_vulkan_resources has been called (and before @a clear is first called).
		 */
		std::vector<vk::WriteDescriptorSet>
		get_write_descriptor_sets(
				vk::DescriptorSet descriptor_set,
				std::uint32_t binding,
				// These need to exist beyond function since referenced by the returned vk::WriteDescriptorSet structures...
				std::vector<vk::DescriptorImageInfo> &descriptor_image_infos,
				std::vector<vk::DescriptorBufferInfo> &descriptor_buffer_infos) const;

		/**
		 * Return the descriptor set layout bindings.
		 *
		 * Specify the binding point (used in shader) and which pipeline shader stages will access these bindings.
		 *
		 * This method can be called once @a initialise_vulkan_resources has been called (and before @a clear is first called).
		 */
		std::vector<vk::DescriptorSetLayoutBinding>
		get_descriptor_set_layout_bindings(
				std::uint32_t binding,
				vk::ShaderStageFlags shader_stage_flags) const;

		/**
		 * Return the descriptor pool sizes.
		 *
		 * This method can be called once @a initialise_vulkan_resources has been called (and before @a clear is first called).
		 */
		std::vector<vk::DescriptorPoolSize>
		get_descriptor_pool_sizes() const;


		/**
		 * Clear the scene fragment images/buffers in preparation for rendering a tile.
		 *
		 * This clears the fragment list head pointers and resets the global fragment allocator.
		 */
		void
		clear(
				Vulkan &vulkan,
				vk::CommandBuffer preprocess_command_buffer);

	private:

		//! Format for the storage image containing fragment list head pointers (a single unsigned 32-bit integer).
		static constexpr vk::Format FRAGMENT_LIST_HEAD_IMAGE_TEXEL_FORMAT = vk::Format::eR32Uint;

		/**
		 * Dimensions of square tile containing scene fragments.
		 *
		 * Note: Vulkan guarantees support for 2D images up to 4096 dimension.
		 *       Anything higher and we need to query device for support.
		 */
		static const unsigned int FRAGMENT_TILE_DIMENSION = 1024;

		//! Each fragment consumes this many bytes in the storage buffer (including the list 'next' pointer).
		static const unsigned int NUM_BYTES_PER_FRAGMENT = 12;


		//! The number of fragments that can be contained in the storage buffer.
		std::uint32_t d_num_fragments_in_storage;

		// Storage image (and image view) containing fragment list head pointers.
		VulkanImage d_fragment_list_head_image;
		vk::ImageView d_fragment_list_head_image_view;

		// Storage buffer containing all the scene fragments.
		VulkanBuffer d_fragment_storage_buffer;

		// Storage buffer containing the global fragment allocator.
		VulkanBuffer d_fragment_allocator_buffer;
	};
}

#endif // GPLATES_OPENGL_SCENETILE_H
