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

#include <opengl/OpenGL3.h>  // Should be included at TOP of ".cc" file.

#include <string>
#include <utility>
#include <QDebug>
#include <QtGlobal>

#include "GLContext.h"

#include "GL.h"
#include "GLStateStore.h"
#include "GLUtils.h"
#include "OpenGLException.h"

#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

// We use macros in <GL/glew.h> that contain old-style casts.
DISABLE_GCC_WARNING("-Wold-style-cast")


bool GPlatesOpenGL::GLContext::s_initialised_GLEW = false;
GPlatesOpenGL::GLCapabilities GPlatesOpenGL::GLContext::s_capabilities;


QGLFormat
GPlatesOpenGL::GLContext::get_qgl_format_to_create_context_with()
{
	// We turn *off* multisampling because lines actually look better without it...
	// We need a stencil buffer for filling polygons.
	// We need an alpha channel in case falling back to main framebuffer for render textures.
	QGLFormat format(/*QGL::SampleBuffers |*/ QGL::DepthBuffer | QGL::StencilBuffer | QGL::AlphaChannel);

	// Use the OpenGL 3.3 core profile with forward compatibility (ie, deprecated functions removed).
	//
	// This is because we cannot easily access OpenGL 3 functionality (required for volume visualization)
	// on macOS and still retain functionality in OpenGL 1 and 2. MacOS which gives you a choice of either
	// 2.1 (which excludes volume visualization), or 3.2+ (their hardware supports 3.3/4.1) but only as a
	// core profile which means no backward compatibility (and hence no fixed-function pipeline and hence
	// no support for old graphics cards) and also only with forward compatibility (which removes support
	// for wide lines). Previously we supported right back to OpenGL version 1.1 and used OpengGL extensions to
	// provide optional support up to and including version 3 (eg, for volume visualization) when available.
	// However we started running into problems with things like 'texture2D()' not working in GLSL
	// (is called 'texture()' now). So it was easiest to move to OpenGL 3.3, and the time is right since
	// (as of 2020) most hardware in last decade should support 3.3 (and it's also supported on Ubuntu 16.04,
	// our current min Ubuntu requirement, by Mesa 11.2). It also gives us geometry shaders
	// (which is useful for rendering wide lines and symbology in general). And removes the need to deal
	// with all the various OpenGL extensions we've been dealing with. Eventually Apple with remove OpenGL though,
	// and the hope is we will have access to the Rendering Hardware Interface (RHI) in the upcoming Qt6
	// (it's currently in Qt5.14 but only accessible to Qt internally). RHI will allow us to use GLSL shaders,
	// and will provide us with a higher-level interface/abstraction for setting the graphics pipeline state
	// (that RHI then hands off to OpenGL/Vulkan/Direct3D/Metal). But for now it's OpenGL 3.3.
	//
	format.setVersion(3, 3);
	//
	// Qt5 versions prior to 5.9 are unable to mix QPainter calls with OpenGL 3.x *core* profile calls:
	//   https://www.qt.io/blog/2017/01/27/opengl-core-profile-context-support-qpainter
	//
	// This means we must fallback to an OpenGL *compatibility* profile for Qt5 versions less than 5.9.
	// As mentioned above, this is not an option on macOS, which means macOS requires Qt5.9 or above.
	//
#if QT_VERSION >= QT_VERSION_CHECK(5,9,0)
	format.setProfile(QGLFormat::CoreProfile);
#else // Qt < 5.9 ...
	#if defined(Q_OS_MACOS)
		#error "macOS requires Qt5.9 or above."
	#else //  not macOS ...
		format.setProfile(QGLFormat::CompatibilityProfile);
	#endif
#endif

	return format;
}


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
			std::string error_message = std::string("Unable to initialise GLEW: ") +
					reinterpret_cast<const char *>(glewGetErrorString(err));
			throw OpenGLException(
					GPLATES_ASSERTION_SOURCE,
					error_message.c_str());
		}
		//qDebug() << "Status: Using GLEW " << reinterpret_cast<const char *>(glewGetString(GLEW_VERSION));

		s_initialised_GLEW = true;

		// Get the OpenGL capabilities and parameters from the current OpenGL implementation.
		s_capabilities.initialise();
	}

	// The QGLFormat of our OpenGL context.
	const QGLFormat &qgl_format = get_qgl_format();

	// Make sure we got OpenGL 3.3 or greater.
	if (qgl_format.majorVersion() < 3 ||
		(qgl_format.majorVersion() == 3 && qgl_format.minorVersion() < 3))
	{
		throw OpenGLException(
			GPLATES_ASSERTION_SOURCE,
			"OpenGL 3.3 or greater is required.");
	}

	// We require a main framebuffer with an alpha channel.
	// A lot of main framebuffer and render-target rendering uses an alpha channel.
	//
	// TODO: Now that we're guaranteed support framebuffer objects we no longer need the main framebuffer
	//       for render-target rendering. Maybe we don't need alpha in main buffer.
	//       But modern H/W will have it anyway.
	if (!qgl_format.alpha())
	{
		throw OpenGLException(
				GPLATES_ASSERTION_SOURCE,
				"Could not get alpha channel on main framebuffer.");
	}

	// We require a main framebuffer with a stencil buffer (usually interleaved with depth).
	// A lot of main framebuffer and render-target rendering uses a stencil buffer.
	if (!qgl_format.stencil())
	{
		throw OpenGLException(
				GPLATES_ASSERTION_SOURCE,
				"Could not get a stencil buffer on the main framebuffer.");
	}

	qDebug() << "Context QGLFormat:" << qgl_format;
}


GPlatesGlobal::PointerTraits<GPlatesOpenGL::GL>::non_null_ptr_type
GPlatesOpenGL::GLContext::create_gl()
{
	return GL::create(
			get_non_null_pointer(this),
			get_shared_state()->get_state_store(get_capabilities()));
}


const GPlatesOpenGL::GLCapabilities &
GPlatesOpenGL::GLContext::get_capabilities() const
{
	// GLEW must have been initialised.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			s_initialised_GLEW,
			GPLATES_ASSERTION_SOURCE);

	return s_capabilities;
}


void
GPlatesOpenGL::GLContext::begin_render()
{
	deallocate_queued_object_resources();
}


void
GPlatesOpenGL::GLContext::end_render()
{
	deallocate_queued_object_resources();
}


void
GPlatesOpenGL::GLContext::deallocate_queued_object_resources()
{
	get_non_shared_state()->get_framebuffer_resource_manager()->deallocate_queued_resources();
	get_non_shared_state()->get_vertex_array_resource_manager()->deallocate_queued_resources();

	get_shared_state()->get_texture_object_resource_manager()->deallocate_queued_resources();
	get_shared_state()->get_renderbuffer_resource_manager()->deallocate_queued_resources();
	get_shared_state()->get_buffer_resource_manager()->deallocate_queued_resources();
	get_shared_state()->get_shader_resource_manager()->deallocate_queued_resources();
	get_shared_state()->get_program_resource_manager()->deallocate_queued_resources();
}


GPlatesOpenGL::GLContext::SharedState::SharedState() :
	d_buffer_resource_manager(GLBuffer::resource_manager_type::create()),
	d_program_resource_manager(GLProgramObject::resource_manager_type::create()),
	d_renderbuffer_resource_manager(GLRenderbuffer::resource_manager_type::create()),
	d_shader_resource_manager(GLShader::resource_manager_type::create()),
	d_texture_object_resource_manager(GLTexture::resource_manager_type::create())
{
}


GPlatesOpenGL::GLCompiledDrawState::non_null_ptr_to_const_type
GPlatesOpenGL::GLContext::SharedState::get_full_screen_2D_textured_quad(
		GL &gl)
{
	// Create the sole unbound vertex array compile state if it hasn't already been created.
	if (!d_full_screen_2D_textured_quad)
	{
		d_full_screen_2D_textured_quad = GLUtils::create_full_screen_2D_textured_quad(gl);
	}

	return d_full_screen_2D_textured_quad.get();
}


GPlatesOpenGL::GLStateStore::non_null_ptr_type
GPlatesOpenGL::GLContext::SharedState::get_state_store(
		const GLCapabilities &capabilities)
{
	if (!d_state_store)
	{
		d_state_store = GLStateStore::create(
				GLStateSetStore::create(),
				GLStateSetKeys::create(capabilities));
	}

	return d_state_store.get();
}


GPlatesOpenGL::GLContext::NonSharedState::NonSharedState() :
	d_framebuffer_resource_manager(GLFramebuffer::resource_manager_type::create()),
	d_vertex_array_resource_manager(GLVertexArray::resource_manager_type::create())
{
}
