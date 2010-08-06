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
 
#ifndef GPLATES_OPENGL_GLMATRIX_H
#define GPLATES_OPENGL_GLMATRIX_H

#include <opengl/OpenGL.h>

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesMaths
{
	class UnitQuaternion3D;
}

namespace GPlatesOpenGL
{
	/**
	 * A 4x4 matrix in OpenGL column-major format.
	 *
	 * OpenGL column-major matrix format means column 0 is stored in first 4 elements,
	 * column 1 in next 4, etc, as in:
	 *
	 * | m0 m4 m8  m12 |
	 * | m1 m5 m9  m13 |
	 * | m2 m6 m10 m14 |
	 * | m3 m7 m11 m15 |
	 *
	 * NOTE: This means that post-multiply of column-major matrices (OpenGL) is
	 * equivalent to pre-multiply of row-major matrices (the usual way matrices are stored).
	 *
	 * This functionality of this class could be extracted into a class in the GPlatesMaths
	 * namespace that is row-major and this @a GLMatrix class could just wrap that.
	 * In the meantime will just put the functionality in @a GLMatrix since it's the only
	 * code that uses matrices - the rest of GPlates should use quaternions directly in
	 * any transformations with the final conversion to matrix format for OpenGL.
	 * Any matrix transformations done by @a GLMatrix instead of a quaternion are purely
	 * view/visual related such as changing the view position, rotating the globe, etc.
	 */
	class GLMatrix :
			public GPlatesUtils::ReferenceCount<GLMatrix>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLMatrix.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLMatrix> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLMatrix.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLMatrix> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLMatrix object - creates identity matrix.
		 */
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new GLMatrix());
		}

		/**
		 * Creates a @a GLMatrix object - creates arbitrary 4x4 matrix.
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
				const GLdouble *matrix)
		{
			return non_null_ptr_type(new GLMatrix(matrix));
		}

		/**
		 * Creates a @a GLMatrix object - creates matrix from a quaternion.
		 *
		 * Constructs 4x4 matrix from specified unit quaternion (note only the 3x3 rotation
		 * part of the matrix is initialised - the rest is set to zero).
		 */
		static
		non_null_ptr_type
		create(
				const GPlatesMaths::UnitQuaternion3D &quaternion)
		{
			return non_null_ptr_type(new GLMatrix(quaternion));
		}


		//! Constructor - creates identity matrix.
		GLMatrix()
		{
			gl_load_identity();
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
		explicit
		GLMatrix(
				const GLdouble *matrix)
		{
			gl_load_matrix(matrix);
		}

		/**
		 * Constructs 4x4 matrix from specified unit quaternion (note only the 3x3 rotation
		 * part of the matrix is initialised - the rest is set to zero).
		 */
		explicit
		GLMatrix(
				const GPlatesMaths::UnitQuaternion3D &quaternion);

		/**
		 * Returns a clone of 'this' matrix.
		 */
		non_null_ptr_type
		clone()
		{
			// This class is not copy-constructable due to ReferenceCount base class
			// so use a non-copy constructor instead that achieves the same effect.
			return non_null_ptr_type(new GLMatrix(get_matrix()));
		}


		//! Performs function of similarly named OpenGL function.
		GLMatrix &
		gl_load_identity();

		/**
		 * Loads an arbitrary 4x4 matrix.
		 *
		 * The format of @a matrix must be column-major:
		 *
		 * | m0 m4 m8  m12 |
		 * | m1 m5 m9  m13 |
		 * | m2 m6 m10 m14 |
		 * | m3 m7 m11 m15 |
		 */
		GLMatrix &
		gl_load_matrix(
				const GLdouble *matrix);

		/**
		 * Post-multiplies matrix @a matrix with the current internal matrix.
		 */
		GLMatrix &
		gl_mult_matrix(
				const GLMatrix &matrix)
		{
			return gl_mult_matrix(reinterpret_cast<const GLdouble *>(matrix.d_matrix[0]));
		}

		/**
		 * Post-multiplies matrix @a matrix with the current internal matrix.
		 *
		 * The format of @a matrix must be column-major:
		 *
		 * | m0 m4 m8  m12 |
		 * | m1 m5 m9  m13 |
		 * | m2 m6 m10 m14 |
		 * | m3 m7 m11 m15 |
		 */
		GLMatrix &
		gl_mult_matrix(
				const GLdouble *matrix);


		////////////////////////////
		// Used by implementation //
		////////////////////////////

		//! Returns internal matrix in OpenGL column-major format.
		const GLdouble *
		get_matrix() const
		{
			return reinterpret_cast<const GLdouble *>(d_matrix[0]);
		}

	private:
		//! Typedef for a contiguous array of 16 doubles (in 4x4 format).
		typedef GLdouble matrix_type[4][4];

		//! Typedef for a pointer to a 4x4 const array.
		typedef const GLdouble (*matrix_ptr_to_const_type)[4];

		//! Typedef for a pointer to a 4x4 array.
		typedef GLdouble (*matrix_ptr_type)[4];


		matrix_type d_matrix;
	};
}

#endif // GPLATES_OPENGL_GLMATRIX_H
