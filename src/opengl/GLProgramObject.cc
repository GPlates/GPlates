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
/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>
#include <opengl/OpenGL.h>
#include <QDebug>
#include <QString>

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
GPlatesOpenGL::GLProgramObject::Allocator::allocate(
		const GLCapabilities &capabilities)
{
	// We should only get here if the shader objects extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			capabilities.shader.gl_ARB_shader_objects,
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
	glDeleteObjectARB(program_object);
}


bool
GPlatesOpenGL::GLProgramObject::is_supported(
		GLRenderer &renderer)
{
	const GLCapabilities &capabilities = renderer.get_capabilities();

	return capabilities.shader.gl_ARB_shader_objects &&
		capabilities.shader.gl_ARB_vertex_shader;
}


GPlatesOpenGL::GLProgramObject::GLProgramObject(
		GLRenderer &renderer) :
	d_resource(
			resource_type::create(
					renderer.get_capabilities(),
					renderer.get_context().get_shared_state()->get_program_object_resource_manager(renderer)))
{
	const GLCapabilities &capabilities = renderer.get_capabilities();

	// We should only get here if the shader objects extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			capabilities.shader.gl_ARB_shader_objects,
			GPLATES_ASSERTION_SOURCE);
}


void
GPlatesOpenGL::GLProgramObject::gl_attach_shader(
		GLRenderer &renderer,
		const GLShaderObject::shared_ptr_to_const_type &shader)
{
	d_shader_objects.insert(shader);

	glAttachObjectARB(get_program_resource_handle(), shader->get_shader_resource_handle());
}


void
GPlatesOpenGL::GLProgramObject::gl_detach_shader(
		GLRenderer &renderer,
		const GLShaderObject::shared_ptr_to_const_type &shader)
{
	glDetachObjectARB(get_program_resource_handle(), shader->get_shader_resource_handle());

	d_shader_objects.erase(shader);
}


void
GPlatesOpenGL::GLProgramObject::gl_bind_attrib_location(
		const char *attribute_name,
		GLuint attribute_index)
{
	glBindAttribLocationARB(get_program_resource_handle(), attribute_index, attribute_name);
}


void
GPlatesOpenGL::GLProgramObject::gl_program_parameteri(
		GLRenderer &renderer,
		GLenum pname,
		GLint value)
{
	const GLCapabilities &capabilities = renderer.get_capabilities();

// In case old 'glew.h' (since extension added relatively recently).
#if defined(GL_EXT_geometry_shader4)
	// We should only get here if the 'GL_EXT_geometry_shader4' extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			capabilities.shader.gl_EXT_geometry_shader4,
			GPLATES_ASSERTION_SOURCE);

	glProgramParameteriEXT(get_program_resource_handle(), pname, value);
#else
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
#endif
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
		// Log the program info log.
		qDebug() << "Unable to link OpenGL program: ";
		output_info_log();

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
	qDebug() <<
			(validate_status
					? "Validation of OpenGL program succeeded: "
					: "Validation of OpenGL program failed: ");
	// Log the program info log.
	output_info_log();

	return validate_status;
}


bool
GPlatesOpenGL::GLProgramObject::is_active_uniform(
		const char *uniform_name) const
{
	return glGetUniformLocationARB(get_program_resource_handle(), uniform_name) >= 0;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform1f(
		GLRenderer &renderer,
		const char *name,
		GLfloat v0)
{
	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	glUniform1fARB(uniform_location, v0);

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform1f(
		GLRenderer &renderer,
		const char *name,
		const GLfloat *value,
		unsigned int count)
{
	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	glUniform1fvARB(uniform_location, count, value);

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform1i(
		GLRenderer &renderer,
		const char *name,
		GLint v0)
{
	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	glUniform1iARB(uniform_location, v0);

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform1i(
		GLRenderer &renderer,
		const char *name,
		const GLint *value,
		unsigned int count)
{
	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	glUniform1ivARB(uniform_location, count, value);

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform1d(
		GLRenderer &renderer,
		const char *name,
		GLdouble v0)
{
// In case old 'glew.h' (since extension added relatively recently).
// Also it seems some glew headers define 'GL_ARB_gpu_shader_fp64' but not 'glUniform...' API functions.
#if defined(GL_ARB_gpu_shader_fp64) && defined(glUniform1d)
	// We should only get here if the 'GL_ARB_gpu_shader_fp64' extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			renderer.get_capabilities().shader.gl_ARB_gpu_shader_fp64,
			GPLATES_ASSERTION_SOURCE);

	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	glUniform1d(uniform_location, v0);
#else
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
#endif

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform1d(
		GLRenderer &renderer,
		const char *name,
		const GLdouble *value,
		unsigned int count)
{
// In case old 'glew.h' (since extension added relatively recently).
// Also it seems some glew headers define 'GL_ARB_gpu_shader_fp64' but not 'glUniform...' API functions.
#if defined(GL_ARB_gpu_shader_fp64) && defined(glUniform1dv)
	// We should only get here if the 'GL_ARB_gpu_shader_fp64' extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			renderer.get_capabilities().shader.gl_ARB_gpu_shader_fp64,
			GPLATES_ASSERTION_SOURCE);

	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	glUniform1dv(uniform_location, count, value);
#else
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
#endif

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform1ui(
		GLRenderer &renderer,
		const char *name,
		GLuint v0)
{
	const GLCapabilities &capabilities = renderer.get_capabilities();

#ifdef GL_EXT_gpu_shader4 // In case old 'glew.h' (since extension added relatively recently).
	// We should only get here if the 'GL_EXT_gpu_shader4' extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			capabilities.shader.gl_EXT_gpu_shader4,
			GPLATES_ASSERTION_SOURCE);

	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	glUniform1uiEXT(uniform_location, v0);
#else
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
#endif

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform1ui(
		GLRenderer &renderer,
		const char *name,
		const GLuint *value,
		unsigned int count)
{
	const GLCapabilities &capabilities = renderer.get_capabilities();

#ifdef GL_EXT_gpu_shader4 // In case old 'glew.h' (since extension added relatively recently).
	// We should only get here if the 'GL_EXT_gpu_shader4' extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			capabilities.shader.gl_EXT_gpu_shader4,
			GPLATES_ASSERTION_SOURCE);

	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	glUniform1uivEXT(uniform_location, count, value);
#else
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
#endif

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform2f(
		GLRenderer &renderer,
		const char *name,
		GLfloat v0,
		GLfloat v1)
{
	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	glUniform2fARB(uniform_location, v0, v1);

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform2f(
		GLRenderer &renderer,
		const char *name,
		const GLfloat *value,
		unsigned count)
{
	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	glUniform2fvARB(uniform_location, count, value);

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform2i(
		GLRenderer &renderer,
		const char *name,
		GLint v0,
		GLint v1)
{
	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	glUniform2iARB(uniform_location, v0, v1);

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform2i(
		GLRenderer &renderer,
		const char *name,
		const GLint *value,
		unsigned int count)
{
	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	glUniform2ivARB(uniform_location, count, value);

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform2d(
		GLRenderer &renderer,
		const char *name,
		GLdouble v0,
		GLdouble v1)
{
// In case old 'glew.h' (since extension added relatively recently).
// Also it seems some glew headers define 'GL_ARB_gpu_shader_fp64' but not 'glUniform...' API functions.
#if defined(GL_ARB_gpu_shader_fp64) && defined(glUniform2d)
	// We should only get here if the 'GL_ARB_gpu_shader_fp64' extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			renderer.get_capabilities().shader.gl_ARB_gpu_shader_fp64,
			GPLATES_ASSERTION_SOURCE);

	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	glUniform2d(uniform_location, v0, v1);
#else
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
#endif

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform2d(
		GLRenderer &renderer,
		const char *name,
		const GLdouble *value,
		unsigned int count)
{
// In case old 'glew.h' (since extension added relatively recently).
// Also it seems some glew headers define 'GL_ARB_gpu_shader_fp64' but not 'glUniform...' API functions.
#if defined(GL_ARB_gpu_shader_fp64) && defined(glUniform2dv)
	// We should only get here if the 'GL_ARB_gpu_shader_fp64' extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			renderer.get_capabilities().shader.gl_ARB_gpu_shader_fp64,
			GPLATES_ASSERTION_SOURCE);

	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	glUniform2dv(uniform_location, count, value);
#else
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
#endif

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform2ui(
		GLRenderer &renderer,
		const char *name,
		GLuint v0,
		GLuint v1)
{
	const GLCapabilities &capabilities = renderer.get_capabilities();

#ifdef GL_EXT_gpu_shader4 // In case old 'glew.h' (since extension added relatively recently).
	// We should only get here if the 'GL_EXT_gpu_shader4' extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			capabilities.shader.gl_EXT_gpu_shader4,
			GPLATES_ASSERTION_SOURCE);

	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	glUniform2uiEXT(uniform_location, v0, v1);
#else
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
#endif

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform2ui(
		GLRenderer &renderer,
		const char *name,
		const GLuint *value,
		unsigned int count)
{
	const GLCapabilities &capabilities = renderer.get_capabilities();

#ifdef GL_EXT_gpu_shader4 // In case old 'glew.h' (since extension added relatively recently).
	// We should only get here if the 'GL_EXT_gpu_shader4' extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			capabilities.shader.gl_EXT_gpu_shader4,
			GPLATES_ASSERTION_SOURCE);

	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	glUniform2uivEXT(uniform_location, count, value);
#else
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
#endif

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform3f(
		GLRenderer &renderer,
		const char *name,
		GLfloat v0,
		GLfloat v1,
		GLfloat v2)
{
	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	glUniform3fARB(uniform_location, v0, v1, v2);

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform3f(
		GLRenderer &renderer,
		const char *name,
		const GLfloat *value,
		unsigned count)
{
	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	glUniform3fvARB(uniform_location, count, value);

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform3i(
		GLRenderer &renderer,
		const char *name,
		GLint v0,
		GLint v1,
		GLint v2)
{
	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	glUniform3iARB(uniform_location, v0, v1, v2);

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform3i(
		GLRenderer &renderer,
		const char *name,
		const GLint *value,
		unsigned int count)
{
	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	glUniform3ivARB(uniform_location, count, value);

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform3d(
		GLRenderer &renderer,
		const char *name,
		GLdouble v0,
		GLdouble v1,
		GLdouble v2)
{
// In case old 'glew.h' (since extension added relatively recently).
// Also it seems some glew headers define 'GL_ARB_gpu_shader_fp64' but not 'glUniform...' API functions.
#if defined(GL_ARB_gpu_shader_fp64) && defined(glUniform3d)
	// We should only get here if the 'GL_ARB_gpu_shader_fp64' extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			renderer.get_capabilities().shader.gl_ARB_gpu_shader_fp64,
			GPLATES_ASSERTION_SOURCE);

	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	glUniform3d(uniform_location, v0, v1, v2);
#else
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
#endif

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform3d(
		GLRenderer &renderer,
		const char *name,
		const GLdouble *value,
		unsigned int count)
{
// In case old 'glew.h' (since extension added relatively recently).
// Also it seems some glew headers define 'GL_ARB_gpu_shader_fp64' but not 'glUniform...' API functions.
#if defined(GL_ARB_gpu_shader_fp64) && defined(glUniform3dv)
	// We should only get here if the 'GL_ARB_gpu_shader_fp64' extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			renderer.get_capabilities().shader.gl_ARB_gpu_shader_fp64,
			GPLATES_ASSERTION_SOURCE);

	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	glUniform3dv(uniform_location, count, value);
#else
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
#endif

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform3ui(
		GLRenderer &renderer,
		const char *name,
		GLuint v0,
		GLuint v1,
		GLuint v2)
{
	const GLCapabilities &capabilities = renderer.get_capabilities();

#ifdef GL_EXT_gpu_shader4 // In case old 'glew.h' (since extension added relatively recently).
	// We should only get here if the 'GL_EXT_gpu_shader4' extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			capabilities.shader.gl_EXT_gpu_shader4,
			GPLATES_ASSERTION_SOURCE);

	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	glUniform3uiEXT(uniform_location, v0, v1, v2);
#else
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
#endif

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform3ui(
		GLRenderer &renderer,
		const char *name,
		const GLuint *value,
		unsigned int count)
{
	const GLCapabilities &capabilities = renderer.get_capabilities();

#ifdef GL_EXT_gpu_shader4 // In case old 'glew.h' (since extension added relatively recently).
	// We should only get here if the 'GL_EXT_gpu_shader4' extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			capabilities.shader.gl_EXT_gpu_shader4,
			GPLATES_ASSERTION_SOURCE);

	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	glUniform3uivEXT(uniform_location, count, value);
#else
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
#endif

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform4f(
		GLRenderer &renderer,
		const char *name,
		GLfloat v0,
		GLfloat v1,
		GLfloat v2,
		GLfloat v3)
{
	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	glUniform4fARB(uniform_location, v0, v1, v2, v3);

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform4f(
		GLRenderer &renderer,
		const char *name,
		const GLfloat *value,
		unsigned count)
{
	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	glUniform4fvARB(uniform_location, count, value);

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform4i(
		GLRenderer &renderer,
		const char *name,
		GLint v0,
		GLint v1,
		GLint v2,
		GLint v3)
{
	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	glUniform4iARB(uniform_location, v0, v1, v2, v3);

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform4i(
		GLRenderer &renderer,
		const char *name,
		const GLint *value,
		unsigned int count)
{
	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	glUniform4ivARB(uniform_location, count, value);

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform4d(
		GLRenderer &renderer,
		const char *name,
		GLdouble v0,
		GLdouble v1,
		GLdouble v2,
		GLdouble v3)
{
// In case old 'glew.h' (since extension added relatively recently).
// Also it seems some glew headers define 'GL_ARB_gpu_shader_fp64' but not 'glUniform...' API functions.
#if defined(GL_ARB_gpu_shader_fp64) && defined(glUniform4d)
	// We should only get here if the 'GL_ARB_gpu_shader_fp64' extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			renderer.get_capabilities().shader.gl_ARB_gpu_shader_fp64,
			GPLATES_ASSERTION_SOURCE);

	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	glUniform4d(uniform_location, v0, v1, v2, v3);
#else
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
#endif

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform4d(
		GLRenderer &renderer,
		const char *name,
		const GLdouble *value,
		unsigned int count)
{
// In case old 'glew.h' (since extension added relatively recently).
// Also it seems some glew headers define 'GL_ARB_gpu_shader_fp64' but not 'glUniform...' API functions.
#if defined(GL_ARB_gpu_shader_fp64) && defined(glUniform4dv)
	// We should only get here if the 'GL_ARB_gpu_shader_fp64' extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			renderer.get_capabilities().shader.gl_ARB_gpu_shader_fp64,
			GPLATES_ASSERTION_SOURCE);

	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	glUniform4dv(uniform_location, count, value);
#else
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
#endif

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform4ui(
		GLRenderer &renderer,
		const char *name,
		GLuint v0,
		GLuint v1,
		GLuint v2,
		GLuint v3)
{
	const GLCapabilities &capabilities = renderer.get_capabilities();

#ifdef GL_EXT_gpu_shader4 // In case old 'glew.h' (since extension added relatively recently).
	// We should only get here if the 'GL_EXT_gpu_shader4' extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			capabilities.shader.gl_EXT_gpu_shader4,
			GPLATES_ASSERTION_SOURCE);

	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	glUniform4uiEXT(uniform_location, v0, v1, v2, v3);
#else
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
#endif

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform4ui(
		GLRenderer &renderer,
		const char *name,
		const GLuint *value,
		unsigned int count)
{
	const GLCapabilities &capabilities = renderer.get_capabilities();

#ifdef GL_EXT_gpu_shader4 // In case old 'glew.h' (since extension added relatively recently).
	// We should only get here if the 'GL_EXT_gpu_shader4' extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			capabilities.shader.gl_EXT_gpu_shader4,
			GPLATES_ASSERTION_SOURCE);

	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	glUniform4uivEXT(uniform_location, count, value);
#else
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
#endif

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform_matrix2x2f(
		GLRenderer &renderer,
		const char *name,
		const GLfloat *value,
		unsigned int count,
		GLboolean transpose)
{
	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	glUniformMatrix2fvARB(uniform_location, count, transpose, value);

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform_matrix2x2d(
		GLRenderer &renderer,
		const char *name,
		const GLdouble *value,
		unsigned int count,
		GLboolean transpose)
{
// In case old 'glew.h' (since extension added relatively recently).
// Also it seems some glew headers define 'GL_ARB_gpu_shader_fp64' but not 'glUniform...' API functions.
#if defined(GL_ARB_gpu_shader_fp64) && defined(glUniformMatrix2dv)
	// We should only get here if the 'GL_ARB_gpu_shader_fp64' extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			renderer.get_capabilities().shader.gl_ARB_gpu_shader_fp64,
			GPLATES_ASSERTION_SOURCE);

	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	glUniformMatrix2dv(uniform_location, count, transpose, value);
#else
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
#endif

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform_matrix3x3f(
		GLRenderer &renderer,
		const char *name,
		const GLfloat *value,
		unsigned int count,
		GLboolean transpose)
{
	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	glUniformMatrix3fvARB(uniform_location, count, transpose, value);

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform_matrix3x3d(
		GLRenderer &renderer,
		const char *name,
		const GLdouble *value,
		unsigned int count,
		GLboolean transpose)
{
// In case old 'glew.h' (since extension added relatively recently).
// Also it seems some glew headers define 'GL_ARB_gpu_shader_fp64' but not 'glUniform...' API functions.
#if defined(GL_ARB_gpu_shader_fp64) && defined(glUniformMatrix3dv)
	// We should only get here if the 'GL_ARB_gpu_shader_fp64' extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			renderer.get_capabilities().shader.gl_ARB_gpu_shader_fp64,
			GPLATES_ASSERTION_SOURCE);

	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	glUniformMatrix3dv(uniform_location, count, transpose, value);
#else
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
#endif

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform_matrix4x4f(
		GLRenderer &renderer,
		const char *name,
		const GLfloat *value,
		unsigned int count,
		GLboolean transpose)
{
	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	glUniformMatrix4fvARB(uniform_location, count, transpose, value);

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform_matrix4x4d(
		GLRenderer &renderer,
		const char *name,
		const GLdouble *value,
		unsigned int count,
		GLboolean transpose)
{
// In case old 'glew.h' (since extension added relatively recently).
// Also it seems some glew headers define 'GL_ARB_gpu_shader_fp64' but not 'glUniform...' API functions.
#if defined(GL_ARB_gpu_shader_fp64) && defined(glUniformMatrix4dv)
	// We should only get here if the 'GL_ARB_gpu_shader_fp64' extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			renderer.get_capabilities().shader.gl_ARB_gpu_shader_fp64,
			GPLATES_ASSERTION_SOURCE);

	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	glUniformMatrix4dv(uniform_location, count, transpose, value);
#else
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
#endif

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform_matrix4x4f(
		GLRenderer &renderer,
		const char *name,
		const GLMatrix &matrix)
{
	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	const GLdouble *const double_matrix = matrix.get_matrix();

	// Copy the matrix GLdouble elements into GLfloat elements.
	GLfloat float_matrix[16];
	for (int n = 0; n < 16; ++n)
	{
		float_matrix[n] = double_matrix[n];
	}

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	// Note that the matrix is in column-major format.
	glUniformMatrix4fvARB(uniform_location, 1, GL_FALSE/*transpose*/, float_matrix);

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform_matrix4x4d(
		GLRenderer &renderer,
		const char *name,
		const GLMatrix &matrix)
{
// In case old 'glew.h' (since extension added relatively recently).
// Also it seems some glew headers define 'GL_ARB_gpu_shader_fp64' but not 'glUniform...' API functions.
#if defined(GL_ARB_gpu_shader_fp64) && defined(glUniformMatrix4dv)
	// We should only get here if the 'GL_ARB_gpu_shader_fp64' extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			renderer.get_capabilities().shader.gl_ARB_gpu_shader_fp64,
			GPLATES_ASSERTION_SOURCE);

	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	const GLdouble *const double_matrix = matrix.get_matrix();

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	// Note that the matrix is in column-major format.
	glUniformMatrix4dv(uniform_location, 1, GL_FALSE/*transpose*/, double_matrix);
#else
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
#endif

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform_matrix4x4f(
		GLRenderer &renderer,
		const char *name,
		const std::vector<GLMatrix> &matrices)
{
	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

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

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	glUniformMatrix4fvARB(uniform_location, matrices.size(), GL_FALSE/*transpose*/, float_matrices.get());

	return true;
}


bool
GPlatesOpenGL::GLProgramObject::gl_uniform_matrix4x4d(
		GLRenderer &renderer,
		const char *name,
		const std::vector<GLMatrix> &matrices)
{
// In case old 'glew.h' (since extension added relatively recently).
// Also it seems some glew headers define 'GL_ARB_gpu_shader_fp64' but not 'glUniform...' API functions.
#if defined(GL_ARB_gpu_shader_fp64) && defined(glUniformMatrix4dv)
	// We should only get here if the 'GL_ARB_gpu_shader_fp64' extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			renderer.get_capabilities().shader.gl_ARB_gpu_shader_fp64,
			GPLATES_ASSERTION_SOURCE);

	// Bind this program object glUniform applies to it.
	// Revert our program binding on return so we don't affect changes made by clients.
	// This also makes sure the renderer applies the bind to OpenGL before we call OpenGL directly.
	GLRenderer::BindProgramObjectAndApply save_restore_bind(renderer, shared_from_this());

	// Iterate over the matrices and copy to an array of doubles.
	boost::scoped_array<GLdouble> double_matrices(new GLdouble[16 * matrices.size()]);
	for (unsigned int m = 0; m < matrices.size(); ++m)
	{
		// Copy the current matrix into the array.
		std::memcpy(double_matrices.get() + 16 * m, matrices[m].get_matrix(), 16 * sizeof(GLdouble));
	}

	const GLint uniform_location = get_uniform_location(name);
	if (uniform_location < 0)
	{
		return false;
	}

	glUniformMatrix4dv(uniform_location, matrices.size(), GL_FALSE/*transpose*/, double_matrices.get());
#else
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
#endif

	return true;
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

		if (uniform_location < 0)
		{
			qWarning()
					<< "Attempted to set shader program uniform variable '"
					<< uniform_name
					<< "' that (1) does not exist, "
					<< "(2) is not actively used in the linked shader program or "
					<< "(3) is a reserved name.";
		}

		// Override the dummy index (location) with the correct one (or -1 for not-found).
		uniform_insert_result.first->second = uniform_location;
	}

	return uniform_insert_result.first->second;
}


void
GPlatesOpenGL::GLProgramObject::output_info_log()
{
	std::set<QString> shader_filenames;

	// Get a list of unique shader code segments filenames for all shader objects linked.
	shader_object_seq_type::const_iterator shader_objects_iter = d_shader_objects.begin();
	shader_object_seq_type::const_iterator shader_objects_end = d_shader_objects.end();
	for ( ; shader_objects_iter != shader_objects_end; ++shader_objects_iter)
	{
		const GLShaderObject &shader_object = **shader_objects_iter;

		// Get the file source code segments of the current shader.
		const std::vector<GLShaderObject::FileCodeSegment> file_code_segments =
				shader_object.get_file_code_segments();

		for (unsigned int n = 0; n < file_code_segments.size(); ++n)
		{
			shader_filenames.insert(file_code_segments[n].filename);
		}
	}

	// Log the shader info log.

	const resource_handle_type program_resource_handle = get_program_resource_handle();

	// Determine the length of the info log message.
	GLint info_log_length;
	glGetObjectParameterivARB(program_resource_handle, GL_OBJECT_INFO_LOG_LENGTH_ARB, &info_log_length);

	// Allocate and read the info log message.
	boost::scoped_array<GLcharARB> info_log(new GLcharARB[info_log_length]);
	glGetInfoLogARB(program_resource_handle, info_log_length, NULL, info_log.get());
	// ...the returned string is null-terminated.

	// If some of the shader code segments came from files then print that information since
	// it's useful to help locate which compiled shader files were linked.
	if (!shader_filenames.empty())
	{
		qDebug() << " The following compiled OpenGL file shader source code segments were linked: ";

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
		qDebug() << " (all compiled OpenGL shader source code consisted of string literals)";
	}

	qDebug() << endl << info_log.get() << endl;
}
