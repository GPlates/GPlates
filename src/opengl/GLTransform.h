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

		/*
		 * The methods accepting 'GLenum matrix_mode' only accept GL_MODELVIEW and GL_PROJECTION.
		 *
		 * GL_TEXTURE is *not* included here because:
		 * - it is bound to the currently active texture unit unlike GL_MODELVIEW and GL_PROJECTION,
		 * - it does not normally follow a hierarchy of transformations like GL_MODELVIEW tends to,
		 * - it is infrequently used when rendering drawables.
		 * So for these reasons GL_TEXTURE is implemented in class @a GLTextureTransformState.
		 */

		/**
		 * Constructs identity matrix.
		 *
		 * @a matrix_mode must be GL_MODELVIEW or GL_PROJECTION (GL_TEXTURE not included - see above).
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
		 * @a matrix_mode must be GL_MODELVIEW or GL_PROJECTION (GL_TEXTURE not included - see above).
		 */
		static
		non_null_ptr_type
		create(
				GLenum matrix_mode,
				const GLMatrix &matrix)
		{
			return non_null_ptr_type(new GLTransform(matrix_mode, matrix));
		}

		/**
		 * Constructs arbitrary matrix.
		 *
		 * @a matrix_mode must be GL_MODELVIEW or GL_PROJECTION (GL_TEXTURE not included - see above).
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
		 * @a matrix_mode must be GL_MODELVIEW or GL_PROJECTION (GL_TEXTURE not included - see above).
		 */
		static
		non_null_ptr_type
		create(
				GLenum matrix_mode,
				const GPlatesMaths::UnitQuaternion3D &quaternion)
		{
			return non_null_ptr_type(new GLTransform(matrix_mode, quaternion));
		}


		/**
		 * Returns a clone of 'this' transform.
		 */
		non_null_ptr_type
		clone() const
		{
			// This class is not copy-constructable due to ReferenceCount base class
			// so use a non-copy constructor instead that achieves the same effect.
			return non_null_ptr_type(new GLTransform(d_matrix_mode, d_matrix));
		}


		/**
		 * Returns one of GL_MODELVIEW or GL_PROJECTION (GL_TEXTURE not included - see above).
		 */
		GLenum
		get_matrix_mode() const
		{
			return d_matrix_mode;
		}

		/**
		 * Returns the 4x4 matrix of 'this' transform in OpenGL format.
		 *
		 * This can be used to alter the matrix via methods in @a GLMatrix.
		 */
		GLMatrix &
		get_matrix()
		{
			return d_matrix;
		}

		/**
		 * Returns the 4x4 matrix of 'this' transform in OpenGL format.
		 */
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
		explicit
		GLTransform(
				GLenum matrix_mode,
				const GLMatrix &matrix);

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
