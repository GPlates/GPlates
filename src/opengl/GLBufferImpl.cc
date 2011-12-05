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

#include <cstring> // for memcpy

#include "GLBufferImpl.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


void
GPlatesOpenGL::GLBufferImpl::gl_buffer_data(
		GLRenderer &renderer,
		target_type target,
		unsigned int size,
		const void *data,
		usage_type usage)
{
	// Allocate a new array.
	d_size = size;
	d_data.reset(new GLubyte[d_size]);

	// If 'data' is NULL then leave the array uninitialised.
	// This mirrors the behaviour of the OpenGL function 'glBufferData'.
	if (data)
	{
		// Copy the new data into our array.
		std::memcpy(d_data.get(), data, size);
	}

	// Notify clients that a buffer allocation has occurred.
	allocated_buffer();
}


void
GPlatesOpenGL::GLBufferImpl::gl_buffer_sub_data(
		GLRenderer &renderer,
		target_type target,
		unsigned int offset,
		unsigned int size,
		const void* data)
{
	// Range must fit within existing buffer.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			offset + size <= d_size,
			GPLATES_ASSERTION_SOURCE);

	// Replace the sub-range.
	std::memcpy(d_data.get() + offset, data, size);
}


void
GPlatesOpenGL::GLBufferImpl::gl_get_buffer_sub_data(
		GLRenderer &renderer,
		target_type target,
		unsigned int offset,
		unsigned int size,
		void* data) const
{
	// Range must fit within existing buffer.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			offset + size <= d_size,
			GPLATES_ASSERTION_SOURCE);

	// Copy the sub-range.
	std::memcpy(data, d_data.get() + offset, size);
}
