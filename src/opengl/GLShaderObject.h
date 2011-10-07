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

#include <memory> // For std::auto_ptr
#include <string>
#include <vector>
#include <boost/enable_shared_from_this.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <opengl/OpenGL.h>

#include "GLObject.h"
#include "GLObjectResource.h"
#include "GLObjectResourceManager.h"

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
	 *  - GL_ARB_geometry_shader4 (for GL_GEOMETRY_SHADER_ARB)... this is also core in OpenGL 3.2.
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
			allocate();

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
		 * Returns true if @a shader_type is supported on the runtime system.
		 *
		 * Currently @a shader_type can be GL_VERTEX_SHADER_ARB, GL_FRAGMENT_SHADER_ARB or GL_GEOMETRY_SHADER_ARB.
		 */
		static
		bool
		is_supported(
				GLRenderer &renderer,
				GLenum shader_type);


		/**
		 * Creates a shared pointer to a @a GLShaderObject object.
		 *
		 * @a shader_type can be GL_VERTEX_SHADER_ARB, GL_FRAGMENT_SHADER_ARB or GL_GEOMETRY_SHADER_ARB.
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
		 * Same as @a create but returns a std::auto_ptr - to guarantee only one owner.
		 *
		 * See comment in @a create for details on @a shader_type.
		 */
		static
		std::auto_ptr<GLShaderObject>
		create_as_auto_ptr(
				GLRenderer &renderer,
				GLenum shader_type)
		{
			return std::auto_ptr<GLShaderObject>(new GLShaderObject(renderer, shader_type));
		}


		/**
		 * Performs same function as the glShaderSource OpenGL function.
		 *
		 * Each string in @a source_strings is an (ordered) subsection of the shader source code.
		 */
		void
		gl_shader_source(
				GLRenderer &renderer,
				const std::vector<const char *> &source_strings);

		/**
		 * Performs same function as the glShaderSource OpenGL function.
		 *
		 * Each string in @a source_strings is an (ordered) subsection of the shader source code.
		 */
		void
		gl_shader_source(
				GLRenderer &renderer,
				const std::vector<std::string> &source_strings);

		/**
		 * Performs same function as the glShaderSource OpenGL function.
		 *
		 * A convenience overload of @a gl_shader_source when only one source string is provided.
		 */
		void
		gl_shader_source(
				GLRenderer &renderer,
				const char *source_string);

		/**
		 * Performs same function as the glShaderSource OpenGL function.
		 *
		 * A convenience overload of @a gl_shader_source when only one source string is provided.
		 */
		void
		gl_shader_source(
				GLRenderer &renderer,
				const std::string &source_string);


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


		//! Constructor.
		GLShaderObject(
				GLRenderer &renderer,
				GLenum shader_type);
	};
}

#endif // GPLATES_OPENGL_GLSHADEROBJECT_H
