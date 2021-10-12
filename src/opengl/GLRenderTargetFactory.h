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
 
#ifndef GPLATES_OPENGL_GLRENDERTARGETFACTORY_H
#define GPLATES_OPENGL_GLRENDERTARGETFACTORY_H

#include <QGLFramebufferObject>
#include <QGLPixelBuffer>

#include "GLRenderTarget.h"

#include "global/PointerTraits.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	class GLContext;

	/**
	 * Interface for creating render targets.
	 */
	class GLRenderTargetFactory :
			public GPlatesUtils::ReferenceCount<GLRenderTargetFactory>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLRenderTargetFactory.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLRenderTargetFactory> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLRenderTargetFactory.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLRenderTargetFactory> non_null_ptr_to_const_type;


		virtual
		~GLRenderTargetFactory()
		{  }


		/**
		 * Creates a render target for rendering to the main frame buffer.
		 */
		virtual
		GLFrameBufferRenderTarget::non_null_ptr_type
		create_frame_buffer_render_target(
				const GPlatesGlobal::PointerTraits<GLContext>::non_null_ptr_type &context) = 0;


		/**
		 * Creates a render target for rendering to a texture.
		 */
		virtual
		GLTextureRenderTarget::non_null_ptr_type
		create_texture_render_target(
				unsigned int texture_width,
				unsigned int texture_height) = 0;
	};



	////////////////////////////////////////
	// Frame buffer object implementation //
	////////////////////////////////////////


	class GLFrameBufferObjectRenderTargetFactory :
			public GLRenderTargetFactory
	{
	public:
		/**
		 * Returns true if GL_EXT_framebuffer_object is supported on the runtime system.
		 */
		static
		bool
		is_supported()
		{
			return QGLFramebufferObject::hasOpenGLFramebufferObjects();
		}

		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new GLFrameBufferObjectRenderTargetFactory());
		}

		virtual
		GLFrameBufferRenderTarget::non_null_ptr_type
		create_frame_buffer_render_target(
				const GPlatesGlobal::PointerTraits<GLContext>::non_null_ptr_type &/*context*/)
		{
			return GLFrameBufferObjectFrameBufferRenderTarget::create();
		}

		virtual
		GLTextureRenderTarget::non_null_ptr_type
		create_texture_render_target(
				unsigned int texture_width,
				unsigned int texture_height)
		{
			return GLFrameBufferObjectTextureRenderTarget::create(texture_width, texture_height);
		}	

	private:
		GLFrameBufferObjectRenderTargetFactory()
		{  }
	};



	//////////////////////////////
	// 'pbuffer' implementation //
	//////////////////////////////


	class GLPBufferRenderTargetFactory :
			public GLRenderTargetFactory
	{
	public:
		/**
		 * Returns true if the 'pbuffer' is supported on the runtime system.
		 */
		static
		bool
		is_supported()
		{
			return QGLPixelBuffer::hasOpenGLPbuffers();
		}

		static
		non_null_ptr_type
		create(
				QGLWidget *qgl_widget)
		{
			return non_null_ptr_type(new GLPBufferRenderTargetFactory(qgl_widget));
		}

		virtual
		GLFrameBufferRenderTarget::non_null_ptr_type
		create_frame_buffer_render_target(
				const GPlatesGlobal::PointerTraits<GLContext>::non_null_ptr_type &context)
		{
			return GLPBufferFrameBufferRenderTarget::create(context);
		}

		virtual
		GLTextureRenderTarget::non_null_ptr_type
		create_texture_render_target(
				unsigned int texture_width,
				unsigned int texture_height)
		{
			return GLPBufferTextureRenderTarget::create(texture_width, texture_height, d_qgl_widget);
		}	

	private:
		QGLWidget *d_qgl_widget;

		explicit
		GLPBufferRenderTargetFactory(
				QGLWidget *qgl_widget) :
			d_qgl_widget(qgl_widget)
		{  }
	};



	///////////////////////////////////////////////
	// Main frame buffer fallback implementation //
	///////////////////////////////////////////////


	class GLMainFrameBufferRenderTargetFactory :
			public GLRenderTargetFactory
	{
	public:
		/**
		 * Always have a window provided frame buffer so return true always.
		 */
		static
		bool
		is_supported()
		{
			return true;
		}

		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new GLMainFrameBufferRenderTargetFactory());
		}

		virtual
		GLFrameBufferRenderTarget::non_null_ptr_type
		create_frame_buffer_render_target(
				const GPlatesGlobal::PointerTraits<GLContext>::non_null_ptr_type &/*context*/)
		{
			return GLMainFrameBufferFrameBufferRenderTarget::create();
		}

		virtual
		GLTextureRenderTarget::non_null_ptr_type
		create_texture_render_target(
				unsigned int texture_width,
				unsigned int texture_height)
		{
			return GLMainFrameBufferTextureRenderTarget::create(texture_width, texture_height);
		}	

	private:
		GLMainFrameBufferRenderTargetFactory()
		{  }
	};
}

#endif // GPLATES_OPENGL_GLRENDERTARGETFACTORY_H
