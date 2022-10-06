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
 
#ifndef GPLATES_OPENGL_GLTEXTURE_H
#define GPLATES_OPENGL_GLTEXTURE_H

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
	 * Wrapper around an OpenGL texture object.
	 */
	class GLTexture :
			public GLObject,
			public boost::enable_shared_from_this<GLTexture>
	{
	public:
		//
		// Note that the reason boost::shared_ptr is used instead of non_null_intrusive_ptr
		// is so these objects can be used with GPlatesUtils::ObjectCache.
		//

		//! A convenience typedef for a shared pointer to a @a GLTexture.
		typedef boost::shared_ptr<GLTexture> shared_ptr_type;
		typedef boost::shared_ptr<const GLTexture> shared_ptr_to_const_type;

		//! A convenience typedef for a weak pointer to a @a GLTexture.
		typedef boost::weak_ptr<GLTexture> weak_ptr_type;
		typedef boost::weak_ptr<const GLTexture> weak_ptr_to_const_type;


		/**
		 * Creates a shared pointer to a @a GLTexture object.
		 */
		static
		shared_ptr_type
		create(
				GL &gl,
				GLenum target)
		{
			return shared_ptr_type(new GLTexture(gl, target));
		}

		/**
		 * Same as @a create but returns a std::unique_ptr - to guarantee only one owner.
		 */
		static
		std::unique_ptr<GLTexture>
		create_unique(
				GL &gl,
				GLenum target)
		{
			return std::unique_ptr<GLTexture>(new GLTexture(gl, target));
		}


		/**
		 * Returns the target this texture was created with.
		 */
		GLenum
		get_target() const
		{
			return d_target;
		}


		/**
		 * Returns the texture resource handle.
		 */
		GLuint
		get_resource_handle() const;


	public:  // For use by the OpenGL framework...

		/**
		 * Policy class to allocate and deallocate OpenGL shader objects.
		 */
		class Allocator
		{
		public:
			static
			GLuint
			allocate(
					OpenGLFunctions &opengl_functions,
					GLenum target);

			static
			void
			deallocate(
					OpenGLFunctions &opengl_functions,
					GLuint);
		};

	private:
		GLenum d_target;
		GLObjectResource<GLuint, Allocator> d_resource;

		//! Constructor.
		GLTexture(
				GL &gl,
				GLenum target);
	};
}

#endif // GPLATES_OPENGL_GLTEXTURE_H
