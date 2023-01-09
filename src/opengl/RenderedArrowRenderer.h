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

#ifndef GPLATES_OPENGL_RENDERED_ARROW_RENDERER_H
#define GPLATES_OPENGL_RENDERED_ARROW_RENDERER_H

#include <cstdint>
#include <vector>

#include "Vulkan.h"
#include "VulkanBuffer.h"

#include "gui/Colour.h"

#include "maths/UnitVector3D.h"


namespace GPlatesGui
{
	class SceneLightingParameters;
}

namespace GPlatesOpenGL
{
	class GLViewProjection;

	/**
	 * Draw rendered arrows.
	 */
	class RenderedArrowRenderer
	{
	public:

		explicit
		RenderedArrowRenderer(
				const GPlatesGui::SceneLightingParameters &scene_lighting_parameters);


		/**
		 * The Vulkan device was just created.
		 */
		void
		initialise_vulkan_resources(
				Vulkan &vulkan,
				vk::RenderPass default_render_pass,
				vk::CommandBuffer initialisation_command_buffer,
				vk::Fence initialisation_submit_fence);

		/**
		 * The Vulkan device is about to be destroyed.
		 */
		void
		release_vulkan_resources(
				Vulkan &vulkan);


		/**
		 * Added a rendered arrow (to be drawn when @a render is called).
		 *
		 * @param inverse_viewport_zoom_factor is used for rendering the view-dependent rendered arrows.
		 */
		void
		add(
				Vulkan &vulkan,
				const GPlatesMaths::Vector3D &arrow_start,
				const GPlatesMaths::Vector3D &arrow_vector,
				float arrow_body_width,
				float arrowhead_length,
				const GPlatesGui::Colour &arrow_colour,
				const double &inverse_viewport_zoom_factor);

		/**
		 * Draw the rendered arrows added so far (by @a add).
		 */
		void
		render(
				Vulkan &vulkan,
				vk::CommandBuffer preprocess_command_buffer,
				vk::CommandBuffer default_render_pass_command_buffer,
				const GLViewProjection &view_projection,
				bool is_globe_active);

	private:

		//
		// Compute push constants.
		//
		// layout (push_constant) uniform PushConstants
		// {
		//     vec4 frustum_planes[6];
		//     uint num_input_arrow_instances;
		// };
		//
		// NOTE: This fits within the minimum required size limit of 128 bytes for push constants.
		//       And push constants use the std430 layout.
		//
		struct ComputePushConstants
		{
			float frustum_planes[6][4];
			std::uint32_t num_input_arrow_instances;
		};

		//
		// Graphics push constants.
		//
		// layout (push_constant) uniform PushConstants
		// {
		//     mat4 view_projection;
		//     vec3 world_space_light_direction;
		//     bool lighting_enabled;
		//     float light_ambient_contribution;
		// };
		//
		// NOTE: This fits within the minimum required size limit of 128 bytes for push constants.
		//       And push constants use the std430 layout.
		//
		struct GraphicsPushConstants
		{
			float view_projection[16];
			float world_space_light_direction[3];
			std::uint32_t/*bool*/ lighting_enabled;
			float light_ambient_contribution;
		};


		/**
		 * Per-vertex data for an arrow.
		 *
		 * The mesh normal (used when calculating lighting in vertex/fragment shaders) is determined
		 * by weighting the radial normal and the axial normal. We do this instead of the usual
		 * storing of per-vertex normals because for a cone (used in arrow heads) it is difficult
		 * to get the correct lighting at the cone apex (even when using multiple apex vertices
		 * with same position but with different normals). For more details see
		 * http://stackoverflow.com/questions/15283508/low-polygon-cone-smooth-shading-at-the-tip
		 */
		struct MeshVertex
		{
			// Radial (x, y) position is either (0, 0) or on the unit circle.
			float model_space_normalised_radial_position[2];  // (x, y) position in model space

			// Weights that control the model-space surface normal.
			float model_space_radial_normal_weight;  // the radial (x, y) component of the model-space surface normal
			float model_space_axial_normal_weight;   // the axial (z) component of the model-space surface normal

			// Weights that identify where this vertex is on the arrow mesh.
			// These are used to change the model-space radial position (on unit circle) to an arrow instance's
			// model-space (x, y, z) position depending on that particular instance's body/head width/length.
			float arrow_body_width_weight;  // 1.0 if vertex is on arrow body (otherwise 0.0)
			float arrowhead_width_weight;   // 1.0 if vertex is on circular part of arrowhead cone (otherwise 0.0)
			float arrow_body_length_weight; // 1.0 if vertex is at end of arrow body or anywhere on arrowhead (otherwise 0.0)
			float arrowhead_length_weight;  // 1.0 if vertex is at pointy apex of arrowhead (otherwise 0.0)
		};

		/**
		 * Per-instance data for an arrow.
		 */
		struct MeshInstance
		{
			float world_space_start_position[3];  // (x, y, z) position in world space of base of arrow

			// World-space frame of reference of arrow instance.
			// This is used to transform the model-space position and surface normal to world space.
			float world_space_x_axis[3];
			float world_space_y_axis[3];
			float world_space_z_axis[3];  // direction arrow is pointing

			// This arrow's body/head width/length.
			float arrow_body_width;
			float arrowhead_width;
			float arrow_body_length;
			float arrowhead_length;

			float colour[4];  // arrow colour
		};

		struct InstanceResource
		{
			//! Vertex buffer containing per-instance data of all arrows.
			VulkanBuffer instance_buffer;

			//! Persistently mapped pointer to the per-instance buffer.
			void *mapped_pointer = nullptr;

			//! Vertex buffer containing per-instance data of those arrows visible in the view frustum.
			VulkanBuffer visible_instance_buffer;

			//! Contains draw command data vk::DrawIndexedIndirectCommand, used by vk::CommandBuffer::drawIndexedIndirect().
			VulkanBuffer indirect_draw_buffer;

			//! Descriptor pool to allocate descriptor set.
			vk::DescriptorPool compute_descriptor_pool;

			//! Descriptor set referencing the above (storage) buffers.
			vk::DescriptorSet compute_descriptor_set;

			void
			destroy(
					Vulkan &vulkan)
			{
				VulkanBuffer::destroy(vulkan.get_vma_allocator(), instance_buffer);
				VulkanBuffer::destroy(vulkan.get_vma_allocator(), visible_instance_buffer);
				VulkanBuffer::destroy(vulkan.get_vma_allocator(), indirect_draw_buffer);
				vulkan.get_device().destroyDescriptorPool(compute_descriptor_pool);  // also frees compute descriptor set
			}
		};


		void
		create_compute_pipeline(
				Vulkan &vulkan);

		void
		create_graphics_pipeline(
				Vulkan &vulkan,
				vk::RenderPass default_render_pass);

		void
		create_arrow_mesh(
				std::vector<MeshVertex> &vertices,
				std::vector<std::uint32_t> &vertex_indices);

		void
		load_arrow_mesh(
				Vulkan &vulkan,
				vk::CommandBuffer initialisation_command_buffer,
				vk::Fence initialisation_submit_fence,
				const std::vector<MeshVertex> &vertices,
				const std::vector<std::uint32_t> &vertex_indices);

		InstanceResource
		create_instance_resource(
				Vulkan &vulkan);


		/**
		 * Arbitrary compute shader work group size(x).
		 *
		 * Note: Should be less than vk::PhysicalDeviceLimits::maxComputeWorkGroupInvocations (min 128).
		 */
		static constexpr unsigned int COMPUTE_SHADER_WORK_GROUP_SIZE = 16;

		/**
		 * Number of arrow instances per dynamic buffer.
		 *
		 * Note: Should be less than vk::PhysicalDeviceLimits::maxComputeWorkGroupCount[0] (min 65535) multiplied by
		 *       @a COMPUTE_SHADER_WORK_GROUP_SIZE. For example, 65536 * 16 = 1,048,560.
		 */
		static constexpr unsigned int NUM_ARROWS_PER_INSTANCE_BUFFER = 50000;  // ~4MB (at 80 bytes per instance)

		/**
		 * Arrow mesh tessellation (how many vertices in a circular cross-section of arrow body or head).
		 */
		static constexpr unsigned int NUM_VERTICES_IN_ARROW_CIRCULAR_CROSS_SECTION = 16;

		/**
		 * Ratio of an arrowhead width to length.
		 */
		static constexpr float ARROWHEAD_WIDTH_TO_LENGTH_RATIO = 0.5;

		/**
		 * We want to keep the projected arrowhead size constant regardless of the
		 * the length of the arrow body, except...
		 *
		 * ...if the ratio of arrowhead size to arrow body length is large enough then
		 * we need to start scaling the arrowhead size by the arrow body length so
		 * that the arrowhead disappears as the arrow body disappears.
		 */
		static constexpr float MAX_RATIO_ARROWHEAD_TO_ARROW_BODY_LENGTH = 0.5;


		/**
		 * Lighting parameters such as whether lighting enabled for rendered arrows,
		 * the light direction and ambient contribution.
		 */
		const GPlatesGui::SceneLightingParameters &d_scene_lighting_parameters;

		// Compute pipeline and layout.
		vk::DescriptorSetLayout d_compute_descriptor_set_layout;
		vk::PipelineLayout d_compute_pipeline_layout;
		vk::Pipeline d_compute_pipeline;

		// Graphics pipeline and layout.
		vk::PipelineLayout d_graphics_pipeline_layout;
		vk::Pipeline d_graphics_pipeline;

		//! Vertex buffer containing per-vertex data (static buffer).
		VulkanBuffer d_vertex_buffer;
		//! Index buffer (static buffer).
		VulkanBuffer d_index_buffer;

		//! Number of vertex indices for the single arrow mesh.
		std::uint32_t d_num_vertex_indices = 0;

		/**
		 * Instance resources that are available for use (not currently being used).
		 */
		std::vector<InstanceResource> d_available_instance_resources;

		/**
		 * Instance resources currently being used.
		 *
		 * Note: Each asynchronous frame can render one or more instance buffers (depending on the number of arrows).
		 */
		std::vector<InstanceResource> d_async_instance_resources[Vulkan::NUM_ASYNC_FRAMES];

		/**
		 * Number of arrows to render.
		 *
		 * This is incremented by @a add and reset to zero upon returning from @a render.
		 */
		std::uint32_t d_num_arrows_to_render = 0;
	};
}

#endif // GPLATES_OPENGL_RENDERED_ARROW_RENDERER_H
