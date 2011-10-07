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
#include <boost/scoped_array.hpp>
#include <QDebug>

#include "GLShaderObject.h"

#include "GLContext.h"
#include "GLRenderer.h"
#include "OpenGLException.h"

#include "global/AssertionFailureException.h"
#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


// We use macros in <GL/glew.h> that contain old-style casts.
DISABLE_GCC_WARNING("-Wold-style-cast")


GPlatesOpenGL::GLShaderObject::resource_handle_type
GPlatesOpenGL::GLShaderObject::Allocator::allocate()
{
	// We should only get here if the shader objects extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_ARB_shader_objects),
			GPLATES_ASSERTION_SOURCE);

	const resource_handle_type shader_object = glCreateShaderObjectARB(d_shader_type);

	GPlatesGlobal::Assert<OpenGLException>(
			shader_object,
			GPLATES_ASSERTION_SOURCE,
			"Failed to create shader object.");

	return shader_object;
}


void
GPlatesOpenGL::GLShaderObject::Allocator::deallocate(
		resource_handle_type shader_object)
{
	// We should only get here if the shader objects extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_ARB_shader_objects),
			GPLATES_ASSERTION_SOURCE);

	glDeleteObjectARB(shader_object);
}


bool
GPlatesOpenGL::GLShaderObject::is_supported(
		GLRenderer &renderer,
		GLenum shader_type)
{
	if (!GLEW_ARB_shader_objects)
	{
		return false;
	}

	switch (shader_type)
	{
	case GL_VERTEX_SHADER_ARB:
		return GPLATES_OPENGL_BOOL(GLEW_ARB_vertex_shader);

	case GL_FRAGMENT_SHADER_ARB:
		return GPLATES_OPENGL_BOOL(GLEW_ARB_fragment_shader);

	case GL_GEOMETRY_SHADER_ARB:
		return GPLATES_OPENGL_BOOL(GLEW_ARB_geometry_shader4);

	default:
		// Unsupported capability.
		qWarning() << "GLShaderObject: unexpected 'shader_type': " << shader_type;
		GPlatesGlobal::Abort(GPLATES_EXCEPTION_SOURCE);
		break;
	}

	// Shouldn't be able to get here.
	return false;
}


GPlatesOpenGL::GLShaderObject::GLShaderObject(
		GLRenderer &renderer,
		GLenum shader_type) :
	d_resource(
			resource_type::create(
					renderer.get_context().get_shared_state()->get_shader_object_resource_manager(shader_type)))
{
	// We should only get here if the shader objects extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_ARB_shader_objects),
			GPLATES_ASSERTION_SOURCE);
}


void
GPlatesOpenGL::GLShaderObject::gl_shader_source(
		GLRenderer &renderer,
		const std::vector<const char *> &source_strings)
{
	const GLsizei count = source_strings.size();
	if (count == 0)
	{
		return;
	}

	// For some reason glShaderSourceARB accepts a *non*-const pointer to an array of strings.
	// So we have to copy the caller's 'const' array.
	std::vector<const char *> strings(source_strings);

	// 'length' is NULL indicating the source strings are null-terminated.
	glShaderSourceARB(get_shader_resource_handle(), count, &strings[0], NULL);
}


void
GPlatesOpenGL::GLShaderObject::gl_shader_source(
		GLRenderer &renderer,
		const std::vector<std::string> &source_strings)
{
	const GLsizei count = source_strings.size();
	if (count == 0)
	{
		return;
	}

	// Allocate an array of 'char' string pointers.
	boost::scoped_array<const GLchar *> strings(new const GLchar *[count]);
	for (GLsizei n = 0; n < count; ++n)
	{
		strings[n] = source_strings[n].c_str();
	}

	// 'length' is NULL indicating the source strings are null-terminated.
	glShaderSourceARB(get_shader_resource_handle(), count, strings.get(), NULL);
}


void
GPlatesOpenGL::GLShaderObject::gl_shader_source(
		GLRenderer &renderer,
		const char *source_string)
{
	// 'length' is NULL indicating the source string is null-terminated.
	glShaderSourceARB(get_shader_resource_handle(), 1/*count*/, &source_string, NULL);
}


void
GPlatesOpenGL::GLShaderObject::gl_shader_source(
		GLRenderer &renderer,
		const std::string &source_string)
{
	gl_shader_source(renderer, source_string.c_str());
}


bool
GPlatesOpenGL::GLShaderObject::gl_compile_shader(
		GLRenderer &renderer)
{
	const resource_handle_type shader_resource_handle = get_shader_resource_handle();

	glCompileShaderARB(shader_resource_handle);

	// Check the status of the compilation.
	GLint compile_status;
	glGetObjectParameterivARB(shader_resource_handle, GL_OBJECT_COMPILE_STATUS_ARB, &compile_status);

	// Log a compile diagnostic message if compilation was unsuccessful.
	if (!compile_status)
	{
		// Determine the length of the info log message.
		GLint info_log_length;
		glGetObjectParameterivARB(shader_resource_handle, GL_OBJECT_INFO_LOG_LENGTH_ARB, &info_log_length);

		// Allocate and read the info log message.
		boost::scoped_array<GLcharARB> info_log(new GLcharARB[info_log_length]);
		glGetInfoLogARB(shader_resource_handle, info_log_length, NULL, info_log.get());
		// ...the returned string is null-terminated.

		// Log the shader info log.
		qWarning() << "Unable to compile OpenGL shader source code: ";
		qWarning() << info_log.get();

		return false;
	}

	return true;
}
