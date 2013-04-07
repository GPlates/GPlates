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

#ifndef GPLATES_OPENGL_GLVERTEXELEMENTBUFFERIMPL_H
#define GPLATES_OPENGL_GLVERTEXELEMENTBUFFERIMPL_H

#include <opengl/OpenGL.h>

#include "GLBufferImpl.h"
#include "GLVertexElementBuffer.h"


namespace GPlatesOpenGL
{
	class GLRenderer;

	/**
	 * An implementation of the OpenGL buffer objects extension as used for vertex buffers containing
	 * vertex element (indices) data and *not* vertex attribute (vertices) data.
	 *
	 * This implementation is used when the OpenGL extension is not supported - in which case
	 * vertex buffer objects are simulated by using client-side memory arrays in a base OpenGL 1.1 way.
	 */
	class GLVertexElementBufferImpl :
			public GLVertexElementBuffer
	{
	public:
		//! A convenience typedef for a shared pointer to a @a GLVertexElementBufferImpl.
		typedef boost::shared_ptr<GLVertexElementBufferImpl> shared_ptr_type;
		typedef boost::shared_ptr<const GLVertexElementBufferImpl> shared_ptr_to_const_type;


		/**
		 * Creates a @a GLVertexElementBufferImpl object attached to the specified buffer.
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
		std::auto_ptr<GLVertexElementBufferImpl>
		create_as_auto_ptr(
				GLRenderer &renderer,
				const GLBufferImpl::shared_ptr_type &buffer)
		{
			return std::auto_ptr<GLVertexElementBufferImpl>(new GLVertexElementBufferImpl(renderer, buffer));
		}


		/**
		 * Returns the buffer used to store vertex element data (indices).
		 */
		virtual
		GLBuffer::shared_ptr_to_const_type
		get_buffer() const;


		/**
		 * Binds this vertex element buffer so that vertex element data is sourced from it.
		 */
		virtual
		void
		gl_bind(
				GLRenderer &renderer) const;

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
				GLint indices_offset) const;

	private:
		//! The buffer being targeted by this vertex element buffer.
		GLBufferImpl::shared_ptr_type d_buffer;


		//! Constructor.
		explicit
		GLVertexElementBufferImpl(
				GLRenderer &renderer,
				const GLBufferImpl::shared_ptr_type &buffer);
	};
}

#endif // GPLATES_OPENGL_GLVERTEXELEMENTBUFFERIMPL_H
