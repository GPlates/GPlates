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

#include <boost/scoped_array.hpp>
/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>
#include <opengl/OpenGL.h>
#include <QDebug>

#include "GLShaderObject.h"

#include "GLContext.h"
#include "GLRenderer.h"
#include "GLShaderSource.h"
#include "OpenGLException.h"

#include "global/AssertionFailureException.h"
#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


// We use macros in <GL/glew.h> that contain old-style casts.
DISABLE_GCC_WARNING("-Wold-style-cast")


GPlatesOpenGL::GLShaderObject::resource_handle_type
GPlatesOpenGL::GLShaderObject::Allocator::allocate(
		const GLCapabilities &capabilities)
{
	// We should only get here if the shader objects extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			capabilities.shader.gl_ARB_shader_objects,
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
	glDeleteObjectARB(shader_object);
}


bool
GPlatesOpenGL::GLShaderObject::is_supported(
		GLRenderer &renderer,
		GLenum shader_type)
{
	const GLCapabilities &capabilities = renderer.get_capabilities();

	if (!capabilities.shader.gl_ARB_shader_objects)
	{
		return false;
	}

	switch (shader_type)
	{
	case GL_VERTEX_SHADER_ARB:
		return capabilities.shader.gl_ARB_vertex_shader;

	case GL_FRAGMENT_SHADER_ARB:
		return capabilities.shader.gl_ARB_fragment_shader;

#ifdef GL_EXT_geometry_shader4 // In case old 'glew.h' (since extension added relatively recently in OpenGL 3.2).
	case GL_GEOMETRY_SHADER_EXT:
		return capabilities.shader.gl_EXT_geometry_shader4;
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
					renderer.get_capabilities(),
					renderer.get_context().get_shared_state()->get_shader_object_resource_manager(
							renderer,
							shader_type)))
{
	const GLCapabilities &capabilities = renderer.get_capabilities();

	// We should only get here if the shader objects extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			capabilities.shader.gl_ARB_shader_objects,
			GPLATES_ASSERTION_SOURCE);
}


void
GPlatesOpenGL::GLShaderObject::gl_shader_source(
		GLRenderer &renderer,
		const GLShaderSource &shader_source)
{
	const std::vector<GLShaderSource::CodeSegment> &source_code_segments =
			shader_source.get_code_segments();

	const GLsizei count = source_code_segments.size();
	if (count == 0)
	{
		d_source_code_segments = boost::none;
		return;
	}

	// Allocate an array of 'char' string pointers.
	boost::scoped_array<const GLchar *> strings(new const GLchar *[count]);

	// Add the caller's shader source segments.
	d_source_code_segments = std::vector<SourceCodeSegment>();
	for (GLsizei n = 0; n < count; ++n)
	{
		strings[n] = source_code_segments[n].source_code.constData();

		// Also keep track of relevant information about each source code segment in case
		// we fail to compile (and hence can print out files and line numbers to lookup).
		d_source_code_segments->push_back(SourceCodeSegment(source_code_segments[n]));
	}

	// 'length' is NULL indicating the source strings are null-terminated.
	glShaderSourceARB(get_shader_resource_handle(), count, strings.get(), NULL);
}


bool
GPlatesOpenGL::GLShaderObject::gl_compile_shader(
		GLRenderer &renderer)
{
	// 'gl_shader_source()' should have been called first.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_source_code_segments,
			GPLATES_ASSERTION_SOURCE);

	const resource_handle_type shader_resource_handle = get_shader_resource_handle();

	glCompileShaderARB(shader_resource_handle);

	// Check the status of the compilation.
	GLint compile_status;
	glGetObjectParameterivARB(shader_resource_handle, GL_OBJECT_COMPILE_STATUS_ARB, &compile_status);

	// If the compilation was unsuccessful then log a compile diagnostic message.
	if (!compile_status)
	{
		output_info_log();

		return false;
	}

	return true;
}


std::vector<GPlatesOpenGL::GLShaderObject::FileCodeSegment>
GPlatesOpenGL::GLShaderObject::get_file_code_segments() const
{
	// 'gl_shader_source()' should have been called first.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_source_code_segments,
			GPLATES_ASSERTION_SOURCE);

	std::vector<FileCodeSegment> file_code_segments;

	// Iterate over the source code segments that were compiled together (in order) and find
	// any code segments that were loaded from a file.
	unsigned int cumulative_line_number = 0;
	const unsigned int num_code_segments = d_source_code_segments->size();
	for (unsigned int code_segment_index = 0; code_segment_index < num_code_segments; ++code_segment_index)
	{
		const SourceCodeSegment &source_code_segment = d_source_code_segments->at(code_segment_index);

		if (source_code_segment.source_file_name)
		{
			file_code_segments.push_back(
					FileCodeSegment(
							cumulative_line_number,
							cumulative_line_number + source_code_segment.num_lines - 1,
							source_code_segment.source_file_name.get()));
		}

		cumulative_line_number += source_code_segment.num_lines;
	}

	return file_code_segments;
}


void
GPlatesOpenGL::GLShaderObject::output_info_log()
{
	// Iterate over the source code segments that were compiled together (in order) and find
	// any code segments that were loaded from a file.
	const std::vector<FileCodeSegment> file_code_segments = get_file_code_segments();

	// Log the shader info log.

	// If some of the shader code segments came from files then print that information to
	// help locate the line number in GLSL error message.
	if (!file_code_segments.empty())
	{
		qDebug() << "Unable to compile OpenGL shader source code consisting of the following file code segments: ";

		const unsigned int num_file_code_segments = file_code_segments.size();
		for (unsigned int file_code_segment_index = 0;
			file_code_segment_index < num_file_code_segments;
			++file_code_segment_index)
		{
			const FileCodeSegment &file_code_segment = file_code_segments[file_code_segment_index];

			qDebug() << "  '" << file_code_segment.filename
				<< "' maps to line range [" << file_code_segment.first_line_number
				<< "," << file_code_segment.last_line_number
				<< "] in concatenated shader source.";
		}
	}
	else
	{
		qDebug() << "Unable to compile OpenGL shader source code consisting of string literals: ";
	}

	const resource_handle_type shader_resource_handle = get_shader_resource_handle();

	// Determine the length of the info log message.
	GLint info_log_length;
	glGetObjectParameterivARB(shader_resource_handle, GL_OBJECT_INFO_LOG_LENGTH_ARB, &info_log_length);

	// Allocate and read the info log message.
	boost::scoped_array<GLcharARB> info_log(new GLcharARB[info_log_length]);
	glGetInfoLogARB(shader_resource_handle, info_log_length, NULL, info_log.get());
	// ...the returned string is null-terminated.

	qDebug() << endl << info_log.get() << endl;
}


GPlatesOpenGL::GLShaderObject::SourceCodeSegment::SourceCodeSegment(
		const GLShaderSource::CodeSegment &source_code_segment) :
	num_lines(source_code_segment.num_lines),
	source_file_name(source_code_segment.source_file_name)
{
	// We avoid copying the source code to save a little memory.
}
