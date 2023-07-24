/**
 * Copyright (C) 2010 The University of Sydney, Australia
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
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <QString>

#include "Stars.h"

#include "GLMatrix.h"
#include "GLViewProjection.h"
#include "VulkanException.h"
#include "VulkanUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "gui/Colour.h"

#include "presentation/ViewState.h"

#include "utils/CallStackTracker.h"


const GPlatesGui::Colour GPlatesOpenGL::Stars::DEFAULT_COLOUR(0.75f, 0.75f, 0.75f);


GPlatesOpenGL::Stars::Stars(
		const GPlatesGui::Colour &colour) :
	d_colour(colour)
{
}


void
GPlatesOpenGL::Stars::initialise_vulkan_resources(
		Vulkan &vulkan,
		vk::RenderPass default_render_pass,
		vk::SampleCountFlagBits default_render_pass_sample_count,
		vk::CommandBuffer initialisation_command_buffer,
		vk::Fence initialisation_submit_fence)
{
	// Add this scope to the call stack trace printed if exception thrown in this scope.
	TRACK_CALL_STACK();

	// Create the graphics pipeline.
	create_graphics_pipeline(vulkan, default_render_pass, default_render_pass_sample_count);

	// Create the stars and load them into the vertex/index buffers.
	std::vector<vertex_type> vertices;
	std::vector<vertex_index_type> vertex_indices;
	create_stars(vertices, vertex_indices);
	load_stars(vulkan, initialisation_command_buffer, initialisation_submit_fence, vertices, vertex_indices);
}

void
GPlatesOpenGL::Stars::release_vulkan_resources(
		Vulkan &vulkan)
{
	// Vulkan memory allocator.
	VmaAllocator vma_allocator = vulkan.get_vma_allocator();

	// Destroy the vertex/index buffers.
	VulkanBuffer::destroy(vma_allocator, d_vertex_buffer);
	VulkanBuffer::destroy(vma_allocator, d_index_buffer);

	// Destroy the pipeline layout and graphics pipeline.
	vulkan.get_device().destroyPipelineLayout(d_pipeline_layout);
	vulkan.get_device().destroyPipeline(d_graphics_pipeline);
}


void
GPlatesOpenGL::Stars::render(
		Vulkan &vulkan,
		vk::CommandBuffer default_render_pass_command_buffer,
		const GLViewProjection &view_projection,
		int device_pixel_ratio,
		const double &radius_multiplier)
{
	// Bind the graphics pipeline.
	default_render_pass_command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, d_graphics_pipeline);

	// Set viewport and scissor rects.
	default_render_pass_command_buffer.setViewport(0, view_projection.get_viewport().get_vulkan_viewport());
	default_render_pass_command_buffer.setScissor(0, view_projection.get_viewport().get_vulkan_rect_2D());

	// Bind vertex/index buffers.
	default_render_pass_command_buffer.bindVertexBuffers(0, d_vertex_buffer.get_buffer(), vk::DeviceSize(0));
	default_render_pass_command_buffer.bindIndexBuffer(d_index_buffer.get_buffer(), vk::DeviceSize(0), vk::IndexType::eUint32);

	//
	// layout (push_constant) uniform PushConstants
	// {
	//     mat4 view_projection;
	//     vec4 star_colour;
	//     float radius_multiplier;
	//     float point_size;
	// };
	//
	// NOTE: This fits within the minimum required size limit of 128 bytes for push constants.
	//       And push constants use the std430 layout.
	//
	PushConstants push_constants;

	// Convert clip space from OpenGL to Vulkan and pre-multiply projection transform.
	GLMatrix vulkan_view_projection = VulkanUtils::from_opengl_clip_space();
	vulkan_view_projection.gl_mult_matrix(view_projection.get_view_projection_transform());
	// Set view projection matrix.
	vulkan_view_projection.get_float_matrix(push_constants.view_projection);

	// Set the star colour.
	push_constants.star_colour[0] = d_colour.red();
	push_constants.star_colour[1] = d_colour.green();
	push_constants.star_colour[2] = d_colour.blue();
	push_constants.star_colour[3] = d_colour.alpha();

	// Set the radius multiplier.
	//
	// This is used for the 2D map views to expand the positions of the stars radially so that they're
	// outside the map bounding sphere. A value of 1.0 works for the 3D globe view.
	push_constants.radius_multiplier = static_cast<float>(radius_multiplier);

	// Small stars point size.
	//
	// Note: Multiply point sizes to account for ratio of device pixels to device *independent* pixels.
	//       On high-DPI displays there are more pixels in the same physical area on screen and so
	//       without increasing the point size the points would look too small.
	push_constants.point_size = SMALL_STARS_SIZE * device_pixel_ratio;

	// Set the push constants for the small stars.
	default_render_pass_command_buffer.pushConstants(
			d_pipeline_layout,
			vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
			// Update all push constants...
			0, sizeof(PushConstants), &push_constants);

	// Draw the small stars.
	default_render_pass_command_buffer.drawIndexed(d_num_small_star_vertex_indices, 1, 0, 0, 0);

	// Large stars point size.
	//
	// Note: Multiply point sizes to account for ratio of device pixels to device *independent* pixels.
	//       On high-DPI displays there are more pixels in the same physical area on screen and so
	//       without increasing the point size the points would look too small.
	push_constants.point_size = LARGE_STARS_SIZE * device_pixel_ratio;

	// Update the push constants for the large stars.
	default_render_pass_command_buffer.pushConstants(
			d_pipeline_layout,
			vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
			// Update only the point size...
			offsetof(PushConstants, point_size), sizeof(float), &push_constants.point_size);

	// Draw the large stars.
	// Their vertex indices (in the sole index buffer) come after the small star vertex indices.
	default_render_pass_command_buffer.drawIndexed(d_num_large_star_vertex_indices, 1, d_num_small_star_vertex_indices, 0, 0);
}


void
GPlatesOpenGL::Stars::create_graphics_pipeline(
		Vulkan &vulkan,
		vk::RenderPass default_render_pass,
		vk::SampleCountFlagBits default_render_pass_sample_count)
{
	//
	// Shader stages.
	//
	vk::PipelineShaderStageCreateInfo shader_stage_create_infos[2];
	// Vertex shader.
	const std::vector<std::uint32_t> vertex_shader_code = VulkanUtils::load_shader_code(":/stars.vert.spv");
	vk::UniqueShaderModule vertex_shader_module = vulkan.get_device().createShaderModuleUnique({ {}, {vertex_shader_code} });
	shader_stage_create_infos[0]
			.setStage(vk::ShaderStageFlagBits::eVertex)
			.setModule(vertex_shader_module.get())
			.setPName("main");
	// Fragment shader.
	const std::vector<std::uint32_t> fragment_shader_code = VulkanUtils::load_shader_code(":/stars.frag.spv");
	vk::UniqueShaderModule fragment_shader_module = vulkan.get_device().createShaderModuleUnique({ {}, {fragment_shader_code} });
	shader_stage_create_infos[1]
			.setStage(vk::ShaderStageFlagBits::eFragment)
			.setModule(fragment_shader_module.get())
			.setPName("main");

	//
	// Vertex input state.
	//
	vk::VertexInputBindingDescription vertex_binding_description;
	vertex_binding_description
			.setBinding(0)
			.setStride(sizeof(vertex_type))
			.setInputRate(vk::VertexInputRate::eVertex);
	// Specify vertex attributes (position).
	vk::VertexInputAttributeDescription vertex_attribute_description;
	vertex_attribute_description
			.setLocation(0)
			.setBinding(0)
			.setFormat(vk::Format::eR32G32B32Sfloat)  // format supports VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT
			.setOffset(0);
	vk::PipelineVertexInputStateCreateInfo vertex_input_state_create_info;
	vertex_input_state_create_info
			.setVertexBindingDescriptions(vertex_binding_description)
			.setVertexAttributeDescriptions(vertex_attribute_description);

	//
	// Input assembly state.
	//
	vk::PipelineInputAssemblyStateCreateInfo input_assembly_state_create_info;
	input_assembly_state_create_info.setTopology(vk::PrimitiveTopology::ePointList);

	//
	// Viewport state.
	//
	// Using one dynamic viewport and one dynamic scissor rect.
	vk::PipelineViewportStateCreateInfo viewport_state_create_info;
	viewport_state_create_info
			.setViewportCount(1)
			.setScissorCount(1);

	//
	// Rasterization state.
	//
	vk::PipelineRasterizationStateCreateInfo rasterization_state_create_info;
	// Enabling depth clamping disables the near and far clip planes (and clamps depth values outside).
	// This means the stars (which are beyond the far clip plane) get rendered (with the far depth 1.0).
	// However it means (for orthographic projection) that stars behind the viewer also get rendered.
	// Note that this doesn't happen for perspective projection since the 4 side planes form a pyramid
	// with apex at the view/camera position (and these 4 planes remove anything behind the viewer).
	// To get around this we clip to the near plane ourselves (using gl_ClipDistance in the shader).
	rasterization_state_create_info.setDepthClampEnable(true);

	//
	// Multisample state.
	//
	vk::PipelineMultisampleStateCreateInfo multisample_state_create_info;
	// Sample count must match the render pass.
	multisample_state_create_info.setRasterizationSamples(default_render_pass_sample_count);
	// Note: Don't need sample shading since each point primitive is a square (so MSAA only applies to square sides)
	//       and the anti-aliased edge of circle (generated in fragment shader) is inside that square.

	//
	// Depth stencil state.
	//
	// Disable depth testing and depth writes.
	// Stars are rendered in the background and don't really need depth sorting.
	vk::PipelineDepthStencilStateCreateInfo depth_stencil_state_create_info;  // default disables both

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

	// Create the graphics pipeline.
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
GPlatesOpenGL::Stars::create_stars(
		std::vector<vertex_type> &vertices,
		std::vector<vertex_index_type> &vertex_indices)
{
	// Set up the random number generator.
	// It generates doubles uniformly from -1.0 to 1.0 inclusive.
	// Note that we use a fixed seed (0), so that the pattern of stars does not
	// change between sessions. This is useful when trying to reproduce screenshots
	// between sessions.
	boost::mt19937 gen(0);
	boost::uniform_real<> dist(-1.0, 1.0);
	boost::function< double () > rand(
			boost::variate_generator<boost::mt19937&, boost::uniform_real<> >(gen, dist));

	stream_primitives_type stream;

	stream_primitives_type::StreamTarget stream_target(stream);

	stream_target.start_streaming(
			boost::in_place(boost::ref(vertices)),
			boost::in_place(boost::ref(vertex_indices)));

	// Stream the small stars.
	stream_stars(stream, rand, NUM_SMALL_STARS);

	d_num_small_star_vertices = stream_target.get_num_streamed_vertices();
	d_num_small_star_vertex_indices = stream_target.get_num_streamed_vertex_elements();

	stream_target.stop_streaming();

	// We re-start streaming so that we can get a separate stream count for the large stars.
	// However the large stars still get appended onto 'vertices' and 'vertex_indices'.
	stream_target.start_streaming(
			boost::in_place(boost::ref(vertices)),
			boost::in_place(boost::ref(vertex_indices)));

	// Stream the large stars.
	stream_stars(stream, rand, NUM_LARGE_STARS);

	d_num_large_star_vertices = stream_target.get_num_streamed_vertices();
	d_num_large_star_vertex_indices = stream_target.get_num_streamed_vertex_elements();

	stream_target.stop_streaming();
}


void
GPlatesOpenGL::Stars::stream_stars(
		stream_primitives_type &stream,
		boost::function< double () > &rand,
		unsigned int num_stars)
{
	bool ok = true;

	stream_primitives_type::Points stream_points(stream);
	stream_points.begin_points();

	unsigned int points_generated = 0;
	while (points_generated != num_stars)
	{
		// See http://mathworld.wolfram.com/SpherePointPicking.html for a discussion
		// of picking points uniformly on the surface of a sphere.
		// We use the method attributed to Marsaglia (1972).
		double x_1 = rand();
		double x_2 = rand();
		double x_1_sq = x_1 * x_1;
		double x_2_sq = x_2 * x_2;

		double stuff_under_sqrt = 1 - x_1_sq - x_2_sq;
		if (stuff_under_sqrt < 0)
		{
			continue;
		}
		double sqrt_part = std::sqrt(stuff_under_sqrt);

		double x = 2 * x_1 * sqrt_part;
		double y = 2 * x_2 * sqrt_part;
		double z = 1 - 2 * (x_1_sq + x_2_sq);

		// Randomising the distance to the stars gives a nicer 3D effect.
		double radius = RADIUS + rand();

		const vertex_type vertex =
		{
			static_cast<float>(x * radius),
			static_cast<float>(y * radius),
			static_cast<float>(z * radius)
		};
		ok = ok && stream_points.add_vertex(vertex);

		++points_generated;
	}

	stream_points.end_points();

	// Since we added vertices/indices to a std::vector we shouldn't have run out of space.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			ok,
			GPLATES_ASSERTION_SOURCE);
}


void
GPlatesOpenGL::Stars::load_stars(
		GPlatesOpenGL::Vulkan &vulkan,
		vk::CommandBuffer initialisation_command_buffer,
		vk::Fence initialisation_submit_fence,
		const std::vector<vertex_type> &vertices,
		const std::vector<vertex_index_type> &vertex_indices)
{
	// Vulkan memory allocator.
	VmaAllocator vma_allocator = vulkan.get_vma_allocator();

	// Common buffer parameters.
	vk::BufferCreateInfo buffer_create_info;
	buffer_create_info.setSharingMode(vk::SharingMode::eExclusive);

	// Common allocation parameters.
	VmaAllocationCreateInfo allocation_create_info = {};
	allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;


	//
	// Create staging and final vertex buffers.
	//
	buffer_create_info.setSize(vertices.size() * sizeof(vertex_type));

	// Staging vertex buffer (in mappable host memory).
	buffer_create_info.setUsage(vk::BufferUsageFlagBits::eTransferSrc);
	allocation_create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;  // host mappable
	VulkanBuffer staging_vertex_buffer = VulkanBuffer::create(vma_allocator, buffer_create_info, allocation_create_info, GPLATES_EXCEPTION_SOURCE);

	// Final vertex buffer (in device local memory).
	buffer_create_info.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer);
	allocation_create_info.flags = 0;  // device local memory
	d_vertex_buffer = VulkanBuffer::create(vma_allocator, buffer_create_info, allocation_create_info, GPLATES_EXCEPTION_SOURCE);

	//
	// Create staging and final index buffers.
	//
	buffer_create_info.setSize(vertex_indices.size() * sizeof(vertex_index_type));

	// Staging index buffer (in mappable host memory).
	buffer_create_info.setUsage(vk::BufferUsageFlagBits::eTransferSrc);
	allocation_create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;  // host mappable
	VulkanBuffer staging_index_buffer = VulkanBuffer::create(vma_allocator, buffer_create_info, allocation_create_info, GPLATES_EXCEPTION_SOURCE);

	// Final index buffer (in device local memory).
	buffer_create_info.setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer);
	allocation_create_info.flags = 0;  // device local memory
	d_index_buffer = VulkanBuffer::create(vma_allocator, buffer_create_info, allocation_create_info, GPLATES_EXCEPTION_SOURCE);


	//
	// Copy the vertices into the mapped staging vertex buffer.
	//
	void *staging_vertex_buffer_data = staging_vertex_buffer.map_memory(vma_allocator, GPLATES_EXCEPTION_SOURCE);
	std::memcpy(staging_vertex_buffer_data, vertices.data(), vertices.size() * sizeof(vertex_type));
	staging_vertex_buffer.flush_mapped_memory(vma_allocator, 0, VK_WHOLE_SIZE, GPLATES_EXCEPTION_SOURCE);
	staging_vertex_buffer.unmap_memory(vma_allocator);

	//
	// Copy the vertex indices into the mapped staging index buffer.
	//
	void *staging_index_buffer_data = staging_index_buffer.map_memory(vma_allocator, GPLATES_EXCEPTION_SOURCE);
	std::memcpy(staging_index_buffer_data, vertex_indices.data(), vertex_indices.size() * sizeof(vertex_index_type));
	staging_index_buffer.flush_mapped_memory(vma_allocator, 0, VK_WHOLE_SIZE, GPLATES_EXCEPTION_SOURCE);
	staging_index_buffer.unmap_memory(vma_allocator);


	// Begin recording into the initialisation command buffer.
	vk::CommandBufferBeginInfo initialisation_command_buffer_begin_info;
	// Command buffer will only be submitted once.
	initialisation_command_buffer_begin_info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	initialisation_command_buffer.begin(initialisation_command_buffer_begin_info);

	//
	// Copy vertices from staging host buffer to final device buffer.
	//
	vk::BufferCopy vertex_buffer_copy;
	vertex_buffer_copy.setSrcOffset(0).setDstOffset(0).setSize(vertices.size() * sizeof(vertex_type));
	initialisation_command_buffer.copyBuffer(
			staging_vertex_buffer.get_buffer(),
			d_vertex_buffer.get_buffer(),
			vertex_buffer_copy);

	//
	// Copy vertex indices from staging host buffer to final device buffer.
	//
	vk::BufferCopy index_buffer_copy;
	index_buffer_copy.setSrcOffset(0).setDstOffset(0).setSize(vertex_indices.size() * sizeof(vertex_index_type));
	initialisation_command_buffer.copyBuffer(
			staging_index_buffer.get_buffer(),
			d_index_buffer.get_buffer(),
			index_buffer_copy);


	// Pipeline barrier to wait for staging transfer writes to be visible as vertex/index data.
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
		throw VulkanException(GPLATES_EXCEPTION_SOURCE, "Error waiting for initialisation of stars.");
	}
	vulkan.get_device().resetFences(initialisation_submit_fence);

	// Destroy staging buffers now that device is no longer using them.
	VulkanBuffer::destroy(vma_allocator, staging_vertex_buffer);
	VulkanBuffer::destroy(vma_allocator, staging_index_buffer);
}
