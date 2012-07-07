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
#include <vector>
#include <boost/enable_shared_from_this.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <opengl/OpenGL.h>
#include <QByteArray>

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
		 * GLSL shader versions.
		 *
		 * This is used instead of specifying "#version 120" for example.
		 * This is because the "#version" directive must come before any non-commented source code.
		 * But this becomes difficult with multiple source code segments because usually the
		 * "#version" directive is placed in the segment defining the 'main()' shader function and
		 * this usually is the last segment (because it uses other shader segments and hence they
		 * must be defined first).
		 * So the solution used here is this class will create a "#version" shader segment and add
		 * it as the first shader segment which means it should not be defined in any supplied
		 * shader segments.
		 */
		enum ShaderVersion
		{
			GLSL_1_1, // Corresponds to OpenGL version 2.0
			GLSL_1_2, // Corresponds to OpenGL version 2.1
			GLSL_1_3, // Corresponds to OpenGL version 3.0
			GLSL_1_4, // Corresponds to OpenGL version 3.1
			GLSL_1_5, // Corresponds to OpenGL version 3.2
			GLSL_3_3, // Corresponds to OpenGL version 3.3
			GLSL_4_0, // Corresponds to OpenGL version 4.0
			GLSL_4_1, // Corresponds to OpenGL version 4.1
			GLSL_4_2,  // Corresponds to OpenGL version 4.2

			NUM_SHADER_VERSIONS // This must be last.
		};

		/**
		 * The default shader version to compile.
		 *
		 * Version 1.2 is chosen instead of 1.1 since most hardware supporting OpenGL 2.0 also supports OpenGL 2.1.
		 */
		static const ShaderVersion DEFAULT_SHADER_VERSION = GLSL_1_2;


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
				const std::vector<const char *> &source_strings,
				ShaderVersion shader_version = DEFAULT_SHADER_VERSION);

		/**
		 * Performs same function as the glShaderSource OpenGL function.
		 *
		 * Each string in @a source_strings is an (ordered) subsection of the shader source code.
		 */
		void
		gl_shader_source(
				GLRenderer &renderer,
				const std::vector<QByteArray> &source_strings,
				ShaderVersion shader_version = DEFAULT_SHADER_VERSION);

		/**
		 * Performs same function as the glShaderSource OpenGL function.
		 *
		 * A convenience overload of @a gl_shader_source when only one source string is provided.
		 */
		void
		gl_shader_source(
				GLRenderer &renderer,
				const char *source_string,
				ShaderVersion shader_version = DEFAULT_SHADER_VERSION);

		/**
		 * Performs same function as the glShaderSource OpenGL function.
		 *
		 * A convenience overload of @a gl_shader_source when only one source string is provided.
		 */
		void
		gl_shader_source(
				GLRenderer &renderer,
				const QByteArray &source_string,
				ShaderVersion shader_version = DEFAULT_SHADER_VERSION);


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

		//! Shader source version strings.
		static const char *SHADER_VERSION_STRINGS[NUM_SHADER_VERSIONS];


		//! Constructor.
		GLShaderObject(
				GLRenderer &renderer,
				GLenum shader_type);
	};
}

#endif // GPLATES_OPENGL_GLSHADEROBJECT_H
