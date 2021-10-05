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

#include <opengl/OpenGL3.h>  // Should be included at TOP of ".cc" file.

#include <limits>
#include <vector>
#include <boost/optional.hpp>
#include <boost/static_assert.hpp>

#include "GLMultiResolutionMapCubeMesh.h"

#include "GL.h"
#include "GLIntersectPrimitives.h"
#include "GLUtils.h"
#include "GLVertexUtils.h"

#include "maths/CubeCoordinateFrame.h"

#include "global/AssertionFailureException.h"
#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "utils/Base2Utils.h"
#include "utils/Profile.h"


GPlatesOpenGL::GLMultiResolutionMapCubeMesh::GLMultiResolutionMapCubeMesh(
		GL &gl,
		const GPlatesGui::MapProjection &map_projection) :
	d_xy_clip_texture(GLTextureUtils::create_xy_clip_texture_2D(gl)),
	d_mesh_cube_quad_tree(mesh_cube_quad_tree_type::create()),
	d_map_projection_settings(map_projection.get_projection_settings())
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			GPlatesUtils::Base2::is_power_of_two(CUBE_FACE_DIMENSION),
			GPLATES_ASSERTION_SOURCE);

	// The cube quad tree depth cannot exceed that supported by the number of vertices.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			CUBE_FACE_DIMENSION >= (1 << MESH_CUBE_QUAD_TREE_MAXIMUM_DEPTH),
			GPLATES_ASSERTION_SOURCE);

	create_mesh(gl, map_projection);
}


bool
GPlatesOpenGL::GLMultiResolutionMapCubeMesh::update_map_projection(
		GL &gl,
		const GPlatesGui::MapProjection &map_projection)
{
	// Nothing to do if the map projection settings are the same as last time.
	if (d_map_projection_settings == map_projection.get_projection_settings())
	{
		return false;
	}
	d_map_projection_settings = map_projection.get_projection_settings();

	// Generate a new internal mesh.
	create_mesh(gl, map_projection);

	return true;
}


GPlatesOpenGL::GLMultiResolutionMapCubeMesh::QuadTreeNode
GPlatesOpenGL::GLMultiResolutionMapCubeMesh::get_quad_tree_root_node(
		GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face) const
{
	const mesh_cube_quad_tree_type::node_type *root_node =
			d_mesh_cube_quad_tree->get_quad_tree_root_node(cube_face);

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			root_node,
			GPLATES_ASSERTION_SOURCE);

	return QuadTreeNode(*root_node);
}


GPlatesOpenGL::GLMultiResolutionMapCubeMesh::QuadTreeNode
GPlatesOpenGL::GLMultiResolutionMapCubeMesh::get_child_node(
		const QuadTreeNode &parent_node,
		unsigned int child_x_offset,
		unsigned int child_y_offset) const
{
	if (parent_node.d_mesh_node)
	{
		const mesh_cube_quad_tree_type::node_type *child_mesh_node =
				parent_node.d_mesh_node->get_child_node(child_x_offset, child_y_offset);
		if (!child_mesh_node)
		{
			// We've just reached the maximum depth of our mesh cube quad tree.
			// Propagate the parent mesh drawable and start a non-identity clip space transform
			// to compensate.
			return QuadTreeNode(
					parent_node,
					GLUtils::QuadTreeClipSpaceTransform(
							GLUtils::QuadTreeClipSpaceTransform()/*Identity transform*/,
							child_x_offset,
							child_y_offset));
		}

		return QuadTreeNode(*child_mesh_node);
	}

	// We're deeper into the cube quad tree than our pre-generated mesh tree so just continue to
	// propagate the parent mesh drawable and adjust the child's clip space transform to compensate.
	return QuadTreeNode(
			parent_node,
			GLUtils::QuadTreeClipSpaceTransform(
					parent_node.d_clip_space_transform.get(),
					child_x_offset,
					child_y_offset));
}


void
GPlatesOpenGL::GLMultiResolutionMapCubeMesh::create_mesh(
		GL &gl,
		const GPlatesGui::MapProjection &map_projection)
{
	PROFILE_FUNC();

	// Generates the map projection mesh vertices.
	const GLMapCubeMeshGenerator map_cube_mesh_generator(
			map_projection,
			CUBE_FACE_DIMENSION);

	// Iterate over the cube faces and generate the mesh vertices for each face.
	for (unsigned int face = 0; face < 6; ++face)
	{
		const GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face =
				static_cast<GPlatesMaths::CubeCoordinateFrame::CubeFaceType>(face);

		//
		// Create the vertex array and vertex element array for the current cube face by
		// storing vertices/indices in quad tree traversal order.
		//
		// Generate the mesh quad tree for the current cube face.
		const mesh_cube_quad_tree_type::node_type::ptr_type mesh_root_quad_tree_node =
				create_cube_face_mesh(
						gl,
						cube_face,
						map_cube_mesh_generator);

		// Add the root node to the quad tree.
		d_mesh_cube_quad_tree->set_quad_tree_root_node(
				cube_face,
				mesh_root_quad_tree_node);
	}
}


//for boost assert
DISABLE_GCC_WARNING("-Wold-style-cast")

GPlatesOpenGL::GLMultiResolutionMapCubeMesh::mesh_cube_quad_tree_type::node_type::ptr_type
GPlatesOpenGL::GLMultiResolutionMapCubeMesh::create_cube_face_mesh(
		GL &gl,
		GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face,
		const GLMapCubeMeshGenerator &map_cube_mesh_generator)
{
	// Each quad tree tile (at maximum depth) will contain four vertices.
	// This is a duplication of the unique cube face vertices by a factor of four but due to
	// our quad tree storage of vertices we get better vertex array locality and we can specify
	// a smaller (local) range of vertices for the graphics card to process.
	// NOTE: Not that any of this would make much difference on todays graphics cards so really
	// the fact that it's easier to program (with quad tree traversal) is probably the main gain.
	//
	// The extra '8' below accounts for the fact that the cube faces containing the north and south poles
	// will need four extra triangles each to map the four quadrants correctly to the pole singularity.
	// Even without the extra '8' it's an overestimation but it's fine since we're just *reserving* memory as
	// a speed optimisation (to avoid std::vector reallocations) and we free the std::vector soon enough.
	const unsigned int num_mesh_vertices_to_reserve =
			8 + 4 * NUM_MESH_VERTICES_PER_CUBE_FACE_SIDE * NUM_MESH_VERTICES_PER_CUBE_FACE_SIDE;
	const unsigned int num_mesh_indices_to_reserve =
			8 * 3 + 6/*two triangles*/ * NUM_MESH_VERTICES_PER_CUBE_FACE_SIDE * NUM_MESH_VERTICES_PER_CUBE_FACE_SIDE;
	// If we're using 'GLushort' vertex indices which are 16-bit - make sure we don't overflow them.
	// 16-bit indices are faster than 32-bit for graphics cards (but again probably not much gain).
	// The odd combination is to avoid overflow compile error if sizeof(vertex_element_type) happens to be 4.
	BOOST_STATIC_ASSERT((num_mesh_vertices_to_reserve - 1) >> (8/*bits-per-char*/ * sizeof(vertex_element_type) - 1) <= 1);

	std::vector<GLVertexUtils::Texture3DVertex> mesh_vertices;
	mesh_vertices.reserve(num_mesh_vertices_to_reserve);
	std::vector<vertex_element_type> mesh_indices;
	mesh_indices.reserve(num_mesh_indices_to_reserve);

	// Create a single OpenGL vertex array for the current cube face to contain the vertices
	// (and vertex elements or indices) of *all* meshes.
	if (!d_meshes_vertex_array[cube_face])
	{
		d_meshes_vertex_array[cube_face] = GLVertexArray::create(gl);
		d_meshes_vertex_buffer[cube_face] = GLBuffer::create(gl);
		d_meshes_vertex_element_buffer[cube_face] = GLBuffer::create(gl);
	}

	const GPlatesMaths::CubeQuadTreeLocation root_node_location(cube_face);
	AABB root_node_bounding_box;
	double root_max_quad_size_in_map_projection = 0;

	mesh_cube_quad_tree_type::node_type::ptr_type quadrant_mesh_quad_tree_nodes[2][2];

	//
	// Iterate over the child quadrants of the current cube face.
	//
	// This is because adjacent quadrants (in the same cube face) do not necessarily
	// share boundary vertices (if the dateline separates them).
	//
	for (unsigned int quadrant_y_offset = 0; quadrant_y_offset < 2; ++quadrant_y_offset)
	{
		for (unsigned int quadrant_x_offset = 0; quadrant_x_offset < 2; ++quadrant_x_offset)
		{
			// Create all mesh vertices for the current *quadrant* and the current cube face.
			std::vector<GLMapCubeMeshGenerator::Point> cube_face_quadrant_mesh_vertices;
			map_cube_mesh_generator.create_cube_face_quadrant_mesh_vertices(
					cube_face_quadrant_mesh_vertices,
					cube_face,
					quadrant_x_offset,
					quadrant_y_offset);

			// Keep track of the quad tree location so we know which vertices belong to which quad tree nodes.
			const GPlatesMaths::CubeQuadTreeLocation quadrant_node_location(
					root_node_location,
					quadrant_x_offset,
					quadrant_y_offset);

			// Recurse into the quadrant.
			quadrant_mesh_quad_tree_nodes[quadrant_y_offset][quadrant_x_offset] =
					create_cube_face_quad_tree_mesh(
							mesh_vertices,
							mesh_indices,
							root_node_bounding_box,
							root_max_quad_size_in_map_projection,
							cube_face_quadrant_mesh_vertices,
							quadrant_x_offset,
							quadrant_y_offset,
							quadrant_node_location,
							map_cube_mesh_generator);
		}
	}

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_meshes_vertex_array[cube_face],
			GPLATES_ASSERTION_SOURCE);

	// The maximum map projection across the entire cube face.
	const double root_max_map_projection_size = CUBE_FACE_DIMENSION * root_max_quad_size_in_map_projection;

	// Create the cube face root quad tree node.
	mesh_cube_quad_tree_type::node_type::ptr_type cube_face_root_quad_tree_node =
			d_mesh_cube_quad_tree->create_node(
					MeshQuadTreeNode(
							// Specify what to draw for the root quad tree node mesh.
							// The mesh covers all descendants of this quad tree node.
							MeshDrawable(
									d_meshes_vertex_array[cube_face]/*vertex_array*/,
									0/*start*/,
									mesh_vertices.size() - 1/*end*/,
									mesh_indices.size()/*count*/,
									0/*indices_offset*/),
							root_node_bounding_box,
							root_max_map_projection_size));

	// Add the quadrant nodes to the cube face root node.
	for (unsigned int quadrant_y_offset = 0; quadrant_y_offset < 2; ++quadrant_y_offset)
	{
		for (unsigned int quadrant_x_offset = 0; quadrant_x_offset < 2; ++quadrant_x_offset)
		{
			d_mesh_cube_quad_tree->set_child_node(
					*cube_face_root_quad_tree_node,
					quadrant_x_offset,
					quadrant_y_offset,
					quadrant_mesh_quad_tree_nodes[quadrant_y_offset][quadrant_x_offset]);
		}
	}

	// Make sure we leave the OpenGL global state the way it was.
	GPlatesOpenGL::GL::StateScope save_restore_state(gl);

	// Bind vertex array object.
	gl.BindVertexArray(d_meshes_vertex_array[cube_face]);

	// Bind vertex element buffer object to currently bound vertex array object.
	gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, d_meshes_vertex_element_buffer[cube_face]);

	// Transfer vertex element data to currently bound vertex element buffer object.
	glBufferData(
			GL_ELEMENT_ARRAY_BUFFER,
			mesh_indices.size() * sizeof(mesh_indices[0]),
			mesh_indices.data(),
			GL_STATIC_DRAW);

	// Bind vertex buffer object (used by vertex attribute arrays, not vertex array object).
	gl.BindBuffer(GL_ARRAY_BUFFER, d_meshes_vertex_buffer[cube_face]);

	// Transfer vertex data to currently bound vertex buffer object.
	glBufferData(
			GL_ARRAY_BUFFER,
			mesh_vertices.size() * sizeof(mesh_vertices[0]),
			mesh_vertices.data(),
			GL_STATIC_DRAW);

	// Specify vertex attributes (position and texture coords) in currently bound vertex buffer object.
	// This transfers each vertex attribute array (parameters + currently bound vertex buffer object)
	// to currently bound vertex array object.
	gl.EnableVertexAttribArray(0);
	gl.VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLVertexUtils::Texture3DVertex),
			BUFFER_OFFSET(GLVertexUtils::Texture3DVertex, x));
	gl.EnableVertexAttribArray(1);
	gl.VertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GLVertexUtils::Texture3DVertex),
			BUFFER_OFFSET(GLVertexUtils::Texture3DVertex, s));

	return cube_face_root_quad_tree_node;
}

ENABLE_GCC_WARNING("-Wold-style-cast")


GPlatesOpenGL::GLMultiResolutionMapCubeMesh::mesh_cube_quad_tree_type::node_type::ptr_type
GPlatesOpenGL::GLMultiResolutionMapCubeMesh::create_cube_face_quad_tree_mesh(
		std::vector<GLVertexUtils::Texture3DVertex> &mesh_vertices,
		std::vector<vertex_element_type> &mesh_indices,
		AABB &parent_node_bounding_box,
		double &parent_max_quad_size_in_map_projection,
		const std::vector<GLMapCubeMeshGenerator::Point> &cube_face_quadrant_mesh_vertices,
		const unsigned int cube_face_quadrant_x_offset,
		const unsigned int cube_face_quadrant_y_offset,
		const GPlatesMaths::CubeQuadTreeLocation &quad_tree_node_location,
		const GLMapCubeMeshGenerator &map_cube_mesh_generator)
{
	const unsigned int base_vertex_index = mesh_vertices.size();
	const unsigned int base_vertex_element_index = mesh_indices.size();

	AABB node_bounding_box;
	double max_quad_size_in_map_projection = 0;

	mesh_cube_quad_tree_type::node_type::ptr_type child_mesh_quad_tree_nodes[2][2];

	// We only generate the vertices at the leaf nodes of the quad tree.
	if (quad_tree_node_location.get_node_location()->quad_tree_depth == MESH_CUBE_QUAD_TREE_MAXIMUM_DEPTH)
	{
		create_cube_face_quad_tree_mesh_vertices(
				mesh_vertices,
				mesh_indices,
				node_bounding_box,
				max_quad_size_in_map_projection,
				cube_face_quadrant_mesh_vertices,
				cube_face_quadrant_x_offset,
				cube_face_quadrant_y_offset,
				quad_tree_node_location,
				map_cube_mesh_generator);
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
				const GPlatesMaths::CubeQuadTreeLocation child_quad_tree_node_location(
						quad_tree_node_location,
						child_x_offset,
						child_y_offset);

				// Recurse into the child node.
				child_mesh_quad_tree_nodes[child_y_offset][child_x_offset] =
						create_cube_face_quad_tree_mesh(
								mesh_vertices,
								mesh_indices,
								node_bounding_box,
								max_quad_size_in_map_projection,
								cube_face_quadrant_mesh_vertices,
								cube_face_quadrant_x_offset,
								cube_face_quadrant_y_offset,
								child_quad_tree_node_location,
								map_cube_mesh_generator);
			}
		}
	}

	// Expand the parent's bounding box to include ours.
	parent_node_bounding_box.add(node_bounding_box);

	// Update the parent's maximum quad size.
	if (parent_max_quad_size_in_map_projection < max_quad_size_in_map_projection)
	{
		parent_max_quad_size_in_map_projection = max_quad_size_in_map_projection;
	}

	const GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face =
			quad_tree_node_location.get_node_location()->cube_face;

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_meshes_vertex_array[cube_face],
			GPLATES_ASSERTION_SOURCE);

	// Specify what to draw for the current quad tree node mesh.
	// The mesh covers all descendants of this quad tree node.
	const MeshDrawable mesh_drawable(
			d_meshes_vertex_array[cube_face]/*vertex_array*/,
			base_vertex_index/*start*/,
			mesh_vertices.size() - 1/*end*/,
			mesh_indices.size() - base_vertex_element_index/*count*/,
			sizeof(vertex_element_type) * base_vertex_element_index/*indices_offset*/);

	// The maximum map projection across the entire quad tree node.
	const unsigned int num_quads_across_quad_tree_node =
			(CUBE_FACE_DIMENSION >> quad_tree_node_location.get_node_location()->quad_tree_depth);
	const double max_map_projection_size = num_quads_across_quad_tree_node * max_quad_size_in_map_projection;

	// Create a quad tree node.
	mesh_cube_quad_tree_type::node_type::ptr_type mesh_quad_tree_node =
			d_mesh_cube_quad_tree->create_node(
					MeshQuadTreeNode(
							mesh_drawable,
							node_bounding_box,
							max_map_projection_size));

	// Add the child nodes if we visited any.
	if (quad_tree_node_location.get_node_location()->quad_tree_depth < MESH_CUBE_QUAD_TREE_MAXIMUM_DEPTH)
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


void
GPlatesOpenGL::GLMultiResolutionMapCubeMesh::create_cube_face_quad_tree_mesh_vertices(
		std::vector<GLVertexUtils::Texture3DVertex> &mesh_vertices,
		std::vector<vertex_element_type> &mesh_indices,
		AABB &node_bounding_box,
		double &max_quad_size_in_map_projection,
		const std::vector<GLMapCubeMeshGenerator::Point> &cube_face_quadrant_mesh_vertices,
		const unsigned int cube_face_quadrant_x_offset,
		const unsigned int cube_face_quadrant_y_offset,
		const GPlatesMaths::CubeQuadTreeLocation &quad_tree_node_location,
		const GLMapCubeMeshGenerator &map_cube_mesh_generator)
{
	const unsigned int base_vertex_index = mesh_vertices.size();

	// The size of the node in terms of vertex spacing.
	const unsigned int node_dimension =
			(CUBE_FACE_DIMENSION >> quad_tree_node_location.get_node_location()->quad_tree_depth);
	const unsigned int num_vertices_per_node_side = node_dimension + 1;

	// The node offsets relative to the current cube face.
	const unsigned int node_x_offset = node_dimension * quad_tree_node_location.get_node_location()->x_node_offset;
	const unsigned int node_y_offset = node_dimension * quad_tree_node_location.get_node_location()->y_node_offset;

	// The node offsets relative to the current cube face *quadrant*. We need this because the
	// generated vertices are for the quadrant (of the cube face) and not the whole cube face.
	// Note that the 'cube_face_quadrant_*_offset' values are either 0 or 1.
	const unsigned int node_x_quadrant_offset =
			node_x_offset - cube_face_quadrant_x_offset * CUBE_FACE_QUADRANT_DIMENSION;
	const unsigned int node_y_quadrant_offset =
			node_y_offset - cube_face_quadrant_y_offset * CUBE_FACE_QUADRANT_DIMENSION;

	//
	// An 3x3 example of triangles covering the current quad tree leaf node looks like:
	//
	// 0-1-2
	// |/|/|
	// 3-4-5
	// |/|/|
	// 6-7-8
	//

	// The vertices...
	for (unsigned int y = 0; y < num_vertices_per_node_side; ++y)
	{
		for (unsigned int x = 0; x < num_vertices_per_node_side; ++x)
		{
			const unsigned int cube_face_quadrant_mesh_vertices_offset =
					(node_x_quadrant_offset + x) +
							(node_y_quadrant_offset + y) * NUM_MESH_VERTICES_PER_CUBE_FACE_QUADRANT_SIDE;

			const GLMapCubeMeshGenerator::Point &point =
					cube_face_quadrant_mesh_vertices[cube_face_quadrant_mesh_vertices_offset];

			mesh_vertices.push_back(
					GLVertexUtils::Texture3DVertex(
							point.point_2D.x/*x*/,
							point.point_2D.y/*y*/,
							0/*z*/,
							point.point_3D.x().dval()/*s*/,
							point.point_3D.y().dval()/*t*/,
							point.point_3D.z().dval()/*r*/));

			// Expand the bounding box bounds to include the current map-projected position.
			node_bounding_box.add(point.point_2D);
		}
	}

	// The triangles...
	for (unsigned int y = 0; y < node_dimension; ++y)
	{
		for (unsigned int x = 0; x < node_dimension; ++x)
		{
			// Index to first vertex of current quad.
			const unsigned int cube_face_quadrant_mesh_vertices_offset =
					(node_x_quadrant_offset + x) +
							(node_y_quadrant_offset + y) * NUM_MESH_VERTICES_PER_CUBE_FACE_QUADRANT_SIDE;

			// The four corner vertices of the current quad.
			const GLMapCubeMeshGenerator::Point &point00 =
					cube_face_quadrant_mesh_vertices[
							cube_face_quadrant_mesh_vertices_offset];
			const GLMapCubeMeshGenerator::Point &point01 =
					cube_face_quadrant_mesh_vertices[
							cube_face_quadrant_mesh_vertices_offset + 1];
			const GLMapCubeMeshGenerator::Point &point10 =
					cube_face_quadrant_mesh_vertices[
							cube_face_quadrant_mesh_vertices_offset + NUM_MESH_VERTICES_PER_CUBE_FACE_QUADRANT_SIDE];
			const GLMapCubeMeshGenerator::Point &point11 =
					cube_face_quadrant_mesh_vertices[
							cube_face_quadrant_mesh_vertices_offset + NUM_MESH_VERTICES_PER_CUBE_FACE_QUADRANT_SIDE + 1];

			//
			// Determine the size of the current quad in map-projection space.
			//

			// A weighting factor for longitude to counteract the longitude expansion near the pole.
			// This would not normally be done other than the fact that the input raster data tends
			// to be in rectangular coordinates (ie, many more texels around smaller pole region)
			// but gets sampled down in the cube map projection. So this factor is effectively
			// preventing us from trying to get back that down-sampling in the map-projection.
			// However this should really be map-projection dependent since not all map projections
			// expand near the poles like the rectangular map projection does.
			const GPlatesMaths::UnitVector3D quad_centroid = (
					GPlatesMaths::Vector3D(point00.point_3D) +
					GPlatesMaths::Vector3D(point01.point_3D) +
					GPlatesMaths::Vector3D(point10.point_3D) +
					GPlatesMaths::Vector3D(point11.point_3D)).get_normalisation();
			// Scale factor is radius of latitude small circle (shrinks to zero near poles).
			const double longitude_scale_factor =
					sqrt(quad_centroid.x() * quad_centroid.x() + quad_centroid.y() * quad_centroid.y()).dval();

			// Find bounding box of current quad.
			AABB quad_bounds;
			quad_bounds.add(point00.point_2D);
			quad_bounds.add(point01.point_2D);
			quad_bounds.add(point10.point_2D);
			quad_bounds.add(point11.point_2D);

			// Choose the maximum AABB dimension..
			double quad_size_in_map_projection = longitude_scale_factor * (quad_bounds.max_x - quad_bounds.min_x);
			if (quad_size_in_map_projection < quad_bounds.max_y - quad_bounds.min_y)
			{
				quad_size_in_map_projection = quad_bounds.max_y - quad_bounds.min_y;
			}

			// Update the global maximum.
			if (max_quad_size_in_map_projection < quad_size_in_map_projection)
			{
				max_quad_size_in_map_projection = quad_size_in_map_projection;
			}

			//
			// Add the triangles to the mesh.
			//
			// Determine which diagonal to split current quad into two triangles.
			// We want to avoid the possibility of one triangle overlapping the other possibly due
			// to some curvature in the map projection (probably wouldn't happen though but just in case).
			//

			// The plane containing the diagonal from (0,0) to (1,1).
			// We use 3D geometry here since we don't have source code for the 2D equivalents
			// (so we set the 'z' components to zero).
			const GLIntersect::Plane diag_00_11(
					// A vector perpendicular to the diagonal...
					GPlatesMaths::Vector3D(
							point00.point_2D.y - point11.point_2D.y,
							point11.point_2D.x - point00.point_2D.x,
							0)/*normal*/,
					GPlatesMaths::Vector3D(point00.point_2D.x, point00.point_2D.y, 0)/*point_on_plane*/);

			// If the other two points (not on the diagonal) are on opposite sides of the diagonal
			// then we've found a suitable diagonal, otherwise choose the other diagonal.
			if (diag_00_11.signed_distance_unnormalised(
					GPlatesMaths::Vector3D(point01.point_2D.x, point01.point_2D.y, 0)) *
				diag_00_11.signed_distance_unnormalised(
					GPlatesMaths::Vector3D(point10.point_2D.x, point10.point_2D.y, 0)) < 0)
			{
				// 00-01
				// | \ |
				// 10-11

				// First triangle of current quad.
				mesh_indices.push_back(base_vertex_index + y * num_vertices_per_node_side + x);
				mesh_indices.push_back(base_vertex_index + (y + 1) * num_vertices_per_node_side + x + 1);
				mesh_indices.push_back(base_vertex_index + (y + 1) * num_vertices_per_node_side + x);

				// Second triangle of current quad.
				mesh_indices.push_back(base_vertex_index + y * num_vertices_per_node_side + x);
				mesh_indices.push_back(base_vertex_index + y * num_vertices_per_node_side + x + 1);
				mesh_indices.push_back(base_vertex_index + (y + 1) * num_vertices_per_node_side + x + 1);
			}
			else // Use diagonal (0,1) -> (1,0) instead...
			{
				// 00-01
				// | / |
				// 10-11

				// First triangle of current quad.
				mesh_indices.push_back(base_vertex_index + y * num_vertices_per_node_side + x);
				mesh_indices.push_back(base_vertex_index + y * num_vertices_per_node_side + x + 1);
				mesh_indices.push_back(base_vertex_index + (y + 1) * num_vertices_per_node_side + x);

				// Second triangle of current quad.
				mesh_indices.push_back(base_vertex_index + y * num_vertices_per_node_side + x + 1);
				mesh_indices.push_back(base_vertex_index + (y + 1) * num_vertices_per_node_side + x + 1);
				mesh_indices.push_back(base_vertex_index + (y + 1) * num_vertices_per_node_side + x);
			}
		}
	}

	//
	// If the cube face contains the north or south pole then we need to add four extra triangles
	// each to map the four quadrants correctly to the pole singularity.
	//
	// The following shows the four missing triangles near the north pole.
	// There are also four at the south pole (but inverted).
	//
	//-180 -90  0  90  180
	//   ---------------
	//   \  |  / \  |  /
	//    \ | /   \ | /
	//     \|/     \|/
	//
	// The extra pole vertices we create below are at longitudes -90 and 90 (and latitude 90 and -90).
	// The others we have already generated above.
	//
	// This section is particularly tricky to visualise due to the fact that we have to determine
	// which of the four possible orientations of the cube face *local* coordinate frame x and y axes
	// (about their local z-axis) is the actual orientation (see CubeCoordinateFrame) and then we have
	// to work out how that relates to the *global* coordinate frame (ie, relative to poles/dateline).
	//

	const GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face =
			quad_tree_node_location.get_node_location()->cube_face;

	const bool north_pole_is_in_cube_face = (cube_face == GPlatesMaths::CubeCoordinateFrame::POSITIVE_Z);
	const bool south_pole_is_in_cube_face = (cube_face == GPlatesMaths::CubeCoordinateFrame::NEGATIVE_Z);

	if (north_pole_is_in_cube_face ||
		south_pole_is_in_cube_face)
	{
		// See if any of the corner points of the current node are at the south/north pole.
		// This also help determine which quadrant the node is in.
		boost::optional< std::pair<bool/*x-axis*/, bool/*y-axis*/> > quadrant_relative_to_x_and_y_axes;
		if (node_x_offset == CUBE_FACE_QUADRANT_DIMENSION)
		{
			if (node_y_offset == CUBE_FACE_QUADRANT_DIMENSION)
			{
				quadrant_relative_to_x_and_y_axes = std::make_pair(true, true);
			}
			else if (node_y_offset + node_dimension == CUBE_FACE_QUADRANT_DIMENSION)
			{
				quadrant_relative_to_x_and_y_axes = std::make_pair(true, false);
			}
		}
		else if (node_x_offset + node_dimension == CUBE_FACE_QUADRANT_DIMENSION)
		{
			if (node_y_offset == CUBE_FACE_QUADRANT_DIMENSION)
			{
				quadrant_relative_to_x_and_y_axes = std::make_pair(false, true);
			}
			else if (node_y_offset + node_dimension == CUBE_FACE_QUADRANT_DIMENSION)
			{
				quadrant_relative_to_x_and_y_axes = std::make_pair(false, false);
			}
		}

		// If current node's corner touches pole...
		if (quadrant_relative_to_x_and_y_axes)
		{
			const bool quadrant_is_in_positive_x_axis = quadrant_relative_to_x_and_y_axes->first;
			const bool quadrant_is_in_positive_y_axis = quadrant_relative_to_x_and_y_axes->second;

			// The cube corner that this node's quadrant is touching.
			const GPlatesMaths::Vector3D &corner_adjacent_to_quadrant =
					GPlatesMaths::CubeCoordinateFrame::get_cube_corner(
							GPlatesMaths::CubeCoordinateFrame::get_cube_corner_index(
									cube_face,
									quadrant_is_in_positive_x_axis,
									quadrant_is_in_positive_y_axis));

			// Is true if the quadrant is in the half-space of the globe with longitude range [0,180].
			// The other half space has longitude range [-180,0].
			const bool quadrant_is_in_upper_longitude_range =
					dot(corner_adjacent_to_quadrant, GPlatesMaths::UnitVector3D::yBasis()).dval() > 0;

			const double new_pole_longitude = quadrant_is_in_upper_longitude_range ? 90 : -90;
			const GLMapCubeMeshGenerator::Point new_pole_point =
					map_cube_mesh_generator.create_pole_mesh_vertex(
							new_pole_longitude,
							north_pole_is_in_cube_face);

			// The new pole vertex index is the vertex about to be added.
			// We don't need 'base_vertex_index' for this.
			const unsigned int new_pole_vertex_index = mesh_vertices.size();
			// Add the new pole vertex.
			mesh_vertices.push_back(
					GLVertexUtils::Texture3DVertex(
							new_pole_point.point_2D.x/*x*/,
							new_pole_point.point_2D.y/*y*/,
							0/*z*/,
							new_pole_point.point_3D.x().dval()/*s*/,
							new_pole_point.point_3D.y().dval()/*t*/,
							new_pole_point.point_3D.z().dval()/*r*/));

			// Expand the bounding box bounds to include the new pole map-projected position.
			node_bounding_box.add(new_pole_point.point_2D);

			// Index to the existing pole (at longitude -180, 0 or 180) already added to the
			// mesh vertices for the current node.
			//
			// Find which of the four corner points of the node is at the pole.
			const unsigned int current_pole_vertex_index = base_vertex_index +
					(quadrant_is_in_positive_x_axis ? 0 : node_dimension) +
					(quadrant_is_in_positive_y_axis ? 0 : node_dimension) * num_vertices_per_node_side;

			// Index to the point adjacent to the existing pole having the same longitude.
			// Note that the cube face local z-axis always points towards centre of cube
			// which makes it a bit easier to determine whether to index along the local
			// x-axis or local y-axis (of the cube face) since the local coordinate frame
			// is always right-handed.
			const GPlatesMaths::UnitVector3D &cube_face_x_axis =
					GPlatesMaths::CubeCoordinateFrame::get_cube_face_coordinate_frame_axis(
							cube_face,
							GPlatesMaths::CubeCoordinateFrame::X_AXIS);
			// If the cube face local x-axis is aligned with the global y-axis then the adjacent
			// point (to the pole) with same longitude (which is along the global y-axis) is
			// also aligned with the local x-axis.
			const bool is_local_x_axis_aligned_with_global_y_axis = abs(
						dot(cube_face_x_axis, GPlatesMaths::UnitVector3D::yBasis())
					) > 0.5;
			const unsigned int adjacent_point_vertex_index = is_local_x_axis_aligned_with_global_y_axis
					// Next point along (positive or negative) x direction from pole point...
					? current_pole_vertex_index + (quadrant_is_in_positive_x_axis ? 1 : -1)
					// Next point along (positive or negative) y direction...
					: current_pole_vertex_index +
							(quadrant_is_in_positive_y_axis ? 1 : -1) * num_vertices_per_node_side;

			// Add the missing triangle at the pole.

			mesh_indices.push_back(current_pole_vertex_index);
			// Try to keep the triangle's vertex orientation the same as the other triangles.
			if (is_local_x_axis_aligned_with_global_y_axis)
			{
				mesh_indices.push_back(new_pole_vertex_index);
				mesh_indices.push_back(adjacent_point_vertex_index);
			}
			else
			{
				mesh_indices.push_back(adjacent_point_vertex_index);
				mesh_indices.push_back(new_pole_vertex_index);
			}
		}
	}
}


GPlatesOpenGL::GLMultiResolutionMapCubeMesh::AABB::AABB() :
	// The parentheses around min/max are to prevent the windows min/max macros
	// from stuffing numeric_limits' min/max.
	min_x((std::numeric_limits<double>::max)()),
	min_y((std::numeric_limits<double>::max)()),
	max_x(-(std::numeric_limits<double>::max)()),
	max_y(-(std::numeric_limits<double>::max)())
{
}


void
GPlatesOpenGL::GLMultiResolutionMapCubeMesh::AABB::add(
		const GLMapCubeMeshGenerator::Point2D &point)
{
	if (min_x > point.x)
	{
		min_x = point.x;
	}
	if (min_y > point.y)
	{
		min_y = point.y;
	}

	if (max_x < point.x)
	{
		max_x = point.x;
	}
	if (max_y < point.y)
	{
		max_y = point.y;
	}
}


void
GPlatesOpenGL::GLMultiResolutionMapCubeMesh::AABB::add(
		const AABB &aabb)
{
	if (min_x > aabb.min_x)
	{
		min_x = aabb.min_x;
	}
	if (min_y > aabb.min_y)
	{
		min_y = aabb.min_y;
	}

	if (max_x < aabb.max_x)
	{
		max_x = aabb.max_x;
	}
	if (max_y < aabb.max_y)
	{
		max_y = aabb.max_y;
	}
}


GPlatesOpenGL::GLMultiResolutionMapCubeMesh::MeshDrawable::MeshDrawable(
		GLVertexArray::shared_ptr_type vertex_array_,
		GLuint start_,
		GLuint end_,
		GLsizei count_,
		GLint indices_offset_) :
	vertex_array(vertex_array_),
	start(start_),
	end(end_),
	count(count_),
	indices_offset(indices_offset_)
{
}


GPlatesOpenGL::GLMultiResolutionMapCubeMesh::MeshQuadTreeNode::MeshQuadTreeNode(
		const MeshDrawable &mesh_drawable_,
		const AABB &aabb_,
		const double &max_map_projection_size_) :
	mesh_drawable(mesh_drawable_),
	bounding_box_centre_x(0.5 * (aabb_.min_x + aabb_.max_x)),
	bounding_box_centre_y(0.5 * (aabb_.min_y + aabb_.max_y)),
	bounding_box_half_length_x(0.5 * (aabb_.max_x - aabb_.min_x)),
	bounding_box_half_length_y(0.5 * (aabb_.max_y - aabb_.min_y)),
	max_map_projection_size(max_map_projection_size_)
{
}


GPlatesOpenGL::GLMultiResolutionMapCubeMesh::QuadTreeNode::QuadTreeNode(
		const mesh_cube_quad_tree_type::node_type &mesh_node) :
	d_mesh_node(&mesh_node),
	// Create the full 3D oriented bounding box from the minimal 2D axis-aligned bounds...
	d_map_projected_bounding_box(
			GPlatesMaths::Vector3D(
					mesh_node.get_element().bounding_box_centre_x,
					mesh_node.get_element().bounding_box_centre_y,
					0)/*centre*/,
			GPlatesMaths::Vector3D(mesh_node.get_element().bounding_box_half_length_x, 0, 0)/*half_length_x_axis*/,
			GPlatesMaths::Vector3D(0, mesh_node.get_element().bounding_box_half_length_y, 0)/*half_length_y_axis*/,
			GPlatesMaths::Vector3D(0, 0, 1/*not used*/)/*half_length_z_axis*/),
	d_max_map_projection_size(mesh_node.get_element().max_map_projection_size),
	d_mesh_drawable(&mesh_node.get_element().mesh_drawable) // NOTE: Object is referenced (not copied).
{
}


GPlatesOpenGL::GLMultiResolutionMapCubeMesh::QuadTreeNode::QuadTreeNode(
		const QuadTreeNode &parent_node,
		const GLUtils::QuadTreeClipSpaceTransform &clip_space_transform) :
	d_mesh_node(NULL),
	// Use the parent's bounding box - it'll be bigger than we need so culling won't be as efficient - but, to
	// get here, we are quite deep in the quad tree already so have already benefited quite a bit from culling.
	d_map_projected_bounding_box(parent_node.d_map_projected_bounding_box),
	// The child node has half the dimension and hence half the number of vertices along the side...
	d_max_map_projection_size(0.5 * parent_node.d_max_map_projection_size),
	d_mesh_drawable(parent_node.d_mesh_drawable), // NOTE: Object is referenced (not copied).
	d_clip_space_transform(clip_space_transform)
{
}


void
GPlatesOpenGL::GLMultiResolutionMapCubeMesh::QuadTreeNode::render_mesh_drawable(
		GL &gl) const
{
	// Bind the vertex array.
	gl.BindVertexArray(d_mesh_drawable->vertex_array);

	// Draw the vertex array.
	glDrawRangeElements(
			GL_TRIANGLES,
			d_mesh_drawable->start,
			d_mesh_drawable->end,
			d_mesh_drawable->count,
			GLVertexUtils::ElementTraits<vertex_element_type>::type,
			GLVertexUtils::buffer_offset(d_mesh_drawable->indices_offset));
}
