/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
 * 
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
 
#ifndef GPLATES_OPENGL_GLRENDERTARGETTYPE_H
#define GPLATES_OPENGL_GLRENDERTARGETTYPE_H

#include "GLRenderTarget.h"
#include "GLRenderTargetManager.h"
#include "GLTexture.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	/**
	 * Interface for rendering to a render target (for now this is a destination colour buffer
	 * that is the target of OpenGL draw commands).
	 */
	class GLRenderTargetType :
			public GPlatesUtils::ReferenceCount<GLRenderTargetType>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLRenderTargetType.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLRenderTargetType> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLRenderTargetType.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLRenderTargetType> non_null_ptr_to_const_type;


		virtual
		~GLRenderTargetType()
		{  }


		/**
		 * Gets a render target based on the derived type of this class.
		 */
		virtual
		GLRenderTarget::non_null_ptr_type
		get_render_target(
				GLRenderTargetManager &render_target_manager) = 0;
	};


	/**
	 * A frame buffer render target type - used simply for rendering to the main frame buffer.
	 *
	 * Use this when rendering the scene - if you need to render to a texture (that in turn will
	 * later be used to render to the scene) then use @a GLTextureRenderTargetType instead.
	 * If you use @a GLTextureRenderTargetType and the system has no support for off-screen
	 * render targets then it will use the main framebuffer anyway (but it's taken care of for you).
	 */
	class GLFrameBufferRenderTargetType :
			public GLRenderTargetType
	{
	public:
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new GLFrameBufferRenderTargetType());
		}

		virtual
		GLRenderTarget::non_null_ptr_type
		get_render_target(
				GLRenderTargetManager &render_target_manager)
		{
			return render_target_manager.get_frame_buffer_render_target();
		}

	private:
		GLFrameBufferRenderTargetType()
		{  }
	};


	/**
	 * A texture render target type - used for rendering to a texture.
	 *
	 * It will use whatever support is has for rendering to a texture.
	 * If the system has no support for off-screen render targets then it will fall back
	 * on using the main framebuffer for rendering and then copying to a texture - and making sure
	 * the appropriate part of the main framebuffer (the part used for rendering to the texture)
	 * is restored - if the scene has been partially rendered to the main framebuffer already).
	 */
	class GLTextureRenderTargetType :
			public GLRenderTargetType
	{
	public:
		static
		non_null_ptr_type
		create(
				const GLTexture::shared_ptr_to_const_type &texture,
				unsigned int texture_width,
				unsigned int texture_height)
		{
			return non_null_ptr_type(new GLTextureRenderTargetType(texture, texture_width, texture_height));
		}

		virtual
		GLRenderTarget::non_null_ptr_type
		get_render_target(
				GLRenderTargetManager &render_target_manager)
		{
			// Get a render target of the correct dimensions for the texture.
			GLTextureRenderTarget::non_null_ptr_type render_target =
					render_target_manager.get_texture_render_target(d_texture_width, d_texture_height);

			// Attach the texture to the render target.
			render_target->attach_texture(d_texture);

			return render_target;
		}

	private:
		GLTexture::shared_ptr_to_const_type d_texture;
		unsigned int d_texture_width;
		unsigned int d_texture_height;

		explicit
		GLTextureRenderTargetType(
				const GLTexture::shared_ptr_to_const_type &texture,
				unsigned int texture_width,
				unsigned int texture_height) :
			d_texture(texture),
			d_texture_width(texture_width),
			d_texture_height(texture_height)
		{  }
	};
}

#endif // GPLATES_OPENGL_GLRENDERTARGETTYPE_H
