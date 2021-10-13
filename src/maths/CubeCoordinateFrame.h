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

#ifndef GPLATES_MATHS_CUBECOORDINATEFRAME_H
#define GPLATES_MATHS_CUBECOORDINATEFRAME_H

#include "UnitVector3D.h"


namespace GPlatesMaths
{
	/**
	 * A standard for a local (x,y,z) coordinate frame for each face of a cube.
	 *
	 * A cube coordinate frame is used in a few areas in GPlates such as rendering rasters and
	 * spatial partitions for reconstructed geometries.
	 *
	 * They all need to agree on the coordinate frame used for each cube face if they are to
	 * be used together in any way (eg, finding which polygons cover which parts of a raster).
	 *
	 * Typically the local coordinate frame is used to turn positions on the globe into
	 * offsets within a quad tree level (of a specific cube face) - since in these situations
	 * each cube face will have a quad tree attached. In other words where on the globe (within
	 * the projection of a cube face) does the quad tree node indexed by (0,0), at some level of
	 * the quad tree, map to - this is determined by the 'x' and 'y' axes of the
	 * local coordinate frame of the respective cube face.
	 */
	namespace CubeCoordinateFrame
	{
		/**
		 * Identifies a face of the cube.
		 *
		 * These enum values can be used as indices in your own arrays.
		 */
		enum CubeFaceType
		{
			POSITIVE_X = 0,
			NEGATIVE_X,
			POSITIVE_Y,
			NEGATIVE_Y,
			POSITIVE_Z,
			NEGATIVE_Z,

			NUM_FACES
		};


		/**
		 * Identifies each axis in the *local* coordinate frame of a cube face.
		 *
		 * The 'x' and 'y' axes are parallel to the plane of a cube face (but not in the plane
		 * of the cube face) and the 'z' axis is the cube face normal vector
		 * (pointing outwards from the cube).
		 *
		 * Note, however, that the coordinate frame is still centred at the origin.
		 *
		 * These enum values can be used as indices in your own arrays.
		 */
		enum CubeFaceCoordinateFrameAxis
		{
			X_AXIS = 0,
			Y_AXIS,
			Z_AXIS,

			NUM_AXES
		};


		/**
		 * An index into an array of cube corner points (eight points).
		 */
		typedef unsigned int cube_corner_index_type;

		//! The number of corners in a cube.
		const unsigned int NUM_CUBE_CORNERS = 8;


		/**
		 * An index into an array of cube edges (twelve edges).
		 */
		typedef unsigned int cube_edge_index_type;

		//! The number of edges in a cube.
		const unsigned int NUM_CUBE_EDGES = 12;


		/**
		 * Returns the cube face opposite the specified cube face.
		 */
		CubeFaceType
		get_cube_face_opposite(
				CubeFaceType cube_face);


		/**
		 * Returns the specified axis in the *local* coordinate frame of the specified cube face.
		 *
		 * These directions are the standard directions used by 3D graphics APIs for cube map
		 * textures so we'll adopt the same convention.
		 */
		const UnitVector3D &
		get_cube_face_coordinate_frame_axis(
				CubeFaceType cube_face,
				CubeFaceCoordinateFrameAxis axis);


		/**
		 * Returns the specified position (which is in the global coordinate frame)
		 * to a vector in the local coordinate frame of the specified cube face.
		 *
		 * This is effectively a 3x3 matrix transform - however it's simplified due to the
		 * sparseness of the matrix.
		 */
		UnitVector3D
		transform_into_cube_face_coordinate_frame(
				CubeFaceType cube_face,
				const UnitVector3D &position);


		/**
		 * Determines which cube face the specified position projects into and
		 * returns the position transformed into the local coordinate frame of that cube face.
		 *
		 * The cube face projected onto is determined by the largest component of the specified position.
		 *
		 * The returned transformed vector components can be directly inserted into
		 * a @a UnitVector3D without performing a unit vector validity check.
		 */
		CubeFaceType
		get_cube_face_and_transformed_position(
				const UnitVector3D &position,
				double &position_x_in_cube_face_coordinate_frame,
				double &position_y_in_cube_face_coordinate_frame,
				double &position_z_in_cube_face_coordinate_frame);


		/**
		 * Returns an index that can be used to index into any array of size eight
		 * (representing the eight corner points of the cube).
		 *
		 * Boolean values for @a positive_x_axis and @a positive_y_axis value locate the
		 * corner of the specified cube face and represent the local x/y coordinate frame
		 * of the cube face.
		 */
		cube_corner_index_type
		get_cube_corner_index(
				CubeFaceType cube_face,
				bool positive_x_axis,
				bool positive_y_axis);

		/**
		 * Returns the corner point of the specified cube corner index.
		 *
		 * NOTE: The corner point is *not* on the sphere, it is on the actual cube centred
		 * at the origin (like the sphere) and with a cube face length of two
		 * (because it bounds the unit radius sphere).
		 */
		const Vector3D &
		get_cube_corner(
				cube_corner_index_type cube_corner_index);

		/**
		 * Returns the corner point, projected onto the sphere, of the specified cube corner index.
		 */
		const UnitVector3D &
		get_projected_cube_corner(
				cube_corner_index_type cube_corner_index);


		/**
		 * Returns an index that can be used to index into any array of size twelve
		 * (representing the twelve edges of the cube).
		 *
		 * If @a x_axis is true then the edge is aligned with the local x-axis of the
		 * specified cube face, otherwise it's aligned with the local y-axis.
		 *
		 * @a positive_orthogonal_axis identifies which of the two parallel edges, specified
		 * by @a x_axis, should be used. For example if @a x_axis is true and
		 * @a positive_orthogonal_axis is true then the edge aligned with the x-axis and that
		 * has a positive local y value along the edge is chosen.
		 *
		 * @a reverse_edge_direction is set to true if the returned edge direction is in the
		 * opposite direction to the local axis.
		 */
		cube_edge_index_type
		get_cube_edge_index(
				CubeFaceType cube_face,
				bool x_axis,
				bool positive_orthogonal_axis,
				bool &reverse_edge_direction);

		/**
		 * Returns the edge direction of the specified cube edge index from
		 * the edge start point to the edge end point.
		 */
		const UnitVector3D &
		get_cube_edge_direction(
				cube_edge_index_type cube_edge_index);

		/**
		 * Returns the start point of the edge of the specified cube edge index.
		 */
		cube_corner_index_type
		get_cube_edge_start_point(
				cube_edge_index_type cube_edge_index);

		/**
		 * Returns the end point of the edge of the specified cube edge index.
		 */
		cube_corner_index_type
		get_cube_edge_end_point(
				cube_edge_index_type cube_edge_index);


		/**
		 * Transforms the x and y cube quad tree node offsets from one cube face another.
		 *
		 * This can be visualised by unwrapping the cube faces onto a plane.
		 * Then the node position is determined relative to the coordinate frame of the
		 * the @a transform_to_cube_face cube face.
		 *
		 *     ---
		 *     |2|
		 * ---------
		 * |1|4|0|5|
		 * ---------
		 *     |3|
		 *     ---
		 *
		 * The returned integers are signed instead of unsigned because node offsets can
		 * be negative depending on where the @a transform_from_cube_face cube face is
		 * relative to the @a transform_to_cube_face cube face.
		 *
		 * This is useful when comparing the 'loose' nodes of a spatial partition for intersection
		 * across cube faces.
		 */
		void
		get_cube_quad_tree_node_location_relative_to_cube_face(
				int &transform_to_x_node_offset,
				int &transform_to_y_node_offset,
				CubeFaceType transform_to_cube_face,
				CubeFaceType transform_from_cube_face,
				unsigned int transform_from_quad_tree_depth,
				unsigned int transform_from_x_node_offset,
				unsigned int transform_from_y_node_offset);
	}
}

#endif // GPLATES_MATHS_CUBECOORDINATEFRAME_H
