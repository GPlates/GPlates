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

#include "opengl/GLBuffer.h"
#include "opengl/GLFramebuffer.h"
#include "opengl/GLRenderbuffer.h"
#include "opengl/GLProgram.h"
#include "opengl/GLTexture.h"
#include "opengl/GLVertexArray.h"
#include "opengl/Stars.h"
#include "opengl/Vulkan.h"

#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	class GLTileRender;
	class GLViewport;
	class GLViewProjection;
	class VulkanDevice;
	class VulkanFrame;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesGui
{
	class Colour;
	class MapProjection;
	class Scene;
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
		 * Creates a new @a Scene object.
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
		 * The initialisation command buffer will be submitted to the graphics+compute queue upon return.
		 */
		void
		initialise_vulkan_resources(
				GPlatesOpenGL::VulkanDevice &vulkan_device,
				vk::RenderPass default_render_pass,
				vk::CommandBuffer initialisation_command_buffer);

		/**
		 * The Vulkan device is about to be destroyed.
		 */
		void
		release_vulkan_resources(
				GPlatesOpenGL::VulkanDevice &vulkan_device);


		/**
		 * Render the scene into the currently bound framebuffer.
		 */
		void
		render(
				GPlatesOpenGL::Vulkan &vulkan,
				vk::CommandBuffer default_render_pass_command_buffer,
				Scene &scene,
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
				GPlatesOpenGL::VulkanDevice &vulkan_device,
				GPlatesOpenGL::VulkanFrame &vulkan_frame,
				Scene &scene,
				SceneOverlays &scene_overlays,
				const SceneView &scene_view,
				const Colour &image_clear_colour);

	private:

		GPlatesPresentation::ViewState &d_view_state;
		MapProjection &d_map_projection;

		//! Stars in the background (in globe and map views).
		GPlatesOpenGL::Stars d_stars;

		//! Shader program that sorts and blends the list of fragments (per pixel) in depth order.
		GPlatesOpenGL::GLProgram::shared_ptr_type d_sort_and_blend_scene_fragments_shader_program;

		//! Image containing the per-pixel head of list of fragments rendered into the scene.
		GPlatesOpenGL::GLTexture::shared_ptr_type d_fragment_list_head_pointer_image;

		//! Buffer containing storage for the per-pixel lists of fragments rendered into the scene.
		GPlatesOpenGL::GLBuffer::shared_ptr_type d_fragment_list_storage_buffer;

		//! Buffer containing a single atomic counter used when allocating from fragment list storage.
		GPlatesOpenGL::GLBuffer::shared_ptr_type d_fragment_list_allocator_atomic_counter_buffer;

		/**
		 * The maximum image width and height supported by the current fragment list head pointer image.
		 *
		 * Initially these are both zero and are expanded as the viewport expands (eg, resized viewport).
		 */
		unsigned int d_max_fragment_list_head_pointer_image_width;
		unsigned int d_max_fragment_list_head_pointer_image_height;

		/**
		 * The maximum number of bytes to store fragments in the current fragment list storage buffer.
		 *
		 * Initially this is zero and is expanded as the viewport expands (eg, resized viewport).
		 */
		GLsizeiptr d_max_fragment_list_storage_buffer_bytes;

		//! Used to draw a full-screen quad.
		GPlatesOpenGL::GLVertexArray::shared_ptr_type d_full_screen_quad;

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


		//! Create the shader program that sorts and blends the list of fragments (per pixel) in depth order.
		void
		create_sort_and_blend_scene_fragments_shader_program(
				GPlatesOpenGL::GL &gl);

		//! Create storage buffer and head pointer image representing the per-pixel lists of fragments rendered into the scene.
		void
		create_fragment_list_storage_buffer_and_head_pointer_image(
				GPlatesOpenGL::GL &gl,
				const GPlatesOpenGL::GLViewport &viewport);

		//! Create the buffer containing a single atomic counter used when allocating from fragment list storage.
		void
		create_fragment_list_allocator_atomic_counter_buffer(
				GPlatesOpenGL::GL &gl);

		//! Destroy resources (buffers/images) used for the per-pixel lists of fragments rendered into the scene.
		void
		destroy_fragment_list_resources(
				GPlatesOpenGL::GL &gl);

		/**
		 * Render one tile of the scene (as specified by @a image_tile_render).
		 *
		 * The sub-rect of @a image to render into is determined by @a image_tile_render.
		 */
		cache_handle_type
		render_scene_tile_to_image(
				GPlatesOpenGL::VulkanDevice &vulkan_device,
				GPlatesOpenGL::VulkanFrame &vulkan_frame,
				QImage &image,
				const GPlatesOpenGL::GLViewport &image_viewport,
				const GPlatesOpenGL::GLTileRender &image_tile_render,
				Scene &scene,
				SceneOverlays &scene_overlays,
				const SceneView &scene_view,
				const Colour &image_clear_colour);

		//! Render the scene into the current framebuffer using the specified view-projection.
		cache_handle_type
		render_scene(
				GPlatesOpenGL::Vulkan &vulkan,
				vk::CommandBuffer default_render_pass_command_buffer,
				Scene &scene,
				SceneOverlays &scene_overlays,
				const SceneView &scene_view,
				const GPlatesOpenGL::GLViewProjection &view_projection,
				int device_pixel_ratio);
	};
}

#endif // GPLATES_GUI_SCENERENDERER_H
