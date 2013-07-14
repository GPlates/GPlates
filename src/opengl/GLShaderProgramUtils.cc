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

#include "GLShaderProgramUtils.h"

#include "GLProgramObject.h"
#include "GLRenderer.h"


GPlatesOpenGL::GLShaderProgramUtils::GeometryShaderProgramParameters::GeometryShaderProgramParameters(
		GLint gl_max_vertices_out_,
		GLint gl_geometry_input_type_,
		GLint gl_geometry_output_type_) :
	gl_max_vertices_out(gl_max_vertices_out_),
	gl_geometry_input_type(gl_geometry_input_type_),
	gl_geometry_output_type(gl_geometry_output_type_)
{
}


boost::optional<GPlatesOpenGL::GLShaderObject::shared_ptr_type>
GPlatesOpenGL::GLShaderProgramUtils::compile_fragment_shader(
		GLRenderer &renderer,
		const GLShaderSource &fragment_shader_source)
{
	// Check for support first.
	if (!GLShaderObject::is_supported(renderer, GL_FRAGMENT_SHADER_ARB))
	{
		return boost::none;
	}

	// Create and compile the fragment shader source.
	GLShaderObject::shared_ptr_type fragment_shader = GLShaderObject::create(renderer, GL_FRAGMENT_SHADER_ARB);
	fragment_shader->gl_shader_source(renderer, fragment_shader_source);
	if (!fragment_shader->gl_compile_shader(renderer))
	{
		return boost::none;
	}

	return fragment_shader;
}


boost::optional<GPlatesOpenGL::GLShaderObject::shared_ptr_type>
GPlatesOpenGL::GLShaderProgramUtils::compile_vertex_shader(
		GLRenderer &renderer,
		const GLShaderSource &vertex_shader_source)
{
	// Check for support first.
	if (!GLShaderObject::is_supported(renderer, GL_VERTEX_SHADER_ARB))
	{
		return boost::none;
	}

	// Create and compile the vertex shader source.
	GLShaderObject::shared_ptr_type vertex_shader = GLShaderObject::create(renderer, GL_VERTEX_SHADER_ARB);
	vertex_shader->gl_shader_source(renderer, vertex_shader_source);
	if (!vertex_shader->gl_compile_shader(renderer))
	{
		return boost::none;
	}

	return vertex_shader;
}


boost::optional<GPlatesOpenGL::GLShaderObject::shared_ptr_type>
GPlatesOpenGL::GLShaderProgramUtils::compile_geometry_shader(
		GLRenderer &renderer,
		const GLShaderSource &geometry_shader_source)
{
#ifdef GL_EXT_geometry_shader4 // In case old 'glew.h' (since extension added relatively recently in OpenGL 3.2).

	// Check for support first.
	if (!GLShaderObject::is_supported(renderer, GL_GEOMETRY_SHADER_EXT))
	{
		return boost::none;
	}

	// Create and compile the geometry shader source.
	GLShaderObject::shared_ptr_type geometry_shader = GLShaderObject::create(renderer, GL_GEOMETRY_SHADER_EXT);
	geometry_shader->gl_shader_source(renderer, geometry_shader_source);
	if (!geometry_shader->gl_compile_shader(renderer))
	{
		return boost::none;
	}

	return geometry_shader;

#else

	// Geometry shaders not supported on runtime system.
	return boost::none;

#endif
}


boost::optional<GPlatesOpenGL::GLProgramObject::shared_ptr_type>
GPlatesOpenGL::GLShaderProgramUtils::link_fragment_program(
		GLRenderer &renderer,
		const GLShaderObject::shared_ptr_to_const_type &fragment_shader)
{
	// Check for support first.
	if (!GLProgramObject::is_supported(renderer))
	{
		return boost::none;
	}

	// Create the shader program, attach the fragment shader and link the program.
	GLProgramObject::shared_ptr_type shader_program = GLProgramObject::create(renderer);
	shader_program->gl_attach_shader(renderer, fragment_shader);
	if (!shader_program->gl_link_program(renderer))
	{
		return boost::none;
	}

	return shader_program;
}


boost::optional<GPlatesOpenGL::GLProgramObject::shared_ptr_type>
GPlatesOpenGL::GLShaderProgramUtils::link_vertex_fragment_program(
		GLRenderer &renderer,
		const GLShaderObject::shared_ptr_to_const_type &vertex_shader,
		const GLShaderObject::shared_ptr_to_const_type &fragment_shader)
{
	// Check for support first.
	if (!GLProgramObject::is_supported(renderer))
	{
		return boost::none;
	}

	// Create the shader program, attach the vertex/fragment shaders and link the program.
	GLProgramObject::shared_ptr_type shader_program = GLProgramObject::create(renderer);
	shader_program->gl_attach_shader(renderer, vertex_shader);
	shader_program->gl_attach_shader(renderer, fragment_shader);
	if (!shader_program->gl_link_program(renderer))
	{
		return boost::none;
	}

	return shader_program;
}


boost::optional<GPlatesOpenGL::GLProgramObject::shared_ptr_type>
GPlatesOpenGL::GLShaderProgramUtils::link_vertex_geometry_fragment_program(
		GLRenderer &renderer,
		const GLShaderObject::shared_ptr_to_const_type &vertex_shader,
		const GLShaderObject::shared_ptr_to_const_type &geometry_shader,
		const GLShaderObject::shared_ptr_to_const_type &fragment_shader,
		const GeometryShaderProgramParameters &geometry_shader_program_parameters)
{
	// Check for support first.
	if (!GLProgramObject::is_supported(renderer))
	{
		return boost::none;
	}

	// Create the shader program, attach the vertex/geometry/fragment shaders.
	GLProgramObject::shared_ptr_type shader_program = GLProgramObject::create(renderer);
	shader_program->gl_attach_shader(renderer, vertex_shader);
	shader_program->gl_attach_shader(renderer, geometry_shader);
	shader_program->gl_attach_shader(renderer, fragment_shader);

	//
	// Set the geometry shader program parameters.
	//

	// Specify the input primitive types to the geometry shader.
	shader_program->gl_program_parameteri(
			renderer,
			GL_GEOMETRY_INPUT_TYPE_EXT,
			geometry_shader_program_parameters.gl_geometry_input_type);

	// Specify the output primitive types of the geometry shader.
	shader_program->gl_program_parameteri(
			renderer,
			GL_GEOMETRY_OUTPUT_TYPE_EXT,
			geometry_shader_program_parameters.gl_geometry_output_type);

	// Specify the maximum number of vertices output by the geometry shader.
	shader_program->gl_program_parameteri(
			renderer,
			GL_GEOMETRY_VERTICES_OUT_EXT,
			geometry_shader_program_parameters.gl_max_vertices_out);

	// Link the program.
	if (!shader_program->gl_link_program(renderer))
	{
		return boost::none;
	}

	return shader_program;
}


boost::optional<GPlatesOpenGL::GLProgramObject::shared_ptr_type>
GPlatesOpenGL::GLShaderProgramUtils::compile_and_link_fragment_program(
		GLRenderer &renderer,
		const GLShaderSource &fragment_shader_source)
{
	// First create and compile the fragment shader source.
	boost::optional<GLShaderObject::shared_ptr_type> fragment_shader =
			compile_fragment_shader(renderer, fragment_shader_source);
	if (!fragment_shader)
	{
		return boost::none;
	}

	return link_fragment_program(renderer, fragment_shader.get());
}


boost::optional<GPlatesOpenGL::GLProgramObject::shared_ptr_type>
GPlatesOpenGL::GLShaderProgramUtils::compile_and_link_vertex_fragment_program(
		GLRenderer &renderer,
		const GLShaderSource &vertex_shader_source,
		const GLShaderSource &fragment_shader_source)
{
	// Create and compile the vertex shader source.
	boost::optional<GLShaderObject::shared_ptr_type> vertex_shader =
			compile_vertex_shader(renderer, vertex_shader_source);
	if (!vertex_shader)
	{
		return boost::none;
	}

	// First create and compile the fragment shader source.
	boost::optional<GLShaderObject::shared_ptr_type> fragment_shader =
			compile_fragment_shader(renderer, fragment_shader_source);
	if (!fragment_shader)
	{
		return boost::none;
	}

	return link_vertex_fragment_program(renderer, vertex_shader.get(), fragment_shader.get());
}


boost::optional<GPlatesOpenGL::GLProgramObject::shared_ptr_type>
GPlatesOpenGL::GLShaderProgramUtils::compile_and_link_vertex_geometry_fragment_program(
		GLRenderer &renderer,
		const GLShaderSource &vertex_shader_source,
		const GLShaderSource &geometry_shader_source,
		const GLShaderSource &fragment_shader_source,
		const GeometryShaderProgramParameters &geometry_shader_program_parameters)
{
	// Create and compile the vertex shader source.
	boost::optional<GLShaderObject::shared_ptr_type> vertex_shader =
			compile_vertex_shader(renderer, vertex_shader_source);
	if (!vertex_shader)
	{
		return boost::none;
	}

	// Create and compile the geometry shader source.
	boost::optional<GLShaderObject::shared_ptr_type> geometry_shader =
			compile_geometry_shader(renderer, geometry_shader_source);
	if (!geometry_shader)
	{
		return boost::none;
	}

	// First create and compile the fragment shader source.
	boost::optional<GLShaderObject::shared_ptr_type> fragment_shader =
			compile_fragment_shader(renderer, fragment_shader_source);
	if (!fragment_shader)
	{
		return boost::none;
	}

	return link_vertex_geometry_fragment_program(
			renderer,
			vertex_shader.get(),
			geometry_shader.get(),
			fragment_shader.get(),
			geometry_shader_program_parameters);
}
