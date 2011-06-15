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

#include <vector>
#include <boost/static_assert.hpp>

#include "GLMultiResolutionCubeMesh.h"

#include "GLUtils.h"
#include "GLVertexArrayDrawable.h"

#include "global/AssertionFailureException.h"
#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "utils/Profile.h"


GPlatesOpenGL::GLMultiResolutionCubeMesh::GLMultiResolutionCubeMesh(
		const GLTextureResourceManager::shared_ptr_type &texture_resource_manager,
		const GLVertexBufferResourceManager::shared_ptr_type &vertex_buffer_resource_manager) :
	d_texture_resource_manager(texture_resource_manager),
	d_vertex_buffer_resource_manager(vertex_buffer_resource_manager),
	d_xy_clip_texture(GLTextureUtils::create_xy_clip_texture(texture_resource_manager)),
	d_mesh_cube_quad_tree(mesh_cube_quad_tree_type::create())
{
	create_mesh_drawables();
}


GPlatesOpenGL::GLMultiResolutionCubeMesh::QuadTreeNode
GPlatesOpenGL::GLMultiResolutionCubeMesh::get_quad_tree_root_node(
		GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face) const
{
	const mesh_cube_quad_tree_type::node_type *root_node =
			d_mesh_cube_quad_tree->get_quad_tree_root_node(cube_face);

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			root_node,
			GPLATES_ASSERTION_SOURCE);

	return QuadTreeNode(*root_node);
}


GPlatesOpenGL::GLMultiResolutionCubeMesh::QuadTreeNode
GPlatesOpenGL::GLMultiResolutionCubeMesh::get_child_node(
		const QuadTreeNode &parent_node,
		unsigned int child_x_offset,
		unsigned int child_y_offset) const
{
	if (parent_node.d_mesh_node)
	{
		const mesh_cube_quad_tree_type::node_type *child_node =
				parent_node.d_mesh_node->get_child_node(child_x_offset, child_y_offset);
		if (!child_node)
		{
			// We've just reached the maximum depth of our mesh cube quad tree.
			// Propagate the parent mesh drawable and start a non-identity clip space transform
			// to compensate.
			return QuadTreeNode(
					parent_node.d_mesh_drawable,
					GLUtils::QuadTreeClipSpaceTransform(
							child_x_offset,
							child_y_offset));
		}

		return QuadTreeNode(*child_node);
	}

	// We're deeper into the cube quad tree than our pre-generated mesh tree so just continue to
	// propagate the parent mesh drawable and adjust the child's clip space transform to compensate.
	return QuadTreeNode(
			parent_node.d_mesh_drawable,
			GLUtils::QuadTreeClipSpaceTransform(
					parent_node.d_clip_space_transform.get(),
					child_x_offset,
					child_y_offset));
}


void
GPlatesOpenGL::GLMultiResolutionCubeMesh::create_mesh_drawables()
{
	PROFILE_FUNC();

	// Get the eight corner vertices of the cube.
	const GLVertex cube_corner_vertices[GPlatesMaths::CubeCoordinateFrame::NUM_CUBE_CORNERS] =
	{
		GLVertex(GPlatesMaths::CubeCoordinateFrame::get_projected_cube_corner(0)),
		GLVertex(GPlatesMaths::CubeCoordinateFrame::get_projected_cube_corner(1)),
		GLVertex(GPlatesMaths::CubeCoordinateFrame::get_projected_cube_corner(2)),
		GLVertex(GPlatesMaths::CubeCoordinateFrame::get_projected_cube_corner(3)),
		GLVertex(GPlatesMaths::CubeCoordinateFrame::get_projected_cube_corner(4)),
		GLVertex(GPlatesMaths::CubeCoordinateFrame::get_projected_cube_corner(5)),
		GLVertex(GPlatesMaths::CubeCoordinateFrame::get_projected_cube_corner(6)),
		GLVertex(GPlatesMaths::CubeCoordinateFrame::get_projected_cube_corner(7))
	};

	// Get the vertices along the twelve edges of the cube.
	// We want to make sure that adjacent cube faces share the same edge vertices to avoid seams
	// being introduced in the rendering due to numerical precision.
	std::vector<GLVertex> cube_edge_vertices_array[GPlatesMaths::CubeCoordinateFrame::NUM_CUBE_EDGES];
	create_cube_edge_vertices(cube_edge_vertices_array, cube_corner_vertices);

	// Iterate over the cube faces and generate the mesh vertices for each face.
	for (unsigned int face = 0; face < 6; ++face)
	{
		const GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face =
				static_cast<GPlatesMaths::CubeCoordinateFrame::CubeFaceType>(face);

		// Create all mesh vertices for the current cube face.
		std::vector<GLVertex> unique_cube_face_mesh_vertices;
		create_cube_face_mesh_vertices(
				unique_cube_face_mesh_vertices,
				cube_face,
				cube_edge_vertices_array);

		// Create the vertex array and vertex element array for the current cube face by
		// storing vertices/indices in quad tree traversal order.
		create_cube_face_vertex_and_index_array(
				cube_face,
				unique_cube_face_mesh_vertices);

		// Do another quad tree traversal to create an individual vertex element array for
		// each quad tree node (and drawable to wrap it in).
		create_quad_tree_mesh_drawables(cube_face);
	}
}


void
GPlatesOpenGL::GLMultiResolutionCubeMesh::create_cube_edge_vertices(
		std::vector<GLVertex> cube_edge_vertices_array[],
		const GLVertex cube_corner_vertices[])
{
	// Iterate over the cube edges.
	for (GPlatesMaths::CubeCoordinateFrame::cube_edge_index_type cube_edge_index = 0;
		cube_edge_index < GPlatesMaths::CubeCoordinateFrame::NUM_CUBE_EDGES;
		++cube_edge_index)
	{
		std::vector<GLVertex> &cube_edge_vertices = cube_edge_vertices_array[cube_edge_index];
		cube_edge_vertices.reserve(MESH_MAXIMUM_VERTICES_PER_CUBE_FACE_SIDE);

		// Get the edge start/end points and direction.
		const GPlatesMaths::CubeCoordinateFrame::cube_corner_index_type edge_start_point_cube_corner_index =
				GPlatesMaths::CubeCoordinateFrame::get_cube_edge_start_point(cube_edge_index);
		const GPlatesMaths::CubeCoordinateFrame::cube_corner_index_type edge_end_point_cube_corner_index =
				GPlatesMaths::CubeCoordinateFrame::get_cube_edge_end_point(cube_edge_index);
		const GPlatesMaths::Vector3D &edge_start_point =
				GPlatesMaths::CubeCoordinateFrame::get_cube_corner(edge_start_point_cube_corner_index);
		const GPlatesMaths::UnitVector3D &edge_direction =
				GPlatesMaths::CubeCoordinateFrame::get_cube_edge_direction(cube_edge_index);

		const double inv_num_subdivisions = 1.0 / MESH_MAXIMUM_TILES_PER_CUBE_FACE_SIDE;

		// Iterate over the current edge vertices.
		for (unsigned int n = 0; n < MESH_MAXIMUM_VERTICES_PER_CUBE_FACE_SIDE; ++n)
		{
			// If cube corner point then share with other edges...
			if (n == 0)
			{
				cube_edge_vertices.push_back(cube_corner_vertices[edge_start_point_cube_corner_index]);
				continue;
			}

			// If cube corner point then share with other edges...
			if (n == MESH_MAXIMUM_VERTICES_PER_CUBE_FACE_SIDE - 1)
			{
				cube_edge_vertices.push_back(cube_corner_vertices[edge_end_point_cube_corner_index]);
				continue;
			}

			// Generate the interior edge vertex position.
			const GPlatesMaths::UnitVector3D edge_vertex_position =
					(edge_start_point +
							n * 2.0 * inv_num_subdivisions * edge_direction)
									.get_normalisation();

			cube_edge_vertices.push_back(GLVertex(edge_vertex_position));
		}
	}
}


void
GPlatesOpenGL::GLMultiResolutionCubeMesh::create_cube_face_mesh_vertices(
		std::vector<GLVertex> &cube_face_mesh_vertices,
		GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face,
		const std::vector<GLVertex> cube_edge_vertices[])
{
	// Get the local coordinate frame for the current cube face.
	const GPlatesMaths::UnitVector3D &u_direction =
			GPlatesMaths::CubeCoordinateFrame::get_cube_face_coordinate_frame_axis(
					cube_face,
					GPlatesMaths::CubeCoordinateFrame::X_AXIS/*u*/);
	const GPlatesMaths::UnitVector3D &v_direction =
			GPlatesMaths::CubeCoordinateFrame::get_cube_face_coordinate_frame_axis(
					cube_face,
					GPlatesMaths::CubeCoordinateFrame::Y_AXIS/*v*/);

	const double inv_num_subdivisions = 1.0 / MESH_MAXIMUM_TILES_PER_CUBE_FACE_SIDE;

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

	cube_face_mesh_vertices.reserve(
			MESH_MAXIMUM_VERTICES_PER_CUBE_FACE_SIDE * MESH_MAXIMUM_VERTICES_PER_CUBE_FACE_SIDE);

	// Iterate over the vertices of the cube face.
	for (unsigned int y = 0; y < MESH_MAXIMUM_VERTICES_PER_CUBE_FACE_SIDE; ++y)
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
				cube_face_mesh_vertices.insert(
						cube_face_mesh_vertices.end(),
						cube_edge_vertices[cube_edge_index].rbegin(),
						cube_edge_vertices[cube_edge_index].rend());
			}
			else
			{
				cube_face_mesh_vertices.insert(
						cube_face_mesh_vertices.end(),
						cube_edge_vertices[cube_edge_index].begin(),
						cube_edge_vertices[cube_edge_index].end());
			}

			continue;
		}

		// If bottom edge...
		if (y == MESH_MAXIMUM_VERTICES_PER_CUBE_FACE_SIDE - 1)
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
				cube_face_mesh_vertices.insert(
						cube_face_mesh_vertices.end(),
						cube_edge_vertices[cube_edge_index].rbegin(),
						cube_edge_vertices[cube_edge_index].rend());
			}
			else
			{
				cube_face_mesh_vertices.insert(
						cube_face_mesh_vertices.end(),
						cube_edge_vertices[cube_edge_index].begin(),
						cube_edge_vertices[cube_edge_index].end());
			}

			continue;
		}

		for (unsigned int x = 0; x < MESH_MAXIMUM_VERTICES_PER_CUBE_FACE_SIDE; ++x)
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
					cube_face_mesh_vertices.push_back(
							cube_edge_vertices
									[cube_edge_index]
									[MESH_MAXIMUM_VERTICES_PER_CUBE_FACE_SIDE - y - 1]);
				}
				else
				{
					cube_face_mesh_vertices.push_back(
							cube_edge_vertices[cube_edge_index][y]);
				}

				continue;
			}

			// If right edge...
			if (x == MESH_MAXIMUM_VERTICES_PER_CUBE_FACE_SIDE - 1)
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
					cube_face_mesh_vertices.push_back(
							cube_edge_vertices
									[cube_edge_index]
									[MESH_MAXIMUM_VERTICES_PER_CUBE_FACE_SIDE - y - 1]);
				}
				else
				{
					cube_face_mesh_vertices.push_back(
							cube_edge_vertices[cube_edge_index][y]);
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

			cube_face_mesh_vertices.push_back(GLVertex(vertex_position));
		}
	}
}

//for boost assert
DISABLE_GCC_WARNING("-Wold-style-cast")

void
GPlatesOpenGL::GLMultiResolutionCubeMesh::create_cube_face_vertex_and_index_array(
		GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face,
		const std::vector<GLVertex> &unique_cube_face_mesh_vertices)
{
	// Each quad tree tile (at maximum depth) will contain four vertices.
	// This is a duplication of the unique cube face vertices by a factor of four but due to
	// our quad tree storage of vertices we get better vertex array locality and we can specify
	// a smaller (local) range of vertices for the graphics card to process.
	// NOTE: Not that any of this would make much difference on todays graphics cards so really
	// the fact that it's easier to program (with quad tree traversal) is probably the main gain.
	const unsigned int num_mesh_vertices =
			4 * MESH_MAXIMUM_TILES_PER_CUBE_FACE_SIDE * MESH_MAXIMUM_TILES_PER_CUBE_FACE_SIDE;
	const unsigned int num_mesh_indices =
			6 /*two triangles*/ * MESH_MAXIMUM_TILES_PER_CUBE_FACE_SIDE * MESH_MAXIMUM_TILES_PER_CUBE_FACE_SIDE;
	std::vector<GLVertex> mesh_vertices;
	mesh_vertices.reserve(num_mesh_vertices);
	// We're using 'GLushort' vertex indices which are 16-bit - make sure we don't overflow them.
	// 16-bit indices are faster than 32-bit for graphics cards (but again probably not much gain).
	BOOST_STATIC_ASSERT(num_mesh_vertices <= (1 << 16));
	std::vector<GLushort> mesh_indices;
	mesh_indices.reserve(num_mesh_indices);

	// Keep track of the quad tree location as we traverse so we know which vertices belong
	// to which quad tree nodes.
	const GPlatesMaths::CubeQuadTreeLocation root_node_location(cube_face);
	create_cube_face_vertex_and_index_array(
			mesh_vertices,
			mesh_indices,
			unique_cube_face_mesh_vertices,
			root_node_location);

	// Create a single OpenGL vertex array for the current cube face to contain the vertices
	// of *all* meshes.
	d_meshes_vertex_array[cube_face] = GLVertexArray::create(
			mesh_vertices,
			GLArray::USAGE_STATIC,
			d_vertex_buffer_resource_manager);

	// Create a single OpenGL vertex array for the current cube face to contain the
	// vertex indices of *all* meshes.
	// Note that this vertex element array is never used to draw - it just contains the indices.
	// Each mesh drawable will creates its own vertex element array that shares the indices of this array.
	d_meshes_vertex_element_array[cube_face] = GLVertexElementArray::create(
					mesh_indices,
					GLArray::USAGE_STATIC,
					d_vertex_buffer_resource_manager);
}


void
GPlatesOpenGL::GLMultiResolutionCubeMesh::create_cube_face_vertex_and_index_array(
		std::vector<GLVertex> &mesh_vertices,
		std::vector<GLushort> &mesh_indices,
		const std::vector<GLVertex> &unique_cube_face_mesh_vertices,
		const GPlatesMaths::CubeQuadTreeLocation &quad_tree_node_location)
{
	// We only generate the vertices at the leaf nodes of the quad tree.
	if (quad_tree_node_location.get_node_location()->quad_tree_depth == MESH_CUBE_QUAD_TREE_MAXIMUM_DEPTH)
	{
		const GLushort base_vertex_index = mesh_vertices.size();

		const unsigned int node_x_offset = quad_tree_node_location.get_node_location()->x_node_offset;
		const unsigned int node_y_offset = quad_tree_node_location.get_node_location()->y_node_offset;

		//
		// The two triangles of the quad covering the current quad tree leaf node look like:
		//
		// 0-1
		// |/|
		// 2-3
		//

		// Vertex 0...
		mesh_vertices.push_back(
				unique_cube_face_mesh_vertices[
						node_y_offset * MESH_MAXIMUM_VERTICES_PER_CUBE_FACE_SIDE + node_x_offset]);
		// Vertex 1...
		mesh_vertices.push_back(
				unique_cube_face_mesh_vertices[
						node_y_offset * MESH_MAXIMUM_VERTICES_PER_CUBE_FACE_SIDE + node_x_offset + 1]);
		// Vertex 2...
		mesh_vertices.push_back(
				unique_cube_face_mesh_vertices[
						(node_y_offset + 1) * MESH_MAXIMUM_VERTICES_PER_CUBE_FACE_SIDE + node_x_offset]);
		// Vertex 3...
		mesh_vertices.push_back(
				unique_cube_face_mesh_vertices[
						(node_y_offset + 1) * MESH_MAXIMUM_VERTICES_PER_CUBE_FACE_SIDE + node_x_offset + 1]);

		// First triangle of quad.
		mesh_indices.push_back(base_vertex_index);
		mesh_indices.push_back(base_vertex_index + 1);
		mesh_indices.push_back(base_vertex_index + 2);

		// Second triangle of quad.
		mesh_indices.push_back(base_vertex_index + 3);
		mesh_indices.push_back(base_vertex_index + 2);
		mesh_indices.push_back(base_vertex_index + 1);

		return;
	}

	//
	// Iterate over the child quad tree nodes.
	//

	for (unsigned int child_y_offset = 0; child_y_offset < 2; ++child_y_offset)
	{
		for (unsigned int child_x_offset = 0; child_x_offset < 2; ++child_x_offset)
		{
			const GPlatesMaths::CubeQuadTreeLocation child_quad_tree_node_location(
					quad_tree_node_location,
					child_x_offset,
					child_y_offset);

			// Recurse into the child node.
			create_cube_face_vertex_and_index_array(
					mesh_vertices,
					mesh_indices,
					unique_cube_face_mesh_vertices,
					child_quad_tree_node_location);
		}
	}
}


void
GPlatesOpenGL::GLMultiResolutionCubeMesh::create_quad_tree_mesh_drawables(
		GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face)
{
	unsigned int vertex_index = 0;
	unsigned int vertex_element_index = 0;

	// Generate the mesh drawables quad tree for the current cube face.
	mesh_cube_quad_tree_type::node_type::ptr_type root_mesh_quad_tree_node =
			create_quad_tree_mesh_drawables(
					vertex_index,
					vertex_element_index,
					cube_face,
					0/*depth*/);

	// Add the root node to the quad tree.
	d_mesh_cube_quad_tree->set_quad_tree_root_node(
			cube_face,
			root_mesh_quad_tree_node);
}


GPlatesOpenGL::GLMultiResolutionCubeMesh::mesh_cube_quad_tree_type::node_type::ptr_type
GPlatesOpenGL::GLMultiResolutionCubeMesh::create_quad_tree_mesh_drawables(
		unsigned int &vertex_index,
		unsigned int &vertex_element_index,
		GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face,
		unsigned int depth)
{
	const unsigned int base_vertex_index = vertex_index;
	const unsigned int base_vertex_element_index = vertex_element_index;

	mesh_cube_quad_tree_type::node_type::ptr_type child_mesh_quad_tree_nodes[2][2];

	if (depth == MESH_CUBE_QUAD_TREE_MAXIMUM_DEPTH)
	{
		// There are four vertices for the quad covering this leaf quad tree node.
		vertex_index += 4;
		// There are two triangles (six vertex indices) for the quad covering this leaf quad tree node.
		vertex_element_index += 6;
	}
	else
	{
		//
		// Iterate over the child quad tree nodes.
		//

		for (unsigned int child_y_offset = 0; child_y_offset < 2; ++child_y_offset)
		{
			for (unsigned int child_x_offset = 0; child_x_offset < 2; ++child_x_offset)
			{
				// Recurse into the child node.
				child_mesh_quad_tree_nodes[child_y_offset][child_x_offset] =
						create_quad_tree_mesh_drawables(
								vertex_index,
								vertex_element_index,
								cube_face,
								depth + 1);
			}
		}
	}

	// Create a vertex element array that shares the same indices as the other meshes in the current cube face.
	const GLVertexElementArray::shared_ptr_type mesh_vertex_element_array =
			GLVertexElementArray::create(
					d_meshes_vertex_element_array[cube_face]->get_array_data(),
					GL_UNSIGNED_SHORT);

	// Specify what to draw for the current quad tree node mesh.
	// The mesh covers all descendants of this quad tree node.
	mesh_vertex_element_array->gl_draw_range_elements_EXT(
			GL_TRIANGLES,
			base_vertex_index/*start*/,
			vertex_index - 1/*end*/,
			vertex_element_index - base_vertex_element_index/*count*/,
			sizeof(GLushort) * base_vertex_element_index/*indices_offset*/);

	// Create the drawable.
	GLVertexArrayDrawable::non_null_ptr_type mesh_drawable =
			GLVertexArrayDrawable::create(
					d_meshes_vertex_array[cube_face],
					mesh_vertex_element_array);

	// Create a quad tree node.
	mesh_cube_quad_tree_type::node_type::ptr_type mesh_quad_tree_node =
			d_mesh_cube_quad_tree->create_node(
					MeshQuadTreeNode(mesh_drawable));

	// Add the child nodes if we visited any.
	if (depth < MESH_CUBE_QUAD_TREE_MAXIMUM_DEPTH)
	{
		for (unsigned int child_y_offset = 0; child_y_offset < 2; ++child_y_offset)
		{
			for (unsigned int child_x_offset = 0; child_x_offset < 2; ++child_x_offset)
			{
				// Add the child node.
				d_mesh_cube_quad_tree->set_child_node(
						*mesh_quad_tree_node,
						child_x_offset,
						child_y_offset,
						child_mesh_quad_tree_nodes[child_y_offset][child_x_offset]);
			}
		}
	}

	return mesh_quad_tree_node;
}

ENABLE_GCC_WARNING("-Wold-style-cast")

