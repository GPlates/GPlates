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


#include "CubeCoordinateFrame.h"


namespace GPlatesMaths
{
	namespace CubeCoordinateFrame
	{
		namespace
		{
			/**
			 * The cube face opposite each cube face.
			 */
			const CubeFaceType OPPOSING_CUBE_FACE[NUM_FACES] =
			{
				NEGATIVE_X,
				POSITIVE_X,
				NEGATIVE_Y,
				POSITIVE_Y,
				NEGATIVE_Z,
				POSITIVE_Z
			};


			/**
			 * These directions are the standard directions used by 3D graphics APIs for cube map
			 * textures so we'll adopt the same convention.
			 *
			 * NOTE: I must've gotten these from Direct3D which uses a left-handed coordinate
			 * system so that means we're currently using a left-handed coordinate system.
			 *
			 * FIXME: This will probably cause confusion in some areas so should change this to a
			 * right-handed coordinate system.
			 *
			 * NOTE: This should be kept in sync with @a CUBE_FACE_COORDINATE_TRANSFORMS.
			 */
			const UnitVector3D CUBE_FACE_COORDINATES_FRAMES[NUM_FACES][NUM_AXES] =
			{
				{ UnitVector3D(0,0,-1), UnitVector3D(0,-1,0), UnitVector3D(1,0,0) },
				{ UnitVector3D(0,0,1), UnitVector3D(0,-1,0), UnitVector3D(-1,0,0) },
				{ UnitVector3D(1,0,0), UnitVector3D(0,0,1), UnitVector3D(0,1,0) },
				{ UnitVector3D(1,0,0), UnitVector3D(0,0,-1), UnitVector3D(0,-1,0) },
				{ UnitVector3D(1,0,0), UnitVector3D(0,-1,0), UnitVector3D(0,0,1) },
				{ UnitVector3D(-1,0,0), UnitVector3D(0,-1,0), UnitVector3D(0,0,-1) }
			};


			/**
			 * Used to look up a component of the untransformed vector (in global coord frame).
			 */
			struct CoordinateTransform
			{
				CubeFaceCoordinateFrameAxis component_offset;
				float component_sign;
			};

			/**
			 * Easy way to transform a vector from global coord frame to
			 * the local coord frame of a cube face.
			 *
			 * Avoids a full 3x3 matrix multiply that is not necessary.
			 *
			 * NOTE: This should be kept in sync with @a CUBE_FACE_COORDINATES_FRAMES.
			 */
			const CoordinateTransform CUBE_FACE_COORDINATE_TRANSFORMS[NUM_FACES][NUM_AXES] =
			{
				{ { Z_AXIS, -1 }, { Y_AXIS, -1 }, { X_AXIS,  1 } }, // POSITIVE_X
				{ { Z_AXIS,  1 }, { Y_AXIS, -1 }, { X_AXIS, -1 } }, // NEGATIVE_X
				{ { X_AXIS,  1 }, { Z_AXIS,  1 }, { Y_AXIS,  1 } }, // POSITIVE_Y
				{ { X_AXIS,  1 }, { Z_AXIS, -1 }, { Y_AXIS, -1 } }, // NEGATIVE_Y
				{ { X_AXIS,  1 }, { Y_AXIS, -1 }, { Z_AXIS,  1 } }, // POSITIVE_Z
				{ { X_AXIS, -1 }, { Y_AXIS, -1 }, { Z_AXIS, -1 } }  // NEGATIVE_Z
			};


			/**
			 * The indices of corner points for each face of the cube.
			 *
			 * NOTE: The array is indexed as [cube_face][positive_local_y][positive_local_x].
			 *
			 * NOTE: This should be kept in sync with @a CUBE_FACE_COORDINATE_TRANSFORMS.
			 */
			const cube_corner_index_type CUBE_CORNER_INDICES[NUM_FACES][2][2] =
			{
				{ { 7, 3 }, { 5, 1 } }, // POSITIVE_X
				{ { 2, 6 }, { 0, 4 } }, // NEGATIVE_X
				{ { 2, 3 }, { 6, 7 } }, // POSITIVE_Y
				{ { 4, 5 }, { 0, 1 } }, // NEGATIVE_Y
				{ { 6, 7 }, { 4, 5 } }, // POSITIVE_Z
				{ { 3, 2 }, { 1, 0 } }  // NEGATIVE_Z
			};

			/**
			 * The corner points of the cube as an indexable array.
			 *
			 * NOTE: This should be kept in sync with @a CUBE_FACE_COORDINATE_TRANSFORMS.
			 */
			const Vector3D CUBE_CORNERS[NUM_CUBE_CORNERS] =
			{
				Vector3D(-1,-1,-1),
				Vector3D(+1,-1,-1),
				Vector3D(-1,+1,-1),
				Vector3D(+1,+1,-1),
				Vector3D(-1,-1,+1),
				Vector3D(+1,-1,+1),
				Vector3D(-1,+1,+1),
				Vector3D(+1,+1,+1)
			};

			/**
			 * The projected corner points of the cube, projected onto the sphere, as an indexable array.
			 *
			 * NOTE: This should be kept in sync with @a CUBE_CORNERS.
			 */
			const UnitVector3D PROJECTED_CUBE_CORNERS[NUM_CUBE_CORNERS] =
			{
				Vector3D(-1,-1,-1).get_normalisation(),
				Vector3D(+1,-1,-1).get_normalisation(),
				Vector3D(-1,+1,-1).get_normalisation(),
				Vector3D(+1,+1,-1).get_normalisation(),
				Vector3D(-1,-1,+1).get_normalisation(),
				Vector3D(+1,-1,+1).get_normalisation(),
				Vector3D(-1,+1,+1).get_normalisation(),
				Vector3D(+1,+1,+1).get_normalisation()
			};


			/**
			 * Used to look up a component of the untransformed vector (in global coord frame).
			 */
			struct CubeEdgeInfo
			{
				cube_edge_index_type cube_edge_index;
				bool is_local_axis_direction_opposite_edge_direction;
			};

			/**
			 * The indices of cube edges for each face of the cube.
			 *
			 * NOTE: The array is indexed as [cube_face][is_local_x_axis][positive_orthogonal_axis].
			 *
			 * NOTE: This should be kept in sync with @a CUBE_FACE_COORDINATE_TRANSFORMS.
			 */
			const CubeEdgeInfo CUBE_EDGE_INDICES[NUM_FACES][2][2] =
			{
				{ { { 10,  true }, {  2,  true } }, { {  7,  true }, {  5,  true } } }, // POSITIVE_X
				{ { {  1,  true }, {  9,  true } }, { {  6, false }, {  4, false } } }, // NEGATIVE_X
				{ { {  6, false }, {  7, false } }, { {  3, false }, { 11, false } } }, // POSITIVE_Y
				{ { {  4,  true }, {  5,  true } }, { {  8, false }, {  0, false } } }, // NEGATIVE_Y
				{ { {  9,  true }, { 10,  true } }, { { 11, false }, {  8, false } } }, // POSITIVE_Z
				{ { {  2,  true }, {  1,  true } }, { {  3,  true }, {  0,  true } } }  // NEGATIVE_Z
			};

			/**
			 * The edge directions of the edges of the cube as an indexable array.
			 *
			 * NOTE: This should be kept in sync with @a CUBE_CORNERS.
			 */
			const UnitVector3D CUBE_EDGE_DIRECTIONS[NUM_CUBE_EDGES] =
			{
				UnitVector3D(1, 0, 0), // Edge  0: Corner 0 -> 1
				UnitVector3D(0, 1, 0), // Edge  1: Corner 0 -> 2
				UnitVector3D(0, 1, 0), // Edge  2: Corner 1 -> 3
				UnitVector3D(1, 0, 0), // Edge  3: Corner 2 -> 3
				UnitVector3D(0, 0, 1), // Edge  4: Corner 0 -> 4
				UnitVector3D(0, 0, 1), // Edge  5: Corner 1 -> 5
				UnitVector3D(0, 0, 1), // Edge  6: Corner 2 -> 6
				UnitVector3D(0, 0, 1), // Edge  7: Corner 3 -> 7
				UnitVector3D(1, 0, 0), // Edge  8: Corner 4 -> 5
				UnitVector3D(0, 1, 0), // Edge  9: Corner 4 -> 6
				UnitVector3D(0, 1, 0), // Edge 10: Corner 5 -> 7
				UnitVector3D(1, 0, 0)  // Edge 11: Corner 6 -> 7
			};

			/**
			 * The edge start points as indices into the cube corners.
			 *
			 * NOTE: This should be kept in sync with @a CUBE_CORNERS.
			 */
			const cube_edge_index_type CUBE_EDGE_START_POINTS[NUM_CUBE_EDGES] =
			{
				0, // Edge  0: Corner 0 -> 1
				0, // Edge  1: Corner 0 -> 2
				1, // Edge  2: Corner 1 -> 3
				2, // Edge  3: Corner 2 -> 3
				0, // Edge  4: Corner 0 -> 4
				1, // Edge  5: Corner 1 -> 5
				2, // Edge  6: Corner 2 -> 6
				3, // Edge  7: Corner 3 -> 7
				4, // Edge  8: Corner 4 -> 5
				4, // Edge  9: Corner 4 -> 6
				5, // Edge 10: Corner 5 -> 7
				6  // Edge 11: Corner 6 -> 7
			};

			/**
			 * The edge end points as indices into the cube corners.
			 *
			 * NOTE: This should be kept in sync with @a CUBE_CORNERS.
			 */
			const cube_edge_index_type CUBE_EDGE_END_POINTS[NUM_CUBE_EDGES] =
			{
				1, // Edge  0: Corner 0 -> 1
				2, // Edge  1: Corner 0 -> 2
				3, // Edge  2: Corner 1 -> 3
				3, // Edge  3: Corner 2 -> 3
				4, // Edge  4: Corner 0 -> 4
				5, // Edge  5: Corner 1 -> 5
				6, // Edge  6: Corner 2 -> 6
				7, // Edge  7: Corner 3 -> 7
				5, // Edge  8: Corner 4 -> 5
				6, // Edge  9: Corner 4 -> 6
				7, // Edge 10: Corner 5 -> 7
				7  // Edge 11: Corner 6 -> 7
			};


			/**
			 * Used to transform cube quad tree node locations from one cube face to another.
			 */
			struct CubeQuadTreeNodeLocationTransform
			{
				void
				transform(
						int &transform_to_x_node_offset,
						int &transform_to_y_node_offset,
						unsigned int transform_from_quad_tree_depth,
						unsigned int transform_from_x_node_offset,
						unsigned int transform_from_y_node_offset) const
				{
					// We need to do arithmetic on the node centres but we have node offsets which
					// are zero based, eg, (0, 1, 2, 3) instead of (0.5, 1.5, 2.5, 3.5) which is
					// evenly distributed in the (0,4) range and hence can be added or subtracted
					// as is done in the 3x2 transform below.
					// To achieve this we convert (0, 1, 2, 3) to (1, 3, 5, 7) using "2x+1" which
					// is now evenly distributed in the range (0,8). Then after transformation
					// we just need to divide by two (actually turns out the right-shift operator
					// is what we want and it works for negative numbers).
					const int width = (2 << transform_from_quad_tree_depth); // Double the width.
					const int x = (transform_from_x_node_offset << 1) + 1;
					const int y = (transform_from_y_node_offset << 1) + 1;

					transform_to_x_node_offset = ((x_translation * width + xx * x + xy * y) >> 1);
					transform_to_y_node_offset = ((y_translation * width + yx * x + yy * y) >> 1);
				}

				int x_translation;
				int xx;
				int xy;

				int y_translation;
				int yx;
				int yy;
			};

			/**
			 * Transforms (for cube quad tree node locations) for all combinations of cube face pairs.
			 *
			 * The first array index is the cube face to transform *to*.
			 * The second array index is the cube face to transform *from*.
			 *
			 * NOTE: This should be kept in sync with @a CUBE_FACE_COORDINATES_FRAMES.
			 */
			const CubeQuadTreeNodeLocationTransform CUBE_QUAD_TREE_NODE_LOCATIONS_TRANSFORMS[NUM_FACES][NUM_FACES] =
			{
				{ // POSITIVE_X...
					{ 0, 1, 0, 0, 0, 1 }, // POSITIVE_X
					{ -2, 1, 0, 0, 0, 1 }, // NEGATIVE_X
					{ 1, 0, -1, -1, 1, 0 }, // POSITIVE_Y
					{ 0, 0, 1, 2, -1, 0 }, // NEGATIVE_Y
					{ -1, 1, 0, 0, 0, 1 }, // POSITIVE_Z
					{ 1, 1, 0, 0, 0, 1 } // NEGATIVE_Z
				},
				{ // NEGATIVE_X...
					{ -2, 1, 0, 0, 0, 1 }, // POSITIVE_X
					{ 0, 1, 0, 0, 0, 1 }, // NEGATIVE_X
					{ 0, 0, 1, 0, -1, 0 }, // POSITIVE_Y
					{ 1, 0, -1, 1, 1, 0 }, // NEGATIVE_Y
					{ 1, 1, 0, 0, 0, 1 }, // POSITIVE_Z
					{ -1, 1, 0, 0, 0, 1 } // NEGATIVE_Z
				},
				{ // POSITIVE_Y...
					{ 1, 0, 1, 1, -1, 0 }, // POSITIVE_X
					{ 0, 0, -1, 0, 1, 0 }, // NEGATIVE_X
					{ 0, 1, 0, 0, 0, 1 }, // POSITIVE_Y
					{ 0, 1, 0, -2, 0, 1 }, // NEGATIVE_Y
					{ 0, 1, 0, 1, 0, 1 }, // POSITIVE_Z
					{ 1, -1, 0, 0, 0, -1 } // NEGATIVE_Z
				},
				{ // NEGATIVE_Y...
					{ 2, 0, -1, 0, 1, 0 }, // POSITIVE_X
					{ -1, 0, 1, 1, -1, 0 }, // NEGATIVE_X
					{ 0, 1, 0, -2, 0, 1 }, // POSITIVE_Y
					{ 0, 1, 0, 0, 0, 1 }, // NEGATIVE_Y
					{ 0, 1, 0, -1, 0, 1 }, // POSITIVE_Z
					{ 1, -1, 0, 2, 0, -1 } // NEGATIVE_Z
				},
				{ // POSITIVE_Z...
					{ 1, 1, 0, 0, 0, 1 }, // POSITIVE_X
					{ -1, 1, 0, 0, 0, 1 }, // NEGATIVE_X
					{ 0, 1, 0, -1, 0, 1 }, // POSITIVE_Y
					{ 0, 1, 0, 1, 0, 1 }, // NEGATIVE_Y
					{ 0, 1, 0, 0, 0, 1 }, // POSITIVE_Z
					{ -2, 1, 0, 0, 0, 1 } // NEGATIVE_Z
				},
				{ // NEGATIVE_Z...
					{ -1, 1, 0, 0, 0, 1 }, // POSITIVE_X
					{ 1, 1, 0, 0, 0, 1 }, // NEGATIVE_X
					{ 1, -1, 0, 0, 0, -1 }, // POSITIVE_Y
					{ 1, -1, 0, 2, 0, -1 }, // NEGATIVE_Y
					{ -2, 1, 0, 0, 0, 1 }, // POSITIVE_Z
					{ 0, 1, 0, 0, 0, 1 } // NEGATIVE_Z
				}
			};
		}
	}
}


GPlatesMaths::CubeCoordinateFrame::CubeFaceType
GPlatesMaths::CubeCoordinateFrame::get_cube_face_opposite(
		CubeFaceType cube_face)
{
	return OPPOSING_CUBE_FACE[cube_face];
}


const GPlatesMaths::UnitVector3D &
GPlatesMaths::CubeCoordinateFrame::get_cube_face_coordinate_frame_axis(
		CubeFaceType cube_face,
		CubeFaceCoordinateFrameAxis axis)
{
	return CUBE_FACE_COORDINATES_FRAMES[cube_face][axis];
}


GPlatesMaths::UnitVector3D
GPlatesMaths::CubeCoordinateFrame::transform_into_cube_face_coordinate_frame(
		CubeFaceType cube_face,
		const UnitVector3D &position)
{
	// The coordinates of the position in the global coordinate frame.
	const double position_components[3] =
	{
		position.x().dval(),
		position.y().dval(),
		position.z().dval()
	};

	// Effectively the equivalent of a 3x3 transformation matrix.
	const CoordinateTransform *const cube_face_coord_transform =
			CUBE_FACE_COORDINATE_TRANSFORMS[cube_face];

	// The position in the local coordinate frame of the specified cube face.
	return UnitVector3D(
			position_components[cube_face_coord_transform[X_AXIS].component_offset] *
					cube_face_coord_transform[X_AXIS].component_sign,
			position_components[cube_face_coord_transform[Y_AXIS].component_offset] *
					cube_face_coord_transform[Y_AXIS].component_sign,
			position_components[cube_face_coord_transform[Z_AXIS].component_offset] *
					cube_face_coord_transform[Z_AXIS].component_sign,
			false/*check_validity*/);
}


GPlatesMaths::CubeCoordinateFrame::CubeFaceType
GPlatesMaths::CubeCoordinateFrame::get_cube_face_and_transformed_position(
		const UnitVector3D &position,
		double &position_x_in_cube_face_coordinate_frame,
		double &position_y_in_cube_face_coordinate_frame,
		double &position_z_in_cube_face_coordinate_frame)
{
	const double &position_x = position.x().dval();
	const double &position_y = position.y().dval();
	const double &position_z = position.z().dval();

	// This is much faster than using std::fabsf (according to the profiler).
	// It seems 'std::fabsf' sets the rounding mode of the floating-point unit for
	// each call to 'std::fabsf' which is slow.
	const double abs_circle_centre_x = (position_x > 0) ? position_x : -position_x;
	const double abs_circle_centre_y = (position_y > 0) ? position_y : -position_y;
	const double abs_circle_centre_z = (position_z > 0) ? position_z : -position_z;

	CubeFaceType cube_face;
	if (abs_circle_centre_x > abs_circle_centre_y)
	{
		if (abs_circle_centre_x > abs_circle_centre_z)
		{
			if (position_x > 0)
			{
				cube_face = POSITIVE_X;
				// This is quicker than using 'transform_into_cube_face_coordinate_frame'.
				position_z_in_cube_face_coordinate_frame = position_x;
				position_x_in_cube_face_coordinate_frame = -position_z;
				position_y_in_cube_face_coordinate_frame = -position_y;
			}
			else
			{
				cube_face = NEGATIVE_X;
				// This is quicker than using 'transform_into_cube_face_coordinate_frame'.
				position_z_in_cube_face_coordinate_frame = -position_x;
				position_x_in_cube_face_coordinate_frame = position_z;
				position_y_in_cube_face_coordinate_frame = -position_y;
			}
		}
		else
		{
			if (position_z > 0)
			{
				cube_face = POSITIVE_Z;
				// This is quicker than using 'transform_into_cube_face_coordinate_frame'.
				position_z_in_cube_face_coordinate_frame = position_z;
				position_x_in_cube_face_coordinate_frame = position_x;
				position_y_in_cube_face_coordinate_frame = -position_y;
			}
			else
			{
				cube_face = NEGATIVE_Z;
				// This is quicker than using 'transform_into_cube_face_coordinate_frame'.
				position_z_in_cube_face_coordinate_frame = -position_z;
				position_x_in_cube_face_coordinate_frame = -position_x;
				position_y_in_cube_face_coordinate_frame = -position_y;
			}
		}
	}
	else if (abs_circle_centre_y > abs_circle_centre_z)
	{
		if (position_y > 0)
		{
			cube_face = POSITIVE_Y;
			// This is quicker than using 'transform_into_cube_face_coordinate_frame'.
			position_z_in_cube_face_coordinate_frame = position_y;
			position_x_in_cube_face_coordinate_frame = position_x;
			position_y_in_cube_face_coordinate_frame = position_z;
		}
		else
		{
			cube_face = NEGATIVE_Y;
			// This is quicker than using 'transform_into_cube_face_coordinate_frame'.
			position_z_in_cube_face_coordinate_frame = -position_y;
			position_x_in_cube_face_coordinate_frame = position_x;
			position_y_in_cube_face_coordinate_frame = -position_z;
		}
	}
	else
	{
		if (position_z > 0)
		{
			cube_face = POSITIVE_Z;
			// This is quicker than using 'transform_into_cube_face_coordinate_frame'.
			position_z_in_cube_face_coordinate_frame = position_z;
			position_x_in_cube_face_coordinate_frame = position_x;
			position_y_in_cube_face_coordinate_frame = -position_y;
		}
		else
		{
			cube_face = NEGATIVE_Z;
			// This is quicker than using 'transform_into_cube_face_coordinate_frame'.
			position_z_in_cube_face_coordinate_frame = -position_z;
			position_x_in_cube_face_coordinate_frame = -position_x;
			position_y_in_cube_face_coordinate_frame = -position_y;
		}
	}

	return cube_face;
}


GPlatesMaths::CubeCoordinateFrame::cube_corner_index_type
GPlatesMaths::CubeCoordinateFrame::get_cube_corner_index(
		CubeFaceType cube_face,
		bool positive_x_axis,
		bool positive_y_axis)
{
	return CUBE_CORNER_INDICES[cube_face][positive_y_axis][positive_x_axis];
}


const GPlatesMaths::Vector3D &
GPlatesMaths::CubeCoordinateFrame::get_cube_corner(
		cube_corner_index_type cube_corner_index)
{
	return CUBE_CORNERS[cube_corner_index];
}


const GPlatesMaths::UnitVector3D &
GPlatesMaths::CubeCoordinateFrame::get_projected_cube_corner(
		cube_corner_index_type cube_corner_index)
{
	return PROJECTED_CUBE_CORNERS[cube_corner_index];
}


GPlatesMaths::CubeCoordinateFrame::cube_edge_index_type
GPlatesMaths::CubeCoordinateFrame::get_cube_edge_index(
		CubeFaceType cube_face,
		bool x_axis,
		bool positive_orthogonal_axis,
		bool &reverse_edge_direction)
{
	const CubeEdgeInfo& cube_edge_info = CUBE_EDGE_INDICES[cube_face][x_axis][positive_orthogonal_axis];

	reverse_edge_direction = cube_edge_info.is_local_axis_direction_opposite_edge_direction;
	return cube_edge_info.cube_edge_index;
}


const GPlatesMaths::UnitVector3D &
GPlatesMaths::CubeCoordinateFrame::get_cube_edge_direction(
		cube_edge_index_type cube_edge_index)
{
	return CUBE_EDGE_DIRECTIONS[cube_edge_index];
}


GPlatesMaths::CubeCoordinateFrame::cube_corner_index_type
GPlatesMaths::CubeCoordinateFrame::get_cube_edge_start_point(
		cube_edge_index_type cube_edge_index)
{
	return CUBE_EDGE_START_POINTS[cube_edge_index];
}


GPlatesMaths::CubeCoordinateFrame::cube_corner_index_type
GPlatesMaths::CubeCoordinateFrame::get_cube_edge_end_point(
		cube_edge_index_type cube_edge_index)
{
	return CUBE_EDGE_END_POINTS[cube_edge_index];
}


void
GPlatesMaths::CubeCoordinateFrame::get_cube_quad_tree_node_location_relative_to_cube_face(
		int &transform_to_x_node_offset,
		int &transform_to_y_node_offset,
		CubeFaceType transform_to_cube_face,
		CubeFaceType transform_from_cube_face,
		unsigned int transform_from_quad_tree_depth,
		unsigned int transform_from_x_node_offset,
		unsigned int transform_from_y_node_offset)
{
	CUBE_QUAD_TREE_NODE_LOCATIONS_TRANSFORMS[transform_to_cube_face][transform_from_cube_face].transform(
			transform_to_x_node_offset,
			transform_to_y_node_offset,
			transform_from_quad_tree_depth,
			transform_from_x_node_offset,
			transform_from_y_node_offset);
}
