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

#include <cstddef> // For std::size_t
#include <vector>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <opengl/OpenGL.h>

#include "GLArray.h"
#include "GLVertexBufferResource.h"


namespace GPlatesOpenGL
{
	/**
	 * Traits class to find the size of a vertex element from its type.
	 */
	template <typename VertexElementType>
	struct VertexElementTraits; // Unspecialised class intentionally not defined.

	template <>
	struct VertexElementTraits<GLubyte>
	{
		static const GLenum type;
	};

	template <>
	struct VertexElementTraits<GLushort>
	{
		static const GLenum type;
	};

	template <>
	struct VertexElementTraits<GLuint>
	{
		static const GLenum type;
	};


	/**
	 * An array containing vertex indices into an OpenGL vertex array.
	 */
	class GLVertexElementArray :
			public GPlatesUtils::ReferenceCount<GLVertexElementArray>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLVertexElementArray.
		typedef boost::shared_ptr<GLVertexElementArray> shared_ptr_type;
		typedef boost::shared_ptr<const GLVertexElementArray> shared_ptr_to_const_type;

		//! A convenience typedef for a weak pointer to a @a GLVertexElementArray.
		typedef boost::weak_ptr<GLVertexElementArray> weak_ptr_type;
		typedef boost::weak_ptr<const GLVertexElementArray> weak_ptr_to_const_type;


		/**
		 * Creates a @a GLVertexElementArray object with no index data.
		 *
		 * You'll need to call @a set_array_data.
		 */
		static
		shared_ptr_type
		create(
				GLArray::UsageType usage = GLArray::USAGE_STATIC,
				const boost::optional<GLVertexBufferResourceManager::shared_ptr_type> &
						vertex_buffer_manager = boost::none)
		{
			return shared_ptr_type(
					new GLVertexElementArray(
							GLArray::create(
									GLArray::ARRAY_TYPE_VERTEX_ELEMENTS, usage, vertex_buffer_manager)));
		}


		/**
		 * Creates a @a GLVertexElementArray object.
		 *
		 * The vertex element array data is copied into an internal array.
		 *
		 * @a VertexElementType should be one of GLubyte, GLushort, or GLuint.
		 * It is the type (and therefore size) of each vertex index stored in the array.
		 *
		 * If @a vertex_buffer_manager is specified *and* vertex buffer objects are supported
		 * then an OpenGL vertex buffer object is used internally to store the vertex indices.
		 */
		template <typename VertexElementType>
		static
		shared_ptr_type
		create(
				const VertexElementType *elements,
				const unsigned int num_elements,
				GLArray::UsageType usage = GLArray::USAGE_STATIC,
				const boost::optional<GLVertexBufferResourceManager::shared_ptr_type> &
						vertex_buffer_manager = boost::none)
		{
			return shared_ptr_type(
					new GLVertexElementArray(
							GLArray::create(
									elements, num_elements,
									GLArray::ARRAY_TYPE_VERTEX_ELEMENTS, usage, vertex_buffer_manager),
							VertexElementTraits<VertexElementType>::type));
		}


		/**
		 * Creates a @a GLVertexElementArray object.
		 *
		 * The vertex element array data is copied into an internal array.
		 *
		 * @a VertexElementType should be one of GLubyte, GLushort, or GLuint.
		 * It is the type (and therefore size) of each vertex index stored in the array.
		 *
		 * If @a vertex_buffer_manager is specified *and* vertex buffer objects are supported
		 * then an OpenGL vertex buffer object is used internally to store the vertex indices.
		 */
		template <typename VertexElementType>
		static
		shared_ptr_type
		create(
				const std::vector<VertexElementType> &elements,
				GLArray::UsageType usage = GLArray::USAGE_STATIC,
				const boost::optional<GLVertexBufferResourceManager::shared_ptr_type> &
						vertex_buffer_manager = boost::none)
		{
			return create(&elements[0], elements.size(), usage, vertex_buffer_manager);
		}


		/**
		 * Creates a @a GLVertexElementArray object that uses the same array data as
		 * another @a GLVertexElementArray.
		 *
		 * This is useful when you want to use the same set of indices but over several
		 * different index ranges (because drawing different primitives in a single vertex array).
		 *
		 * However note that if you change the array data it will affect all @a GLVertexElementArray
		 * instances sharing it.
		 * NOTE: If you later call @a set_array_data, be careful not to change the VertexElementType
		 * because it could mess up how other @a GLVertexElementArray instances (that share it) will see it.
		 */
		static
		shared_ptr_type
		create(
				const GLArray::non_null_ptr_type &array_data,
				GLenum type)
		{
			return shared_ptr_type(new GLVertexElementArray(array_data, type));
		}


		/**
		 * Returns the array data referenced by us.
		 */
		GLArray::non_null_ptr_type
		get_array_data()
		{
			return d_array_data;
		}


		/**
		 * Specifies the array data to be used for this @a GLVertexElementArray.
		 *
		 * The vertex element array data is copied into an internal array.
		 */
		template <typename VertexElementType>
		void
		set_array_data(
				const VertexElementType *elements,
				const unsigned int num_elements)
		{
			d_array_data->set_array_data(elements, num_elements);

			d_type = VertexElementTraits<VertexElementType>::type;
		}


		/**
		 * Specifies the array data to be used for this @a GLVertexElementArray.
		 *
		 * The vertex element array data is copied into an internal array.
		 */
		template <typename VertexElementType>
		void
		set_array_data(
				const std::vector<VertexElementType> &elements)
		{
			set_array_data(&elements[0], elements.size());
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
		GLArray::non_null_ptr_type d_array_data;

		/**
		 * The type (and hence size) of a vertex index (if any data has been set).
		 */
		boost::optional<GLenum> d_type;

		boost::optional<DrawElements> d_draw_elements;
		boost::optional<DrawRangeElementsEXT> d_draw_range_elements;


		//! Constructor.
		explicit
		GLVertexElementArray(
				const GLArray::non_null_ptr_type &vertex_element_array,
				const boost::optional<GLenum> &type = boost::none) :
			d_array_data(vertex_element_array),
			d_type(type)
		{  }
	};
}

#endif // GPLATES_OPENGL_GLVERTEXELEMENTARRAY_H
