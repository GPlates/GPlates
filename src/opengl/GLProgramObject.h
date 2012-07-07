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

#ifndef GPLATES_OPENGL_GLPROGRAMOBJECT_H
#define GPLATES_OPENGL_GLPROGRAMOBJECT_H

#include <map>
#include <memory> // For std::auto_ptr
#include <string>
#include <vector>
#include <boost/enable_shared_from_this.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <opengl/OpenGL.h>

#include "GLMatrix.h"
#include "GLObject.h"
#include "GLObjectResource.h"
#include "GLObjectResourceManager.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "gui/Colour.h"

#include "maths/UnitQuaternion3D.h"
#include "maths/UnitVector3D.h"
#include "maths/Vector3D.h"


namespace GPlatesOpenGL
{
	class GLRenderer;
	class GLShaderObject;

	/**
	 * A shader program object.
	 *
	 * Note that the 'GL_ARB_shader_objects' and 'GL_ARB_vertex_shader' extensions must be supported.
	 *
	 * Also some methods (such as @a gl_uniform1ui and @a gl_uniform1d) require extra extensions:
	 *  - 'GL_EXT_gpu_shader4' for setting *unsigned* integer uniform variables, and
	 *  - 'GL_ARB_gpu_shader_fp64' for setting *double* uniform variables.
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


		//! Typedef for a resource handle.
		typedef GLuint resource_handle_type;

		/**
		 * Policy class to allocate and deallocate OpenGL shader objects.
		 */
		class Allocator
		{
		public:
			resource_handle_type
			allocate();

			void
			deallocate(
					resource_handle_type texture);
		};

		//! Typedef for a resource allocator.
		typedef Allocator allocator_type;

		//! Typedef for a resource.
		typedef GLObjectResource<resource_handle_type, Allocator> resource_type;

		//! Typedef for a resource manager.
		typedef GLObjectResourceManager<resource_handle_type, Allocator> resource_manager_type;


		/**
		 * Returns true if shader program objects are supported on the runtime system.
		 */
		static
		bool
		is_supported(
				GLRenderer &renderer);


		/**
		 * Creates a shared pointer to a @a GLProgramObject object.
		 */
		static
		shared_ptr_type
		create(
				GLRenderer &renderer)
		{
			return shared_ptr_type(new GLProgramObject(renderer));
		}

		/**
		 * Same as @a create but returns a std::auto_ptr - to guarantee only one owner.
		 *
		 * See comment in @a create for details on @a shader_type.
		 */
		static
		std::auto_ptr<GLProgramObject>
		create_as_auto_ptr(
				GLRenderer &renderer)
		{
			return std::auto_ptr<GLProgramObject>(new GLProgramObject(renderer));
		}


		/**
		 * Performs same function as the glAttachShader OpenGL function.
		 *
		 * Note that if @a shader is subsequently destroyed the OpenGL will only delete the underlying
		 * OpenGL shader object resource when it is no longer attached to any program objects.
		 * This class does not keep a shared reference to @a GLShaderObject.
		 *
		 * Note that it is an OpenGL error to attach the same shader if it is already attached.
		 */
		void
		gl_attach_shader(
				GLRenderer &renderer,
				const GLShaderObject &shader);


		/**
		 * Performs same function as the glDetachShader OpenGL function.
		 *
		 * Note that it is an OpenGL error to detach a shader that is not currently attached.
		 */
		void
		gl_detach_shader(
				GLRenderer &renderer,
				const GLShaderObject &shader);


		/**
		 * Performs same function as the glBindAttribLocation OpenGL function.
		 *
		 * Note that, as dictated by OpenGL, @a attribute_index must be in the half-closed range
		 * [0, GL_MAX_VERTEX_ATTRIBS_ARB).
		 * You can get GL_MAX_VERTEX_ATTRIBS_ARB from 'GLContext::get_parameters().shader.gl_max_vertex_attribs'.
		 *
		 * NOTE: As dictated by OpenGL, generic vertex attributes won't get bound to this program object
		 * until the next call to @a gl_link_program. So when you use a program it uses the bindings
		 * that were in effect at the last @a gl_link_program call.
		 *
		 * NOTE: You'll also need to explictly bind each *generic* attribute index in the vertex array
		 * (see @a GLVertexArray) in order for this program to access the vertex attribute data in
		 * the vertex array (buffers).
		 *
		 * On nVidia hardware the attribute indices are mapped to built-in vertex attributes so you
		 * cannot, for example, use 'glColorPointer' for colour and 'glVertexAttribPointer(3, ...)'
		 * for some other vertex attribute at the same time since they both map to the same attribute index.
		 * nVidia explains this:
		 * "GLSL attempts to eliminate aliasing of vertex attributes but this is integral to NVIDIA’s "
		 * "hardware approach and necessary for maintaining compatibility with existing OpenGL applications "
		 * "that NVIDIA customers rely on. NVIDIA’s GLSL implementation therefore does not allow built-in "
		 * "vertex attributes to collide with a generic vertex attribute that is assigned to a particular "
		 * "vertex attribute index with glBindAttribLocation. For example, you should not use gl_Normal "
		 * "(a built-in vertex attribute) and also use glBindAttribLocation to bind a generic vertex attribute "
		 * "named "whatever" to vertex attribute index 2 because gl_Normal aliases to index 2."
		 *
		 * The following summarises nVidia's vertex attribute aliasing behaviour:
		 *   gl_Vertex          0
		 *   gl_Normal          2
		 *   gl_Color           3
		 *   gl_SecondaryColor  4
		 *   gl_FogCoord        5
		 *   gl_MultiTexCoord0  8
		 *   gl_MultiTexCoord1  9
		 *   gl_MultiTexCoord2  10
		 *   gl_MultiTexCoord3  11
		 *   gl_MultiTexCoord4  12
		 *   gl_MultiTexCoord5  13
		 *   gl_MultiTexCoord6  14
		 *   gl_MultiTexCoord7  15
		 *
		 * NOTE: Ensure you use attribute index zero for one of your vertex attributes - it appears
		 * that some hardware will not work unless this is the case.
		 * This was discovered on a nVidia 7400M - probably it's expecting either 'glVertexPointer'
		 * which aliases to index zero or 'glVertexAttribPointer(0, ...)' which specifically uses index zero.
		 *
		 * Also, if you are using the fixed function vertex pipeline (ie, not using a vertex shader)
		 * then don't use 'glVertexAttribPointer(0, ...)' to set vertex data (for the fixed function
		 * pipeline) even though, on nVidia hardware, this maps to 'glVertexPointer'.
		 * This worked on nVidia hardware, but not other hardware, most likely due to the above aliasing.
		 */
		void
		gl_bind_attrib_location(
				const char *attribute_name,
				GLuint attribute_index);


		/**
		 * Performs same function as the glLinkProgram OpenGL function (and also retrieves
		 * the GL_LINK_STATUS result).
		 *
		 * Returns false if the link was unsuccessful and logs the link diagnostic message as a warning.
		 * Note that if successfully linked then nothing is logged.
		 *
		 * Note that, as dictated by OpenGL, if you re-link a program object you will have to
		 * load the uniform variables again (because the link initialises them to zero).
		 */
		bool
		gl_link_program(
				GLRenderer &renderer);


		/**
		 * Performs same function as the glValidateProgram OpenGL function (and also retrieves
		 * the GL_VALIDATE_STATUS result).
		 *
		 * Returns success or failure for validation.
		 * Also logs the validate diagnostic message as a debug message.
		 *
		 * NOTE: This method is meant for use during development only.
		 */
		bool
		gl_validate_program(
				GLRenderer &renderer);


		//////////////////////////////
		// SETING UNIFORM VARIABLES //
		//////////////////////////////
		//
		// NOTE: Only *active* uniform variables should be set with the following 'gl_uniform*()' functions.
		// Active variables are those declared in the shader source code that are actually used by the
		// currently linked program (this is determined at compile/link time by the shader compiler/linker).
		// The 'gl_uniform*()' functions return false if the uniform variable does not exist or
		// is not *active* or is a reserved name (a warning is also logged once per uniform name per link).
		//
		// NOTE: As dictated by OpenGL, when you (re)link a program object you will have to
		// load the uniform variables again (because the link initialises them to zero).
		//
		// NOTE: As dictated by OpenGL, the *type* (eg, GLfloat, GLint) and *size* (eg, 1,2,3,4) of the
		// uniform variable set with @a gl_uniform must match that declared in the shader source code.
		//
		// NOTE: The methods that set *unsigned* integer and *double* uniforms require extra extensions:
		//  - 'GL_EXT_gpu_shader4' for setting *unsigned* integer uniform variables, and
		//  - 'GL_ARB_gpu_shader_fp64' for setting *double* uniform variables.


		//! Performs same function as the glUniform1f OpenGL function - returns false if not active.
		bool
		gl_uniform1f(
				GLRenderer &renderer,
				const char *name,
				GLfloat v0);

		//! Performs same function as the glUniform1fv OpenGL function - returns false if not active.
		bool
		gl_uniform1f(
				GLRenderer &renderer,
				const char *name,
				const GLfloat *value,
				unsigned int count);

		//! Performs same function as the glUniform1i OpenGL function - returns false if not active.
		bool
		gl_uniform1i(
				GLRenderer &renderer,
				const char *name,
				GLint v0);

		//! Performs same function as the glUniform1iv OpenGL function - returns false if not active.
		bool
		gl_uniform1i(
				GLRenderer &renderer,
				const char *name,
				const GLint *value,
				unsigned int count);

		/**
		 * Performs same function as the glUniform1d OpenGL function - returns false if not active.
		 *
		 * NOTE: Requires 'GL_ARB_gpu_shader_fp64'.
		 */
		bool
		gl_uniform1d(
				GLRenderer &renderer,
				const char *name,
				GLdouble v0);

		/**
		 * Performs same function as the glUniform1dv OpenGL function - returns false if not active.
		 *
		 * NOTE: Requires 'GL_ARB_gpu_shader_fp64'.
		 */
		bool
		gl_uniform1d(
				GLRenderer &renderer,
				const char *name,
				const GLdouble *value,
				unsigned int count);

		/**
		 * Performs same function as the glUniform1ui OpenGL function - returns false if not active.
		 *
		 * NOTE: Requires 'GL_EXT_gpu_shader4'.
		 */
		bool
		gl_uniform1ui(
				GLRenderer &renderer,
				const char *name,
				GLuint v0);

		/**
		 * Performs same function as the glUniform1uiv OpenGL function - returns false if not active.
		 *
		 * NOTE: Requires 'GL_EXT_gpu_shader4'.
		 */
		bool
		gl_uniform1ui(
				GLRenderer &renderer,
				const char *name,
				const GLuint *value,
				unsigned int count);


		//! Performs same function as the glUniform2f OpenGL function - returns false if not active.
		bool
		gl_uniform2f(
				GLRenderer &renderer,
				const char *name,
				GLfloat v0,
				GLfloat v1);

		//! Performs same function as the glUniform2fv OpenGL function - returns false if not active.
		bool
		gl_uniform2f(
				GLRenderer &renderer,
				const char *name,
				const GLfloat *value,
				unsigned int count);

		//! Performs same function as the glUniform2i OpenGL function - returns false if not active.
		bool
		gl_uniform2i(
				GLRenderer &renderer,
				const char *name,
				GLint v0,
				GLint v1);

		//! Performs same function as the glUniform2iv OpenGL function - returns false if not active.
		bool
		gl_uniform2i(
				GLRenderer &renderer,
				const char *name,
				const GLint *value,
				unsigned int count);

		/**
		 * Performs same function as the glUniform2d OpenGL function - returns false if not active.
		 *
		 * NOTE: Requires 'GL_ARB_gpu_shader_fp64'.
		 */
		bool
		gl_uniform2d(
				GLRenderer &renderer,
				const char *name,
				GLdouble v0,
				GLdouble v1);

		/**
		 * Performs same function as the glUniform2dv OpenGL function - returns false if not active.
		 *
		 * NOTE: Requires 'GL_ARB_gpu_shader_fp64'.
		 */
		bool
		gl_uniform2d(
				GLRenderer &renderer,
				const char *name,
				const GLdouble *value,
				unsigned int count);

		/**
		 * Performs same function as the glUniform2ui OpenGL function - returns false if not active.
		 *
		 * NOTE: Requires 'GL_EXT_gpu_shader4'.
		 */
		bool
		gl_uniform2ui(
				GLRenderer &renderer,
				const char *name,
				GLuint v0,
				GLuint v1);

		/**
		 * Performs same function as the glUniform2uiv OpenGL function - returns false if not active.
		 *
		 * NOTE: Requires 'GL_EXT_gpu_shader4'.
		 */
		bool
		gl_uniform2ui(
				GLRenderer &renderer,
				const char *name,
				const GLuint *value,
				unsigned int count);


		//! Performs same function as the glUniform3f OpenGL function - returns false if not active.
		bool
		gl_uniform3f(
				GLRenderer &renderer,
				const char *name,
				GLfloat v0,
				GLfloat v1,
				GLfloat v2);

		//! Performs same function as the glUniform3fv OpenGL function - returns false if not active.
		bool
		gl_uniform3f(
				GLRenderer &renderer,
				const char *name,
				const GLfloat *value,
				unsigned int count);

		//! Performs same function as the glUniform3i OpenGL function - returns false if not active.
		bool
		gl_uniform3i(
				GLRenderer &renderer,
				const char *name,
				GLint v0,
				GLint v1,
				GLint v2);

		//! Performs same function as the glUniform3iv OpenGL function - returns false if not active.
		bool
		gl_uniform3i(
				GLRenderer &renderer,
				const char *name,
				const GLint *value,
				unsigned int count);

		/**
		 * Performs same function as the glUniform3d OpenGL function - returns false if not active.
		 *
		 * NOTE: Requires 'GL_ARB_gpu_shader_fp64'.
		 */
		bool
		gl_uniform3d(
				GLRenderer &renderer,
				const char *name,
				GLdouble v0,
				GLdouble v1,
				GLdouble v2);

		/**
		 * Performs same function as the glUniform3dv OpenGL function - returns false if not active.
		 *
		 * NOTE: Requires 'GL_ARB_gpu_shader_fp64'.
		 */
		bool
		gl_uniform3d(
				GLRenderer &renderer,
				const char *name,
				const GLdouble *value,
				unsigned int count);

		/**
		 * Performs same function as the glUniform3ui OpenGL function - returns false if not active.
		 *
		 * NOTE: Requires 'GL_EXT_gpu_shader4'.
		 */
		bool
		gl_uniform3ui(
				GLRenderer &renderer,
				const char *name,
				GLuint v0,
				GLuint v1,
				GLuint v2);

		/**
		 * Performs same function as the glUniform3uiv OpenGL function - returns false if not active.
		 *
		 * NOTE: Requires 'GL_EXT_gpu_shader4'.
		 */
		bool
		gl_uniform3ui(
				GLRenderer &renderer,
				const char *name,
				const GLuint *value,
				unsigned int count);

		//! Writes @a UnitVector3D as single-precision (x,y,z).
		bool
		gl_uniform3f(
				GLRenderer &renderer,
				const char *name,
				const GPlatesMaths::UnitVector3D &value)
		{
			return gl_uniform3f(renderer, name, value.x().dval(), value.y().dval(), value.z().dval());
		}

		/**
		 * Writes @a UnitVector3D as double-precision (x,y,z).
		 *
		 * NOTE: Requires 'GL_ARB_gpu_shader_fp64'.
		 */
		bool
		gl_uniform3d(
				GLRenderer &renderer,
				const char *name,
				const GPlatesMaths::UnitVector3D &value)
		{
			return gl_uniform3d(renderer, name, value.x().dval(), value.y().dval(), value.z().dval());
		}

		//! Writes @a Vector3D as (x,y,z).
		bool
		gl_uniform3f(
				GLRenderer &renderer,
				const char *name,
				const GPlatesMaths::Vector3D &value)
		{
			return gl_uniform3f(renderer, name, value.x().dval(), value.y().dval(), value.z().dval());
		}

		/**
		 * Writes @a Vector3D as double-precision (x,y,z).
		 *
		 * NOTE: Requires 'GL_ARB_gpu_shader_fp64'.
		 */
		bool
		gl_uniform3d(
				GLRenderer &renderer,
				const char *name,
				const GPlatesMaths::Vector3D &value)
		{
			return gl_uniform3d(renderer, name, value.x().dval(), value.y().dval(), value.z().dval());
		}


		//! Performs same function as the glUniform4f OpenGL function - returns false if not active.
		bool
		gl_uniform4f(
				GLRenderer &renderer,
				const char *name,
				GLfloat v0,
				GLfloat v1,
				GLfloat v2,
				GLfloat v3);

		//! Performs same function as the glUniform4fv OpenGL function - returns false if not active.
		bool
		gl_uniform4f(
				GLRenderer &renderer,
				const char *name,
				const GLfloat *value,
				unsigned int count);

		//! Performs same function as the glUniform4i OpenGL function - returns false if not active.
		bool
		gl_uniform4i(
				GLRenderer &renderer,
				const char *name,
				GLint v0,
				GLint v1,
				GLint v2,
				GLint v3);

		//! Performs same function as the glUniform4iv OpenGL function - returns false if not active.
		bool
		gl_uniform4i(
				GLRenderer &renderer,
				const char *name,
				const GLint *value,
				unsigned int count);

		/**
		 * Performs same function as the glUniform4d OpenGL function - returns false if not active.
		 *
		 * NOTE: Requires 'GL_ARB_gpu_shader_fp64'.
		 */
		bool
		gl_uniform4d(
				GLRenderer &renderer,
				const char *name,
				GLdouble v0,
				GLdouble v1,
				GLdouble v2,
				GLdouble v3);

		/**
		 * Performs same function as the glUniform4dv OpenGL function - returns false if not active.
		 *
		 * NOTE: Requires 'GL_ARB_gpu_shader_fp64'.
		 */
		bool
		gl_uniform4d(
				GLRenderer &renderer,
				const char *name,
				const GLdouble *value,
				unsigned int count);

		/**
		 * Performs same function as the glUniform4ui OpenGL function - returns false if not active.
		 *
		 * NOTE: Requires 'GL_EXT_gpu_shader4'.
		 */
		bool
		gl_uniform4ui(
				GLRenderer &renderer,
				const char *name,
				GLuint v0,
				GLuint v1,
				GLuint v2,
				GLuint v3);

		/**
		 * Performs same function as the glUniform4uiv OpenGL function - returns false if not active.
		 *
		 * NOTE: Requires 'GL_EXT_gpu_shader4'.
		 */
		bool
		gl_uniform4ui(
				GLRenderer &renderer,
				const char *name,
				const GLuint *value,
				unsigned int count);


		//! Writes @a UnitVector3D as single-precision (x,y,z,w).
		bool
		gl_uniform4f(
				GLRenderer &renderer,
				const char *name,
				const GPlatesMaths::UnitVector3D &value_xyz,
				GLfloat value_w = 1)
		{
			return gl_uniform4f(renderer, name, value_xyz.x().dval(), value_xyz.y().dval(), value_xyz.z().dval(), value_w);
		}

		/**
		 * Writes @a UnitVector3D as double-precision (x,y,z,w).
		 *
		 * NOTE: Requires 'GL_ARB_gpu_shader_fp64'.
		 */
		bool
		gl_uniform4d(
				GLRenderer &renderer,
				const char *name,
				const GPlatesMaths::UnitVector3D &value_xyz,
				GLdouble value_w = 1)
		{
			return gl_uniform4d(renderer, name, value_xyz.x().dval(), value_xyz.y().dval(), value_xyz.z().dval(), value_w);
		}

		//! Writes @a Vector3D as single-precision (x,y,z,w).
		bool
		gl_uniform4f(
				GLRenderer &renderer,
				const char *name,
				const GPlatesMaths::Vector3D &value_xyz,
				GLfloat value_w = 1)
		{
			return gl_uniform4f(renderer, name, value_xyz.x().dval(), value_xyz.y().dval(), value_xyz.z().dval(), value_w);
		}

		/**
		 * Writes @a Vector3D as double-precision (x,y,z,w).
		 *
		 * NOTE: Requires 'GL_ARB_gpu_shader_fp64'.
		 */
		bool
		gl_uniform4d(
				GLRenderer &renderer,
				const char *name,
				const GPlatesMaths::Vector3D &value_xyz,
				GLdouble value_w = 1)
		{
			return gl_uniform4d(renderer, name, value_xyz.x().dval(), value_xyz.y().dval(), value_xyz.z().dval(), value_w);
		}

		//! Writes @a UnitQuaternion as single-precision (x,y,z,w).
		bool
		gl_uniform4f(
				GLRenderer &renderer,
				const char *name,
				const GPlatesMaths::UnitQuaternion3D &unit_quat)
		{
			return gl_uniform4f(renderer, name, unit_quat.x().dval(), unit_quat.y().dval(), unit_quat.z().dval(), unit_quat.w().dval());
		}

		/**
		 * Writes @a UnitQuaternion as double-precision (x,y,z,w).
		 *
		 * NOTE: Requires 'GL_ARB_gpu_shader_fp64'.
		 */
		bool
		gl_uniform4d(
				GLRenderer &renderer,
				const char *name,
				const GPlatesMaths::UnitQuaternion3D &unit_quat)
		{
			return gl_uniform4d(renderer, name, unit_quat.x().dval(), unit_quat.y().dval(), unit_quat.z().dval(), unit_quat.w().dval());
		}

		//! Writes @a value as single-precision (r,g,b,a).
		bool
		gl_uniform4f(
				GLRenderer &renderer,
				const char *name,
				const GPlatesGui::Colour &colour)
		{
			return gl_uniform4f(renderer, name, colour, 1/*count*/);
		}


		/**
		 * Performs same function as the glUniformMatrix2fv OpenGL function - returns false if not active.
		 *
		 * NOTE: If @a transpose is false then matrix must be laid out in column-major format (ie, col0, col1).
		 */
		bool
		gl_uniform_matrix2x2f(
				GLRenderer &renderer,
				const char *name,
				const GLfloat *value,
				unsigned int count,
				GLboolean transpose);

		/**
		 * Performs same function as the glUniformMatrix2dv OpenGL function - returns false if not active.
		 *
		 * NOTE: If @a transpose is false then matrix must be laid out in column-major format (ie, col0, col1).
		 *
		 * NOTE: Requires 'GL_ARB_gpu_shader_fp64'.
		 */
		bool
		gl_uniform_matrix2x2d(
				GLRenderer &renderer,
				const char *name,
				const GLdouble *value,
				unsigned int count,
				GLboolean transpose);

		/**
		 * Performs same function as the glUniformMatrix3fv OpenGL function - returns false if not active.
		 *
		 * NOTE: If @a transpose is false then matrix must be laid out in column-major format (ie, col0, col1, col2).
		 */
		bool
		gl_uniform_matrix3x3f(
				GLRenderer &renderer,
				const char *name,
				const GLfloat *value,
				unsigned int count,
				GLboolean transpose);

		/**
		 * Performs same function as the glUniformMatrix3dv OpenGL function - returns false if not active.
		 *
		 * NOTE: If @a transpose is false then matrix must be laid out in column-major format (ie, col0, col1, col2).
		 *
		 * NOTE: Requires 'GL_ARB_gpu_shader_fp64'.
		 */
		bool
		gl_uniform_matrix3x3d(
				GLRenderer &renderer,
				const char *name,
				const GLdouble *value,
				unsigned int count,
				GLboolean transpose);

		/**
		 * Performs same function as the glUniformMatrix4fv OpenGL function - returns false if not active.
		 *
		 * NOTE: If @a transpose is false then matrix must be laid out in column-major format (ie, col0, col1, col2, col3).
		 */
		bool
		gl_uniform_matrix4x4f(
				GLRenderer &renderer,
				const char *name,
				const GLfloat *value,
				unsigned int count,
				GLboolean transpose);

		/**
		 * Performs same function as the glUniformMatrix4dv OpenGL function - returns false if not active.
		 *
		 * NOTE: If @a transpose is false then matrix must be laid out in column-major format (ie, col0, col1, col2, col3).
		 *
		 * NOTE: Requires 'GL_ARB_gpu_shader_fp64'.
		 */
		bool
		gl_uniform_matrix4x4d(
				GLRenderer &renderer,
				const char *name,
				const GLdouble *value,
				unsigned int count,
				GLboolean transpose);


		//! Performs same function as the glUniformMatrix4fv OpenGL function with a single matrix - returns false if not active.
		bool
		gl_uniform_matrix4x4f(
				GLRenderer &renderer,
				const char *name,
				const GLMatrix &matrix);

		/**
		 * Performs same function as the glUniformMatrix4dv OpenGL function with a single matrix - returns false if not active.
		 *
		 * NOTE: Requires 'GL_ARB_gpu_shader_fp64'.
		 */
		bool
		gl_uniform_matrix4x4d(
				GLRenderer &renderer,
				const char *name,
				const GLMatrix &matrix);

		//! Performs same function as the glUniformMatrix4fv OpenGL function with one or more matrices - returns false if not active.
		bool
		gl_uniform_matrix4x4f(
				GLRenderer &renderer,
				const char *name,
				const std::vector<GLMatrix> &matrices);

		/**
		 * Performs same function as the glUniformMatrix4dv OpenGL function with one or more matrices - returns false if not active.
		 *
		 * NOTE: Requires 'GL_ARB_gpu_shader_fp64'.
		 */
		bool
		gl_uniform_matrix4x4d(
				GLRenderer &renderer,
				const char *name,
				const std::vector<GLMatrix> &matrices);


		/**
		 * Returns the program resource handle.
		 *
		 * NOTE: This is a lower-level function used to help implement the OpenGL framework.
		 */
		resource_handle_type
		get_program_resource_handle() const
		{
			return d_resource->get_resource_handle();
		}

	private:
		//! Typedef for a name of a uniform variable.
		typedef std::string uniform_name_type;

		//! Typedef for the index, or location, of a uniform variable.
		typedef GLint uniform_location_type;

		//! Typedef for a map of uniform variable names to indices (or locations).
		typedef std::map<uniform_name_type, uniform_location_type> uniform_location_map_type;


		resource_type::non_null_ptr_to_const_type d_resource;

		mutable uniform_location_map_type d_uniform_locations;


		//! Constructor.
		GLProgramObject(
				GLRenderer &renderer);

		/**
		 * Get the uniform location index of the specified uniform variable name.
		 */
		uniform_location_type
		get_uniform_location(
				const char *uniform_name) const;
	};
}

#endif // GPLATES_OPENGL_GLPROGRAMOBJECT_H
