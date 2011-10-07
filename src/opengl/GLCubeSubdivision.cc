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

#include "maths/PointOnSphere.h"

#include "utils/Base2Utils.h"


GPlatesOpenGL::GLCubeSubdivision::GLCubeSubdivision(
		std::size_t tile_texel_dimension,
		GLdouble zNear,
		GLdouble zFar) :
	d_tile_texel_dimension(tile_texel_dimension),
	d_near(zNear),
	d_far(zFar)
{
	// Tile dimension should be a power-of-two.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPlatesUtils::Base2::is_power_of_two(tile_texel_dimension),
			GPLATES_ASSERTION_SOURCE);
}


GPlatesOpenGL::GLTransform::non_null_ptr_to_const_type
GPlatesOpenGL::GLCubeSubdivision::get_view_transform(
		GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face) const
{
	// Start off with an identify view matrix.
	GLTransform::non_null_ptr_type view = GLTransform::create();
	GLMatrix &view_matrix = view->get_matrix();

	// The view looks out from the centre of the globe along the face normal.
	// The 'up' orientation is determined by the 'V' direction (in the plane of the face).
	// Note that:
	//    cross(U, V) = -NORMAL
	// and this is a result of looking outwards from the centre of the cube (which is how
	// rendering is done).
	const GPlatesMaths::UnitVector3D &centre =
			GPlatesMaths::CubeCoordinateFrame::get_cube_face_coordinate_frame_axis(
					cube_face,
					GPlatesMaths::CubeCoordinateFrame::Z_AXIS);
	const GPlatesMaths::UnitVector3D &up =
			GPlatesMaths::CubeCoordinateFrame::get_cube_face_coordinate_frame_axis(
					cube_face,
					GPlatesMaths::CubeCoordinateFrame::Y_AXIS/*v*/);
	view_matrix.glu_look_at(
			0, 0, 0, /* eye */
			centre.x().dval(), centre.y().dval(), centre.z().dval(),
			up.x().dval(), up.y().dval(), up.z().dval());

	return view;
}


GPlatesOpenGL::GLTransform::non_null_ptr_to_const_type
GPlatesOpenGL::GLCubeSubdivision::get_projection_transform(
		unsigned int level_of_detail,
		unsigned int tile_u_offset,
		unsigned int tile_v_offset) const
{
	const unsigned int num_subdivisions = (1 << level_of_detail);

	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			tile_u_offset < num_subdivisions && tile_v_offset < num_subdivisions,
			GPLATES_ASSERTION_SOURCE);

	// Start off with an identify projection matrix.
	GLTransform::non_null_ptr_type projection = GLTransform::create();
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
			1 - (2.0 * tile_u_offset + 1) * inv_num_subdivisions,
			1 - (2.0 * tile_v_offset + 1) * inv_num_subdivisions,
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


GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type
GPlatesOpenGL::GLCubeSubdivision::get_bounding_polygon(
		GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face,
		unsigned int level_of_detail,
		unsigned int tile_u_offset,
		unsigned int tile_v_offset) const
{
	const unsigned int num_subdivisions = (1 << level_of_detail);

	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			tile_u_offset < num_subdivisions && tile_v_offset < num_subdivisions,
			GPLATES_ASSERTION_SOURCE);

	// The view looks out from the centre of the globe along the face normal.

	const GPlatesMaths::Vector3D face_centre(
			GPlatesMaths::CubeCoordinateFrame::get_cube_face_coordinate_frame_axis(
					cube_face,
					GPlatesMaths::CubeCoordinateFrame::Z_AXIS));
	const GPlatesMaths::UnitVector3D &u_direction =
			GPlatesMaths::CubeCoordinateFrame::get_cube_face_coordinate_frame_axis(
					cube_face,
					GPlatesMaths::CubeCoordinateFrame::X_AXIS/*u*/);
	const GPlatesMaths::UnitVector3D &v_direction =
			GPlatesMaths::CubeCoordinateFrame::get_cube_face_coordinate_frame_axis(
					cube_face,
					GPlatesMaths::CubeCoordinateFrame::Y_AXIS/*v*/);

	const double inv_num_subdivisions = 1.0 / num_subdivisions;

	// Determine where in [-1,1] on the cube face the subdivision tile lies.
	const double lower_u_scale = -1 + (2.0 * tile_u_offset) * inv_num_subdivisions;
	const double upper_u_scale = -1 + (2.0 * tile_u_offset + 2) * inv_num_subdivisions;
	const double lower_v_scale = -1 + (2.0 * tile_v_offset) * inv_num_subdivisions;
	const double upper_v_scale = -1 + (2.0 * tile_v_offset + 2) * inv_num_subdivisions;

	// Get the corner points of the subdivision tile and project onto the sphere.
	const GPlatesMaths::PointOnSphere corner_points[4] =
	{
		GPlatesMaths::PointOnSphere(
				(face_centre + lower_u_scale * u_direction + lower_v_scale * v_direction)
						.get_normalisation()),
		GPlatesMaths::PointOnSphere(
				(face_centre + upper_u_scale * u_direction + lower_v_scale * v_direction)
						.get_normalisation()),
		GPlatesMaths::PointOnSphere(
				(face_centre + upper_u_scale * u_direction + upper_v_scale * v_direction)
						.get_normalisation()),
		GPlatesMaths::PointOnSphere(
				(face_centre + lower_u_scale * u_direction + upper_v_scale * v_direction)
						.get_normalisation())
	};

	// Return the bounding polygon.
	//
	// It is counter-clockwise when viewed the sphere centre which means it's
	// clockwise when viewed from above the surface of the sphere.
	return GPlatesMaths::PolygonOnSphere::create_on_heap(corner_points, corner_points + 4);
}


GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type
GPlatesOpenGL::GLCubeSubdivision::get_loose_bounding_polygon(
		GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face,
		unsigned int level_of_detail,
		unsigned int tile_u_offset,
		unsigned int tile_v_offset) const
{
	const unsigned int num_subdivisions = (1 << level_of_detail);

	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			tile_u_offset < num_subdivisions && tile_v_offset < num_subdivisions,
			GPLATES_ASSERTION_SOURCE);

	// The view looks out from the centre of the globe along the face normal.

	const GPlatesMaths::Vector3D face_centre(
			GPlatesMaths::CubeCoordinateFrame::get_cube_face_coordinate_frame_axis(
					cube_face,
					GPlatesMaths::CubeCoordinateFrame::Z_AXIS));
	const GPlatesMaths::UnitVector3D &u_direction =
			GPlatesMaths::CubeCoordinateFrame::get_cube_face_coordinate_frame_axis(
					cube_face,
					GPlatesMaths::CubeCoordinateFrame::X_AXIS/*u*/);
	const GPlatesMaths::UnitVector3D &v_direction =
			GPlatesMaths::CubeCoordinateFrame::get_cube_face_coordinate_frame_axis(
					cube_face,
					GPlatesMaths::CubeCoordinateFrame::Y_AXIS/*v*/);

	const double inv_num_subdivisions = 1.0 / num_subdivisions;

	// Determine where in [-1,1] on the cube face the subdivision tile lies.
	// NOTE: Since this is a loose bounds we double the normal size of the tile which
	// involves subtracting/adding 'inv_num_subdivisions' compared to a non-loose tile.
	const double lower_u_scale = -1 + (2.0 * tile_u_offset - 1) * inv_num_subdivisions;
	const double upper_u_scale = -1 + (2.0 * tile_u_offset + 3) * inv_num_subdivisions;
	const double lower_v_scale = -1 + (2.0 * tile_v_offset - 1) * inv_num_subdivisions;
	const double upper_v_scale = -1 + (2.0 * tile_v_offset + 3) * inv_num_subdivisions;

	// Get the corner points of the subdivision tile and project onto the sphere.
	const GPlatesMaths::PointOnSphere corner_points[4] =
	{
		GPlatesMaths::PointOnSphere(
				(face_centre + lower_u_scale * u_direction + lower_v_scale * v_direction)
						.get_normalisation()),
		GPlatesMaths::PointOnSphere(
				(face_centre + upper_u_scale * u_direction + lower_v_scale * v_direction)
						.get_normalisation()),
		GPlatesMaths::PointOnSphere(
				(face_centre + upper_u_scale * u_direction + upper_v_scale * v_direction)
						.get_normalisation()),
		GPlatesMaths::PointOnSphere(
				(face_centre + lower_u_scale * u_direction + upper_v_scale * v_direction)
						.get_normalisation())
	};

	// Return the bounding polygon.
	//
	// It is counter-clockwise when viewed the sphere centre which means it's
	// clockwise when viewed from above the surface of the sphere.
	return GPlatesMaths::PolygonOnSphere::create_on_heap(corner_points, corner_points + 4);
}


GPlatesOpenGL::GLIntersect::OrientedBoundingBox
GPlatesOpenGL::GLCubeSubdivision::get_oriented_bounding_box(
		GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face,
		unsigned int level_of_detail,
		unsigned int tile_u_offset,
		unsigned int tile_v_offset) const
{
	const unsigned int num_subdivisions = (1 << level_of_detail);

	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			tile_u_offset < num_subdivisions && tile_v_offset < num_subdivisions,
			GPLATES_ASSERTION_SOURCE);

	// The view looks out from the centre of the globe along the face normal.

	const GPlatesMaths::Vector3D face_centre(
			GPlatesMaths::CubeCoordinateFrame::get_cube_face_coordinate_frame_axis(
					cube_face,
					GPlatesMaths::CubeCoordinateFrame::Z_AXIS));
	const GPlatesMaths::UnitVector3D &u_direction =
			GPlatesMaths::CubeCoordinateFrame::get_cube_face_coordinate_frame_axis(
					cube_face,
					GPlatesMaths::CubeCoordinateFrame::X_AXIS/*u*/);
	const GPlatesMaths::UnitVector3D &v_direction =
			GPlatesMaths::CubeCoordinateFrame::get_cube_face_coordinate_frame_axis(
					cube_face,
					GPlatesMaths::CubeCoordinateFrame::Y_AXIS/*v*/);

	const double inv_num_subdivisions = 1.0 / num_subdivisions;

	// Determine where in [-1,1] on the cube face the subdivision tile lies.
	const double lower_u_scale = -1 + (2.0 * tile_u_offset) * inv_num_subdivisions;
	const double upper_u_scale = -1 + (2.0 * tile_u_offset + 2) * inv_num_subdivisions;
	const double lower_v_scale = -1 + (2.0 * tile_v_offset) * inv_num_subdivisions;
	const double upper_v_scale = -1 + (2.0 * tile_v_offset + 2) * inv_num_subdivisions;

	// Get the corner points of the subdivision tile in the plane of the cube face.
	const GPlatesMaths::Vector3D face_corner_points[4] =
	{
		face_centre + lower_u_scale * u_direction + lower_v_scale * v_direction,
		face_centre + upper_u_scale * u_direction + lower_v_scale * v_direction,
		face_centre + upper_u_scale * u_direction + upper_v_scale * v_direction,
		face_centre + lower_u_scale * u_direction + upper_v_scale * v_direction
	};

	return create_oriented_bounding_box(face_corner_points);
}


GPlatesOpenGL::GLIntersect::OrientedBoundingBox
GPlatesOpenGL::GLCubeSubdivision::get_loose_oriented_bounding_box(
		GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face,
		unsigned int level_of_detail,
		unsigned int tile_u_offset,
		unsigned int tile_v_offset) const
{
	const unsigned int num_subdivisions = (1 << level_of_detail);

	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			tile_u_offset < num_subdivisions && tile_v_offset < num_subdivisions,
			GPLATES_ASSERTION_SOURCE);

	// The view looks out from the centre of the globe along the face normal.

	const GPlatesMaths::Vector3D face_centre(
			GPlatesMaths::CubeCoordinateFrame::get_cube_face_coordinate_frame_axis(
					cube_face,
					GPlatesMaths::CubeCoordinateFrame::Z_AXIS));
	const GPlatesMaths::UnitVector3D &u_direction =
			GPlatesMaths::CubeCoordinateFrame::get_cube_face_coordinate_frame_axis(
					cube_face,
					GPlatesMaths::CubeCoordinateFrame::X_AXIS/*u*/);
	const GPlatesMaths::UnitVector3D &v_direction =
			GPlatesMaths::CubeCoordinateFrame::get_cube_face_coordinate_frame_axis(
					cube_face,
					GPlatesMaths::CubeCoordinateFrame::Y_AXIS/*v*/);

	const double inv_num_subdivisions = 1.0 / num_subdivisions;

	// Determine where in [-1,1] on the cube face the subdivision tile lies.
	// NOTE: Since this is a loose bounds we double the normal size of the tile which
	// involves subtracting/adding 'inv_num_subdivisions' compared to a non-loose tile.
	const double lower_u_scale = -1 + (2.0 * tile_u_offset - 1) * inv_num_subdivisions;
	const double upper_u_scale = -1 + (2.0 * tile_u_offset + 3) * inv_num_subdivisions;
	const double lower_v_scale = -1 + (2.0 * tile_v_offset - 1) * inv_num_subdivisions;
	const double upper_v_scale = -1 + (2.0 * tile_v_offset + 3) * inv_num_subdivisions;

	// Get the corner points of the subdivision tile in the plane of the cube face.
	const GPlatesMaths::Vector3D face_corner_points[4] =
	{
		face_centre + lower_u_scale * u_direction + lower_v_scale * v_direction,
		face_centre + upper_u_scale * u_direction + lower_v_scale * v_direction,
		face_centre + upper_u_scale * u_direction + upper_v_scale * v_direction,
		face_centre + lower_u_scale * u_direction + upper_v_scale * v_direction
	};

	return create_oriented_bounding_box(face_corner_points);
}


GPlatesOpenGL::GLIntersect::OrientedBoundingBox
GPlatesOpenGL::GLCubeSubdivision::create_oriented_bounding_box(
		const GPlatesMaths::Vector3D face_corner_points[])
{
	// Normalise the face corner points.
	const GPlatesMaths::UnitVector3D normalised_face_corner_points[4] =
	{
		face_corner_points[0].get_normalisation(),
		face_corner_points[1].get_normalisation(),
		face_corner_points[2].get_normalisation(),
		face_corner_points[3].get_normalisation()
	};
	// Same but as Vector3D instead of UnitVector3D.
	const GPlatesMaths::Vector3D normalised_face_corner_vectors[4] =
	{
		GPlatesMaths::Vector3D(normalised_face_corner_points[0]),
		GPlatesMaths::Vector3D(normalised_face_corner_points[1]),
		GPlatesMaths::Vector3D(normalised_face_corner_points[2]),
		GPlatesMaths::Vector3D(normalised_face_corner_points[3])
	};

	// Make the average of the four points the z-axis of the OBB.
	const GPlatesMaths::UnitVector3D obb_z_axis =
			(normalised_face_corner_vectors[0] +
				normalised_face_corner_vectors[1] +
				normalised_face_corner_vectors[2] +
				normalised_face_corner_vectors[3]).get_normalisation();

	// To get the bounding box to fit fairly tightly with the subdivision tile we need
	// to align the OBB y-axis roughly with the subdivision tile.
	// So pick two corner points that have same 'u' offset but different 'v' offsets.
	const GPlatesMaths::Vector3D obb_y_axis_unnormalised =
			normalised_face_corner_vectors[3] - normalised_face_corner_vectors[0];

	// The bounding box builder will be used to create the bounding box for the current subdivision tile.
	GLIntersect::OrientedBoundingBoxBuilder bounding_box_builder =
			GLIntersect::create_oriented_bounding_box_builder(obb_y_axis_unnormalised, obb_z_axis);

	// Add the z-axis point since it falls within the subdivision tile and represents the
	// maximum extent along the OBB z-axis.
	bounding_box_builder.add(obb_z_axis);

	// Get the corner points of the subdivision tile on the *sphere*.
	const GPlatesMaths::PointOnSphere sphere_corner_points[4] =
	{
		GPlatesMaths::PointOnSphere(normalised_face_corner_points[0]),
		GPlatesMaths::PointOnSphere(normalised_face_corner_points[1]),
		GPlatesMaths::PointOnSphere(normalised_face_corner_points[2]),
		GPlatesMaths::PointOnSphere(normalised_face_corner_points[3])
	};

	// Now create great circle arcs for the edges of the subdivision tile and add to OBB builder.
	bounding_box_builder.add(
			GPlatesMaths::GreatCircleArc::create(
					sphere_corner_points[0], sphere_corner_points[1]));
	bounding_box_builder.add(
			GPlatesMaths::GreatCircleArc::create(
					sphere_corner_points[1], sphere_corner_points[2]));
	bounding_box_builder.add(
			GPlatesMaths::GreatCircleArc::create(
					sphere_corner_points[2], sphere_corner_points[3]));
	bounding_box_builder.add(
			GPlatesMaths::GreatCircleArc::create(
					sphere_corner_points[3], sphere_corner_points[0]));

	return bounding_box_builder.get_oriented_bounding_box();
}
