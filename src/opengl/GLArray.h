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
 
#ifndef GPLATES_OPENGL_GLARRAY_H
#define GPLATES_OPENGL_GLARRAY_H

#include <cstddef> // For std::size_t
#include <utility>
#include <vector>
#include <boost/optional.hpp>
#include <boost/scoped_array.hpp>
#include <opengl/OpenGL.h>

#include "GLVertexBufferResource.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	/**
	 * An interface class for arrays in OpenGL (such as vertex and index arrays).
	 */
	class GLArray :
			public GPlatesUtils::ReferenceCount<GLArray>
	{
	public:
		typedef GPlatesUtils::non_null_intrusive_ptr<GLArray> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLArray> non_null_ptr_to_const_type;

		//! The type of array.
		enum ArrayType
		{
			// Array is used to store the vertices themselves.
			ARRAY_TYPE_VERTICES,

			// Array is used to store the indices (into vertex array) used to build primitives.
			ARRAY_TYPE_VERTEX_ELEMENTS
		};

		//! The usage of the array.
		enum UsageType
		{
			// You will specify the data only once,
			// then use it many times without modifying it.
			USAGE_STATIC,

			// You will specify or modify the data repeatedly,
			// and use it repeatedly after each time you do this.
			USAGE_DYNAMIC,

			// You will modify the data once, then use it once,
			// and repeat this process many times.
			USAGE_STREAM
		};


		/**
		 * Creates an array of the appropriate internal array structure - either system memory
		 * or a vertex buffer object (OpenGL extension) - but stores no data in it.
		 *
		 * If @a vertex_buffer_manager is not none and the vertex buffer objects extension
		 * is supported then then internal buffer will be a vertex buffer object.
		 */
		static
		non_null_ptr_type
		create(
				ArrayType array_type,
				UsageType usage_type,
				const boost::optional<GLVertexBufferResourceManager::shared_ptr_type> &
						vertex_buffer_manager = boost::none);


		/**
		 * Wrap the specified elements in an internal array structure - either system memory
		 * or a vertex buffer object (OpenGL extension).
		 *
		 * If @a vertex_buffer_manager is not none and the vertex buffer objects extension
		 * is supported then then internal buffer will be a vertex buffer object.
		 *
		 * @a ElementType is the type (or structure) of the data passed in.
		 * For example it could be a vertex structure for a vertex array or
		 * a GL_UNSIGNED_SHORT for a vertex index array.
		 */
		template <class ElementType>
		static
		non_null_ptr_type
		create(
				const ElementType *elements,
				const unsigned int num_elements,
				ArrayType array_type,
				UsageType usage_type,
				const boost::optional<GLVertexBufferResourceManager::shared_ptr_type> &
						vertex_buffer_manager = boost::none);


		/**
		 * Wrap the specified elements in an internal array structure - either system memory
		 * or a vertex buffer object (OpenGL extension).
		 *
		 * If @a vertex_buffer_manager is not none and the vertex buffer objects extension
		 * is supported then then internal buffer will be a vertex buffer object.
		 *
		 * @a ElementType is the type (or structure) of the data passed in.
		 * For example it could be a vertex structure for a vertex array or
		 * a GL_UNSIGNED_SHORT for a vertex index array.
		 */
		template <class ElementType>
		static
		non_null_ptr_type
		create(
				const std::vector<ElementType> &elements,
				ArrayType array_type,
				UsageType usage_type,
				const boost::optional<GLVertexBufferResourceManager::shared_ptr_type> &
						vertex_buffer_manager = boost::none)
		{
			return create(&elements[0], elements.size(), array_type, usage_type, vertex_buffer_manager);
		}


		virtual
		~GLArray()
		{  }


		/**
		 * Specifies the array data to be used for this @a GLArray.
		 *
		 * The array data is copied into an internal array.
		 *
		 * This method can be used to set the array data if the @a create overload
		 * with no data was used to create 'this' object, or this method can be used
		 * to change the array data.
		 */
		template <class ElementType>
		void
		set_array_data(
				const ElementType *elements,
				const unsigned int num_elements)
		{
			set_buffer_data(elements, num_elements * sizeof(ElementType));
		}


		/**
		 * Specifies the array data to be used for this @a GLArray.
		 *
		 * The array data is copied into an internal array.
		 *
		 * This method can be used to set the array data if the @a create overload
		 * with no data was used to create 'this' object, or this method can be used
		 * to change the array data.
		 */
		template <class ElementType>
		void
		set_array_data(
				const std::vector<ElementType> &elements)
		{
			set_array_data(&elements[0], elements.size());
		}


		/**
		 * Binds the internal array (if applicable) and returns the opaque void pointer to
		 * the internal array data.
		 *
		 * Note that binding the internal array is currently only applicable to the
		 * vertex buffer objects OpenGL extension.
		 *
		 * Note that for vertex buffer objects (internal array) the returned pointer is NULL.
		 */
		virtual
		const GLubyte *
		bind() const = 0;


		/**
		 * Unbinds the internal array (if applicable).
		 *
		 * Note that unbinding the internal array is currently only applicable to the
		 * vertex buffer objects OpenGL extension.
		 */
		virtual
		void
		unbind() const = 0;

	private:
		/**
		 * Implementation method (derived class) that copies array to internal
		 * implementation-defined buffer.
		 */
		virtual
		void
		set_buffer_data(
				const void *array_data,
				unsigned int num_bytes) = 0;
	};


	//
	// Implementation
	//


	/**
	 * Stores the elements in system memory.
	 */
	class GLSystemMemoryArray :
			public GLArray
	{
	public:
		//! Constructor - stores no data.
		GLSystemMemoryArray();

		GLSystemMemoryArray(
				const void *array_data,
				unsigned int num_bytes);


		virtual
		const GLubyte *
		bind() const
		{
			// Nothing to do except return the pointer to our internal array.
			return d_array_storage.get();
		}


		virtual
		void
		unbind() const
		{
			// Nothing to do.
		}


		virtual
		void
		set_buffer_data(
				const void *array_data,
				unsigned int num_bytes);


	private:
		boost::scoped_array<GLubyte> d_array_storage;
		unsigned int d_num_bytes;
	};


	/**
	 * Stores the elements in a vertex buffer object (an OpenGL extension that
	 * can be used to store vertex data and vertex index data).
	 */
	class GLVertexBufferObject :
			public GLArray
	{
	public:
		//! Constructor - stores no data.
		GLVertexBufferObject(
				const GLVertexBufferResourceManager::shared_ptr_type &vertex_buffer_manager,
				ArrayType array_type,
				UsageType usage_type);

		GLVertexBufferObject(
				const GLVertexBufferResourceManager::shared_ptr_type &vertex_buffer_manager,
				ArrayType array_type,
				UsageType usage_type,
				const void *array_data,
				unsigned int num_bytes);


		virtual
		const GLubyte *
		bind() const;


		virtual
		void
		unbind() const;


		virtual
		void
		set_buffer_data(
				const void *array_data,
				unsigned int num_bytes);


	private:
		//! The array data is stored in an OpenGL vertex buffer object.
		GLVertexBufferResource::non_null_ptr_type d_vertex_buffer_resource;

		//! Whether the buffer is for vertices or vertex indices.
		GLenum d_target;

		//! How the buffer is going to be used.
		GLenum d_usage;


		void
		set_target(
				ArrayType array_type);

		void
		set_usage(
				UsageType usage_type);
	};


	template <class ElementType>
	GLArray::non_null_ptr_type
	GLArray::create(
			const ElementType *elements,
			const unsigned int num_elements,
			ArrayType array_type,
			UsageType usage_type,
			const boost::optional<GLVertexBufferResourceManager::shared_ptr_type> &vertex_buffer_manager)
	{
		if (vertex_buffer_manager && are_vertex_buffer_objects_supported())
		{
			return non_null_ptr_type(
					new GLVertexBufferObject(
							vertex_buffer_manager.get(),
							array_type,
							usage_type,
							elements,
							num_elements * sizeof(ElementType)));
		}

		return non_null_ptr_type(
				new GLSystemMemoryArray(
						elements,
						num_elements * sizeof(ElementType)));
	}
}

#endif // GPLATES_OPENGL_GLARRAY_H
