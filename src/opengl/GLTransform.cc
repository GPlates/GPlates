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

#include "GLTransform.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "maths/MathsUtils.h"
#include "maths/UnitVector3D.h"
#include "maths/Vector3D.h"


GPlatesOpenGL::GLTransform::GLTransform(
		GLenum matrix_mode) :
	d_matrix_mode(matrix_mode)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			matrix_mode == GL_MODELVIEW || matrix_mode == GL_PROJECTION,
			GPLATES_ASSERTION_SOURCE);
}


GPlatesOpenGL::GLTransform::GLTransform(
		GLenum matrix_mode,
		const GLdouble *matrix) :
	d_matrix_mode(matrix_mode),
	d_matrix(matrix)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			matrix_mode == GL_MODELVIEW || matrix_mode == GL_PROJECTION,
			GPLATES_ASSERTION_SOURCE);
}


GPlatesOpenGL::GLTransform::GLTransform(
		GLenum matrix_mode,
		const GPlatesMaths::UnitQuaternion3D &quaternion) :
	d_matrix_mode(matrix_mode),
	d_matrix(quaternion)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			matrix_mode == GL_MODELVIEW || matrix_mode == GL_PROJECTION,
			GPLATES_ASSERTION_SOURCE);
}


GPlatesOpenGL::GLTransform &
GPlatesOpenGL::GLTransform::gl_translate(
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

	d_matrix.gl_mult_matrix(translate);

	return *this;
}


GPlatesOpenGL::GLTransform &
GPlatesOpenGL::GLTransform::gl_rotate(
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

	d_matrix.gl_mult_matrix(rotate);

	return *this;
}


GPlatesOpenGL::GLTransform &
GPlatesOpenGL::GLTransform::gl_scale(
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

	d_matrix.gl_mult_matrix(scale);

	return *this;
}


GPlatesOpenGL::GLTransform &
GPlatesOpenGL::GLTransform::gl_ortho(
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

	d_matrix.gl_mult_matrix(ortho);

	return *this;
}


GPlatesOpenGL::GLTransform &
GPlatesOpenGL::GLTransform::gl_frustum(
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

	d_matrix.gl_mult_matrix(frustum);

	return *this;
}


GPlatesOpenGL::GLTransform &
GPlatesOpenGL::GLTransform::glu_look_at(
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

	const GPlatesMaths::UnitVector3D s( cross(f, up.get_normalisation()) );

	const GPlatesMaths::UnitVector3D u( cross(s, f) );

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

	d_matrix.gl_mult_matrix(look_at);

	return *this;
}


GPlatesOpenGL::GLTransform &
GPlatesOpenGL::GLTransform::glu_perspective(
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
