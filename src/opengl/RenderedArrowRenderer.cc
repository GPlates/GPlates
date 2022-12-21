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
#include <cstring>
#include <vector>

#include "RenderedArrowRenderer.h"

#include "GLMatrix.h"
#include "GLViewProjection.h"
#include "VulkanException.h"
#include "VulkanUtils.h"

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
		vk::CommandBuffer initialisation_command_buffer,
		vk::Fence initialisation_submit_fence)
{
	// Add this scope to the call stack trace printed if exception thrown in this scope.
	TRACK_CALL_STACK();

	// Create the graphics pipeline.
	create_graphics_pipeline(vulkan, default_render_pass);

	// Create the arrow mesh and load it into the vertex/index buffers.
	std::vector<MeshVertex> vertices;
	std::vector<std::uint32_t> vertex_indices;
	create_arrow_mesh(vertices, vertex_indices);
	load_arrow_mesh(vulkan, initialisation_command_buffer, initialisation_submit_fence, vertices, vertex_indices);
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
	for (unsigned int instance = 0; instance < Vulkan::NUM_ASYNC_FRAMES; ++instance)
	{
		VulkanBuffer::destroy(vma_allocator, d_instance_buffers[instance]);
		d_instance_buffer_mapped_pointers[instance] = nullptr;
	}

	d_num_vertex_indices = 0;
	d_num_arrows_to_render = 0;

	// Destroy the pipeline layout and graphics pipeline.
	vulkan.get_device().destroyPipelineLayout(d_pipeline_layout);
	vulkan.get_device().destroyPipeline(d_graphics_pipeline);
}


void
GPlatesOpenGL::RenderedArrowRenderer::add(
		Vulkan &vulkan,
		const GPlatesMaths::Vector3D &arrow_start,
		const GPlatesMaths::Vector3D &arrow_vector,
		float arrow_body_width,
		float arrowhead_length,
		const GPlatesGui::Colour &arrow_colour,
		const double &inverse_viewport_zoom_factor)
{
	// Calculate position from start point along vector direction to end point.
	// The length of the arrow in world space is inversely proportional to the zoom (or magnification).
	const GPlatesMaths::Vector3D arrow_end = arrow_start + inverse_viewport_zoom_factor * arrow_vector;

	const GPlatesMaths::Vector3D arrow_body_vector = arrow_end - arrow_start;
	const GPlatesMaths::real_t arrow_body_length = arrow_body_vector.magnitude();

	// Avoid divide-by-zero - and if arrow length is near zero it won't be visible.
	if (arrow_body_length == 0)
	{
		return;
	}
	const GPlatesMaths::UnitVector3D arrow_unit_vector((1.0 / arrow_body_length) * arrow_body_vector);

	// Also scale the arrow body width and arrowhead length according to the zoom.
	arrow_body_width *= inverse_viewport_zoom_factor;
	arrowhead_length *= inverse_viewport_zoom_factor;

	// We want to keep the projected arrowhead size constant regardless of the
	// the length of the arrow body, except...
	//
	// ...if the ratio of arrowhead size to arrow body length is large enough then
	// we need to start scaling the arrowhead size by the arrow body length so
	// that the arrowhead disappears as the arrow body disappears.
	const float max_arrowhead_size = MAX_RATIO_ARROWHEAD_TO_ARROW_BODY_LENGTH * arrow_body_length.dval();
	if (arrowhead_length > max_arrowhead_size)
	{
		const float arrow_scale = max_arrowhead_size / arrowhead_length;

		arrowhead_length *= arrow_scale;

		// Also linearly shrink the arrow body width.
		arrow_body_width *= arrow_scale;
	}
	const float arrowhead_width = ARROWHEAD_WIDTH_TO_LENGTH_RATIO * arrowhead_length;

	// Find an orthonormal basis using 'arrow_unit_vector'.
	const GPlatesMaths::UnitVector3D &arrow_z_axis = arrow_unit_vector;
	const GPlatesMaths::UnitVector3D arrow_y_axis = generate_perpendicular(arrow_z_axis);
	const GPlatesMaths::UnitVector3D arrow_x_axis( cross(arrow_y_axis, arrow_z_axis) );

	MeshInstance arrow_instance;
	// Copy arrow start position into instance data.
	arrow_instance.world_space_start_position[0] = arrow_start.x().dval();
	arrow_instance.world_space_start_position[1] = arrow_start.y().dval();
	arrow_instance.world_space_start_position[2] = arrow_start.z().dval();
	// Copy arrow position weights into instance data.
	arrow_instance.arrow_body_width = arrow_body_width;
	arrow_instance.arrowhead_width = arrowhead_width;
	arrow_instance.arrow_body_length = arrow_body_length.dval();
	arrow_instance.arrowhead_length = arrowhead_length;
	// Copy orthonormal basis into instance data.
	arrow_instance.world_space_x_axis[0] = arrow_x_axis.x().dval();
	arrow_instance.world_space_x_axis[1] = arrow_x_axis.y().dval();
	arrow_instance.world_space_x_axis[2] = arrow_x_axis.z().dval();
	arrow_instance.world_space_y_axis[0] = arrow_y_axis.x().dval();
	arrow_instance.world_space_y_axis[1] = arrow_y_axis.y().dval();
	arrow_instance.world_space_y_axis[2] = arrow_y_axis.z().dval();
	arrow_instance.world_space_z_axis[0] = arrow_z_axis.x().dval();
	arrow_instance.world_space_z_axis[1] = arrow_z_axis.y().dval();
	arrow_instance.world_space_z_axis[2] = arrow_z_axis.z().dval();
	// Copy arrow colour into instance data.
	arrow_instance.colour[0] = arrow_colour.red();
	arrow_instance.colour[1] = arrow_colour.blue();
	arrow_instance.colour[2] = arrow_colour.green();
	arrow_instance.colour[3] = arrow_colour.alpha();

	// Copy the arrow instance data into the persistently-mapped instance buffer.
	void *arrow_instance_buffer_mapped_pointer = d_instance_buffer_mapped_pointers[vulkan.get_frame_index()];
	const std::uint32_t arrow_instance_index = d_num_arrows_to_render++;
	std::memcpy(
			// Pointer to current arrow instance within the buffer...
			static_cast<std::uint8_t *>(arrow_instance_buffer_mapped_pointer) + arrow_instance_index * sizeof(MeshInstance),
			&arrow_instance,
			sizeof(MeshInstance));
}


void
GPlatesOpenGL::RenderedArrowRenderer::render(
		Vulkan &vulkan,
		vk::CommandBuffer command_buffer,
		const GLViewProjection &view_projection,
		bool is_globe_active)
{
	// Return early if no arrows to render.
	if (d_num_arrows_to_render == 0)
	{
		return;
	}

	// Bind the graphics pipeline.
	command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, d_graphics_pipeline);

	// Set viewport and scissor rects.
	command_buffer.setViewport(0, view_projection.get_viewport().get_vulkan_viewport());
	command_buffer.setScissor(0, view_projection.get_viewport().get_vulkan_rect_2D());

	// Bind the vertex buffers.
	vk::Buffer vertex_buffers[2] =
	{
		d_vertex_buffer.get_buffer(),
		d_instance_buffers[vulkan.get_frame_index()].get_buffer()
	};
	vk::DeviceSize vertex_buffer_offsets[2] = { 0, 0 };
	command_buffer.bindVertexBuffers(0, vertex_buffers, vertex_buffer_offsets);

	// Bind the index buffer.
	command_buffer.bindIndexBuffer(d_index_buffer.get_buffer(), vk::DeviceSize(0), vk::IndexType::eUint32);

	//
	// Push constants for arrows.
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
	PushConstants push_constants = {};

	// Convert clip space from OpenGL to Vulkan and pre-multiply projection transform.
	GLMatrix vulkan_view_projection = VulkanUtils::from_opengl_clip_space();
	vulkan_view_projection.gl_mult_matrix(view_projection.get_view_projection_transform());
	// Set view projection matrix.
	vulkan_view_projection.get_float_matrix(push_constants.view_projection);

	// Is lighting enabled for arrows?
	push_constants.lighting_enabled = d_scene_lighting_parameters.is_lighting_enabled(
			GPlatesGui::SceneLightingParameters::LIGHTING_DIRECTION_ARROW);
	if (push_constants.lighting_enabled)
	{
		// Light direction.
		push_constants.world_space_light_direction[0] = d_scene_lighting_parameters.get_globe_view_light_direction().x().dval();
		push_constants.world_space_light_direction[1] = d_scene_lighting_parameters.get_globe_view_light_direction().y().dval();
		push_constants.world_space_light_direction[2] = d_scene_lighting_parameters.get_globe_view_light_direction().z().dval();
		// Ambient light contribution.
		push_constants.light_ambient_contribution = d_scene_lighting_parameters.get_ambient_light_contribution();
	}

	// Set the push constants.
	command_buffer.pushConstants(
			d_pipeline_layout,
			vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
			// Update all push constants...
			0, sizeof(PushConstants), &push_constants);

	// Draw the arrows.
	//
	// There's a single arrow mesh that gets instanced by the number of arrows.
	// Each instance supplies an arrow position and direction in world space, a body/head width/length and a colour.
	command_buffer.drawIndexed(d_num_vertex_indices, d_num_arrows_to_render, 0, 0, 0);

	// Reset the number of arrows to render.
	d_num_arrows_to_render = 0;
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
	vertex_binding_descriptions[1].setBinding(1).setStride(sizeof(MeshInstance)).setInputRate(vk::VertexInputRate::eInstance);

	vk::VertexInputAttributeDescription vertex_attribute_descriptions[8];
	// Per-vertex attributes.
	vertex_attribute_descriptions[0].setLocation(0).setBinding(0).setFormat(vk::Format::eR32G32B32A32Sfloat)
			.setOffset(offsetof(MeshVertex, model_space_normalised_radial_position));
	vertex_attribute_descriptions[1].setLocation(1).setBinding(0).setFormat(vk::Format::eR32G32B32A32Sfloat)
			.setOffset(offsetof(MeshVertex, arrow_body_width_weight));
	// Per-instance attributes.
	vertex_attribute_descriptions[2].setLocation(2).setBinding(1).setFormat(vk::Format::eR32G32B32Sfloat)
			.setOffset(offsetof(MeshInstance, world_space_start_position));
	vertex_attribute_descriptions[3].setLocation(3).setBinding(1).setFormat(vk::Format::eR32G32B32Sfloat)
			.setOffset(offsetof(MeshInstance, world_space_x_axis));
	vertex_attribute_descriptions[4].setLocation(4).setBinding(1).setFormat(vk::Format::eR32G32B32Sfloat)
			.setOffset(offsetof(MeshInstance, world_space_y_axis));
	vertex_attribute_descriptions[5].setLocation(5).setBinding(1).setFormat(vk::Format::eR32G32B32Sfloat)
			.setOffset(offsetof(MeshInstance, world_space_z_axis));
	vertex_attribute_descriptions[6].setLocation(6).setBinding(1).setFormat(vk::Format::eR32G32B32A32Sfloat)
			.setOffset(offsetof(MeshInstance, arrow_body_width));
	vertex_attribute_descriptions[7].setLocation(7).setBinding(1).setFormat(vk::Format::eR32G32B32A32Sfloat)
			.setOffset(offsetof(MeshInstance, colour));

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
			.setSize(sizeof(PushConstants));
	vk::PipelineLayoutCreateInfo pipeline_layout_create_info;
	pipeline_layout_create_info.setPushConstantRanges(push_constant_range);
	d_pipeline_layout = vulkan.get_device().createPipelineLayout(pipeline_layout_create_info);

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
			.setLayout(d_pipeline_layout)
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
	// Instance/vertex/index buffers.
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
	// Create dynamic instance buffers.
	//
	// These will be written to every frame (in alternatively fashion, for double-buffered asynchronous frame rendering).
	//
	buffer_create_info.setSize(NUM_ARROWS_PER_INSTANCE_BUFFER * sizeof(MeshInstance));
	buffer_create_info.setUsage(vk::BufferUsageFlagBits::eVertexBuffer);
	// Instance buffer should be mappable (in host memory) and preferably also in device local memory.
	allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;  // prefer device local
	allocation_create_info.flags =
			VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | // host mappable
			VMA_ALLOCATION_CREATE_MAPPED_BIT; // persistently mapped

	// Create a buffer for each asynchronous frame.
	for (unsigned int instance = 0; instance < Vulkan::NUM_ASYNC_FRAMES; ++instance)
	{
		// Create instance buffer.
		d_instance_buffers[instance] = VulkanBuffer::create(vma_allocator, buffer_create_info, allocation_create_info, GPLATES_EXCEPTION_SOURCE);

		// Get persistently-mapped pointer to start of instance buffer.
		VmaAllocationInfo allocation_info;
		vmaGetAllocationInfo(vma_allocator, d_instance_buffers[instance].get_allocation(), &allocation_info);
		d_instance_buffer_mapped_pointers[instance] = allocation_info.pMappedData;
	}


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
	// Pipeline barrier to wait for staging transfer writes to be made available before using the vertex/index data.
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
