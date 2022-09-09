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
#include "global/config.h" // For GPLATES_USE_VULKAN_BACKEND
#if !defined(GPLATES_USE_VULKAN_BACKEND)
#	include <QOpenGLFunctions_4_5_Core>
#	if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
	// Qt6 moved QOpenGLContext::versionFunctions() to QOpenGLVersionFunctionsFactory::get().
#	include <QOpenGLVersionFunctionsFactory>
#	endif
#	include <QSurfaceFormat>
#endif

#include "GLContext.h"

#include "GLStateStore.h"
#include "OpenGL.h"  // For Class GL
#include "OpenGLException.h"
#include "OpenGLFunctions.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


#if !defined(GPLATES_USE_VULKAN_BACKEND)
// Require OpenGL 4.5
//
// Note: macOS only supports OpenGL version 4.1 (and they've deprecated OpenGL).
//       So OpenGL 4.5 will only work on Windows and Linux.
//       But we'll be switching over to Vulkan soon though (for all platforms).
//       While that switchover is happening we'll use OpenGL 4.5 (which has similar functionality to Vulkan).
const QPair<int, int> GPlatesOpenGL::GLContext::REQUIRED_OPENGL_VERSION(4, 5);
#endif


#if !defined(GPLATES_USE_VULKAN_BACKEND)
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
#endif // GPLATES_USE_VULKAN_BACKEND


GPlatesOpenGL::GLContext::GLContext()
{
	// The boost::shared_ptr<> deleter is a lambda function that does nothing
	// (since we don't actually want to delete 'this').
	d_context_handle.reset(this, [](GLContext*) {});

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
#if defined(GPLATES_USE_VULKAN_BACKEND)
		QVulkanWindow &vulkan_window
#else
		QOpenGLWindow &opengl_window
#endif
)
{
#if defined(GPLATES_USE_VULKAN_BACKEND)

	d_surface = vulkan_window;

	// Create the OpenGL functions wrapper from the Vulkan device functions.
	QVulkanDeviceFunctions *vulkan_device_functions = d_surface->vulkanInstance()->deviceFunctions(d_surface->device());
	d_opengl_functions = OpenGLFunctions::create(vulkan_device_functions);

#else

	d_surface = opengl_window;

	// The QSurfaceFormat of our OpenGL context.
	const QSurfaceFormat surface_format = d_surface->context()->format();

	qDebug() << "Context QSurfaceFormat:" << surface_format;

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
		get_opengl_version_functions<QOpenGLFunctions_4_5_Core>(4, 5))
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

#endif

	// Get the OpenGL capabilities and parameters from the current OpenGL implementation.
	d_capabilities.initialise(
			*d_opengl_functions.get()
#if !defined(GPLATES_USE_VULKAN_BACKEND)
		, *d_surface->context()
#endif
	);

	// Used by GL to efficiently allocate GLState objects.
	d_state_store = GLStateStore::create(
			GLStateSetStore::create(),
			GLStateSetKeys::create(d_capabilities));
}


void
GPlatesOpenGL::GLContext::shutdown_gl()
{
	d_state_store = boost::none;

	d_opengl_functions = boost::none;
	d_surface = boost::none;
}


GPlatesGlobal::PointerTraits<GPlatesOpenGL::GL>::non_null_ptr_type
GPlatesOpenGL::GLContext::access_opengl()
{
	// We should be between 'initialise_gl()' and 'shutdown_gl()'.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_surface && d_opengl_functions && d_capabilities.is_initialised() && d_state_store,
			GPLATES_ASSERTION_SOURCE);

#if !defined(GPLATES_USE_VULKAN_BACKEND)
	// Make sure the OpenGL context is current.
	// Also this sets the default framebuffer object used by the QOpenGLWindow.
	d_surface->makeCurrent();
#endif

	// The default viewport of the QOpenGLWindow.
	const GLViewport default_viewport(0, 0,
			// Dimensions, in OpenGL, are in device pixels...
			d_surface->width() * d_surface->devicePixelRatio(),
			d_surface->height() * d_surface->devicePixelRatio());

	// The default framebuffer object of the QOpenGLWindow.
	const GLuint default_framebuffer_object =
#if defined(GPLATES_USE_VULKAN_BACKEND)
		0
#else
		d_surface->defaultFramebufferObject()
#endif
		;

	return GL::create(
			get_non_null_pointer(this),
			d_capabilities,
			*d_opengl_functions.get(),
			d_state_store.get(),
			default_viewport,
			default_framebuffer_object);
}


boost::optional<GPlatesOpenGL::OpenGLFunctions &>
GPlatesOpenGL::GLContext::get_opengl_functions()
{
	if (!d_opengl_functions)
	{
		return boost::none;
	}

	return *d_opengl_functions.get();
}


#if !defined(GPLATES_USE_VULKAN_BACKEND)

template <class OpenGLFunctionsType>
boost::optional<OpenGLFunctionsType *>
GPlatesOpenGL::GLContext::get_opengl_version_functions(
		int major_version,
		int minor_version) const
{
	// We should be between 'initialise_gl()' and 'shutdown_gl()'.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_surface,
			GPLATES_ASSERTION_SOURCE);

	QOpenGLVersionProfile version_profile;
	version_profile.setProfile(QSurfaceFormat::CoreProfile);
	version_profile.setVersion(major_version, minor_version);

	// See if OpenGL functions are available for the specified version (of core profile) in the context.
	QAbstractOpenGLFunctions *abstract_opengl_functions =
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
			// Qt6 moved QOpenGLContext::versionFunctions() to QOpenGLVersionFunctionsFactory::get().
			QOpenGLVersionFunctionsFactory::get(version_profile, d_surface->context());
#else
			d_surface->context()->versionFunctions(version_profile);
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

#endif
