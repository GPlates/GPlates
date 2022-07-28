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
#include <set>
#include <sstream>

#include <boost/scoped_array.hpp>
#include <QDebug>
#include <QString>
#include <QtGlobal>

#include "GLProgram.h"

#include "GL.h"
#include "GLContext.h"
#include "GLShader.h"
#include "OpenGLException.h"
#include "OpenGLFunctions.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


GPlatesOpenGL::GLProgram::GLProgram(
		GL &gl) :
	d_resource(
			resource_type::create(
					gl.get_opengl_functions(),
					gl.get_capabilities(),
					gl.get_context().get_shared_state()->get_program_resource_manager()))
{
}


void
GPlatesOpenGL::GLProgram::attach_shader(
		GL &gl,
		const GLShader::shared_ptr_to_const_type &shader)
{
	d_shaders.insert(shader);

	gl.get_opengl_functions().glAttachShader(get_resource_handle(), shader->get_resource_handle());
}


void
GPlatesOpenGL::GLProgram::detach_shader(
		GL &gl,
		const GLShader::shared_ptr_to_const_type &shader)
{
	gl.get_opengl_functions().glDetachShader(get_resource_handle(), shader->get_resource_handle());

	d_shaders.erase(shader);
}


void
GPlatesOpenGL::GLProgram::link_program(
		GL &gl)
{
	// First clear our mapping of uniform names to uniform locations (in default uniform block) and
	// the mapping of uniform block names to uniform block indices (for named uniform blocks).
	// Linking (or re-linking) can change these.
	// They will get cached (again) as needed when the client subsequently calls
	// 'get_uniform_location()' and 'get_uniform_block_index'.
	d_uniform_locations.clear();
	d_uniform_block_indices.clear();

	const GLuint program_resource_handle = get_resource_handle();

	// Link the attached compiled shader objects into a program.
	gl.get_opengl_functions().glLinkProgram(program_resource_handle);

	// Check the status of linking.
	GLint link_status;
	gl.get_opengl_functions().glGetProgramiv(program_resource_handle, GL_LINK_STATUS, &link_status);

	// Log a link diagnostic message if compilation was unsuccessful.
	if (!link_status)
	{
		qDebug() << "Unable to link OpenGL program: ";

		// Log the program info log.
		output_info_log(gl);

		throw OpenGLException(
				GPLATES_EXCEPTION_SOURCE,
				"Unable to link OpenGL program. See log file for details.");
	}
}


void
GPlatesOpenGL::GLProgram::validate_program(
		GL &gl)
{
	const GLuint program_resource_handle = get_resource_handle();

	gl.get_opengl_functions().glValidateProgram(program_resource_handle);

	// Check the validation status.
	GLint validate_status;
	gl.get_opengl_functions().glGetProgramiv(program_resource_handle, GL_VALIDATE_STATUS, &validate_status);

	if (!validate_status)
	{
		// Log the validate diagnostic message.
		qDebug() << "Validation of OpenGL program failed: ";

		// Log the program info log.
		output_info_log(gl);

		throw OpenGLException(
				GPLATES_EXCEPTION_SOURCE,
				"Unable to validate OpenGL program. See log file for details.");
	}
}


GLint
GPlatesOpenGL::GLProgram::get_uniform_location(
		GL &gl,
		const char *uniform_name) const
{
	auto uniform_location_insert_result = d_uniform_locations.insert({ std::string(uniform_name), 0/*dummy index*/ });
	if (uniform_location_insert_result.second)
	{
		// The uniform name was inserted which means it didn't already exist.
		// So find, and assign, its location index.
		// Note that the location might be -1 (indicating it's not an active uniform).
		const GLint uniform_location = gl.get_opengl_functions().glGetUniformLocation(get_resource_handle(), uniform_name);

		// Override the dummy index (location) with the correct one (or -1 for not-found).
		uniform_location_insert_result.first->second = uniform_location;
	}

	return uniform_location_insert_result.first->second;
}


GLuint
GPlatesOpenGL::GLProgram::get_uniform_block_index(
		GL &gl,
		const char *uniform_block_name) const
{
	auto uniform_block_index_insert_result = d_uniform_block_indices.insert({ std::string(uniform_block_name), 0/*dummy index*/ });
	if (uniform_block_index_insert_result.second)
	{
		// The uniform block name was inserted which means it didn't already exist.
		// So find, and assign, its block index.
		// Note that the block index might be GL_INVALID_INDEX (indicating it's not an active named uniform block).
		const GLuint uniform_block_index = gl.get_opengl_functions().glGetUniformBlockIndex(get_resource_handle(), uniform_block_name);

		// Override the dummy uniform block index with the correct one (or GL_INVALID_INDEX for not-found).
		uniform_block_index_insert_result.first->second = uniform_block_index;
	}

	return uniform_block_index_insert_result.first->second;
}


GLuint
GPlatesOpenGL::GLProgram::get_resource_handle() const
{
	return d_resource->get_resource_handle();
}


void
GPlatesOpenGL::GLProgram::output_info_log(
		GL &gl)
{
	std::set<QString> shader_filenames;

	// Get a list of unique shader code segments filenames for all shader objects linked.
	for (auto shader_ptr : d_shaders)
	{
		const GLShader &shader = *shader_ptr;

		// Insert the filenames of the file source code segments of the current shader.
		for (const GLShader::FileCodeSegment &file_code_segment : shader.get_file_code_segments())
		{
			shader_filenames.insert(file_code_segment.filename);
		}
	}

	// Log the shader info log.

	const GLuint program_resource_handle = get_resource_handle();

	// Determine the length of the info log message.
	GLint info_log_length;
	gl.get_opengl_functions().glGetProgramiv(program_resource_handle, GL_INFO_LOG_LENGTH, &info_log_length);

	// Allocate and read the info log message.
	boost::scoped_array<GLchar> info_log(new GLchar[info_log_length]);
	gl.get_opengl_functions().glGetProgramInfoLog(program_resource_handle, info_log_length, NULL, info_log.get());
	// ...the returned string is null-terminated.

	// If some of the shader code segments came from files then print that information since
	// it's useful to help locate which compiled shader files were linked.
	if (!shader_filenames.empty())
	{
		qDebug() << " Some (or all) source segments came from files: ";

		for (QString shader_filename : shader_filenames)
		{
			qDebug() << "  '" << shader_filename << "'";
		}
	}
	else
	{
		qDebug() << " (all source segments consisted of string literals)";
	}

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
GPlatesOpenGL::GLProgram::Allocator::allocate(
		OpenGLFunctions &opengl_functions,
		const GLCapabilities &capabilities)
{
	const GLuint program = opengl_functions.glCreateProgram();

	GPlatesGlobal::Assert<OpenGLException>(
		program,
		GPLATES_ASSERTION_SOURCE,
		"Failed to create shader program object.");

	return program;
}


void
GPlatesOpenGL::GLProgram::Allocator::deallocate(
		OpenGLFunctions &opengl_functions,
		GLuint program)
{
	opengl_functions.glDeleteProgram(program);
}
