/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_OPENGL_GLBUFFER_H
#define GPLATES_OPENGL_GLBUFFER_H

#include <memory> // For std::unique_ptr
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
// For OpenGL constants and typedefs...
// Note: Cannot include "OpenGL.h" due to cyclic dependency with class GL.
#include <qopengl.h>

#include "GLObject.h"
#include "GLObjectResource.h"


namespace GPlatesOpenGL
{
	class GL;
	class OpenGLFunctions;

	/**
	 * Wrapper around an OpenGL buffer object.
	 */
	class GLBuffer :
			public GLObject,
			public boost::enable_shared_from_this<GLBuffer>
	{
	public:
		//
		// Note that the reason boost::shared_ptr is used instead of non_null_intrusive_ptr
		// is so these objects can be used with GPlatesUtils::ObjectCache.
		//

		//! A convenience typedef for a shared pointer to a non-const @a GLBuffer.
		typedef boost::shared_ptr<GLBuffer> shared_ptr_type;
		typedef boost::shared_ptr<const GLBuffer> shared_ptr_to_const_type;

		//! A convenience typedef for a weak pointer to a @a GLBuffer.
		typedef boost::weak_ptr<GLBuffer> weak_ptr_type;
		typedef boost::weak_ptr<const GLBuffer> weak_ptr_to_const_type;


		/**
		 * Creates a @a GLBuffer object.
		 */
		static
		shared_ptr_type
		create(
				GL &gl)
		{
			return shared_ptr_type(create_unique(gl).release());
		}

		/**
		 * Same as @a create but returns a std::unique_ptr - to guarantee only one owner.
		 */
		static
		std::unique_ptr<GLBuffer>
		create_unique(
				GL &gl)
		{
			return std::unique_ptr<GLBuffer>(new GLBuffer(gl));
		}

		/**
		 * Returns the buffer resource handle.
		 */
		GLuint
		get_resource_handle() const;

	public:  // For use by the OpenGL framework...

		/**
		 * Policy class to allocate and deallocate OpenGL buffer objects.
		 */
		class Allocator
		{
		public:
			static
			GLuint
			allocate(
					OpenGLFunctions &opengl_functions);

			static
			void
			deallocate(
					OpenGLFunctions &opengl_functions,
					GLuint);
		};

	private:
		GLObjectResource<GLuint, Allocator> d_resource;

		//! Constructor.
		explicit
		GLBuffer(
				GL &gl);
	};
}

#endif // GPLATES_OPENGL_GLBUFFER_H
