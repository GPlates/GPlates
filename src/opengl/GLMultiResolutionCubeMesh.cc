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

#include "GLCubeMeshGenerator.h"
#include "GLRenderer.h"
#include "GLUtils.h"

#include "global/AssertionFailureException.h"
#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "utils/Profile.h"


GPlatesOpenGL::GLMultiResolutionCubeMesh::GLMultiResolutionCubeMesh(
		GLRenderer &renderer) :
	d_xy_clip_texture(GLTextureUtils::create_xy_clip_texture_2D(renderer)),
	d_mesh_cube_quad_tree(mesh_cube_quad_tree_type::create())
{
	create_mesh_drawables(renderer);
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
					*parent_node.d_mesh_drawable,
					GLUtils::QuadTreeClipSpaceTransform(
							GLUtils::QuadTreeClipSpaceTransform()/*Identity transform*/,
							child_x_offset,
							child_y_offset));
		}

		return QuadTreeNode(*child_node);
	}

	// We're deeper into the cube quad tree than our pre-generated mesh tree so just continue to
	// propagate the parent mesh drawable and adjust the child's clip space transform to compensate.
	return QuadTreeNode(
			*parent_node.d_mesh_drawable,
			GLUtils::QuadTreeClipSpaceTransform(
					parent_node.d_clip_space_transform.get(),
					child_x_offset,
					child_y_offset));
}


void
GPlatesOpenGL::GLMultiResolutionCubeMesh::create_mesh_drawables(
		GLRenderer &renderer)
{
	PROFILE_FUNC();

	// Generates the mesh vertices.
	const GLCubeMeshGenerator cube_mesh_generator(MESH_MAXIMUM_TILES_PER_CUBE_FACE_SIDE);

	// Iterate over the cube faces and generate the mesh vertices for each face.
	for (unsigned int face = 0; face < 6; ++face)
	{
		const GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face =
				static_cast<GPlatesMaths::CubeCoordinateFrame::CubeFaceType>(face);

		// Create all mesh vertices for the current cube face.
		std::vector<GPlatesMaths::UnitVector3D> unique_cube_face_mesh_vertices;
		cube_mesh_generator.create_cube_face_mesh_vertices(unique_cube_face_mesh_vertices, cube_face);

		// Create the vertex array and vertex element array for the current cube face by
		// storing vertices/indices in quad tree traversal order.
		create_cube_face_vertex_and_index_array(
				renderer,
				cube_face,
				unique_cube_face_mesh_vertices);

		// Do another quad tree traversal to create an individual vertex element array for
		// each quad tree node (and drawable to wrap it in).
		create_quad_tree_mesh_drawables(renderer, cube_face);
	}
}


//for boost assert
DISABLE_GCC_WARNING("-Wold-style-cast")

void
GPlatesOpenGL::GLMultiResolutionCubeMesh::create_cube_face_vertex_and_index_array(
		GLRenderer &renderer,
		GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face,
		const std::vector<GPlatesMaths::UnitVector3D> &unique_cube_face_mesh_vertices)
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
	// If we're using 'GLushort' vertex indices which are 16-bit - make sure we don't overflow them.
	// 16-bit indices are faster than 32-bit for graphics cards (but again probably not much gain).
	BOOST_STATIC_ASSERT(sizeof(vertex_element_type) >= 4 ||
		num_mesh_vertices <= (1 << (8/*bits-per-char*/ * sizeof(vertex_element_type))));
	std::vector<vertex_element_type> mesh_indices;
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
	// (and vertex elements or indices) of *all* meshes.
	GLVertexArray::shared_ptr_type vertex_array = GLVertexArray::create(renderer);
	// Store the vertices/indices in a new vertex buffer and vertex element buffer that is then
	// bound to the vertex array.
	set_vertex_array_data(renderer, *vertex_array, mesh_vertices, mesh_indices);
	d_meshes_vertex_array[cube_face] = vertex_array;
}


void
GPlatesOpenGL::GLMultiResolutionCubeMesh::create_cube_face_vertex_and_index_array(
		std::vector<GLVertex> &mesh_vertices,
		std::vector<vertex_element_type> &mesh_indices,
		const std::vector<GPlatesMaths::UnitVector3D> &unique_cube_face_mesh_vertices,
		const GPlatesMaths::CubeQuadTreeLocation &quad_tree_node_location)
{
	// We only generate the vertices at the leaf nodes of the quad tree.
	if (quad_tree_node_location.get_node_location()->quad_tree_depth == MESH_CUBE_QUAD_TREE_MAXIMUM_DEPTH)
	{
		const vertex_element_type base_vertex_index = mesh_vertices.size();

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
				GLVertex(
						unique_cube_face_mesh_vertices[
								node_y_offset * MESH_MAXIMUM_VERTICES_PER_CUBE_FACE_SIDE + node_x_offset]));
		// Vertex 1...
		mesh_vertices.push_back(
				GLVertex(
						unique_cube_face_mesh_vertices[
								node_y_offset * MESH_MAXIMUM_VERTICES_PER_CUBE_FACE_SIDE + node_x_offset + 1]));
		// Vertex 2...
		mesh_vertices.push_back(
				GLVertex(
						unique_cube_face_mesh_vertices[
								(node_y_offset + 1) * MESH_MAXIMUM_VERTICES_PER_CUBE_FACE_SIDE + node_x_offset]));
		// Vertex 3...
		mesh_vertices.push_back(
				GLVertex(
						unique_cube_face_mesh_vertices[
								(node_y_offset + 1) * MESH_MAXIMUM_VERTICES_PER_CUBE_FACE_SIDE + node_x_offset + 1]));

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
		GLRenderer &renderer,
		GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face)
{
	unsigned int vertex_index = 0;
	unsigned int vertex_element_index = 0;

	// Generate the mesh drawables quad tree for the current cube face.
	mesh_cube_quad_tree_type::node_type::ptr_type root_mesh_quad_tree_node =
			create_quad_tree_mesh_drawables(
					renderer,
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
		GLRenderer &renderer,
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
								renderer,
								vertex_index,
								vertex_element_index,
								cube_face,
								depth + 1);
			}
		}
	}

	// Specify what to draw for the current quad tree node mesh.
	// The mesh covers all descendants of this quad tree node.
	const MeshDrawable mesh_drawable =
	{
		d_meshes_vertex_array[cube_face]/*vertex_array*/,
		base_vertex_index/*start*/,
		vertex_index - 1/*end*/,
		static_cast<GLsizei>(vertex_element_index - base_vertex_element_index)/*count*/,
                static_cast<GLint>(sizeof(vertex_element_type) * base_vertex_element_index)/*indices_offset*/
	};

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


void
GPlatesOpenGL::GLMultiResolutionCubeMesh::QuadTreeNode::render_mesh_drawable(
		GLRenderer &renderer) const
{
	// Bind the vertex array.
	d_mesh_drawable->vertex_array->gl_bind(renderer);

	// Draw the vertex array.
	d_mesh_drawable->vertex_array->gl_draw_range_elements(
			renderer,
			GL_TRIANGLES,
			d_mesh_drawable->start,
			d_mesh_drawable->end,
			d_mesh_drawable->count,
			GLVertexElementTraits<vertex_element_type>::type,
			d_mesh_drawable->indices_offset);
}
