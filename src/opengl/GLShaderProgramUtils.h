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

#include <string>
#include <vector>
#include <boost/optional.hpp>
#include <QByteArray>
#include <QFile>

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

			/**
			 * Creates a @a ShaderSource object when only a single shader source, from a file, is required.
			 */
			static
			ShaderSource
			create_shader_source_from_file(
					const QString& shader_source_file_name,
					GLShaderObject::ShaderVersion shader_version = GLShaderObject::DEFAULT_SHADER_VERSION)
			{
				return ShaderSource(
						get_shader_source_from_file(shader_source_file_name),
						shader_version);
			}


			//! Default constructor contains no shader source.
			explicit
			ShaderSource(
					GLShaderObject::ShaderVersion shader_version = GLShaderObject::DEFAULT_SHADER_VERSION) :
				d_shader_version(shader_version)
			{  }

			/**
			 * Implicit converting constructor when only a single shader source is required.
			 *
			 * NOTE: The char array pointed by @a shader_source must remain in existence after this call.
			 */
			ShaderSource(
					const char *shader_source,
					GLShaderObject::ShaderVersion shader_version = GLShaderObject::DEFAULT_SHADER_VERSION);

			//! Implicit converting constructor when only a single shader source is required.
			ShaderSource(
					const QByteArray &shader_source,
					GLShaderObject::ShaderVersion shader_version = GLShaderObject::DEFAULT_SHADER_VERSION);

			/**
			 * Adds a shader source code segment.
			 *
			 * NOTE: The char array pointed by @a shader_source must remain in existence after this call.
			 */
			void
			add_shader_source(
					const char *shader_source)
			{
				add_shader_source(QByteArray::fromRawData(shader_source, qstrlen(shader_source)));
			}

			//! Adds a shader source code segment.
			void
			add_shader_source(
					const QByteArray &shader_source)
			{
				d_shader_source.push_back(shader_source);
			}

			//! Adds a shader source code segment from a file.
			void
			add_shader_source_from_file(
					const QString& shader_source_file_name)
			{
				add_shader_source(get_shader_source_from_file(shader_source_file_name));
			}


			//! Returns all shader source code segments.
			const std::vector<QByteArray> &
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
			std::vector<QByteArray> d_shader_source;

			//! Extracts shader source code from a file.
			static
			QByteArray
			get_shader_source_from_file(
					const QString& shader_source_file_name);
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
	}
}

#endif // GPLATES_OPENGL_GLSHADERPROGRAMUTILS_H
