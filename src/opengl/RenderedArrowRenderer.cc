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

#include <cmath>
#include <cstddef>  // offsetof
#include <cstring>  // memcpy
#include <vector>

#include "RenderedArrowRenderer.h"

#include "GLFrustum.h"
#include "GLMatrix.h"
#include "GLViewProjection.h"
#include "MapProjectionImage.h"
#include "VulkanException.h"
#include "VulkanUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "gui/SceneLightingParameters.h"

#include "maths/MathsUtils.h"

#include "utils/CallStackTracker.h"


GPlatesOpenGL::RenderedArrowRenderer::RenderedArrowRenderer(
		const GPlatesGui::SceneLightingParameters &scene_lighting_parameters) :
	d_scene_lighting_parameters(scene_lighting_parameters)
{
}


void
GPlatesOpenGL::RenderedArrowRenderer::initialise_vulkan_resources(
		Vulkan &vulkan,
		vk::RenderPass default_render_pass,
		const MapProjectionImage &map_projection_image,
		vk::CommandBuffer initialisation_command_buffer,
		vk::Fence initialisation_submit_fence)
{
	// Add this scope to the call stack trace printed if exception thrown in this scope.
	TRACK_CALL_STACK();

	// Create the compute pipeline (to cull arrows outside the view frustum prior to rendering).
	create_compute_pipeline(vulkan);

	// Create the graphics pipeline (to render arrows frustum culled by the compute shader).
	create_graphics_pipeline(vulkan, default_render_pass);

	// Create the arrow mesh and load it into the vertex/index buffers.
	std::vector<MeshVertex> vertices;
	std::vector<std::uint32_t> vertex_indices;
	create_arrow_mesh(vertices, vertex_indices);
	load_arrow_mesh(vulkan, initialisation_command_buffer, initialisation_submit_fence, vertices, vertex_indices);

	// Create descriptor set for map projection textures.
	create_map_projection_descriptor_set(vulkan, map_projection_image);
}


void
GPlatesOpenGL::RenderedArrowRenderer::release_vulkan_resources(
		Vulkan &vulkan)
{
	// Vulkan memory allocator.
	VmaAllocator vma_allocator = vulkan.get_vma_allocator();

	// Destroy the vertex/index/instance buffers.
	VulkanBuffer::destroy(vma_allocator, d_vertex_buffer);
	VulkanBuffer::destroy(vma_allocator, d_index_buffer);
	for (auto &instance_resources : d_async_instance_resources)
	{
		for (auto &instance_resource : instance_resources)
		{
			instance_resource.destroy(vulkan);
		}
		instance_resources.clear();
	}
	for (auto &instance_resource : d_available_instance_resources)
	{
		instance_resource.destroy(vulkan);
	}
	d_available_instance_resources.clear();

	d_num_vertex_indices = 0;
	d_num_arrows_to_render = 0;

	// Destroy the graphics pipeline layout and pipeline.
	vulkan.get_device().destroyPipeline(d_graphics_pipeline);
	vulkan.get_device().destroyPipelineLayout(d_graphics_pipeline_layout);

	// Destroy the compute pipeline layout and pipeline.
	vulkan.get_device().destroyPipeline(d_compute_pipeline);
	vulkan.get_device().destroyPipelineLayout(d_compute_pipeline_layout);

	// Destroy descriptor set layouts.
	vulkan.get_device().destroyDescriptorSetLayout(d_instance_descriptor_set_layout);
	vulkan.get_device().destroyDescriptorSetLayout(d_map_projection_descriptor_set_layout);

	// Destroy descriptor pool/set for the map projection texture.
	vulkan.get_device().destroyDescriptorPool(d_map_projection_descriptor_pool);  // also frees descriptor set
}


void
GPlatesOpenGL::RenderedArrowRenderer::add(
		Vulkan &vulkan,
		const GPlatesMaths::PointOnSphere &arrow_start,
		const GPlatesMaths::Vector3D &arrow_vector,
		float arrow_body_width,
		float arrowhead_length,
		const GPlatesGui::Colour &arrow_colour)
{
	MeshInstance arrow_instance;
	// Copy arrow start position into instance data.
	arrow_instance.arrow_start[0] = arrow_start.position_vector().x().dval();
	arrow_instance.arrow_start[1] = arrow_start.position_vector().y().dval();
	arrow_instance.arrow_start[2] = arrow_start.position_vector().z().dval();
	// Copy arrow vector into instance data.
	arrow_instance.arrow_vector[0] = arrow_vector.x().dval();
	arrow_instance.arrow_vector[1] = arrow_vector.y().dval();
	arrow_instance.arrow_vector[2] = arrow_vector.z().dval();
	// Copy arrow body length and arrowhead width into instance data.
	arrow_instance.arrow_body_width = arrow_body_width;
	arrow_instance.arrowhead_length = arrowhead_length;
	// Copy arrow colour into instance data.
	arrow_instance.colour[0] = arrow_colour.red();
	arrow_instance.colour[1] = arrow_colour.green();
	arrow_instance.colour[2] = arrow_colour.blue();
	arrow_instance.colour[3] = arrow_colour.alpha();


	// Instance resources used for rendering the current frame.
	auto &instance_resources = d_async_instance_resources[vulkan.get_frame_index()];

	// If we're encountering a new frame then the resources used NUM_ASYNC_FRAMES frames ago are now available for re-use.
	if (d_num_arrows_to_render == 0)
	{
		d_available_instance_resources.insert(
				d_available_instance_resources.end(),
				instance_resources.begin(),
				instance_resources.end());
		instance_resources.clear();
	}

	// The arrow instance index into the current instance buffer.
	const std::uint32_t instance_index_in_current_buffer = d_num_arrows_to_render % NUM_ARROWS_PER_INSTANCE_BUFFER;

	// If we need to use a new instance buffer then get one that's available or create a new one.
	if (instance_index_in_current_buffer == 0)
	{
		// Create a new instance resource if none currently available.
		if (d_available_instance_resources.empty())
		{
			d_available_instance_resources.push_back(create_instance_resource(vulkan));
		}

		// Pop available instance resources and use for current frame.
		instance_resources.push_back(d_available_instance_resources.back());
		d_available_instance_resources.pop_back();
	}

	// The instance resource that we're currently rendering with.
	InstanceResource &instance_resource = instance_resources.back();
	// Copy the arrow instance data into the current instance buffer.
	std::memcpy(
			// Pointer to current arrow instance within persistently-mapped buffer...
			static_cast<std::uint8_t *>(instance_resource.mapped_pointer) + instance_index_in_current_buffer * sizeof(MeshInstance),
			&arrow_instance,
			sizeof(MeshInstance));

	++d_num_arrows_to_render;
}


void
GPlatesOpenGL::RenderedArrowRenderer::render(
		Vulkan &vulkan,
		vk::CommandBuffer preprocess_command_buffer,
		vk::CommandBuffer default_render_pass_command_buffer,
		const GLViewProjection &view_projection,
		const double &inverse_viewport_zoom_factor,
		bool is_map_active,
		const double &map_projection_central_meridian)
{
	// Return early if no arrows to render.
	if (d_num_arrows_to_render == 0)
	{
		return;
	}

	// Convert clip space from OpenGL to Vulkan and pre-multiply projection transform.
	GLMatrix vulkan_view_projection = VulkanUtils::from_opengl_clip_space();
	vulkan_view_projection.gl_mult_matrix(view_projection.get_view_projection_transform());


	//
	// Compute pipeline.
	//

	// Bind the compute pipeline.
	preprocess_command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute, d_compute_pipeline);

	//
	// Compute push constants.
	//
	// layout (push_constant) uniform PushConstants
	// {
	//     vec4 frustum_planes[6];
	//     bool use_map_projection;
	//     float map_projection_central_meridian;
	//     float arrow_size_scale_factor;
	//     float max_ratio_arrowhead_length_to_arrow_length;
	//     float arrowhead_width_to_length_ratio;
	//     uint num_input_arrow_instances;
	// };
	//
	// NOTE: This fits within the minimum required size limit of 128 bytes for push constants.
	//       And push constants use the std430 layout.
	//
	ComputePushConstants compute_push_constants = {};

	// Set the frustum planes.
	GLFrustum frustum(vulkan_view_projection);
	const GLIntersect::Plane *frustum_planes = frustum.get_planes();
	for (unsigned int n = 0; n < 6; ++n)
	{
		frustum_planes[n].get_float_plane(compute_push_constants.frustum_planes[n]);
	}

	// Map projection push constants (only used if map is active).
	compute_push_constants.use_map_projection = is_map_active;
	compute_push_constants.map_projection_central_meridian = GPlatesMaths::convert_deg_to_rad(map_projection_central_meridian);

	// Extra push constants.
	compute_push_constants.arrow_size_scale_factor = inverse_viewport_zoom_factor *
			// Apply map projected arrow scale factor only in map view...
			(is_map_active ? MAP_PROJECTED_ARROW_SCALE_FACTOR : 1.0);
	compute_push_constants.max_ratio_arrowhead_length_to_arrow_length = MAX_RATIO_ARROWHEAD_LENGTH_TO_ARROW_LENGTH;
	compute_push_constants.arrowhead_width_to_length_ratio = ARROWHEAD_WIDTH_TO_LENGTH_RATIO;

	// Set all push constants except the number of instances.
	preprocess_command_buffer.pushConstants(
			d_compute_pipeline_layout,
			vk::ShaderStageFlagBits::eCompute,
			// Update everything except the number of instances...
			0,
			offsetof(ComputePushConstants, num_input_arrow_instances),  // size
			&compute_push_constants);

	// Initialise the command in each indirect draw buffer.
	// Note: The instance count needs to be reset to zero at every frame (the other data is static).
	for (auto &instance_resource : d_async_instance_resources[vulkan.get_frame_index()])
	{
		vk::DrawIndexedIndirectCommand draw_indexed_indirect_command;
		draw_indexed_indirect_command
				.setIndexCount(d_num_vertex_indices)
				.setInstanceCount(0)  // the only dynamic data in the command (updated by compute shader)
				.setFirstIndex(0)
				.setVertexOffset(0)
				.setFirstInstance(0);
		preprocess_command_buffer.updateBuffer(
				instance_resource.indirect_draw_buffer.get_buffer(),
				vk::DeviceSize(0),
				vk::DeviceSize(sizeof(vk::DrawIndexedIndirectCommand)),
				&draw_indexed_indirect_command);
	}

	// Pipeline barrier to wait for the above copy (update buffer) operations to complete before accessing in compute shader.
#if defined(_WIN32) && defined(MemoryBarrier) // See "VulkanHpp.h" for why this is necessary.
#	undef MemoryBarrier
#endif
	vk::MemoryBarrier reset_indirect_draw_memory_barrier;
	reset_indirect_draw_memory_barrier
			.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
			.setDstAccessMask(vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite);
	preprocess_command_buffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eComputeShader,
			{},  // dependencyFlags
			reset_indirect_draw_memory_barrier,  // memoryBarriers
			{},  // bufferMemoryBarriers
			{});  // imageMemoryBarriers

	// Bind the arrow instance buffers of the current frame.
	auto &instance_resources = d_async_instance_resources[vulkan.get_frame_index()];
	for (unsigned int instance_resource_index = 0; instance_resource_index < instance_resources.size(); ++instance_resource_index)
	{
		auto &instance_resource = instance_resources[instance_resource_index];

		// All instance buffers, except the last, render the full number of arrows that fit in an instance buffer.
		const unsigned int num_arrows_in_instance_buffer = (instance_resource_index == instance_resources.size() - 1)
				? (d_num_arrows_to_render % NUM_ARROWS_PER_INSTANCE_BUFFER)
				: NUM_ARROWS_PER_INSTANCE_BUFFER;

		// Flush the mapped instance data (only happens if instance buffer is in *non-coherent* host-visible memory).
		//
		// Note: Writes to host mapped memory are automatically made visible to the device (GPU) when command buffer is submitted.
		instance_resource.instance_buffer.flush_mapped_memory(
				vulkan.get_vma_allocator(),
				0,
				num_arrows_in_instance_buffer * sizeof(MeshInstance),
				GPLATES_EXCEPTION_SOURCE);

		// Bind the descriptor sets used by compute shader.
		//
		// Set 0: Instance and indirect draw storage buffer descriptors.
		// Set 1: Map projection image descriptor.
		//        Note: this is only used when map is active, but still must be bound since is "statically" used in shader.
		preprocess_command_buffer.bindDescriptorSets(
				vk::PipelineBindPoint::eCompute,
				d_compute_pipeline_layout,
				0/*firstSet*/,
				{ instance_resource.compute_descriptor_set, d_map_projection_descriptor_set },
				nullptr/*dynamicOffsets*/);

		// Set the number-of-instances push constant.
		compute_push_constants.num_input_arrow_instances = num_arrows_in_instance_buffer;
		preprocess_command_buffer.pushConstants(
				d_compute_pipeline_layout,
				vk::ShaderStageFlagBits::eCompute,
				// Update only the number of arrow instances...
				offsetof(ComputePushConstants, num_input_arrow_instances),
				sizeof(ComputePushConstants::num_input_arrow_instances),
				&compute_push_constants.num_input_arrow_instances);

		// Dispatch compute shader.
		std::uint32_t num_arrow_work_groups = num_arrows_in_instance_buffer / COMPUTE_SHADER_WORK_GROUP_SIZE;
		if (num_arrows_in_instance_buffer % COMPUTE_SHADER_WORK_GROUP_SIZE) ++num_arrow_work_groups;
		preprocess_command_buffer.dispatch(num_arrow_work_groups, 1, 1);
	}


	// Pipeline barrier to wait for compute shader writes to be made visible for use as vertex data and indirect draw data.
	//
	// Note: The preprocess command buffer (containing our compute shader dispatches) will be submitted before the
	//       default render pass command buffer (containing our indirect draws).
	//       And both command buffers will be submitted to the same queue (the graphics+compute queue).
#if defined(_WIN32) && defined(MemoryBarrier) // See "VulkanHpp.h" for why this is necessary.
#	undef MemoryBarrier
#endif
	vk::MemoryBarrier compute_to_graphics_memory_barrier;
	compute_to_graphics_memory_barrier
			.setSrcAccessMask(vk::AccessFlagBits::eShaderWrite)
			.setDstAccessMask(vk::AccessFlagBits::eVertexAttributeRead | vk::AccessFlagBits::eIndirectCommandRead);
	preprocess_command_buffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eComputeShader,
			vk::PipelineStageFlagBits::eVertexInput | vk::PipelineStageFlagBits::eDrawIndirect,
			{},  // dependencyFlags
			compute_to_graphics_memory_barrier,  // memoryBarriers
			{},  // bufferMemoryBarriers
			{});  // imageMemoryBarriers


	//
	// Graphics pipeline.
	//

	// Bind the graphics pipeline.
	default_render_pass_command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, d_graphics_pipeline);

	// Set viewport and scissor rects.
	default_render_pass_command_buffer.setViewport(0, view_projection.get_viewport().get_vulkan_viewport());
	default_render_pass_command_buffer.setScissor(0, view_projection.get_viewport().get_vulkan_rect_2D());

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
	GraphicsPushConstants graphics_push_constants = {};

	// Set view projection matrix.
	vulkan_view_projection.get_float_matrix(graphics_push_constants.view_projection);

	// Is lighting enabled for arrows?
	graphics_push_constants.lighting_enabled = d_scene_lighting_parameters.is_lighting_enabled(
			GPlatesGui::SceneLightingParameters::LIGHTING_DIRECTION_ARROW);
	if (graphics_push_constants.lighting_enabled)
	{
		// Light direction.
		const GPlatesMaths::UnitVector3D &world_space_light_direction = is_map_active
				? d_scene_lighting_parameters.get_map_view_light_direction()
				: d_scene_lighting_parameters.get_globe_view_light_direction();
		graphics_push_constants.world_space_light_direction[0] = world_space_light_direction.x().dval();
		graphics_push_constants.world_space_light_direction[1] = world_space_light_direction.y().dval();
		graphics_push_constants.world_space_light_direction[2] = world_space_light_direction.z().dval();

		// Ambient light contribution.
		graphics_push_constants.light_ambient_contribution = d_scene_lighting_parameters.get_ambient_light_contribution();
	}

	// Set the push constants.
	default_render_pass_command_buffer.pushConstants(
			d_graphics_pipeline_layout,
			vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
			// Update all push constants...
			0, sizeof(GraphicsPushConstants), &graphics_push_constants);

	// Bind the arrow mesh vertex and index buffers.
	default_render_pass_command_buffer.bindVertexBuffers(0, d_vertex_buffer.get_buffer(), vk::DeviceSize(0));
	default_render_pass_command_buffer.bindIndexBuffer(d_index_buffer.get_buffer(), vk::DeviceSize(0), vk::IndexType::eUint32);

	// Bind and draw the visible arrow instance buffers of the current frame.
	for (auto &instance_resource : d_async_instance_resources[vulkan.get_frame_index()])
	{
		// Bind visible arrow instance buffer.
		default_render_pass_command_buffer.bindVertexBuffers(1, instance_resource.visible_instance_buffer.get_buffer(), vk::DeviceSize(0));

		// Draw visible arrows (in view frustum).
		//
		// There's a single arrow mesh that gets instanced by the number of arrows.
		// Each instance supplies an arrow position and direction in world space, a body/head width/length and a colour.
		//
		// Note: The draw command parameters are sourced from the buffer (parameters that were written by compute shader).
		default_render_pass_command_buffer.drawIndexedIndirect(instance_resource.indirect_draw_buffer.get_buffer(), 0, 1, sizeof(vk::DrawIndexedIndirectCommand));
	}

	// Reset the number of arrows to render.
	d_num_arrows_to_render = 0;
}


void
GPlatesOpenGL::RenderedArrowRenderer::create_compute_pipeline(
		Vulkan &vulkan)
{
	//
	// Shader stage.
	//
	// Compute shader.
	const std::vector<std::uint32_t> compute_shader_code = VulkanUtils::load_shader_code(":/arrows.comp.spv");
	vk::UniqueShaderModule compute_shader_module = vulkan.get_device().createShaderModuleUnique({ {}, {compute_shader_code} });
	// Specialization constants (set the 'local_size_x' in the compute shader).
	std::uint32_t specialization_data[1] = { COMPUTE_SHADER_WORK_GROUP_SIZE };
	vk::SpecializationMapEntry specialization_map_entries[1] = { { 1/*constant_id*/, 0/*offset*/, sizeof(std::uint32_t) } };
	vk::SpecializationInfo specialization_info;
	specialization_info
			.setMapEntries(specialization_map_entries)
			.setData<std::uint32_t>(specialization_data);
	vk::PipelineShaderStageCreateInfo shader_stage_create_info;
	shader_stage_create_info
			.setStage(vk::ShaderStageFlagBits::eCompute)
			.setModule(compute_shader_module.get())
			.setPName("main")
			.setPSpecializationInfo(&specialization_info);

	//
	// Pipeline layout.
	//

	// Instance descriptor set layout.
	vk::DescriptorSetLayoutBinding instance_descriptor_set_layout_bindings[3];
	instance_descriptor_set_layout_bindings[0]
			.setBinding(0)
			.setDescriptorType(vk::DescriptorType::eStorageBuffer)
			.setDescriptorCount(1)
			.setStageFlags(vk::ShaderStageFlagBits::eCompute);
	instance_descriptor_set_layout_bindings[1]
			.setBinding(1)
			.setDescriptorType(vk::DescriptorType::eStorageBuffer)
			.setDescriptorCount(1)
			.setStageFlags(vk::ShaderStageFlagBits::eCompute);
	instance_descriptor_set_layout_bindings[2]
			.setBinding(2)
			.setDescriptorType(vk::DescriptorType::eStorageBuffer)
			.setDescriptorCount(1)
			.setStageFlags(vk::ShaderStageFlagBits::eCompute);
	vk::DescriptorSetLayoutCreateInfo instance_descriptor_set_layout_create_info;
	instance_descriptor_set_layout_create_info.setBindings(instance_descriptor_set_layout_bindings);
	d_instance_descriptor_set_layout = vulkan.get_device().createDescriptorSetLayout(instance_descriptor_set_layout_create_info);

	// Map projection descriptor set layout.
	vk::DescriptorSetLayoutBinding map_projection_descriptor_set_layout_bindings[1];
	// Map projection image array binding.
	map_projection_descriptor_set_layout_bindings[0]
			.setBinding(0)
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			.setDescriptorCount(2)
			.setStageFlags(vk::ShaderStageFlagBits::eCompute);
	vk::DescriptorSetLayoutCreateInfo map_projection_descriptor_set_layout_create_info;
	map_projection_descriptor_set_layout_create_info.setBindings(map_projection_descriptor_set_layout_bindings);
	d_map_projection_descriptor_set_layout = vulkan.get_device().createDescriptorSetLayout(map_projection_descriptor_set_layout_create_info);

	// Descriptor set layouts.
	vk::DescriptorSetLayout descriptor_set_layouts[2] = { d_instance_descriptor_set_layout, d_map_projection_descriptor_set_layout };

	// Push constants.
	vk::PushConstantRange push_constant_range;
	push_constant_range
			.setStageFlags(vk::ShaderStageFlagBits::eCompute)
			.setOffset(0)
			.setSize(sizeof(ComputePushConstants));

	vk::PipelineLayoutCreateInfo pipeline_layout_create_info;
	pipeline_layout_create_info
			.setSetLayouts(descriptor_set_layouts)
			.setPushConstantRanges(push_constant_range);
	d_compute_pipeline_layout = vulkan.get_device().createPipelineLayout(pipeline_layout_create_info);

	//
	// Compute pipeline.
	//
	vk::ComputePipelineCreateInfo compute_pipeline_info;
	compute_pipeline_info
			.setStage(shader_stage_create_info)
			.setLayout(d_compute_pipeline_layout);
	d_compute_pipeline = vulkan.get_device().createComputePipeline({}, compute_pipeline_info).value;
}


void
GPlatesOpenGL::RenderedArrowRenderer::create_graphics_pipeline(
		Vulkan &vulkan,
		vk::RenderPass default_render_pass)
{
	//
	// Shader stages.
	//
	vk::PipelineShaderStageCreateInfo shader_stage_create_infos[2];
	// Vertex shader.
	const std::vector<std::uint32_t> vertex_shader_code = VulkanUtils::load_shader_code(":/arrows.vert.spv");
	vk::UniqueShaderModule vertex_shader_module = vulkan.get_device().createShaderModuleUnique({ {}, {vertex_shader_code} });
	shader_stage_create_infos[0].setStage(vk::ShaderStageFlagBits::eVertex).setModule(vertex_shader_module.get()).setPName("main");
	// Fragment shader.
	const std::vector<std::uint32_t> fragment_shader_code = VulkanUtils::load_shader_code(":/arrows.frag.spv");
	vk::UniqueShaderModule fragment_shader_module = vulkan.get_device().createShaderModuleUnique({ {}, {fragment_shader_code} });
	shader_stage_create_infos[1].setStage(vk::ShaderStageFlagBits::eFragment).setModule(fragment_shader_module.get()).setPName("main");

	//
	// Vertex input state.
	//
	vk::VertexInputBindingDescription vertex_binding_descriptions[2];
	// Per-vertex data.
	vertex_binding_descriptions[0].setBinding(0).setStride(sizeof(MeshVertex)).setInputRate(vk::VertexInputRate::eVertex);
	// Per-instance data.
	vertex_binding_descriptions[1].setBinding(1).setStride(sizeof(VisibleMeshInstance)).setInputRate(vk::VertexInputRate::eInstance);

	vk::VertexInputAttributeDescription vertex_attribute_descriptions[7];
	// Per-vertex attributes.
	vertex_attribute_descriptions[0].setLocation(0).setBinding(0).setFormat(vk::Format::eR32G32B32A32Sfloat)
			.setOffset(offsetof(MeshVertex, model_space_normalised_radial_position));
	vertex_attribute_descriptions[1].setLocation(1).setBinding(0).setFormat(vk::Format::eR32G32B32A32Sfloat)
			.setOffset(offsetof(MeshVertex, arrow_body_width_weight));
	// Per-instance attributes.
	vertex_attribute_descriptions[2].setLocation(2).setBinding(1).setFormat(vk::Format::eR32G32B32A32Sfloat)
			.setOffset(offsetof(VisibleMeshInstance, world_space_start_position));
	vertex_attribute_descriptions[3].setLocation(3).setBinding(1).setFormat(vk::Format::eR32G32B32A32Sfloat)
			.setOffset(offsetof(VisibleMeshInstance, world_space_x_axis));
	vertex_attribute_descriptions[4].setLocation(4).setBinding(1).setFormat(vk::Format::eR32G32B32A32Sfloat)
			.setOffset(offsetof(VisibleMeshInstance, world_space_y_axis));
	vertex_attribute_descriptions[5].setLocation(5).setBinding(1).setFormat(vk::Format::eR32G32B32A32Sfloat)
			.setOffset(offsetof(VisibleMeshInstance, world_space_z_axis));
	vertex_attribute_descriptions[6].setLocation(6).setBinding(1).setFormat(vk::Format::eR32G32B32A32Sfloat)
			.setOffset(offsetof(VisibleMeshInstance, colour));

	vk::PipelineVertexInputStateCreateInfo vertex_input_state_create_info;
	vertex_input_state_create_info
			.setVertexBindingDescriptions(vertex_binding_descriptions)
			.setVertexAttributeDescriptions(vertex_attribute_descriptions);

	//
	// Input assembly state.
	//
	vk::PipelineInputAssemblyStateCreateInfo input_assembly_state_create_info;
	input_assembly_state_create_info.setTopology(vk::PrimitiveTopology::eTriangleList);

	//
	// Viewport state.
	//
	// Using one dynamic viewport and one dynamic scissor rect.
	vk::PipelineViewportStateCreateInfo viewport_state_create_info;
	viewport_state_create_info.setViewportCount(1).setScissorCount(1);

	//
	// Rasterization state.
	//
	vk::PipelineRasterizationStateCreateInfo rasterization_state_create_info;
	rasterization_state_create_info
			.setPolygonMode(vk::PolygonMode::eFill)
			.setCullMode(vk::CullModeFlagBits::eBack)
			.setFrontFace(vk::FrontFace::eCounterClockwise);

	//
	// Multisample state.
	//
	vk::PipelineMultisampleStateCreateInfo multisample_state_create_info;  // default

	//
	// Depth stencil state.
	//
	vk::PipelineDepthStencilStateCreateInfo depth_stencil_state_create_info;
	depth_stencil_state_create_info
			.setDepthTestEnable(true)
			.setDepthWriteEnable(true)
			.setDepthCompareOp(vk::CompareOp::eLess);

	//
	// Colour blend state.
	//
	vk::PipelineColorBlendAttachmentState blend_attachment_state;
	blend_attachment_state
			.setBlendEnable(true)
			// RGB = A_src * RGB_src + (1-A_src) * RGB_dst ...
			.setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
			.setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
			.setColorBlendOp(vk::BlendOp::eAdd)
			//   A =     1 *   A_src + (1-A_src) *   A_dst ...
			.setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
			.setDstAlphaBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
			.setAlphaBlendOp(vk::BlendOp::eAdd)
			.setColorWriteMask(
					vk::ColorComponentFlagBits::eR |
					vk::ColorComponentFlagBits::eG |
					vk::ColorComponentFlagBits::eB |
					vk::ColorComponentFlagBits::eA);
	vk::PipelineColorBlendStateCreateInfo color_blend_state_create_info;
	color_blend_state_create_info.setAttachments(blend_attachment_state);

	//
	// Dynamic state.
	//
	// Using one dynamic viewport and one dynamic scissor rect.
	const vk::DynamicState dynamic_states[] = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
	vk::PipelineDynamicStateCreateInfo dynamic_state_create_info;
	dynamic_state_create_info.setDynamicStates(dynamic_states);

	//
	// Pipeline layout.
	//
	// We only use push constants (and no descriptor sets).
	vk::PushConstantRange push_constant_range;
	push_constant_range
			.setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
			.setOffset(0)
			.setSize(sizeof(GraphicsPushConstants));
	vk::PipelineLayoutCreateInfo pipeline_layout_create_info;
	pipeline_layout_create_info.setPushConstantRanges(push_constant_range);
	d_graphics_pipeline_layout = vulkan.get_device().createPipelineLayout(pipeline_layout_create_info);

	//
	// Graphics pipeline.
	//
	vk::GraphicsPipelineCreateInfo graphics_pipeline_info;
	graphics_pipeline_info
			.setStages(shader_stage_create_infos)
			.setPVertexInputState(&vertex_input_state_create_info)
			.setPInputAssemblyState(&input_assembly_state_create_info)
			.setPViewportState(&viewport_state_create_info)
			.setPRasterizationState(&rasterization_state_create_info)
			.setPMultisampleState(&multisample_state_create_info)
			.setPDepthStencilState(&depth_stencil_state_create_info)
			.setPColorBlendState(&color_blend_state_create_info)
			.setPDynamicState(&dynamic_state_create_info)
			.setLayout(d_graphics_pipeline_layout)
			.setRenderPass(default_render_pass);
	d_graphics_pipeline = vulkan.get_device().createGraphicsPipeline({}, graphics_pipeline_info).value;
}


void
GPlatesOpenGL::RenderedArrowRenderer::create_arrow_mesh(
		std::vector<MeshVertex> &vertices,
		std::vector<std::uint32_t> &vertex_indices)
{
	//
	// Create arrow mesh.
	//
	// NOTE: We orient front-facing triangles counter-clockwise (in Vulkan framebuffer space).
	//       Vulkan is opposite to OpenGL since Vulkan 'y' is top-down (OpenGL is bottom-up).
	//       This means the triangles look clockwise in OpenGL but are counter-clockwise in Vulkan.
	//

	// Unit circle.
	const double vertex_angle = 2 * GPlatesMaths::PI / NUM_VERTICES_IN_ARROW_CIRCULAR_CROSS_SECTION;
	float unit_circle[NUM_VERTICES_IN_ARROW_CIRCULAR_CROSS_SECTION][2];
	for (unsigned int n = 0; n < NUM_VERTICES_IN_ARROW_CIRCULAR_CROSS_SECTION; ++n)
	{
		unit_circle[n][0] = static_cast<float>(GPlatesMaths::cos(n * vertex_angle).dval());
		unit_circle[n][1] = static_cast<float>(GPlatesMaths::sin(n * vertex_angle).dval());
	}

	// Create the cap to close off the start of the arrow body cylinder.
	//
	// Add the triangle fan vertex at the centre of the start cap.
	vertices.push_back(
		MeshVertex{
			{ 0, 0 },  // vertex on arrow axis
			0, -1,  // surface normal along -z
			0, 0, 0, 0  // vertex on arrow axis at start of arrow
		});
	// Add the remaining triangle fan vertices of the start cap.
	for (unsigned int n = 0; n < NUM_VERTICES_IN_ARROW_CIRCULAR_CROSS_SECTION; ++n)
	{
		vertices.push_back(
			MeshVertex{
				{ unit_circle[n][0], unit_circle[n][1] },
				0, -1,  // surface normal along -z
				1, 0, 0, 0  // vertex on arrow body at start of arrow
			});
	}
	// Add triangle fan vertex indices.
	for (unsigned int n = 0; n < NUM_VERTICES_IN_ARROW_CIRCULAR_CROSS_SECTION; ++n)
	{
		vertex_indices.push_back(0);  // fan centre
		vertex_indices.push_back(1/*skip fan centre*/ + ((n + 1) % NUM_VERTICES_IN_ARROW_CIRCULAR_CROSS_SECTION)/*circle wraparound*/);
		vertex_indices.push_back(1/*skip fan centre*/ + n);
	}

	// Create the arrow body cylinder.
	//
	// Add the cylinder start vertices.
	const std::uint32_t start_cylinder_vertex_offset = vertices.size();
	for (unsigned int n = 0; n < NUM_VERTICES_IN_ARROW_CIRCULAR_CROSS_SECTION; ++n)
	{
		vertices.push_back(
			MeshVertex{
				{ unit_circle[n][0], unit_circle[n][1] },
				1, 0,  // surface normal is radially outward
				1, 0, 0, 0  // vertex on arrow body at start of arrow
			});
	}
	// Add the cylinder end vertices.
	const std::uint32_t end_cylinder_vertex_offset = vertices.size();
	for (unsigned int n = 0; n < NUM_VERTICES_IN_ARROW_CIRCULAR_CROSS_SECTION; ++n)
	{
		vertices.push_back(
			MeshVertex{
				{ unit_circle[n][0], unit_circle[n][1] },
				1, 0,  // surface normal is radially outward
				1, 0, 1, 0  // vertex on arrow body at end of arrow body (start of arrowhead)
			});
	}
	// Add the cylinder vertex indices.
	for (unsigned int n = 0; n < NUM_VERTICES_IN_ARROW_CIRCULAR_CROSS_SECTION; ++n)
	{
		// First triangle of current quad.
		vertex_indices.push_back(start_cylinder_vertex_offset + n);
		vertex_indices.push_back(start_cylinder_vertex_offset + ((n + 1) % NUM_VERTICES_IN_ARROW_CIRCULAR_CROSS_SECTION)/*start circle wraparound*/);
		vertex_indices.push_back(end_cylinder_vertex_offset + n);
		// Second triangle of current quad.
		vertex_indices.push_back(end_cylinder_vertex_offset + ((n + 1) % NUM_VERTICES_IN_ARROW_CIRCULAR_CROSS_SECTION)/*end circle wraparound*/);
		vertex_indices.push_back(end_cylinder_vertex_offset + n);
		vertex_indices.push_back(start_cylinder_vertex_offset + ((n + 1) % NUM_VERTICES_IN_ARROW_CIRCULAR_CROSS_SECTION)/*start circle wraparound*/);
	}

	// Create the arrow head cone annulus (the flat part of cone connecting to the arrow body).
	//
	// Add the annular inner vertices (end circle of body cylinder).
	const std::uint32_t annulus_inner_vertex_offset = vertices.size();
	for (unsigned int n = 0; n < NUM_VERTICES_IN_ARROW_CIRCULAR_CROSS_SECTION; ++n)
	{
		vertices.push_back(
			MeshVertex{
				{ unit_circle[n][0], unit_circle[n][1] },
				0, -1,  // surface normal along -z
				1, 0, 1, 0  // vertex on arrow body at end of arrow body (start of arrowhead)
			});
	}
	// Add the annular outer vertices (widest part of arrow head).
	const std::uint32_t annulus_outer_vertex_offset = vertices.size();
	for (unsigned int n = 0; n < NUM_VERTICES_IN_ARROW_CIRCULAR_CROSS_SECTION; ++n)
	{
		vertices.push_back(
			MeshVertex{
				{ unit_circle[n][0], unit_circle[n][1] },
				0, -1,  // surface normal along -z
				0, 1, 1, 0  // vertex on arrowhead at start of arrowhead (end of arrow body)
			});
	}
	// Add the cone vertex indices.
	for (unsigned int n = 0; n < NUM_VERTICES_IN_ARROW_CIRCULAR_CROSS_SECTION; ++n)
	{
		// First triangle of current quad.
		vertex_indices.push_back(annulus_inner_vertex_offset + n);
		vertex_indices.push_back(annulus_inner_vertex_offset + ((n + 1) % NUM_VERTICES_IN_ARROW_CIRCULAR_CROSS_SECTION)/*start circle wraparound*/);
		vertex_indices.push_back(annulus_outer_vertex_offset + n);
		// Second triangle of current quad.
		vertex_indices.push_back(annulus_outer_vertex_offset + ((n + 1) % NUM_VERTICES_IN_ARROW_CIRCULAR_CROSS_SECTION)/*end circle wraparound*/);
		vertex_indices.push_back(annulus_outer_vertex_offset + n);
		vertex_indices.push_back(annulus_inner_vertex_offset + ((n + 1) % NUM_VERTICES_IN_ARROW_CIRCULAR_CROSS_SECTION)/*start circle wraparound*/);
	}

	// Create the curved surface of the arrow head cone.
	//
	const float cone_radial_normal_weight = static_cast<float>(std::cos(std::atan(ARROWHEAD_WIDTH_TO_LENGTH_RATIO)));
	const float cone_axial_normal_weight = static_cast<float>(std::sin(std::atan(ARROWHEAD_WIDTH_TO_LENGTH_RATIO)));
	// Add the cone apex vertex (at the pointy end of the arrow).
	const std::uint32_t cone_vertex_offset = vertices.size();
	vertices.push_back(
		MeshVertex{
			{ 0, 0 },  // vertex on arrow axis
			cone_radial_normal_weight, cone_axial_normal_weight,  // surface normal is perpendicular to cone curved surface
			0, 0, 1, 1  // vertex on arrow axis at end of arrow (pointy apex)
		});
	// Add the remaining triangle fan vertices of the cone.
	for (unsigned int n = 0; n < NUM_VERTICES_IN_ARROW_CIRCULAR_CROSS_SECTION; ++n)
	{
		vertices.push_back(
			MeshVertex{
				{ unit_circle[n][0], unit_circle[n][1] },
				cone_radial_normal_weight, cone_axial_normal_weight,  // surface normal is perpendicular to cone curved surface
				0, 1, 1, 0  // vertex on arrowhead at start of arrowhead (end of arrow body)
			});
	}
	// Add triangle fan vertex indices.
	for (unsigned int n = 0; n < NUM_VERTICES_IN_ARROW_CIRCULAR_CROSS_SECTION; ++n)
	{
		vertex_indices.push_back(cone_vertex_offset);  // fan centre
		vertex_indices.push_back(cone_vertex_offset + 1/*skip fan centre*/ + n);
		vertex_indices.push_back(cone_vertex_offset + 1/*skip fan centre*/ + ((n + 1) % NUM_VERTICES_IN_ARROW_CIRCULAR_CROSS_SECTION)/*circle wraparound*/);
	}

	d_num_vertex_indices = vertex_indices.size();
}


void
GPlatesOpenGL::RenderedArrowRenderer::load_arrow_mesh(
		Vulkan &vulkan,
		vk::CommandBuffer initialisation_command_buffer,
		vk::Fence initialisation_submit_fence,
		const std::vector<MeshVertex> &vertices,
		const std::vector<std::uint32_t> &vertex_indices)
{
	//
	// Vertex/index buffers.
	//

	// Vulkan memory allocator.
	VmaAllocator vma_allocator = vulkan.get_vma_allocator();

	// Common buffer parameters.
	vk::BufferCreateInfo buffer_create_info;
	buffer_create_info.setSharingMode(vk::SharingMode::eExclusive);

	// Common allocation parameters.
	VmaAllocationCreateInfo allocation_create_info = {};

	//
	// Create staging and final vertex buffers.
	//
	buffer_create_info.setSize(vertices.size() * sizeof(MeshVertex));

	// Staging vertex buffer (in mappable host memory).
	buffer_create_info.setUsage(vk::BufferUsageFlagBits::eTransferSrc);
	allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;
	allocation_create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;  // host mappable
	VulkanBuffer staging_vertex_buffer = VulkanBuffer::create(vma_allocator, buffer_create_info, allocation_create_info, GPLATES_EXCEPTION_SOURCE);

	// Final vertex buffer (in device local memory).
	buffer_create_info.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer);
	allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;
	allocation_create_info.flags = 0;  // device local memory
	d_vertex_buffer = VulkanBuffer::create(vma_allocator, buffer_create_info, allocation_create_info, GPLATES_EXCEPTION_SOURCE);

	//
	// Create staging and final index buffers.
	//
	buffer_create_info.setSize(vertex_indices.size() * sizeof(std::uint32_t));

	// Staging index buffer (in mappable host memory).
	buffer_create_info.setUsage(vk::BufferUsageFlagBits::eTransferSrc);
	allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;
	allocation_create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;  // host mappable
	VulkanBuffer staging_index_buffer = VulkanBuffer::create(vma_allocator, buffer_create_info, allocation_create_info, GPLATES_EXCEPTION_SOURCE);

	// Final index buffer (in device local memory).
	buffer_create_info.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer);
	allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;
	allocation_create_info.flags = 0;  // device local memory
	d_index_buffer = VulkanBuffer::create(vma_allocator, buffer_create_info, allocation_create_info, GPLATES_EXCEPTION_SOURCE);

	//
	// Copy the vertices into the mapped staging vertex buffer.
	//
	void *staging_vertex_buffer_data = staging_vertex_buffer.map_memory(vma_allocator, GPLATES_EXCEPTION_SOURCE);
	std::memcpy(staging_vertex_buffer_data, vertices.data(), vertices.size() * sizeof(MeshVertex));
	staging_vertex_buffer.flush_mapped_memory(vma_allocator, 0, VK_WHOLE_SIZE, GPLATES_EXCEPTION_SOURCE);
	staging_vertex_buffer.unmap_memory(vma_allocator);

	//
	// Copy the vertex indices into the mapped staging index buffer.
	//
	void *staging_index_buffer_data = staging_index_buffer.map_memory(vma_allocator, GPLATES_EXCEPTION_SOURCE);
	std::memcpy(staging_index_buffer_data, vertex_indices.data(), vertex_indices.size() * sizeof(std::uint32_t));
	staging_index_buffer.flush_mapped_memory(vma_allocator, 0, VK_WHOLE_SIZE, GPLATES_EXCEPTION_SOURCE);
	staging_index_buffer.unmap_memory(vma_allocator);


	//
	// Record initialisation command buffer.
	//

	// Begin recording into the initialisation command buffer.
	vk::CommandBufferBeginInfo initialisation_command_buffer_begin_info;
	// Command buffer will only be submitted once.
	initialisation_command_buffer_begin_info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	initialisation_command_buffer.begin(initialisation_command_buffer_begin_info);

	//
	// Copy vertices from staging host buffer to final device buffer.
	//
	vk::BufferCopy vertex_buffer_copy;
	vertex_buffer_copy.setSrcOffset(0).setDstOffset(0).setSize(vertices.size() * sizeof(MeshVertex));
	initialisation_command_buffer.copyBuffer(
			staging_vertex_buffer.get_buffer(),
			d_vertex_buffer.get_buffer(),
			vertex_buffer_copy);

	//
	// Copy vertex indices from staging host buffer to final device buffer.
	//
	vk::BufferCopy index_buffer_copy;
	index_buffer_copy.setSrcOffset(0).setDstOffset(0).setSize(vertex_indices.size() * sizeof(std::uint32_t));
	initialisation_command_buffer.copyBuffer(
			staging_index_buffer.get_buffer(),
			d_index_buffer.get_buffer(),
			index_buffer_copy);

#if defined(_WIN32) && defined(MemoryBarrier) // See "VulkanHpp.h" for why this is necessary.
#	undef MemoryBarrier
#endif
	// Pipeline barrier to wait for staging transfer writes to be made visible for use as vertex/index data.
	vk::MemoryBarrier memory_barrier;
	memory_barrier
			.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
			.setDstAccessMask(vk::AccessFlagBits::eVertexAttributeRead | vk::AccessFlagBits::eIndexRead);
	initialisation_command_buffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eVertexInput,
			{},  // dependencyFlags
			memory_barrier,  // memoryBarriers
			{},  // bufferMemoryBarriers
			{});  // imageMemoryBarriers

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
		throw VulkanException(GPLATES_EXCEPTION_SOURCE, "Error waiting for initialisation of arrows.");
	}
	vulkan.get_device().resetFences(initialisation_submit_fence);

	// Destroy staging buffers now that device is no longer using them.
	VulkanBuffer::destroy(vma_allocator, staging_vertex_buffer);
	VulkanBuffer::destroy(vma_allocator, staging_index_buffer);
}


void
GPlatesOpenGL::RenderedArrowRenderer::create_map_projection_descriptor_set(
		Vulkan &vulkan,
		const MapProjectionImage &map_projection_image)
{
	// Create descriptor pool.
	vk::DescriptorPoolSize descriptor_pool_size;
	descriptor_pool_size
			.setType(vk::DescriptorType::eCombinedImageSampler)
			.setDescriptorCount(2);
	vk::DescriptorPoolCreateInfo descriptor_pool_create_info;
	descriptor_pool_create_info
			.setMaxSets(1)
			.setPoolSizes(descriptor_pool_size);
	d_map_projection_descriptor_pool = vulkan.get_device().createDescriptorPool(descriptor_pool_create_info);

	// Allocate descriptor set.
	vk::DescriptorSetAllocateInfo descriptor_set_allocate_info;
	descriptor_set_allocate_info
			.setDescriptorPool(d_map_projection_descriptor_pool)
			.setSetLayouts(d_map_projection_descriptor_set_layout);
	const std::vector<vk::DescriptorSet> descriptor_sets =
			vulkan.get_device().allocateDescriptorSets(descriptor_set_allocate_info);
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			descriptor_sets.size() == 1,
			GPLATES_ASSERTION_SOURCE);
	d_map_projection_descriptor_set = descriptor_sets[0];

	// Descriptor writes for the map projection textures.
	vk::DescriptorImageInfo descriptor_image_infos[2] =
	{
		map_projection_image.get_forward_transform_descriptor_image_info(),
		map_projection_image.get_jacobian_matrix_descriptor_image_info()
	};
	vk::WriteDescriptorSet descriptor_writes[1];
	descriptor_writes[0]
			.setDstSet(d_map_projection_descriptor_set).setDstBinding(0).setDstArrayElement(0).setDescriptorCount(2)
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			.setImageInfo(descriptor_image_infos);

	// Update descriptor set.
	vulkan.get_device().updateDescriptorSets(descriptor_writes, nullptr/*descriptorCopies*/);
}


GPlatesOpenGL::RenderedArrowRenderer::InstanceResource
GPlatesOpenGL::RenderedArrowRenderer::create_instance_resource(
		Vulkan &vulkan)
{
	// Vulkan memory allocator.
	VmaAllocator vma_allocator = vulkan.get_vma_allocator();

	// Buffer parameters.
	vk::BufferCreateInfo buffer_create_info;
	buffer_create_info.setSharingMode(vk::SharingMode::eExclusive);

	// Allocation parameters.
	VmaAllocationCreateInfo allocation_create_info = {};

	//
	// Instances buffer.
	//

	buffer_create_info.setSize(NUM_ARROWS_PER_INSTANCE_BUFFER * sizeof(MeshInstance));
	// Buffer is read by compute shader (as a storage buffer).
	buffer_create_info.setUsage(vk::BufferUsageFlagBits::eStorageBuffer);

	// Instance buffer should be mappable (in host memory) and preferably also in device local memory.
	allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;  // prefer device local
	allocation_create_info.flags =
			VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | // host mappable
			VMA_ALLOCATION_CREATE_MAPPED_BIT; // persistently mapped

	// Create instance buffer.
	VulkanBuffer instance_buffer = VulkanBuffer::create(vma_allocator, buffer_create_info, allocation_create_info, GPLATES_EXCEPTION_SOURCE);

	// Get persistently-mapped pointer to start of instance buffer.
	VmaAllocationInfo allocation_info;
	vmaGetAllocationInfo(vma_allocator, instance_buffer.get_allocation(), &allocation_info);
	void *instance_buffer_mapped_pointer = allocation_info.pMappedData;

	//
	// Visible instances buffer.
	//

	buffer_create_info.setSize(NUM_ARROWS_PER_INSTANCE_BUFFER * sizeof(VisibleMeshInstance));
	// Buffer is written (by compute shader) as a storage buffer and read as a vertex buffer.
	buffer_create_info.setUsage(vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eVertexBuffer);

	allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;
	allocation_create_info.flags = 0;  // device local memory

	// Create visible instance buffer.
	VulkanBuffer visible_instance_buffer = VulkanBuffer::create(vma_allocator, buffer_create_info, allocation_create_info, GPLATES_EXCEPTION_SOURCE);

	//
	// Indirect draw buffer.
	//

	buffer_create_info.setSize(sizeof(vk::DrawIndexedIndirectCommand));
	// Buffer is destination of a copy, and written (by compute shader) as a storage buffer, and read as an indirect buffer.
	buffer_create_info.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndirectBuffer);

	allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;
	allocation_create_info.flags = 0;  // device local memory

	// Create indirect draw buffer.
	VulkanBuffer indirect_draw_buffer = VulkanBuffer::create(vma_allocator, buffer_create_info, allocation_create_info, GPLATES_EXCEPTION_SOURCE);

	//
	// Descriptor set.
	//

	// Create descriptor pool.
	vk::DescriptorPoolSize compute_descriptor_pool_size;
	compute_descriptor_pool_size
			.setType(vk::DescriptorType::eStorageBuffer)
			.setDescriptorCount(3);  // only use 3 storage buffers in compute shader
	vk::DescriptorPoolCreateInfo compute_descriptor_pool_create_info;
	compute_descriptor_pool_create_info
			.setMaxSets(1)
			.setPoolSizes(compute_descriptor_pool_size);
	vk::DescriptorPool compute_descriptor_pool = vulkan.get_device().createDescriptorPool(compute_descriptor_pool_create_info);

	// Allocate descriptor set.
	vk::DescriptorSetAllocateInfo compute_descriptor_set_allocate_info;
	compute_descriptor_set_allocate_info
			.setDescriptorPool(compute_descriptor_pool)
			.setSetLayouts(d_instance_descriptor_set_layout);
	const std::vector<vk::DescriptorSet> compute_descriptor_sets =
			vulkan.get_device().allocateDescriptorSets(compute_descriptor_set_allocate_info);
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			compute_descriptor_sets.size() == 1,
			GPLATES_ASSERTION_SOURCE);
	vk::DescriptorSet compute_descriptor_set = compute_descriptor_sets[0];

	vk::WriteDescriptorSet compute_descriptor_writes[3];
	// Descriptor write for instance buffer.
	vk::DescriptorBufferInfo instance_descriptor_buffer_info;
	instance_descriptor_buffer_info.setBuffer(instance_buffer.get_buffer()).setOffset(vk::DeviceSize(0)).setRange(VK_WHOLE_SIZE);
	compute_descriptor_writes[0]
			.setDstSet(compute_descriptor_set).setDstBinding(0).setDstArrayElement(0).setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eStorageBuffer)
			.setBufferInfo(instance_descriptor_buffer_info);
	// Descriptor write for visible instance buffer.
	vk::DescriptorBufferInfo visible_instance_descriptor_buffer_info;
	visible_instance_descriptor_buffer_info.setBuffer(visible_instance_buffer.get_buffer()).setOffset(vk::DeviceSize(0)).setRange(VK_WHOLE_SIZE);
	compute_descriptor_writes[1]
			.setDstSet(compute_descriptor_set).setDstBinding(1).setDstArrayElement(0).setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eStorageBuffer)
			.setBufferInfo(visible_instance_descriptor_buffer_info);
	// Descriptor write for indirect draw buffer.
	vk::DescriptorBufferInfo indirect_draw_descriptor_buffer_info;
	indirect_draw_descriptor_buffer_info.setBuffer(indirect_draw_buffer.get_buffer()).setOffset(vk::DeviceSize(0)).setRange(VK_WHOLE_SIZE);
	compute_descriptor_writes[2]
			.setDstSet(compute_descriptor_set).setDstBinding(2).setDstArrayElement(0).setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eStorageBuffer)
			.setBufferInfo(indirect_draw_descriptor_buffer_info);

	// Update descriptor set.
	vulkan.get_device().updateDescriptorSets(compute_descriptor_writes, nullptr/*descriptorCopies*/);

	return { instance_buffer, instance_buffer_mapped_pointer, visible_instance_buffer, indirect_draw_buffer, compute_descriptor_pool, compute_descriptor_set };
}
