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


const char *GPlatesOpenGL::GLShaderObject::SHADER_VERSION_STRINGS[GPlatesOpenGL::GLShaderObject::NUM_SHADER_VERSIONS] =
{
	"#version 110\n",
	"#version 120\n",
	"#version 130\n",
	"#version 140\n",
	"#version 150 compatibility\n",
	"#version 330 compatibility\n",
	"#version 400 compatibility\n",
	"#version 410 compatibility\n",
	"#version 420 compatibility\n"
};


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

#ifdef GL_EXT_geometry_shader4 // In case old 'glew.h' (since extension added relatively recently in OpenGL 3.2).
	case GL_GEOMETRY_SHADER_EXT:
		return GPLATES_OPENGL_BOOL(GLEW_EXT_geometry_shader4);
#endif

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
		const std::vector<const char *> &source_strings,
		ShaderVersion shader_version)
{
	if (source_strings.empty())
	{
		return;
	}

	// For some reason glShaderSourceARB accepts a *non*-const pointer to an array of strings.
	// So we have to copy the caller's 'const' array.
	// We have to do it anyway since we're also adding the shader version string.
	std::vector<const char *> strings;
	strings.reserve(source_strings.size() + 1);

	// Add the shader version string first (it needs to come before any non-commented source code).
	strings.push_back(SHADER_VERSION_STRINGS[shader_version]);

	// Add the caller's shader source segments.
	strings.insert(strings.end(), source_strings.begin(), source_strings.end());

	// 'length' is NULL indicating the source strings are null-terminated.
	glShaderSourceARB(get_shader_resource_handle(), strings.size(), &strings[0], NULL);
}


void
GPlatesOpenGL::GLShaderObject::gl_shader_source(
		GLRenderer &renderer,
		const std::vector<QByteArray> &source_strings,
		ShaderVersion shader_version)
{
	const GLsizei count = source_strings.size();
	if (count == 0)
	{
		return;
	}

	// Allocate an array of 'char' string pointers.
	boost::scoped_array<const GLchar *> strings(new const GLchar *[count + 1]);

	// Add the shader version string first (it needs to come before any non-commented source code).
	strings[0] = SHADER_VERSION_STRINGS[shader_version];

	// Add the caller's shader source segments.
	for (GLsizei n = 0; n < count; ++n)
	{
		strings[n + 1] = source_strings[n].constData();
	}

	// 'length' is NULL indicating the source strings are null-terminated.
	glShaderSourceARB(get_shader_resource_handle(), count + 1, strings.get(), NULL);
}


void
GPlatesOpenGL::GLShaderObject::gl_shader_source(
		GLRenderer &renderer,
		const char *source_string,
		ShaderVersion shader_version)
{
	gl_shader_source(
			renderer,
			std::vector<const char *>(1, source_string),
			shader_version);
}


void
GPlatesOpenGL::GLShaderObject::gl_shader_source(
		GLRenderer &renderer,
		const QByteArray &source_string,
		ShaderVersion shader_version)
{
	gl_shader_source(renderer, source_string.constData(), shader_version);
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
		qDebug() << "Unable to compile OpenGL shader source code: ";
		qDebug() << info_log.get();

		return false;
	}

	return true;
}
