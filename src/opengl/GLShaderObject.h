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

#ifndef GPLATES_OPENGL_GLSHADEROBJECT_H
#define GPLATES_OPENGL_GLSHADEROBJECT_H

#include <memory> // For std::unique_ptr
#include <vector>
#include <boost/enable_shared_from_this.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <opengl/OpenGL.h>

#include "GLObject.h"
#include "GLObjectResource.h"
#include "GLObjectResourceManager.h"
#include "GLShaderSource.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


namespace GPlatesOpenGL
{
	class GLRenderer;

	/**
	 * A shader object.
	 *
	 * Note that the 'GL_ARB_shader_objects' extension must be supported and also, for the
	 * three currently supported shader types, the following extensions must also be supported:
	 *  - GL_ARB_vertex_shader (for GL_VERTEX_SHADER_ARB)... this is also core in OpenGL 2.0,
	 *  - GL_ARB_fragment_shader (for GL_FRAGMENT_SHADER_ARB)... this is also core in OpenGL 2.0,
	 *  - GL_EXT_geometry_shader4 (for GL_GEOMETRY_SHADER_EXT)... this is also core in OpenGL 3.2.
	 */
	class GLShaderObject :
			public GLObject,
			public boost::enable_shared_from_this<GLShaderObject>
	{
	public:
		//
		// Note that the reason boost::shared_ptr is used instead of non_null_intrusive_ptr
		// is so these objects can be used with GPlatesUtils::ObjectCache.
		//

		//! A convenience typedef for a shared pointer to a @a GLShaderObject.
		typedef boost::shared_ptr<GLShaderObject> shared_ptr_type;
		typedef boost::shared_ptr<const GLShaderObject> shared_ptr_to_const_type;

		//! A convenience typedef for a weak pointer to a @a GLShaderObject.
		typedef boost::weak_ptr<GLShaderObject> weak_ptr_type;
		typedef boost::weak_ptr<const GLShaderObject> weak_ptr_to_const_type;


		//! Typedef for a resource handle.
		typedef GLuint resource_handle_type;

		/**
		 * Policy class to allocate and deallocate OpenGL shader objects.
		 */
		class Allocator
		{
		public:
			explicit
			Allocator(
					GLenum shader_type) :
				d_shader_type(shader_type)
			{  }

			resource_handle_type
			allocate(
					const GLCapabilities &capabilities);

			void
			deallocate(
					resource_handle_type texture);

		private:
			GLenum d_shader_type;
		};

		//! Typedef for a resource allocator.
		typedef Allocator allocator_type;

		//! Typedef for a resource.
		typedef GLObjectResource<resource_handle_type, Allocator> resource_type;

		//! Typedef for a resource manager.
		typedef GLObjectResourceManager<resource_handle_type, Allocator> resource_manager_type;


		/**
		 * Represents information of one (of potentially many) shader code segments.
		 *
		 * This is primarily used to locate the source of compile errors.
		 */
		struct SourceCodeSegment
		{
			explicit
			SourceCodeSegment(
					const GLShaderSource::CodeSegment &source_code_segment);

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
		 * Returns true if @a shader_type is supported on the runtime system.
		 *
		 * Currently @a shader_type can be GL_VERTEX_SHADER_ARB, GL_FRAGMENT_SHADER_ARB or GL_GEOMETRY_SHADER_EXT.
		 */
		static
		bool
		is_supported(
				GLRenderer &renderer,
				GLenum shader_type);


		/**
		 * Creates a shared pointer to a @a GLShaderObject object.
		 *
		 * @a shader_type can be GL_VERTEX_SHADER_ARB, GL_FRAGMENT_SHADER_ARB or GL_GEOMETRY_SHADER_EXT.
		 *
		 * Note that @a is_supported must returned true for @a shader_type.
		 */
		static
		shared_ptr_type
		create(
				GLRenderer &renderer,
				GLenum shader_type)
		{
			return shared_ptr_type(new GLShaderObject(renderer, shader_type));
		}

		/**
		 * Same as @a create but returns a std::unique_ptr - to guarantee only one owner.
		 *
		 * See comment in @a create for details on @a shader_type.
		 */
		static
		std::unique_ptr<GLShaderObject>
		create_as_unique_ptr(
				GLRenderer &renderer,
				GLenum shader_type)
		{
			return std::unique_ptr<GLShaderObject>(new GLShaderObject(renderer, shader_type));
		}


		/**
		 * Performs same function as the glShaderSource OpenGL function.
		 *
		 * Each string, or code segment, in @a shader_source is an (ordered) subsection of the shader source code.
		 */
		void
		gl_shader_source(
				GLRenderer &renderer,
				const GLShaderSource &shader_source);


		/**
		 * Performs same function as the glCompileShader OpenGL function (and also retrieves
		 * the GL_COMPILE_STATUS result).
		 *
		 * Returns false if the compilation was unsuccessful and logs the compile diagnostic message as a warning.
		 * Note that if successfully compiled then nothing is logged.
		 */
		bool
		gl_compile_shader(
				GLRenderer &renderer);


		/**
		 * Returns the shader source set with @a gl_shader_source, or boost::none if it hasn't been called.
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
		 *
		 * NOTE: This is a lower-level function used to help implement the OpenGL framework.
		 */
		resource_handle_type
		get_shader_resource_handle() const
		{
			return d_resource->get_resource_handle();
		}

	private:

		resource_type::non_null_ptr_to_const_type d_resource;

		//! Source code segments set by @a gl_shader_source.
		boost::optional< std::vector<SourceCodeSegment> > d_source_code_segments;


		//! Constructor.
		GLShaderObject(
				GLRenderer &renderer,
				GLenum shader_type);

		void
		output_info_log();
	};
}

#endif // GPLATES_OPENGL_GLSHADEROBJECT_H
