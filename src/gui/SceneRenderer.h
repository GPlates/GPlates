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

#ifndef GPLATES_GUI_SCENERENDERER_H
#define GPLATES_GUI_SCENERENDERER_H

#include <boost/shared_ptr.hpp>
#include <QImage>

#include "opengl/GLFramebuffer.h"
#include "opengl/GLRenderbuffer.h"
#include "opengl/MapProjectionImage.h"
#include "opengl/RenderedGeometryRenderer.h"
#include "opengl/SceneTile.h"
#include "opengl/Stars.h"
#include "opengl/Vulkan.h"

#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	class GLTileRender;
	class GLViewport;
	class GLViewProjection;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesGui
{
	class Colour;
	class MapProjection;
	class SceneOverlays;
	class SceneView;

	/**
	 * Render the scene in the scene's view (globe and map views).
	 */
	class SceneRenderer :
			public GPlatesUtils::ReferenceCount<SceneRenderer>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<SceneRenderer> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const SceneRenderer> non_null_ptr_to_const_type;


		/**
		 * Typedef for an opaque object that caches a particular rendering.
		 */
		typedef boost::shared_ptr<void> cache_handle_type;


		/**
		 * Creates a new @a SceneRenderer object.
		 */
		static
		non_null_ptr_type
		create(
				GPlatesPresentation::ViewState &view_state)
		{
			return non_null_ptr_type(new SceneRenderer(view_state));
		}


		/**
		 * The Vulkan device was just created.
		 *
		 * The initialisation command buffer and fence can be used to submit initialisation commands to the graphics+compute queue.
		 * If the command buffer is submitted then wait for the fence (and reset it for the next client that uses it).
		 */
		void
		initialise_vulkan_resources(
				GPlatesOpenGL::Vulkan &vulkan,
				vk::RenderPass default_render_pass,
				vk::SampleCountFlagBits default_render_pass_sample_count,
				vk::CommandBuffer initialisation_command_buffer,
				vk::Fence initialisation_submit_fence);

		/**
		 * The Vulkan device is about to be destroyed.
		 */
		void
		release_vulkan_resources(
				GPlatesOpenGL::Vulkan &vulkan);


		/**
		 * Render the scene into the currently bound framebuffer.
		 */
		void
		render(
				GPlatesOpenGL::Vulkan &vulkan,
				vk::CommandBuffer preprocess_command_buffer,
				vk::CommandBuffer default_render_pass_command_buffer,
				SceneOverlays &scene_overlays,
				const SceneView &scene_view,
				const GPlatesOpenGL::GLViewport &viewport,
				int device_pixel_ratio);

		/**
		 * Render the scene into the specified image.
		 */
		void
		render_to_image(
				QImage &image,
				GPlatesOpenGL::Vulkan &vulkan,
				SceneOverlays &scene_overlays,
				const SceneView &scene_view,
				const Colour &image_clear_colour);


		/**
		 * Returns the OpenGL layers used to filled polygons, render rasters and scalar fields.
		 */
		GPlatesOpenGL::GLVisualLayers::non_null_ptr_type
		get_gl_visual_layers()
		{
			return d_rendered_geometry_renderer.get_gl_visual_layers();
		}

	private:

		GPlatesPresentation::ViewState &d_view_state;

		//! Render the scene in tiles using order-independent-transparency.
		GPlatesOpenGL::SceneTile d_scene_tile;

		MapProjection &d_map_projection;
		//! Image that maps latitude/longitude to map projection space.
		GPlatesOpenGL::MapProjectionImage d_map_projection_image;

		//! Draw the rendered geometries in the layers of the rendered geometry collection.
		GPlatesOpenGL::RenderedGeometryRenderer d_rendered_geometry_renderer;

		//! Stars in the background (in globe and map views).
		GPlatesOpenGL::Stars d_stars;

		//! Colour renderbuffer object used for offscreen rendering.
		GPlatesOpenGL::GLRenderbuffer::shared_ptr_type d_off_screen_colour_renderbuffer;

		//! Depth/stencil renderbuffer object used for offscreen rendering.
		GPlatesOpenGL::GLRenderbuffer::shared_ptr_type d_off_screen_depth_stencil_renderbuffer;

		//! Framebuffer object used for offscreen rendering.
		GPlatesOpenGL::GLFramebuffer::shared_ptr_type d_off_screen_framebuffer;

		//! Dimensions of square render target used for offscreen rendering.
		unsigned int d_off_screen_render_target_dimension;

		/**
		 * Enables frame-to-frame caching of persistent OpenGL resources.
		 *
		 * There is a certain amount of caching without this already.
		 * This just prevents a render frame from invalidating cached resources of the previous frame
		 * in order to avoid regenerating the same cached resources unnecessarily each frame.
		 * We hold onto the previous frame's cached resources *while* generating the current frame and
		 * then release our hold on the previous frame (and continue this pattern each new frame).
		 */
		cache_handle_type d_gl_frame_cache_handle;


		//! Dimensions of square render target used for offscreen rendering.
		static const unsigned int OFF_SCREEN_RENDER_TARGET_DIMENSION = 1024;


		explicit
		SceneRenderer(
				GPlatesPresentation::ViewState &view_state);

		//! Create and initialise the framebuffer and its renderbuffers used for offscreen rendering.
		void
		create_off_screen_render_target(
				GPlatesOpenGL::GL &gl);

		//! Destroy the framebuffer and its renderbuffers used for offscreen rendering.
		void
		destroy_off_screen_render_target(
				GPlatesOpenGL::GL &gl);


		/**
		 * Render one tile of the scene (as specified by @a image_tile_render).
		 *
		 * The sub-rect of @a image to render into is determined by @a image_tile_render.
		 */
		cache_handle_type
		render_scene_tile_to_image(
				GPlatesOpenGL::Vulkan &vulkan,
				QImage &image,
				const GPlatesOpenGL::GLViewport &image_viewport,
				const GPlatesOpenGL::GLTileRender &image_tile_render,
				SceneOverlays &scene_overlays,
				const SceneView &scene_view,
				const Colour &image_clear_colour);

		//! Render the scene into the current framebuffer using the specified view-projection.
		cache_handle_type
		render_scene(
				GPlatesOpenGL::Vulkan &vulkan,
				vk::CommandBuffer preprocess_command_buffer,
				vk::CommandBuffer default_render_pass_command_buffer,
				SceneOverlays &scene_overlays,
				const SceneView &scene_view,
				const GPlatesOpenGL::GLViewProjection &view_projection,
				int device_pixel_ratio);
	};
}

#endif // GPLATES_GUI_SCENERENDERER_H
