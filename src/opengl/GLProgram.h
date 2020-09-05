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

#ifndef GPLATES_OPENGL_GLPROGRAM_H
#define GPLATES_OPENGL_GLPROGRAM_H

#include <map>
#include <memory> // For std::unique_ptr
#include <set>
#include <string>
#include <vector>
#include <boost/enable_shared_from_this.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <opengl/OpenGL1.h>

#include "GLObject.h"
#include "GLObjectResource.h"
#include "GLObjectResourceManager.h"
#include "GLShader.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


namespace GPlatesOpenGL
{
	class GL;

	/**
	* Wrapper around an OpenGL program object.
	 */
	class GLProgram :
			public GLObject,
			public boost::enable_shared_from_this<GLProgram>
	{
	public:
		//
		// Note that the reason boost::shared_ptr is used instead of non_null_intrusive_ptr
		// is so these objects can be used with GPlatesUtils::ObjectCache.
		//

		//! A convenience typedef for a shared pointer to a @a GLProgram.
		typedef boost::shared_ptr<GLProgram> shared_ptr_type;
		typedef boost::shared_ptr<const GLProgram> shared_ptr_to_const_type;

		//! A convenience typedef for a weak pointer to a @a GLProgram.
		typedef boost::weak_ptr<GLProgram> weak_ptr_type;
		typedef boost::weak_ptr<const GLProgram> weak_ptr_to_const_type;


		/**
		 * Creates a shared pointer to a @a GLProgram object.
		 */
		static
		shared_ptr_type
		create(
				GL &gl)
		{
			return shared_ptr_type(new GLProgram(gl));
		}

		/**
		 * Same as @a create but returns a std::unique_ptr - to guarantee only one owner.
		 *
		 * See comment in @a create for details on @a shader_type.
		 */
		static
		std::unique_ptr<GLProgram>
		create_as_unique_ptr(
				GL &gl)
		{
			return std::unique_ptr<GLProgram>(new GLProgram(gl));
		}


		/**
		 * Performs same function as glAttachShader.
		 *
		 * A shared reference to @a shader is kept internally while it is attached.
		 *
		 * Note that it is an OpenGL error to attach the same shader if it is already attached.
		 */
		void
		attach_shader(
				const GLShader::shared_ptr_to_const_type &shader);


		/**
		 * Performs same function as glDetachShader.
		 */
		void
		detach_shader(
				const GLShader::shared_ptr_to_const_type &shader);


		/**
		 * Performs same function as glLinkProgram (and also retrieves the GL_LINK_STATUS result).
		 *
		 * @throws OpenGLException if the link was unsuccessful and logs the link diagnostic message.
		 * Note that if successfully linked then nothing is logged.
		 *
		 * Note that, as dictated by OpenGL, if you re-link a program object you will have to
		 * load the uniform variables again (because the link initialises them to zero).
		 *
		 * NOTE: Use this method instead of calling glLinkProgram directly since this method will also clear
		 *       the internal mapping of uniform names to uniform locations (used by @a get_uniform_location).
		 *       If you use glLinkProgram directly then do not use @a get_uniform_location or @a is_active_uniform.
		 */
		void
		link_program();


		/**
		 * Performs same function as glValidateProgram (and also retrieves the GL_VALIDATE_STATUS result).
		 *
		 * @throws OpenGLException if validation was unsuccessful and logs the validate diagnostic message.
		 * Note that if successfully validated then nothing is logged.
		 *
		 * NOTE: This method is meant for use during development only.
		 */
		void
		validate_program();


		//////////////////////////////
		// SETING UNIFORM VARIABLES //
		//////////////////////////////


		/**
		 * Returns true if the specified uniform name corresponds to an active uniform variable
		 * (in default uniform block) in the most recent linking of this program (see @a link_program).
		 *
		 * Returns false for any of the following:
		 *  (1) variable does not exist,
		 *  (2) variable is not actively used in the linked program or
		 *  (3) variable is a reserved name.
		 *
		 * Note that OpenGL will generate an error if this is called before @a link_program is first called.
		 */
		bool
		is_active_uniform(
				const char *uniform_name) const
		{
			return get_uniform_location(uniform_name) >= 0;
		}

		/**
		 * Get the uniform location index (in default uniform block) of the specified uniform variable name.
		 *
		 * Returns -1 if @a uniform_name is not an active uniform.
		 * Note: Calling glUniform* with a location of -1 is *not* an error according to OpenGL 3.3 core specification
		 *       (instead the glUniform* call is silently ignored).
		 *
		 * You can use the returned location with a native glUniform* call. Such as:
		 *
		 *   gl.UseProgram(program);
		 *   glUniform4f(program->get_uniform_location('colour'), red, green, blue, alpha);
		 *
		 * Internally this calls glGetUniformLocation and caches its results. If this program is
		 * subsequently re-linked (by another call to @a link_program) then the cache is cleared.
		 * Caching is the only difference compared to using glGetUniformLocation directly as in:
		 *
		 *   gl.UseProgram(program);
		 *   glUniform4f(glGetUniformLocation(program->get_resource_handle(), 'colour'), red, green, blue, alpha);
		 *
		 * Note that OpenGL will generate an error if this is called before @a link_program is first called.
		 */
		GLint
		get_uniform_location(
				const char *uniform_name) const;


		/**
		 * Returns the program resource handle.
		 */
		GLuint
		get_resource_handle() const;


	public:  // For use by the OpenGL framework...

		/**
		 * Policy class to allocate and deallocate OpenGL buffer objects.
		 */
		class Allocator
		{
		public:
			GLuint
			allocate(
					const GLCapabilities &capabilities);

			void
			deallocate(
					GLuint);
		};

		//! Typedef for a resource.
		typedef GLObjectResource<GLuint, Allocator> resource_type;

		//! Typedef for a resource manager.
		typedef GLObjectResourceManager<GLuint, Allocator> resource_manager_type;

	private:

		//! Typedef for a sequence of shader objects.
		typedef std::set<GLShader::shared_ptr_to_const_type> shader_seq_type;

		//! Typedef for a map of uniform variable names to locations.
		typedef std::map<std::string, GLint> uniform_location_map_type;


		resource_type::non_null_ptr_to_const_type d_resource;

		shader_seq_type d_shaders;

		mutable uniform_location_map_type d_uniform_locations;


		//! Constructor.
		GLProgram(
				GL &gl);

		void
		output_info_log();
	};
}

#endif // GPLATES_OPENGL_GLPROGRAM_H