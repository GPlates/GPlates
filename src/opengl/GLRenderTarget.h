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
 
#ifndef GPLATES_OPENGL_GLRENDERTARGET_H
#define GPLATES_OPENGL_GLRENDERTARGET_H

#include <boost/scoped_ptr.hpp>
#include <QGLFramebufferObject>
#include <QGLPixelBuffer>

#include "GLTexture.h"

#include "global/PointerTraits.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	class GLContext;

	/**
	 * Interface for rendering to a render target (for now this is a destination colour buffer
	 * that is the target of OpenGL draw commands).
	 */
	class GLRenderTarget :
			public GPlatesUtils::ReferenceCount<GLRenderTarget>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLRenderTarget.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLRenderTarget> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLRenderTarget.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLRenderTarget> non_null_ptr_to_const_type;


		virtual
		~GLRenderTarget()
		{  }


		/**
		 * Binds the current render target.
		 *
		 * This only needs to be called when changing render targets.
		 */
		virtual
		void
		bind() = 0;


		/**
		 * Some derived classes require an unbind on the last render target instead
		 * of a bind on the next render target.
		 *
		 * This only needs to be called when changing render targets.
		 */
		virtual
		void
		unbind() = 0;


		/**
		 * Call before rendering to 'this' render target.
		 *
		 * Does any pre-rendering tasks (such as binding render target to a texture).
		 */
		virtual
		void
		begin_render_to_target() = 0;


		/**
		 * Call after rendering to 'this' render target (and before rendering to a new target).
		 *
		 * Does any post-rendering tasks (such as copying render target to a texture).
		 */
		virtual
		void
		end_render_to_target() = 0;
	};


	/**
	 * A render target that is simply used for drawing to the main frame buffer.
	 */
	class GLFrameBufferRenderTarget :
			public GLRenderTarget
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLFrameBufferRenderTarget.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLFrameBufferRenderTarget> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLFrameBufferRenderTarget.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLFrameBufferRenderTarget> non_null_ptr_to_const_type;
	};


	/**
	 * A render target for rendering to a texture.
	 */
	class GLTextureRenderTarget :
			public GLRenderTarget
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLTextureRenderTarget.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLTextureRenderTarget> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLTextureRenderTarget.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLTextureRenderTarget> non_null_ptr_to_const_type;


		/**
		 * Attach a texture to the render target.
		 */
		virtual
		void
		attach_texture(
				const GLTexture::shared_ptr_to_const_type &render_texture) = 0;
	};


	////////////////////////////////////////
	// Frame buffer object implementation //
	////////////////////////////////////////

	class GLFrameBufferObjectFrameBufferRenderTarget :
			public GLFrameBufferRenderTarget
	{
	public:
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new GLFrameBufferObjectFrameBufferRenderTarget());
		}

		virtual
		void
		bind()
		{  }

		virtual
		void
		unbind()
		{  }

		virtual
		void
		begin_render_to_target()
		{  }

		virtual
		void
		end_render_to_target()
		{  }

	private:
		GLFrameBufferObjectFrameBufferRenderTarget()
		{  }
	};


	class GLFrameBufferObjectTextureRenderTarget :
			public GLTextureRenderTarget
	{
	public:
		static
		non_null_ptr_type
		create(
				unsigned int width,
				unsigned int height)
		{
			return non_null_ptr_type(new GLFrameBufferObjectTextureRenderTarget(width, height));
		}

		virtual
		void
		bind();

		virtual
		void
		unbind();

		virtual
		void
		begin_render_to_target();

		virtual
		void
		end_render_to_target()
		{  }

		virtual
		void
		attach_texture(
				const GLTexture::shared_ptr_to_const_type &render_texture)
		{
			d_render_texture = render_texture;
		}

	private:
		boost::scoped_ptr<QGLFramebufferObject> d_frame_buffer_object;
		GLTexture::shared_ptr_to_const_type d_render_texture;

		GLFrameBufferObjectTextureRenderTarget(
				unsigned int width,
				unsigned int height);
	};


	////////////////////////////////////////
	// 'pbuffer' implementation //
	////////////////////////////////////////

	class GLPBufferFrameBufferRenderTarget :
			public GLFrameBufferRenderTarget
	{
	public:
		static
		non_null_ptr_type
		create(
				const GPlatesGlobal::PointerTraits<GLContext>::non_null_ptr_type &context)
		{
			return non_null_ptr_type(new GLPBufferFrameBufferRenderTarget(context));
		}

		virtual
		void
		bind();

		virtual
		void
		unbind()
		{  }

		virtual
		void
		begin_render_to_target()
		{  }

		virtual
		void
		end_render_to_target()
		{  }

	private:
		GPlatesGlobal::PointerTraits<GLContext>::non_null_ptr_type d_context;


		explicit
		GLPBufferFrameBufferRenderTarget(
				const GPlatesGlobal::PointerTraits<GLContext>::non_null_ptr_type &context) :
			d_context(context)
		{  }
	};


	class GLPBufferTextureRenderTarget :
			public GLTextureRenderTarget
	{
	public:
		static
		non_null_ptr_type
		create(
				unsigned int width,
				unsigned int height,
				QGLWidget *qgl_widget)
		{
			return non_null_ptr_type(new GLPBufferTextureRenderTarget(width, height, qgl_widget));
		}

		virtual
		void
		bind();

		virtual
		void
		unbind()
		{  }

		virtual
		void
		begin_render_to_target()
		{  }

		virtual
		void
		end_render_to_target();

		virtual
		void
		attach_texture(
				const GLTexture::shared_ptr_to_const_type &render_texture)
		{
			d_render_texture = render_texture;
		}

	private:
		boost::scoped_ptr<QGLPixelBuffer> d_pixel_buffer;
		GLTexture::shared_ptr_to_const_type d_render_texture;

		GLPBufferTextureRenderTarget(
				unsigned int width,
				unsigned int height,
				QGLWidget *qgl_widget);
	};


	///////////////////////////////////////////////
	// Main frame buffer fallback implementation //
	///////////////////////////////////////////////

	class GLMainFrameBufferFrameBufferRenderTarget :
			public GLFrameBufferRenderTarget
	{
	public:
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new GLMainFrameBufferFrameBufferRenderTarget());
		}

		virtual
		void
		bind()
		{  }

		virtual
		void
		unbind()
		{  }

		virtual
		void
		begin_render_to_target()
		{  }

		virtual
		void
		end_render_to_target()
		{  }

	private:
		GLMainFrameBufferFrameBufferRenderTarget()
		{  }
	};


	class GLMainFrameBufferTextureRenderTarget :
			public GLTextureRenderTarget
	{
	public:
		static
		non_null_ptr_type
		create(
				unsigned int width,
				unsigned int height)
		{
			return non_null_ptr_type(new GLMainFrameBufferTextureRenderTarget(width, height));
		}

		virtual
		void
		bind()
		{  }

		virtual
		void
		unbind()
		{  }

		virtual
		void
		begin_render_to_target()
		{  }

		virtual
		void
		end_render_to_target();

		virtual
		void
		attach_texture(
				const GLTexture::shared_ptr_to_const_type &render_texture)
		{
			d_render_texture = render_texture;
		}

	private:
		GLTexture::shared_ptr_to_const_type d_render_texture;
		unsigned int d_texture_width;
		unsigned int d_texture_height;


		GLMainFrameBufferTextureRenderTarget(
				unsigned int width,
				unsigned int height);
	};
}

#endif // GPLATES_OPENGL_GLRENDERTARGET_H
