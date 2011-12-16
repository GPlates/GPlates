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

#include "GLCubeMeshGenerator.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "utils/Base2Utils.h"


GPlatesOpenGL::GLCubeMeshGenerator::GLCubeMeshGenerator(
		unsigned int cube_face_dimension) :
	d_cube_face_dimension(cube_face_dimension)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPlatesUtils::Base2::is_power_of_two(cube_face_dimension),
			GPLATES_ASSERTION_SOURCE);

	// Get the eight corner vertices of the cube.
	const GPlatesMaths::UnitVector3D cube_corner_vertices[GPlatesMaths::CubeCoordinateFrame::NUM_CUBE_CORNERS] =
	{
		GPlatesMaths::CubeCoordinateFrame::get_projected_cube_corner(0),
		GPlatesMaths::CubeCoordinateFrame::get_projected_cube_corner(1),
		GPlatesMaths::CubeCoordinateFrame::get_projected_cube_corner(2),
		GPlatesMaths::CubeCoordinateFrame::get_projected_cube_corner(3),
		GPlatesMaths::CubeCoordinateFrame::get_projected_cube_corner(4),
		GPlatesMaths::CubeCoordinateFrame::get_projected_cube_corner(5),
		GPlatesMaths::CubeCoordinateFrame::get_projected_cube_corner(6),
		GPlatesMaths::CubeCoordinateFrame::get_projected_cube_corner(7)
	};

	// Get the vertices along the twelve edges of the cube.
	// We want to make sure that adjacent cube faces share the same edge vertices to avoid seams
	// being introduced in the rendering due to numerical precision.
	create_cube_edge_vertices(cube_corner_vertices);
}


void
GPlatesOpenGL::GLCubeMeshGenerator::create_cube_face_mesh_vertices(
		std::vector<GPlatesMaths::UnitVector3D> &cube_face_mesh_vertices,
		GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face) const
{
	// Create mesh vertices for the entire cube face.
	create_mesh_vertices(
			cube_face_mesh_vertices,
			cube_face,
			0,
			0,
			get_cube_face_dimension_in_vertex_samples(),
			get_cube_face_dimension_in_vertex_samples());
}


void
GPlatesOpenGL::GLCubeMeshGenerator::create_mesh_vertices(
		std::vector<GPlatesMaths::UnitVector3D> &mesh_vertices,
		GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face,
		unsigned int rect_x_offset,
		unsigned int rect_y_offset,
		unsigned int rect_width_in_samples,
		unsigned int rect_height_in_samples) const
{
	const unsigned int cube_face_dimension_in_vertex_samples = get_cube_face_dimension_in_vertex_samples();

	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			rect_x_offset + rect_width_in_samples <= cube_face_dimension_in_vertex_samples &&
				rect_y_offset + rect_height_in_samples <= cube_face_dimension_in_vertex_samples,
			GPLATES_ASSERTION_SOURCE);

	// Get the local coordinate frame for the specified cube face.
	const GPlatesMaths::UnitVector3D &u_direction =
			GPlatesMaths::CubeCoordinateFrame::get_cube_face_coordinate_frame_axis(
					cube_face,
					GPlatesMaths::CubeCoordinateFrame::X_AXIS/*u*/);
	const GPlatesMaths::UnitVector3D &v_direction =
			GPlatesMaths::CubeCoordinateFrame::get_cube_face_coordinate_frame_axis(
					cube_face,
					GPlatesMaths::CubeCoordinateFrame::Y_AXIS/*v*/);

	const double inv_num_subdivisions = 1.0 / d_cube_face_dimension;

	// Get the cube corner, at local coordinate offset (0,0), of the current cube face.
	const GPlatesMaths::CubeCoordinateFrame::cube_corner_index_type cube_corner_index =
			GPlatesMaths::CubeCoordinateFrame::get_cube_corner_index(
					cube_face,
					// We want the corner at offset (0,0)...
					false/*positive_x_axis*/,
					false/*positive_y_axis*/);
	const GPlatesMaths::Vector3D &cube_corner =
			GPlatesMaths::CubeCoordinateFrame::get_cube_corner(cube_corner_index);

	//
	// Create all the vertices of the current cube face.
	//

	mesh_vertices.reserve(rect_width_in_samples * rect_height_in_samples);

	// Iterate over the vertices of the cube face.
	for (unsigned int y = rect_y_offset; y < rect_y_offset + rect_height_in_samples; ++y)
	{
		// If top edge...
		if (y == 0)
		{
			// Copy the shared edge vertices into our array.
			bool reverse_edge_direction;
			const GPlatesMaths::CubeCoordinateFrame::cube_edge_index_type cube_edge_index =
					get_cube_edge_index(cube_face,
							true/*x_axis*/,
							false/*positive_orthogonal_axis*/,
							reverse_edge_direction);
			if (reverse_edge_direction)
			{
				// Need to reverse edge points as we add them...
				mesh_vertices.insert(
						mesh_vertices.end(),
						d_cube_edge_vertices_array[cube_edge_index].rbegin() + rect_x_offset,
						d_cube_edge_vertices_array[cube_edge_index].rbegin() + rect_x_offset + rect_width_in_samples);
			}
			else
			{
				mesh_vertices.insert(
						mesh_vertices.end(),
						d_cube_edge_vertices_array[cube_edge_index].begin() + rect_x_offset,
						d_cube_edge_vertices_array[cube_edge_index].begin() + rect_x_offset + rect_width_in_samples);
			}

			continue;
		}

		// If bottom edge...
		if (y == cube_face_dimension_in_vertex_samples - 1)
		{
			// Copy the shared edge vertices into our array.
			bool reverse_edge_direction;
			const GPlatesMaths::CubeCoordinateFrame::cube_edge_index_type cube_edge_index =
					get_cube_edge_index(cube_face,
							true/*x_axis*/,
							true/*positive_orthogonal_axis*/,
							reverse_edge_direction);
			if (reverse_edge_direction)
			{
				// Need to reverse edge points as we add them...
				mesh_vertices.insert(
						mesh_vertices.end(),
						d_cube_edge_vertices_array[cube_edge_index].rbegin() + rect_x_offset,
						d_cube_edge_vertices_array[cube_edge_index].rbegin() + rect_x_offset + rect_width_in_samples);
			}
			else
			{
				mesh_vertices.insert(
						mesh_vertices.end(),
						d_cube_edge_vertices_array[cube_edge_index].begin() + rect_x_offset,
						d_cube_edge_vertices_array[cube_edge_index].begin() + rect_x_offset + rect_width_in_samples);
			}

			continue;
		}

		for (unsigned int x = rect_x_offset; x < rect_x_offset + rect_width_in_samples; ++x)
		{
			// If left edge...
			if (x == 0)
			{
				// Copy the shared edge vertex into our array.
				bool reverse_edge_direction;
				const GPlatesMaths::CubeCoordinateFrame::cube_edge_index_type cube_edge_index =
						get_cube_edge_index(cube_face,
								false/*x_axis*/,
								false/*positive_orthogonal_axis*/,
								reverse_edge_direction);
				if (reverse_edge_direction)
				{
					// Need to reverse edge points as we add them...
					mesh_vertices.push_back(
							d_cube_edge_vertices_array
									[cube_edge_index]
									[cube_face_dimension_in_vertex_samples - y - 1]);
				}
				else
				{
					mesh_vertices.push_back(
							d_cube_edge_vertices_array[cube_edge_index][y]);
				}

				continue;
			}

			// If right edge...
			if (x == cube_face_dimension_in_vertex_samples - 1)
			{
				// Copy the shared edge vertex into our array.
				bool reverse_edge_direction;
				const GPlatesMaths::CubeCoordinateFrame::cube_edge_index_type cube_edge_index =
						get_cube_edge_index(cube_face,
								false/*x_axis*/,
								true/*positive_orthogonal_axis*/,
								reverse_edge_direction);
				if (reverse_edge_direction)
				{
					// Need to reverse edge points as we add them...
					mesh_vertices.push_back(
							d_cube_edge_vertices_array
									[cube_edge_index]
									[cube_face_dimension_in_vertex_samples - y - 1]);
				}
				else
				{
					mesh_vertices.push_back(
							d_cube_edge_vertices_array[cube_edge_index][y]);
				}

				continue;
			}

			//
			// It's a vertex interior to the cube face (not an edge vertex)...
			//

			const GPlatesMaths::UnitVector3D vertex_position =
					(cube_corner +
							x * 2.0 * inv_num_subdivisions * u_direction +
							y * 2.0 * inv_num_subdivisions * v_direction)
									.get_normalisation();

			mesh_vertices.push_back(vertex_position);
		}
	}
}


void
GPlatesOpenGL::GLCubeMeshGenerator::create_cube_edge_vertices(
		const GPlatesMaths::UnitVector3D cube_corner_vertices[])
{
	// Iterate over the cube edges.
	for (GPlatesMaths::CubeCoordinateFrame::cube_edge_index_type cube_edge_index = 0;
		cube_edge_index < GPlatesMaths::CubeCoordinateFrame::NUM_CUBE_EDGES;
		++cube_edge_index)
	{
		const unsigned int num_vertices_per_cube_face_size = d_cube_face_dimension + 1;

		std::vector<GPlatesMaths::UnitVector3D> &cube_edge_vertices = d_cube_edge_vertices_array[cube_edge_index];
		cube_edge_vertices.reserve(num_vertices_per_cube_face_size);

		// Get the edge start/end points and direction.
		const GPlatesMaths::CubeCoordinateFrame::cube_corner_index_type edge_start_point_cube_corner_index =
				GPlatesMaths::CubeCoordinateFrame::get_cube_edge_start_point(cube_edge_index);
		const GPlatesMaths::CubeCoordinateFrame::cube_corner_index_type edge_end_point_cube_corner_index =
				GPlatesMaths::CubeCoordinateFrame::get_cube_edge_end_point(cube_edge_index);
		const GPlatesMaths::Vector3D &edge_start_point =
				GPlatesMaths::CubeCoordinateFrame::get_cube_corner(edge_start_point_cube_corner_index);
		const GPlatesMaths::UnitVector3D &edge_direction =
				GPlatesMaths::CubeCoordinateFrame::get_cube_edge_direction(cube_edge_index);

		const double inv_num_subdivisions = 1.0 / d_cube_face_dimension;

		// Iterate over the current edge vertices.
		for (unsigned int n = 0; n < num_vertices_per_cube_face_size; ++n)
		{
			// If cube corner point then share with other edges...
			if (n == 0)
			{
				cube_edge_vertices.push_back(cube_corner_vertices[edge_start_point_cube_corner_index]);
				continue;
			}

			// If cube corner point then share with other edges...
			if (n == num_vertices_per_cube_face_size - 1)
			{
				cube_edge_vertices.push_back(cube_corner_vertices[edge_end_point_cube_corner_index]);
				continue;
			}

			// Generate the interior edge vertex position.
			const GPlatesMaths::UnitVector3D edge_vertex_position =
					(edge_start_point + n * 2.0 * inv_num_subdivisions * edge_direction).get_normalisation();

			cube_edge_vertices.push_back(edge_vertex_position);
		}
	}
}
