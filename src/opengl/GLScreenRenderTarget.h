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

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <opengl/OpenGL.h>

#include "GLRenderTargetImpl.h"


namespace GPlatesOpenGL
{
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
	class GLScreenRenderTarget :
			private boost::noncopyable
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
		 * Returns true if the texture internal format and optional depth/stencil buffer combination are
		 * supported by the runtime system (also requires support for GL_EXT_framebuffer_object).
		 *
		 * Also requires support for non-power-of-two textures since the screen dimensions can
		 * change and are unlikely to be a power-of-two.
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


		/**
		 * Creates a shared pointer to a @a GLScreenRenderTarget object.
		 *
		 * Creates the texture and optional depth/stencil buffer resources but doesn't allocate them yet.
		 *
		 * @a texture_internalformat is the same parameter used for 'GLTexture::gl_tex_image_2D()'.
		 */
		static
		shared_ptr_type
		create(
				GLRenderer &renderer,
				GLint texture_internalformat,
				bool include_depth_buffer,
				bool include_stencil_buffer)
		{
			return shared_ptr_type(
					new GLScreenRenderTarget(
							renderer, texture_internalformat, include_depth_buffer, include_stencil_buffer));
		}

		/**
		 * Same as @a create but returns a std::auto_ptr - to guarantee only one owner.
		 */
		static
		std::auto_ptr<GLScreenRenderTarget>
		create_as_auto_ptr(
				GLRenderer &renderer,
				GLint texture_internalformat,
				bool include_depth_buffer,
				bool include_stencil_buffer)
		{
			return std::auto_ptr<GLScreenRenderTarget>(
					new GLScreenRenderTarget(
							renderer, texture_internalformat, include_depth_buffer, include_stencil_buffer));
		}


		/**
		 * Ensures internal texture (and optional depth buffer) have a storage allocation of the
		 * specified dimensions and binds the internal framebuffer object for rendering to them.
		 *
		 * NOTE: The framebuffer object (if any) that is currently bound will be re-bound when
		 * @a end_render is called.
		 */
		void
		begin_render(
				GLRenderer &renderer,
				unsigned int render_target_width,
				unsigned int render_target_height);


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
		 * RAII class to call @a begin_render and @a end_render over a scope.
		 */
		class RenderScope :
				private boost::noncopyable
		{
		public:
			RenderScope(
					GLScreenRenderTarget &screen_render_target,
					GLRenderer &renderer,
					unsigned int render_target_width,
					unsigned int render_target_height);

			~RenderScope();

			//! Opportunity to end rendering before the scope exits (when destructor is called).
			void
			end_render();

		private:
			GLScreenRenderTarget &d_screen_render_target;
			GLRenderer &d_renderer;
			bool d_called_end_render;
		};


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
		 * The render target implementation.
		 */
		GLRenderTargetImpl d_impl;

		GLScreenRenderTarget(
				GLRenderer &renderer,
				GLint texture_internalformat,
				bool include_depth_buffer,
				bool include_stencil_buffer);

	};
}

#endif // GPLATES_OPENGL_GLSCREENRENDERTARGET_H
