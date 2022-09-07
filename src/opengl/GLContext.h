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
// For OpenGL constants and typedefs...
// Note: Cannot include "OpenGL.h" due to cyclic dependency with class GL.
#include <qopengl.h>
#include <QPair>
#include <QOpenGLContext>
#include <QOpenGLVersionFunctions>  // For QAbstractOpenGLFunctions
#include <QOpenGLVersionProfile>
#include <QOpenGLWindow>
#include <QSurfaceFormat>

#include "GLBuffer.h"
#include "GLCapabilities.h"
#include "GLFramebuffer.h"
#include "GLProgram.h"
#include "GLRenderbuffer.h"
#include "GLSampler.h"
#include "GLShader.h"
#include "GLStateStore.h"
#include "GLTexture.h"
#include "GLVertexArray.h"

#include "global/PointerTraits.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ObjectCache.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	class GL;
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
		 * OpenGL state that can be shared between contexts (such as texture objects, buffer objects, etc).
		 */
		class SharedState
		{
		public:
			//! Constructor.
			SharedState();


			/**
			 * Returns the buffer resource manager.
			 *
			 * The returned resource manager can create an OpenGL buffer object (eg, vertex, pixel).
			 */
			const boost::shared_ptr<GLBuffer::resource_manager_type> &
			get_buffer_resource_manager() const
			{
				return d_buffer_resource_manager;
			}

			/**
			 * Returns the framebuffer resource manager.
			 */
			const boost::shared_ptr<GLFramebuffer::resource_manager_type> &
			get_framebuffer_resource_manager() const
			{
				return d_framebuffer_resource_manager;
			}

			/**
			 * Returns the shader program resource manager.
			 */
			const boost::shared_ptr<GLProgram::resource_manager_type> &
			get_program_resource_manager() const
			{
				return d_program_resource_manager;
			}

			/**
			 * Returns the renderbuffer resource manager.
			 */
			const boost::shared_ptr<GLRenderbuffer::resource_manager_type> &
			get_renderbuffer_resource_manager() const
			{
				return d_renderbuffer_resource_manager;
			}

			/**
			 * Returns the sampler resource manager.
			 */
			const boost::shared_ptr<GLSampler::resource_manager_type> &
			get_sampler_resource_manager() const
			{
				return d_sampler_resource_manager;
			}

			/**
			 * Returns the shader resource manager.
			 */
			const boost::shared_ptr<GLShader::resource_manager_type> &
			get_shader_resource_manager() const
			{
				return d_shader_resource_manager;
			}

			/**
			 * Returns the texture resource manager.
			 */
			const boost::shared_ptr<GLTexture::resource_manager_type> &
			get_texture_resource_manager() const
			{
				return d_texture_resource_manager;
			}

			/**
			 * Returns the vertex array resource manager.
			 */
			const boost::shared_ptr<GLVertexArray::resource_manager_type> &
			get_vertex_array_resource_manager() const
			{
				return d_vertex_array_resource_manager;
			}


			/**
			 * Returns a vertex array for rendering a full-screen quad.
			 *
			 * The full-screen quad contains positions only (at vertex attribute index 0) covering
			 * range [-1,1] for x and y (and where z = 0).
			 *
			 * The returned vertex array can be used to draw a full-screen quad in order to apply
			 * a texture to the screen-space of a render target (eg, using GLSL built-in 'gl_FragCoord').
			 *
			 * It can be rendered as:
			 *
			 *   gl.BindVertexArray(vertex_array);
			 *   gl.DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			 *
			 * Note: This also creates a new vertex buffer (but no vertex element buffer) that is
			 *       kept alive by the returned vertex array.
			 *
			 * NOTE: This method maintains a single instance of a full-screen quad that clients can use.
			 */
			GLVertexArray::shared_ptr_type
			get_full_screen_quad(
					GL &gl);

		private:

			boost::shared_ptr<GLBuffer::resource_manager_type> d_buffer_resource_manager;
			boost::shared_ptr<GLFramebuffer::resource_manager_type> d_framebuffer_resource_manager;
			boost::shared_ptr<GLProgram::resource_manager_type> d_program_resource_manager;
			boost::shared_ptr<GLRenderbuffer::resource_manager_type> d_renderbuffer_resource_manager;
			boost::shared_ptr<GLSampler::resource_manager_type> d_sampler_resource_manager;
			boost::shared_ptr<GLShader::resource_manager_type> d_shader_resource_manager;
			boost::shared_ptr<GLTexture::resource_manager_type> d_texture_resource_manager;
			boost::shared_ptr<GLVertexArray::resource_manager_type> d_vertex_array_resource_manager;

			/**
			 * Used by @a GL to efficiently allocate @a GLState objects.
			 *
			 * It's optional because we can't create it until we have a valid OpenGL context.
			 * NOTE: Access using @a get_state_store.
			 */
			boost::optional<GLStateStore::non_null_ptr_type> d_state_store;

			/**
			 * The vertex array representing a full screen quad.
			 */
			boost::optional<GLVertexArray::shared_ptr_type> d_full_screen_quad;

			// To access @a d_state_set_store and @a d_state_set_keys without exposing to clients.
			friend class GLContext;


			//! Create state store if not yet done - an OpenGL context must be valid.
			GLStateStore::non_null_ptr_type
			get_state_store(
					const GLCapabilities &capabilities);
		};


		/**
		 * Set the global default surface format (eg, used by QOpenGLWidget's).
		 *
		 * This sets various parameters required for OpenGL rendering in GPlates.
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
		 * Access OpenGL.
		 *
		 * The returned object should be used as a local object (on C++ runtime stack) and only
		 * created/used when need to render something (it should not be stored persistently).
		 *
		 * Typically a @a GL is created each time a frame needs drawing.
		 */
		GPlatesGlobal::PointerTraits<GL>::non_null_ptr_type
		access_opengl();


		/**
		 * Returns the OpenGL state that can be shared with other OpenGL contexts.
		 *
		 * You can compare the pointers returned by @a get_shared_state on two
		 * different contexts to see if they are sharing or not.
		 */
		boost::shared_ptr<const SharedState>
		get_shared_state() const
		{
			return d_shared_state;
		}

		/**
		 * Returns the OpenGL state that can be shared with other OpenGL contexts.
		 *
		 * You can compare the pointers returned by @a get_shared_state on two
		 * different contexts to see if they are sharing or not.
		 */
		boost::shared_ptr<SharedState>
		get_shared_state()
		{
			return d_shared_state;
		}


		/**
		 * Function to return OpenGL implementation-dependent capabilities and parameters.
		 *
		 * @throws PreconditionViolationError if @a initialise_gl not yet called.
		 */
		const GLCapabilities &
		get_capabilities() const;


		/**
		 * Call this before rendering a scene.
		 *
		 * NOTE: Only @a GL need call this.
		 */
		void
		begin_render();


		/**
		 * Call this after rendering a scene.
		 *
		 * NOTE: Only @a GL need call this.
		 */
		void
		end_render();


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
		 * OpenGL state that can be shared with another context.
		 */
		boost::shared_ptr<SharedState> d_shared_state;


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

		/**
		 * Deallocates OpenGL objects that have been released but not yet destroyed/deallocated.
		 *
		 * They are queued for deallocation so that it can be done at a time when the OpenGL
		 * context is known to be active and hence OpenGL calls can be made.
		 */
		void
		deallocate_queued_object_resources();
	};
}

#endif // GPLATES_OPENGL_GLCONTEXT_H
