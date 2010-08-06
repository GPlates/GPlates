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

#include <opengl/OpenGL.h>

#include "GLResourceManager.h"
#include "GLState.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	/**
	 * Represents the OpenGL graphics state, transform state, texture state
	 * and drawables as a graph.
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
		 * OpenGL state that can be shared between contexts (such as display lists,
		 * texture objects, vertex buffer objects, etc).
		 */
		class SharedState
		{
		public:
			//! Constructor.
			SharedState();

			/**
			 * Returns the texture resource manager.
			 */
			boost::shared_ptr<GLTextureResourceManager>
			get_texture_resource_manager()
			{
				return d_texture_resource_manager;
			}

		private:
			boost::shared_ptr<GLTextureResourceManager> d_texture_resource_manager;
		};


		/**
		 * Creates a @a GLContext object.
		 */
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new GLContext());
		}


		/**
		 * Creates a @a GLContext object that shares state with another context.
		 */
		static
		non_null_ptr_type
		create(
				GLContext &context)
		{
			return non_null_ptr_type(new GLContext(context.get_shared_state()));
		}


		/**
		 * Initialises this @a GLContext.
		 *
		 * NOTE: An OpenGL context must be current before this is called.
		 */
		void
		initialise();


		/**
		 * Call this before rendering a scene.
		 */
		void
		begin_render()
		{
			get_shared_state()->get_texture_resource_manager()->deallocate_queued_resources();
		}


		/**
		 * Call this after rendering a scene.
		 */
		void
		end_render()
		{
			get_shared_state()->get_texture_resource_manager()->deallocate_queued_resources();
		}


		boost::shared_ptr<const SharedState>
		get_shared_state() const
		{
			return d_shared_state;
		}

		boost::shared_ptr<SharedState>
		get_shared_state()
		{
			return d_shared_state;
		}


		const GLState &
		get_state() const
		{
			return d_state;
		}

		GLState &
		get_state()
		{
			return d_state;
		}

		/**
		 * Returns the maximum number of texture units supported by the
		 * GL_ARB_multitexture extension (or one if it's not supported).
		 *
		 * @throws PreconditionViolationError if @a initialise not yet called.
		 * This method is static only to avoid having to pass around a @a GLContext.
		 */
		static
		std::size_t
		get_max_texture_units_ARB();

		/**
		 * Simply returns GL_TEXTURE0_ARB.
		 *
		 * This method is here solely so we can include <GL/glew.h>, which defines
		 * GL_TEXTURE0_ARB, in "GLContext.cc" and hence avoid problems caused by
		 * including <GL/glew.h> in header files (because <GL/glew.h> must be included
		 * before OpenGL headers which means before Qt headers which is difficult).
		 */
		static
		GLenum
		get_GL_TEXTURE0_ARB();

	private:
		/**
		 * OpenGL state that not shared between contexts.
		 */
		GLState d_state;

		/**
		 * OpenGL state that can be shared with another context.
		 */
		boost::shared_ptr<SharedState> d_shared_state;

		/**
		 * Is true if the GLEW library has been initialised (if @a initialise has been called).
		 */
		static bool s_initialised_GLEW;

		/**
		 * The maximum number of texture units supported by the
		 * GL_ARB_multitexture extension (or one if it's not supported).
		 */
		static GLint s_max_texture_units;


		//! Default constructor.
		GLContext() :
			d_shared_state(new SharedState())
		{  }

		//! Constructor.
		explicit
		GLContext(
				const boost::shared_ptr<SharedState> &shared_state) :
			d_shared_state(shared_state)
		{  }
	};
}

#endif // GPLATES_OPENGL_GLCONTEXT_H
