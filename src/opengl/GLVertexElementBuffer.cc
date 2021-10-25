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

#include "GLVertexElementBuffer.h"

#include "GLVertexElementBufferImpl.h"
#include "GLVertexElementBufferObject.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


const GLenum GPlatesOpenGL::GLVertexElementTraits<GLubyte>::type = GL_UNSIGNED_BYTE;
const GLenum GPlatesOpenGL::GLVertexElementTraits<GLushort>::type = GL_UNSIGNED_SHORT;
const GLenum GPlatesOpenGL::GLVertexElementTraits<GLuint>::type = GL_UNSIGNED_INT;


std::unique_ptr<GPlatesOpenGL::GLVertexElementBuffer>
GPlatesOpenGL::GLVertexElementBuffer::create_as_unique_ptr(
		GLRenderer &renderer,
		const GLBuffer::shared_ptr_type &buffer)
{
	const GLBufferObject::shared_ptr_type buffer_object = boost::dynamic_pointer_cast<GLBufferObject>(buffer);
	if (buffer_object)
	{
		return std::unique_ptr<GPlatesOpenGL::GLVertexElementBuffer>(
				GLVertexElementBufferObject::create_as_unique_ptr(renderer, buffer_object));
	}

	// If it's not a buffer object then it can only be a buffer impl (only two types are possible).
	const GLBufferImpl::shared_ptr_type buffer_impl = boost::dynamic_pointer_cast<GLBufferImpl>(buffer);
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			buffer_impl,
			GPLATES_ASSERTION_SOURCE);

	return std::unique_ptr<GPlatesOpenGL::GLVertexElementBuffer>(
			GLVertexElementBufferImpl::create_as_unique_ptr(renderer, buffer_impl));
}
