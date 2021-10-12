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

#include "GLCubeSubdivision.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


/**
 * Standard directions used by 3D graphics APIs for cube map textures so we'll adopt
 * the same convention.
 */
const GPlatesMaths::UnitVector3D
GPlatesOpenGL::GLCubeSubdivision::UV_FACE_DIRECTIONS[6][2] =
{
	{ GPlatesMaths::UnitVector3D(0,0,-1), GPlatesMaths::UnitVector3D(0,-1,0) },
	{ GPlatesMaths::UnitVector3D(0,0,1), GPlatesMaths::UnitVector3D(0,-1,0) },
	{ GPlatesMaths::UnitVector3D(1,0,0), GPlatesMaths::UnitVector3D(0,0,1) },
	{ GPlatesMaths::UnitVector3D(1,0,0), GPlatesMaths::UnitVector3D(0,0,-1) },
	{ GPlatesMaths::UnitVector3D(1,0,0), GPlatesMaths::UnitVector3D(0,-1,0) },
	{ GPlatesMaths::UnitVector3D(-1,0,0), GPlatesMaths::UnitVector3D(0,-1,0) }
};

const GPlatesMaths::UnitVector3D
GPlatesOpenGL::GLCubeSubdivision::FACE_NORMALS[6] =
{
	GPlatesMaths::UnitVector3D(1,0,0),
	GPlatesMaths::UnitVector3D(-1,0,0),
	GPlatesMaths::UnitVector3D(0,1,0),
	GPlatesMaths::UnitVector3D(0,-1,0),
	GPlatesMaths::UnitVector3D(0,0,1),
	GPlatesMaths::UnitVector3D(0,0,-1)
};


GPlatesOpenGL::GLTransform::non_null_ptr_to_const_type
GPlatesOpenGL::GLCubeSubdivision::get_projection_transform(
		CubeFaceType cube_face,
		unsigned int level_of_detail,
		unsigned int tile_u_offset,
		unsigned int tile_v_offset) const
{
	const unsigned int num_subdivisions = (1 << level_of_detail);

	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			tile_u_offset < num_subdivisions && tile_v_offset < num_subdivisions,
			GPLATES_ASSERTION_SOURCE);

	// Start off with an identify projection matrix.
	GLTransform::non_null_ptr_type projection = GLTransform::create(GL_PROJECTION);
	GLMatrix &projection_matrix = projection->get_matrix();

	//
	// See http://www.opengl.org/resources/code/samples/sig99/advanced99/notes/node30.html
	// for an explanation of the following...
	//
	// Basically we're setting up off-axis perspective view frustums that view from the
	// centre of the globe to a square sub-section of the cube face.
	//
	// Doing it this way also makes it easier to make further adjustments such
	// as having overlapping subdivisions (eg, one texel overlap between adjacent tiles).
	//

	const GLdouble inv_num_subdivisions = 1.0 / num_subdivisions;

	// Scale the subdivision view volume to fill NDC space (-1,1).
	projection_matrix.gl_scale(num_subdivisions, num_subdivisions, 1);

	// Translate the subdivided tile so that's it is centred about the z axis.
	projection_matrix.gl_translate(
			1 - (2 * tile_u_offset + 1) * inv_num_subdivisions,
			1 - (2 * tile_v_offset + 1) * inv_num_subdivisions,
			0);

	// What gets translated and scaled is the 90 degree field-of-view perspective frustum
	// in normalised device coordinates space (NDC) - ie, the cube (-1,1) range on the three axes).
	// The 90 degrees is because that maps to the field-of-view of a cube face when
	// viewed from the centre of the globe.
	// This is done last because the order of multiplies is such that the last transform
	// specified is actually the first that's applied to a vertex being transformed.
	projection_matrix.glu_perspective(90.0, 1.0, d_near, d_far);

	return projection;
}


GPlatesOpenGL::GLTransform::non_null_ptr_to_const_type
GPlatesOpenGL::GLCubeSubdivision::get_view_transform(
		CubeFaceType cube_face,
		unsigned int level_of_detail,
		unsigned int tile_u_offset,
		unsigned int tile_v_offset) const
{
	const unsigned int num_subdivisions = (1 << level_of_detail);

	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			tile_u_offset < num_subdivisions && tile_v_offset < num_subdivisions,
			GPLATES_ASSERTION_SOURCE);

	// Start off with an identify view matrix.
	GLTransform::non_null_ptr_type view = GLTransform::create(GL_MODELVIEW);
	GLMatrix &view_matrix = view->get_matrix();

	// The view looks out from the centre of the globe along the face normal.
	// The 'up' orientation is determined by the 'V' direction (in the plane of the face).
	const GPlatesMaths::UnitVector3D &centre = FACE_NORMALS[cube_face];
	const GPlatesMaths::UnitVector3D &up = UV_FACE_DIRECTIONS[cube_face][1/*v*/];
	view_matrix.glu_look_at(
			0, 0, 0, /* eye */
			centre.x().dval(), centre.y().dval(), centre.z().dval(),
			up.x().dval(), up.y().dval(), up.z().dval());

	return view;
}
