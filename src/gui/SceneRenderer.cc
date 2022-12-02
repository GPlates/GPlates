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

#include <QString>

#include "SceneRenderer.h"

#include "Colour.h"
#include "MapProjection.h"
#include "Scene.h"
#include "SceneOverlays.h"
#include "SceneView.h"
#include "ViewportZoom.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "opengl/GLCapabilities.h"
#include "opengl/GLImageUtils.h"
#include "opengl/GLMatrix.h"
#include "opengl/GLShader.h"
#include "opengl/GLShaderSource.h"
#include "opengl/GLTileRender.h"
#include "opengl/GLUtils.h"
#include "opengl/GLViewProjection.h"
#include "opengl/OpenGL.h"
#include "opengl/OpenGLException.h"

#include "presentation/ViewState.h"

#include "utils/CallStackTracker.h"


namespace
{
	// Vertex and fragment shader that sorts and blends the list of fragments (per pixel) in depth order.
	const QString SORT_AND_BLEND_SCENE_FRAGMENTS_VERTEX_SHADER_SOURCE_FILE_NAME = ":/opengl/sort_and_blend_scene_fragments.vert";
	const QString SORT_AND_BLEND_SCENE_FRAGMENTS_FRAGMENT_SHADER_SOURCE_FILE_NAME = ":/opengl/sort_and_blend_scene_fragments.frag";

	// The maximum allowed scene fragments that can be rendered per pixel on average.
	// Some pixels will have more fragments that this and most will have less.
	// As long as the average over all screen pixels is less than this then we have allocated enough storage.
	const unsigned int MAX_AVERAGE_FRAGMENTS_PER_PIXEL = 4;
	// Each fragment is a 16 byte 'uvec4'.
	const unsigned int MAX_AVERAGE_FRAGMENT_BYTES_PER_PIXEL = 16/*bytes per rgba32ui*/ * MAX_AVERAGE_FRAGMENTS_PER_PIXEL;
}


GPlatesGui::SceneRenderer::SceneRenderer(
		GPlatesPresentation::ViewState &view_state) :
	d_view_state(view_state),
	d_map_projection(view_state.get_map_projection()),
	d_max_fragment_list_head_pointer_image_width(0),
	d_max_fragment_list_head_pointer_image_height(0),
	d_max_fragment_list_storage_buffer_bytes(0),
	d_off_screen_render_target_dimension(OFF_SCREEN_RENDER_TARGET_DIMENSION)
{
}


void
GPlatesGui::SceneRenderer::initialise_vulkan_resources(
		GPlatesOpenGL::VulkanDevice &vulkan_device,
		vk::RenderPass default_render_pass)
{
	//
	// Create a graphics+compute command pool.
	//
	// Note that we not creating a separate command pool for each asynchronous frame.
	// Each command buffer will tend to be used for a specific purpose (eg, default command buffer
	// used to render into default framebuffer) and so will have consistent memory allocations (from pool).
	vk::CommandPoolCreateInfo graphics_and_compute_command_pool_create_info;
	graphics_and_compute_command_pool_create_info
			// Each command buffer can be reset (eg, implicitly by beginning a command buffer).
			// And each command buffer will not be re-used (ie, used once and then reset)...
			.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient)
			.setQueueFamilyIndex(vulkan_device.get_graphics_and_compute_queue_family());
	d_graphics_and_compute_command_pool = vulkan_device.get_device().createCommandPool(graphics_and_compute_command_pool_create_info);

	// Set render pass for rendering to the default framebuffer.
	d_default_render_pass = default_render_pass;

	//
	// Allocate a command buffer, for each asynchronous frame, for recording within default render pass.
	//
	vk::CommandBufferAllocateInfo default_render_pass_command_buffer_allocate_info;
	default_render_pass_command_buffer_allocate_info
			.setCommandPool(d_graphics_and_compute_command_pool)
			.setLevel(vk::CommandBufferLevel::ePrimary)
			.setCommandBufferCount(GPlatesOpenGL::Vulkan::NUM_ASYNC_FRAMES);
	const std::vector<vk::CommandBuffer> default_render_pass_command_buffers =
			vulkan_device.get_device().allocateCommandBuffers(default_render_pass_command_buffer_allocate_info);
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			default_render_pass_command_buffers.size() == GPlatesOpenGL::Vulkan::NUM_ASYNC_FRAMES,
			GPLATES_ASSERTION_SOURCE);
	for (unsigned int frame_index = 0; frame_index < GPlatesOpenGL::Vulkan::NUM_ASYNC_FRAMES; ++frame_index)
	{
		d_default_render_pass_command_buffers[frame_index] = default_render_pass_command_buffers[frame_index];
	}

	// Initialise the background stars.
	d_stars.initialise_vulkan_resources(vulkan_device, default_render_pass);

#if 0
	// Create the shader program that sorts and blends the list of fragments (per pixel) in depth order.
	create_sort_and_blend_scene_fragments_shader_program(gl);

	// Create buffer for fragment list allocator atomic counter.
	create_fragment_list_allocator_atomic_counter_buffer(gl);
	// Note: We don't create the fragment list buffer and image yet.
	//       That happens when they are first used, and also when viewport is resized to be larger than them.

	// Create full-screen quad.
	d_full_screen_quad = GPlatesOpenGL::GLUtils::create_full_screen_quad(gl);

	// Create and initialise the offscreen render target.
	create_off_screen_render_target(gl);
#endif
}


void
GPlatesGui::SceneRenderer::release_vulkan_resources(
		GPlatesOpenGL::VulkanDevice &vulkan_device)
{
#if 0
	// Destroy the offscreen render target.
	destroy_off_screen_render_target(gl);

	// Destroy the full-screen quad.
	d_full_screen_quad.reset();

	// Destroy buffers/images containing the per-pixel lists of fragments rendered into the scene.
	destroy_fragment_list_resources(gl);

	// Destroy the shader program that sorts and blends the list of fragments (per pixel) in depth order..
	d_sort_and_blend_scene_fragments_shader_program.reset();
#endif

	// Destroy the background stars.
	//d_stars.release_vulkan_resources(vulkan_device);

	// Free the default render pass command buffers.
	for (unsigned int frame_index = 0; frame_index < GPlatesOpenGL::Vulkan::NUM_ASYNC_FRAMES; ++frame_index)
	{
		// Free command buffer.
		vulkan_device.get_device().freeCommandBuffers(
				d_graphics_and_compute_command_pool,
				d_default_render_pass_command_buffers[frame_index]);
		d_default_render_pass_command_buffers[frame_index] = nullptr;
	}

	// Reset default render pass.
	d_default_render_pass = nullptr;

	// Destroy the graphics+compute command pool.
	vulkan_device.get_device().destroyCommandPool(d_graphics_and_compute_command_pool);
	d_graphics_and_compute_command_pool = nullptr;
}


void
GPlatesGui::SceneRenderer::render(
		GPlatesOpenGL::Vulkan &vulkan,
		vk::Framebuffer default_frame_buffer,
		Scene &scene,
		SceneOverlays &scene_overlays,
		const SceneView &scene_view,
		const GPlatesOpenGL::GLViewport &viewport,
		int device_pixel_ratio)
{
	const GPlatesOpenGL::GLViewProjection view_projection = scene_view.get_view_projection(viewport);

	// Access the command buffer for use in the current asynchronous frame.
	vk::CommandBuffer default_render_pass_command_buffer = d_default_render_pass_command_buffers[vulkan.get_frame_index()];
	// Let 'vulkan' know which command buffer is used to render into default framebuffer
	// (so it can submit it to the graphics+compute queue at the end of the frame).
	vulkan.set_default_render_pass_command_buffer(default_render_pass_command_buffer);

	//
	// Begin recording into the command buffer for the default render pass.
	//
	vk::CommandBufferBeginInfo command_buffer_begin_info;
	// Command buffer will only be submitted once.
	command_buffer_begin_info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	// Begin implicitly resets the command buffer (because command pool was created with 'eResetCommandBuffer' flag).
	default_render_pass_command_buffer.begin(command_buffer_begin_info);


	//
	// Default render pass (for rendering to default framebuffer).
	//
	// Clear colour of the default framebuffer.
	//
	// Note that we clear the colour to (0,0,0,1) and not (0,0,0,0) because we want any parts of
	// the scene, that are not rendered, to have *opaque* alpha (=1). This ensures that if there is any
	// further alpha composition of the framebuffer (we attempt to set vk::CompositeAlphaFlagBitsKHR::eOpaque
	// in VulkanSwapchain, if it's supported, to avoid further composition) then it will have no effect.
	// For example, we don't want our black background converted to white if the underlying surface happens to be white.
	const vk::ClearColorValue default_clear_colour = std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f };
	// Clear depth/stencil of the default framebuffer.
	const vk::ClearDepthStencilValue default_clear_depth_stencil = { 1.0f, 0 };

	// Begin default render pass to the current default framebuffer.
	const vk::Rect2D default_render_area = viewport.get_rect2D();
	const vk::ClearValue default_clear_values[2] = { default_clear_colour, default_clear_depth_stencil };
	vk::RenderPassBeginInfo default_render_pass_begin_info;
	default_render_pass_begin_info
			.setRenderPass(d_default_render_pass)
			.setFramebuffer(default_frame_buffer)
			.setRenderArea(default_render_area)
			.setClearValues(default_clear_values);
	default_render_pass_command_buffer.beginRenderPass(default_render_pass_begin_info, vk::SubpassContents::eInline);

	// Hold onto the previous frame's cached resources *while* generating the current frame.
	//
	// NOTE: We hold onto the previous frame's cached resources *while* generating the current frame
	// and then release our hold on the previous frame (by assigning the current frame's cache).
	// This just prevents a render frame from invalidating cached resources of the previous frame
	// in order to avoid regenerating the same cached resources unnecessarily each frame.
	// Since the view direction usually differs little from one frame to the next there is a lot
	// of overlap that we want to reuse (and not recalculate).
	d_gl_frame_cache_handle = render_scene(
			vulkan,
			scene,
			scene_overlays,
			scene_view,
			view_projection,
			device_pixel_ratio);

	// End default render pass.
	default_render_pass_command_buffer.endRenderPass();

	// End recording into default command buffer.
	default_render_pass_command_buffer.end();
}


void
GPlatesGui::SceneRenderer::render_to_image(
		QImage &image,
		GPlatesOpenGL::VulkanDevice &vulkan_device,
		GPlatesOpenGL::VulkanFrame &vulkan_frame,
		Scene &scene,
		SceneOverlays &scene_overlays,
		const SceneView &scene_view,
		const Colour &image_clear_colour)
{
	const GPlatesOpenGL::GLViewport image_viewport(
			0, 0,
			// Image size is in device pixels (as used by OpenGL)...
			image.width(), image.height());

	// The border is half the point size or line width, rounded up to nearest pixel.
	// TODO: Use the actual maximum point size or line width to calculate this.
	const unsigned int image_tile_border = 10;
	// Set up for rendering the scene into tiles using the offscreen render target.
	GPlatesOpenGL::GLTileRender image_tile_render(
			d_off_screen_render_target_dimension/*tile_render_target_width*/,
			d_off_screen_render_target_dimension/*tile_render_target_height*/,
			image_viewport/*destination_viewport*/,
			image_tile_border);

	// Keep track of the cache handles of all rendered tiles.
	boost::shared_ptr< std::vector<cache_handle_type> > frame_cache_handle(
			new std::vector<cache_handle_type>());

	// Render the scene tile-by-tile.
	for (image_tile_render.first_tile(); !image_tile_render.finished(); image_tile_render.next_tile())
	{
		// Render the scene to current image tile.
		// Hold onto the previous frame's cached resources *while* generating the current frame.
		const cache_handle_type image_tile_cache_handle = render_scene_tile_to_image(
				vulkan_device, vulkan_frame, image, image_viewport, image_tile_render,
				scene, scene_overlays, scene_view,
				image_clear_colour);
		frame_cache_handle->push_back(image_tile_cache_handle);
	}

	// The previous cached resources were kept alive *while* in the rendering loop above.
	d_gl_frame_cache_handle = frame_cache_handle;
}


GPlatesGui::SceneRenderer::cache_handle_type
GPlatesGui::SceneRenderer::render_scene_tile_to_image(
		GPlatesOpenGL::VulkanDevice &vulkan_device,
		GPlatesOpenGL::VulkanFrame &vulkan_frame,
		QImage &image,
		const GPlatesOpenGL::GLViewport &image_viewport,
		const GPlatesOpenGL::GLTileRender &image_tile_render,
		Scene &scene,
		SceneOverlays &scene_overlays,
		const SceneView &scene_view,
		const Colour &image_clear_colour)
{
	GPlatesOpenGL::GLViewport image_tile_render_target_viewport;
	image_tile_render.get_tile_render_target_viewport(image_tile_render_target_viewport);

	GPlatesOpenGL::GLViewport image_tile_render_target_scissor_rect;
	image_tile_render.get_tile_render_target_scissor_rectangle(image_tile_render_target_scissor_rect);

#if 0
	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GL::StateScope save_restore_state(
			gl,
			// We're rendering to a render target so reset to the default OpenGL state...
			true/*reset_to_default_state*/);

	// Bind our offscreen framebuffer object for drawing and reading.
	// This directs drawing to and reading from the offscreen colour renderbuffer at the first colour attachment, and
	// its associated depth/stencil renderbuffer at the depth/stencil attachment.
	gl.BindFramebuffer(GL_FRAMEBUFFER, d_off_screen_framebuffer);

	// Mask off rendering outside the current tile region in case the tile is smaller than the
	// render target. Note that the tile's viewport is slightly larger than the tile itself
	// (the scissor rectangle) in order that fat points and wide lines just outside the tile
	// have pixels rasterised inside the tile (the projection transform has also been expanded slightly).
	//
	// This includes 'glClear()' calls which are bounded by the scissor rectangle.
	gl.Enable(GL_SCISSOR_TEST);
	gl.Scissor(
			image_tile_render_target_scissor_rect.x(),
			image_tile_render_target_scissor_rect.y(),
			image_tile_render_target_scissor_rect.width(),
			image_tile_render_target_scissor_rect.height());
	gl.Viewport(
			image_tile_render_target_viewport.x(),
			image_tile_render_target_viewport.y(),
			image_tile_render_target_viewport.width(),
			image_tile_render_target_viewport.height());

	// Clear the colour buffer of the framebuffer currently bound to GL_DRAW_FRAMEBUFFER target using the requested clear colour.
	//
	// Note: We don't use depth or stencil buffers.
	gl.ClearColor(image_clear_colour.red(), image_clear_colour.green(), image_clear_colour.blue(), image_clear_colour.alpha());
	gl.Clear(GL_COLOR_BUFFER_BIT);

#endif

	// The view/projection/viewport for the *entire* image.
	const GPlatesOpenGL::GLViewProjection image_view_projection = scene_view.get_view_projection(image_viewport);

	// Projection transform associated with current image tile will be post-multiplied with the
	// projection transform for the whole image.
	GPlatesOpenGL::GLMatrix image_tile_projection_transform =
			image_tile_render.get_tile_projection_transform()->get_matrix();
	image_tile_projection_transform.gl_mult_matrix(image_view_projection.get_projection_transform());

	// Note: The view transform is unaffected by the tile (only the projection transform is affected).
	const GPlatesOpenGL::GLMatrix image_tile_view_transform = image_view_projection.get_view_transform();

	// The view/projection/viewport for the current image tile.
	const GPlatesOpenGL::GLViewProjection image_tile_view_projection(
			image_tile_render_target_viewport,  // viewport for the image *tile*
			image_tile_view_transform,
			image_tile_projection_transform);

	vk::Fence frame_rendering_finished_fence = vulkan_frame.next_frame(vulkan_device.get_device());

	GPlatesOpenGL::Vulkan vulkan(vulkan_device, vulkan_frame);

	//
	// Render the scene.
	//
	const cache_handle_type tile_cache_handle = render_scene(
			vulkan, scene, scene_overlays, scene_view,
			image_tile_view_projection,
			image.devicePixelRatio());

	//
	// Copy the rendered tile into the appropriate sub-rect of the image.
	//

	GPlatesOpenGL::GLViewport current_tile_source_viewport;
	image_tile_render.get_tile_source_viewport(current_tile_source_viewport);

	GPlatesOpenGL::GLViewport current_tile_destination_viewport;
	image_tile_render.get_tile_destination_viewport(current_tile_destination_viewport);

#if 0
	GPlatesOpenGL::GLImageUtils::copy_rgba8_framebuffer_into_argb32_qimage(
			gl,
			image,
			current_tile_source_viewport,
			current_tile_destination_viewport);
#endif

	return tile_cache_handle;
}


GPlatesGui::SceneRenderer::cache_handle_type
GPlatesGui::SceneRenderer::render_scene(
		GPlatesOpenGL::Vulkan &vulkan,
		Scene &scene,
		SceneOverlays &scene_overlays,
		const SceneView &scene_view,
		const GPlatesOpenGL::GLViewProjection &view_projection,
		int device_pixel_ratio)
{
#if 1

	//
	// Render the background stars.
	//
	if (d_view_state.get_show_stars())
	{
		/*
		d_stars.render(
				vulkan,
				view_projection,
				device_pixel_ratio,
				scene_view.is_map_active()
						// Expand the star positions radially in the 2D map views so that they're outside the map bounding sphere...
						? d_map_projection.get_map_bounding_radius()
						// The default of 1.0 works for the 3D globe view...
						: 1.0);
		*/
	}

#if 0
	//
	// Prepare for rendering the scene.
	//
	// After this, all objects rendered into the scene will add fragments to per-pixel fragment lists.

	// Create the fragment list storage buffer and head pointer image.
	//
	// Note: They only gets (re)created if either (1) not yet created or (2) the current viewport exceeds the buffer/image dimensions.
	create_fragment_list_storage_buffer_and_head_pointer_image(gl, view_projection.get_viewport());

	// Clear the per-pixel fragment list head pointers (we start with empty per-pixel lists).
	//
	// Note that we don't also need to clear the storage buffer containing the fragments because we're just overwriting them from scratch.
	const GLuint fragment_list_null_pointer = 0xffffffff;
	gl.ClearTexImage(d_fragment_list_head_pointer_image,
			0/*level*/, GL_RED_INTEGER, GL_UNSIGNED_INT, &fragment_list_null_pointer);

	// Clear the counter of the fragment list allocator.
	const GLuint fragment_list_allocator_initial_count = 0;
	gl.ClearNamedBufferData(d_fragment_list_allocator_atomic_counter_buffer,
			GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &fragment_list_allocator_initial_count);

	// Bind the fragment list allocator atomic counter buffer.
	gl.BindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0/*index*/, d_fragment_list_allocator_atomic_counter_buffer);

	// Bind the fragment list storage buffer.
	gl.BindBufferBase(GL_SHADER_STORAGE_BUFFER, 0/*index*/, d_fragment_list_storage_buffer);

	// Bind the fragment list head pointer image.
	gl.BindImageTexture(0, d_fragment_list_head_pointer_image, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32UI);
#endif

	//
	// Render the scene.
	//
	// All objects rendered into the scene will add fragments to per-pixel fragment lists instead of
	// rendering directly to the framebuffer.
	cache_handle_type frame_cache_handle;

#if 0
	// Bind the shader program that sorts and blends scene fragments accumulated from rendering the scene and
	// renders them into the framebuffer.
	gl.UseProgram(d_sort_and_blend_scene_fragments_shader_program);

	// Draw a full screen quad to process all screen pixels.
	gl.BindVertexArray(d_full_screen_quad);
	gl.DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
#endif

#else

	gl.Clear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	//
	// Render the globe or map (and its contents) depending on whether the globe or map is currently active.
	//
	cache_handle_type frame_cache_handle;
	if (scene_view.is_globe_active())
	{
		frame_cache_handle = scene.render_globe(
				gl,
				view_projection,
				scene_view.get_viewport_zoom().zoom_factor(),
				scene_view.get_globe_camera_front_horizon_plane());
	}
	else
	{
		frame_cache_handle = scene.render_map(
				gl,
				view_projection,
				scene_view.get_viewport_zoom().zoom_factor());
	}

#endif

#if 0
	// Render the 2D overlays on top of the 3D scene just rendered.
	scene_overlays.render(gl, view_projection, device_pixel_ratio);
#endif

	return frame_cache_handle;
}


void
GPlatesGui::SceneRenderer::create_off_screen_render_target(
		GPlatesOpenGL::GL &gl)
{
	if (d_off_screen_render_target_dimension > gl.get_capabilities().gl_max_texture_size)
	{
		d_off_screen_render_target_dimension = gl.get_capabilities().gl_max_texture_size;
	}

	// Create the framebuffer and its renderbuffers.
	d_off_screen_colour_renderbuffer = GPlatesOpenGL::GLRenderbuffer::create(gl);
	d_off_screen_depth_stencil_renderbuffer = GPlatesOpenGL::GLRenderbuffer::create(gl);
	d_off_screen_framebuffer = GPlatesOpenGL::GLFramebuffer::create(gl);

	// Initialise offscreen colour renderbuffer.
	gl.BindRenderbuffer(GL_RENDERBUFFER, d_off_screen_colour_renderbuffer);
	gl.RenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, d_off_screen_render_target_dimension, d_off_screen_render_target_dimension);

	// Initialise offscreen depth/stencil renderbuffer.
	// Note that (in modern OpenGL) an OpenGL implementation is only *required* to provide stencil if a
	// depth/stencil format is requested, and furthermore GL_DEPTH24_STENCIL8 is a specified required format.
	gl.BindRenderbuffer(GL_RENDERBUFFER, d_off_screen_depth_stencil_renderbuffer);
	gl.RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, d_off_screen_render_target_dimension, d_off_screen_render_target_dimension);

	// Bind the framebuffer that we'll subsequently attach the renderbuffers to.
	gl.BindFramebuffer(GL_FRAMEBUFFER, d_off_screen_framebuffer);

	// Bind the colour renderbuffer to framebuffer's first colour attachment.
	gl.FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, d_off_screen_colour_renderbuffer);

	// Bind the depth/stencil renderbuffer to framebuffer's depth/stencil attachment.
	gl.FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, d_off_screen_depth_stencil_renderbuffer);

	const GLenum completeness = gl.CheckFramebufferStatus(GL_FRAMEBUFFER);
	GPlatesGlobal::Assert<GPlatesOpenGL::OpenGLException>(
			completeness == GL_FRAMEBUFFER_COMPLETE,
			GPLATES_ASSERTION_SOURCE,
			"Framebuffer not complete for rendering tiles in globe filled polygons.");
}


void
GPlatesGui::SceneRenderer::destroy_off_screen_render_target(
		GPlatesOpenGL::GL &gl)
{
	// Destroy the framebuffer's renderbuffers and then destroy the framebuffer itself.
	d_off_screen_framebuffer.reset();
	d_off_screen_colour_renderbuffer.reset();
	d_off_screen_depth_stencil_renderbuffer.reset();
}


void
GPlatesGui::SceneRenderer::create_sort_and_blend_scene_fragments_shader_program(
		GPlatesOpenGL::GL &gl)
{
	// Add this scope to the call stack trace printed if exception thrown in this scope (eg, failure to compile/link shader).
	TRACK_CALL_STACK();

	// Vertex shader source.
	GPlatesOpenGL::GLShaderSource vertex_shader_source;
	vertex_shader_source.add_code_segment_from_file(SORT_AND_BLEND_SCENE_FRAGMENTS_VERTEX_SHADER_SOURCE_FILE_NAME);

	// Vertex shader.
	GPlatesOpenGL::GLShader::shared_ptr_type vertex_shader = GPlatesOpenGL::GLShader::create(gl, GL_VERTEX_SHADER);
	vertex_shader->shader_source(gl, vertex_shader_source);
	vertex_shader->compile_shader(gl);

	// Fragment shader source.
	GPlatesOpenGL::GLShaderSource fragment_shader_source;
	fragment_shader_source.add_code_segment_from_file(SORT_AND_BLEND_SCENE_FRAGMENTS_FRAGMENT_SHADER_SOURCE_FILE_NAME);

	// Fragment shader.
	GPlatesOpenGL::GLShader::shared_ptr_type fragment_shader = GPlatesOpenGL::GLShader::create(gl, GL_FRAGMENT_SHADER);
	fragment_shader->shader_source(gl, fragment_shader_source);
	fragment_shader->compile_shader(gl);

	d_sort_and_blend_scene_fragments_shader_program = GPlatesOpenGL::GLProgram::create(gl);

	// Vertex-fragment program.
	d_sort_and_blend_scene_fragments_shader_program->attach_shader(gl, vertex_shader);
	d_sort_and_blend_scene_fragments_shader_program->attach_shader(gl, fragment_shader);
	d_sort_and_blend_scene_fragments_shader_program->link_program(gl);
}


void
GPlatesGui::SceneRenderer::create_fragment_list_storage_buffer_and_head_pointer_image(
		GPlatesOpenGL::GL &gl,
		const GPlatesOpenGL::GLViewport &viewport)
{
	// Check if our image dimensions contain the viewport.
	if (d_max_fragment_list_head_pointer_image_width < viewport.width() ||
		d_max_fragment_list_head_pointer_image_height < viewport.height())
	{
		// Expand the image dimensions to fit the viewport.
		if (d_max_fragment_list_head_pointer_image_width < viewport.width())
		{
			d_max_fragment_list_head_pointer_image_width = viewport.width();
		}
		if (d_max_fragment_list_head_pointer_image_height < viewport.height())
		{
			d_max_fragment_list_head_pointer_image_height = viewport.height();
		}

		// Destroy the current image (if it exists).
		d_fragment_list_head_pointer_image.reset();

		// Create the fragment list head pointer image.
		d_fragment_list_head_pointer_image = GPlatesOpenGL::GLTexture::create(gl, GL_TEXTURE_2D);
		// Allocate storage for the fragment list head pointer image.
		// This is a 2D image with dimensions that should match (or exceed) the current viewport dimensions.
		gl.TextureStorage2D(
				d_fragment_list_head_pointer_image,
				1/*levels*/,
				GL_R32UI,
				d_max_fragment_list_head_pointer_image_width, d_max_fragment_list_head_pointer_image_height);

		// Check there are no OpenGL errors.
		GPlatesOpenGL::GLUtils::check_gl_errors(gl, GPLATES_ASSERTION_SOURCE);
	}

	// Convert the viewport dimensions into the storage buffer space required for the fragment lists.
	GLsizeiptr viewport_fragment_list_storage_buffer_bytes =
			GLsizeiptr(MAX_AVERAGE_FRAGMENT_BYTES_PER_PIXEL) * viewport.width() * viewport.height();
	// Make sure we don't exceed GL_MAX_SHADER_STORAGE_BLOCK_SIZE (which has a minimum value of 128MB).
	if (viewport_fragment_list_storage_buffer_bytes > GLsizeiptr(gl.get_capabilities().gl_max_shader_storage_block_size))
	{
		viewport_fragment_list_storage_buffer_bytes = GLsizeiptr(gl.get_capabilities().gl_max_shader_storage_block_size);
	}

	// Check if our buffer has storage for enough pixels based on the viewport.
	if (d_max_fragment_list_storage_buffer_bytes < viewport_fragment_list_storage_buffer_bytes)
	{
		// Expand the buffer to support the viewport.
		d_max_fragment_list_storage_buffer_bytes = viewport_fragment_list_storage_buffer_bytes;

		// Destroy the current buffer (if it exists).
		d_fragment_list_storage_buffer.reset();

		// Create buffer for the per-pixel fragment lists.
		d_fragment_list_storage_buffer = GPlatesOpenGL::GLBuffer::create(gl);
		// Allocate storage for all the per-pixel fragment lists.
		gl.NamedBufferStorage(
				d_fragment_list_storage_buffer,
				d_max_fragment_list_storage_buffer_bytes,
				nullptr,
				0/*flags*/);

		// Check there are no OpenGL errors.
		GPlatesOpenGL::GLUtils::check_gl_errors(gl, GPLATES_ASSERTION_SOURCE);
	}
}


void
GPlatesGui::SceneRenderer::create_fragment_list_allocator_atomic_counter_buffer(
		GPlatesOpenGL::GL &gl)
{
	// Create buffer for the atomic counter (used when allocating from fragment list storage).
	d_fragment_list_allocator_atomic_counter_buffer = GPlatesOpenGL::GLBuffer::create(gl);
	// Allocate space for a single atomic counter used when allocating from fragment list storage.
	gl.NamedBufferStorage(
			d_fragment_list_allocator_atomic_counter_buffer,
			sizeof(GLuint),
			nullptr,
			0/*flags*/);

	// Check there are no OpenGL errors.
	GPlatesOpenGL::GLUtils::check_gl_errors(gl, GPLATES_ASSERTION_SOURCE);
}


void
GPlatesGui::SceneRenderer::destroy_fragment_list_resources(
		GPlatesOpenGL::GL &gl)
{
	d_fragment_list_allocator_atomic_counter_buffer.reset();
	d_fragment_list_storage_buffer.reset();
	d_fragment_list_head_pointer_image.reset();

	// Make sure the buffer and image get recreated the first time they are needed again.
	d_max_fragment_list_head_pointer_image_width = 0;
	d_max_fragment_list_head_pointer_image_height = 0;
	d_max_fragment_list_storage_buffer_bytes = 0;
}
