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

/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>
#include <opengl/OpenGL.h>
#include <QDebug>

#include "GLBufferObject.h"

#include "GLContext.h"
#include "GLRenderer.h"

#include "global/AssertionFailureException.h"
#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


// We use macros in <GL/glew.h> that contain old-style casts.
DISABLE_GCC_WARNING("-Wold-style-cast")


GPlatesOpenGL::GLBufferObject::resource_handle_type
GPlatesOpenGL::GLBufferObject::Allocator::allocate()
{
	// We should only get here if the vertex buffer object extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_ARB_vertex_buffer_object),
			GPLATES_ASSERTION_SOURCE);

	resource_handle_type buffer_object;
	glGenBuffersARB(1, &buffer_object);
	return buffer_object;
}


void
GPlatesOpenGL::GLBufferObject::Allocator::deallocate(
		resource_handle_type buffer_object)
{
	// We should only get here if the vertex buffer object extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_ARB_vertex_buffer_object),
			GPLATES_ASSERTION_SOURCE);

	glDeleteBuffersARB(1, &buffer_object);
}


GPlatesOpenGL::GLBufferObject::GLBufferObject(
		GLRenderer &renderer) :
	d_resource(
			resource_type::create(
					renderer.get_context().get_shared_state()->get_buffer_object_resource_manager())),
	d_size(0)
{
	// We should only get here if the vertex buffer object extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_ARB_vertex_buffer_object),
			GPLATES_ASSERTION_SOURCE);
}


void
GPlatesOpenGL::GLBufferObject::gl_buffer_data(
		GLRenderer &renderer,
		target_type target,
		unsigned int size,
		const void* data,
		usage_type usage)
{
	// Revert our binding on return so we don't affect changes made by clients.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Bind this buffer object.
	// Make sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	renderer.gl_bind_buffer_object_and_apply(
			boost::dynamic_pointer_cast<const GLBufferObject>(shared_from_this()),
			target);

	glBufferDataARB(target, size, data, usage);

	// Record the new buffer size.
	d_size = size;

	// Notify clients that a buffer allocation has occurred.
	allocated_buffer();
}


void
GPlatesOpenGL::GLBufferObject::gl_buffer_sub_data(
		GLRenderer &renderer,
		target_type target,
		unsigned int offset,
		unsigned int size,
		const void* data)
{
	// Revert our binding on return so we don't affect changes made by clients.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Bind this buffer object.
	// Make sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	renderer.gl_bind_buffer_object_and_apply(
			boost::dynamic_pointer_cast<const GLBufferObject>(shared_from_this()),
			target);

	glBufferSubDataARB(target, offset, size, data);
}


void
GPlatesOpenGL::GLBufferObject::gl_get_buffer_sub_data(
		GLRenderer &renderer,
		target_type target,
		unsigned int offset,
		unsigned int size,
		void* data) const
{
	// Revert our binding on return so we don't affect changes made by clients.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Bind this buffer object.
	// Make sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	renderer.gl_bind_buffer_object_and_apply(
			boost::dynamic_pointer_cast<const GLBufferObject>(shared_from_this()),
			target);

	glGetBufferSubDataARB(target, offset, size, data);
}


GLvoid *
GPlatesOpenGL::GLBufferObject::gl_map_buffer(
		GLRenderer &renderer,
		target_type target,
		access_type access)
{
	// Revert our binding on return so we don't affect changes made by clients.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Bind this buffer object.
	// Make sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	renderer.gl_bind_buffer_object_and_apply(
			boost::dynamic_pointer_cast<const GLBufferObject>(shared_from_this()),
			target);

	return glMapBufferARB(target, access);
}


bool
GPlatesOpenGL::GLBufferObject::is_map_buffer_range_supported() const
{
	return GPLATES_OPENGL_BOOL(GLEW_ARB_map_buffer_range);
}


void *
GPlatesOpenGL::GLBufferObject::gl_map_buffer_range(
		GLRenderer &renderer,
		target_type target,
		unsigned int offset,
		unsigned int length,
		range_access_type access)
{
	// We should only get here if the map buffer range extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_ARB_map_buffer_range),
			GPLATES_ASSERTION_SOURCE);

	// Revert our binding on return so we don't affect changes made by clients.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Bind this buffer object.
	// Make sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	renderer.gl_bind_buffer_object_and_apply(
			boost::dynamic_pointer_cast<const GLBufferObject>(shared_from_this()),
			target);

	return glMapBufferRange(target, offset, length, access);
}


void
GPlatesOpenGL::GLBufferObject::gl_flush_mapped_buffer_range(
		GLRenderer &renderer,
		target_type target,
		unsigned int offset,
		unsigned int length)
{
	// We should only get here if the map buffer range extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_ARB_map_buffer_range),
			GPLATES_ASSERTION_SOURCE);

	// Revert our binding on return so we don't affect changes made by clients.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Bind this buffer object.
	// Make sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	renderer.gl_bind_buffer_object_and_apply(
			boost::dynamic_pointer_cast<const GLBufferObject>(shared_from_this()),
			target);

	glFlushMappedBufferRange(target, offset, length);
}


void
GPlatesOpenGL::GLBufferObject::gl_unmap_buffer(
		GLRenderer &renderer,
		target_type target)
{
	// Revert our binding on return so we don't affect changes made by clients.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Bind this buffer object.
	// Make sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	renderer.gl_bind_buffer_object_and_apply(
			boost::dynamic_pointer_cast<const GLBufferObject>(shared_from_this()),
			target);

	glUnmapBufferARB(target);
}
