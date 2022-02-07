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

#ifndef GPLATES_OPENGL_GLSHADERPROGRAMUTILS_H
#define GPLATES_OPENGL_GLSHADERPROGRAMUTILS_H

#include <boost/optional.hpp>

#include "GLProgramObject.h"
#include "GLShaderObject.h"
#include "GLShaderSource.h"


namespace GPlatesOpenGL
{
	class GLRenderer;

	namespace GLShaderProgramUtils
	{
		/**
		 * Shader program parameters required to be set before a geometry shader can be linked.
		 *
		 * For example, @a gl_max_vertices_out corresponds to GL_GEOMETRY_VERTICES_OUT and
		 * is the maximum number of vertices the geometry shader will output. This must be set
		 * to a non-zero (ie, non-default) value on some platforms (MacOS) *before* linking.
		 * Another constraint includes setting the geometry input type such that it matches the
		 * input array size declared in the geometry shader GLSL code.
		 */
		struct GeometryShaderProgramParameters
		{
			explicit
			GeometryShaderProgramParameters(
					GLint gl_max_vertices_out_,
					GLint gl_geometry_input_type_ = GL_TRIANGLES,
					GLint gl_geometry_output_type_ = GL_TRIANGLE_STRIP);

			GLint gl_max_vertices_out; // GL_GEOMETRY_VERTICES_OUT
			GLint gl_geometry_input_type; // GL_GEOMETRY_INPUT_TYPE
			GLint gl_geometry_output_type; // GL_GEOMETRY_OUTPUT_TYPE_EXT
		};


		/**
		 * The filename of the shader source file (Qt resource) containing shader utilities.
		 */
		const QString UTILS_SHADER_SOURCE_FILE_NAME = ":/opengl/utils.glsl";


		/**
		 * Compiles the specified fragment shader source into a shader object.
		 *
		 * Returns boost::none if:
		 *   1) Fragment shaders are not supported on the runtime system, or
		 *   2) Shader source compilation failed.
		 */
		boost::optional<GLShaderObject::shared_ptr_type>
		compile_fragment_shader(
				GLRenderer &renderer,
				const GLShaderSource &fragment_shader_source);


		/**
		 * Compiles the specified vertex shader source into a shader object.
		 *
		 * Returns boost::none if:
		 *   1) Vertex shaders are not supported on the runtime system, or
		 *   2) Shader source compilation failed.
		 */
		boost::optional<GLShaderObject::shared_ptr_type>
		compile_vertex_shader(
				GLRenderer &renderer,
				const GLShaderSource &vertex_shader_source);


		/**
		 * Compiles the specified geometry shader source into a shader object.
		 *
		 * Returns boost::none if:
		 *   1) Geometry shaders are not supported on the runtime system, or
		 *   2) Shader source compilation failed.
		 */
		boost::optional<GLShaderObject::shared_ptr_type>
		compile_geometry_shader(
				GLRenderer &renderer,
				const GLShaderSource &geometry_shader_source);


		/**
		 * Links the specified fragment shader into a program object.
		 *
		 * Returns boost::none if:
		 *   1) Shader program link failed.
		 */
		boost::optional<GLProgramObject::shared_ptr_type>
		link_fragment_program(
				GLRenderer &renderer,
				const GLShaderObject::shared_ptr_to_const_type &fragment_shader);


		/**
		 * Links the specified vertex/fragment shader into a program object.
		 *
		 * Returns boost::none if:
		 *   1) Shader program link failed.
		 */
		boost::optional<GLProgramObject::shared_ptr_type>
		link_vertex_fragment_program(
				GLRenderer &renderer,
				const GLShaderObject::shared_ptr_to_const_type &vertex_shader,
				const GLShaderObject::shared_ptr_to_const_type &fragment_shader);


		/**
		 * Links the specified vertex/geometry/fragment shader into a program object.
		 *
		 * @a geometry_shader_program_parameters are program parameters for the geometry shader
		 * that must be set to appropriate values on some platforms (MacOS) *before* linking.
		 *
		 * Returns boost::none if:
		 *   1) Shader program link failed.
		 */
		boost::optional<GLProgramObject::shared_ptr_type>
		link_vertex_geometry_fragment_program(
				GLRenderer &renderer,
				const GLShaderObject::shared_ptr_to_const_type &vertex_shader,
				const GLShaderObject::shared_ptr_to_const_type &geometry_shader,
				const GLShaderObject::shared_ptr_to_const_type &fragment_shader,
				const GeometryShaderProgramParameters &geometry_shader_program_parameters);


		/**
		 * Compiles the specified fragment shader source and links into a program object.
		 *
		 * Returns boost::none if:
		 *   1) Appropriate shaders are not supported on the runtime system, or
		 *   2) Shader source compilation failed, or
		 *   3) Shader program link failed.
		 */
		boost::optional<GLProgramObject::shared_ptr_type>
		compile_and_link_fragment_program(
				GLRenderer &renderer,
				const GLShaderSource &fragment_shader_source);


		/**
		 * Compiles the specified vertex/fragment shader source and links into a program object.
		 *
		 * Returns boost::none if:
		 *   1) Appropriate shaders are not supported on the runtime system, or
		 *   2) Shader source compilation failed, or
		 *   3) Shader program link failed.
		 */
		boost::optional<GLProgramObject::shared_ptr_type>
		compile_and_link_vertex_fragment_program(
				GLRenderer &renderer,
				const GLShaderSource &vertex_shader_source,
				const GLShaderSource &fragment_shader_source);


		/**
		 * Compiles the specified vertex/geometry/fragment shader source and links into a program object.
		 *
		 * @a geometry_shader_program_parameters are program parameters for the geometry shader
		 * that must be set to appropriate values on some platforms (MacOS) *before* linking.
		 *
		 * Returns boost::none if:
		 *   1) Appropriate shaders are not supported on the runtime system, or
		 *   2) Shader source compilation failed, or
		 *   3) Shader program link failed.
		 */
		boost::optional<GLProgramObject::shared_ptr_type>
		compile_and_link_vertex_geometry_fragment_program(
				GLRenderer &renderer,
				const GLShaderSource &vertex_shader_source,
				const GLShaderSource &geometry_shader_source,
				const GLShaderSource &fragment_shader_source,
				const GeometryShaderProgramParameters &geometry_shader_program_parameters);
	}
}

#endif // GPLATES_OPENGL_GLSHADERPROGRAMUTILS_H
