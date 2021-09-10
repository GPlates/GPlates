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

#include <opengl/OpenGL3.h>  // Should be included at TOP of ".cc" file.

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

#include "global/AssertionFailureException.h"
#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


// We use macros in <GL/glew.h> that contain old-style casts.
DISABLE_GCC_WARNING("-Wold-style-cast")


GPlatesOpenGL::GLProgram::GLProgram(
		GL &gl) :
	d_resource(
			resource_type::create(
					gl.get_capabilities(),
					gl.get_context().get_shared_state()->get_program_resource_manager()))
{
}


void
GPlatesOpenGL::GLProgram::attach_shader(
		const GLShader::shared_ptr_to_const_type &shader)
{
	d_shaders.insert(shader);

	glAttachShader(get_resource_handle(), shader->get_resource_handle());
}


void
GPlatesOpenGL::GLProgram::detach_shader(
		const GLShader::shared_ptr_to_const_type &shader)
{
	glDetachShader(get_resource_handle(), shader->get_resource_handle());

	d_shaders.erase(shader);
}


void
GPlatesOpenGL::GLProgram::link_program()
{
	// First clear our mapping of uniform names to uniform indices (locations).
	// Linking (or re-linking) can change the indices.
	// When the client sets uniforms variables, after (re)linking, they will get cached (again) as needed.
	d_uniform_locations.clear();

	const GLuint program_resource_handle = get_resource_handle();

	// Link the attached compiled shader objects into a program.
	glLinkProgram(program_resource_handle);

	// Check the status of linking.
	GLint link_status;
	glGetProgramiv(program_resource_handle, GL_LINK_STATUS, &link_status);

	// Log a link diagnostic message if compilation was unsuccessful.
	if (!link_status)
	{
		qDebug() << "Unable to link OpenGL program: ";

		// Log the program info log.
		output_info_log();

		throw OpenGLException(
				GPLATES_EXCEPTION_SOURCE,
				"Unable to link OpenGL program. See log file for details.");
	}
}


void
GPlatesOpenGL::GLProgram::validate_program()
{
	const GLuint program_resource_handle = get_resource_handle();

	glValidateProgram(program_resource_handle);

	// Check the validation status.
	GLint validate_status;
	glGetProgramiv(program_resource_handle, GL_VALIDATE_STATUS, &validate_status);

	if (!validate_status)
	{
		// Log the validate diagnostic message.
		qDebug() << "Validation of OpenGL program failed: ";

		// Log the program info log.
		output_info_log();

		throw OpenGLException(
				GPLATES_EXCEPTION_SOURCE,
				"Unable to validate OpenGL program. See log file for details.");
	}
}


GLint
GPlatesOpenGL::GLProgram::get_uniform_location(
		const char *uniform_name) const
{
	std::pair<uniform_location_map_type::iterator, bool> uniform_insert_result =
			d_uniform_locations.insert(
					uniform_location_map_type::value_type(std::string(uniform_name), 0/*dummy index*/));
	if (uniform_insert_result.second)
	{
		// The uniform name was inserted which means it didn't already exist.
		// So find, and assign, its location index.
		// Note that the location might be -1 (indicating it's not an active uniform).
		const GLint uniform_location = glGetUniformLocation(get_resource_handle(), uniform_name);

		// Override the dummy index (location) with the correct one (or -1 for not-found).
		uniform_insert_result.first->second = uniform_location;
	}

	return uniform_insert_result.first->second;
}


GLuint
GPlatesOpenGL::GLProgram::get_resource_handle() const
{
	return d_resource->get_resource_handle();
}


void
GPlatesOpenGL::GLProgram::output_info_log()
{
	std::set<QString> shader_filenames;

	// Get a list of unique shader code segments filenames for all shader objects linked.
	shader_seq_type::const_iterator shaders_iter = d_shaders.begin();
	shader_seq_type::const_iterator shaders_end = d_shaders.end();
	for ( ; shaders_iter != shaders_end; ++shaders_iter)
	{
		const GLShader &shader = **shaders_iter;

		// Get the file source code segments of the current shader.
		const std::vector<GLShader::FileCodeSegment> file_code_segments =
				shader.get_file_code_segments();

		for (unsigned int n = 0; n < file_code_segments.size(); ++n)
		{
			shader_filenames.insert(file_code_segments[n].filename);
		}
	}

	// Log the shader info log.

	const GLuint program_resource_handle = get_resource_handle();

	// Determine the length of the info log message.
	GLint info_log_length;
	glGetProgramiv(program_resource_handle, GL_INFO_LOG_LENGTH, &info_log_length);

	// Allocate and read the info log message.
	boost::scoped_array<GLchar> info_log(new GLchar[info_log_length]);
	glGetProgramInfoLog(program_resource_handle, info_log_length, NULL, info_log.get());
	// ...the returned string is null-terminated.

	// If some of the shader code segments came from files then print that information since
	// it's useful to help locate which compiled shader files were linked.
	if (!shader_filenames.empty())
	{
		qDebug() << " Some (or all) source segments came from files: ";

		std::set<QString>::const_iterator shader_filenames_iter = shader_filenames.begin();
		std::set<QString>::const_iterator shader_filenames_end = shader_filenames.end();
		for ( ; shader_filenames_iter != shader_filenames_end; ++shader_filenames_iter)
		{
			const QString shader_filename = *shader_filenames_iter;

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
	const GLCapabilities &capabilities)
{
	const GLuint program = glCreateProgram();

	GPlatesGlobal::Assert<OpenGLException>(
		program,
		GPLATES_ASSERTION_SOURCE,
		"Failed to create shader program object.");

	return program;
}


void
GPlatesOpenGL::GLProgram::Allocator::deallocate(
	GLuint program)
{
	glDeleteProgram(program);
}
