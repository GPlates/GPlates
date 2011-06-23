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
#include <boost/foreach.hpp>
#include <opengl/OpenGL.h>

#include "GLReconstructedStaticPolygonMeshes.h"

#include "GLIntersect.h"
#include "GLRenderer.h"
#include "GLTransformState.h"
#include "GLVertex.h"
#include "GLVertexArrayDrawable.h"

#include "app-logic/GeometryUtils.h"

#include "global/AssertionFailureException.h"
#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "maths/SmallCircleBounds.h"

#include "utils/Profile.h"


GPlatesOpenGL::GLReconstructedStaticPolygonMeshes::GLReconstructedStaticPolygonMeshes(
		const polygon_mesh_seq_type &polygon_meshes,
		const geometries_seq_type &present_day_geometries,
		const reconstructions_spatial_partition_type::non_null_ptr_to_const_type &reconstructions_spatial_partition,
		const cube_subdivision_projection_transforms_cache_type::non_null_ptr_type &cube_subdivision_projection_transforms_cache,
		const cube_subdivision_loose_bounds_cache_type::non_null_ptr_type &cube_subdivision_loose_bounds_cache,
		const cube_subdivision_bounding_polygons_cache_type::non_null_ptr_type &cube_subdivision_bounding_polygons_cache,
		const GLVertexBufferResourceManager::shared_ptr_type &vertex_buffer_resource_manager) :
	d_cube_subdivision_projection_transforms_cache(cube_subdivision_projection_transforms_cache),
	d_cube_subdivision_loose_bounds_cache(cube_subdivision_loose_bounds_cache),
	d_cube_subdivision_bounding_polygons_cache(cube_subdivision_bounding_polygons_cache),
	d_vertex_buffer_resource_manager(vertex_buffer_resource_manager),
	d_present_day_polygon_meshes_node_intersections(polygon_meshes.size()),
	d_reconstructions_spatial_partition(reconstructions_spatial_partition)
{
	//PROFILE_FUNC();

	create_polygon_mesh_drawables(polygon_meshes);

	find_present_day_polygon_mesh_node_intersections(present_day_geometries, polygon_meshes);
}


void
GPlatesOpenGL::GLReconstructedStaticPolygonMeshes::update(
		const reconstructions_spatial_partition_type::non_null_ptr_to_const_type &reconstructions_spatial_partition,
		boost::optional<reconstructions_spatial_partition_type::non_null_ptr_to_const_type>
						active_or_inactive_reconstructions_spatial_partition)
{
	d_reconstructions_spatial_partition = reconstructions_spatial_partition;
	d_active_or_inactive_reconstructions_spatial_partition = active_or_inactive_reconstructions_spatial_partition;
}


GPlatesOpenGL::GLReconstructedStaticPolygonMeshes::ReconstructedPolygonMeshTransformsGroups::non_null_ptr_to_const_type
GPlatesOpenGL::GLReconstructedStaticPolygonMeshes::get_visible_reconstructed_polygon_meshes(
		const GLTransformState &transform_state)
{
	PROFILE_FUNC();

	//
	// Iterate over the current reconstructed polygons and determine which ones intersect the view frustum.
	//

	reconstructed_polygon_mesh_transform_group_seq_type reconstructed_polygon_mesh_transform_groups;

	// Keep track of which reconstructed polygon mesh transform groups are associated with
	// which finite rotations.
	reconstructed_polygon_mesh_transform_group_map_type reconstructed_polygon_mesh_transform_group_map;

	// The total number of polygon meshes.
	const unsigned int num_polygon_meshes = d_present_day_polygon_mesh_drawables.size();

	// Add any reconstructed polygons that exist in the root of the reconstructions cube quad tree.
	// These are the ones that were too big to fit into any loose cube face.
	add_visible_reconstructed_polygon_meshes(
			reconstructed_polygon_mesh_transform_groups,
			reconstructed_polygon_mesh_transform_group_map,
			num_polygon_meshes,
			d_reconstructions_spatial_partition->begin_root_elements(),
			d_reconstructions_spatial_partition->end_root_elements(),
			true/*active_reconstructions_only*/);
	if (d_active_or_inactive_reconstructions_spatial_partition)
	{
		add_visible_reconstructed_polygon_meshes(
				reconstructed_polygon_mesh_transform_groups,
				reconstructed_polygon_mesh_transform_group_map,
				num_polygon_meshes,
				d_active_or_inactive_reconstructions_spatial_partition.get()->begin_root_elements(),
				d_active_or_inactive_reconstructions_spatial_partition.get()->end_root_elements(),
				false/*active_reconstructions_only*/);
	}

	// Get the view frustum planes.
	const GLFrustum &frustum_planes = transform_state.get_current_frustum_planes_in_model_space();

	// Traverse reconstructed feature geometries of the quad trees of the cube faces.
	for (unsigned int face = 0; face < 6; ++face)
	{
		const GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face =
				static_cast<GPlatesMaths::CubeCoordinateFrame::CubeFaceType>(face);

		// The root node of the current reconstructions quad tree.
		const reconstructions_spatial_partition_type::const_node_reference_type
				reconstructions_quad_tree_root = d_reconstructions_spatial_partition
						->get_quad_tree_root_node(cube_face);

		// The root node of the current active-or-inactive reconstructions quad tree, if we
		// have the corresponding spatial partition.
		reconstructions_spatial_partition_type::const_node_reference_type active_or_inactive_reconstructions_quad_tree_root;
		if (d_active_or_inactive_reconstructions_spatial_partition)
		{
			// The root node of the current active-or-inactive reconstructions quad tree.
			active_or_inactive_reconstructions_quad_tree_root =
					d_active_or_inactive_reconstructions_spatial_partition.get()
							->get_quad_tree_root_node(cube_face);
		}

		// If there are no reconstructed feature geometries in the current loose cube face
		// then continue to next cube face.
		if (!reconstructions_quad_tree_root && !active_or_inactive_reconstructions_quad_tree_root)
		{
			continue;
		}

		// Get the loose bounds quad tree root node.
		const cube_subdivision_loose_bounds_cache_type::node_reference_type
				loose_bounds_quad_tree_root =
						d_cube_subdivision_loose_bounds_cache->get_quad_tree_root_node(cube_face);

		get_visible_reconstructed_polygon_meshes_from_quad_tree(
				reconstructed_polygon_mesh_transform_groups,
				reconstructed_polygon_mesh_transform_group_map,
				num_polygon_meshes,
				reconstructions_quad_tree_root,
				active_or_inactive_reconstructions_quad_tree_root,
				loose_bounds_quad_tree_root,
				frustum_planes,
				// There are six frustum planes initially active
				GLFrustum::ALL_PLANES_ACTIVE_MASK);
	}

	return ReconstructedPolygonMeshTransformsGroups::create(
			reconstructed_polygon_mesh_transform_groups,
			num_polygon_meshes);
}


void
GPlatesOpenGL::GLReconstructedStaticPolygonMeshes::get_visible_reconstructed_polygon_meshes_from_quad_tree(
		reconstructed_polygon_mesh_transform_group_seq_type &reconstructed_polygon_mesh_transform_groups,
		reconstructed_polygon_mesh_transform_group_map_type &reconstructed_polygon_mesh_transform_group_map,
		unsigned int num_polygon_meshes,
		const reconstructions_spatial_partition_type::const_node_reference_type &reconstructions_quad_tree_node,
		const reconstructions_spatial_partition_type::const_node_reference_type &active_or_inactive_reconstructions_quad_tree_node,
		const cube_subdivision_loose_bounds_cache_type::node_reference_type &loose_bounds_quad_tree_node,
		const GLFrustum &frustum_planes,
		boost::uint32_t frustum_plane_mask)
{
	// If the frustum plane mask is zero then it means we are entirely inside the view frustum.
	// So only test for intersection if the mask is non-zero.
	if (frustum_plane_mask != 0)
	{
		boost::shared_ptr<const cube_subdivision_loose_bounds_cache_type::element_type>
				loose_bounds_cached_element =
						d_cube_subdivision_loose_bounds_cache->get_cached_element(
								loose_bounds_quad_tree_node);
		const GLIntersect::OrientedBoundingBox &quad_tree_node_loose_bounds =
				loose_bounds_cached_element->get_loose_oriented_bounding_box();

		// See if the current quad tree node intersects the view frustum.
		// Use the static quad tree node's bounding box.
		boost::optional<boost::uint32_t> out_frustum_plane_mask =
				GLIntersect::intersect_OBB_frustum(
						quad_tree_node_loose_bounds,
						frustum_planes.get_planes(),
						frustum_plane_mask);
		if (!out_frustum_plane_mask)
		{
			// No intersection so quad tree node is outside view frustum and we can cull it.
			return;
		}

		// Update the frustum plane mask so we only test against those planes that
		// the current quad tree render node intersects. The node is entirely inside
		// the planes with a zero bit and so its child nodes are also entirely inside
		// those planes too and so they won't need to test against them.
		frustum_plane_mask = out_frustum_plane_mask.get();
	}

	// Add the polygon meshes of the current quad tree node to the visible list.
	if (reconstructions_quad_tree_node)
	{
		add_visible_reconstructed_polygon_meshes(
				reconstructed_polygon_mesh_transform_groups,
				reconstructed_polygon_mesh_transform_group_map,
				num_polygon_meshes,
				reconstructions_quad_tree_node.begin(),
				reconstructions_quad_tree_node.end(),
				true/*active_reconstructions_only*/);
	}
	if (active_or_inactive_reconstructions_quad_tree_node)
	{
		add_visible_reconstructed_polygon_meshes(
				reconstructed_polygon_mesh_transform_groups,
				reconstructed_polygon_mesh_transform_group_map,
				num_polygon_meshes,
				active_or_inactive_reconstructions_quad_tree_node.begin(),
				active_or_inactive_reconstructions_quad_tree_node.end(),
				false/*active_reconstructions_only*/);
	}

	//
	// Iterate over the child quad tree nodes.
	//

	for (unsigned int child_v_offset = 0; child_v_offset < 2; ++child_v_offset)
	{
		for (unsigned int child_u_offset = 0; child_u_offset < 2; ++child_u_offset)
		{
			// See if there are reconstructed feature geometries in the current child node.
			reconstructions_spatial_partition_type::const_node_reference_type child_reconstructions_quad_tree_node;
			if (reconstructions_quad_tree_node)
			{
				child_reconstructions_quad_tree_node =
						reconstructions_quad_tree_node.get_child_node(
								child_u_offset, child_v_offset);
			}

			// See if there are reconstructed feature geometries in the current child node.
			reconstructions_spatial_partition_type::const_node_reference_type child_active_or_inactive_reconstructions_quad_tree_node;
			if (active_or_inactive_reconstructions_quad_tree_node)
			{
				child_active_or_inactive_reconstructions_quad_tree_node =
						active_or_inactive_reconstructions_quad_tree_node.get_child_node(
								child_u_offset, child_v_offset);
			}

			if (!child_reconstructions_quad_tree_node && !child_active_or_inactive_reconstructions_quad_tree_node)
			{
				continue;
			}

			// Get the loose bounds child quad tree node.
			const cube_subdivision_loose_bounds_cache_type::node_reference_type
					child_loose_bounds_quad_tree_node =
							d_cube_subdivision_loose_bounds_cache->get_child_node(
									loose_bounds_quad_tree_node,
									child_u_offset,
									child_v_offset);

			get_visible_reconstructed_polygon_meshes_from_quad_tree(
					reconstructed_polygon_mesh_transform_groups,
					reconstructed_polygon_mesh_transform_group_map,
					num_polygon_meshes,
					child_reconstructions_quad_tree_node,
					child_active_or_inactive_reconstructions_quad_tree_node,
					child_loose_bounds_quad_tree_node,
					frustum_planes,
					frustum_plane_mask);
		}
	}
}


void
GPlatesOpenGL::GLReconstructedStaticPolygonMeshes::add_visible_reconstructed_polygon_meshes(
		reconstructed_polygon_mesh_transform_group_seq_type &reconstructed_polygon_mesh_transform_groups,
		reconstructed_polygon_mesh_transform_group_map_type &reconstructed_polygon_mesh_transform_group_map,
		unsigned int num_polygon_meshes,
		const reconstructions_spatial_partition_type::element_const_iterator &begin_reconstructions,
		const reconstructions_spatial_partition_type::element_const_iterator &end_reconstructions,
		bool active_reconstructions_only)
{
	// Iterate over the sequence of reconstructions.
	for (reconstructions_spatial_partition_type::element_const_iterator reconstructions_iter = begin_reconstructions;
		reconstructions_iter != end_reconstructions;
		++reconstructions_iter)
	{
		const GPlatesAppLogic::ReconstructContext::Reconstruction &reconstruction = *reconstructions_iter;

		// Get the index into our present day geometries.
		const GPlatesAppLogic::ReconstructContext::geometry_property_handle_type present_day_geometry_index =
				reconstruction.get_geometry_property_handle();
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				present_day_geometry_index < d_present_day_polygon_mesh_drawables.size(),
				GPLATES_ASSERTION_SOURCE);

		// This is the drawable for the present day polygon mesh that corresponds to the
		// current reconstructed feature geometry.
		const boost::optional<GLDrawable::non_null_ptr_to_const_type> &present_day_polygon_mesh_drawable =
				d_present_day_polygon_mesh_drawables[present_day_geometry_index];
		if (!present_day_polygon_mesh_drawable)
		{
			// If there's no polygon mesh drawable then it means there's no polygon mesh which means
			// a mesh couldn't be generated. So we'll skip the current reconstructed polygon mesh.
			continue;
		}

		// The current reconstructed feature geometry.
		const GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type &rfg =
				reconstruction.get_reconstructed_feature_geometry();

		const boost::optional<GPlatesAppLogic::ReconstructedFeatureGeometry::FiniteRotationReconstruction> &
				finite_rotation_reconstruction = rfg->finite_rotation_reconstruction();
		// We're expecting a finite rotation - if we don't get one then we don't do anything
		// because reconstructing rasters with static polygons is relying on the fact that
		// static polygons don't change over time and hence we can create a present-day mesh
		// of the polygons and simply rotate it on the graphics hardware.
		if (!finite_rotation_reconstruction)
		{
			continue;
		}

		// Get the finite rotation.
		GPlatesAppLogic::ReconstructMethodFiniteRotation::non_null_ptr_to_const_type
				reconstruct_method_finite_rotation =
						finite_rotation_reconstruction->get_reconstruct_method_finite_rotation();

		// See if we have a transform group (of reconstructed polygon meshes) for the finite rotation.
		// Try to insert a transform group - if it fails then we already have one representing
		// the finite rotation.
		std::pair<reconstructed_polygon_mesh_transform_group_map_type::iterator, bool> inserted =
				reconstructed_polygon_mesh_transform_group_map.insert(
						reconstructed_polygon_mesh_transform_group_map_type::value_type(
								// Store a reference in the map...
								boost::cref(*reconstruct_method_finite_rotation),
								// Index of next transform group in client's sequence...
								reconstructed_polygon_mesh_transform_groups.size()));

		// If we successfully inserted a new transform group then we need to create it.
		// Because we only inserted it's index into the client's sequence.
		if (inserted.second)
		{
			// Convert the finite rotation from a unit quaternion to a matrix so we can feed it to OpenGL.
			const GPlatesMaths::UnitQuaternion3D &quat_rotation =
					reconstruct_method_finite_rotation->get_finite_rotation().unit_quat();
			GLTransform::non_null_ptr_type rotation_transform = GLTransform::create(GL_MODELVIEW, quat_rotation);

			// Add a new transform group to the client's sequence.
			reconstructed_polygon_mesh_transform_groups.push_back(
					ReconstructedPolygonMeshTransformGroup(
							rotation_transform,
							num_polygon_meshes));
		}

		// Get the transform group for the current finite rotation.
		const reconstructed_polygon_mesh_transform_group_seq_type::size_type transform_group_index =
				inserted.first->second;
		ReconstructedPolygonMeshTransformGroup &reconstructed_polygon_mesh_transform_group =
				reconstructed_polygon_mesh_transform_groups[transform_group_index];

		// Finally add the polygon mesh to the appropriate membership list of the current transform group.
		if (active_reconstructions_only)
		{
			reconstructed_polygon_mesh_transform_group.add_present_day_polygon_mesh_for_active_reconstruction(
					present_day_geometry_index);
		}
		else
		{
			reconstructed_polygon_mesh_transform_group.add_present_day_polygon_mesh_for_active_or_inactive_reconstruction(
					present_day_geometry_index);
		}
	}
}


void
GPlatesOpenGL::GLReconstructedStaticPolygonMeshes::create_polygon_mesh_drawables(
		const polygon_mesh_seq_type &polygon_meshes)
{
	PROFILE_FUNC();

	// The OpenGL vertices and vertex elements (indices) of all polygon meshes are
	// placed in a single vertex array (and vertex element array).
	std::vector<GLVertex> all_polygon_meshes_vertices;
	std::vector<GLuint> all_polygon_meshes_indices;

	//
	// First iterate over the polygon meshes and build the vertex array and vertex element array.
	//
	BOOST_FOREACH(
			const boost::optional<GPlatesMaths::PolygonMesh::non_null_ptr_to_const_type> &polygon_mesh_opt,
			polygon_meshes)
	{
		if (!polygon_mesh_opt)
		{
			continue;
		}
		const GPlatesMaths::PolygonMesh::non_null_ptr_to_const_type &polygon_mesh = polygon_mesh_opt.get();

		// Get the base vertex index for the current polygon mesh.
		// All its vertex indices are offset by zero so we need to adjust that offset since
		// all polygon meshes are going into a *single* vertex array.
		const GLuint base_vertex_index = all_polygon_meshes_vertices.size();

		// Add the vertices.
		typedef std::vector<GPlatesMaths::PolygonMesh::Vertex> vertex_seq_type;
		const vertex_seq_type &vertices = polygon_mesh->get_vertices();
		for (vertex_seq_type::const_iterator vertices_iter = vertices.begin();
			vertices_iter != vertices.end();
			++vertices_iter)
		{
			all_polygon_meshes_vertices.push_back(GLVertex(vertices_iter->get_vertex()));
		}

		// Add the indices.
		typedef std::vector<GPlatesMaths::PolygonMesh::Triangle> triangle_seq_type;
		const triangle_seq_type &triangles = polygon_mesh->get_triangles();
		for (triangle_seq_type::const_iterator triangles_iter = triangles.begin();
			triangles_iter != triangles.end();
			++triangles_iter)
		{
			const GPlatesMaths::PolygonMesh::Triangle &triangle = *triangles_iter;

			// Iterate over the triangle vertices.
			for (unsigned int tri_vertex_index = 0; tri_vertex_index < 3; ++tri_vertex_index)
			{
				all_polygon_meshes_indices.push_back(
						base_vertex_index + triangle.get_mesh_vertex_index(tri_vertex_index));
			}
		}
	}

	// If we don't have any polygon meshes for some reason then just return.
	if (all_polygon_meshes_vertices.empty() || all_polygon_meshes_indices.empty())
	{
		return;
	}

	// Create a single OpenGL vertex array to contain the vertices of *all* polygon meshes.
	d_polygon_meshes_vertex_array = GLVertexArray::create(
			all_polygon_meshes_vertices,
			GLArray::USAGE_STATIC,
			d_vertex_buffer_resource_manager);

	// Create a single OpenGL vertex array to contain the vertex indices of *all* polygon meshes.
	// Note that this vertex element array is never used to draw - it just contains the indices.
	// Each polygon mesh drawable will creates its own vertex element array that shares the
	// indices of this array.
	d_polygon_meshes_vertex_element_array = GLVertexElementArray::create(
					all_polygon_meshes_indices,
					GLArray::USAGE_STATIC,
					d_vertex_buffer_resource_manager);


	// The polygon mesh drawables must map to the input polygon meshes.
	// If there's a missing input polygon mesh (because the polygon couldn't be meshed) then
	// there should also be a corresponding missing drawable.
	d_present_day_polygon_mesh_drawables.reserve(polygon_meshes.size());


	//
	// Next iterate over the polygon meshes again and create the drawables.
	//
	GLuint polygon_mesh_base_vertex_index = 0;
	GLuint polygon_mesh_base_triangle_index = 0;
	BOOST_FOREACH(
			const boost::optional<GPlatesMaths::PolygonMesh::non_null_ptr_to_const_type> &polygon_mesh_opt,
			polygon_meshes)
	{
		boost::optional<GLDrawable::non_null_ptr_to_const_type> polygon_mesh_drawable;

		// There might be no polygon mesh for the current slot.
		if (polygon_mesh_opt)
		{
			const GPlatesMaths::PolygonMesh::non_null_ptr_to_const_type &polygon_mesh = polygon_mesh_opt.get();

			// Create a vertex element array that shares the same indices as the other polygon meshes.
			const GLVertexElementArray::shared_ptr_type polygon_mesh_vertex_element_array =
					GLVertexElementArray::create(
							d_polygon_meshes_vertex_element_array->get_array_data(),
							GL_UNSIGNED_INT);

			// Specify what to draw for the current polygon mesh.
			const GLuint num_vertices_in_polygon_mesh = polygon_mesh->get_vertices().size();
			const GLuint num_triangles_in_polygon_mesh = polygon_mesh->get_triangles().size();
			polygon_mesh_vertex_element_array->gl_draw_range_elements_EXT(
					GL_TRIANGLES,
					polygon_mesh_base_vertex_index/*start*/,
					polygon_mesh_base_vertex_index + num_vertices_in_polygon_mesh - 1/*end*/,
					3 * num_triangles_in_polygon_mesh/*count*/,
					sizeof(GLuint) * 3 * polygon_mesh_base_triangle_index/*indices_offset*/);

			// Create the drawable.
			polygon_mesh_drawable = GLVertexArrayDrawable::create(
							d_polygon_meshes_vertex_array,
							polygon_mesh_vertex_element_array);

			// Update the base vertex index for the next polygon mesh.
			polygon_mesh_base_vertex_index += num_vertices_in_polygon_mesh;

			// Update the base triangle index for the next polygon mesh.
			polygon_mesh_base_triangle_index += num_triangles_in_polygon_mesh;
		}

		// Add the polygon mesh drawable even if it's boost::none.
		// This is because we index into the drawables using the same indices as used to index
		// into the input polygon meshes.
		d_present_day_polygon_mesh_drawables.push_back(polygon_mesh_drawable);
	}
}


void
GPlatesOpenGL::GLReconstructedStaticPolygonMeshes::find_present_day_polygon_mesh_node_intersections(
		const geometries_seq_type &present_day_geometries,
		const polygon_mesh_seq_type &polygon_meshes)
{
	PROFILE_FUNC();

	// The number of polygon meshes (optional) should equal the number of geometries.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			present_day_geometries.size() == polygon_meshes.size(),
			GPLATES_ASSERTION_SOURCE);

	// Iterate over the present day polygon meshes.
	const unsigned int num_polygon_meshes = polygon_meshes.size();
	for (present_day_polygon_mesh_handle_type polygon_mesh_handle = 0;
		polygon_mesh_handle < num_polygon_meshes;
		++polygon_mesh_handle)
	{
		const boost::optional<GPlatesMaths::PolygonMesh::non_null_ptr_to_const_type> &polygon_mesh_opt =
				polygon_meshes[polygon_mesh_handle];
		if (!polygon_mesh_opt)
		{
			continue;
		}
		const GPlatesMaths::PolygonMesh::non_null_ptr_to_const_type &polygon_mesh = polygon_mesh_opt.get();

		// Get the bounding small circle of the polygon mesh if appropriate for its geometry type.
		// It should be if we were able to generate a polygon mesh from the geometry.
		boost::optional<const GPlatesMaths::BoundingSmallCircle &> polygon_mesh_bounding_small_circle =
				GPlatesAppLogic::GeometryUtils::get_geometry_bounding_small_circle(
						*present_day_geometries[polygon_mesh_handle]);

		// Initial coverage of triangles of the current polygon mesh is all triangles
		// because we're at the root of the cube quad tree which is the entire globe.
		std::vector<unsigned int> polygon_mesh_triangle_indices;
		const unsigned int num_triangles_in_polygon_mesh = polygon_mesh->get_triangles().size();
		polygon_mesh_triangle_indices.reserve(num_triangles_in_polygon_mesh);
		for (unsigned int triangle_index = 0; triangle_index < num_triangles_in_polygon_mesh; ++triangle_index)
		{
			polygon_mesh_triangle_indices.push_back(triangle_index);
		}

		// Traverse the quad trees of the cube faces to determine intersection of current polygon mesh
		// with the nodes of each cube face quad tree.
		for (unsigned int face = 0; face < 6; ++face)
		{
			const GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face =
					static_cast<GPlatesMaths::CubeCoordinateFrame::CubeFaceType>(face);

			// Get the intersections quad tree root node.
			PresentDayPolygonMeshesNodeIntersections::intersection_partition_type::node_type &
					intersections_quad_tree_root_node =
							d_present_day_polygon_meshes_node_intersections.get_or_create_quad_tree_root_node(cube_face);

			// Get the projection transforms quad tree root node.
			const cube_subdivision_projection_transforms_cache_type::node_reference_type
					projection_transforms_cache_root_node =
							d_cube_subdivision_projection_transforms_cache->get_quad_tree_root_node(cube_face);

			// Get the bounding polygons quad tree root node.
			const cube_subdivision_bounding_polygons_cache_type::node_reference_type
					bounding_polygons_cache_root_node =
							d_cube_subdivision_bounding_polygons_cache->get_quad_tree_root_node(cube_face);

			// Recursively generate an intersections quad tree for the current cube face.
			find_present_day_polygon_mesh_node_intersections(
					polygon_mesh_handle,
					num_polygon_meshes,
					*polygon_mesh,
					polygon_mesh_bounding_small_circle,
					polygon_mesh_triangle_indices,
					intersections_quad_tree_root_node,
					projection_transforms_cache_root_node,
					bounding_polygons_cache_root_node,
					0/*current_depth*/);
		}
	}
}


void
GPlatesOpenGL::GLReconstructedStaticPolygonMeshes::find_present_day_polygon_mesh_node_intersections(
		const present_day_polygon_mesh_handle_type present_day_polygon_mesh_handle,
		unsigned int num_polygon_meshes,
		const GPlatesMaths::PolygonMesh &polygon_mesh,
		const boost::optional<const GPlatesMaths::BoundingSmallCircle &> &polygon_mesh_bounding_small_circle,
		const std::vector<unsigned int> &polygon_mesh_parent_triangle_indices,
		PresentDayPolygonMeshesNodeIntersections::intersection_partition_type::node_type &intersections_quad_tree_node,
		const cube_subdivision_projection_transforms_cache_type::node_reference_type &projection_transforms_cache_node,
		const cube_subdivision_bounding_polygons_cache_type::node_reference_type &bounding_polygons_cache_node,
		unsigned int current_depth)
{
	//
	// Quick test to see if the current polygon mesh possibly intersects the current quad tree node.
	//
	// NOTE: Turns out this slows things down significantly - probably because we traverse more
	// child nodes than are in the cache and hence get no reuse - so we're constantly calculating
	// the bounding polygon and bounding small circle. Significantly increasing the cache size
	// does noticeably reduce running time but it's still higher than without this test at all.
#if 0
	if (polygon_mesh_bounding_small_circle)
	{
		// Get the bounding polygon for the current cube quad tree node.
		boost::shared_ptr<const cube_subdivision_bounding_polygons_cache_type::element_type>
				bounding_polygons_cached_element =
						d_cube_subdivision_bounding_polygons_cache->get_cached_element(
								bounding_polygons_cache_node);
		const GPlatesMaths::BoundingSmallCircle &node_bounding_small_circle =
				bounding_polygons_cached_element->get_bounding_polygon()->get_bounding_small_circle();

		if (!intersect(polygon_mesh_bounding_small_circle.get(), node_bounding_small_circle))
		{
			return;
		}
	}
#endif

	//
	// Now do a more accurate intersection test involving the triangles of the polygon mesh and
	// the frustum planes of the current quad tree node.
	//

	// Get the frustum for the current cube quad tree node.
	boost::shared_ptr<const cube_subdivision_projection_transforms_cache_type::element_type>
			projection_transforms_cached_element =
					d_cube_subdivision_projection_transforms_cache->get_cached_element(
							projection_transforms_cache_node);
	const GLFrustum &quad_tree_node_frustum = projection_transforms_cached_element->get_frustum();

	//
	// Find the triangles (from the parent triangles subset) of the current polygon mesh
	// that possibly intersect the current quad tree node.
	//

	// The triangles and vertices of the current polygon mesh.
	const std::vector<GPlatesMaths::PolygonMesh::Triangle> &polygon_mesh_triangles = polygon_mesh.get_triangles();
	const std::vector<GPlatesMaths::PolygonMesh::Vertex> &polygon_mesh_vertices = polygon_mesh.get_vertices();

	// The triangles of the current polygon mesh that possibly intersect the current cube quad tree node.
	std::vector<unsigned int> polygon_mesh_triangle_indices;
	// Might as well reserve memory for the largest possible indices size - we're traversing
	// the runtime stack so the number of std::vector objects on the stack at any one time will
	// be relatively small (the maximum depth of the quad tree).
	polygon_mesh_triangle_indices.reserve(polygon_mesh_parent_triangle_indices.size());

	// Iterate over the parent triangles.
	const unsigned int num_triangles_to_test = polygon_mesh_parent_triangle_indices.size();
	for (unsigned int triangle_indices_index = 0; triangle_indices_index < num_triangles_to_test; ++triangle_indices_index)
	{
		const unsigned int triangle_index = polygon_mesh_parent_triangle_indices[triangle_indices_index];
		const GPlatesMaths::PolygonMesh::Triangle &triangle = polygon_mesh_triangles[triangle_index];

		// Test the current triangle against the frustum planes.
		bool is_triangle_outside_frustum = false;
		for (unsigned int plane_index = 0; plane_index < GLFrustum::NUM_PLANES; ++plane_index)
		{
			const GLFrustum::PlaneType plane_type = static_cast<GLFrustum::PlaneType>(plane_index);
			const GLIntersect::Plane &plane = quad_tree_node_frustum.get_plane(plane_type);

			// If all vertices of the triangle are outside a single plane then
			// the triangle is outside the frustum.
			if (plane.signed_distance(polygon_mesh_vertices[triangle.get_mesh_vertex_index(0)].get_vertex()) < 0 &&
				plane.signed_distance(polygon_mesh_vertices[triangle.get_mesh_vertex_index(1)].get_vertex()) < 0 &&
				plane.signed_distance(polygon_mesh_vertices[triangle.get_mesh_vertex_index(2)].get_vertex()) < 0)
			{
				is_triangle_outside_frustum = true;
				break;
			}
		}

		// If the current triangle possibly intersects the current quad tree node then record
		// the triangle index - this means child nodes have fewer triangles to test.
		if (!is_triangle_outside_frustum)
		{
			polygon_mesh_triangle_indices.push_back(triangle_index);
		}
	}

	// If no triangles intersect the current quad tree node then we are finished and can return.
	if (polygon_mesh_triangle_indices.empty())
	{
		return;
	}

	// Record that the current polygon mesh possibly intersects the current quad tree node.
	// Note that this is the main reason we are doing this whole traversal.
	d_present_day_polygon_meshes_node_intersections
			.get_intersecting_polygon_meshes(intersections_quad_tree_node)
			.add_present_day_polygon_mesh(present_day_polygon_mesh_handle);

	// Return if we've reached the maximum quad tree depth.
	if (d_present_day_polygon_meshes_node_intersections.is_node_at_maximum_depth(intersections_quad_tree_node))
	{
		return;
	}

	// Release our cached element before we traverse the child nodes otherwise the cache
	// will continue to grow as we recurse.
	projection_transforms_cached_element.reset();

	//
	// Iterate over the child quad tree nodes.
	//

	for (unsigned int child_y_offset = 0; child_y_offset < 2; ++child_y_offset)
	{
		for (unsigned int child_x_offset = 0; child_x_offset < 2; ++child_x_offset)
		{
			// Get the child intersections quad tree node.
			PresentDayPolygonMeshesNodeIntersections::intersection_partition_type::node_type &
					child_intersections_quad_tree_node =
							d_present_day_polygon_meshes_node_intersections.get_or_create_child_node(
									intersections_quad_tree_node,
									child_x_offset,
									child_y_offset);

			// Get the projection transforms child quad tree node.
			const cube_subdivision_projection_transforms_cache_type::node_reference_type
					child_projection_transforms_cache_node =
							d_cube_subdivision_projection_transforms_cache->get_child_node(
									projection_transforms_cache_node,
									child_x_offset,
									child_y_offset);

			// Get the bounding polygons child quad tree node.
			const cube_subdivision_bounding_polygons_cache_type::node_reference_type
					child_bounding_polygons_cache_node =
							d_cube_subdivision_bounding_polygons_cache->get_child_node(
									bounding_polygons_cache_node,
									child_x_offset,
									child_y_offset);

			// Returns a non-null child if the current polygon mesh possibly intersects the child quad tree node.
			find_present_day_polygon_mesh_node_intersections(
					present_day_polygon_mesh_handle,
					num_polygon_meshes,
					polygon_mesh,
					polygon_mesh_bounding_small_circle,
					polygon_mesh_triangle_indices,
					child_intersections_quad_tree_node,
					child_projection_transforms_cache_node,
					child_bounding_polygons_cache_node,
					current_depth + 1);
		}
	}
}


const GPlatesOpenGL::GLReconstructedStaticPolygonMeshes::PresentDayPolygonMeshesNodeIntersections::intersection_partition_type::node_type *
GPlatesOpenGL::GLReconstructedStaticPolygonMeshes::PresentDayPolygonMeshesNodeIntersections::get_child_node(
		const intersection_partition_type::node_type &parent_node,
		unsigned int child_x_offset,
		unsigned int child_y_offset) const
{
	const intersection_partition_type::node_type *child_node =
			parent_node.get_child_node(child_x_offset, child_y_offset);

	if (!child_node)
	{
		// If there's no child node and it's because the parent node is at the maximum depth
		// then create a new child node that shares the polygon mesh membership of the parent.
		// This is so clients continue to propagate the intersection coverage down the quad tree
		// as they traverse it - because we have not calculated a deep enough tree for them.
		if (parent_node.get_element().d_quad_tree_depth >= MAXIMUM_DEPTH)
		{
			// Create a new child node that shares the polygon mesh membership of the parent.
			// NOTE: Using a 'const_cast' here since we're modifying the parent node, by adding a child.
			child_node = &d_intersection_partition->set_child_node(
					const_cast<intersection_partition_type::node_type &>(parent_node),
					child_x_offset,
					child_y_offset,
					Node(
							parent_node.get_element().d_polygon_mesh_membership,
							parent_node.get_element().d_quad_tree_depth + 1));
		}
	}

	// The child node can be NULL - if we get here and it's NULL then it means
	// no polygon meshes intersected the child node.
	return child_node;
}


GPlatesOpenGL::GLReconstructedStaticPolygonMeshes::PresentDayPolygonMeshesNodeIntersections::intersection_partition_type::node_type &
GPlatesOpenGL::GLReconstructedStaticPolygonMeshes::PresentDayPolygonMeshesNodeIntersections::get_or_create_quad_tree_root_node(
		GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face)
{
	// Get the intersections quad tree root node.
	intersection_partition_type::node_type *intersections_quad_tree_root_node =
			d_intersection_partition->get_quad_tree_root_node(cube_face);
	if (!intersections_quad_tree_root_node)
	{
		// Set the quad tree root node if we're the first to access it.
		intersections_quad_tree_root_node =
				&d_intersection_partition->set_quad_tree_root_node(
						cube_face,
						Node(
								PresentDayPolygonMeshMembership::create(d_num_polygon_meshes),
								0/*depth*/));
	}

	return *intersections_quad_tree_root_node;
}


GPlatesOpenGL::GLReconstructedStaticPolygonMeshes::PresentDayPolygonMeshesNodeIntersections::intersection_partition_type::node_type &
GPlatesOpenGL::GLReconstructedStaticPolygonMeshes::PresentDayPolygonMeshesNodeIntersections::get_or_create_child_node(
		intersection_partition_type::node_type &parent_node,
		unsigned int child_x_offset,
		unsigned int child_y_offset)
{
	// Get the child intersections quad tree node.
	intersection_partition_type::node_type *child_node =
			parent_node.get_child_node(child_x_offset, child_y_offset);
	if (!child_node)
	{
		// Set the child quad tree node if we're the first to access it.
		child_node = &d_intersection_partition->set_child_node(
				parent_node,
				child_x_offset,
				child_y_offset,
				Node(
						PresentDayPolygonMeshMembership::create(d_num_polygon_meshes),
						parent_node.get_element().d_quad_tree_depth + 1));
	}

	return *child_node;
}


GPlatesOpenGL::GLReconstructedStaticPolygonMeshes::PresentDayPolygonMeshMembership::non_null_ptr_type
GPlatesOpenGL::GLReconstructedStaticPolygonMeshes::ReconstructedPolygonMeshTransformsGroups
	::gather_present_day_polygon_mesh_memberships_for_active_reconstructions(
		const reconstructed_polygon_mesh_transform_group_seq_type &transform_groups,
		unsigned int num_polygon_meshes)
{
	// All bitset's have the same number of flags.
	// Initially they are all false.
	boost::dynamic_bitset<> polygon_mesh_membership(num_polygon_meshes);

	// Combine the flags of all transform groups.
	BOOST_FOREACH(const ReconstructedPolygonMeshTransformGroup &transform_group, transform_groups)
	{
		polygon_mesh_membership |=
				transform_group.get_present_day_polygon_meshes_for_active_reconstructions()
						.get_polygon_meshes_membership();
	}

	return PresentDayPolygonMeshMembership::create(polygon_mesh_membership);
}


GPlatesOpenGL::GLReconstructedStaticPolygonMeshes::PresentDayPolygonMeshMembership::non_null_ptr_type
GPlatesOpenGL::GLReconstructedStaticPolygonMeshes::ReconstructedPolygonMeshTransformsGroups
	::gather_present_day_polygon_mesh_memberships_for_active_or_inactive_reconstructions(
		const reconstructed_polygon_mesh_transform_group_seq_type &transform_groups,
		unsigned int num_polygon_meshes)
{
	// All bitset's have the same number of flags.
	// Initially they are all false.
	boost::dynamic_bitset<> polygon_mesh_membership(num_polygon_meshes);

	// Combine the flags of all transform groups.
	BOOST_FOREACH(const ReconstructedPolygonMeshTransformGroup &transform_group, transform_groups)
	{
		polygon_mesh_membership |=
				transform_group.get_present_day_polygon_meshes_for_active_or_inactive_reconstructions()
						.get_polygon_meshes_membership();
	}

	return PresentDayPolygonMeshMembership::create(polygon_mesh_membership);
}
