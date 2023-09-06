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
	class GLViewProjection;

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
				vk::RenderPass default_render_pass,
				vk::SampleCountFlagBits default_render_pass_sample_count,
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
		 * Add specialization constants (used by this scene tile) to the specified data and map entry arrays.
		 *
		 * The specified "constant IDs" are those used in the caller's fragment shader.
		 *
		 * The specified specialization data (and associated map entries) can be non-empty when
		 * calling this function if the caller already has specialization data of its own.
		 */
		void
		get_specialization_constants(
				std::vector<std::uint32_t> &fragment_shader_specialization_data,
				std::vector<vk::SpecializationMapEntry> &fragment_shader_specialization_map_entries,
				std::uint32_t tile_dimension_constant_id,
				std::uint32_t num_fragments_in_storage_constant_id,
				std::uint32_t max_fragments_per_sample_constant_id,
				std::uint32_t sample_count_constant_id,
				vk::SampleCountFlagBits default_render_pass_sample_count) const;


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
		 *
		 * NOTE: This is done inside a render pass (since it uses graphics operations to perform the clear).
		 */
		void
		clear(
				Vulkan &vulkan,
				vk::CommandBuffer default_render_pass_command_buffer,
				const GLViewProjection &view_projection);


		/**
		 * Renders a tile by blending per-pixel fragment lists into the framebuffer.
		 */
		void
		render(
				Vulkan &vulkan,
				vk::CommandBuffer default_render_pass_command_buffer,
				const GLViewProjection &view_projection);

	private:

		void
		create_graphics_pipelines(
				Vulkan &vulkan,
				vk::RenderPass default_render_pass,
				vk::SampleCountFlagBits default_render_pass_sample_count);

		void
		create_descriptors(
				Vulkan &vulkan,
				vk::CommandBuffer initialisation_command_buffer,
				vk::Fence initialisation_submit_fence);

		void
		create_descriptor_set(
				Vulkan &vulkan);


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
		static const unsigned int NUM_BYTES_PER_FRAGMENT = 16;

		/**
		 * The number of scene fragments (per pixel) requested for storage.
		 *
		 * The total number of fragments requested for storage is this value multiplied by the square of the tile dimension.
		 *
		 * The actual number in storage may be less if the total requested tile storage exceeds the
		 * maximum storage buffer range (Vulkan guarantees support for at least 128MB).
		 *
		 * Ideally this should be a power-of-two value because any unused high-order bits in a 32-bit uint are used as
		 * a fragment tag bits (to help avoid the ABA problem - see https://en.wikipedia.org/wiki/ABA_problem).
		 * If this value is a power-of-two then the total number of fragments in storage will be a power-of-two and use
		 * up the lower-order bits exactly, leaving the remaining high-order bits for use as tag bits (the more the better).
		 * But of course the memory usage should be considered first and foremost.
		 */
		static const unsigned int REQUESTED_NUM_FRAGMENTS_IN_STORAGE_PER_PIXEL = 8;

		/**
		 * The maximum number of fragments covering any sample (in a pixel).
		 *
		 * Only this many fragments (closest to the viewer in z) are blended together into the framebuffer.
		 *
		 * Note: This is per-sample (rather than per-pixel). So, for example, with 4 samples per pixel (4xMSAA)
		 *       it is possible (though unlikely) that each sample is covered by separate fragments
		 *       (eg, 4 adjacent triangles intersect a single pixel with each triangle covering a single sample).
		 *       This would result in 4x the number of fragments than if a single triangle covered the entire pixel.
		 */
		static const unsigned int MAX_FRAGMENTS_PER_SAMPLE = 6;

		//! The descriptor 'binding' used in the graphics pipelines.
		static constexpr unsigned int DESCRIPTOR_BINDING = 0;

		//! The tile dimension 'constant_id' used in the graphics pipelines.
		static constexpr unsigned int FRAGMENT_TILE_DIMENSION_CONSTANT_ID = 0;
		//! The total number of fragments (in storage) 'constant_id' used in the graphics pipelines.
		static constexpr unsigned int NUM_FRAGMENTS_IN_STORAGE_CONSTANT_ID = 1;
		//! The maximum number of fragments (per sample) 'constant_id' used in the graphics pipelines.
		static constexpr unsigned int MAX_FRAGMENTS_PER_SAMPLE_CONSTANT_ID = 2;
		//! The sample count 'constant_id' used in the graphics pipelines.
		static constexpr unsigned int SAMPLE_COUNT_CONSTANT_ID = 3;


		// Descriptor set layout.
		vk::DescriptorSetLayout d_descriptor_set_layout;

		// Descriptor pool.
		vk::DescriptorPool d_descriptor_pool;

		// Descriptor set.
		vk::DescriptorSet d_descriptor_set;

		// The graphics pipeline layout.
		vk::PipelineLayout d_graphics_pipeline_layout;

		// The "clear" graphics pipeline and layout.
		vk::Pipeline d_clear_graphics_pipeline;

		// The "blend" graphics pipeline.
		vk::Pipeline d_blend_graphics_pipeline;

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
