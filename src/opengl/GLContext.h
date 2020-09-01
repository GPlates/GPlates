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
#include <opengl/OpenGL1.h>
#include <QGLFormat>

#include "GLBuffer.h"
#include "GLCapabilities.h"
#include "GLCompiledDrawState.h"
#include "GLFramebuffer.h"
#include "GLProgramObject.h"
#include "GLRenderbuffer.h"
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
		 * Used to delegate to the real OpenGL context.
		 */
		class Impl
		{
		public:
			virtual
			~Impl()
			{  }

			//! Make this context the current context.
			virtual
			void
			make_current() = 0;

			//! Return the QGLFormat of the QGLContext OpenGL context.
			virtual
			const QGLFormat
			get_qgl_format() const = 0;

			//! The width of the framebuffer currently attached to the OpenGL context.
			virtual
			unsigned int
			get_width() const = 0;

			//! The height of the framebuffer currently attached to the OpenGL context.
			virtual
			unsigned int
			get_height() const = 0;
		};


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
			 * Returns the shader program resource manager.
			 */
			const boost::shared_ptr<GLProgramObject::resource_manager_type> &
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
			get_texture_object_resource_manager() const
			{
				return d_texture_object_resource_manager;
			}


			/**
			 * Creates a compiled draw state that can render a full-screen textured quad.
			 *
			 * The vertex colours are white - RGBA(1.0, 1.0, 1.0, 1.0).
			 *
			 * The returned compiled draw state can be used to draw a full-screen quad in order apply a
			 * texture to the screen-space of a render target.
			 *
			 * NOTE: This method maintains a single instance of a full-screen quad that clients can use.
			 * Which is why it is returned as a shared pointer to 'const'.
			 */
			GLCompiledDrawState::non_null_ptr_to_const_type
			get_full_screen_2D_textured_quad(
					GL &gl);

		private:

			boost::shared_ptr<GLBuffer::resource_manager_type> d_buffer_resource_manager;
			boost::shared_ptr<GLProgramObject::resource_manager_type> d_program_resource_manager;
			boost::shared_ptr<GLRenderbuffer::resource_manager_type> d_renderbuffer_resource_manager;
			boost::shared_ptr<GLShader::resource_manager_type> d_shader_resource_manager;
			boost::shared_ptr<GLTexture::resource_manager_type> d_texture_object_resource_manager;

			/**
			 * Used by @a GL to efficiently allocate @a GLState objects.
			 *
			 * It's optional because we can't create it until we have a valid OpenGL context.
			 * NOTE: Access using @a get_state_store.
			 */
			boost::optional<GLStateStore::non_null_ptr_type> d_state_store;

			/**
			 * The compiled draw state representing a full screen textured quad.
			 */
			boost::optional<GLCompiledDrawState::non_null_ptr_to_const_type> d_full_screen_2D_textured_quad;

			// To access @a d_state_set_store and @a d_state_set_keys without exposing to clients.
			friend class GLContext;


			//! Create state store if not yet done - an OpenGL context must be valid.
			GLStateStore::non_null_ptr_type
			get_state_store(
					const GLCapabilities &capabilities);
		};


		/**
		 * OpenGL state that *cannot* be shared between contexts.
		 *
		 * This is typically *container* objects such as vertex array objects and framebuffer objects.
		 *
		 * Note that while framebuffer objects cannot be shared across contexts their contained objects
		 * (textures and renderbuffers) can be shared across contexts.
		 * Similarly for vertex array objects (with contained buffer objects).
		 */
		class NonSharedState
		{
		public:
			//! Constructor.
			NonSharedState();


			/**
			 * Returns the framebuffer resource manager.
			 *
			 * This is used by @a GLFramebuffer to allocate a native vertex array object per OpenGL context.
			 *
			 * Note that even though @a GLFramebuffer objects are implemented such that they can be used
			 * freely across different OpenGL contexts, the native OpenGL framebuffer objects (resources)
			 * contained within them can only be used in the context they were created in.
			 * Hence they are allocated here (in @a NonSharedState, rather than @a SharedState).
			 * This ensures the native resources are deallocated in the context they were allocated in
			 * (ie, deallocated when the correct context is active).
			 */
			const boost::shared_ptr<GLFramebuffer::resource_manager_type> &
			get_framebuffer_resource_manager() const
			{
				return d_framebuffer_resource_manager;
			}

			/**
			 * Returns the vertex array resource manager.
			 *
			 * This is used by @a GLVertexArray to allocate a native vertex array object per OpenGL context.
			 *
			 * Note that even though @a GLVertexArray objects are implemented such that they can be used
			 * freely across different OpenGL contexts, the native OpenGL vertex array objects (resources)
			 * contained within them can only be used in the context they were created in.
			 * Hence they are allocated here (in @a NonSharedState, rather than @a SharedState).
			 * This ensures the native resources are deallocated in the context they were allocated in
			 * (ie, deallocated when the correct context is active).
			 */
			const boost::shared_ptr<GLVertexArray::resource_manager_type> &
			get_vertex_array_resource_manager() const
			{
				return d_vertex_array_resource_manager;
			}

		private:

			boost::shared_ptr<GLFramebuffer::resource_manager_type> d_framebuffer_resource_manager;
			boost::shared_ptr<GLVertexArray::resource_manager_type> d_vertex_array_resource_manager;
		};


		/**
		 * Returns the QGLFormat to use when creating a Qt OpenGL context (eg, QGLWidget).
		 *
		 * This sets various parameters required for OpenGL rendering in GPlates.
		 * Such as specifying an alpha-channel.
		 */
		static
		QGLFormat
		get_qgl_format_to_create_context_with();


		/**
		 * Creates a @a GLContext object.
		 */
		static
		non_null_ptr_type
		create(
				const boost::shared_ptr<Impl> &context_impl)
		{
			return non_null_ptr_type(new GLContext(context_impl));
		}


		/**
		 * Creates a @a GLContext object that shares state with another context.
		 */
		static
		non_null_ptr_type
		create(
				const boost::shared_ptr<Impl> &context_impl,
				GLContext &shared_context)
		{
			return non_null_ptr_type(new GLContext(context_impl, shared_context.get_shared_state()));
		}


		/**
		 * Initialises this @a GLContext.
		 *
		 * NOTE: An OpenGL context must be current before this is called.
		 */
		void
		initialise();


		/**
		 * Sets this context as the active OpenGL context.
		 */
		void
		make_current()
		{
			d_context_impl->make_current();
		}


		/**
		 * Returns the QGLFormat of the QGLContext OpenGL context.
		 *
		 * This can be used to determine the number of colour/depth/stencil bits in the framebuffer.
		 */
		const QGLFormat &
		get_qgl_format() const
		{
			return d_qgl_format;
		}


		/**
		 * The width (in device pixels) of the framebuffer currently attached to the OpenGL context.
		 *
		 * NOTE: Dimensions, in OpenGL, are in device pixels (not the device independent pixels used for widget sizes).
		 */
		unsigned int
		get_width() const
		{
			return d_context_impl->get_width();
		}


		/**
		 * The height (in device pixels) of the framebuffer currently attached to the OpenGL context.
		 *
		 * NOTE: Dimensions, in OpenGL, are in device pixels (not the device independent pixels used for widget sizes).
		 */
		unsigned int
		get_height() const
		{
			return d_context_impl->get_height();
		}


		/**
		 * Creates a @a GL.
		 *
		 * This is the interface through which OpenGL is used to draw.
		 *
		 * Typically a @a GL is created each time a frame needs drawing.
		 */
		GPlatesGlobal::PointerTraits<GL>::non_null_ptr_type
		create_gl();


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
		 * Returns the OpenGL state that *cannot* be shared with other OpenGL contexts.
		 */
		boost::shared_ptr<const NonSharedState>
		get_non_shared_state() const
		{
			return d_non_shared_state;
		}

		/**
		 * Returns the OpenGL state that *cannot* be shared with other OpenGL contexts.
		 */
		boost::shared_ptr<NonSharedState>
		get_non_shared_state()
		{
			return d_non_shared_state;
		}

		/**
		 * Function to return OpenGL implementation-dependent capabilities and parameters.
		 *
		 * NOTE: This used to be a static method (to enable global access).
		 * However it is now non-static to force clients to access a valid GLContext object.
		 * This ensures that the GLEW (and hence these parameters) have been initialised before access.
		 *
		 * @throws PreconditionViolationError if @a initialise not yet called.
		 * This method is static only to avoid having to pass around a @a GLContext.
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
		 * For delegating to the real OpenGL context.
		 */
		boost::shared_ptr<Impl> d_context_impl;

		/**
		 * The format of the OpenGL context.
		 */
		QGLFormat d_qgl_format;

		/**
		 * OpenGL state that can be shared with another context.
		 */
		boost::shared_ptr<SharedState> d_shared_state;

		/**
		 * OpenGL state that *cannot* be shared with another context.
		 */
		boost::shared_ptr<NonSharedState> d_non_shared_state;

		/**
		 * Is true if the GLEW library has been initialised (if @a initialise has been called).
		 */
		static bool s_initialised_GLEW;

		/**
		 * OpenGL implementation-dependent capabilities and parameters.
		 */
		static GLCapabilities s_capabilities;


		//! Constructor.
		explicit
		GLContext(
				const boost::shared_ptr<Impl> &context_impl) :
			d_context_impl(context_impl),
			d_qgl_format(context_impl->get_qgl_format()),
			d_shared_state(new SharedState()),
			d_non_shared_state(new NonSharedState())
		{  }

		//! Constructor.
		GLContext(
				const boost::shared_ptr<Impl> &context_impl,
				const boost::shared_ptr<SharedState> &shared_state) :
			d_context_impl(context_impl),
			d_qgl_format(context_impl->get_qgl_format()),
			d_shared_state(shared_state),
			d_non_shared_state(new NonSharedState())
		{  }

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
