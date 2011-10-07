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

#ifndef GPLATES_OPENGL_GLVERTEXBUFFEROBJECT_H
#define GPLATES_OPENGL_GLVERTEXBUFFEROBJECT_H

#include "GLBufferObject.h"
#include "GLObject.h"
#include "GLVertexBuffer.h"


namespace GPlatesOpenGL
{
	class GLRenderer;

	/**
	 * An OpenGL buffer object used to stored vertices (vertex attributes) but *not* vertex elements (indices).
	 *
	 * Requires the GL_ARB_vertex_buffer_object extension.
	 */
	class GLVertexBufferObject :
			public GLVertexBuffer,
			public GLObject
	{
	public:
		//
		// Note that the reason boost::shared_ptr is used instead of non_null_intrusive_ptr
		// is so these objects can be used with GPlatesUtils::ObjectCache.
		//

		//! A convenience typedef for a shared pointer to a @a GLVertexBufferObject.
		typedef boost::shared_ptr<GLVertexBufferObject> shared_ptr_type;
		typedef boost::shared_ptr<const GLVertexBufferObject> shared_ptr_to_const_type;

		//! A convenience typedef for a weak pointer to a @a GLVertexBufferObject.
		typedef boost::weak_ptr<GLVertexBufferObject> weak_ptr_type;
		typedef boost::weak_ptr<const GLVertexBufferObject> weak_ptr_to_const_type;


		/**
		 * Returns the target GL_ARRAY_BUFFER_ARB.
		 */
		static
		GLenum
		get_target_type();


		/**
		 * Creates a shared pointer to a @a GLVertexBufferObject object.
		 */
		static
		shared_ptr_type
		create(
				GLRenderer &renderer,
				const GLBufferObject::shared_ptr_type &buffer)
		{
			return shared_ptr_type(create_as_auto_ptr(renderer, buffer).release());
		}

		/**
		 * Same as @a create but returns a std::auto_ptr - to guarantee only one owner.
		 */
		static
		std::auto_ptr<GLVertexBufferObject>
		create_as_auto_ptr(
				GLRenderer &renderer,
				const GLBufferObject::shared_ptr_type &buffer)
		{
			return std::auto_ptr<GLVertexBufferObject>(new GLVertexBufferObject(buffer));
		}


		/**
		 * Returns the buffer used to store vertex attribute data (vertices).
		 */
		virtual
		GLBuffer::shared_ptr_to_const_type
		get_buffer() const;


		/**
		 * Binds the vertex position data ('glVertexPointer') to 'this' vertex buffer.
		 */
		virtual
		void
		gl_vertex_pointer(
				GLRenderer &renderer,
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint offset) const;


		/**
		 * Binds the vertex color data ('glColorPointer') to 'this' vertex buffer.
		 */
		virtual
		void
		gl_color_pointer(
				GLRenderer &renderer,
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint offset) const;


		/**
		 * Binds the vertex normal data ('glNormalPointer') to 'this' vertex buffer.
		 */
		virtual
		void
		gl_normal_pointer(
				GLRenderer &renderer,
				GLenum type,
				GLsizei stride,
				GLint offset) const;


		/**
		 * Binds the vertex texture coordinate data ('glTexCoordPointer') to 'this' vertex buffer.
		 */
		virtual
		void
		gl_tex_coord_pointer(
				GLRenderer &renderer,
				GLenum texture_unit,
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint offset) const;


		/**
		 * Binds the specified *generic* vertex attribute data at attribute index @a attribute_index
		 * to 'this' vertex buffer.
		 *
		 * NOTE: The 'GL_ARB_vertex_shader' extension must be supported.
		 */
		virtual
		void
		gl_vertex_attrib_pointer(
				GLRenderer &renderer,
				GLuint attribute_index,
				GLint size,
				GLenum type,
				GLboolean normalized,
				GLsizei stride,
				GLint offset) const;


		/**
		 * Returns the buffer object.
		 */
		GLBufferObject::shared_ptr_to_const_type
		get_buffer_object() const
		{
			return d_buffer;
		}

	private:
		GLBufferObject::shared_ptr_type d_buffer;


		//! Constructor.
		GLVertexBufferObject(
				const GLBufferObject::shared_ptr_type &buffer);
	};
}

#endif // GPLATES_OPENGL_GLVERTEXBUFFEROBJECT_H
