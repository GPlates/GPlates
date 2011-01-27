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

#include <cstddef> // For std::size_t
/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>
#include <opengl/OpenGL.h>
#include <QDebug>

#include "GLContext.h"

#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


bool GPlatesOpenGL::GLContext::s_initialised_GLEW = false;
boost::optional<GPlatesOpenGL::GLContext::TextureParameters> GPlatesOpenGL::GLContext::s_texture_parameters;
boost::optional<GPlatesOpenGL::GLRenderTargetFactory::non_null_ptr_type> GPlatesOpenGL::GLContext::s_render_target_factory;

// Set the GL_TEXTURE0_ARB constant.
const GLenum GPlatesOpenGL::GLContext::TextureParameters::gl_texture0_ARB = GL_TEXTURE0_ARB;

// We use macros in <GL/glew.h> that contain old-style casts.
DISABLE_GCC_WARNING("-Wold-style-cast")

void
GPlatesOpenGL::GLContext::initialise()
{
	// Currently we only initialise once for the whole application instead of
	// once for each rendering context.
	// This is because the GLEW library would need to be compiled with the GLEW_MX
	// flag (see http://glew.sourceforge.net/advanced.html under multiple rendering contexts)
	// and this does not appear to be supported in all package managers (eg, linux and MacOS X).
	// There's not much information on whether we need one if we share contexts in Qt but
	// this is the assumption here.
	if (!s_initialised_GLEW)
	{
		GLenum err = glewInit();
		if (GLEW_OK != err)
		{
			// glewInit failed.
			//
			// We'll assume all calls to test whether an extension is available
			// (such as "if (GLEW_ARB_multitexture) ..." will fail since they just
			// test boolean variables which are assumed to be initialised by GLEW to zero.
			// This just means we will be forced to fall back to OpenGL version 1.1.
			qWarning() << "Error: " << reinterpret_cast<const char *>(glewGetErrorString(err));
		}
		qDebug() << "Status: Using GLEW " << reinterpret_cast<const char *>(glewGetString(GLEW_VERSION));

		s_initialised_GLEW = true;

		// Initialise texture parameters with default values.
		TextureParameters texture_parameters =
		{
			64, // Minimum size texture that must be supported by all OpenGL implementations
			1, // GL_MAX_TEXTURE_UNITS_ARB
			1.0f // GL_TEXTURE_MAX_ANISOTROPY_EXT
		};

		// Get the maximum texture size (dimension).
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texture_parameters.gl_max_texture_size);  

		// Get the maximum number of texture units supported.
		if (GLEW_ARB_multitexture)
		{
			glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &texture_parameters.gl_max_texture_units_ARB);
		}

		// Get the maximum texture anisotropy supported.
		if (GLEW_EXT_texture_filter_anisotropic)
		{
			glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &texture_parameters.gl_texture_max_anisotropy_EXT);
		}

		s_texture_parameters = texture_parameters;

		//
		// Create the render target factory based on the capabilities of the run-time system.
		//

		if (GLFrameBufferObjectRenderTargetFactory::is_supported())
		{
			qDebug() << "Using frame buffer object support for render targets";
			s_render_target_factory = GLFrameBufferObjectRenderTargetFactory::create();
		}
		else if (GLPBufferRenderTargetFactory::is_supported())
		{
			qDebug() << "Using pbuffer support for render targets";
			QGLWidget *qgl_widget = &static_cast<QGLWidgetImpl *>(d_context_impl.get())->d_qgl_widget;
			s_render_target_factory = GLPBufferRenderTargetFactory::create(qgl_widget);
		}
		else
		{
			qDebug() << "Falling back to main frame buffer for render targets";
			s_render_target_factory = GLMainFrameBufferRenderTargetFactory::create();

			// Alot of render-target rendering uses an alpha channel so emit a warning if
			// the frame buffer doesn't have an alpha channel.
			 if (!static_cast<QGLWidgetImpl *>(d_context_impl.get())->d_qgl_widget.format().alpha())
			 {
				 qWarning(
						"Could not get alpha channel on main frame buffer; "
						"render-target results will be suboptimal");
			 }
		}
	}
}

ENABLE_GCC_WARNING("-Wold-style-cast")


const GPlatesOpenGL::GLContext::TextureParameters &
GPlatesOpenGL::GLContext::get_texture_parameters()
{
	// GLEW must have been initialised.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			s_initialised_GLEW && s_texture_parameters,
			GPLATES_ASSERTION_SOURCE);

	return s_texture_parameters.get();
}


GPlatesOpenGL::GLRenderTargetFactory &
GPlatesOpenGL::GLContext::get_render_target_factory()
{
	// GLEW must have been initialised.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			s_initialised_GLEW && s_render_target_factory,
			GPLATES_ASSERTION_SOURCE);

	return *s_render_target_factory.get();
}


GPlatesOpenGL::GLContext::SharedState::SharedState() :
	d_texture_resource_manager(GLTextureResourceManager::create())
{
}
