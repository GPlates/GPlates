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
 
#ifndef GPLATES_OPENGL_GLVERTEXARRAY_H
#define GPLATES_OPENGL_GLVERTEXARRAY_H

#include <memory> // For std::auto_ptr
#include <vector>
#include <boost/enable_shared_from_this.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <opengl/OpenGL.h>

#include "GLCompiledDrawState.h"
#include "GLVertex.h"
#include "GLVertexBuffer.h"
#include "GLVertexElementBuffer.h"


namespace GPlatesOpenGL
{
	class GLRenderer;

	/**
	 * An abstraction based on OpenGL vertex array objects (GL_ARB_vertex_array_object extension)
	 * that behaves like vertex array objects even if the extension is not supported.
	 */
	class GLVertexArray :
			public boost::enable_shared_from_this<GLVertexArray>
	{
	public:
		//
		// Note that the reason boost::shared_ptr is used instead of non_null_intrusive_ptr
		// is so these objects can be used with GPlatesUtils::ObjectCache.
		//

		//! A convenience typedef for a shared pointer to a non-const @a GLVertexArray.
		typedef boost::shared_ptr<GLVertexArray> shared_ptr_type;
		typedef boost::shared_ptr<const GLVertexArray> shared_ptr_to_const_type;

		//! A convenience typedef for a weak pointer to a @a GLVertexArray.
		typedef boost::weak_ptr<GLVertexArray> weak_ptr_type;
		typedef boost::weak_ptr<const GLVertexArray> weak_ptr_to_const_type;


		/**
		 * Creates a @a GLVertexArray object with no array data.
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
		std::auto_ptr<GLVertexArray>
		create_as_auto_ptr(
				GLRenderer &renderer);


		virtual
		~GLVertexArray()
		{  }


		/**
		 * Binds this vertex array so that all vertex data is sourced from the vertex buffers,
		 * specified in this interface, and vertex element data is sourced from the vertex element buffer.
		 */
		virtual
		void
		gl_bind(
				GLRenderer &renderer) const = 0;


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
				GLint indices_offset) const = 0;


		/**
		 * Clears this vertex array.
		 *
		 * This includes the following:
		 *  - detach vertex element buffer
		 *  - detach vertex buffers (attribute arrays)
		 *  - disable vertex attribute arrays (including all texture coord arrays on all texture units)
		 *
		 * This method is useful when reusing a vertex array and you don't know what attribute
		 * arrays were previously enabled on the vertex array for example.
		 *
		 * NOTE: The detachments/disables are not until the next call to @a gl_bind.
		 */
		virtual
		void
		clear(
				GLRenderer &renderer) = 0;


		/**
		 * Specify the source of vertex element (vertex indices) data.
		 *
		 * NOTE: A vertex element buffer needs to be set before any drawing can occur.
		 */
		virtual
		void
		set_vertex_element_buffer(
				GLRenderer &renderer,
				const GLVertexElementBuffer::shared_ptr_to_const_type &vertex_element_buffer) = 0;


		//
		// NOTE: An easier way to use the methods below is to call @a bind_vertex_buffer_to_vertex_array.
		//


		/**
		 * Enables the specified (@a array) vertex array (in the fixed-function pipeline).
		 *
		 * These are the arrays selected for use by @a gl_bind.
		 * By default they are all disabled.
		 *
		 * @a array should be one of GL_VERTEX_ARRAY, GL_COLOR_ARRAY, or GL_NORMAL_ARRAY.
		 *
		 * NOTE: not including GL_INDEX_ARRAY since using RGBA mode (and not colour index mode) and
		 * not including GL_EDGE_FLAG_ARRAY (should be moving more towards generic attributes anyway).
		 */
		virtual
		void
		set_enable_client_state(
				GLRenderer &renderer,
				GLenum array,
				bool enable) = 0;

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
				bool enable) = 0;


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
				GLint offset) = 0;


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
				GLint offset) = 0;


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
				GLint offset) = 0;


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
				GLint offset) = 0;


		///////////////////////////////////////////////////////////////
		//
		// The following methods deal with *generic* vertex attributes.
		//
		// NOTE: For these methods the 'GL_ARB_vertex_shader' extension must be supported.
		//
		// NOTE: On nVidia hardware the generic attribute indices map onto the built-in attributes
		// so care should be taken if you mix non-generic with generic. Ideally no mixing should be
		// done (ie, use *only* the built-in attributes for the fixed-function pipeline and use
		// either the built-in attributes *only* or the generic attributes *only* for vertex shaders.
		// This means just within a single vertex array - it's ok to have one vertex array use
		// non-generic attributes while another uses generic attributes.
		// See 'GLProgramObject::gl_bind_attrib_location' for more details about nVidia's mapping.
		//
		///////////////////////////////////////////////////////////////


		/**
		 * Enables the specified *generic* vertex attribute data at attribute index @a attribute_index.
		 *
		 * These are the *generic* attribute arrays selected for use by @a gl_bind.
		 * They can be used in addition to the non-generic arrays (or instead of).
		 * These *generic* attributes can only be accessed by shader programs (see @a GLProgramObject).
		 * The non-generic arrays can be accessed by both the fixed-function pipeline and shader programs.
		 * Although starting with OpenGL 3 the non-generic arrays are deprecated/removed from the core OpenGL profile.
		 * But graphics vendors support compatibility profiles so using them in a pre version 3 way is still ok.
		 *
		 * By default all arrays are disabled.
		 *
		 * Note that, as dictated by OpenGL, @a attribute_index must be in the half-closed range
		 * [0, GL_MAX_VERTEX_ATTRIBS_ARB).
		 * You can get GL_MAX_VERTEX_ATTRIBS_ARB from 'GLContext::get_parameters().shader.gl_max_vertex_attribs'.
		 *
		 * NOTE: The 'GL_ARB_vertex_shader' extension must be supported.
		 */
		virtual
		void
		set_enable_vertex_attrib_array(
				GLRenderer &renderer,
				GLuint attribute_index,
				bool enable) = 0;


		/**
		 * Specify the source of *generic* vertex attribute data at attribute index @a attribute_index.
		 *
		 * The attribute index @a attribute_index should match up with the attribute index bound
		 * to the shader program object - see 'GLProgramObject::gl_bind_attrib_location()'.
		 * You can either do this explicitly or implement a specialisation of the template function
		 * 'bind_vertex_buffer_to_vertex_array()' for your particular vertex structure(s) - see 'GLVertex.h'.
		 *
		 * These are the *generic* attribute arrays selected for use by @a gl_bind.
		 * They can be used in addition to the non-generic arrays (or instead of).
		 * These *generic* attributes can only be accessed by shader programs (see @a GLProgramObject).
		 * The non-generic arrays can be accessed by both the fixed-function pipeline and shader programs.
		 * Although starting with OpenGL 3 the non-generic arrays are deprecated/removed from the core OpenGL profile.
		 * But graphics vendors support compatibility profiles so using them in a pre version 3 way is still ok.
		 *
		 * Note that, as dictated by OpenGL, @a attribute_index must be in the half-closed range
		 * [0, GL_MAX_VERTEX_ATTRIBS_ARB).
		 * You can get GL_MAX_VERTEX_ATTRIBS_ARB from 'GLContext::get_parameters().shader.gl_max_vertex_attribs'.
		 *
		 * NOTE: You still need to call the appropriate @a set_enable_vertex_attrib_array.
		 *
		 * @a offset is a byte offset into the vertex buffer at which data will be retrieved.
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
				GLint offset) = 0;

		/**
		 * Same as @a set_vertex_attrib_pointer except used to specify attributes mapping to *integer* shader variables.
		 *
		 * NOTE: The 'GL_ARB_vertex_shader' *and* 'GL_EXT_gpu_shader4' extensions must be supported.
		 */
		virtual
		void
		set_vertex_attrib_i_pointer(
				GLRenderer &renderer,
				const GLVertexBuffer::shared_ptr_to_const_type &vertex_buffer,
				GLuint attribute_index,
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint offset) = 0;

		/**
		 * Same as @a set_vertex_attrib_pointer except used to specify attributes mapping to *double* shader variables.
		 *
		 * NOTE: The 'GL_ARB_vertex_shader' *and* 'GL_ARB_vertex_attrib_64bit' extensions must be supported.
		 */
		virtual
		void
		set_vertex_attrib_l_pointer(
				GLRenderer &renderer,
				const GLVertexBuffer::shared_ptr_to_const_type &vertex_buffer,
				GLuint attribute_index,
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint offset) = 0;
	};


	/**
	 * Stores the specified vertices and indices in the specified vertex array.
	 *
	 * To do this it creates a vertex buffer using @a vertices and a vertex element buffer
	 * using @a vertex_elements and attaches the buffers to the vertex array.
	 *
	 * This is basically a convenience function for when you just want to create a drawable
	 * around some vertices (and vertex elements).
	 *
	 * NOTE: Since this creates new vertex and vertex element buffers it is not the best way
	 * to update a vertex array (this is only meant for once-only initialisation of static data).
	 */
	template <class VertexType, typename VertexElementType>
	void
	set_vertex_array_data(
			GLRenderer &renderer,
			GLVertexArray &vertex_array,
			const std::vector<VertexType> &vertices,
			const std::vector<VertexElementType> &vertex_elements);


	/**
	 * Compiles a draw state that, whenever applied to a renderer, will bind and draw the vertex array.
	 *
	 * The parameters are the same as for 'glDrawRangeElements()'.
	 *
	 * NOTE: The caller is responsible for setting the vertices/indices of @a vertex_array.
	 * See @a set_vertex_array_data for this.
	 *
	 * NOTE: The returned compiled draw state does not need @a vertex_array to exist
	 * when it is applied to a renderer (the internal buffers, or buffer objects, are kept alive
	 * by the compiled draw state).
	 */
	GLCompiledDrawState::non_null_ptr_type
	compile_vertex_array_draw_state(
			GLRenderer &renderer,
			GLVertexArray &vertex_array,
			GLenum mode,
			GLuint start,
			GLuint end,
			GLsizei count,
			GLenum type,
			GLint indices_offset = 0);


	/**
	 * Combines the @a set_vertex_array_data and @a compile_vertex_array_draw_state functions to
	 * set vertices/indices in the specified vertex array and return a compile draw state that
	 * will draw those specified vertices/indices.
	 */
	template <class VertexType, typename VertexElementType>
	GLCompiledDrawState::non_null_ptr_type
	compile_vertex_array_draw_state(
			GLRenderer &renderer,
			GLVertexArray &vertex_array,
			const std::vector<VertexType> &vertices,
			const std::vector<VertexElementType> &vertex_elements,
			GLenum mode);


	////////////////////
	// Implementation //
	////////////////////


	template <class VertexType, typename VertexElementType>
	void
	set_vertex_array_data(
			GLRenderer &renderer,
			GLVertexArray &vertex_array,
			const std::vector<VertexType> &vertices,
			const std::vector<VertexElementType> &vertex_elements)
	{
		// Set up the vertex element buffer.
		GLBuffer::shared_ptr_type vertex_element_buffer_data = GLBuffer::create(renderer);
		vertex_element_buffer_data->gl_buffer_data(
				renderer,
				GLBuffer::TARGET_ELEMENT_ARRAY_BUFFER,
				vertex_elements,
				GLBuffer::USAGE_STATIC_DRAW);
		// Attach vertex element buffer to the vertex array.
		vertex_array.set_vertex_element_buffer(
				renderer,
				GLVertexElementBuffer::create(renderer, vertex_element_buffer_data));

		// Set up the vertex buffer.
		GLBuffer::shared_ptr_type vertex_buffer_data = GLBuffer::create(renderer);
		vertex_buffer_data->gl_buffer_data(
				renderer,
				GLBuffer::TARGET_ARRAY_BUFFER,
				vertices,
				GLBuffer::USAGE_STATIC_DRAW);
		// Attach vertex buffer to the vertex array.
		bind_vertex_buffer_to_vertex_array<VertexType>(
				renderer,
				vertex_array,
				GLVertexBuffer::create(renderer, vertex_buffer_data));
	}


	template <class VertexType, typename VertexElementType>
	GLCompiledDrawState::non_null_ptr_type
	compile_vertex_array_draw_state(
			GLRenderer &renderer,
			GLVertexArray &vertex_array,
			const std::vector<VertexType> &vertices,
			const std::vector<VertexElementType> &vertex_elements,
			GLenum mode)
	{
		set_vertex_array_data(renderer, vertex_array, vertices, vertex_elements);

		return compile_vertex_array_draw_state(
				renderer,
				vertex_array,
				mode,
				0/*start*/,
				vertices.size() - 1/*end*/,
				vertex_elements.size()/*count*/,
				GLVertexElementTraits<VertexElementType>::type);
	}
}

#endif // GPLATES_OPENGL_GLVERTEXARRAY_H
