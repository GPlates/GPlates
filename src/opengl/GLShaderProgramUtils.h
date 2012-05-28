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

#include <vector>
#include <boost/optional.hpp>

#include "GLProgramObject.h"
#include "GLShaderObject.h"


namespace GPlatesOpenGL
{
	class GLRenderer;

	namespace GLShaderProgramUtils
	{
		/**
		 * A convenience class to handle shader source code segments.
		 *
		 * One or more shader source code segments can be grouped together before they are compiled.
		 */
		class ShaderSource
		{
		public:
			//! Default constructor contains no shader source.
			explicit
			ShaderSource(
					GLShaderObject::ShaderVersion shader_version = GLShaderObject::DEFAULT_SHADER_VERSION) :
				d_shader_version(shader_version)
			{  }

			//! Implicit converting constructor when only a single shader source is required.
			ShaderSource(
					const char *shader_source,
					GLShaderObject::ShaderVersion shader_version = GLShaderObject::DEFAULT_SHADER_VERSION) :
				d_shader_version(shader_version),
				d_shader_source(1, shader_source)
			{  }

			//! Adds a shader source code segment.
			void
			add_shader_source(
					const char *shader_source)
			{
				d_shader_source.push_back(shader_source);
			}


			//! Returns all shader source code segments.
			const std::vector<const char *> &
			get_shader_source() const
			{
				return d_shader_source;
			}

			//! Returns the shader version.
			GLShaderObject::ShaderVersion
			get_shader_version() const
			{
				return d_shader_version;
			}

		private:
			GLShaderObject::ShaderVersion d_shader_version;
			std::vector<const char *> d_shader_source;
		};


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
				const ShaderSource &fragment_shader_source);


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
				const ShaderSource &vertex_shader_source);


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
				const ShaderSource &geometry_shader_source);


		/**
		 * Links the specified fragment shader into a program object.
		 *
		 * Returns boost::none if:
		 *   1) Shader program link failed.
		 */
		boost::optional<GLProgramObject::shared_ptr_type>
		link_fragment_program(
				GLRenderer &renderer,
				const GLShaderObject &fragment_shader);


		/**
		 * Links the specified vertex/fragment shader into a program object.
		 *
		 * Returns boost::none if:
		 *   1) Shader program link failed.
		 */
		boost::optional<GLProgramObject::shared_ptr_type>
		link_vertex_fragment_program(
				GLRenderer &renderer,
				const GLShaderObject &vertex_shader,
				const GLShaderObject &fragment_shader);


		/**
		 * Links the specified vertex/geometry/fragment shader into a program object.
		 *
		 * Returns boost::none if:
		 *   1) Shader program link failed.
		 */
		boost::optional<GLProgramObject::shared_ptr_type>
		link_vertex_geometry_fragment_program(
				GLRenderer &renderer,
				const GLShaderObject &vertex_shader,
				const GLShaderObject &geometry_shader,
				const GLShaderObject &fragment_shader);


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
				const ShaderSource &fragment_shader_source);


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
				const ShaderSource &vertex_shader_source,
				const ShaderSource &fragment_shader_source);


		/**
		 * Compiles the specified vertex/geometry/fragment shader source and links into a program object.
		 *
		 * Returns boost::none if:
		 *   1) Appropriate shaders are not supported on the runtime system, or
		 *   2) Shader source compilation failed, or
		 *   3) Shader program link failed.
		 */
		boost::optional<GLProgramObject::shared_ptr_type>
		compile_and_link_vertex_geometry_fragment_program(
				GLRenderer &renderer,
				const ShaderSource &vertex_shader_source,
				const ShaderSource &geometry_shader_source,
				const ShaderSource &fragment_shader_source);


		/**
		 * Shader source code to bilinearly interpolate a *non-mipmapped*,
		 * *non-anisotropically filtered* 2D texture.
		 *
		 * The first overload of 'bilinearly_interpolate' returns the interpolated texture result
		 * while the second overload returns the four sampled texels and the interpolation coefficients.
		 *
		 * 'tex_dimensions' should contain the following (xyzw) components:
		 *    x: texture width,
		 *    y: texture height,
		 *    z: inverse texture width,
		 *    w: inverse texture height.
		 *
		 * This is useful for floating-point textures because bilinear filtering is not supported
		 * in earlier hardware.
		 */
		const char *const BILINEAR_FILTER_SHADER_SOURCE =
			"void\n"
			"bilinearly_interpolate(\n"
			"		sampler2D tex_sampler,\n"
			"		vec2 tex_coords,\n"
			"		vec4 tex_dimensions,\n"
			"		out vec4 tex11,\n"
			"		out vec4 tex21,\n"
			"		out vec4 tex12,\n"
			"		out vec4 tex22,\n"
			"		out vec2 interp)\n"
			"{\n"

			"	// Multiply tex coords by texture dimensions to convert to unnormalised form.\n"
			"	vec2 uv = tex_coords * tex_dimensions.xy;\n"

			"	vec4 st;\n"

			"	// The lower-left texel centre.\n"
			"	st.xy = floor(uv - 0.5) + 0.5;\n"
			"	// The upper-right texel centre.\n"
			"	st.zw = st.xy + 1;\n"

			"	// The bilinear interpolation coefficients.\n"
			"	interp = uv - st.xy;\n"

			"	// Multiply tex coords by inverse texture dimensions to return to normalised form.\n"
			"	st *= tex_dimensions.zwzw;\n"

			"	// The first texture access starts a new indirection phase since it accesses a temporary\n"
			"	// written in the current phase (see issue 24 in GL_ARB_fragment_program spec).\n"
			"	tex11 = texture2D(tex_sampler, st.xy);\n"
			"	tex21 = texture2D(tex_sampler, st.zy);\n"
			"	tex12 = texture2D(tex_sampler, st.xw);\n"
			"	tex22 = texture2D(tex_sampler, st.zw);\n"

			"}\n"

			"vec4\n"
			"bilinearly_interpolate(\n"
			"		sampler2D tex_sampler,\n"
			"		vec2 tex_coords,\n"
			"		vec4 tex_dimensions)\n"
			"{\n"

			"	// The 2x2 texture sample to interpolate.\n"
			"	vec4 tex11;\n"
			"	vec4 tex21;\n"
			"	vec4 tex12;\n"
			"	vec4 tex22;\n"

			"	// The bilinear interpolation coefficients.\n"
			"	vec2 interp;\n"

			"	// Call the other overload of 'bilinearly_interpolate()'.\n"
			"	bilinearly_interpolate(\n"
			"		tex_sampler, tex_coords, tex_dimensions, tex11, tex21, tex12, tex22, interp);\n"

			"	// Bilinearly interpolate the four texels.\n"
			"	return mix(mix(tex11, tex21, interp.x), mix(tex12, tex22, interp.x), interp.y);\n"

			"}\n";


		/**
		 * Shader source code to rotate an (x,y,z) vector by a quaternion.
		 *
		 * Normally it is faster to convert a quaternion to a matrix and then use that one matrix
		 * to transform many vectors. However this usually means storing the rotation matrix
		 * as shader constants which reduces batching when the matrix needs to be changed.
		 * In some situations batching can be improved by sending the rotation matrix as vertex
		 * attribute data (can then send a lot more geometries, each with different matrices,
		 * in one batch because not limited by shader constant space limit) - and using
		 * quaternions means 4 floats instead of 9 floats (ie, a single 4-component vertex attribute).
		 * The only issue is a quaternion needs to be sent with *each* vertex of each geometry and
		 * the shader code to do the transform is more expensive but in some situations (involving
		 * large numbers of geometries) the much-improved batching is more than worth it.
		 * The reason batching is important is each batch has a noticeable CPU overhead
		 * (in OpenGL and the driver, etc) and it's easy to become CPU-limited.
		 *
		 * The following shader code is based on http://code.google.com/p/kri/wiki/Quaternions
		 */
		const char *const ROTATE_VECTOR_BY_QUATERNION_SHADER_SOURCE =
			"vec3\n"
			"rotate_vector_by_quaternion(\n"
			"		vec4 q,\n"
			"		vec3 v)\n"
			"{\n"

			"   return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);\n"

			"}\n";
	}
}

#endif // GPLATES_OPENGL_GLSHADERPROGRAMUTILS_H
