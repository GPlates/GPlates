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

#ifndef GPLATES_OPENGL_GLVERTEXBUFFER_H
#define GPLATES_OPENGL_GLVERTEXBUFFER_H

#include <memory> // For std::auto_ptr
#include <vector>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <opengl/OpenGL.h>

#include "GLBuffer.h"


namespace GPlatesOpenGL
{
	class GLRenderer;

	/**
	 * An abstraction of the OpenGL buffer objects extension as used for vertex buffers containing
	 * vertex (attribute) data and *not* vertex element (indices) data.
	 */
	class GLVertexBuffer :
			public boost::enable_shared_from_this<GLVertexBuffer>
	{
	public:
		//
		// Note that the reason boost::shared_ptr is used instead of non_null_intrusive_ptr
		// is so these objects can be used with GPlatesUtils::ObjectCache.
		//

		//! A convenience typedef for a shared pointer to a non-const @a GLVertexBuffer.
		typedef boost::shared_ptr<GLVertexBuffer> shared_ptr_type;
		typedef boost::shared_ptr<const GLVertexBuffer> shared_ptr_to_const_type;

		//! A convenience typedef for a weak pointer to a @a GLVertexBuffer.
		typedef boost::weak_ptr<GLVertexBuffer> weak_ptr_type;
		typedef boost::weak_ptr<const GLVertexBuffer> weak_ptr_to_const_type;


		/**
		 * Creates a @a GLVertexBuffer object attached to the specified buffer.
		 *
		 * Note that is is possible to attach the same buffer object to a @a GLVertexBuffer
		 * and a @a GLVertexElementBuffer. This means vertices and indices are stored in the same buffer.
		 */
		static
		shared_ptr_type
		create(
				GLRenderer &renderer,
				const GLBuffer::shared_ptr_type &buffer)
		{
			return shared_ptr_type(create_as_auto_ptr(renderer, buffer).release());
		}

		/**
		 * Same as @a create but returns a std::auto_ptr - to guarantee only one owner.
		 */
		static
		std::auto_ptr<GLVertexBuffer>
		create_as_auto_ptr(
				GLRenderer &renderer,
				const GLBuffer::shared_ptr_type &buffer);


		virtual
		~GLVertexBuffer()
		{  }


		/**
		 * Returns the 'const' buffer used to store vertex attribute data (vertices).
		 */
		virtual
		GLBuffer::shared_ptr_to_const_type
		get_buffer() const = 0;

		/**
		 * Returns the 'non-const' buffer used to store vertex attribute data (vertices).
		 */
		GLBuffer::shared_ptr_type
		get_buffer()
		{
			return boost::const_pointer_cast<GLBuffer>(
					static_cast<const GLVertexBuffer*>(this)->get_buffer());
		}


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
				GLint offset) const = 0;


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
				GLint offset) const = 0;


		/**
		 * Binds the vertex normal data ('glNormalPointer') to 'this' vertex buffer.
		 */
		virtual
		void
		gl_normal_pointer(
				GLRenderer &renderer,
				GLenum type,
				GLsizei stride,
				GLint offset) const = 0;


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
				GLint offset) const = 0;


		/**
		 * Binds the specified *generic* vertex attribute data at attribute index @a attribute_index
		 * to 'this' vertex buffer.
		 *
		 * Note that generic attributes can be used in addition to the non-generic arrays or instead of.
		 * These *generic* attributes can only be accessed by shader programs (see @a GLProgramObject).
		 * The non-generic arrays can be accessed by both the fixed-function pipeline and shader programs.
		 * Although starting with OpenGL 3 the non-generic arrays are deprecated/removed from the core OpenGL profile.
		 * But graphics vendors support compatibility profiles so using them in a pre version 3 way is still ok.
		 *
		 * Note that, as dictated by OpenGL, @a attribute_index must be in the half-closed range
		 * [0, GL_MAX_VERTEX_ATTRIBS_ARB).
		 * You can get GL_MAX_VERTEX_ATTRIBS_ARB from 'context.get_capabilities().shader.gl_max_vertex_attribs'.
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
				GLint offset) const = 0;

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
				GLint offset) const = 0;

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
				GLint offset) const = 0;
	};
}

#endif // GPLATES_OPENGL_GLVERTEXBUFFER_H
