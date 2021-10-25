/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#ifndef GPLATES_OPENGL_GLRENDERTARGETIMPL_H
#define GPLATES_OPENGL_GLRENDERTARGETIMPL_H

#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
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
	 * Implementation used to render to both fixed-size and screen-size textures
	 * (with optional associated hardware depth/stencil buffer).
	 *
	 * NOTE: While native framebuffer objects in OpenGL cannot be shared across contexts,
	 * the @a GLRenderTargetImpl wrapper can (because internally it creates a framebuffer object
	 * for each context that it encounters - that uses it).
	 * So you can freely use it in different OpenGL contexts.
	 * This enables sharing of the associated texture and renderbuffer (which are shareable across contexts).
	 */
	class GLRenderTargetImpl :
			private boost::noncopyable

	{
	public:

		/**
		 * Returns true if the texture internal format and optional depth/stencil buffer combination are
		 * supported by the runtime system (also requires support for GL_EXT_framebuffer_object).
		 *
		 * If @a include_stencil_buffer is true then GL_EXT_packed_depth_stencil is also required
		 * because, for the most part, consumer hardware only supports stencil for FBOs if it's
		 * packed in with depth.
		 */
		static
		bool
		is_supported(
				GLRenderer &renderer,
				GLint texture_internalformat,
				bool include_depth_buffer,
				bool include_stencil_buffer);


		GLRenderTargetImpl(
				GLRenderer &renderer,
				GLint texture_internalformat,
				bool include_depth_buffer,
				bool include_stencil_buffer);


		/**
		 * Ensures internal texture (and optional depth/stencil buffer) have a storage allocation
		 * of the specified dimensions.
		 *
		 * This must be called at least once before the first call to @a begin_render.
		 * It can be called anytime to change the render target dimensions except it cannot be
		 * called between @a begin_render and @a end_render.
		 */
		void
		set_render_target_dimensions(
				GLRenderer &renderer,
				unsigned int render_target_width,
				unsigned int render_target_height);


		/**
		 * Binds the internal framebuffer object for rendering to the internal texture and optional depth buffer.
		 *
		 * NOTE: The framebuffer object (if any) that is currently bound will be re-bound when
		 * @a end_render is called.
		 */
		void
		begin_render(
				GLRenderer &renderer);


		/**
		 * Binds the framebuffer object that was bound when @a begin_render was called, or the main
		 * framebuffer if no framebuffer object was bound.
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

		/**
		 * The framebuffer object state as currently set in each OpenGL context.
		 *
		 * Since framebuffer objects cannot be shared across OpenGL contexts, in contrast to
		 * textures and render buffers, we create a separate framebuffer object for each context.
		 *
		 * This makes it much easier for clients to share a GLRenderTargetImpl across contexts without
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
		 * Information kept during a @a begin_render / @a end_render pair.
		 */
		struct RenderInfo
		{
			explicit
			RenderInfo(
					boost::optional<GLFrameBufferObject::shared_ptr_to_const_type> previous_framebuffer_) :
				previous_framebuffer(previous_framebuffer_)
			{  }

			boost::optional<GLFrameBufferObject::shared_ptr_to_const_type> previous_framebuffer;
		};

		/**
		 * Information for a depth/stencil render buffer.
		 */
		struct RenderBuffer
		{
			explicit
			RenderBuffer(
					const GLRenderBufferObject::shared_ptr_type &render_buffer_,
					GLint internalformat_) :
				render_buffer(render_buffer_),
				internalformat(internalformat_)
			{  }

			GLRenderBufferObject::shared_ptr_type render_buffer;
			GLint internalformat;
		};


		/**
		 * The vertex array object state for each context that we've encountered.
		 */
		context_object_state_seq_type d_context_object_states;

		GLTexture::shared_ptr_type d_texture;
		GLint d_texture_internalformat;
		boost::optional<RenderBuffer> d_depth_buffer;
		boost::optional<RenderBuffer> d_stencil_buffer;

		/**
		 * Is false if we've not yet allocated storage for the texture and depth buffer.
		 */
		bool d_allocated_storage;

		/**
		 * Render information kept between @a begin_render and @a end_render.
		 */
		boost::optional<RenderInfo> d_current_render_info;


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

		bool
		is_currently_rendering() const
		{
			return d_current_render_info;
		}
	};
}

#endif // GPLATES_OPENGL_GLRENDERTARGETIMPL_H
