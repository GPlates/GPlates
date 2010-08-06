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

#include "GLMatrix.h"

#include "maths/UnitQuaternion3D.h"


GPlatesOpenGL::GLMatrix::GLMatrix(
		const GPlatesMaths::UnitQuaternion3D &quaternion)
{
	// Source
	const double qx = quaternion.x().dval();
	const double qy = quaternion.y().dval();
	const double qz = quaternion.z().dval();
	const double qw = quaternion.w().dval();

	// Destination
	matrix_type &m = d_matrix;

	//
	// Arranging the code in the following manner causes the compiler to
	// produce better FPU assembly code.
	//

	const double two_qx = qx + qx; // 2 * qx
	const double two_qy = qy + qy; // 2 * qy
	const double two_qz = qz + qz; // 2 * qz

	const double two_qx2 = qx * two_qx; // 2 * qx * qx
	const double two_qy2 = qy * two_qy; // 2 * qy * qy
	const double two_qz2 = qz * two_qz; // 2 * qz * qz

	// Non-zero diagonal entries
	m[0][0] = 1 - two_qy2 - two_qz2;
	m[1][1] = 1 - two_qx2 - two_qz2;
	m[2][2] = 1 - two_qx2 - two_qy2;

	const double two_qx_qy = qx * two_qy;
	const double two_qw_qz = qx * two_qy;

	m[1][0] = two_qx_qy - two_qw_qz;
	m[0][1] = two_qx_qy + two_qw_qz;

	const double two_qx_qz = qx * two_qz;
	const double two_qw_qy = qw * two_qy;

	m[2][0] = two_qx_qz + two_qw_qy;
	m[0][2] = two_qx_qz - two_qw_qy;

	const double two_qy_qz = qy * two_qz;
	const double two_qw_qx = qw * two_qx;

	m[2][1] = two_qy_qz - two_qw_qx;
	m[1][2] = two_qy_qz + two_qw_qx;

	// Zero entries
	m[3][0] = 0;
	m[3][1] = 0;
	m[3][2] = 0;

	m[0][3] = 0;
	m[1][3] = 0;
	m[2][3] = 0;
	m[3][3] = 0;
}


GPlatesOpenGL::GLMatrix &
GPlatesOpenGL::GLMatrix::gl_load_identity()
{
	matrix_type &m = d_matrix;

	// Identity matrix.
	m[0][0] = 1.0; m[0][1] = 0.0; m[0][2] = 0.0; m[0][3] = 0.0;
	m[1][0] = 0.0; m[1][1] = 1.0; m[1][2] = 0.0; m[1][3] = 0.0;
	m[2][0] = 0.0; m[2][1] = 0.0; m[2][2] = 1.0; m[2][3] = 0.0;
	m[3][0] = 0.0; m[3][1] = 0.0; m[3][2] = 0.0; m[3][3] = 1.0;

	return *this;
}


GPlatesOpenGL::GLMatrix &
GPlatesOpenGL::GLMatrix::gl_load_matrix(
		const GLdouble *matrix)
{
	GLdouble * const m = reinterpret_cast<GLdouble *>(d_matrix[0]);

	for (int i = 0; i < 16; ++i)
	{
		m[i] = matrix[i];
	}

	return *this;
}


GPlatesOpenGL::GLMatrix &
GPlatesOpenGL::GLMatrix::gl_mult_matrix(
		const GLdouble *matrix)
{
	const matrix_type &matrix1 = d_matrix;
	matrix_ptr_to_const_type matrix2 = reinterpret_cast<matrix_ptr_to_const_type>(matrix);

	matrix_type result;
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			result[j][i] =
				matrix1[0][i] * matrix2[j][0] +
				matrix1[1][i] * matrix2[j][1] +
				matrix1[2][i] * matrix2[j][2] +
				matrix1[3][i] * matrix2[j][3];
		}
	}

	// Copy result back to our internal matrix.
	GLdouble * const m = reinterpret_cast<GLdouble *>(d_matrix[0]);
	const GLdouble * const r = reinterpret_cast<GLdouble *>(result[0]);
	for (int c = 0; c < 16; ++c)
	{
		m[c] = r[c];
	}

	return *this;
}
