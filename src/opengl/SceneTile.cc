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

#include "SceneTile.h"

#include "GLViewProjection.h"
#include "VulkanException.h"
#include "VulkanUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "utils/CallStackTracker.h"


namespace
{
	//
	// The allowed number of scene fragments that can be rendered per pixel on average.
	//
	// Some pixels will have more fragments that this and most will have less.
	// As long as the average over all screen pixels is less than this then we have allocated enough storage.
	//
	const unsigned int AVERAGE_FRAGMENTS_PER_PIXEL = 4;
}


GPlatesOpenGL::SceneTile::SceneTile() :
	d_num_fragments_in_storage(0)
{
}


void
GPlatesOpenGL::SceneTile::initialise_vulkan_resources(
		Vulkan &vulkan,
		vk::RenderPass default_render_pass,
		vk::SampleCountFlagBits default_render_pass_sample_count,
		vk::CommandBuffer initialisation_command_buffer,
		vk::Fence initialisation_submit_fence)
{
	// Add this scope to the call stack trace printed if exception thrown in this scope.
	TRACK_CALL_STACK();

	// Create the "clear" and "blend" graphics pipelines.
	create_graphics_pipelines(vulkan, default_render_pass, default_render_pass_sample_count);

	// Create the descriptors (image/buffer resources for fragment list head and fragment storage/allocator).
	create_descriptors(vulkan, initialisation_command_buffer, initialisation_submit_fence);

	// Create descriptor set.
	create_descriptor_set(vulkan);
}


void
GPlatesOpenGL::SceneTile::release_vulkan_resources(
		Vulkan &vulkan)
{
	// Vulkan memory allocator.
	VmaAllocator vma_allocator = vulkan.get_vma_allocator();

	vulkan.get_device().destroyImageView(d_fragment_list_head_image_view);
	VulkanImage::destroy(vma_allocator, d_fragment_list_head_image);

	VulkanBuffer::destroy(vma_allocator, d_fragment_storage_buffer);
	VulkanBuffer::destroy(vma_allocator, d_fragment_allocator_buffer);

	d_num_fragments_in_storage = 0;

	// Destroy descriptor pools/sets.
	vulkan.get_device().destroyDescriptorPool(d_descriptor_pool);  // also frees descriptor set

	// Destroy descriptor set layouts.
	vulkan.get_device().destroyDescriptorSetLayout(d_descriptor_set_layout);

	// Destroy the graphics pipelines and layouts.
	vulkan.get_device().destroyPipeline(d_clear_graphics_pipeline);
	vulkan.get_device().destroyPipeline(d_blend_graphics_pipeline);
	vulkan.get_device().destroyPipelineLayout(d_graphics_pipeline_layout);
}


std::vector<vk::WriteDescriptorSet>
GPlatesOpenGL::SceneTile::get_write_descriptor_sets(
		vk::DescriptorSet descriptor_set,
		std::uint32_t binding,
		std::vector<vk::DescriptorImageInfo> &descriptor_image_infos,
		std::vector<vk::DescriptorBufferInfo> &descriptor_buffer_infos) const
{
	// These structures need to exist beyond this function since they are
	// referenced by the returned vk::WriteDescriptorSet structures.
	//
	// We have one descriptor image.
	descriptor_image_infos.resize(1);
	vk::DescriptorImageInfo &fragment_list_head_descriptor_image_info = descriptor_image_infos[0];
	// We have two descriptor buffers.
	descriptor_buffer_infos.resize(2);
	vk::DescriptorBufferInfo &fragment_storage_descriptor_buffer_info = descriptor_buffer_infos[0];
	vk::DescriptorBufferInfo &fragment_allocator_descriptor_buffer_info = descriptor_buffer_infos[1];

	// Fragment list head image.
	fragment_list_head_descriptor_image_info
			.setImageView(d_fragment_list_head_image_view)
			.setImageLayout(vk::ImageLayout::eGeneral);
	vk::WriteDescriptorSet fragment_list_head_write_descriptor_set;
	fragment_list_head_write_descriptor_set
			.setDstSet(descriptor_set)
			.setDstBinding(binding)
			.setDstArrayElement(0)
			.setDescriptorType(vk::DescriptorType::eStorageImage)
			.setImageInfo(fragment_list_head_descriptor_image_info);

	// Fragment storage buffer.
	fragment_storage_descriptor_buffer_info
			.setBuffer(d_fragment_storage_buffer.get_buffer())
			.setOffset(vk::DeviceSize(0))
			.setRange(VK_WHOLE_SIZE);
	vk::WriteDescriptorSet fragment_storage_write_descriptor_set;
	fragment_storage_write_descriptor_set
			.setDstSet(descriptor_set)
			.setDstBinding(binding + 1)
			.setDstArrayElement(0)
			.setDescriptorType(vk::DescriptorType::eStorageBuffer)
			.setBufferInfo(fragment_storage_descriptor_buffer_info);

	// Fragment allocator buffer.
	fragment_allocator_descriptor_buffer_info
			.setBuffer(d_fragment_allocator_buffer.get_buffer())
			.setOffset(vk::DeviceSize(0))
			.setRange(VK_WHOLE_SIZE);
	vk::WriteDescriptorSet fragment_allocator_write_descriptor_set;
	fragment_allocator_write_descriptor_set
			.setDstSet(descriptor_set)
			.setDstBinding(binding + 2)
			.setDstArrayElement(0)
			.setDescriptorType(vk::DescriptorType::eStorageBuffer)
			.setBufferInfo(fragment_allocator_descriptor_buffer_info);

	return {
			fragment_list_head_write_descriptor_set,
			fragment_storage_write_descriptor_set,
			fragment_allocator_write_descriptor_set };
}


std::vector<vk::DescriptorSetLayoutBinding>
GPlatesOpenGL::SceneTile::get_descriptor_set_layout_bindings(
		std::uint32_t binding,
		vk::ShaderStageFlags shader_stage_flags) const
{
	// Fragment list head image.
	vk::DescriptorSetLayoutBinding fragment_list_head_descriptor_set_layout_binding;
	fragment_list_head_descriptor_set_layout_binding
			.setBinding(binding)
			.setDescriptorType(vk::DescriptorType::eStorageImage)
			.setDescriptorCount(1)
			.setStageFlags(shader_stage_flags);

	// Fragment storage buffer.
	vk::DescriptorSetLayoutBinding fragment_storage_descriptor_set_layout_binding;
	fragment_storage_descriptor_set_layout_binding
			.setBinding(binding + 1)
			.setDescriptorType(vk::DescriptorType::eStorageBuffer)
			.setDescriptorCount(1)
			.setStageFlags(shader_stage_flags);

	// Fragment allocator buffer.
	vk::DescriptorSetLayoutBinding fragment_allocator_descriptor_set_layout_binding;
	fragment_allocator_descriptor_set_layout_binding
			.setBinding(binding + 2)
			.setDescriptorType(vk::DescriptorType::eStorageBuffer)
			.setDescriptorCount(1)
			.setStageFlags(shader_stage_flags);

	return {
			fragment_list_head_descriptor_set_layout_binding,
			fragment_storage_descriptor_set_layout_binding,
			fragment_allocator_descriptor_set_layout_binding };
}


std::vector<vk::DescriptorPoolSize>
GPlatesOpenGL::SceneTile::get_descriptor_pool_sizes() const
{
	// Fragment list head image.
	vk::DescriptorPoolSize image_descriptor_pool_size;
	image_descriptor_pool_size
			.setType(vk::DescriptorType::eStorageImage)
			.setDescriptorCount(1);

	// Fragment storage buffer and fragment allocator buffer.
	vk::DescriptorPoolSize buffer_descriptor_pool_size;
	buffer_descriptor_pool_size
			.setType(vk::DescriptorType::eStorageBuffer)
			.setDescriptorCount(2);

	return { image_descriptor_pool_size, buffer_descriptor_pool_size };
}


void
GPlatesOpenGL::SceneTile::clear(
		Vulkan &vulkan,
		vk::CommandBuffer default_render_pass_command_buffer,
		const GLViewProjection &view_projection)
{
	// Pipeline barrier to wait for any fragment shader write operations (from a previous "blend")
	// to complete before reading/writing in fragment shader.
#if defined(_WIN32) && defined(MemoryBarrier) // See "VulkanHpp.h" for why this is necessary.
#	undef MemoryBarrier
#endif
	vk::MemoryBarrier clear_memory_barrier;
	clear_memory_barrier
			.setSrcAccessMask(vk::AccessFlagBits::eShaderWrite)
			.setDstAccessMask(vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite);
	default_render_pass_command_buffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eFragmentShader,
			vk::PipelineStageFlagBits::eFragmentShader,
			// Barriers must specify "vk::DependencyFlagBits::eByRegion" when inside a subpass...
			vk::DependencyFlagBits::eByRegion,  // dependencyFlags
			clear_memory_barrier,  // memoryBarriers
			{},  // bufferMemoryBarriers
			{});  // imageMemoryBarriers

	// Bind the graphics pipeline.
	default_render_pass_command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, d_clear_graphics_pipeline);

	// Set viewport and scissor rects.
	default_render_pass_command_buffer.setViewport(0, view_projection.get_viewport().get_vulkan_viewport());
	default_render_pass_command_buffer.setScissor(0, view_projection.get_viewport().get_vulkan_rect_2D());

	// Bind the descriptor sets used by graphics pipeline.
	//
	// Set 0: Scene tile image/buffer descriptors.
	default_render_pass_command_buffer.bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics,
			d_graphics_pipeline_layout,
			0/*firstSet*/,
			d_descriptor_set,
			nullptr/*dynamicOffsets*/);

	// Draw the fullscreen "quad" (actually a triangle that covers the screen after clipping).
	default_render_pass_command_buffer.draw(3, 1, 0, 0);

	// Pipeline barrier to wait for the above clear write operations to complete before
	// the scene can be rendered into this scene tile.
	vk::MemoryBarrier pre_render_memory_barrier;
	pre_render_memory_barrier
			.setSrcAccessMask(vk::AccessFlagBits::eShaderWrite)
			.setDstAccessMask(vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite);
	default_render_pass_command_buffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eFragmentShader,
			vk::PipelineStageFlagBits::eFragmentShader,
			// Barriers must specify "vk::DependencyFlagBits::eByRegion" when inside a subpass...
			vk::DependencyFlagBits::eByRegion,  // dependencyFlags
			pre_render_memory_barrier,  // memoryBarriers
			{},  // bufferMemoryBarriers
			{});  // imageMemoryBarriers
}


void
GPlatesOpenGL::SceneTile::render(
		Vulkan &vulkan,
		vk::CommandBuffer default_render_pass_command_buffer,
		const GLViewProjection &view_projection)
{
	// Pipeline barrier to wait for any fragment shader write operations (from rendering the scene into
	// this scene tile) to complete before reading/writing in fragment shader (during "blend" operations).
#if defined(_WIN32) && defined(MemoryBarrier) // See "VulkanHpp.h" for why this is necessary.
#	undef MemoryBarrier
#endif
	vk::MemoryBarrier blend_memory_barrier;
	blend_memory_barrier
			.setSrcAccessMask(vk::AccessFlagBits::eShaderWrite)
			.setDstAccessMask(vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite);
	default_render_pass_command_buffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eFragmentShader,
			vk::PipelineStageFlagBits::eFragmentShader,
			// Barriers must specify "vk::DependencyFlagBits::eByRegion" when inside a subpass...
			vk::DependencyFlagBits::eByRegion,  // dependencyFlags
			blend_memory_barrier,  // memoryBarriers
			{},  // bufferMemoryBarriers
			{});  // imageMemoryBarriers

	// Bind the graphics pipeline.
	default_render_pass_command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, d_blend_graphics_pipeline);

	// Set viewport and scissor rects.
	default_render_pass_command_buffer.setViewport(0, view_projection.get_viewport().get_vulkan_viewport());
	default_render_pass_command_buffer.setScissor(0, view_projection.get_viewport().get_vulkan_rect_2D());

	// Bind the descriptor sets used by graphics pipeline.
	//
	// Set 0: Scene tile image/buffer descriptors.
	default_render_pass_command_buffer.bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics,
			d_graphics_pipeline_layout,
			0/*firstSet*/,
			d_descriptor_set,
			nullptr/*dynamicOffsets*/);

	// Draw the fullscreen "quad" (actually a triangle that covers the screen after clipping).
	default_render_pass_command_buffer.draw(3, 1, 0, 0);
}


void
GPlatesOpenGL::SceneTile::create_graphics_pipelines(
		Vulkan &vulkan,
		vk::RenderPass default_render_pass,
		vk::SampleCountFlagBits default_render_pass_sample_count)
{
	//
	// Shader stages.
	//
	vk::PipelineShaderStageCreateInfo clear_pipeline_shader_stage_create_infos[2];
	vk::PipelineShaderStageCreateInfo blend_pipeline_shader_stage_create_infos[2];

	// Vertex shader (for clear and blend pipelines).
	const std::vector<std::uint32_t> vertex_shader_code = VulkanUtils::load_shader_code(":/scene_tile.vert.spv");
	vk::UniqueShaderModule vertex_shader_module = vulkan.get_device().createShaderModuleUnique({ {}, {vertex_shader_code} });
	clear_pipeline_shader_stage_create_infos[0].setStage(vk::ShaderStageFlagBits::eVertex).setModule(vertex_shader_module.get()).setPName("main");
	blend_pipeline_shader_stage_create_infos[0].setStage(vk::ShaderStageFlagBits::eVertex).setModule(vertex_shader_module.get()).setPName("main");

	// Fragment shader specialization constants (set the sample count in the fragment shader).
	std::uint32_t fragment_shader_specialization_data[1] = { VulkanUtils::get_sample_count(default_render_pass_sample_count) };
	vk::SpecializationMapEntry fragment_shader_specialization_map_entries[1] =
	{
		{ SAMPLE_COUNT_CONSTANT_ID/*constant_id*/, 0/*offset*/, sizeof(std::uint32_t) }
	};
	vk::SpecializationInfo fragment_shader_specialization_info;
	fragment_shader_specialization_info
			.setMapEntries(fragment_shader_specialization_map_entries)
			.setData<std::uint32_t>(fragment_shader_specialization_data);

	// Fragment shader for "clear" pipeline.
	const std::vector<std::uint32_t> clear_fragment_shader_code = VulkanUtils::load_shader_code(":/scene_tile_clear.frag.spv");
	vk::UniqueShaderModule clear_fragment_shader_module = vulkan.get_device().createShaderModuleUnique({ {}, {clear_fragment_shader_code} });
	clear_pipeline_shader_stage_create_infos[1]
			.setStage(vk::ShaderStageFlagBits::eFragment)
			.setModule(clear_fragment_shader_module.get())
			.setPName("main")
			.setPSpecializationInfo(&fragment_shader_specialization_info);

	// Fragment shader for "blend" pipeline.
	const std::vector<std::uint32_t> blend_fragment_shader_code = VulkanUtils::load_shader_code(":/scene_tile_blend.frag.spv");
	vk::UniqueShaderModule blend_fragment_shader_module = vulkan.get_device().createShaderModuleUnique({ {}, {blend_fragment_shader_code} });
	blend_pipeline_shader_stage_create_infos[1]
			.setStage(vk::ShaderStageFlagBits::eFragment)
			.setModule(blend_fragment_shader_module.get())
			.setPName("main")
			.setPSpecializationInfo(&fragment_shader_specialization_info);

	//
	// Vertex input state.
	//
	// Our vertex shader requires no vertex buffer (since rendering fullscreen quad using 'gl_VertexIndex').
	// So we have no vertex input attributes or bindings.
	vk::PipelineVertexInputStateCreateInfo vertex_input_state_create_info;

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
	// Fullscreen quad or oriented clockwise.
	vk::PipelineRasterizationStateCreateInfo rasterization_state_create_info;
	rasterization_state_create_info
			.setPolygonMode(vk::PolygonMode::eFill)
			.setCullMode(vk::CullModeFlagBits::eBack)
			.setFrontFace(vk::FrontFace::eClockwise);

	//
	// Multisample state.
	//
	vk::PipelineMultisampleStateCreateInfo multisample_state_create_info;
	// Sample count must match the render pass.
	multisample_state_create_info.setRasterizationSamples(default_render_pass_sample_count);

	//
	// Depth stencil state.
	//
	// Disable depth testing and depth writes.
	vk::PipelineDepthStencilStateCreateInfo depth_stencil_state_create_info;  // default disables both

	//
	// Colour blend state.
	//

	// For the "clear" pipeline we disable colour writes because we're not writing to the framebuffer
	// (since fragment shader is instead clearing per-pixel fragment lists in a storage image/buffer)
	// and so our colour attachment output will be undefined.
	vk::PipelineColorBlendAttachmentState clear_pipeline_blend_attachment_state;
	clear_pipeline_blend_attachment_state
			.setBlendEnable(false)
			.setColorWriteMask({});
	vk::PipelineColorBlendStateCreateInfo clear_pipeline_color_blend_state_create_info;
	clear_pipeline_color_blend_state_create_info.setAttachments(clear_pipeline_blend_attachment_state);

	// However the "blend" pipeline does write to the framebuffer.
	vk::PipelineColorBlendAttachmentState blend_pipeline_blend_attachment_state;
	blend_pipeline_blend_attachment_state
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
	vk::PipelineColorBlendStateCreateInfo blend_pipeline_color_blend_state_create_info;
	blend_pipeline_color_blend_state_create_info.setAttachments(blend_pipeline_blend_attachment_state);

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

	// Scene tile descriptor set layout.
	const std::vector<vk::DescriptorSetLayoutBinding> descriptor_set_layout_bindings =
			get_descriptor_set_layout_bindings(DESCRIPTOR_BINDING, vk::ShaderStageFlagBits::eFragment);
	vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info;
	descriptor_set_layout_create_info.setBindings(descriptor_set_layout_bindings);
	d_descriptor_set_layout = vulkan.get_device().createDescriptorSetLayout(descriptor_set_layout_create_info);

	// Descriptor set layouts:
	// - set 0: scene tile descriptors
	vk::DescriptorSetLayout descriptor_set_layouts[1] = { d_descriptor_set_layout };

	vk::PipelineLayoutCreateInfo pipeline_layout_create_info;
	pipeline_layout_create_info.setSetLayouts(descriptor_set_layouts);
	d_graphics_pipeline_layout = vulkan.get_device().createPipelineLayout(pipeline_layout_create_info);

	//
	// Graphics pipeline.
	//

	// Create the "clear" graphics pipelines (to clear the fragment list head image and reset the fragment allocator buffer).
	vk::GraphicsPipelineCreateInfo clear_graphics_pipeline_info;
	clear_graphics_pipeline_info
			.setStages(clear_pipeline_shader_stage_create_infos)
			.setPVertexInputState(&vertex_input_state_create_info)
			.setPInputAssemblyState(&input_assembly_state_create_info)
			.setPViewportState(&viewport_state_create_info)
			.setPRasterizationState(&rasterization_state_create_info)
			.setPMultisampleState(&multisample_state_create_info)
			.setPDepthStencilState(&depth_stencil_state_create_info)
			.setPColorBlendState(&clear_pipeline_color_blend_state_create_info)
			.setPDynamicState(&dynamic_state_create_info)
			.setLayout(d_graphics_pipeline_layout)
			.setRenderPass(default_render_pass);
	d_clear_graphics_pipeline = vulkan.get_device().createGraphicsPipeline({}, clear_graphics_pipeline_info).value;

	// Create the "blend" graphics pipelines (to blend the fragment lists into the framebuffer).
	vk::GraphicsPipelineCreateInfo blend_graphics_pipeline_info;
	blend_graphics_pipeline_info
			.setStages(blend_pipeline_shader_stage_create_infos)
			.setPVertexInputState(&vertex_input_state_create_info)
			.setPInputAssemblyState(&input_assembly_state_create_info)
			.setPViewportState(&viewport_state_create_info)
			.setPRasterizationState(&rasterization_state_create_info)
			.setPMultisampleState(&multisample_state_create_info)
			.setPDepthStencilState(&depth_stencil_state_create_info)
			.setPColorBlendState(&blend_pipeline_color_blend_state_create_info)
			.setPDynamicState(&dynamic_state_create_info)
			.setLayout(d_graphics_pipeline_layout)
			.setRenderPass(default_render_pass);
	d_blend_graphics_pipeline = vulkan.get_device().createGraphicsPipeline({}, blend_graphics_pipeline_info).value;
}


void
GPlatesOpenGL::SceneTile::create_descriptors(
		Vulkan &vulkan,
		vk::CommandBuffer initialisation_command_buffer,
		vk::Fence initialisation_submit_fence)
{
	// Image and allocation create info parameters for fragment list head image.
	vk::ImageCreateInfo fragment_list_head_image_create_info;
	fragment_list_head_image_create_info
			.setImageType(vk::ImageType::e2D)
			.setFormat(FRAGMENT_LIST_HEAD_IMAGE_TEXEL_FORMAT)
			.setExtent({ FRAGMENT_TILE_DIMENSION, FRAGMENT_TILE_DIMENSION, 1 })
			.setMipLevels(1)
			.setArrayLayers(1)
			.setSamples(vk::SampleCountFlagBits::e1)
			.setTiling(vk::ImageTiling::eOptimal)
			.setUsage(vk::ImageUsageFlagBits::eStorage)
			.setSharingMode(vk::SharingMode::eExclusive)
			.setInitialLayout(vk::ImageLayout::eUndefined);
	// Image allocation.
	VmaAllocationCreateInfo fragment_list_head_image_allocation_create_info = {};
	fragment_list_head_image_allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;
	fragment_list_head_image_allocation_create_info.flags = 0;  // device local memory

	// Create the fragment list head image.
	d_fragment_list_head_image = VulkanImage::create(
			vulkan.get_vma_allocator(),
			fragment_list_head_image_create_info,
			fragment_list_head_image_allocation_create_info,
			GPLATES_EXCEPTION_SOURCE);

	// Image view create info parameters for fragment list head image.
	vk::ImageViewCreateInfo fragment_list_head_image_view_create_info;
	fragment_list_head_image_view_create_info
			.setImage(d_fragment_list_head_image.get_image())
			.setViewType(vk::ImageViewType::e2D)
			.setFormat(FRAGMENT_LIST_HEAD_IMAGE_TEXEL_FORMAT)
			.setComponents({})  // identity swizzle
			.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

	// Create the fragment list head image view.
	d_fragment_list_head_image_view = vulkan.get_device().createImageView(fragment_list_head_image_view_create_info);


	// Determine size of fragment storage buffer and ensure it doesn't exceed maximum storage buffer range.
	//
	// Note: Vulkan guarantees support of at least 128MB for the maximum storage buffer range.
	d_num_fragments_in_storage = FRAGMENT_TILE_DIMENSION * FRAGMENT_TILE_DIMENSION * AVERAGE_FRAGMENTS_PER_PIXEL;
	if (d_num_fragments_in_storage * NUM_BYTES_PER_FRAGMENT > vulkan.get_physical_device_properties().limits.maxStorageBufferRange)
	{
		d_num_fragments_in_storage = vulkan.get_physical_device_properties().limits.maxStorageBufferRange / NUM_BYTES_PER_FRAGMENT;
	}

	// Buffer and allocation create info parameters for fragment storage buffer.
	vk::BufferCreateInfo fragment_storage_buffer_create_info;
	fragment_storage_buffer_create_info
			.setSize(vk::DeviceSize(d_num_fragments_in_storage) * NUM_BYTES_PER_FRAGMENT)
			.setUsage(vk::BufferUsageFlagBits::eStorageBuffer)
			.setSharingMode(vk::SharingMode::eExclusive);
	VmaAllocationCreateInfo fragment_storage_buffer_allocation_create_info = {};
	fragment_storage_buffer_allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;
	fragment_storage_buffer_allocation_create_info.flags = 0;  // device local memory

	// Create the fragment storage buffer.
	d_fragment_storage_buffer = VulkanBuffer::create(
			vulkan.get_vma_allocator(),
			fragment_storage_buffer_create_info,
			fragment_storage_buffer_allocation_create_info,
			GPLATES_EXCEPTION_SOURCE);


	// Buffer and allocation create info parameters for fragment allocator buffer.
	vk::BufferCreateInfo fragment_allocator_buffer_create_info;
	fragment_allocator_buffer_create_info
			.setSize(4)  // buffer contains a single uint
			.setUsage(vk::BufferUsageFlagBits::eStorageBuffer)
			.setSharingMode(vk::SharingMode::eExclusive);
	VmaAllocationCreateInfo fragment_allocator_buffer_allocation_create_info = {};
	fragment_allocator_buffer_allocation_create_info.usage = VMA_MEMORY_USAGE_AUTO;
	fragment_allocator_buffer_allocation_create_info.flags = 0;  // device local memory

	// Create the fragment allocator buffer.
	d_fragment_allocator_buffer = VulkanBuffer::create(
			vulkan.get_vma_allocator(),
			fragment_allocator_buffer_create_info,
			fragment_allocator_buffer_allocation_create_info,
			GPLATES_EXCEPTION_SOURCE);

	// Begin recording into the initialisation command buffer.
	vk::CommandBufferBeginInfo initialisation_command_buffer_begin_info;
	// Command buffer will only be submitted once.
	initialisation_command_buffer_begin_info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	initialisation_command_buffer.begin(initialisation_command_buffer_begin_info);

	// Pipeline barrier to transition fragment list head image to an image layout
	// suitable for shader reads/writes of a storage image (vk::ImageLayout::eGeneral).
	vk::ImageMemoryBarrier fragment_list_head_image_memory_barrier;
	fragment_list_head_image_memory_barrier
			.setSrcAccessMask({})
			.setDstAccessMask(vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite)
			.setOldLayout(vk::ImageLayout::eUndefined)
			.setNewLayout(vk::ImageLayout::eGeneral)
			.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setImage(d_fragment_list_head_image.get_image())
			.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });
	initialisation_command_buffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eTopOfPipe,  // don't need to wait to access freshly allocated memory
			vk::PipelineStageFlagBits::eFragmentShader, // clearing image happens in fragment shader
			{},  // dependencyFlags
			{},  // memoryBarriers
			{},  // bufferMemoryBarriers
			fragment_list_head_image_memory_barrier);  // imageMemoryBarriers

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
		throw VulkanException(GPLATES_EXCEPTION_SOURCE, "Error waiting for initialisation of scene tile image.");
	}
	vulkan.get_device().resetFences(initialisation_submit_fence);
}


void
GPlatesOpenGL::SceneTile::create_descriptor_set(
		Vulkan &vulkan)
{
	// Create descriptor pool.
	const std::vector<vk::DescriptorPoolSize> descriptor_pool_sizes = get_descriptor_pool_sizes();
	vk::DescriptorPoolCreateInfo descriptor_pool_create_info;
	descriptor_pool_create_info.setMaxSets(1).setPoolSizes(descriptor_pool_sizes);
	d_descriptor_pool = vulkan.get_device().createDescriptorPool(descriptor_pool_create_info);

	// Allocate descriptor set.
	vk::DescriptorSetAllocateInfo descriptor_set_allocate_info;
	descriptor_set_allocate_info
			.setDescriptorPool(d_descriptor_pool)
			.setSetLayouts(d_descriptor_set_layout);
	const std::vector<vk::DescriptorSet> descriptor_sets =
			vulkan.get_device().allocateDescriptorSets(descriptor_set_allocate_info);
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			descriptor_sets.size() == 1,
			GPLATES_ASSERTION_SOURCE);
	d_descriptor_set = descriptor_sets[0];

	// Descriptor writes.
	std::vector<vk::DescriptorImageInfo> descriptor_image_infos;
	std::vector<vk::DescriptorBufferInfo> descriptor_buffer_infos;
	const std::vector<vk::WriteDescriptorSet> descriptor_writes =
			get_write_descriptor_sets(
					d_descriptor_set,
					DESCRIPTOR_BINDING,
					descriptor_image_infos,
					descriptor_buffer_infos);

	// Update descriptor set.
	vulkan.get_device().updateDescriptorSets(descriptor_writes, nullptr/*descriptorCopies*/);
}
