/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
 
#ifndef GPLATES_OPENGL_GLTRANSFORM_H
#define GPLATES_OPENGL_GLTRANSFORM_H

#include <opengl/OpenGL.h>

#include "GLMatrix.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesMaths
{
	class UnitQuaternion3D;
}

namespace GPlatesOpenGL
{
	/**
	 * Used to set a GL_MODELVIEW or GL_PROJECTION transform 4x4 matrix.
	 */
	class GLTransform :
			public GPlatesUtils::ReferenceCount<GLTransform>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLTransform.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLTransform> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLTransform.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLTransform> non_null_ptr_to_const_type;


		/**
		 * Constructs identity matrix.
		 *
		 * @a matrix_mode must be one of GL_MODELVIEW or GL_PROJECTION.
		 * NOTE: GL_TEXTURE is *not* included here (see @a GLTransformState for the reasons).
		 */
		static
		non_null_ptr_type
		create(
				GLenum matrix_mode)
		{
			return non_null_ptr_type(new GLTransform(matrix_mode));
		}

		/**
		 * Constructs arbitrary matrix.
		 *
		 * @a matrix_mode must be one of GL_MODELVIEW or GL_PROJECTION.
		 * NOTE: GL_TEXTURE is *not* included here (see @a GLTransformState for the reasons).
		 *
		 * The format of @a m must be column-major:
		 *
		 * | m0 m4 m8  m12 |
		 * | m1 m5 m9  m13 |
		 * | m2 m6 m10 m14 |
		 * | m3 m7 m11 m15 |
		 */
		static
		non_null_ptr_type
		create(
				GLenum matrix_mode,
				const GLdouble *matrix)
		{
			return non_null_ptr_type(new GLTransform(matrix_mode, matrix));
		}

		/**
		 * Constructs 4x4 matrix from specified unit quaternion (note only the 3x3 rotation
		 * part of the matrix is initialised - the rest is set to zero).
		 *
		 * @a matrix_mode must be one of GL_MODELVIEW or GL_PROJECTION.
		 * NOTE: GL_TEXTURE is *not* included here (see @a GLTransformState for the reasons).
		 */
		static
		non_null_ptr_type
		create(
				GLenum matrix_mode,
				const GPlatesMaths::UnitQuaternion3D &quaternion)
		{
			return non_null_ptr_type(new GLTransform(matrix_mode, quaternion));
		}


		//! Performs function of similarly named OpenGL function.
		GLTransform &
		gl_load_identity()
		{
			d_matrix.gl_load_identity();
			return *this;
		}


		/**
		 * Constructs an arbitrary 4x4 matrix.
		 *
		 * The format of @a m must be column-major:
		 *
		 * | m0 m4 m8  m12 |
		 * | m1 m5 m9  m13 |
		 * | m2 m6 m10 m14 |
		 * | m3 m7 m11 m15 |
		 */
		GLTransform &
		gl_load_matrix(
				const GLdouble *matrix)
		{
			d_matrix.gl_load_matrix(matrix);
			return *this;
		}


		/**
		 * Converts @a quaternion to a 3x3 OpenGL format matrix and post-multiplies it
		 * with the current internal matrix.
		 */
		GLTransform &
		gl_mult_matrix(
				const GPlatesMaths::UnitQuaternion3D &quaternion)
		{
			const GLMatrix quat_matrix(quaternion);
			d_matrix.gl_mult_matrix(quat_matrix);
			return *this;
		}

		/**
		 * Post-multiplies matrix @a matrix with the current internal matrix.
		 *
		 * Format of @a m is OpenGL column-major matrix format
		 * (column 0 stored in first 4 elements, column 1 in next 4, etc).
		 */
		GLTransform &
		gl_mult_matrix(
				const GLMatrix &matrix)
		{
			d_matrix.gl_mult_matrix(matrix);
			return *this;
		}


		//! Performs function of similarly named OpenGL function (including post-multiplication).
		GLTransform &
		gl_translate(
				double x,
				double y,
				double z);

		//! Performs function of similarly named OpenGL function (including post-multiplication).
		GLTransform &
		gl_rotate(
				double angle,
				double x,
				double y,
				double z);

		//! Performs function of similarly named OpenGL function (including post-multiplication).
		GLTransform &
		gl_scale(
				double x,
				double y,
				double z);

		//! Performs function of similarly named OpenGL function (including post-multiplication).
		GLTransform &
		gl_ortho(
				double left,
				double right,
				double bottom,
				double top,
				double zNear,
				double zFar);

		//! Performs function of similarly named OpenGL function (including post-multiplication).
		GLTransform &
		gl_frustum(
				double left,
				double right,
				double bottom,
				double top,
				double zNear,
				double zFar);

		//! Performs function of similarly named OpenGL function (including post-multiplication).
		GLTransform &
		glu_look_at(
				double eyex,
				double eyey,
				double eyez,
				double centerx,
				double centery,
				double centerz,
				double upx,
				double upy,
				double upz);

		//! Performs function of similarly named OpenGL function (including post-multiplication).
		GLTransform &
		glu_ortho_2D(
				double left,
				double right,
				double bottom,
				double top)
		{
			return gl_ortho(left, right, bottom, top, -1.0, 1.0);
		}

		//! Performs function of similarly named OpenGL function (including post-multiplication).
		GLTransform &
		glu_perspective(
				double fovy,
				double aspect,
				double zNear,
				double zFar);


		////////////////////////////
		// Used by implementation //
		////////////////////////////

		/**
		 * Returns one of GL_MODELVIEW or GL_PROJECTION.
		 * NOTE: GL_TEXTURE is *not* included here (see @a GLTransformState for the reasons).
		 */
		GLenum
		get_matrix_mode() const
		{
			return d_matrix_mode;
		}

		//! Returns the 4x4 matrix of 'this' transform in OpenGL format.
		const GLMatrix &
		get_matrix() const
		{
			return d_matrix;
		}

	private:
		GLenum d_matrix_mode;
		GLMatrix d_matrix;


		//! Constructor.
		explicit
		GLTransform(
				GLenum matrix_mode);

		//! Constructor.
		GLTransform(
				GLenum matrix_mode,
				const GLdouble *matrix);

		//! Constructor.
		GLTransform(
				GLenum matrix_mode,
				const GPlatesMaths::UnitQuaternion3D &quaternion);
	};
}

#endif // GPLATES_OPENGL_GLTRANSFORM_H
