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
 
#ifndef GPLATES_OPENGL_GLCONTEXT_H
#define GPLATES_OPENGL_GLCONTEXT_H

#include <map>
#include <utility>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
 // For OpenGL constants and typedefs...
// Note: Cannot include "OpenGL.h" due to cyclic dependency with class GL.
#include <qopengl.h>
#include <QPair>
#include <QOpenGLContext>
#include <QOpenGLVersionFunctions>  // For QAbstractOpenGLFunctions
#include <QOpenGLVersionProfile>
#include <QOpenGLWindow>
#include <QSurfaceFormat>

#include "GLCapabilities.h"

#include "global/PointerTraits.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	class GL;
	class GLStateStore;
	class OpenGLFunctions;

	/**
	 * Mirrors an OpenGL context and provides a central place to manage low-level OpenGL objects.
	 */
	class GLContext :
			public GPlatesUtils::ReferenceCount<GLContext>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLContext.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLContext> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLContext.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLContext> non_null_ptr_to_const_type;


		/**
		 * Set the global default surface format (eg, used by QOpenGLWindow).
		 *
		 * Note: This should be called before constructing the QApplication instance (on macOS at least).
		 */
		static
		void
		set_default_surface_format();


		/**
		 * Creates a @a GLContext object.
		 *
		 * We reference the OpenGL context of a particular QOpenGLWindow (see @a initialise_gl).
		 */
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new GLContext());
		}


		~GLContext();


		/**
		 * The OpenGL context represents the specified QOpenGLWindow.
		 */
		void
		initialise_gl(
				QOpenGLWindow &opengl_window);

		/**
		 * The OpenGL context is about to be destroyed.
		 */
		void
		shutdown_gl();


		/**
		 * Access OpenGL for the lifetime of the returned @a GL object.
		 *
		 * The returned object should be used as a local object (on C++ runtime stack) and only
		 * created/used when need to render something (it should not be stored persistently).
		 *
		 * Typically a @a GL is created each time a frame needs drawing.
		 *
		 * The viewport/scissor rectangle is set to the current dimensions (in device pixels) of the
		 * default framebuffer of the @a QOpenGLWindow passed to @a initialise_gl.
		 * And this is then considered the default viewport/scissor rectangle for the current rendering scope.
		 *
		 * NOTE: OpenGL must be in the default state before entering this scope.
		 *       On exiting this scope the default OpenGL state is restored.
		 */
		GPlatesGlobal::PointerTraits<GL>::non_null_ptr_type
		access_opengl();


	private: // For use by OpenGL resource object classes (such as @a GLTexture)...

		//
		// Only resource object classes should be able to access the low-level OpenGL functions and OpenGL context.
		//
		template <typename ResourceHandleType, class ResourceAllocatorType>
		friend class GLObjectResource;

		/**
		 * Access low-level OpenGL functions.
		 *
		 * Returns none if not called between @a initialise_gl and @a shutdown_gl.
		 */
		boost::optional<OpenGLFunctions &>
		get_opengl_functions();

		/**
		 * Access this context without using an owning pointer.
		 *
		 * This is used by an OpenGL resource object to ensure this @a GLContext is still alive
		 * when the time comes for it to deallocate its resource.
		 */
		boost::weak_ptr<GLContext>
		get_context_handle()
		{
			return boost::weak_ptr<GLContext>(d_context_handle);
		}

	private:
		/**
		 * We reference the QOpenGLContext of a particular QOpenGLWindow.
		 *
		 * It's only valid between @a initialise_gl and @a shutdown_gl.
		 */
		boost::optional<QOpenGLWindow &> d_opengl_window;

		/**
		 * The OpenGL functions for the version and profile of this context.
		 *
		 * It's only valid between @a initialise_gl and @a shutdown_gl.
		 */
		boost::optional<GPlatesGlobal::PointerTraits<OpenGLFunctions>::non_null_ptr_type> d_opengl_functions;

		/**
		 * OpenGL implementation-dependent capabilities and parameters.
		 *
		 * Note: This should be the same for all contexts (created for all widgets) since they
		 *       they all share the same default QSurfaceFormat.
		 */
		GLCapabilities d_capabilities;

		/**
		 * Used by @a GL to efficiently allocate @a GLState objects.
		 *
		 * It's only valid between @a initialise_gl and @a shutdown_gl.
		 */
		boost::optional<GPlatesGlobal::PointerTraits<GLStateStore>::non_null_ptr_type> d_state_store;

		/**
		 * A pointer to 'this' context that does not delete it.
		 *
		 * This is used by @a GLObjectResource to obtain a boost::weak_ptr<GLContext>,
		 * via @a get_context_handle, to use later when it deallocates its object resource.
		 *
		 * When 'this' @a GLContext is destroyed then this context handle will get destroyed
		 * indicating to any @a GLObjectResource that it can no longer use 'this' @a GLContext.
		 */
		boost::shared_ptr<GLContext> d_context_handle;


		/**
		 * The *required* OpenGL version (in a core profile).
		 */
		static const QPair<int, int> REQUIRED_OPENGL_VERSION;


		//! Constructor.
		GLContext();


		/**
		 * Returns the OpenGL functions (via Qt) of the specified version core profile in the OpenGL context.
		 *
		 * Returns none if request failed. For example, if requesting functions (via specified version)
		 * that are not in the version (and core profile) of the OpenGL context, such as requesting
		 * 4.5 core functions from a 3.3 core context.
		 *
		 * Note: The template type pointer 'OpenGLFunctionsType' should match the version (and 'core' profile).
		 *       For example, 'QOpenGLFunctions_4_5_Core' matches a version 4.5 core profile).
		 */
		template <class OpenGLFunctionsType>
		boost::optional<OpenGLFunctionsType *>
		get_version_functions(
				int major_version,
				int minor_version) const;
	};
}

#endif // GPLATES_OPENGL_GLCONTEXT_H
