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

#include <string>
#include <utility>
#include <QDebug>
#include <QOpenGLFunctions_4_3_Core>
#include <QOpenGLFunctions_4_2_Core>
#include <QOpenGLFunctions_4_1_Core>
#include <QOpenGLFunctions_4_0_Core>
#include <QOpenGLFunctions_3_3_Core>
#include <QSurfaceFormat>
#include <QtGlobal>

#include "GLContext.h"

#include "GLStateStore.h"
#include "GLUtils.h"
#include "OpenGL.h"  // For Class GL
#include "OpenGLException.h"
#include "OpenGLFunctions.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


//
// Prefer OpenGL 4.3 (core profile) but allow a minimum of OpenGL 3.3 (core profile) with forward compatibility
// (forward compatibility means deprecated functions are removed, ie, don't specify 'QSurfaceFormat::DeprecatedFunctions').
//
// The OpenGL 3.3 minimum is because, on macOS, we cannot easily retain functionality in OpenGL 1 and 2 and also
// access OpenGL 3 functionality (required for volume visualization, new symbology, etc). MacOS gives you
// a choice of either 2.1 (which excludes volume visualization), or 3.2+ (their hardware supports 3.3/4.1)
// but only as a core profile which means no backward compatibility (and hence no fixed-function pipeline
// and hence no support for old graphics cards) and also only with forward compatibility (which removes support
// for wide lines). Previously we supported right back to OpenGL version 1.1 and used OpengGL extensions to
// provide optional support up to and including version 3 (eg, for volume visualization) when available.
// However we started running into problems with things like 'texture2D()' not working in GLSL
// (is called 'texture()' now). So it was easiest to move to OpenGL 3.3, and the time is right since
// (as of 2020) most hardware in last decade should support 3.3 (and it's also supported on Ubuntu 16.04,
// our min Ubuntu requirement at that time, by Mesa 11.2). It also gives us geometry shaders
// (which is useful for rendering wide lines and symbology in general). And removes the need to deal with
// all the various OpenGL extensions we've been dealing with.
//
// Also, Apple has deprecated OpenGL and will eventually remove it altogether.
// So when that happens the plan is to use Zink (OpenGL on top of Vulkan on top of Metal) on macOS.
//
const QPair<int, int> GPlatesOpenGL::GLContext::PREFERRED_OPENGL_VERSION(4, 3);
const QPair<int, int> GPlatesOpenGL::GLContext::MINIMUM_OPENGL_VERSION(3, 3);


void
GPlatesOpenGL::GLContext::set_default_surface_format()
{
	QSurfaceFormat default_surface_format;

	// Set our preferred OpenGL version.
	//
	// We might not get our preferred version. According to the "QOpenGLContext::create()" docs:
	//
	//   For example, if you request a context that supports OpenGL 4.3 Core profile but the driver and/or
	//   hardware only supports version 3.2 Core profile contexts then you will get a 3.2 Core profile context.
	//
	// But as long as the version we get is greater than or equal to our minimum version then that's fine.
	// We'll do that check when each widget/context is initialised (see 'initialise_gl()').
	default_surface_format.setVersion(
			PREFERRED_OPENGL_VERSION.first/*major*/,
			PREFERRED_OPENGL_VERSION.second/*minor*/);

	// OpenGL core profile, with forward compatibility (since not specifying 'QSurfaceFormat::DeprecatedFunctions').
	//
	// Note: Qt5 versions prior to 5.9 are unable to mix QPainter calls with OpenGL 3.2+ *core* profile calls:
	//       https://www.qt.io/blog/2017/01/27/opengl-core-profile-context-support-qpainter
	//       This means Qt < 5.9 must fallback to a *compatibility* profile to support this mixing.
	//       As mentioned above, this is not an option on macOS, which means macOS would require Qt5.9 or above to enable mixing.
	//       However we now no longer mix QPainter and OpenGL calls anyway, so it's not a problem.
	//       But if we returned to mixing we'd just need to ensure Qt >= 5.9 on macOS.
	default_surface_format.setProfile(QSurfaceFormat::CoreProfile);

	// GL_DEPTH24_STENCIL8 is a specified required format (by OpenGL 3.3 core).
	default_surface_format.setDepthBufferSize(24);
	default_surface_format.setStencilBufferSize(8);

	default_surface_format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);

	QSurfaceFormat::setDefaultFormat(default_surface_format);
}


GPlatesOpenGL::GLContext::GLContext(
		const boost::shared_ptr<Impl> &context_impl) :
	d_context_impl(context_impl),
	d_shared_state(new SharedState()),
	d_non_shared_state(new NonSharedState())
{
	// Defined in ".cc" file because...
	// non_null_ptr destructors require complete type of class they're referring to.
	// Compiler will call destructors of already-constructed members if constructor throws exception.
}


GPlatesOpenGL::GLContext::GLContext(
		const boost::shared_ptr<Impl> &context_impl,
		const boost::shared_ptr<SharedState> &shared_state) :
	d_context_impl(context_impl),
	d_shared_state(shared_state),
	d_non_shared_state(new NonSharedState())
{
	// Defined in ".cc" file because...
	// non_null_ptr destructors require complete type of class they're referring to.
	// Compiler will call destructors of already-constructed members if constructor throws exception.
}


GPlatesOpenGL::GLContext::~GLContext()
{
	// Defined in ".cc" file because...
	// non_null_ptr destructors require complete type of class they're referring to.
}


void
GPlatesOpenGL::GLContext::initialise_gl()
{
	// Get the OpenGL capabilities and parameters from the current OpenGL implementation.
	d_capabilities.initialise(get_opengl_functions(), d_context_impl->get_opengl_context());

	// The QSurfaceFormat of our OpenGL context.
	const QSurfaceFormat surface_format = get_surface_format();

	// Make sure we got our minimum OpenGL version or greater.
	if (surface_format.majorVersion() < MINIMUM_OPENGL_VERSION.first/*major*/ ||
		(surface_format.majorVersion() == MINIMUM_OPENGL_VERSION.first/*major*/ &&
				surface_format.minorVersion() < MINIMUM_OPENGL_VERSION.second/*minor*/))
	{
		throw OpenGLException(
				GPLATES_ASSERTION_SOURCE,
				QString("OpenGL %1.%2 (core profile) or greater is required.")
						.arg(MINIMUM_OPENGL_VERSION.first/*major*/)
						.arg(MINIMUM_OPENGL_VERSION.second/*minor*/).toStdString());
	}

	// We require a main framebuffer with a stencil buffer (usually interleaved with depth).
	// A lot of main framebuffer and render-target rendering uses a stencil buffer.
	if (surface_format.stencilBufferSize() < 8)
	{
		throw OpenGLException(
				GPLATES_ASSERTION_SOURCE,
				"Could not get a stencil buffer on the main framebuffer.");
	}

	qDebug() << "Context QSurfaceFormat:" << surface_format;
}


void
GPlatesOpenGL::GLContext::shutdown_gl()
{
	deallocate_queued_object_resources();

	d_shared_state->d_full_screen_quad = boost::none;
	d_shared_state->d_state_store = boost::none;

	d_opengl_functions = boost::none;
}


GPlatesGlobal::PointerTraits<GPlatesOpenGL::GL>::non_null_ptr_type
GPlatesOpenGL::GLContext::create_gl()
{
	return GL::create(
			get_non_null_pointer(this),
			get_opengl_functions(),
			get_shared_state()->get_state_store(get_capabilities()));
}


const GPlatesOpenGL::GLCapabilities&
GPlatesOpenGL::GLContext::get_capabilities() const
{
	// The capabilities must have been initialised (which means our 'initialise_gl()' must have been called).
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_capabilities.is_initialised(),
			GPLATES_ASSERTION_SOURCE);

	return d_capabilities;
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


GPlatesOpenGL::OpenGLFunctions &
GPlatesOpenGL::GLContext::get_opengl_functions()
{
	// Create/initialise the OpenGL functions for this context (if haven't already).
	if (!d_opengl_functions)
	{
		create_opengl_functions();
	}

	return *d_opengl_functions.get();
}


void
GPlatesOpenGL::GLContext::create_opengl_functions()
{
	// Get the version of OpenGL functions associated with the version of the OpenGL context.
	if (boost::optional<QOpenGLFunctions_4_3_Core *> opengl_functions_4_3 =
		get_version_functions<QOpenGLFunctions_4_3_Core>(4, 3))
	{
		d_opengl_functions = OpenGLFunctions::create(opengl_functions_4_3.get());
	}
	else if (boost::optional<QOpenGLFunctions_4_2_Core *> opengl_functions_4_2 =
		get_version_functions<QOpenGLFunctions_4_2_Core>(4, 2))
	{
		d_opengl_functions = OpenGLFunctions::create(opengl_functions_4_2.get());
	}
	else if (boost::optional<QOpenGLFunctions_4_1_Core *> opengl_functions_4_1 =
		get_version_functions<QOpenGLFunctions_4_1_Core>(4, 1))
	{
		d_opengl_functions = OpenGLFunctions::create(opengl_functions_4_1.get());
	}
	else if (boost::optional<QOpenGLFunctions_4_0_Core *> opengl_functions_4_0 =
		get_version_functions<QOpenGLFunctions_4_0_Core>(4, 0))
	{
		d_opengl_functions = OpenGLFunctions::create(opengl_functions_4_0.get());
	}
	else if (boost::optional<QOpenGLFunctions_3_3_Core *> opengl_functions_3_3 =
		get_version_functions<QOpenGLFunctions_3_3_Core>(3, 3))
	{
		d_opengl_functions = OpenGLFunctions::create(opengl_functions_3_3.get());
	}
	else
	{
		// The QSurfaceFormat of our OpenGL context.
		const QSurfaceFormat surface_format = get_surface_format();

		throw OpenGLException(
				GPLATES_ASSERTION_SOURCE,
				QString("Failed to access OpenGL functions in valid OpenGL context (version %1.%2 core).")
						.arg(surface_format.majorVersion())
						.arg(surface_format.minorVersion()).toStdString());
	}
}


template <class OpenGLFunctionsType>
boost::optional<OpenGLFunctionsType *>
GPlatesOpenGL::GLContext::get_version_functions(
		int major_version,
		int minor_version) const
{
	QOpenGLVersionProfile version_profile;
	version_profile.setProfile(QSurfaceFormat::CoreProfile);
	version_profile.setVersion(major_version, minor_version);

	// See if OpenGL functions are available for the specified version (of core profile) in the context.
	QAbstractOpenGLFunctions *abstract_opengl_functions = d_context_impl->get_version_functions(version_profile);
	if (!abstract_opengl_functions)
	{
		return boost::none;
	}

	// Downcast to the expected version functions type.
	OpenGLFunctionsType *opengl_functions = dynamic_cast<OpenGLFunctionsType *>(abstract_opengl_functions);
	// If downcast fails then we've made a programming error (mismatched the version number and the class type).
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			opengl_functions,
			GPLATES_ASSERTION_SOURCE);

	// Initialise the OpenGL functions.
	//
	// This only needs to be done once (per instance) and this instance is owned by QOpenGLContext.
	// Also there is no need to call this as long as this context is current (which is should be),
	// but we'll call it anyway just in case.
	if (!opengl_functions->initializeOpenGLFunctions())
	{
		throw OpenGLException(
				GPLATES_ASSERTION_SOURCE,
				QString("Failed to initialise OpenGL functions in OpenGL %1.%2 (core profile).")
						.arg(major_version)
						.arg(minor_version).toStdString());
	}

	return opengl_functions;
}


void
GPlatesOpenGL::GLContext::deallocate_queued_object_resources()
{
	OpenGLFunctions &opengl_functions = get_opengl_functions();

	get_non_shared_state()->get_framebuffer_resource_manager()->deallocate_queued_resources(opengl_functions);
	get_non_shared_state()->get_vertex_array_resource_manager()->deallocate_queued_resources(opengl_functions);

	get_shared_state()->get_texture_resource_manager()->deallocate_queued_resources(opengl_functions);
	get_shared_state()->get_renderbuffer_resource_manager()->deallocate_queued_resources(opengl_functions);
	get_shared_state()->get_buffer_resource_manager()->deallocate_queued_resources(opengl_functions);
	get_shared_state()->get_shader_resource_manager()->deallocate_queued_resources(opengl_functions);
	get_shared_state()->get_program_resource_manager()->deallocate_queued_resources(opengl_functions);
}


GPlatesOpenGL::GLContext::SharedState::SharedState() :
	d_buffer_resource_manager(GLBuffer::resource_manager_type::create()),
	d_program_resource_manager(GLProgram::resource_manager_type::create()),
	d_renderbuffer_resource_manager(GLRenderbuffer::resource_manager_type::create()),
	d_sampler_resource_manager(GLSampler::resource_manager_type::create()),
	d_shader_resource_manager(GLShader::resource_manager_type::create()),
	d_texture_resource_manager(GLTexture::resource_manager_type::create())
{
}


GPlatesOpenGL::GLVertexArray::shared_ptr_type
GPlatesOpenGL::GLContext::SharedState::get_full_screen_quad(
		GL &gl)
{
	// Create the sole vertex array if it hasn't already been created.
	if (!d_full_screen_quad)
	{
		d_full_screen_quad = GLUtils::create_full_screen_quad(gl);
	}

	return d_full_screen_quad.get();
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
