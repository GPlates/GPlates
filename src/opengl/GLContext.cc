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
#include <QtGlobal>
#include <QDebug>
#include <QOpenGLFunctions_4_5_Core>
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
 // Qt6 moved QOpenGLContext::versionFunctions() to QOpenGLVersionFunctionsFactory::get().
#include <QOpenGLVersionFunctionsFactory>
#endif
#include <QSurfaceFormat>

#include "GLContext.h"

#include "GLStateStore.h"
#include "OpenGL.h"  // For Class GL
#include "OpenGLException.h"
#include "OpenGLFunctions.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


// Require OpenGL 4.5
//
// Note: macOS only supports OpenGL version 4.1 (and they've deprecated OpenGL).
//       So OpenGL 4.5 will only work on Windows and Linux.
//       But we'll be switching over to Vulkan soon though (for all platforms).
//       While that switchover is happening we'll use OpenGL 4.5 (which has similar functionality to Vulkan).
const QPair<int, int> GPlatesOpenGL::GLContext::REQUIRED_OPENGL_VERSION(4, 5);


void
GPlatesOpenGL::GLContext::set_default_surface_format()
{
	QSurfaceFormat default_surface_format;

	// Set our required OpenGL version.
	//
	// We might not get our required version. According to the "QOpenGLContext::create()" docs:
	//
	//   For example, if you request a context that supports OpenGL 4.3 Core profile but the driver and/or
	//   hardware only supports version 3.2 Core profile contexts then you will get a 3.2 Core profile context.
	//
	// We'll do that check when the QOpenGLWindow is initialised (see 'initialise_gl()').
	default_surface_format.setVersion(REQUIRED_OPENGL_VERSION.first/*major*/, REQUIRED_OPENGL_VERSION.second/*minor*/);

	// OpenGL core profile, with forward compatibility (since not specifying 'QSurfaceFormat::DeprecatedFunctions').
	// Forward compatibility removes support for wide lines (but wide lines are not supported on macOS anyway, and that includes Vulkan).
	default_surface_format.setProfile(QSurfaceFormat::CoreProfile);

	// GL_DEPTH24_STENCIL8 is a specified required format (by OpenGL 3.3 core).
	default_surface_format.setDepthBufferSize(24);
	default_surface_format.setStencilBufferSize(8);

	default_surface_format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);

	QSurfaceFormat::setDefaultFormat(default_surface_format);
}


GPlatesOpenGL::GLContext::GLContext() :
	d_shared_state(new SharedState())
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
GPlatesOpenGL::GLContext::initialise_gl(
		QOpenGLWindow &opengl_window)
{
	d_opengl_window = opengl_window;

	// The QSurfaceFormat of our OpenGL context.
	const QSurfaceFormat surface_format = d_opengl_window->context()->format();

	// Make sure we got our required OpenGL version or greater.
	if (surface_format.majorVersion() < REQUIRED_OPENGL_VERSION.first/*major*/ ||
		(surface_format.majorVersion() == REQUIRED_OPENGL_VERSION.first/*major*/ &&
				surface_format.minorVersion() < REQUIRED_OPENGL_VERSION.second/*minor*/))
	{
		throw OpenGLException(
				GPLATES_ASSERTION_SOURCE,
				QString("OpenGL %1.%2 (core profile) or greater is required.")
						.arg(REQUIRED_OPENGL_VERSION.first/*major*/)
						.arg(REQUIRED_OPENGL_VERSION.second/*minor*/).toStdString());
	}

	// Get the version of OpenGL functions associated with the version of the OpenGL context.
	if (boost::optional<QOpenGLFunctions_4_5_Core *> opengl_functions_4_5 =
		get_version_functions<QOpenGLFunctions_4_5_Core>(4, 5))
	{
		d_opengl_functions = OpenGLFunctions::create(opengl_functions_4_5.get());
	}
	else
	{
		throw OpenGLException(
				GPLATES_ASSERTION_SOURCE,
				QString("Failed to access OpenGL functions in valid OpenGL context (version %1.%2 core).")
						.arg(surface_format.majorVersion())
						.arg(surface_format.minorVersion()).toStdString());
	}

	// We require a main framebuffer with a stencil buffer (usually interleaved with depth).
	// A lot of main framebuffer and render-target rendering uses a stencil buffer.
	if (surface_format.stencilBufferSize() < 8)
	{
		throw OpenGLException(
				GPLATES_ASSERTION_SOURCE,
				"Could not get a stencil buffer on the main framebuffer.");
	}

	// Get the OpenGL capabilities and parameters from the current OpenGL implementation.
	d_capabilities.initialise(*d_opengl_functions.get(), *d_opengl_window->context());

	qDebug() << "Context QSurfaceFormat:" << surface_format;
}


void
GPlatesOpenGL::GLContext::shutdown_gl()
{
	deallocate_queued_object_resources();

	d_shared_state->d_state_store = boost::none;

	d_opengl_functions = boost::none;
	d_opengl_window = boost::none;
}


GPlatesGlobal::PointerTraits<GPlatesOpenGL::GL>::non_null_ptr_type
GPlatesOpenGL::GLContext::access_opengl()
{
	// We should be between 'initialise_gl()' and 'shutdown_gl()'.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_opengl_window && d_opengl_functions,
			GPLATES_ASSERTION_SOURCE);

	// Make sure the OpenGL context is current.
	// Also this sets the default framebuffer object used by the QOpenGLWindow.
	d_opengl_window->makeCurrent();

	// The default viewport of the QOpenGLWindow.
	const GLViewport default_viewport(0, 0,
			// Dimensions, in OpenGL, are in device pixels...
			d_opengl_window->width() * d_opengl_window->devicePixelRatio(),
			d_opengl_window->height() * d_opengl_window->devicePixelRatio());

	// The default framebuffer object of the QOpenGLWindow.
	const GLuint default_framebuffer_object = d_opengl_window->defaultFramebufferObject();

	return GL::create(
			get_non_null_pointer(this),
			get_capabilities(),
			*d_opengl_functions.get(),
			get_shared_state()->get_state_store(get_capabilities()),
			default_viewport,
			default_framebuffer_object);
}


const GPlatesOpenGL::GLCapabilities &
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


template <class OpenGLFunctionsType>
boost::optional<OpenGLFunctionsType *>
GPlatesOpenGL::GLContext::get_version_functions(
		int major_version,
		int minor_version) const
{
	// We should be between 'initialise_gl()' and 'shutdown_gl()'.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_opengl_window,
			GPLATES_ASSERTION_SOURCE);

	QOpenGLVersionProfile version_profile;
	version_profile.setProfile(QSurfaceFormat::CoreProfile);
	version_profile.setVersion(major_version, minor_version);

	// See if OpenGL functions are available for the specified version (of core profile) in the context.
	QAbstractOpenGLFunctions *abstract_opengl_functions =
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
			// Qt6 moved QOpenGLContext::versionFunctions() to QOpenGLVersionFunctionsFactory::get().
			QOpenGLVersionFunctionsFactory::get(version_profile, d_opengl_window->context());
#else
			d_opengl_window->context()->versionFunctions(version_profile);
#endif
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
	// If we're between 'initialise_gl()' and 'shutdown_gl()', otherwise wait until then to deallocate.
	if (d_opengl_functions)
	{
		OpenGLFunctions &opengl_functions = *d_opengl_functions.get();

		get_shared_state()->get_buffer_resource_manager()->deallocate_queued_resources(opengl_functions);
		get_shared_state()->get_framebuffer_resource_manager()->deallocate_queued_resources(opengl_functions);
		get_shared_state()->get_program_resource_manager()->deallocate_queued_resources(opengl_functions);
		get_shared_state()->get_renderbuffer_resource_manager()->deallocate_queued_resources(opengl_functions);
		get_shared_state()->get_shader_resource_manager()->deallocate_queued_resources(opengl_functions);
		get_shared_state()->get_texture_resource_manager()->deallocate_queued_resources(opengl_functions);
		get_shared_state()->get_vertex_array_resource_manager()->deallocate_queued_resources(opengl_functions);
	}
}


GPlatesOpenGL::GLContext::SharedState::SharedState() :
	d_buffer_resource_manager(GLBuffer::resource_manager_type::create()),
	d_framebuffer_resource_manager(GLFramebuffer::resource_manager_type::create()),
	d_program_resource_manager(GLProgram::resource_manager_type::create()),
	d_renderbuffer_resource_manager(GLRenderbuffer::resource_manager_type::create()),
	d_sampler_resource_manager(GLSampler::resource_manager_type::create()),
	d_shader_resource_manager(GLShader::resource_manager_type::create()),
	d_texture_resource_manager(GLTexture::resource_manager_type::create()),
	d_vertex_array_resource_manager(GLVertexArray::resource_manager_type::create())
{
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
