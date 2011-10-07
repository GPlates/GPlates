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

#ifndef GPLATES_OPENGL_GLVERTEXELEMENTBUFFER_H
#define GPLATES_OPENGL_GLVERTEXELEMENTBUFFER_H

#include <vector>
#include <boost/cstdint.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/integer_traits.hpp>
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
	 * Traits class to find the size of a vertex element from its type.
	 */
	template <typename VertexElementType>
	struct GLVertexElementTraits; // Unspecialised class intentionally not defined.

	template <>
	struct GLVertexElementTraits<GLubyte>
	{
		//! GL_UNSIGNED_BYTE
		static const GLenum type;
		//! The maximum number of vertices that can be indexed.
		static const unsigned int MAX_INDEXABLE_VERTEX = boost::integer_traits<boost::uint8_t>::const_max;
	};

	template <>
	struct GLVertexElementTraits<GLushort>
	{
		//! GL_UNSIGNED_SHORT
		static const GLenum type;
		//! The maximum number of vertices that can be indexed.
		static const unsigned int MAX_INDEXABLE_VERTEX = boost::integer_traits<boost::uint16_t>::const_max;
	};

	template <>
	struct GLVertexElementTraits<GLuint>
	{
		//! GL_UNSIGNED_INT
		static const GLenum type;
		//! The maximum number of vertices that can be indexed.
		static const unsigned int MAX_INDEXABLE_VERTEX = boost::integer_traits<boost::uint32_t>::const_max;
	};


	/**
	 * An abstraction of the OpenGL buffer objects extension as used for vertex element buffers
	 * containing vertex element (index) data and *not* vertex attribute (vertices) data.
	 *
	 * This implementation is used when the OpenGL extension is not supported - in which case
	 * vertex buffer objects are simulated by using client-side memory arrays in a base OpenGL 1.1 way.
	 */
	class GLVertexElementBuffer :
			public boost::enable_shared_from_this<GLVertexElementBuffer>
	{
	public:
		//
		// Note that the reason boost::shared_ptr is used instead of non_null_intrusive_ptr
		// is so these objects can be used with GPlatesUtils::ObjectCache.
		//

		//! A convenience typedef for a shared pointer to a non-const @a GLVertexElementBuffer.
		typedef boost::shared_ptr<GLVertexElementBuffer> shared_ptr_type;
		typedef boost::shared_ptr<const GLVertexElementBuffer> shared_ptr_to_const_type;

		//! A convenience typedef for a weak pointer to a @a GLVertexElementBuffer.
		typedef boost::weak_ptr<GLVertexElementBuffer> weak_ptr_type;
		typedef boost::weak_ptr<const GLVertexElementBuffer> weak_ptr_to_const_type;


		/**
		 * Creates a @a GLVertexElementBuffer object attached to the specified buffer.
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
		std::auto_ptr<GLVertexElementBuffer>
		create_as_auto_ptr(
				GLRenderer &renderer,
				const GLBuffer::shared_ptr_type &buffer);


		virtual
		~GLVertexElementBuffer()
		{  }


		/**
		 * Returns the 'const' buffer used to store vertex element data (indices).
		 */
		virtual
		GLBuffer::shared_ptr_to_const_type
		get_buffer() const = 0;

		/**
		 * Returns the 'non-const' buffer used to store vertex element data (indices).
		 */
		GLBuffer::shared_ptr_type
		get_buffer()
		{
			return boost::const_pointer_cast<GLBuffer>(
					static_cast<const GLVertexElementBuffer*>(this)->get_buffer());
		}


		/**
		 * Binds this vertex element buffer so that vertex element data is sourced from it.
		 */
		virtual
		void
		gl_bind(
				GLRenderer &renderer) const = 0;


		/**
		 * Performs the equivalent of the OpenGL command 'glDrawRangeElements'.
		 *
		 * @a indices_offset is a byte offset from the start of 'this' indices array.
		 */
		virtual
		void
		gl_draw_range_elements(
				GLRenderer &renderer,
				GLenum mode,
				GLuint start,
				GLuint end,
				GLsizei count,
				GLenum type,
				GLint indices_offset) const = 0;
	};
}

#endif // GPLATES_OPENGL_GLVERTEXELEMENTBUFFER_H
