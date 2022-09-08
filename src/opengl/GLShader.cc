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
#include <QDebug>
#include <QtGlobal>

#include "GLShader.h"

#include "GLContext.h"
#include "GLShaderSource.h"
#include "OpenGL.h"  // For Class GL
#include "OpenGLException.h"
#include "OpenGLFunctions.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


GPlatesOpenGL::GLShader::GLShader(
		GL &gl,
		GLenum shader_type) :
	d_resource(
			resource_type::create(
					gl.get_opengl_functions(),
					gl.get_capabilities(),
					gl.get_context().d_shader_resource_manager,
					shader_type))
{
}


void
GPlatesOpenGL::GLShader::shader_source(
		GL &gl,
		const GLShaderSource &shader_source)
{
	const std::vector<GLShaderSource::CodeSegment> source_code_segments =
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
	gl.get_opengl_functions().glShaderSource(get_resource_handle(), count, strings.get(), NULL);
}


void
GPlatesOpenGL::GLShader::compile_shader(
		GL &gl)
{
	// 'shader_source()' should have been called first.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_source_code_segments,
			GPLATES_ASSERTION_SOURCE);

	const GLuint shader_resource_handle = get_resource_handle();

	gl.get_opengl_functions().glCompileShader(shader_resource_handle);

	// Check the status of the compilation.
	GLint compile_status;
	gl.get_opengl_functions().glGetShaderiv(shader_resource_handle, GL_COMPILE_STATUS, &compile_status);

	// If the compilation was unsuccessful then log a compile diagnostic message.
	if (!compile_status)
	{
		qDebug() << "Unable to compile OpenGL shader source code: ";

		// Log the shader info log.
		output_info_log(gl);

		throw OpenGLException(
				GPLATES_EXCEPTION_SOURCE,
				"Unable to compile OpenGL shader source code. See log file for details.");
	}
}


std::vector<GPlatesOpenGL::GLShader::FileCodeSegment>
GPlatesOpenGL::GLShader::get_file_code_segments() const
{
	// 'shader_source()' should have been called first.
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


GLuint
GPlatesOpenGL::GLShader::get_resource_handle() const
{
	return d_resource->get_resource_handle();
}


void
GPlatesOpenGL::GLShader::output_info_log(
		GL &gl)
{
	// Iterate over the source code segments that were compiled together (in order) and find
	// any code segments that were loaded from a file.
	const std::vector<FileCodeSegment> file_code_segments = get_file_code_segments();

	// Log the shader info log.

	// If some of the shader code segments came from files then print that information to
	// help locate the line number in GLSL error message.
	if (!file_code_segments.empty())
	{
		qDebug() << " Some (or all) source segments came from files: ";

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
		qDebug() << " (all source segments consisted of string literals)";
	}

	const GLuint shader_resource_handle = get_resource_handle();

	// Determine the length of the info log message.
	GLint info_log_length;
	gl.get_opengl_functions().glGetShaderiv(shader_resource_handle, GL_INFO_LOG_LENGTH, &info_log_length);

	// Allocate and read the info log message.
	boost::scoped_array<GLchar> info_log(new GLchar[info_log_length]);
	gl.get_opengl_functions().glGetShaderInfoLog(shader_resource_handle, info_log_length, NULL, info_log.get());
	// ...the returned string is null-terminated.

	qDebug()
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
		<< Qt::endl
#else
		<< endl
#endif
		<< info_log.get()
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
		<< Qt::endl;
#else
		<< endl;
#endif
}


GLuint
GPlatesOpenGL::GLShader::Allocator::allocate(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities,
		GLenum shader_type)
{
	const GLuint shader = opengl_functions.glCreateShader(shader_type);

	GPlatesGlobal::Assert<OpenGLException>(
			shader,
			GPLATES_ASSERTION_SOURCE,
			"Failed to create shader object.");

	return shader;
}


void
GPlatesOpenGL::GLShader::Allocator::deallocate(
		OpenGLFunctions &opengl_functions,
		GLuint shader)
{
	opengl_functions.glDeleteShader(shader);
}
