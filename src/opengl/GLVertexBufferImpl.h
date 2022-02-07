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

#ifndef GPLATES_OPENGL_GLVERTEXBUFFERIMPL_H
#define GPLATES_OPENGL_GLVERTEXBUFFERIMPL_H

#include <opengl/OpenGL.h>

#include "GLBufferImpl.h"
#include "GLVertexBuffer.h"


namespace GPlatesOpenGL
{
	class GLRenderer;

	/**
	 * An implementation of the OpenGL buffer objects extension as used for vertex buffers containing
	 * vertex (attribute) data and *not* vertex element (indices) data.
	 *
	 * This implementation is used when the OpenGL extension is not supported - in which case
	 * vertex buffer objects are simulated by using client-side memory arrays in a base OpenGL 1.1 way.
	 */
	class GLVertexBufferImpl :
			public GLVertexBuffer
	{
	public:
		//! A convenience typedef for a shared pointer to a @a GLVertexBufferImpl.
		typedef boost::shared_ptr<GLVertexBufferImpl> shared_ptr_type;
		typedef boost::shared_ptr<const GLVertexBufferImpl> shared_ptr_to_const_type;


		/**
		 * Creates a @a GLVertexBufferImpl object attached to the specified buffer.
		 */
		static
		shared_ptr_type
		create(
				GLRenderer &renderer,
				const GLBufferImpl::shared_ptr_type &buffer)
		{
			return shared_ptr_type(create_as_auto_ptr(renderer, buffer).release());
		}

		/**
		 * Same as @a create but returns a std::auto_ptr - to guarantee only one owner.
		 */
		static
		std::auto_ptr<GLVertexBufferImpl>
		create_as_auto_ptr(
				GLRenderer &renderer,
				const GLBufferImpl::shared_ptr_type &buffer)
		{
			return std::auto_ptr<GLVertexBufferImpl>(new GLVertexBufferImpl(renderer, buffer));
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
		 * Same as @a gl_vertex_attrib_pointer except used to specify attributes mapping to *integer* shader variables.
		 *
		 * NOTE: The 'GL_ARB_vertex_shader' *and* 'GL_EXT_gpu_shader4' extensions must be supported.
		 */
		virtual
		void
		gl_vertex_attrib_i_pointer(
				GLRenderer &renderer,
				GLuint attribute_index,
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint offset) const;

		/**
		 * Same as @a gl_vertex_attrib_pointer except used to specify attributes mapping to *double* shader variables.
		 *
		 * NOTE: The 'GL_ARB_vertex_shader' *and* 'GL_ARB_vertex_attrib_64bit' extensions must be supported.
		 */
		virtual
		void
		gl_vertex_attrib_l_pointer(
				GLRenderer &renderer,
				GLuint attribute_index,
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint offset) const;

	private:
		//! The buffer being targeted by this vertex buffer.
		GLBufferImpl::shared_ptr_type d_buffer;


		//! Constructor.
		explicit
		GLVertexBufferImpl(
				GLRenderer &renderer,
				const GLBufferImpl::shared_ptr_type &buffer);
	};
}

#endif // GPLATES_OPENGL_GLVERTEXBUFFERIMPL_H
