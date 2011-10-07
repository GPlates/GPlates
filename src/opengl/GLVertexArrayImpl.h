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

#ifndef GPLATES_OPENGL_GLVERTEXARRAYIMPL_H
#define GPLATES_OPENGL_GLVERTEXARRAYIMPL_H

#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <opengl/OpenGL.h>

#include "GLVertexArray.h"


namespace GPlatesOpenGL
{
	class GLRenderer;

	/**
	 * An implementation of the OpenGL vertex array objects (GL_ARB_vertex_array_object extension)
	 * to simulate equivalent behaviour when the extension is not supported.
	 */
	class GLVertexArrayImpl :
			public GLVertexArray
	{
	public:
		//! A convenience typedef for a shared pointer to a @a GLVertexArrayImpl.
		typedef boost::shared_ptr<GLVertexArrayImpl> shared_ptr_type;
		typedef boost::shared_ptr<const GLVertexArrayImpl> shared_ptr_to_const_type;


		/**
		 * Creates a @a GLVertexArrayImpl object with no array data.
		 */
		static
		shared_ptr_type
		create(
				GLRenderer &renderer)
		{
			return shared_ptr_type(create_as_auto_ptr(renderer).release());
		}

		/**
		 * Same as @a create but returns a std::auto_ptr - to guarantee only one owner.
		 */
		static
		std::auto_ptr<GLVertexArrayImpl>
		create_as_auto_ptr(
				GLRenderer &renderer)
		{
			return std::auto_ptr<GLVertexArrayImpl>(new GLVertexArrayImpl(renderer));
		}


		/**
		 * Binds this vertex array so that all vertex data is sourced from the vertex buffers,
		 * specified in this interface, and vertex element data is sourced from the vertex element buffer.
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
		 * @a indices_offset is a byte offset from the start of vertex element buffer set with
		 * @a set_vertex_element_buffer.
		 *
		 * This function can be more efficient for OpenGL than @a gl_draw_elements since
		 * you are guaranteeing that the range of vertex indices is bounded by [start, end].
		 *
		 * @throws @a PreconditionViolationError if a vertex element buffer has not been set
		 * with @a set_vertex_element_buffer.
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
		 * Clears this vertex array.
		 */
		virtual
		void
		clear(
				GLRenderer &renderer);


		/**
		 * Specify the source of vertex element (vertex indices) data.
		 *
		 * NOTE: A vertex element buffer needs to be set before any drawing can occur.
		 */
		virtual
		void
		set_vertex_element_buffer(
				GLRenderer &renderer,
				const GLVertexElementBuffer::shared_ptr_to_const_type &vertex_element_buffer);


		/**
		 * Enables the specified (@a array) vertex array (in the fixed-function pipeline).
		 *
		 * These are the arrays selected for use by @a gl_bind.
		 * By default they are all disabled.
		 *
		 * @a array should be one of GL_VERTEX_ARRAY, GL_COLOR_ARRAY, or GL_NORMAL_ARRAY.
		 *
		 * NOTE: not including GL_INDEX_ARRAY since using RGBA mode (and not colour index mode)
		 * and not including GL_EDGE_FLAG_ARRAY for now.
		 */
		virtual
		void
		set_enable_client_state(
				GLRenderer &renderer,
				GLenum array,
				bool enable = true);

		/**
		 * Enables the vertex attribute array GL_TEXTURE_COORD_ARRAY (in the fixed-function pipeline)
		 * on the specified texture unit.
		 *
		 * By default the texture coordinate arrays for all texture units are disabled.
		 */
		virtual
		void
		set_enable_client_texture_state(
				GLRenderer &renderer,
				GLenum texture_unit,
				bool enable = true);


		/**
		 * Specify the source of vertex position data.
		 *
		 * NOTE: You still need to call the appropriate @a set_enable_client_state.
		 *
		 * @a offset is a byte offset into the vertex buffer at which data will be retrieved.
		 */
		virtual
		void
		set_vertex_pointer(
				GLRenderer &renderer,
				const GLVertexBuffer::shared_ptr_to_const_type &vertex_buffer,
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint offset);


		/**
		 * Specify the source of vertex colour data.
		 *
		 * NOTE: You still need to call the appropriate @a set_enable_client_state.
		 *
		 * @a offset is a byte offset into the vertex buffer at which data will be retrieved.
		 */
		virtual
		void
		set_color_pointer(
				GLRenderer &renderer,
				const GLVertexBuffer::shared_ptr_to_const_type &vertex_buffer,
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint offset);


		/**
		 * Specify the source of vertex normal data.
		 *
		 * NOTE: You still need to call the appropriate @a set_enable_client_state.
		 *
		 * @a offset is a byte offset into the vertex buffer at which data will be retrieved.
		 */
		virtual
		void
		set_normal_pointer(
				GLRenderer &renderer,
				const GLVertexBuffer::shared_ptr_to_const_type &vertex_buffer,
				GLenum type,
				GLsizei stride,
				GLint offset);


		/**
		 * Specify the source of vertex texture coordinate data.
		 *
		 * NOTE: You still need to call the appropriate @a set_enable_client_texture_state.
		 *
		 * @a offset is a byte offset into the vertex buffer at which data will be retrieved.
		 */
		virtual
		void
		set_tex_coord_pointer(
				GLRenderer &renderer,
				const GLVertexBuffer::shared_ptr_to_const_type &vertex_buffer,
				GLenum texture_unit,
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint offset);


		/**
		 * Enables the specified *generic* vertex attribute data at attribute index @a attribute_index.
		 *
		 * NOTE: The 'GL_ARB_vertex_shader' extension must be supported.
		 */
		virtual
		void
		set_enable_vertex_attrib_array(
				GLRenderer &renderer,
				GLuint attribute_index,
				bool enable);


		/**
		 * Specify the source of *generic* vertex attribute data at attribute index @a attribute_index.
		 *
		 * NOTE: The 'GL_ARB_vertex_shader' extension must be supported.
		 */
		virtual
		void
		set_vertex_attrib_pointer(
				GLRenderer &renderer,
				const GLVertexBuffer::shared_ptr_to_const_type &vertex_buffer,
				GLuint attribute_index,
				GLint size,
				GLenum type,
				GLboolean normalized,
				GLsizei stride,
				GLint offset);


		/**
		 * Returns the compiled bind state for this vertex array.
		 *
		 * NOTE: This is a lower-level function used to help implement the OpenGL framework.
		 */
		boost::shared_ptr<const GLState>
		get_compiled_bind_state() const
		{
			return d_compiled_bind_state->get_state();
		}

	private:
		/**
		 * The sole vertex element buffer containing vertex indices.
		 *
		 * This must be set before drawing otherwise it's an error.
		 */
		boost::optional<GLVertexElementBuffer::shared_ptr_to_const_type> d_vertex_element_buffer;

		/**
		 * Maintains all the binding/enabling state for this vertex array.
		 */
		GLCompiledDrawState::non_null_ptr_type d_compiled_bind_state;


		//! Constructor.
		explicit
		GLVertexArrayImpl(
				GLRenderer &renderer);
	};
}

#endif // GPLATES_OPENGL_GLVERTEXARRAYIMPL_H
