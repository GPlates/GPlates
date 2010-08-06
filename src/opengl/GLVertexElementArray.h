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
 
#ifndef GPLATES_OPENGL_GLVERTEXELEMENTARRAY_H
#define GPLATES_OPENGL_GLVERTEXELEMENTARRAY_H

#include <vector>
#include <boost/optional.hpp>
#include <boost/shared_array.hpp>
#include <opengl/OpenGL.h>

#include "GLArray.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	/**
	 * An array containing vertex indices into an OpenGL vertex array.
	 * 
	 */
	class GLVertexElementArray :
			public GPlatesUtils::ReferenceCount<GLVertexElementArray>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLVertexElementArray.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLVertexElementArray> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLVertexElementArray.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLVertexElementArray> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLVertexElementArray object with no index data.
		 *
		 * You'll need to call @a set_array_data.
		 */
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new GLVertexElementArray());
		}


		/**
		 * Creates a @a GLVertexElementArray object.
		 *
		 * The vertex element array data is copied into an internal array.
		 *
		 * @a VertexElementType should be one of GLubyte, GLushort, or GLuint.
		 * It is the type (and therefore size) of each vertex index stored in the array.
		 * It must match the type specified in @a gl_draw_elements or @a gl_draw_range_elements_EXT.
		 */
		template <typename VertexElementType>
		static
		non_null_ptr_type
		create(
				const std::vector<VertexElementType> &vertex_element_array)
		{
			return non_null_ptr_type(new GLVertexElementArray(GLArray(vertex_element_array)));
		}


		/**
		 * Creates a @a GLVertexElementArray object.
		 *
		 * The vertex element array data is copied into an internal array.
		 *
		 * @a VertexElementType should be one of GLubyte, GLushort, or GLuint.
		 * It is the type (and therefore size) of each vertex index stored in the array.
		 * It must match the type specified in @a gl_draw_elements or @a gl_draw_range_elements_EXT.
		 */
		template <typename VertexElementForwardIter>
		static
		non_null_ptr_type
		create(
				VertexElementForwardIter begin,
				VertexElementForwardIter end)
		{
			return non_null_ptr_type(new GLVertexElementArray(GLArray(begin, end)));
		}


		/**
		 * Creates a @a GLVertexElementArray object.
		 * 
		 * Ownership of the vertex element array data is shared internally.
		 *
		 * @a VertexElementType should be one of GLubyte, GLushort, or GLuint.
		 * It is the type (and therefore size) of each vertex index stored in the array.
		 * It must match the type specified in @a gl_draw_elements or @a gl_draw_range_elements_EXT.
		 *
		 * When @a draw is called it will not do anything unless you call
		 * @a gl_draw_elements or @a gl_draw_range_elements_EXT to setup the draw parameters.
		 */
		template <typename VertexElementType>
		static
		non_null_ptr_type
		create(
				const boost::shared_array<VertexElementType> &vertex_element_array)
		{
			return non_null_ptr_type(new GLVertexElementArray(GLArray(vertex_element_array)));
		}


		/**
		 * Specifies the array data to be used for this @a GLVertexElementArray.
		 *
		 * The vertex element array data is copied into an internal array.
		 *
		 * This method can be used to set the array data if the @a create overload
		 * with no data was used to create 'this' object, or this method can be used
		 * to change the array data.
		 *
		 * NOTE: If 'VertexElementType' is a different type to that passed to @a create
		 * then you may need to call @a gl_draw_elements or @a glDrawRangeElementsExt again.
		 */
		template <typename VertexElementType>
		void
		set_array_data(
				const std::vector<VertexElementType> &array_data)
		{
			d_array_data = GLArray(array_data);
		}


		/**
		 * Specifies the array data to be used for this @a GLVertexElementArray.
		 *
		 * The vertex element array data is copied into an internal array.
		 *
		 * @a VertexElementType should be one of GLubyte, GLushort, or GLuint.
		 * It is the type (and therefore size) of each vertex index stored in the array.
		 * It must match the type specified in @a gl_draw_elements or @a gl_draw_range_elements_EXT.
		 *
		 * This method can be used to set the array data if the @a create overload
		 * with no data was used to create 'this' object, or this method can be used
		 * to change the array data.
		 *
		 * NOTE: If 'VertexElementType' is a different type to that passed to @a create
		 * then you may need to call @a gl_draw_elements or @a glDrawRangeElementsExt again.
		 */
		template <typename VertexElementForwardIter>
		void
		set_array_data(
				VertexElementForwardIter begin,
				VertexElementForwardIter end)
		{
			d_array_data = GLArray(begin, end);
		}


		/**
		 * Specifies the array data to be used for this @a GLVertexElementArray.
		 * 
		 * Ownership of the vertex element array data is shared internally.
		 *
		 * @a VertexElementType should be one of GLubyte, GLushort, or GLuint.
		 * It is the type (and therefore size) of each vertex index stored in the array.
		 * It must match the type specified in @a gl_draw_elements or @a gl_draw_range_elements_EXT.
		 *
		 * This method can be used to set the array data if the @a create overload
		 * with no data was used to create 'this' object, or this method can be used
		 * to change the array data.
		 *
		 * NOTE: If 'VertexElementType' is a different type to that passed to @a create
		 * then you may need to call @a gl_draw_elements or @a glDrawRangeElementsExt again.
		 */
		template <typename VertexElementType>
		void
		set_array_data(
				const boost::shared_array<VertexElementType> &array_data)
		{
			d_array_data = GLArray(array_data);
		}


		/**
		 * Stores parameters for the call to OpenGL glDrawElements inside @a draw.
		 *
		 * @a indices_offset is a byte offset from the start of the indices array
		 * (passed into @a create) from which to start retrieving indices.
		 */
		void
		gl_draw_elements(
				GLenum mode,
				GLsizei count,
				GLenum type,
				GLint indices_offset);

		/**
		 * Stores parameters for the call to OpenGL glDrawRangeElementsEXT inside @a draw.
		 *
		 * If the GL_EXT_draw_range_elements OpenGL extension is not available
		 * then @a start and @a end are ignored and this call effectively becomes
		 * a @a gl_draw_elements call.
		 *
		 * @a indices_offset is a byte offset from the start of the indices array
		 * (passed into @a create) from which to start retrieving indices.
		 *
		 * This function can be more efficient for OpenGL than @a gl_draw_elements since
		 * you are guaranteeing that the range of indices is bounded by [start, end].
		 */
		void
		gl_draw_range_elements_EXT(
				GLenum mode,
				GLuint start,
				GLuint end,
				GLsizei count,
				GLenum type,
				GLint indices_offset);


		/**
		 * Does the actual drawing to OpenGL.
		 *
		 * NOTE: The vertices are dereferenced from the currently bound @a GLVertexArray.
		 */
		void
		draw() const;

	private:
		struct DrawElements
		{
			GLenum mode;
			GLsizei count;
			GLenum type;
			GLint indices_offset;
		};
		struct DrawRangeElementsEXT
		{
			GLuint start;
			GLuint end;
		};


		/**
		 * The opaque vertex index data.
		 */
		boost::optional<GLArray> d_array_data;

		boost::optional<DrawElements> d_draw_elements;
		boost::optional<DrawRangeElementsEXT> d_draw_range_elements;


		//! Default constructor.
		GLVertexElementArray()
		{  }
			
		//! Constructor.
		explicit
		GLVertexElementArray(
				const GLArray &vertex_element_array) :
			d_array_data(vertex_element_array)
		{  }
	};
}

#endif // GPLATES_OPENGL_GLVERTEXELEMENTARRAY_H
