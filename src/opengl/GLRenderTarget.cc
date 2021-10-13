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

/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>
#include <opengl/OpenGL.h>
#include <QDebug>

#include "GLContext.h"
#include "GLRenderTarget.h"

#include "global/CompilerWarnings.h"


////////////////////////////////////////
// Frame buffer object implementation //
////////////////////////////////////////


GPlatesOpenGL::GLFrameBufferObjectTextureRenderTarget::GLFrameBufferObjectTextureRenderTarget(
		unsigned int width,
		unsigned int height) :
	d_frame_buffer_object(new QGLFramebufferObject(width, height))
{
}


// We use macros in <GL/glew.h> that contain old-style casts.
DISABLE_GCC_WARNING("-Wold-style-cast")

void
GPlatesOpenGL::GLFrameBufferObjectTextureRenderTarget::begin_render_to_target()
{
	if (d_render_texture)
	{
		if (GLEW_EXT_framebuffer_object)
		{
			// Attach to the texture.
			glFramebufferTexture2DEXT(
					GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D,
					d_render_texture.get()->get_texture_resource().get_resource(), 0);
		}
	}
}

ENABLE_GCC_WARNING("-Wold-style-cast")


void
GPlatesOpenGL::GLFrameBufferObjectTextureRenderTarget::bind()
{
	d_frame_buffer_object->bind();
}


void
GPlatesOpenGL::GLFrameBufferObjectTextureRenderTarget::unbind()
{
	// Release back to the main frame buffer.
	d_frame_buffer_object->release();
}

////////////////////////////////////////
// 'pbuffer' implementation //
////////////////////////////////////////

void
GPlatesOpenGL::GLPBufferFrameBufferRenderTarget::bind()
{
	d_context->make_current();
}


GPlatesOpenGL::GLPBufferTextureRenderTarget::GLPBufferTextureRenderTarget(
		unsigned int width,
		unsigned int height,
		QGLWidget *qgl_widget) :
	d_pixel_buffer(new QGLPixelBuffer(width, height, QGLFormat(QGL::AlphaChannel), qgl_widget))
{
     if (!d_pixel_buffer->format().alpha())
	 {
         qWarning("Could not get alpha channel on pbuffer render target; results will be suboptimal");
	 }
}


void
GPlatesOpenGL::GLPBufferTextureRenderTarget::bind()
{
	d_pixel_buffer->makeCurrent();
}


void
GPlatesOpenGL::GLPBufferTextureRenderTarget::end_render_to_target()
{
	// Copy the pbuffer contents to our texture.
	d_pixel_buffer->updateDynamicTexture(
			d_render_texture->get_texture_resource().get_resource());
}

///////////////////////////////////////////////
// Main frame buffer fallback implementation //
///////////////////////////////////////////////


GPlatesOpenGL::GLMainFrameBufferTextureRenderTarget::GLMainFrameBufferTextureRenderTarget(
		unsigned int width,
		unsigned int height) :
	d_texture_width(width),
	d_texture_height(height)
{
}


void
GPlatesOpenGL::GLMainFrameBufferTextureRenderTarget::end_render_to_target()
{
	// Bind the texture and copy the main framebuffer to the texture.
    glBindTexture(GL_TEXTURE_2D, d_render_texture->get_texture_resource().get_resource());
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 0, 0, d_texture_width, d_texture_height, 0);
}
