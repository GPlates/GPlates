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

#ifndef GPLATES_OPENGL_GLVERTEXELEMENTBUFFEROBJECT_H
#define GPLATES_OPENGL_GLVERTEXELEMENTBUFFEROBJECT_H

#include "GLBufferObject.h"
#include "GLObject.h"
#include "GLVertexElementBuffer.h"


namespace GPlatesOpenGL
{
	class GLRenderer;

	/**
	 * An OpenGL buffer object used to stored vertex elements (vertex indices) but *not* vertex
	 * attributes (vertices).
	 *
	 * Requires the GL_ARB_vertex_buffer_object extension.
	 */
	class GLVertexElementBufferObject :
			public GLVertexElementBuffer,
			public GLObject
	{
	public:
		//
		// Note that the reason boost::shared_ptr is used instead of non_null_intrusive_ptr
		// is so these objects can be used with GPlatesUtils::ObjectCache.
		//

		//! A convenience typedef for a shared pointer to a @a GLVertexElementBufferObject.
		typedef boost::shared_ptr<GLVertexElementBufferObject> shared_ptr_type;
		typedef boost::shared_ptr<const GLVertexElementBufferObject> shared_ptr_to_const_type;

		//! A convenience typedef for a weak pointer to a @a GLVertexElementBufferObject.
		typedef boost::weak_ptr<GLVertexElementBufferObject> weak_ptr_type;
		typedef boost::weak_ptr<const GLVertexElementBufferObject> weak_ptr_to_const_type;


		/**
		 * Returns the target GL_ELEMENT_ARRAY_BUFFER_ARB.
		 */
		static
		GLenum
		get_target_type();


		/**
		 * Creates a shared pointer to a @a GLVertexElementBufferObject object.
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
		std::auto_ptr<GLVertexElementBufferObject>
		create_as_auto_ptr(
				GLRenderer &renderer,
				const GLBufferObject::shared_ptr_type &buffer)
		{
			return std::auto_ptr<GLVertexElementBufferObject>(new GLVertexElementBufferObject(renderer, buffer));
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
		 * NOTE: If the GL_EXT_draw_range_elements extension is not present then reverts to
		 * using @a gl_draw_elements instead (ie, @a start and @a end are effectively ignored).
		 *
		 * @a indices_offset is a byte offset from the start of 'this' indices array.
		 *
		 * This function can be more efficient for OpenGL than @a gl_draw_elements since
		 * you are guaranteeing that the range of vertex indices is bounded by [start, end].
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
		GLVertexElementBufferObject(
				GLRenderer &renderer,
				const GLBufferObject::shared_ptr_type &buffer);
	};
}

#endif // GPLATES_OPENGL_GLVERTEXELEMENTBUFFEROBJECT_H
