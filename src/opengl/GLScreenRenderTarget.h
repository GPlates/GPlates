/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_OPENGL_GLSCREENRENDERTARGET_H
#define GPLATES_OPENGL_GLSCREENRENDERTARGET_H

#include <map>
#include <vector>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>
#include <opengl/OpenGL.h>

#include "GLFrameBufferObject.h"
#include "GLRenderBufferObject.h"
#include "GLTexture.h"


namespace GPlatesOpenGL
{
	class GLContext;
	class GLRenderer;

	/**
	 * Used to render to a screen-size texture (with optional associated hardware depth buffer).
	 *
	 * Your rendering is done between @a begin_render and @a end_render.
	 *
	 * NOTE: While native framebuffer objects in OpenGL cannot be shared across contexts,
	 * the @a GLScreenRenderTarget wrapper can (because internally it creates a framebuffer object
	 * for each context that it encounters - that uses it).
	 * So you can freely use it in different OpenGL contexts.
	 * This enables sharing of the associated texture and renderbuffer (which are shareable across contexts).
	 */
	class GLScreenRenderTarget
	{
	public:

		//
		// Note that the reason boost::shared_ptr is used instead of non_null_intrusive_ptr
		// is so these objects can be used with GPlatesUtils::ObjectCache.
		//

		//! A convenience typedef for a shared pointer to a @a GLScreenRenderTarget.
		typedef boost::shared_ptr<GLScreenRenderTarget> shared_ptr_type;
		typedef boost::shared_ptr<const GLScreenRenderTarget> shared_ptr_to_const_type;


		/**
		 * Returns true if supported by the runtime system.
		 *
		 * Requires support for framebuffer objects and non-power-of-two textures.
		 * The screen dimensions can change and are unlikely to be a power-of-two.
		 */
		static
		bool
		is_supported(
				GLRenderer &renderer,
				GLint texture_internalformat,
				bool include_depth_buffer);


		/**
		 * Creates a shared pointer to a @a GLScreenRenderTarget object.
		 *
		 * Creates the texture and optional depth buffer resources but doesn't allocate them yet.
		 *
		 * @a texture_internalformat is the same parameter used for 'GLTexture::gl_tex_image_2D()'.
		 */
		static
		shared_ptr_type
		create(
				GLRenderer &renderer,
				GLint texture_internalformat,
				bool include_depth_buffer)
		{
			return shared_ptr_type(
					new GLScreenRenderTarget(
							renderer, texture_internalformat, include_depth_buffer));
		}

		/**
		 * Same as @a create but returns a std::auto_ptr - to guarantee only one owner.
		 */
		static
		std::auto_ptr<GLScreenRenderTarget>
		create_as_auto_ptr(
				GLRenderer &renderer,
				GLint texture_internalformat,
				bool include_depth_buffer)
		{
			return std::auto_ptr<GLScreenRenderTarget>(
					new GLScreenRenderTarget(
							renderer, texture_internalformat, include_depth_buffer));
		}


		/**
		 * Ensures internal texture (and optional depth buffer) have a storage allocation of the
		 * specified dimensions and binds the internal framebuffer for rendering to them.
		 */
		void
		begin_render(
				GLRenderer &renderer,
				unsigned int render_target_width,
				unsigned int render_target_height);


		/**
		 * Unbinds the render texture target (binds to the main framebuffer).
		 *
		 * The render texture can now be retrieved using @a get_texture.
		 */
		void
		end_render(
				GLRenderer &renderer);


		/**
		 * Returns the render texture.
		 *
		 * The returned texture is 'const' so that its filtering parameters, for example, cannot be modified.
		 *
		 * Throws exception if called between @a begin_render and @a end_render.
		 * Because cannot use texture until finished rendering to it.
		 */
		GLTexture::shared_ptr_to_const_type
		get_texture() const;

	private:

		//! Typedef for a key made up of the parameters of parameters to @a is_supported.
		typedef boost::tuple<GLint, bool> supported_key_type;

		//! Typedef for a mapping of supported parameters (key) to boolean flag indicating supported.
		typedef std::map<supported_key_type, bool> supported_map_type;

		/**
		 * The framebuffer object state as currently set in each OpenGL context.
		 *
		 * Since framebuffer objects cannot be shared across OpenGL contexts, in contrast to
		 * textures and render buffers, we create a separate framebuffer object for each context.
		 *
		 * This makes it much easier for clients to share a GLScreenRenderTarget across contexts without
		 * having to worry about sharing the texture/renderbuffer and not sharing the framebuffer object.
		 */
		struct ContextObjectState
		{
			/**
			 * Constructor creates a new framebuffer object using the specified context.
			 */
			ContextObjectState(
					const GLContext &context_,
					GLRenderer &renderer_);

			/**
			 * The OpenGL context using our framebuffer object.
			 *
			 * NOTE: This should *not* be a shared pointer otherwise it'll create a cyclic shared reference.
			 */
			const GLContext *context;

			//! The framebuffer object created in a specific OpenGL context.
			GLFrameBufferObject::shared_ptr_type framebuffer;

			//! Whether the texture (and render buffer) have been attached to the framebuffer object yet.
			bool attached_to_framebuffer;
		};

		/**
		 * Typedef for a sequence of context object states.
		 *
		 * A 'vector' is fine since we're not expecting many OpenGL contexts so searches should be fast.
		 */
		typedef std::vector<ContextObjectState> context_object_state_seq_type;


		/**
		 * The vertex array object state for each context that we've encountered.
		 */
		context_object_state_seq_type d_context_object_states;

		GLTexture::shared_ptr_type d_texture;
		GLint d_texture_internalformat;
		boost::optional<GLRenderBufferObject::shared_ptr_type> d_depth_buffer;

		/**
		 * Is false if we've not yet allocated storage for the texture and depth buffer.
		 */
		bool d_allocated_storage;

		/**
		 * Is true if we're currently between @a begin_render and @a end_render.
		 */
		bool d_currently_rendering;


		GLScreenRenderTarget(
				GLRenderer &renderer,
				GLint texture_internalformat,
				bool include_depth_buffer);

		//! Returns the framebuffer associated with the OpenGL context used by the specified renderer.
		GLFrameBufferObject::shared_ptr_type
		get_frame_buffer_object(
				GLRenderer &renderer)
		{
			return get_object_state_for_current_context(renderer).framebuffer;
		}

		ContextObjectState &
		get_object_state_for_current_context(
				GLRenderer &renderer);
	};
}

#endif // GPLATES_OPENGL_GLSCREENRENDERTARGET_H
