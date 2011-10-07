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

#include "GLProgramObject.h"

#include "GLContext.h"
#include "GLRenderer.h"
#include "GLShaderObject.h"
#include "OpenGLException.h"

#include "global/AssertionFailureException.h"
#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


// We use macros in <GL/glew.h> that contain old-style casts.
DISABLE_GCC_WARNING("-Wold-style-cast")


GPlatesOpenGL::GLProgramObject::resource_handle_type
GPlatesOpenGL::GLProgramObject::Allocator::allocate()
{
	// We should only get here if the shader objects extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_ARB_shader_objects),
			GPLATES_ASSERTION_SOURCE);

	const resource_handle_type program_object = glCreateProgramObjectARB();

	GPlatesGlobal::Assert<OpenGLException>(
			program_object,
			GPLATES_ASSERTION_SOURCE,
			"Failed to create shader program object.");

	return program_object;
}


void
GPlatesOpenGL::GLProgramObject::Allocator::deallocate(
		resource_handle_type program_object)
{
	// We should only get here if the shader objects extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_ARB_shader_objects),
			GPLATES_ASSERTION_SOURCE);

	glDeleteObjectARB(program_object);
}


bool
GPlatesOpenGL::GLProgramObject::is_supported(
		GLRenderer &renderer)
{
	return GLEW_ARB_shader_objects && GLEW_ARB_vertex_shader;
}


GPlatesOpenGL::GLProgramObject::GLProgramObject(
		GLRenderer &renderer) :
	d_resource(
			resource_type::create(
					renderer.get_context().get_shared_state()->get_program_object_resource_manager()))
{
	// We should only get here if the shader objects extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_ARB_shader_objects),
			GPLATES_ASSERTION_SOURCE);
}


void
GPlatesOpenGL::GLProgramObject::gl_attach_shader(
		GLRenderer &renderer,
		const GLShaderObject &shader)
{
	glAttachObjectARB(get_program_resource_handle(), shader.get_shader_resource_handle());
}


void
GPlatesOpenGL::GLProgramObject::gl_detach_shader(
		GLRenderer &renderer,
		const GLShaderObject &shader)
{
	glDetachObjectARB(get_program_resource_handle(), shader.get_shader_resource_handle());
}


void
GPlatesOpenGL::GLProgramObject::gl_bind_attrib_location(
		const char *attribute_name,
		GLuint attribute_index)
{
	glBindAttribLocationARB(get_program_resource_handle(), attribute_index, attribute_name);
}


bool
GPlatesOpenGL::GLProgramObject::gl_link_program(
		GLRenderer &renderer)
{
	// First clear our mapping of uniform names to uniform indices (locations).
	// Linking (or re-linking) can change the indices.
	// When the client sets uniforms variables, after (re)linking, they will get cached (again) as needed.
	d_uniform_locations.clear();

	const resource_handle_type program_resource_handle = get_program_resource_handle();

	// Link the attached compiled shader objects into a program.
	glLinkProgramARB(program_resource_handle);

	// Check the status of linking.
	GLint link_status;
	glGetObjectParameterivARB(program_resource_handle, GL_OBJECT_LINK_STATUS_ARB, &link_status);

	// Log a link diagnostic message if compilation was unsuccessful.
	if (!link_status)
	{
		// Determine the length of the info log message.
		GLint info_log_length;
		glGetObjectParameterivARB(program_resource_handle, GL_OBJECT_INFO_LOG_LENGTH_ARB, &info_log_length);

		// Allocate and read the info log message.
		boost::scoped_array<GLcharARB> info_log(new GLcharARB[info_log_length]);
		glGetInfoLogARB(program_resource_handle, info_log_length, NULL, info_log.get());
		// ...the returned string is null-terminated.

		// Log the program info log.
		qWarning() << "Unable to link OpenGL program: ";
		qWarning() << info_log.get();

		return false;
	}

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_validate_program(
		GLRenderer &renderer)
{
	const resource_handle_type program_resource_handle = get_program_resource_handle();

	glValidateProgramARB(program_resource_handle);

	// Check the validation status.
	GLint validate_status;
	glGetObjectParameterivARB(program_resource_handle, GL_OBJECT_VALIDATE_STATUS_ARB, &validate_status);

	// Log the validate diagnostic message.
	// We do this on success *or* failure since this method is really meant for use during development.

	// Determine the length of the info log message.
	GLint info_log_length;
	glGetObjectParameterivARB(program_resource_handle, GL_OBJECT_INFO_LOG_LENGTH_ARB, &info_log_length);

	// Allocate and read the info log message.
	boost::scoped_array<GLcharARB> info_log(new GLcharARB[info_log_length]);
	glGetInfoLogARB(program_resource_handle, info_log_length, NULL, info_log.get());
	// ...the returned string is null-terminated.

	// Log the program info log.
	qDebug() <<
			(validate_status
					? "Validation of OpenGL program succeeded: "
					: "Validation of OpenGL program failed: ");
	qDebug() << info_log.get();

	return validate_status;
}


void
GPlatesOpenGL::GLProgramObject::gl_uniform1f(
		GLRenderer &renderer,
		const char *name,
		GLfloat v0)
{
	// Revert our program object binding on return so we don't affect changes made by clients.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Bind this program object glUniform applies to it.
	// Make sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	renderer.gl_bind_program_object_and_apply(shared_from_this());

	glUniform1fARB(get_uniform_location(name), v0);
}


void
GPlatesOpenGL::GLProgramObject::gl_uniform1f(
		GLRenderer &renderer,
		const char *name,
		const GLfloat *value,
		unsigned int count)
{
	// Revert our program object binding on return so we don't affect changes made by clients.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Bind this program object glUniform applies to it.
	// Make sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	renderer.gl_bind_program_object_and_apply(shared_from_this());

	glUniform1fvARB(get_uniform_location(name), count, value);
}


void
GPlatesOpenGL::GLProgramObject::gl_uniform1i(
		GLRenderer &renderer,
		const char *name,
		GLint v0)
{
	// Revert our program object binding on return so we don't affect changes made by clients.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Bind this program object glUniform applies to it.
	// Make sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	renderer.gl_bind_program_object_and_apply(shared_from_this());

	glUniform1iARB(get_uniform_location(name), v0);
}


void
GPlatesOpenGL::GLProgramObject::gl_uniform1i(
		GLRenderer &renderer,
		const char *name,
		const GLint *value,
		unsigned int count)
{
	// Revert our program object binding on return so we don't affect changes made by clients.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Bind this program object glUniform applies to it.
	// Make sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	renderer.gl_bind_program_object_and_apply(shared_from_this());

	glUniform1ivARB(get_uniform_location(name), count, value);
}


void
GPlatesOpenGL::GLProgramObject::gl_uniform2f(
		GLRenderer &renderer,
		const char *name,
		GLfloat v0,
		GLfloat v1)
{
	// Revert our program object binding on return so we don't affect changes made by clients.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Bind this program object glUniform applies to it.
	// Make sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	renderer.gl_bind_program_object_and_apply(shared_from_this());

	glUniform2fARB(get_uniform_location(name), v0, v1);
}


void
GPlatesOpenGL::GLProgramObject::gl_uniform2f(
		GLRenderer &renderer,
		const char *name,
		const GLfloat *value,
		unsigned count)
{
	// Revert our program object binding on return so we don't affect changes made by clients.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Bind this program object glUniform applies to it.
	// Make sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	renderer.gl_bind_program_object_and_apply(shared_from_this());

	glUniform2fvARB(get_uniform_location(name), count, value);
}


void
GPlatesOpenGL::GLProgramObject::gl_uniform2i(
		GLRenderer &renderer,
		const char *name,
		GLint v0,
		GLint v1)
{
	// Revert our program object binding on return so we don't affect changes made by clients.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Bind this program object glUniform applies to it.
	// Make sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	renderer.gl_bind_program_object_and_apply(shared_from_this());

	glUniform2iARB(get_uniform_location(name), v0, v1);
}


void
GPlatesOpenGL::GLProgramObject::gl_uniform2i(
		GLRenderer &renderer,
		const char *name,
		const GLint *value,
		unsigned int count)
{
	// Revert our program object binding on return so we don't affect changes made by clients.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Bind this program object glUniform applies to it.
	// Make sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	renderer.gl_bind_program_object_and_apply(shared_from_this());

	glUniform2ivARB(get_uniform_location(name), count, value);
}


void
GPlatesOpenGL::GLProgramObject::gl_uniform3f(
		GLRenderer &renderer,
		const char *name,
		GLfloat v0,
		GLfloat v1,
		GLfloat v2)
{
	// Revert our program object binding on return so we don't affect changes made by clients.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Bind this program object glUniform applies to it.
	// Make sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	renderer.gl_bind_program_object_and_apply(shared_from_this());

	glUniform3fARB(get_uniform_location(name), v0, v1, v2);
}


void
GPlatesOpenGL::GLProgramObject::gl_uniform3f(
		GLRenderer &renderer,
		const char *name,
		const GLfloat *value,
		unsigned count)
{
	// Revert our program object binding on return so we don't affect changes made by clients.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Bind this program object glUniform applies to it.
	// Make sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	renderer.gl_bind_program_object_and_apply(shared_from_this());

	glUniform3fvARB(get_uniform_location(name), count, value);
}


void
GPlatesOpenGL::GLProgramObject::gl_uniform3i(
		GLRenderer &renderer,
		const char *name,
		GLint v0,
		GLint v1,
		GLint v2)
{
	// Revert our program object binding on return so we don't affect changes made by clients.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Bind this program object glUniform applies to it.
	// Make sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	renderer.gl_bind_program_object_and_apply(shared_from_this());

	glUniform3iARB(get_uniform_location(name), v0, v1, v2);
}


void
GPlatesOpenGL::GLProgramObject::gl_uniform3i(
		GLRenderer &renderer,
		const char *name,
		const GLint *value,
		unsigned int count)
{
	// Revert our program object binding on return so we don't affect changes made by clients.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Bind this program object glUniform applies to it.
	// Make sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	renderer.gl_bind_program_object_and_apply(shared_from_this());

	glUniform3ivARB(get_uniform_location(name), count, value);
}


void
GPlatesOpenGL::GLProgramObject::gl_uniform4f(
		GLRenderer &renderer,
		const char *name,
		GLfloat v0,
		GLfloat v1,
		GLfloat v2,
		GLfloat v3)
{
	// Revert our program object binding on return so we don't affect changes made by clients.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Bind this program object glUniform applies to it.
	// Make sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	renderer.gl_bind_program_object_and_apply(shared_from_this());

	glUniform4fARB(get_uniform_location(name), v0, v1, v2, v3);
}


void
GPlatesOpenGL::GLProgramObject::gl_uniform4f(
		GLRenderer &renderer,
		const char *name,
		const GLfloat *value,
		unsigned count)
{
	// Revert our program object binding on return so we don't affect changes made by clients.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Bind this program object glUniform applies to it.
	// Make sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	renderer.gl_bind_program_object_and_apply(shared_from_this());

	glUniform4fvARB(get_uniform_location(name), count, value);
}


void
GPlatesOpenGL::GLProgramObject::gl_uniform4i(
		GLRenderer &renderer,
		const char *name,
		GLint v0,
		GLint v1,
		GLint v2,
		GLint v3)
{
	// Revert our program object binding on return so we don't affect changes made by clients.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Bind this program object glUniform applies to it.
	// Make sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	renderer.gl_bind_program_object_and_apply(shared_from_this());

	glUniform4iARB(get_uniform_location(name), v0, v1, v2, v3);
}


void
GPlatesOpenGL::GLProgramObject::gl_uniform4i(
		GLRenderer &renderer,
		const char *name,
		const GLint *value,
		unsigned int count)
{
	// Revert our program object binding on return so we don't affect changes made by clients.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Bind this program object glUniform applies to it.
	// Make sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	renderer.gl_bind_program_object_and_apply(shared_from_this());

	glUniform4ivARB(get_uniform_location(name), count, value);
}


void
GPlatesOpenGL::GLProgramObject::gl_uniform_matrix2x2(
		GLRenderer &renderer,
		const char *name,
		const GLfloat *value,
		unsigned int count,
		GLboolean transpose)
{
	// Revert our program object binding on return so we don't affect changes made by clients.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Bind this program object glUniform applies to it.
	// Make sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	renderer.gl_bind_program_object_and_apply(shared_from_this());

	glUniformMatrix2fvARB(get_uniform_location(name), count, transpose, value);
}


void
GPlatesOpenGL::GLProgramObject::gl_uniform_matrix3x3(
		GLRenderer &renderer,
		const char *name,
		const GLfloat *value,
		unsigned int count,
		GLboolean transpose)
{
	// Revert our program object binding on return so we don't affect changes made by clients.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Bind this program object glUniform applies to it.
	// Make sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	renderer.gl_bind_program_object_and_apply(shared_from_this());

	glUniformMatrix3fvARB(get_uniform_location(name), count, transpose, value);
}


void
GPlatesOpenGL::GLProgramObject::gl_uniform_matrix4x4(
		GLRenderer &renderer,
		const char *name,
		const GLfloat *value,
		unsigned int count,
		GLboolean transpose)
{
	// Revert our program object binding on return so we don't affect changes made by clients.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Bind this program object glUniform applies to it.
	// Make sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	renderer.gl_bind_program_object_and_apply(shared_from_this());

	glUniformMatrix4fvARB(get_uniform_location(name), count, transpose, value);
}


void
GPlatesOpenGL::GLProgramObject::gl_uniform_matrix4x4(
		GLRenderer &renderer,
		const char *name,
		const GLMatrix &matrix)
{
	// Revert our program object binding on return so we don't affect changes made by clients.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Bind this program object glUniform applies to it.
	// Make sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	renderer.gl_bind_program_object_and_apply(shared_from_this());

	const GLdouble *const double_matrix = matrix.get_matrix();

	// Copy the matrix GLdouble elements into GLfloat elements.
	GLfloat float_matrix[16];
	for (int n = 0; n < 16; ++n)
	{
		float_matrix[n] = double_matrix[n];
	}

	// Note that the matrix is in column-major format.
	glUniformMatrix4fvARB(get_uniform_location(name), 1, GL_FALSE/*transpose*/, float_matrix);
}


void
GPlatesOpenGL::GLProgramObject::gl_uniform_matrix4x4(
		GLRenderer &renderer,
		const char *name,
		const std::vector<GLMatrix> &matrices)
{
	// Revert our program object binding on return so we don't affect changes made by clients.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Bind this program object glUniform applies to it.
	// Make sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	renderer.gl_bind_program_object_and_apply(shared_from_this());

	// Iterate over the matrices and convert from double to float.
	boost::scoped_array<GLfloat> float_matrices(new GLfloat[16 * matrices.size()]);
	for (unsigned int m = 0; m < matrices.size(); ++m)
	{
		const GLMatrix &matrix = matrices[m];
		const GLdouble *const double_matrix = matrix.get_matrix();

		// Copy the matrix GLdouble elements into GLfloat elements.
		GLfloat *const float_matrix = float_matrices.get() + 16 * m;
		for (unsigned int n = 0; n < 16; ++n)
		{
			float_matrix[n] = double_matrix[n];
		}
	}

	glUniformMatrix4fvARB(get_uniform_location(name), matrices.size(), GL_FALSE/*transpose*/, float_matrices.get());
}


GPlatesOpenGL::GLProgramObject::uniform_location_type
GPlatesOpenGL::GLProgramObject::get_uniform_location(
		const char *uniform_name) const
{
	std::pair<uniform_location_map_type::iterator, bool> uniform_insert_result =
			d_uniform_locations.insert(
					uniform_location_map_type::value_type(uniform_name_type(uniform_name), 0/*dummy index*/));
	if (uniform_insert_result.second)
	{
		// The uniform name was inserted which means it didn't already exist.
		// So find, and assign, its location index.
		const uniform_location_type uniform_location =
				glGetUniformLocationARB(get_program_resource_handle(), uniform_name);

		GPlatesGlobal::Assert<OpenGLException>(
				uniform_location >= 0,
				GPLATES_ASSERTION_SOURCE,
				"Attempted to set uniform variable, on a shader program, that (1) does not exist, "
				"(2) is not actively used in the linked shader program or (3) is a reserved name.");

		// Override the dummy index (location) with the correct one.
		uniform_insert_result.first->second = uniform_location;
	}

	return uniform_insert_result.first->second;
}
