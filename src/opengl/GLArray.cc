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

#include <cstring> // for memcpy
/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>

#include "GLArray.h"


GPlatesOpenGL::GLArray::non_null_ptr_type
GPlatesOpenGL::GLArray::create(
		ArrayType array_type,
		UsageType usage_type,
		const boost::optional<GLVertexBufferResourceManager::shared_ptr_type> &vertex_buffer_manager)
{
	if (vertex_buffer_manager && are_vertex_buffer_objects_supported())
	{
		return non_null_ptr_type(
				new GLVertexBufferObject(
						vertex_buffer_manager.get(), array_type, usage_type));
	}

	return non_null_ptr_type(new GLSystemMemoryArray());
}


GPlatesOpenGL::GLSystemMemoryArray::GLSystemMemoryArray() :
	d_num_bytes(0)
{
}


GPlatesOpenGL::GLSystemMemoryArray::GLSystemMemoryArray(
		const void *array_data,
		unsigned int num_bytes) :
	d_array_storage(new GLubyte[num_bytes]),
	d_num_bytes(num_bytes)
{
	// Copy the array data into our array.
	std::memcpy(d_array_storage.get(), array_data, num_bytes);
}


void
GPlatesOpenGL::GLSystemMemoryArray::set_buffer_data(
		const void *array_data,
		unsigned int num_bytes)
{
	// Resize the array if necessary.
	if (d_num_bytes != num_bytes)
	{
		d_num_bytes = num_bytes;
		d_array_storage.reset(new GLubyte[d_num_bytes]);
	}

	// Copy the new data into our array.
	std::memcpy(d_array_storage.get(), array_data, num_bytes);
}


GPlatesOpenGL::GLVertexBufferObject::GLVertexBufferObject(
		const GLVertexBufferResourceManager::shared_ptr_type &vertex_buffer_manager,
		ArrayType array_type,
		UsageType usage_type) :
	d_vertex_buffer_resource(GLVertexBufferResource::create(vertex_buffer_manager))
{
	set_target(array_type);
	set_usage(usage_type);
}


GPlatesOpenGL::GLVertexBufferObject::GLVertexBufferObject(
		const GLVertexBufferResourceManager::shared_ptr_type &vertex_buffer_manager,
		ArrayType array_type,
		UsageType usage_type,
		const void *array_data,
		unsigned int num_bytes) :
	d_vertex_buffer_resource(GLVertexBufferResource::create(vertex_buffer_manager))
{
	set_target(array_type);
	set_usage(usage_type);

	set_buffer_data(array_data, num_bytes);
}


void
GPlatesOpenGL::GLVertexBufferObject::set_target(
		ArrayType array_type)
{
	switch (array_type)
	{
	case ARRAY_TYPE_VERTEX_ELEMENTS:
		d_target = GL_ELEMENT_ARRAY_BUFFER_ARB;
		break;

	case ARRAY_TYPE_VERTICES:
	default:
		d_target = GL_ARRAY_BUFFER_ARB;
		break;
	}
}


void
GPlatesOpenGL::GLVertexBufferObject::set_usage(
		UsageType usage_type)
{
	switch (usage_type)
	{
	case USAGE_DYNAMIC:
		d_usage = GL_DYNAMIC_DRAW_ARB;
		break;

	case USAGE_STREAM:
		d_usage = GL_STREAM_DRAW_ARB;
		break;

	case USAGE_STATIC:
	default:
		d_usage = GL_STATIC_DRAW_ARB;
		break;
	}
}


const GLubyte *
GPlatesOpenGL::GLVertexBufferObject::bind() const
{
	// Bind the vertex buffer object.
	glBindBufferARB(
			d_target,
			d_vertex_buffer_resource->get_resource());

	// Vertex buffer objects deal with offsets rather than pointers.
	return reinterpret_cast<const GLubyte *>(NULL);
}


void
GPlatesOpenGL::GLVertexBufferObject::unbind() const
{
	// Unbind the vertex buffer object - goes back to normal OpenGL vertex array mode.
	glBindBufferARB(d_target, 0);
}


void
GPlatesOpenGL::GLVertexBufferObject::set_buffer_data(
		const void *array_data,
		unsigned int num_bytes)
{
	// Get the OpenGL vertex buffer object id.
	const GLint vertex_buffer_object = d_vertex_buffer_resource->get_resource();

	// Bind the vertex buffer object and upload the vertex data to it.
	glBindBufferARB(d_target, vertex_buffer_object);
	glBufferDataARB(d_target, num_bytes, array_data, d_usage);

	// Switch back to regular vertex arrays in case the next array does not use vertex buffer objects.
	glBindBufferARB(d_target, 0);
}
