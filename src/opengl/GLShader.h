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

#ifndef GPLATES_OPENGL_GLSHADER_H
#define GPLATES_OPENGL_GLSHADER_H

#include <memory> // For std::unique_ptr
#include <vector>
#include <boost/enable_shared_from_this.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <opengl/OpenGL1.h>

#include "GLObject.h"
#include "GLObjectResource.h"
#include "GLObjectResourceManager.h"
#include "GLShaderSource.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


namespace GPlatesOpenGL
{
	class GL;
	class OpenGLFunctions;

	/**
	 * Wrapper around an OpenGL shader object (vertex, geometry or fragment).
	 */
	class GLShader :
			public GLObject,
			public boost::enable_shared_from_this<GLShader>
	{
	public:
		//
		// Note that the reason boost::shared_ptr is used instead of non_null_intrusive_ptr
		// is so these objects can be used with GPlatesUtils::ObjectCache.
		//

		//! A convenience typedef for a shared pointer to a @a GLShader.
		typedef boost::shared_ptr<GLShader> shared_ptr_type;
		typedef boost::shared_ptr<const GLShader> shared_ptr_to_const_type;

		//! A convenience typedef for a weak pointer to a @a GLShader.
		typedef boost::weak_ptr<GLShader> weak_ptr_type;
		typedef boost::weak_ptr<const GLShader> weak_ptr_to_const_type;


		/**
		 * Represents information of one (of potentially many) shader code segments.
		 *
		 * This is primarily used to locate the source of compile errors.
		 */
		struct SourceCodeSegment
		{
			explicit
			SourceCodeSegment(
					const GLShaderSource::CodeSegment &source_code_segment) :
				num_lines(source_code_segment.num_lines),
				source_file_name(source_code_segment.source_file_name)
			{
				// We avoid copying the source code to save a little memory.
			}

			unsigned int num_lines;

			//! Source filename is valid if code segment loaded from a file, otherwise was loaded from a string.
			boost::optional<QString> source_file_name;
		};

		/**
		 * Locates a *file* code segment within the concatenated source code.
		 */
		struct FileCodeSegment
		{
			FileCodeSegment(
					unsigned int first_line_number_,
					unsigned int last_line_number_,
					QString filename_) :
				first_line_number(first_line_number_),
				last_line_number(last_line_number_),
				filename(filename_)
			{  }

			unsigned int first_line_number;
			unsigned int last_line_number;
			QString filename;
		};


		/**
		 * Creates a shared pointer to a @a GLShader object.
		 *
		 * @a shader_type can be GL_VERTEX_SHADER, GL_FRAGMENT_SHADER or GL_GEOMETRY_SHADER.
		 */
		static
		shared_ptr_type
		create(
				GL &gl,
				GLenum shader_type)
		{
			return shared_ptr_type(new GLShader(gl, shader_type));
		}

		/**
		 * Same as @a create but returns a std::unique_ptr - to guarantee only one owner.
		 *
		 * See comment in @a create for details on @a shader_type.
		 */
		static
		std::unique_ptr<GLShader>
		create_unique(
				GL &gl,
				GLenum shader_type)
		{
			return std::unique_ptr<GLShader>(new GLShader(gl, shader_type));
		}


		/**
		 * Performs same function as glShaderSource.
		 *
		 * Each string, or code segment, in @a shader_source is an (ordered) subsection of the shader source code.
		 */
		void
		shader_source(
				GL &gl,
				const GLShaderSource &shader_source);


		/**
		 * Performs same function as glCompileShader (and also retrieves GL_COMPILE_STATUS result).
		 *
		 * @throws OpenGLException if the compilation was unsuccessful and logs the compile diagnostic message.
		 * Note that if successfully compiled then nothing is logged.
		 */
		void
		compile_shader(
				GL &gl);


		/**
		 * Returns the shader source set with @a shader_source, or boost::none if it hasn't been called.
		 *
		 * All shader source code segments of the shader source are returned (in compiled order).
		 */
		const boost::optional< std::vector<SourceCodeSegment> > &
		get_source_code_segments() const
		{
			return d_source_code_segments;
		}


		/**
		 * Similar to @a get_source_code_segments except only returns code segment that came
		 * from files and returns the line number range of the code segment within the concatenated
		 * shader source code.
		 */
		std::vector<FileCodeSegment>
		get_file_code_segments() const;


		/**
		 * Returns the shader resource handle.
		 */
		GLuint
		get_resource_handle() const;


	public:  // For use by the OpenGL framework...

		/**
		 * Policy class to allocate and deallocate OpenGL shader objects.
		 */
		class Allocator
		{
		public:
			GLuint
			allocate(
					OpenGLFunctions &opengl_functions,
					const GLCapabilities &capabilities,
					GLenum shader_type);

			void
			deallocate(
					OpenGLFunctions &opengl_functions,
					GLuint);
		};

		//! Typedef for a resource.
		typedef GLObjectResource<GLuint, Allocator> resource_type;

		//! Typedef for a resource manager.
		typedef GLObjectResourceManager<GLuint, Allocator> resource_manager_type;

	private:

		resource_type::non_null_ptr_to_const_type d_resource;

		//! Source code segments set by @a shader_source.
		boost::optional< std::vector<SourceCodeSegment> > d_source_code_segments;


		//! Constructor.
		GLShader(
				GL &gl,
				GLenum shader_type);

		void
		output_info_log(
				GL &gl);
	};
}

#endif // GPLATES_OPENGL_GLSHADER_H
