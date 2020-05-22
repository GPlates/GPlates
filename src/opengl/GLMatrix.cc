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

#include <cmath>
#include <QDebug>

#include "GLMatrix.h"

#include "maths/MathsUtils.h"
#include "maths/UnitQuaternion3D.h"
#include "maths/UnitVector3D.h"
#include "maths/Vector3D.h"


const GPlatesOpenGL::GLMatrix GPlatesOpenGL::GLMatrix::IDENTITY;


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
	const double two_qw_qz = qw * two_qz;

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
	m[3][3] = 1.0;
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
	GLdouble * const m = reinterpret_cast<GLdouble *>(d_matrix);

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
	GLdouble * const m = reinterpret_cast<GLdouble *>(d_matrix);
	const GLdouble * const r = reinterpret_cast<GLdouble *>(result);
	for (int c = 0; c < 16; ++c)
	{
		m[c] = r[c];
	}

	return *this;
}


GPlatesOpenGL::GLMatrix &
GPlatesOpenGL::GLMatrix::gl_translate(
		double x,
		double y,
		double z)
{
	GLdouble translate[16];

	// Column 0
	translate[0] = 1.0;
	translate[1] = 0.0;
	translate[2] = 0.0;
	translate[3] = 0.0;

	// Column 1
	translate[4] = 0.0;
	translate[5] = 1.0;
	translate[6] = 0.0;
	translate[7] = 0.0;

	// Column 2
	translate[8] = 0.0;
	translate[9] = 0.0;
	translate[10] = 1.0;
	translate[11] = 0.0;

	// Column 3
	translate[12] = x;
	translate[13] = y;
	translate[14] = z;
	translate[15] = 1.0;

	gl_mult_matrix(translate);

	return *this;
}


GPlatesOpenGL::GLMatrix &
GPlatesOpenGL::GLMatrix::gl_rotate(
		double angle_degrees,
		double x,
		double y,
		double z)
{
	const GLdouble mag_xyz = x * x + y * y + z * z;

	// If the magnitude of the (x,y,z) vector is zero the we are effectively
	// multiplying by the identity matrix when we do nothing and return.
	if (mag_xyz < 1e-12)
	{
		qWarning() << "Zero vector passed to GLTransform::gl_rotate().";
		return *this;
	}

	// Normalise (x,y,z).
	const GLdouble inv_mag_xyz = 1.0 / mag_xyz;
	x *= inv_mag_xyz;
	y *= inv_mag_xyz;
	z *= inv_mag_xyz;

	const double angle = GPlatesMaths::convert_deg_to_rad(angle_degrees);

	GLdouble rotate[16];

	const GLdouble c = std::cos(angle);
	const GLdouble s = std::sin(angle);

	const GLdouble one_minus_c = 1.0 - c;

	const GLdouble xy = x * y;
	const GLdouble yz = y * z;
	const GLdouble xz = x * z;

	const GLdouble xs = x * s;
	const GLdouble ys = y * s;
	const GLdouble zs = z * s;

	// Column 0
	rotate[0] = x * x * one_minus_c + c;
	rotate[1] = xy * one_minus_c + zs;
	rotate[2] = xz * one_minus_c - ys;
	rotate[3] = 0.0;

	// Column 1
	rotate[4] = xy * one_minus_c - zs;
	rotate[5] = y * y * one_minus_c + c;
	rotate[6] = yz * one_minus_c + xs;
	rotate[7] = 0.0;

	// Column 2
	rotate[8] = xz * one_minus_c + ys;
	rotate[9] = yz * one_minus_c - xs;
	rotate[10] = z * z * one_minus_c + c;
	rotate[11] = 0.0;

	// Column 3
	rotate[12] = 0.0;
	rotate[13] = 0.0;
	rotate[14] = 0.0;
	rotate[15] = 1.0;

	gl_mult_matrix(rotate);

	return *this;
}


GPlatesOpenGL::GLMatrix &
GPlatesOpenGL::GLMatrix::gl_scale(
		double x,
		double y,
		double z)
{
	GLdouble scale[16];

	// Column 0
	scale[0] = x;
	scale[1] = 0.0;
	scale[2] = 0.0;
	scale[3] = 0.0;

	// Column 1
	scale[4] = 0.0;
	scale[5] = y;
	scale[6] = 0.0;
	scale[7] = 0.0;

	// Column 2
	scale[8] = 0.0;
	scale[9] = 0.0;
	scale[10] = z;
	scale[11] = 0.0;

	// Column 3
	scale[12] = 0.0;
	scale[13] = 0.0;
	scale[14] = 0.0;
	scale[15] = 1.0;

	gl_mult_matrix(scale);

	return *this;
}


GPlatesOpenGL::GLMatrix &
GPlatesOpenGL::GLMatrix::gl_ortho(
		double left,
		double right,
		double bottom,
		double top,
		double zNear,
		double zFar)
{
	GLdouble ortho[16];

	const GLdouble inv_right_left = 1.0 / (right - left);

	// Column 0
	ortho[0] = 2.0 * inv_right_left;
	ortho[1] = 0.0;
	ortho[2] = 0.0;
	ortho[3] = 0.0;

	const GLdouble inv_top_bottom = 1.0 / (top - bottom);

	// Column 1
	ortho[4] = 0.0;
	ortho[5] = 2.0 * inv_top_bottom;
	ortho[6] = 0.0;
	ortho[7] = 0.0;

	const GLdouble inv_far_near = 1.0 / (zFar - zNear);

	// Column 2
	ortho[8] = 0.0;
	ortho[9] = 0.0;
	ortho[10] = -2.0 * inv_far_near;
	ortho[11] = 0.0;

	const GLdouble tx = -(right + left) * inv_right_left;
	const GLdouble ty = -(top + bottom) * inv_top_bottom;
	const GLdouble tz = -(zFar + zNear) * inv_far_near;

	// Column 3
	ortho[12] = tx;
	ortho[13] = ty;
	ortho[14] = tz;
	ortho[15] = 1.0;

	gl_mult_matrix(ortho);

	return *this;
}


GPlatesOpenGL::GLMatrix &
GPlatesOpenGL::GLMatrix::gl_frustum(
		double left,
		double right,
		double bottom,
		double top,
		double zNear,
		double zFar)
{
	GLdouble frustum[16];

	const GLdouble inv_right_left = 1.0 / (right - left);
	const GLdouble two_near = 2 * zNear;

	// Column 0
	frustum[0] = two_near * inv_right_left;
	frustum[1] = 0.0;
	frustum[2] = 0.0;
	frustum[3] = 0.0;

	const GLdouble inv_top_bottom = 1.0 / (top - bottom);

	// Column 1
	frustum[4] = 0.0;
	frustum[5] = two_near * inv_top_bottom;
	frustum[6] = 0.0;
	frustum[7] = 0.0;

	const GLdouble inv_far_near = 1.0 / (zFar - zNear);

	// Column 2
	frustum[8] = (right + left) * inv_right_left;
	frustum[9] = (top + bottom) * inv_top_bottom;
	frustum[10] = -(zFar + zNear) * inv_far_near;
	frustum[11] = -1.0;

	// Column 3
	frustum[12] = 0.0;
	frustum[13] = 0.0;
	frustum[14] = -two_near * zFar * inv_far_near;
	frustum[15] = 0.0;

	gl_mult_matrix(frustum);

	return *this;
}


GPlatesOpenGL::GLMatrix &
GPlatesOpenGL::GLMatrix::glu_look_at(
		double eyex,
		double eyey,
		double eyez,
		double centerx,
		double centery,
		double centerz,
		double upx,
		double upy,
		double upz)
{
	const GPlatesMaths::Vector3D eye(eyex, eyey, eyez);
	const GPlatesMaths::Vector3D center(centerx, centery, centerz);
	const GPlatesMaths::Vector3D up(upx, upy, upz);

	const GPlatesMaths::UnitVector3D f = (center - eye).get_normalisation();

	const GPlatesMaths::UnitVector3D s( cross(f, up).get_normalisation() );

	const GPlatesMaths::Vector3D u( cross(s, f) );

	GLdouble look_at[16];

	// Column 0
	look_at[0] = s.x().dval();
	look_at[1] = u.x().dval();
	look_at[2] = -f.x().dval();
	look_at[3] = 0.0;

	// Column 1
	look_at[4] = s.y().dval();
	look_at[5] = u.y().dval();
	look_at[6] = -f.y().dval();
	look_at[7] = 0.0;

	// Column 2
	look_at[8] = s.z().dval();
	look_at[9] = u.z().dval();
	look_at[10] = -f.z().dval();
	look_at[11] = 0.0;

	// Column 3
	look_at[12] = 0.0;
	look_at[13] = 0.0;
	look_at[14] = 0.0;
	look_at[15] = 1.0;

	gl_mult_matrix(look_at);

	gl_translate(-eyex, -eyey, -eyez);

	return *this;
}


GPlatesOpenGL::GLMatrix &
GPlatesOpenGL::GLMatrix::glu_perspective(
		double fovy,
		double aspect,
		double zNear,
		double zFar)
{
	const GLdouble top = zNear * std::tan(fovy * GPlatesMaths::PI / 360.0);
	const GLdouble bottom = -top;

	const GLdouble left = bottom * aspect;
	const GLdouble right = top * aspect;

	return gl_frustum(left, right, bottom, top, zNear, zFar);
}


bool
GPlatesOpenGL::GLMatrix::glu_invert_matrix()
{
	// Read from our internal matrix when inverting.
	const GLdouble *const m = reinterpret_cast<const GLdouble *>(d_matrix);
	// Store intermediate inverse matrix results.
	GLdouble inv[16];

	//
	// Based on the MESA library but using column-major instead of row-major storage.
	//

	inv[0] = m[5] * m[10] * m[15] - m[5] * m[14] * m[11] - m[6] * m[9] * m[15]
		+ m[6] * m[13] * m[11] + m[7] * m[9] * m[14] - m[7] * m[13] * m[10];
	inv[1] = -m[1] * m[10] * m[15] + m[1] * m[14] * m[11] + m[2] * m[9] * m[15]
		- m[2] * m[13] * m[11] - m[3] * m[9] * m[14] + m[3] * m[13] * m[10];
	inv[2] = m[1] * m[6] * m[15] - m[1] * m[14] * m[7] - m[2] * m[5] * m[15]
		+ m[2] * m[13] * m[7] + m[3] * m[5] * m[14] - m[3] * m[13] * m[6];
	inv[3] = -m[1] * m[6] * m[11] + m[1] * m[10] * m[7] + m[2] * m[5] * m[11]
		- m[2] * m[9] * m[7] - m[3] * m[5] * m[10] + m[3] * m[9] * m[6];
	inv[4] = -m[4] * m[10] * m[15] + m[4] * m[14] * m[11] + m[6] * m[8] * m[15]
		- m[6] * m[12] * m[11] - m[7] * m[8] * m[14] + m[7] * m[12] * m[10];
	inv[5] = m[0] * m[10] * m[15] - m[0] * m[14] * m[11] - m[2] * m[8] * m[15]
		+ m[2] * m[12] * m[11] + m[3] * m[8] * m[14] - m[3] * m[12] * m[10];
	inv[6] = -m[0] * m[6] * m[15] + m[0] * m[14] * m[7] + m[2] * m[4] * m[15]
		- m[2] * m[12] * m[7] - m[3] * m[4] * m[14] + m[3] * m[12] * m[6];
	inv[7] = m[0] * m[6] * m[11] - m[0] * m[10] * m[7] - m[2] * m[4] * m[11]
		+ m[2] * m[8] * m[7] + m[3] * m[4] * m[10] - m[3] * m[8] * m[6];
	inv[8] = m[4] * m[9] * m[15] - m[4] * m[13] * m[11] - m[5] * m[8] * m[15]
		+ m[5] * m[12] * m[11] + m[7] * m[8] * m[13] - m[7] * m[12] * m[9];
	inv[9] = -m[0] * m[9] * m[15] + m[0] * m[13] * m[11] + m[1] * m[8] * m[15]
		- m[1] * m[12] * m[11] - m[3] * m[8] * m[13] + m[3] * m[12] * m[9];
	inv[10] = m[0] * m[5] * m[15] - m[0] * m[13] * m[7] - m[1] * m[4] * m[15]
		+ m[1] * m[12] * m[7] + m[3] * m[4] * m[13] - m[3] * m[12] * m[5];
	inv[11] = -m[0] * m[5] * m[11] + m[0] * m[9] * m[7] + m[1] * m[4] * m[11]
		- m[1] * m[8] * m[7] - m[3] * m[4] * m[9] + m[3] * m[8] * m[5];
	inv[12] = -m[4] * m[9] * m[14] + m[4] * m[13] * m[10] + m[5] * m[8] * m[14]
		- m[5] * m[12] * m[10] - m[6] * m[8] * m[13] + m[6] * m[12] * m[9];
	inv[13] = m[0] * m[9] * m[14] - m[0] * m[13] * m[10] - m[1] * m[8] * m[14]
		+ m[1] * m[12] * m[10] + m[2] * m[8] * m[13] - m[2] * m[12] * m[9];
	inv[14] = -m[0] * m[5] * m[14] + m[0] * m[13] * m[6] + m[1] * m[4] * m[14]
		- m[1] * m[12] * m[6] - m[2] * m[4] * m[13] + m[2] * m[12] * m[5];
	inv[15] = m[0] * m[5] * m[10] - m[0] * m[9] * m[6] - m[1] * m[4] * m[10]
		+ m[1] * m[8] * m[6] + m[2] * m[4] * m[9] - m[2] * m[8] * m[5];

	const double det = m[0] * inv[0] + m[4] * inv[1] + m[8] * inv[2] + m[12] * inv[3];

	if (GPlatesMaths::are_almost_exactly_equal(det, 0.0))
	{
		return false;
	}

	const double inv_det = 1.0 / det;

	// Divide by determinant and store result back to our internal matrix.
	GLdouble *const m_inverted = reinterpret_cast<GLdouble *>(d_matrix);
	for (unsigned int i = 0; i < 16; i++)
	{
		m_inverted[i] = inv[i] * inv_det;
	}

	return true;
}
