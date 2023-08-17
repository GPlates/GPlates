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
#include "SceneOverlays.h"
#include "SceneView.h"
#include "ViewportZoom.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "opengl/GLImageUtils.h"
#include "opengl/GLMatrix.h"
#include "opengl/GLTileRender.h"
#include "opengl/GLUtils.h"
#include "opengl/GLViewProjection.h"
#include "opengl/OpenGL.h"
#include "opengl/OpenGLException.h"

#include "presentation/ViewState.h"

#include "utils/CallStackTracker.h"


GPlatesGui::SceneRenderer::SceneRenderer(
		GPlatesPresentation::ViewState &view_state) :
	d_view_state(view_state),
	d_map_projection(view_state.get_map_projection()),
	d_rendered_geometry_renderer(view_state),
	d_off_screen_render_target_dimension(OFF_SCREEN_RENDER_TARGET_DIMENSION)
{
}


void
GPlatesGui::SceneRenderer::initialise_vulkan_resources(
		GPlatesOpenGL::Vulkan &vulkan,
		vk::RenderPass default_render_pass,
		vk::SampleCountFlagBits default_render_pass_sample_count,
		vk::CommandBuffer initialisation_command_buffer,
		vk::Fence initialisation_submit_fence)
{
	// Initialise the scene tile.
	d_scene_tile.initialise_vulkan_resources(
			vulkan,
			default_render_pass,
			default_render_pass_sample_count,
			initialisation_command_buffer,
			initialisation_submit_fence);

	// Initialise the map projection image.
	d_map_projection_image.initialise_vulkan_resources(
			vulkan,
			initialisation_command_buffer,
			initialisation_submit_fence);

	// Initialise the background stars.
	d_stars.initialise_vulkan_resources(
			vulkan,
			default_render_pass,
			default_render_pass_sample_count,
			initialisation_command_buffer,
			initialisation_submit_fence);

	// Initialise rendered geometry renderer.
	d_rendered_geometry_renderer.initialise_vulkan_resources(
			vulkan,
			default_render_pass,
			default_render_pass_sample_count,
			d_scene_tile,
			d_map_projection_image,
			initialisation_command_buffer,
			initialisation_submit_fence);

#if 0
	// Create and initialise the offscreen render target.
	create_off_screen_render_target(gl);
#endif
}


void
GPlatesGui::SceneRenderer::release_vulkan_resources(
		GPlatesOpenGL::Vulkan &vulkan)
{
#if 0
	// Destroy the offscreen render target.
	destroy_off_screen_render_target(gl);
#endif

	// Release the rendered geometry renderer.
	d_rendered_geometry_renderer.release_vulkan_resources(vulkan);

	// Release the background stars.
	d_stars.release_vulkan_resources(vulkan);

	// Release the map projection image.
	d_map_projection_image.release_vulkan_resources(vulkan);

	// Release the scene tile.
	d_scene_tile.release_vulkan_resources(vulkan);
}


void
GPlatesGui::SceneRenderer::render(
		GPlatesOpenGL::Vulkan &vulkan,
		vk::CommandBuffer preprocess_command_buffer,
		vk::CommandBuffer default_render_pass_command_buffer,
		SceneOverlays &scene_overlays,
		const SceneView &scene_view,
		const GPlatesOpenGL::GLViewport &viewport,
		int device_pixel_ratio)
{
	const GPlatesOpenGL::GLViewProjection view_projection = scene_view.get_view_projection(viewport);

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
			preprocess_command_buffer,
			default_render_pass_command_buffer,
			scene_overlays,
			scene_view,
			view_projection,
			device_pixel_ratio);
}


void
GPlatesGui::SceneRenderer::render_to_image(
		QImage &image,
		GPlatesOpenGL::Vulkan &vulkan,
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
				vulkan, image, image_viewport, image_tile_render,
				scene_overlays, scene_view,
				image_clear_colour);
		frame_cache_handle->push_back(image_tile_cache_handle);
	}

	// The previous cached resources were kept alive *while* in the rendering loop above.
	d_gl_frame_cache_handle = frame_cache_handle;
}


GPlatesGui::SceneRenderer::cache_handle_type
GPlatesGui::SceneRenderer::render_scene_tile_to_image(
		GPlatesOpenGL::Vulkan &vulkan,
		QImage &image,
		const GPlatesOpenGL::GLViewport &image_viewport,
		const GPlatesOpenGL::GLTileRender &image_tile_render,
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

	vk::Fence frame_rendering_finished_fence = vulkan.next_frame();
	// TODO: Use 'vulkan_frame.get_frame_index()' to choose between NUM_ASYNC_FRAMES command buffers passed by caller.
	vk::CommandBuffer preprocess_command_buffer;
	vk::CommandBuffer default_render_pass_command_buffer;

	//
	// Render the scene.
	//
	const cache_handle_type tile_cache_handle = render_scene(
			vulkan,
			default_render_pass_command_buffer, preprocess_command_buffer,
			scene_overlays, scene_view,
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
		vk::CommandBuffer preprocess_command_buffer,
		vk::CommandBuffer default_render_pass_command_buffer,
		SceneOverlays &scene_overlays,
		const SceneView &scene_view,
		const GPlatesOpenGL::GLViewProjection &view_projection,
		int device_pixel_ratio)
{
	// If the map view is active then see if the map projection image needs updating
	// (if map projection changed in any way since the last update).
	if (scene_view.is_map_active())
	{
		d_map_projection_image.update(vulkan, preprocess_command_buffer, d_map_projection);
	}

	//
	// Render the background stars.
	//
	// Note: Stars are not part of order-independent-transparency and hence are not rendered to
	//       per-pixel fragment lists (like other 3D objects). This is because stars are always
	//       rendered into the background and hence don't need any depth sorting. Also the stars are
	//       rendered behind the far clip plane and, due to depth clamping, always have a depth of 1.0.
	//
	if (d_view_state.get_show_stars())
	{
		d_stars.render(
				vulkan,
				default_render_pass_command_buffer,
				view_projection,
				device_pixel_ratio,
				scene_view.is_map_active()
						// Expand the star positions radially in the 2D map views so that they're outside the map bounding sphere...
						? d_map_projection.get_map_bounding_radius()
						// The default of 1.0 works for the 3D globe view...
						: 1.0);
	}

	//
	// Prepare for rendering the scene to the scene tile.
	//
	// NOTE: This is done inside a render pass (since it uses graphics operations to perform the clear).
	//
	// After this, all objects rendered into the scene tile will add fragments to per-pixel fragment lists.
	d_scene_tile.clear(vulkan, default_render_pass_command_buffer, view_projection);

	//
	// Render the scene to the scene tile.
	//
	// All objects rendered into the scene will add fragments to per-pixel fragment lists instead of
	// rendering directly to the framebuffer.
	//
	const cache_handle_type frame_cache_handle = d_rendered_geometry_renderer.render(
			vulkan,
			preprocess_command_buffer,
			default_render_pass_command_buffer,
			view_projection,
			scene_view.get_viewport_zoom().zoom_factor(),
			// Render the globe or map (and its contents) depending on whether the globe or map is currently active...
			scene_view.is_map_active(),
			d_map_projection.central_meridian());  // only used if 'is_map_active' is true

	//
	// Render the scene tile.
	//
	// This blends the fragments in per-pixel linked lists into the framebuffer.
	d_scene_tile.render(vulkan, default_render_pass_command_buffer, view_projection);


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
