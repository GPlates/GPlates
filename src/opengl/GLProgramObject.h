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

#include "GLMatrix.h"
#include "GLObject.h"
#include "GLObjectResource.h"
#include "GLObjectResourceManager.h"
#include "GLShader.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "gui/Colour.h"

#include "maths/UnitQuaternion3D.h"
#include "maths/UnitVector3D.h"
#include "maths/Vector3D.h"


namespace GPlatesOpenGL
{
	class GL;

	/**
	* Wrapper around an OpenGL program object.
	 */
	class GLProgramObject :
			public GLObject,
			public boost::enable_shared_from_this<GLProgramObject>
	{
	public:
		//
		// Note that the reason boost::shared_ptr is used instead of non_null_intrusive_ptr
		// is so these objects can be used with GPlatesUtils::ObjectCache.
		//

		//! A convenience typedef for a shared pointer to a @a GLProgramObject.
		typedef boost::shared_ptr<GLProgramObject> shared_ptr_type;
		typedef boost::shared_ptr<const GLProgramObject> shared_ptr_to_const_type;

		//! A convenience typedef for a weak pointer to a @a GLProgramObject.
		typedef boost::weak_ptr<GLProgramObject> weak_ptr_type;
		typedef boost::weak_ptr<const GLProgramObject> weak_ptr_to_const_type;


		/**
		 * Creates a shared pointer to a @a GLProgramObject object.
		 */
		static
		shared_ptr_type
		create(
				GL &gl)
		{
			return shared_ptr_type(new GLProgramObject(gl));
		}

		/**
		 * Same as @a create but returns a std::unique_ptr - to guarantee only one owner.
		 *
		 * See comment in @a create for details on @a shader_type.
		 */
		static
		std::unique_ptr<GLProgramObject>
		create_as_unique_ptr(
				GL &gl)
		{
			return std::unique_ptr<GLProgramObject>(new GLProgramObject(gl));
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
		 * Returns false if the link was unsuccessful and logs the link diagnostic message as a warning.
		 * Note that if successfully linked then nothing is logged.
		 *
		 * Note that, as dictated by OpenGL, if you re-link a program object you will have to
		 * load the uniform variables again (because the link initialises them to zero).
		 *
		 * NOTE: Use this method instead of calling glLinkProgram directly since this method will also clear the
		 *       internal mapping of uniform names to uniform locations (used by @a get_uniform_location and @a uniform).
		 *       If you do use glLinkProgram directly then do not use @a get_uniform_location or @a uniform.
		 */
		bool
		link_program();


		/**
		 * Performs same function as glValidateProgram (and also retrieves the GL_VALIDATE_STATUS result).
		 *
		 * Returns success or failure for validation and logs the validate diagnostic message as a debug message.
		 *
		 * NOTE: This method is meant for use during development only.
		 */
		bool
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
		 */
		bool
		is_active_uniform(
				const char *uniform_name) const;

		/**
		 * Get the uniform location index (in default uniform block) of the specified uniform variable name.
		 *
		 * Returns -1 if @a uniform_name is not an active uniform.
		 * Note: Calling glUniform* with a location of -1 is not an error according to OpenGL 3.3 core specification
		 *       (instead the glUniform* call is silently ignored).
		 *
		 * You can use the returned location with a native glUniform* call. Such as:
		 *
		 *   glUniform4f(program->get_uniform_location('colour'), red, green, blue, alpha);
		 *
		 * Internally this calls glGetUniformLocation and caches its results (until/if re-linked with another call to @a link_program).
		 * Caching is the only difference compared to using glGetUniformLocation directly as in:
		 *
		 *   glUniform4f(glGetUniformLocation(program->get_resource_handle(), 'colour'), red, green, blue, alpha);
		 */
		GLint
		get_uniform_location(
				const char *uniform_name) const;


		//
		// Convenience wrappers around glUniform* for some common types (like GLMatrix, UnitVector3D, Colour, etc).
		//

		/**
		 * Writes @a UnitVector3D as a vec3 (or a vec4 with specified @a w) to uniform in default block.
		 *
		 * Returns false if @a name is not an active uniform (and hence silently not written).
		 */
		bool
		uniform(
				const char *name,
				const GPlatesMaths::UnitVector3D &value,
				boost::optional<GLfloat> w = boost::none);

		/**
		 * Writes @a Vector3D as a vec3 (or a vec4 with specified @a w) uniform in default block.
		 *
		 * Returns false if @a name is not an active uniform (and hence silently not written).
		 */
		bool
		uniform(
				const char *name,
				const GPlatesMaths::Vector3D &value,
				boost::optional<GLfloat> w = boost::none);

		/**
		 * Writes @a UnitQuaternion as a vec4 uniform in default block.
		 *
		 * Returns false if @a name is not an active uniform (and hence silently not written).
		 */
		bool
		uniform(
				const char *name,
				const GPlatesMaths::UnitQuaternion3D &unit_quat);

		/**
		 * Writes @a Colour as a vec4 uniform in default block.
		 *
		 * Returns false if @a name is not an active uniform (and hence silently not written).
		 */
		bool
		uniform(
				const char *name,
				const GPlatesGui::Colour &colour);

		/**
		 * Writes a single @a GLMatrix as a mat4 uniform in default block.
		 *
		 * Returns false if @a name is not an active uniform (and hence silently not written).
		 */
		bool
		uniform(
				const char *name,
				const GLMatrix &matrix);


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
		GLProgramObject(
				GL &gl);

		void
		output_info_log();
	};
}

#endif // GPLATES_OPENGL_GLPROGRAM_H
