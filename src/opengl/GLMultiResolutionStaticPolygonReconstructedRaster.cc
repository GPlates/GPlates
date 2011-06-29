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

#include <cmath>

#include "global/CompilerWarnings.h"

#include <boost/cast.hpp>
#include <boost/foreach.hpp>
/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>
#include <opengl/OpenGL.h>

#include "GLMultiResolutionStaticPolygonReconstructedRaster.h"

#include "GLBlendState.h"
#include "GLClearBuffers.h"
#include "GLClearBuffersState.h"
#include "GLCompositeStateSet.h"
#include "GLContext.h"
#include "GLDepthRangeState.h"
#include "GLFragmentTestStates.h"
#include "GLIntersect.h"
#include "GLMatrix.h"
#include "GLPointLinePolygonState.h"
#include "GLRenderer.h"
#include "GLTextureEnvironmentState.h"
#include "GLTransformState.h"
#include "GLUtils.h"
#include "GLVertex.h"
#include "GLViewport.h"
#include "GLViewportState.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "gui/Colour.h"

#include "utils/Profile.h"


namespace
{
	//! The inverse of log(2.0).
	const float INVERSE_LOG2 = 1.0 / std::log(2.0);
}


GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::GLMultiResolutionStaticPolygonReconstructedRaster(
		const GLMultiResolutionCubeRaster::non_null_ptr_type &raster_to_reconstruct,
		const GLReconstructedStaticPolygonMeshes::non_null_ptr_type &reconstructed_static_polygon_meshes,
		const cube_subdivision_projection_transforms_cache_type::non_null_ptr_type &
				cube_subdivision_projection_transforms_cache,
		const cube_subdivision_bounds_cache_type::non_null_ptr_type &cube_subdivision_bounds_cache,
		const GLTextureResourceManager::shared_ptr_type &texture_resource_manager,
		const GLVertexBufferResourceManager::shared_ptr_type &vertex_buffer_resource_manager,
		boost::optional<AgeGridParams> age_grid_params) :
	d_raster_to_reconstruct(raster_to_reconstruct),
	d_reconstructed_static_polygon_meshes(reconstructed_static_polygon_meshes),
	d_age_grid_params(age_grid_params),
	d_cube_subdivision_projection_transforms_cache(cube_subdivision_projection_transforms_cache),
	d_cube_subdivision_bounds_cache(cube_subdivision_bounds_cache),
	d_tile_texel_dimension(cube_subdivision_projection_transforms_cache->get_tile_texel_dimension()),
	d_texture_resource_manager(texture_resource_manager),
	d_vertex_buffer_resource_manager(vertex_buffer_resource_manager),
	d_age_masked_texture_cache(GPlatesUtils::ObjectCache<GLTexture>::create(200)),
	// NOTE: An OpenGL context must be active at this point...
	d_xy_clip_texture(GLTextureUtils::create_xy_clip_texture(texture_resource_manager)),
	d_z_clip_texture(GLTextureUtils::create_z_clip_texture(texture_resource_manager)),
	d_xy_clip_texture_transform(GLTextureUtils::get_clip_texture_clip_space_to_texture_space_transform()),
	d_full_screen_quad_drawable(GLUtils::create_full_screen_2D_textured_quad()),
	d_cube_quad_tree(cube_quad_tree_type::create())
{
}


unsigned int
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::get_level_of_detail(
		const GLTransformState &transform_state) const
{
	// Get the minimum size of a pixel in the current viewport when projected
	// onto the unit sphere (in model space).
	const double min_pixel_size_on_unit_sphere =
			transform_state.get_min_pixel_size_on_unit_sphere();

	//
	// Calculate the level-of-detail.
	// This is the equivalent of:
	//
	//    t = t0 * 2 ^ (-lod)
	//
	// ...where 't0' is the texel size of the *lowest* resolution level-of-detail
	// (note that this is the opposite to GLMultiResolutionRaster where it's the *highest*)
	// and 't' is the projected size of a pixel of the viewport.
	//

	// The maximum texel size of any texel projected onto the unit sphere occurs at the centre
	// of the cube faces. Not all cube subdivisions occur at the face centres but the projected
	// texel size will always be less that at the face centre so at least it's bounded and the
	// variation across the cube face is not that large so we shouldn't be using a level-of-detail
	// that is much higher than what we need.
	const float max_lowest_resolution_texel_size_on_unit_sphere =
			2.0 / d_cube_subdivision_projection_transforms_cache->get_tile_texel_dimension();

	const float level_of_detail_factor = INVERSE_LOG2 *
			(std::log(max_lowest_resolution_texel_size_on_unit_sphere) -
					std::log(min_pixel_size_on_unit_sphere));

	// We need to round up instead of down and then clamp to zero.
	// We don't have an upper limit - as we traverse the quad tree to higher and higher
	// resolution nodes we might eventually reach the leaf nodes of the tree without
	// having satisfied the requested level-of-detail resolution - in this case we'll
	// just render the leaf nodes as that's the highest we can provide.
	int level_of_detail = static_cast<int>(level_of_detail_factor + 0.99f);
	// Clamp to lowest resolution level of detail.
	if (level_of_detail < 0)
	{
		// If we get there then even our lowest resolution level of detail
		// had too much resolution - but this is pretty unlikely for all but the very
		// smallest of viewports.
		level_of_detail = 0;
	}

	return boost::numeric_cast<unsigned int>(level_of_detail);
}


void
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::render(
		GLRenderer &renderer)
{
	//PROFILE_FUNC();

	// Get the level-of-detail based on the size of viewport pixels projected onto the globe.
	// We'll try to render of this level of detail if our quad tree is deep enough.
	const unsigned int render_level_of_detail = get_level_of_detail(renderer.get_transform_state());

	// The polygon mesh drawables and polygon mesh cube quad tree node intersections.
	const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables =
			d_reconstructed_static_polygon_meshes->get_present_day_polygon_mesh_drawables();
	const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections =
			d_reconstructed_static_polygon_meshes->get_present_day_polygon_mesh_node_intersections();

	// Get the transform groups of reconstructed polygon meshes that are visible in the view frustum.
	reconstructed_polygon_mesh_transform_groups_type::non_null_ptr_to_const_type
			reconstructed_polygon_mesh_transform_groups =
					d_reconstructed_static_polygon_meshes->get_reconstructed_polygon_meshes(
							// Determines the view frustum...
							renderer.get_transform_state());

	// The source raster cube quad tree.
	const raster_cube_quad_tree_type &raster_cube_quad_tree = d_raster_to_reconstruct->get_cube_quad_tree();

	// Used to cache information that can be reused as we traverse the source raster for each
	// transform in the reconstructed polygon meshes.
	render_traversal_cube_quad_tree_type::non_null_ptr_type render_traversal_cube_quad_tree =
			render_traversal_cube_quad_tree_type::create();

	// Iterate over the transform groups and traverse the cube quad tree separately for each transform.
	BOOST_FOREACH(
			const reconstructed_polygon_mesh_transform_group_type &reconstructed_polygon_mesh_transform_group,
			reconstructed_polygon_mesh_transform_groups->get_transform_groups())
	{
		// Push the current finite rotation transform.
		renderer.push_transform(*reconstructed_polygon_mesh_transform_group.get_finite_rotation());

		// First get the view frustum planes.
		//
		// NOTE: We do this *after* pushing the above rotation transform because the
		// frustum planes are affected by the current model-view and projection transforms.
		// Our quad tree bounding boxes are in model space but the polygon meshes they
		// bound are rotating to new positions so we want to take that into account and map
		// the view frustum back to model space where we can test against our bounding boxes.
		//
		const GLFrustum &frustum_planes = renderer.get_transform_state().get_current_frustum_planes_in_model_space();

		//
		// Traverse the source raster cube quad tree and the spatial partition of reconstructed polygon meshes.
		//

		// Traverse the quad trees of the cube faces.
		for (unsigned int face = 0; face < 6; ++face)
		{
			const GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face =
					static_cast<GPlatesMaths::CubeCoordinateFrame::CubeFaceType>(face);

			// Get the quad tree root node of the current cube face of the source raster.
			const raster_cube_quad_tree_type::node_type *source_raster_quad_tree_root =
					raster_cube_quad_tree.get_quad_tree_root_node(cube_face);
			// If there is no source raster for the current cube face then continue to the next face.
			if (source_raster_quad_tree_root == NULL)
			{
				continue;
			}

			// This is used to find those nodes that intersect the present day polygon meshes of
			// the reconstructed polygon meshes in the current transform group.
			// This is so we know which polygon meshes to draw for each source raster tile.
			const present_day_polygon_meshes_intersection_partition_type::node_type *
					intersections_quad_tree_root_node =
							polygon_mesh_node_intersections.get_quad_tree_root_node(cube_face);
			if (!intersections_quad_tree_root_node)
			{
				// No polygon meshes intersect the current cube face.
				continue;
			}

			// Get, or create, the root cube quad tree node.
			cube_quad_tree_type::node_type *cube_quad_tree_root = d_cube_quad_tree->get_quad_tree_root_node(cube_face);
			if (!cube_quad_tree_root)
			{
				cube_quad_tree_root = &d_cube_quad_tree->set_quad_tree_root_node(
						cube_face,
						QuadTreeNode(d_age_masked_texture_cache->allocate_volatile_object()));
			}

			// Get, or create, the root render traversal node.
			render_traversal_cube_quad_tree_type::node_type &render_traversal_cube_quad_tree_root =
					render_traversal_cube_quad_tree->get_or_create_quad_tree_root_node(cube_face);

			// Get the root projection transforms node.
			const cube_subdivision_projection_transforms_cache_type::node_reference_type
					projection_transforms_cache_root_node =
							d_cube_subdivision_projection_transforms_cache->get_quad_tree_root_node(cube_face);

			// Get the root bounds node.
			const cube_subdivision_bounds_cache_type::node_reference_type
					bounds_cache_root_node =
							d_cube_subdivision_bounds_cache->get_quad_tree_root_node(cube_face);

			// If we're using an age grid to smooth the reconstruction then perform a slightly
			// different quad tree traversal code path.
			if (d_age_grid_params)
			{
				// Get the quad tree root node of the current cube face of the age grid mask/coverage rasters.
				const age_grid_mask_cube_quad_tree_type::node_type *age_grid_mask_quad_tree_root =
						d_age_grid_params->age_grid_mask_cube_raster->get_cube_quad_tree()
								.get_quad_tree_root_node(cube_face);
				const age_grid_coverage_cube_quad_tree_type::node_type *age_grid_coverage_quad_tree_root =
						d_age_grid_params->age_grid_coverage_cube_raster->get_cube_quad_tree()
								.get_quad_tree_root_node(cube_face);

				// If there is an age grid mask/coverage raster for the current cube face then...
				if (age_grid_mask_quad_tree_root && age_grid_coverage_quad_tree_root)
				{
					render_quad_tree_source_raster_and_age_grid(
							renderer,
							*cube_quad_tree_root,
							*render_traversal_cube_quad_tree,
							render_traversal_cube_quad_tree_root,
							*source_raster_quad_tree_root,
							*age_grid_mask_quad_tree_root,
							*age_grid_coverage_quad_tree_root,
							*reconstructed_polygon_mesh_transform_groups,
							reconstructed_polygon_mesh_transform_group,
							polygon_mesh_drawables,
							polygon_mesh_node_intersections,
							*intersections_quad_tree_root_node,
							projection_transforms_cache_root_node,
							bounds_cache_root_node,
							0/*level_of_detail*/,
							render_level_of_detail,
							frustum_planes,
							// There are six frustum planes initially active
							GLFrustum::ALL_PLANES_ACTIVE_MASK);

					// Continue to the next cube face.
					continue;
				}
				// Fall through to render without and age grid...
			}

			// Render without using an age grid.
			render_quad_tree_source_raster(
					renderer,
					*render_traversal_cube_quad_tree,
					render_traversal_cube_quad_tree_root,
					*source_raster_quad_tree_root,
					reconstructed_polygon_mesh_transform_group,
					polygon_mesh_drawables,
					polygon_mesh_node_intersections,
					*intersections_quad_tree_root_node,
					projection_transforms_cache_root_node,
					bounds_cache_root_node,
					0/*level_of_detail*/,
					render_level_of_detail,
					frustum_planes,
					// There are six frustum planes initially active
					GLFrustum::ALL_PLANES_ACTIVE_MASK);
		}

		renderer.pop_transform();
	}
}


void
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::render_quad_tree_source_raster(
		GLRenderer &renderer,
		render_traversal_cube_quad_tree_type &render_traversal_cube_quad_tree,
		render_traversal_cube_quad_tree_type::node_type &render_traversal_cube_quad_tree_node,
		const raster_cube_quad_tree_type::node_type &source_raster_quad_tree_node,
		const reconstructed_polygon_mesh_transform_group_type &reconstructed_polygon_mesh_transform_group,
		const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables,
		const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
		const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
		const cube_subdivision_projection_transforms_cache_type::node_reference_type &projection_transforms_cache_node,
		const cube_subdivision_bounds_cache_type::node_reference_type &bounds_cache_node,
		unsigned int level_of_detail,
		unsigned int render_level_of_detail,
		const GLFrustum &frustum_planes,
		boost::uint32_t frustum_plane_mask)
{
	// If the current quad tree node does not intersect any polygon meshes in the current rotation
	// group or the current quad tree node is outside the view frustum then we can return early.
	if (cull_quad_tree(
			// We're rendering using only *active* reconstructed polygons.
			reconstructed_polygon_mesh_transform_group.get_visible_present_day_polygon_meshes_for_active_reconstructions(),
			polygon_mesh_node_intersections,
			intersections_quad_tree_node,
			bounds_cache_node,
			frustum_planes,
			frustum_plane_mask))
	{
		return;
	}

	// If either:
	// - we're at the correct level of detail for rendering, or
	// - we've reached a leaf node of the source raster (highest resolution supplied the source raster),
	// ...then render the current source raster tile.
	if (level_of_detail == render_level_of_detail ||
		source_raster_quad_tree_node.get_element().is_leaf_node())
	{
		render_source_raster_tile_to_scene(
				renderer,
				render_traversal_cube_quad_tree_node,
				source_raster_quad_tree_node,
				// Note that there's no uv scaling of the source raster because we're at the
				// source raster leaf node (and are not traversing deeper)...
				GLUtils::QuadTreeUVTransform()/*source_raster_uv_transform*/,
				reconstructed_polygon_mesh_transform_group,
				polygon_mesh_node_intersections,
				intersections_quad_tree_node,
				polygon_mesh_drawables,
				projection_transforms_cache_node);

		return;
	}

	//
	// Iterate over the child quad tree nodes.
	//

	for (unsigned int child_v_offset = 0; child_v_offset < 2; ++child_v_offset)
	{
		for (unsigned int child_u_offset = 0; child_u_offset < 2; ++child_u_offset)
		{
			// Get the child node of the current source raster quad tree node.
			const raster_cube_quad_tree_type::node_type *child_source_raster_quad_tree_node =
					source_raster_quad_tree_node.get_child_node(child_u_offset, child_v_offset);

			// If there is no source raster for the current child then continue to the next child.
			// This happens if the current child is not covered by the source raster.
			// Note that if we get here then the current parent is not a leaf node.
			if (child_source_raster_quad_tree_node == NULL)
			{
				continue;
			}

			// This is used to find those nodes that intersect the present day polygon meshes of
			// the reconstructed polygon meshes in the current transform group.
			// This is so we know which polygon meshes to draw for each source raster tile.
			const present_day_polygon_meshes_intersection_partition_type::node_type *
					child_intersections_quad_tree_node =
							polygon_mesh_node_intersections.get_child_node(
									intersections_quad_tree_node,
									child_u_offset,
									child_v_offset);
			if (!child_intersections_quad_tree_node)
			{
				// No polygon meshes intersect the current quad tree node.
				continue;
			}

			// Get, or create, the child render traversal node.
			render_traversal_cube_quad_tree_type::node_type &child_render_traversal_cube_quad_tree_node =
					render_traversal_cube_quad_tree.get_or_create_child_node(
							render_traversal_cube_quad_tree_node, child_u_offset, child_v_offset);

			// Get the child projection transforms node.
			const cube_subdivision_projection_transforms_cache_type::node_reference_type
					child_projection_transforms_cache_node =
							d_cube_subdivision_projection_transforms_cache->get_child_node(
									projection_transforms_cache_node,
									child_u_offset,
									child_v_offset);

			// Get the child bounds node.
			const cube_subdivision_bounds_cache_type::node_reference_type
					child_bounds_cache_node =
							d_cube_subdivision_bounds_cache->get_child_node(
									bounds_cache_node,
									child_u_offset,
									child_v_offset);

			render_quad_tree_source_raster(
					renderer,
					render_traversal_cube_quad_tree,
					child_render_traversal_cube_quad_tree_node,
					*child_source_raster_quad_tree_node,
					reconstructed_polygon_mesh_transform_group,
					polygon_mesh_drawables,
					polygon_mesh_node_intersections,
					*child_intersections_quad_tree_node,
					child_projection_transforms_cache_node,
					child_bounds_cache_node,
					level_of_detail + 1,
					render_level_of_detail,
					frustum_planes,
					frustum_plane_mask);
		}
	}
}


void
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::render_quad_tree_scaled_source_raster_and_unscaled_age_grid(
		GLRenderer &renderer,
		cube_quad_tree_type::node_type &cube_quad_tree_node,
		render_traversal_cube_quad_tree_type &render_traversal_cube_quad_tree,
		render_traversal_cube_quad_tree_type::node_type &render_traversal_cube_quad_tree_node,
		const raster_cube_quad_tree_type::node_type &source_raster_quad_tree_node,
		const GLUtils::QuadTreeUVTransform &source_raster_uv_transform,
		const age_grid_mask_cube_quad_tree_type::node_type &age_grid_mask_quad_tree_node,
		const age_grid_coverage_cube_quad_tree_type::node_type &age_grid_coverage_quad_tree_node,
		const reconstructed_polygon_mesh_transform_groups_type &reconstructed_polygon_mesh_transform_groups,
		const reconstructed_polygon_mesh_transform_group_type &reconstructed_polygon_mesh_transform_group,
		const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables,
		const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
		const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
		const cube_subdivision_projection_transforms_cache_type::node_reference_type &projection_transforms_cache_node,
		const cube_subdivision_bounds_cache_type::node_reference_type &bounds_cache_node,
		unsigned int level_of_detail,
		unsigned int render_level_of_detail,
		const GLFrustum &frustum_planes,
		boost::uint32_t frustum_plane_mask)
{
	//PROFILE_FUNC();

	// If the current quad tree node does not intersect any polygon meshes in the current rotation
	// group or the current quad tree node is outside the view frustum then we can return early.
	if (cull_quad_tree(
			// We're rendering using active *and* inactive reconstructed polygons since the
			// age grid now decides the begin times of oceanic crust.
			reconstructed_polygon_mesh_transform_group.get_visible_present_day_polygon_meshes_for_active_or_inactive_reconstructions(),
			polygon_mesh_node_intersections,
			intersections_quad_tree_node,
			bounds_cache_node,
			frustum_planes,
			frustum_plane_mask))
	{
		return;
	}

	// If either:
	// - we're at the correct level of detail for rendering, or
	// - we've reached a leaf node of the age grid mask/coverage rasters (highest resolution supplied),
	// ...then render the current source raster tile.
	// Note that we've already reached the highest resolution of the source raster which is why
	// we are uv scaling it.
	if (
			level_of_detail == render_level_of_detail ||
			(
				age_grid_mask_quad_tree_node.get_element().is_leaf_node() ||
				age_grid_coverage_quad_tree_node.get_element().is_leaf_node()
			)
		)
	{
		render_source_raster_and_age_grid_tile_to_scene(
				renderer,
				cube_quad_tree_node,
				render_traversal_cube_quad_tree_node,
				source_raster_quad_tree_node,
				source_raster_uv_transform,
				age_grid_mask_quad_tree_node,
				age_grid_coverage_quad_tree_node,
				// Note that there's no uv scaling of the age grid because we're at the
				// age grid leaf node (and are not traversing deeper)...
				GLUtils::QuadTreeUVTransform()/*age_grid_uv_transform*/,
				reconstructed_polygon_mesh_transform_groups,
				reconstructed_polygon_mesh_transform_group,
				polygon_mesh_node_intersections,
				intersections_quad_tree_node,
				polygon_mesh_drawables,
				projection_transforms_cache_node);

		return;
	}

	//
	// Iterate over the child quad tree nodes.
	//

	for (unsigned int child_v_offset = 0; child_v_offset < 2; ++child_v_offset)
	{
		for (unsigned int child_u_offset = 0; child_u_offset < 2; ++child_u_offset)
		{
			// This is used to find those nodes that intersect the present day polygon meshes of
			// the reconstructed polygon meshes in the current transform group.
			// This is so we know which polygon meshes to draw for each source raster tile.
			const present_day_polygon_meshes_intersection_partition_type::node_type *
					child_intersections_quad_tree_node =
							polygon_mesh_node_intersections.get_child_node(
									intersections_quad_tree_node,
									child_u_offset,
									child_v_offset);
			if (!child_intersections_quad_tree_node)
			{
				// No polygon meshes intersect the current quad tree node.
				continue;
			}

			// Get, or create, the child cube quad tree node.
			cube_quad_tree_type::node_type *child_cube_quad_tree_node =
					cube_quad_tree_node.get_child_node(child_u_offset, child_v_offset);
			if (!child_cube_quad_tree_node)
			{
				child_cube_quad_tree_node = &d_cube_quad_tree->set_child_node(
						cube_quad_tree_node,
						child_u_offset,
						child_v_offset,
						QuadTreeNode(d_age_masked_texture_cache->allocate_volatile_object()));
			}

			// Get, or create, the child render traversal node.
			render_traversal_cube_quad_tree_type::node_type &child_render_traversal_cube_quad_tree_node =
					render_traversal_cube_quad_tree.get_or_create_child_node(
							render_traversal_cube_quad_tree_node, child_u_offset, child_v_offset);

			// Get the child projection transforms node.
			const cube_subdivision_projection_transforms_cache_type::node_reference_type
					child_projection_transforms_cache_node =
							d_cube_subdivision_projection_transforms_cache->get_child_node(
									projection_transforms_cache_node,
									child_u_offset,
									child_v_offset);

			// Get the child bounds node.
			const cube_subdivision_bounds_cache_type::node_reference_type
					child_bounds_cache_node =
							d_cube_subdivision_bounds_cache->get_child_node(
									bounds_cache_node,
									child_u_offset,
									child_v_offset);

			// UV scaling for the child source raster node.
			const GLUtils::QuadTreeUVTransform child_source_raster_uv_transform(
					source_raster_uv_transform, child_u_offset, child_v_offset);

			// Get the child node of the current age grid mask/coverage quad tree nodes.
			const age_grid_mask_cube_quad_tree_type::node_type *child_age_grid_mask_quad_tree_node =
					age_grid_mask_quad_tree_node.get_child_node(child_u_offset, child_v_offset);
			const age_grid_coverage_cube_quad_tree_type::node_type *child_age_grid_coverage_quad_tree_node =
					age_grid_coverage_quad_tree_node.get_child_node(child_u_offset, child_v_offset);

			// If there is no age grid for the current child then render source raster without an age grid.
			// This happens if the current child is not covered by the age grid.
			// Note that if we get here then the current parent age grid node is not a leaf node.
			if (child_age_grid_mask_quad_tree_node == NULL ||
				child_age_grid_coverage_quad_tree_node == NULL)
			{
				// Render the current child node if it isn't culled.
				// Note that we're *not* continuing to traverse the quad tree because the source raster
				// has previously reached its highest resolution (which is why we are uv scaling it)
				// and so we simply render the current child directly.
				if (!cull_quad_tree(
						// We're rendering using only *active* reconstructed polygons.
						reconstructed_polygon_mesh_transform_group.get_visible_present_day_polygon_meshes_for_active_reconstructions(),
						polygon_mesh_node_intersections,
						*child_intersections_quad_tree_node,
						child_bounds_cache_node,
						frustum_planes,
						frustum_plane_mask))
				{
					render_source_raster_tile_to_scene(
							renderer,
							child_render_traversal_cube_quad_tree_node,
							source_raster_quad_tree_node,
							child_source_raster_uv_transform,
							reconstructed_polygon_mesh_transform_group,
							polygon_mesh_node_intersections,
							*child_intersections_quad_tree_node,
							polygon_mesh_drawables,
							child_projection_transforms_cache_node);
				}

				continue;
			}

			// Continue to uv scale the source raster while traversing deeper into the age grid
			// (which has not reached its maximum resolution yet).
			render_quad_tree_scaled_source_raster_and_unscaled_age_grid(
					renderer,
					*child_cube_quad_tree_node,
					render_traversal_cube_quad_tree,
					child_render_traversal_cube_quad_tree_node,
					// Note that we're uv scaling the source raster because it doesn't have children...
					source_raster_quad_tree_node,
					child_source_raster_uv_transform,
					*child_age_grid_mask_quad_tree_node,
					*child_age_grid_coverage_quad_tree_node,
					reconstructed_polygon_mesh_transform_groups,
					reconstructed_polygon_mesh_transform_group,
					polygon_mesh_drawables,
					polygon_mesh_node_intersections,
					*child_intersections_quad_tree_node,
					child_projection_transforms_cache_node,
					child_bounds_cache_node,
					level_of_detail + 1,
					render_level_of_detail,
					frustum_planes,
					frustum_plane_mask);
		}
	}
}


void
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::render_quad_tree_unscaled_source_raster_and_scaled_age_grid(
		GLRenderer &renderer,
		cube_quad_tree_type::node_type &cube_quad_tree_node,
		render_traversal_cube_quad_tree_type &render_traversal_cube_quad_tree,
		render_traversal_cube_quad_tree_type::node_type &render_traversal_cube_quad_tree_node,
		const raster_cube_quad_tree_type::node_type &source_raster_quad_tree_node,
		const age_grid_mask_cube_quad_tree_type::node_type &age_grid_mask_quad_tree_node,
		const age_grid_coverage_cube_quad_tree_type::node_type &age_grid_coverage_quad_tree_node,
		const GLUtils::QuadTreeUVTransform &age_grid_uv_transform,
		const reconstructed_polygon_mesh_transform_groups_type &reconstructed_polygon_mesh_transform_groups,
		const reconstructed_polygon_mesh_transform_group_type &reconstructed_polygon_mesh_transform_group,
		const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables,
		const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
		const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
		const cube_subdivision_projection_transforms_cache_type::node_reference_type &projection_transforms_cache_node,
		const cube_subdivision_bounds_cache_type::node_reference_type &bounds_cache_node,
		unsigned int level_of_detail,
		unsigned int render_level_of_detail,
		const GLFrustum &frustum_planes,
		boost::uint32_t frustum_plane_mask)
{
	//PROFILE_FUNC();

	// If the current quad tree node does not intersect any polygon meshes in the current rotation
	// group or the current quad tree node is outside the view frustum then we can return early.
	if (cull_quad_tree(
			// We're rendering using active *and* inactive reconstructed polygons since the
			// age grid now decides the begin times of oceanic crust.
			reconstructed_polygon_mesh_transform_group.get_visible_present_day_polygon_meshes_for_active_or_inactive_reconstructions(),
			polygon_mesh_node_intersections,
			intersections_quad_tree_node,
			bounds_cache_node,
			frustum_planes,
			frustum_plane_mask))
	{
		return;
	}

	// If either:
	// - we're at the correct level of detail for rendering, or
	// - we've reached a leaf node of the source raster (highest resolution supplied),
	// ...then render the current source raster tile.
	// Note that we've already reached the highest resolution of the age grid which is why
	// we are uv scaling it.
	if (level_of_detail == render_level_of_detail ||
		source_raster_quad_tree_node.get_element().is_leaf_node())
	{
		render_source_raster_and_age_grid_tile_to_scene(
				renderer,
				cube_quad_tree_node,
				render_traversal_cube_quad_tree_node,
				source_raster_quad_tree_node,
				// Note that there's no uv scaling of the source raster...
				GLUtils::QuadTreeUVTransform()/*source_raster_uv_transform*/,
				age_grid_mask_quad_tree_node,
				age_grid_coverage_quad_tree_node,
				age_grid_uv_transform,
				reconstructed_polygon_mesh_transform_groups,
				reconstructed_polygon_mesh_transform_group,
				polygon_mesh_node_intersections,
				intersections_quad_tree_node,
				polygon_mesh_drawables,
				projection_transforms_cache_node);

		return;
	}

	//
	// Iterate over the child quad tree nodes.
	//

	for (unsigned int child_v_offset = 0; child_v_offset < 2; ++child_v_offset)
	{
		for (unsigned int child_u_offset = 0; child_u_offset < 2; ++child_u_offset)
		{
			// This is used to find those nodes that intersect the present day polygon meshes of
			// the reconstructed polygon meshes in the current transform group.
			// This is so we know which polygon meshes to draw for each source raster tile.
			const present_day_polygon_meshes_intersection_partition_type::node_type *
					child_intersections_quad_tree_node =
							polygon_mesh_node_intersections.get_child_node(
									intersections_quad_tree_node,
									child_u_offset,
									child_v_offset);
			if (!child_intersections_quad_tree_node)
			{
				// No polygon meshes intersect the current quad tree node.
				continue;
			}

			// Get the child node of the current source raster quad tree node.
			const raster_cube_quad_tree_type::node_type *child_source_raster_quad_tree_node =
					source_raster_quad_tree_node.get_child_node(child_u_offset, child_v_offset);

			// If there is no source raster for the current child then continue to the next child.
			// This happens if the current child is not covered by the source raster.
			// Note that if we get here then the current parent is not a leaf node.
			if (child_source_raster_quad_tree_node == NULL)
			{
				continue;
			}

			// Get, or create, the child cube quad tree node.
			cube_quad_tree_type::node_type *child_cube_quad_tree_node =
					cube_quad_tree_node.get_child_node(child_u_offset, child_v_offset);
			if (!child_cube_quad_tree_node)
			{
				child_cube_quad_tree_node = &d_cube_quad_tree->set_child_node(
						cube_quad_tree_node,
						child_u_offset,
						child_v_offset,
						QuadTreeNode(d_age_masked_texture_cache->allocate_volatile_object()));
			}

			// Get, or create, the child render traversal node.
			render_traversal_cube_quad_tree_type::node_type &child_render_traversal_cube_quad_tree_node =
					render_traversal_cube_quad_tree.get_or_create_child_node(
							render_traversal_cube_quad_tree_node, child_u_offset, child_v_offset);

			// Get the child projection transforms node.
			const cube_subdivision_projection_transforms_cache_type::node_reference_type
					child_projection_transforms_cache_node =
							d_cube_subdivision_projection_transforms_cache->get_child_node(
									projection_transforms_cache_node,
									child_u_offset,
									child_v_offset);

			// Get the child bounds node.
			const cube_subdivision_bounds_cache_type::node_reference_type
					child_bounds_cache_node =
							d_cube_subdivision_bounds_cache->get_child_node(
									bounds_cache_node,
									child_u_offset,
									child_v_offset);

			// UV scaling for the child age grid node.
			const GLUtils::QuadTreeUVTransform child_age_grid_uv_transform(
					age_grid_uv_transform, child_u_offset, child_v_offset);

			// Continue to uv scale the source raster while traversing deeper into the age grid
			// (which has not reached its maximum resolution yet).
			render_quad_tree_unscaled_source_raster_and_scaled_age_grid(
					renderer,
					*child_cube_quad_tree_node,
					render_traversal_cube_quad_tree,
					child_render_traversal_cube_quad_tree_node,
					*child_source_raster_quad_tree_node,
					// Note that we're uv scaling the age grid because it doesn't have children...
					age_grid_mask_quad_tree_node,
					age_grid_coverage_quad_tree_node,
					child_age_grid_uv_transform,
					reconstructed_polygon_mesh_transform_groups,
					reconstructed_polygon_mesh_transform_group,
					polygon_mesh_drawables,
					polygon_mesh_node_intersections,
					*child_intersections_quad_tree_node,
					child_projection_transforms_cache_node,
					child_bounds_cache_node,
					level_of_detail + 1,
					render_level_of_detail,
					frustum_planes,
					frustum_plane_mask);
		}
	}
}


void
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::render_quad_tree_source_raster_and_age_grid(
		GLRenderer &renderer,
		cube_quad_tree_type::node_type &cube_quad_tree_node,
		render_traversal_cube_quad_tree_type &render_traversal_cube_quad_tree,
		render_traversal_cube_quad_tree_type::node_type &render_traversal_cube_quad_tree_node,
		const raster_cube_quad_tree_type::node_type &source_raster_quad_tree_node,
		const age_grid_mask_cube_quad_tree_type::node_type &age_grid_mask_quad_tree_node,
		const age_grid_coverage_cube_quad_tree_type::node_type &age_grid_coverage_quad_tree_node,
		const reconstructed_polygon_mesh_transform_groups_type &reconstructed_polygon_mesh_transform_groups,
		const reconstructed_polygon_mesh_transform_group_type &reconstructed_polygon_mesh_transform_group,
		const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables,
		const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
		const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
		const cube_subdivision_projection_transforms_cache_type::node_reference_type &projection_transforms_cache_node,
		const cube_subdivision_bounds_cache_type::node_reference_type &bounds_cache_node,
		unsigned int level_of_detail,
		unsigned int render_level_of_detail,
		const GLFrustum &frustum_planes,
		boost::uint32_t frustum_plane_mask)
{
	// If the current quad tree node does not intersect any polygon meshes in the current rotation
	// group or the current quad tree node is outside the view frustum then we can return early.
	if (cull_quad_tree(
			// We're rendering using active *and* inactive reconstructed polygons since the
			// age grid now decides the begin times of oceanic crust.
			reconstructed_polygon_mesh_transform_group.get_visible_present_day_polygon_meshes_for_active_or_inactive_reconstructions(),
			polygon_mesh_node_intersections,
			intersections_quad_tree_node,
			bounds_cache_node,
			frustum_planes,
			frustum_plane_mask))
	{
		return;
	}

	// If either:
	// - we're at the correct level of detail for rendering, or
	// - we've reached a leaf node of the source raster (highest resolution supplied the source raster) *and*
	//   we've reached a leaf node of the age grid mask/coverage rasters,
	// ...then render the current source raster tile.
	// The second condition means the source raster and the age grid rasters reached their
	// maximum resolutions at the same quad tree depth.
	if (
			level_of_detail == render_level_of_detail ||
			(
				source_raster_quad_tree_node.get_element().is_leaf_node() &&
				(
					age_grid_mask_quad_tree_node.get_element().is_leaf_node() ||
					age_grid_coverage_quad_tree_node.get_element().is_leaf_node()
				)
			)
		)
	{
		render_source_raster_and_age_grid_tile_to_scene(
				renderer,
				cube_quad_tree_node,
				render_traversal_cube_quad_tree_node,
				source_raster_quad_tree_node,
				// Note that there's no uv scaling of the source raster...
				GLUtils::QuadTreeUVTransform()/*source_raster_uv_transform*/,
				age_grid_mask_quad_tree_node,
				age_grid_coverage_quad_tree_node,
				// Note that there's no uv scaling of the age grid...
				GLUtils::QuadTreeUVTransform()/*age_grid_uv_transform*/,
				reconstructed_polygon_mesh_transform_groups,
				reconstructed_polygon_mesh_transform_group,
				polygon_mesh_node_intersections,
				intersections_quad_tree_node,
				polygon_mesh_drawables,
				projection_transforms_cache_node);

		return;
	}

	//
	// Iterate over the child quad tree nodes.
	//

	for (unsigned int child_v_offset = 0; child_v_offset < 2; ++child_v_offset)
	{
		for (unsigned int child_u_offset = 0; child_u_offset < 2; ++child_u_offset)
		{
			// This is used to find those nodes that intersect the present day polygon meshes of
			// the reconstructed polygon meshes in the current transform group.
			// This is so we know which polygon meshes to draw for each source raster tile.
			const present_day_polygon_meshes_intersection_partition_type::node_type *
					child_intersections_quad_tree_node =
							polygon_mesh_node_intersections.get_child_node(
									intersections_quad_tree_node,
									child_u_offset,
									child_v_offset);
			if (!child_intersections_quad_tree_node)
			{
				// No polygon meshes intersect the current quad tree node.
				continue;
			}

			// Get, or create, the child cube quad tree node.
			cube_quad_tree_type::node_type *child_cube_quad_tree_node =
					cube_quad_tree_node.get_child_node(child_u_offset, child_v_offset);
			if (!child_cube_quad_tree_node)
			{
				child_cube_quad_tree_node = &d_cube_quad_tree->set_child_node(
						cube_quad_tree_node,
						child_u_offset,
						child_v_offset,
						QuadTreeNode(d_age_masked_texture_cache->allocate_volatile_object()));
			}

			// Get, or create, the child render traversal node.
			render_traversal_cube_quad_tree_type::node_type &child_render_traversal_cube_quad_tree_node =
					render_traversal_cube_quad_tree.get_or_create_child_node(
							render_traversal_cube_quad_tree_node, child_u_offset, child_v_offset);

			// Get the child projection transforms node.
			const cube_subdivision_projection_transforms_cache_type::node_reference_type
					child_projection_transforms_cache_node =
							d_cube_subdivision_projection_transforms_cache->get_child_node(
									projection_transforms_cache_node,
									child_u_offset,
									child_v_offset);

			// Get the child bounds node.
			const cube_subdivision_bounds_cache_type::node_reference_type
					child_bounds_cache_node =
							d_cube_subdivision_bounds_cache->get_child_node(
									bounds_cache_node,
									child_u_offset,
									child_v_offset);

			// Get the child node of the current age grid mask/coverage quad tree nodes.
			// NOTE: These could be NULL.
			const age_grid_mask_cube_quad_tree_type::node_type *child_age_grid_mask_quad_tree_node =
					age_grid_mask_quad_tree_node.get_child_node(child_u_offset, child_v_offset);
			const age_grid_coverage_cube_quad_tree_type::node_type *child_age_grid_coverage_quad_tree_node =
					age_grid_coverage_quad_tree_node.get_child_node(child_u_offset, child_v_offset);

			if (source_raster_quad_tree_node.get_element().is_leaf_node())
			{
				// If the source raster parent node is a leaf node then it has no children.
				// So we need to start scaling the source raster tile texture as our children
				// tiles get smaller and smaller the deeper we go.
				GLUtils::QuadTreeUVTransform child_source_raster_uv_transform(child_u_offset, child_v_offset);

				// NOTE: The age grid has not yet reached a leaf node otherwise we wouldn't be here.
				// The age grid can however have one or more NULL children if the age grid does
				// not cover some child nodes.

				// If there is no age grid for the current child then render source raster without an age grid.
				// This happens if the current child is not covered by the age grid.
				// Note that if we get here then the current parent age grid node is *not* a leaf node
				// (because the parent source raster node is a leaf node and only one of source raster
				// or age grid can be a leaf node - otherwise we wouldn't be here).
				if (child_age_grid_mask_quad_tree_node == NULL ||
					child_age_grid_coverage_quad_tree_node == NULL)
				{
					// Render the current child node if it isn't culled.
					// Note that we're *not* continuing to traverse the quad tree because the source raster
					// has reached its highest resolution (which is why we are uv scaling it)
					// and so we simply render the current child directly.
					if (!cull_quad_tree(
							// We're rendering using only *active* reconstructed polygons.
							reconstructed_polygon_mesh_transform_group.get_visible_present_day_polygon_meshes_for_active_reconstructions(),
							polygon_mesh_node_intersections,
							*child_intersections_quad_tree_node,
							child_bounds_cache_node,
							frustum_planes,
							frustum_plane_mask))
					{
						render_source_raster_tile_to_scene(
								renderer,
								child_render_traversal_cube_quad_tree_node,
								source_raster_quad_tree_node,
								child_source_raster_uv_transform,
								reconstructed_polygon_mesh_transform_group,
								polygon_mesh_node_intersections,
								*child_intersections_quad_tree_node,
								polygon_mesh_drawables,
								child_projection_transforms_cache_node);
					}

					continue;
				}

				// Start uv scaling the source raster.
				render_quad_tree_scaled_source_raster_and_unscaled_age_grid(
						renderer,
						*child_cube_quad_tree_node,
						render_traversal_cube_quad_tree,
						child_render_traversal_cube_quad_tree_node,
						source_raster_quad_tree_node,
						child_source_raster_uv_transform,
						*child_age_grid_mask_quad_tree_node,
						*child_age_grid_coverage_quad_tree_node,
						reconstructed_polygon_mesh_transform_groups,
						reconstructed_polygon_mesh_transform_group,
						polygon_mesh_drawables,
						polygon_mesh_node_intersections,
						*child_intersections_quad_tree_node,
						child_projection_transforms_cache_node,
						child_bounds_cache_node,
						level_of_detail + 1,
						render_level_of_detail,
						frustum_planes,
						frustum_plane_mask);

				continue;
			}

			// Get the child node of the current source raster quad tree node.
			const raster_cube_quad_tree_type::node_type *child_source_raster_quad_tree_node =
					source_raster_quad_tree_node.get_child_node(child_u_offset, child_v_offset);

			// If there is no source raster for the current child then continue to the next child.
			// This happens if the current child is not covered by the source raster.
			// Note that if we get here then the current parent is not a leaf node.
			if (child_source_raster_quad_tree_node == NULL)
			{
				continue;
			}

			// Note that both mask and coverage nodes will be leaf nodes at the same time but
			// we'll test both just to make sure.
			if (age_grid_mask_quad_tree_node.get_element().is_leaf_node() ||
				age_grid_coverage_quad_tree_node.get_element().is_leaf_node())
			{
				// If the age grid parent node is a leaf node then it has no children.
				// So we need to start scaling the age grid tile textures as our children
				// tiles get smaller and smaller the deeper we go.
				GLUtils::QuadTreeUVTransform child_age_grid_uv_transform(child_u_offset, child_v_offset);

				// NOTE: The source raster has not yet reached a leaf node otherwise we wouldn't be here.

				// Start uv scaling the age grid.
				render_quad_tree_unscaled_source_raster_and_scaled_age_grid(
						renderer,
						*child_cube_quad_tree_node,
						render_traversal_cube_quad_tree,
						child_render_traversal_cube_quad_tree_node,
						*child_source_raster_quad_tree_node,
						// Note that we're uv scaling the age grid because it doesn't have children...
						age_grid_mask_quad_tree_node,
						age_grid_coverage_quad_tree_node,
						child_age_grid_uv_transform,
						reconstructed_polygon_mesh_transform_groups,
						reconstructed_polygon_mesh_transform_group,
						polygon_mesh_drawables,
						polygon_mesh_node_intersections,
						*child_intersections_quad_tree_node,
						child_projection_transforms_cache_node,
						child_bounds_cache_node,
						level_of_detail + 1,
						render_level_of_detail,
						frustum_planes,
						frustum_plane_mask);

				continue;
			}

			// If there is no age grid for the current child then render source raster without an age grid.
			// This happens if the current child is not covered by the age grid because we've
			// already verified that the parent age grid node is *not* a leaf node.
			if (child_age_grid_mask_quad_tree_node == NULL ||
				child_age_grid_coverage_quad_tree_node == NULL)
			{
				// Note that we're *are* continuing to traverse the quad tree because the source raster
				// has *not* reached its highest resolution yet.
				render_quad_tree_source_raster(
						renderer,
						render_traversal_cube_quad_tree,
						child_render_traversal_cube_quad_tree_node,
						*child_source_raster_quad_tree_node,
						reconstructed_polygon_mesh_transform_group,
						polygon_mesh_drawables,
						polygon_mesh_node_intersections,
						*child_intersections_quad_tree_node,
						child_projection_transforms_cache_node,
						child_bounds_cache_node,
						level_of_detail,
						render_level_of_detail,
						frustum_planes,
						frustum_plane_mask);

				continue;
			}

			// Both the source raster and age grid rasters have not reached their maximum resolution
			// yet so keep traversing both cube quad trees without uv texture scaling.
			render_quad_tree_source_raster_and_age_grid(
					renderer,
					*child_cube_quad_tree_node,
					render_traversal_cube_quad_tree,
					child_render_traversal_cube_quad_tree_node,
					*child_source_raster_quad_tree_node,
					*child_age_grid_mask_quad_tree_node,
					*child_age_grid_coverage_quad_tree_node,
					reconstructed_polygon_mesh_transform_groups,
					reconstructed_polygon_mesh_transform_group,
					polygon_mesh_drawables,
					polygon_mesh_node_intersections,
					*child_intersections_quad_tree_node,
					child_projection_transforms_cache_node,
					child_bounds_cache_node,
					level_of_detail + 1,
					render_level_of_detail,
					frustum_planes,
					frustum_plane_mask);
		}
	}
}


bool
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::cull_quad_tree(
		const present_day_polygon_mesh_membership_type &reconstructed_polygon_mesh_membership,
		const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
		const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
		const cube_subdivision_bounds_cache_type::node_reference_type &bounds_cache_node,
		const GLFrustum &frustum_planes,
		boost::uint32_t &frustum_plane_mask)
{
	// Of all the present day polygon meshes that intersect the current quad tree node
	// see if any belong to the current transform group.
	//
	// This is done by seeing if there's any membership flag (for a present day polygon mesh)
	// that's true in both:
	// - the polygon meshes in the current transform group, and
	// - the polygon meshes that can possibly intersect the current quad tree node.
	if (!reconstructed_polygon_mesh_membership.get_polygon_meshes_membership().intersects(
					polygon_mesh_node_intersections.get_intersecting_polygon_meshes(
							intersections_quad_tree_node).get_polygon_meshes_membership()))
	{
		// We can return early since none of the present day polygon meshes in the current
		// transform group intersect the current quad tree node.
		return true;
	}

	// If the frustum plane mask is zero then it means we are entirely inside the view frustum.
	// So only test for intersection if the mask is non-zero.
	if (frustum_plane_mask != 0)
	{
		const GLIntersect::OrientedBoundingBox &quad_tree_node_bounds =
				d_cube_subdivision_bounds_cache->get_cached_element(bounds_cache_node)
						->get_oriented_bounding_box();

		// See if the current quad tree node intersects the view frustum.
		// Use the quad tree node's bounding box.
		boost::optional<boost::uint32_t> out_frustum_plane_mask =
				GLIntersect::intersect_OBB_frustum(
						quad_tree_node_bounds,
						frustum_planes.get_planes(),
						frustum_plane_mask);
		if (!out_frustum_plane_mask)
		{
			// No intersection so quad tree node is outside view frustum and we can cull it.
			return true;
		}

		// Update the frustum plane mask so we only test against those planes that
		// the current quad tree render node intersects. The node is entirely inside
		// the planes with a zero bit and so its child nodes are also entirely inside
		// those planes too and so they won't need to test against them.
		frustum_plane_mask = out_frustum_plane_mask.get();
	}

	// The current quad tree node is potentially visible in the frustum so we can't cull it.
	return false;
}


GPlatesOpenGL::GLStateSet::non_null_ptr_to_const_type
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::create_scene_tile_state_set(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_to_const_type &scene_tile_texture,
		const GLUtils::QuadTreeUVTransform &scene_tile_uv_transform,
		const cube_subdivision_projection_transforms_cache_type::node_reference_type &projection_transforms_cache_node)
{
	//PROFILE_FUNC();

	// Get the view/projection transforms for the current cube quad tree node.
	const GLTransform::non_null_ptr_to_const_type projection_transform =
			d_cube_subdivision_projection_transforms_cache->get_cached_element(
					projection_transforms_cache_node)->get_projection_transform();

	// The view transform never changes within a cube face so it's the same across
	// an entire cube face quad tree (each cube face has its own quad tree).
	const GLTransform::non_null_ptr_to_const_type view_transform =
			d_cube_subdivision_projection_transforms_cache->get_view_transform(
					projection_transforms_cache_node.get_cube_face());

	// The composite state to return to the caller.
	GLCompositeStateSet::non_null_ptr_type scene_tile_state_set = GLCompositeStateSet::create();

	// Used to transform texture coordinates to account for partial coverage of current tile.
	GLMatrix scene_tile_texture_matrix;
	scene_tile_uv_transform.transform(scene_tile_texture_matrix);
	scene_tile_texture_matrix.gl_mult_matrix(GLUtils::get_clip_space_to_texture_space_transform());

	// The state for the scene tile texture.
	GLUtils::set_frustum_texture_state(
			*scene_tile_state_set,
			scene_tile_texture,
			projection_transform->get_matrix(),
			view_transform->get_matrix(),
			0/*texture_unit*/,
			GL_REPLACE,
			scene_tile_texture_matrix);

	// Modulate the scene tile texture with the clip texture.
	//
	// Convert the clip-space range [-1, 1] to texture coord range [0.25, 0.75] so that the
	// frustrum edges will map to the boundary of the interior 2x2 clip region of our
	// 4x4 clip texture.
	// NOTE: We don't adjust the clip texture matrix in case viewport only covers a part of the
	// tile texture - this is because the clip texture is always centred on the view frustum regardless.
	GLUtils::set_frustum_texture_state(
			*scene_tile_state_set,
			d_xy_clip_texture,
			projection_transform->get_matrix(),
			view_transform->get_matrix(),
			1/*texture_unit*/,
			GL_MODULATE,
			d_xy_clip_texture_transform);

	// NOTE: We don't set alpha-blending (or alpha-testing) state here because we
	// might not be rendering directly to the final render target and hence we don't
	// want to double-blend semi-transparent rasters - the alpha value is multiplied by
	// all channels including alpha during alpha blending (R,G,B,A) -> (A*R,A*G,A*B,A*A) -
	// the final render target would then have a source blending contribution of (3A*R,3A*G,3A*B,4A)
	// which is not what we want - we want (A*R,A*G,A*B,A*A).
	// Currently we are rendering to the final render target but when we reconstruct
	// rasters in the map view (later) we will render to an intermediate render target.

#if 0
	// Used to render as wire-frame meshes instead of filled textured meshes for
	// visualising mesh density.
	GLPolygonState::non_null_ptr_type polygon_state = GLPolygonState::create();
	polygon_state->gl_polygon_mode(GL_FRONT_AND_BACK, GL_LINE);
	scene_tile_state_set->add_state_set(polygon_state);
#endif

	return scene_tile_state_set;
}


void
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::render_source_raster_tile_to_scene(
		GLRenderer &renderer,
		render_traversal_cube_quad_tree_type::node_type &render_traversal_cube_quad_tree_node,
		const raster_cube_quad_tree_type::node_type &source_raster_quad_tree_node,
		const GLUtils::QuadTreeUVTransform &source_raster_uv_transform,
		const reconstructed_polygon_mesh_transform_group_type &reconstructed_polygon_mesh_transform_group,
		const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
		const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
		const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables,
		const cube_subdivision_projection_transforms_cache_type::node_reference_type &projection_transforms_cache_node)
{
	//PROFILE_FUNC();

	// If we haven't yet cached a scene tile state set for the current quad tree node then do so.
	// This caching is useful since the cube quad tree is traversed once for each transform group
	// but the state set for a specific cube quad tree node is the same for them all.
	// Because the same state sets are used this allows the render to sort by state set and hence
	// sort by tile texture which speeds up graphics rendering.
	// This caching only happens within a render call (not across render frames).
	if (!render_traversal_cube_quad_tree_node.get_element().scene_tile_state_set)
	{
		// Get the source raster texture.
		// Since it's a cube texture it may, in turn, have to render its source raster
		// into its texture (which it then passes to us to use).
		const GLTexture::shared_ptr_to_const_type source_raster_texture =
				d_raster_to_reconstruct->get_tile_texture(
						source_raster_quad_tree_node.get_element(), renderer);

		// Prepare for rendering the current scene tile.
		render_traversal_cube_quad_tree_node.get_element().scene_tile_state_set =
				create_scene_tile_state_set(
						renderer,
						source_raster_texture,
						source_raster_uv_transform,
						projection_transforms_cache_node);
	}

	render_tile_to_scene(
			renderer,
			render_traversal_cube_quad_tree_node.get_element().scene_tile_state_set.get(),
			reconstructed_polygon_mesh_transform_group.get_visible_present_day_polygon_meshes_for_active_reconstructions(),
			polygon_mesh_node_intersections,
			intersections_quad_tree_node,
			polygon_mesh_drawables);
}


void
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::render_tile_to_scene(
		GLRenderer &renderer,
		const GLStateSet::non_null_ptr_to_const_type &render_scene_tile_state_set,
		const present_day_polygon_mesh_membership_type &reconstructed_polygon_mesh_membership,
		const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
		const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
		const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables)
{
	// Get the polygon mesh drawables to render for the current transform group that
	// intersect the current cube quad tree node.
	const boost::dynamic_bitset<> polygon_mesh_membership =
			reconstructed_polygon_mesh_membership.get_polygon_meshes_membership() &
			polygon_mesh_node_intersections.get_intersecting_polygon_meshes(
					intersections_quad_tree_node).get_polygon_meshes_membership();

	// Push the scene tile state set onto the state graph.
	renderer.push_state_set(render_scene_tile_state_set);

	render_polygon_drawables(
			renderer,
			polygon_mesh_membership,
			polygon_mesh_drawables);

	// Pop the state set.
	renderer.pop_state_set();
}


void
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::render_polygon_drawables(
		GLRenderer &renderer,
		const boost::dynamic_bitset<> &polygon_mesh_membership,
		const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables)
{
	// Iterate over the present day polygon mesh drawables and render them.
	for (boost::dynamic_bitset<>::size_type polygon_mesh_index = polygon_mesh_membership.find_first();
		polygon_mesh_index != boost::dynamic_bitset<>::npos;
		polygon_mesh_index = polygon_mesh_membership.find_next(polygon_mesh_index))
	{
		const boost::optional<GLDrawable::non_null_ptr_to_const_type> &polygon_mesh_drawable =
				polygon_mesh_drawables[polygon_mesh_index];

		if (polygon_mesh_drawable)
		{
			renderer.add_drawable(polygon_mesh_drawable.get());
		}
	}
}


void
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::render_source_raster_and_age_grid_tile_to_scene(
		GLRenderer &renderer,
		cube_quad_tree_type::node_type &cube_quad_tree_node,
		render_traversal_cube_quad_tree_type::node_type &render_traversal_cube_quad_tree_node,
		const raster_cube_quad_tree_type::node_type &source_raster_quad_tree_node,
		const GLUtils::QuadTreeUVTransform &source_raster_uv_transform,
		const age_grid_mask_cube_quad_tree_type::node_type &age_grid_mask_quad_tree_node,
		const age_grid_coverage_cube_quad_tree_type::node_type &age_grid_coverage_quad_tree_node,
		const GLUtils::QuadTreeUVTransform &age_grid_uv_transform,
		const reconstructed_polygon_mesh_transform_groups_type &reconstructed_polygon_mesh_transform_groups,
		const reconstructed_polygon_mesh_transform_group_type &reconstructed_polygon_mesh_transform_group,
		const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
		const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
		const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables,
		const cube_subdivision_projection_transforms_cache_type::node_reference_type &projection_transforms_cache_node)
{
	//PROFILE_FUNC();

	// If we haven't yet cached a scene tile state set for the current quad tree node then do so.
	// This caching is useful since the cube quad tree is traversed once for each transform group
	// but the state set for a specific cube quad tree node is the same for them all.
	// Because the same state sets are used this allows the render to sort by state set and hence
	// sort by tile texture which speeds up graphics rendering.
	// This caching only happens within a render call (not across render frames).
	if (!render_traversal_cube_quad_tree_node.get_element().scene_tile_state_set)
	{
		// Render the current age-masked scene tile if it's not currently cached.
		const GLTexture::shared_ptr_to_const_type age_masked_source_tile_texture =
				get_age_masked_source_raster_tile(
						renderer,
						cube_quad_tree_node,
						source_raster_quad_tree_node,
						source_raster_uv_transform,
						age_grid_mask_quad_tree_node,
						age_grid_coverage_quad_tree_node,
						age_grid_uv_transform,
						reconstructed_polygon_mesh_transform_groups,
						polygon_mesh_node_intersections,
						intersections_quad_tree_node,
						polygon_mesh_drawables,
						projection_transforms_cache_node);

		// Prepare for rendering the current age-masked scene tile.
		render_traversal_cube_quad_tree_node.get_element().scene_tile_state_set =
				create_scene_tile_state_set(
						renderer,
						age_masked_source_tile_texture,
						// Note that there's no uv scaling of the age grid here because we've
						// rendered it to fit exactly in the current cube quad tree node tile...
						GLUtils::QuadTreeUVTransform()/*scene_tile_uv_transform*/,
						projection_transforms_cache_node);
	}

	render_tile_to_scene(
			renderer,
			render_traversal_cube_quad_tree_node.get_element().scene_tile_state_set.get(),
			// Rendering using active *and* inactive reconstructed polygons because the age grid
			// decides the begin time of oceanic crust not the polygons. We still need the polygons
			// but we just don't obey their begin times (we treat them as distant past begin times).
			reconstructed_polygon_mesh_transform_group.get_visible_present_day_polygon_meshes_for_active_or_inactive_reconstructions(),
			polygon_mesh_node_intersections,
			intersections_quad_tree_node,
			polygon_mesh_drawables);
}


GPlatesOpenGL::GLTexture::shared_ptr_to_const_type
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::get_age_masked_source_raster_tile(
		GLRenderer &renderer,
		cube_quad_tree_type::node_type &cube_quad_tree_node,
		const raster_cube_quad_tree_type::node_type &source_raster_quad_tree_node,
		const GLUtils::QuadTreeUVTransform &source_raster_uv_transform,
		const age_grid_mask_cube_quad_tree_type::node_type &age_grid_mask_quad_tree_node,
		const age_grid_coverage_cube_quad_tree_type::node_type &age_grid_coverage_quad_tree_node,
		const GLUtils::QuadTreeUVTransform &age_grid_uv_transform,
		const reconstructed_polygon_mesh_transform_groups_type &reconstructed_polygon_mesh_transform_groups,
		const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
		const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
		const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables,
		const cube_subdivision_projection_transforms_cache_type::node_reference_type &projection_transforms_cache_node)
{
	// See if we've generated our tile texture and
	// see if it hasn't been recycled by the texture cache.
	boost::shared_ptr<GLTexture> age_masked_tile_texture =
			cube_quad_tree_node.get_element().age_masked_tile_texture->get_cached_object();
	if (!age_masked_tile_texture)
	{
		age_masked_tile_texture = cube_quad_tree_node.get_element().age_masked_tile_texture->recycle_an_unused_object();
		if (!age_masked_tile_texture)
		{
			age_masked_tile_texture =
					cube_quad_tree_node.get_element().age_masked_tile_texture->set_cached_object(
							GLTexture::create_as_auto_ptr(d_texture_resource_manager));

			// The texture was just allocated so we need to create it in OpenGL.
			create_age_masked_source_tile_texture(age_masked_tile_texture);
		}

		// Render the age-masked source raster into our tile texture.
		render_age_masked_source_raster_into_tile(
				renderer,
				age_masked_tile_texture,
				cube_quad_tree_node,
				source_raster_quad_tree_node,
				source_raster_uv_transform,
				age_grid_mask_quad_tree_node,
				age_grid_coverage_quad_tree_node,
				age_grid_uv_transform,
				reconstructed_polygon_mesh_transform_groups,
				polygon_mesh_node_intersections,
				intersections_quad_tree_node,
				polygon_mesh_drawables,
				projection_transforms_cache_node);

		return age_masked_tile_texture;
	}

	// Our texture wasn't recycled but see if it's still valid in case the source raster or
	// age grid rasters changed underneath us.
	if (!d_raster_to_reconstruct->get_subject_token().is_observer_up_to_date(
			cube_quad_tree_node.get_element().source_raster_observer_token) ||
		!d_age_grid_params->age_grid_mask_cube_raster->get_subject_token().is_observer_up_to_date(
			cube_quad_tree_node.get_element().age_grid_mask_observer_token) ||
		!d_age_grid_params->age_grid_coverage_cube_raster->get_subject_token().is_observer_up_to_date(
			cube_quad_tree_node.get_element().age_grid_coverage_observer_token))
	{
		// Render the age-masked source raster into our tile texture.
		render_age_masked_source_raster_into_tile(
				renderer,
				age_masked_tile_texture,
				cube_quad_tree_node,
				source_raster_quad_tree_node,
				source_raster_uv_transform,
				age_grid_mask_quad_tree_node,
				age_grid_coverage_quad_tree_node,
				age_grid_uv_transform,
				reconstructed_polygon_mesh_transform_groups,
				polygon_mesh_node_intersections,
				intersections_quad_tree_node,
				polygon_mesh_drawables,
				projection_transforms_cache_node);
	}

	return age_masked_tile_texture;
}


// The BOOST_FOREACH macro in versions of boost before 1.37 uses the same local
// variable name in each instantiation. Nested BOOST_FOREACH macros therefore
// cause GCC to warn about shadowed declarations.
DISABLE_GCC_WARNING("-Wshadow")

void
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::render_age_masked_source_raster_into_tile(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_to_const_type &age_masked_source_tile_texture,
		cube_quad_tree_type::node_type &cube_quad_tree_node,
		const raster_cube_quad_tree_type::node_type &source_raster_quad_tree_node,
		const GLUtils::QuadTreeUVTransform &source_raster_uv_transform,
		const age_grid_mask_cube_quad_tree_type::node_type &age_grid_mask_quad_tree_node,
		const age_grid_coverage_cube_quad_tree_type::node_type &age_grid_coverage_quad_tree_node,
		const GLUtils::QuadTreeUVTransform &age_grid_uv_transform,
		const reconstructed_polygon_mesh_transform_groups_type &reconstructed_polygon_mesh_transform_groups,
		const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
		const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
		const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables,
		const cube_subdivision_projection_transforms_cache_type::node_reference_type &projection_transforms_cache_node)
{
	//PROFILE_FUNC();

	//
	// Obtain textures of the source, age grid mask/coverage tiles.
	//

	// Get the source raster texture.
	// Since it's a cube texture it may, in turn, have to render its source raster
	// into its texture (which it then passes to us to use).
	GLTexture::shared_ptr_to_const_type source_raster_texture =
			d_raster_to_reconstruct->get_tile_texture(
					source_raster_quad_tree_node.get_element(),
					renderer);

	// If we got here then we should have an age grid.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_age_grid_params,
			GPLATES_ASSERTION_SOURCE);

	// Get the age grid mask/coverage textures.
	// Since they are cube textures they may, in turn, have to render their source rasters
	// into their textures (which they then pass to us to use).
	const GLTexture::shared_ptr_to_const_type age_grid_mask_texture =
			d_age_grid_params->age_grid_mask_cube_raster->get_tile_texture(
					age_grid_mask_quad_tree_node.get_element(), renderer);
	const GLTexture::shared_ptr_to_const_type age_grid_coverage_texture =
			d_age_grid_params->age_grid_coverage_cube_raster->get_tile_texture(
					age_grid_coverage_quad_tree_node.get_element(), renderer);


	//
	// Setup common rendering state for some of the three age grid mask render passes.
	//

	// Turns off colour channel writes for some passes because we're generating an alpha mask
	// representing what should be drawn.
	GLMaskBuffersState::non_null_ptr_type mask_colour_channels_state = GLMaskBuffersState::create();
	mask_colour_channels_state->gl_color_mask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);

	// Used to transform texture coordinates to account for partial coverage of current tile
	// by the source raster.
	// This can happen if we are rendering a source raster that is lower-resolution than the age grid.
	GLMatrix source_raster_tile_coverage_texture_matrix;
	source_raster_uv_transform.transform(source_raster_tile_coverage_texture_matrix);

	// Used to transform texture coordinates to account for partial coverage of current tile
	// by the age grid tiles.
	// This can happen if we are rendering a source raster that is higher-resolution than the age grid.
	GLMatrix age_grid_partial_tile_coverage_texture_matrix;
	age_grid_uv_transform.transform(age_grid_partial_tile_coverage_texture_matrix);

	// Get the view/projection transforms for the current cube quad tree node.
	const GLTransform::non_null_ptr_to_const_type projection_transform =
			d_cube_subdivision_projection_transforms_cache->get_cached_element(
					projection_transforms_cache_node)->get_projection_transform();

	// The view transform never changes within a cube face so it's the same across
	// an entire cube face quad tree (each cube face has its own quad tree).
	const GLTransform::non_null_ptr_to_const_type view_transform =
			d_cube_subdivision_projection_transforms_cache->get_view_transform(
					projection_transforms_cache_node.get_cube_face());

	// Get all present day polygons that intersect the current cube quad tree node.
	const present_day_polygon_mesh_membership_type &present_day_polygons_intersecting_tile =
			polygon_mesh_node_intersections.get_intersecting_polygon_meshes(intersections_quad_tree_node);


	//
	// Begin rendering to age masked source tile texture.
	//

	// Push a render target that will render to the age masked source tile texture.
	// We can render to the target in parallel because we're caching the age masked source texture.
	renderer.push_render_target(
			GLTextureRenderTargetType::create(
					age_masked_source_tile_texture, d_tile_texel_dimension, d_tile_texel_dimension),
			GLRenderer::RENDER_TARGET_USAGE_PARALLEL);

	// Setup for clearing the render target colour buffer.
	GLClearBuffersState::non_null_ptr_type clear_buffers_state = GLClearBuffersState::create();
	GLClearBuffers::non_null_ptr_type clear_buffers = GLClearBuffers::create();
	// Clear colour to all ones.
	clear_buffers_state->gl_clear_color(1, 1, 1, 1);
	// Clear only the colour buffer.
	clear_buffers->gl_clear(GL_COLOR_BUFFER_BIT);

	// Clear the colour buffer of the render target.
	renderer.push_state_set(clear_buffers_state);
	renderer.add_drawable(clear_buffers);
	renderer.pop_state_set();

	// The render target viewport.
	const GLViewport viewport(0, 0, d_tile_texel_dimension, d_tile_texel_dimension);
	const GLViewportState::non_null_ptr_to_const_type viewport_state = GLViewportState::create(viewport);

	// Push the viewport state set.
	renderer.push_state_set(viewport_state);
	// Let the transform state know of the new viewport.
	renderer.get_transform_state().set_viewport(viewport);



	//  Get raster tile (texture) - may involve GLMRCR pushing a render target if not cached.
	//  Get age grid coverage tile (texture) - may involve GLMRCR pushing a render target.
	//  Get age grid mask tile (texture) - may involve GLMRCR pushing a render target.
	//  Push render target (blank OpenGL state)
	//    Set viewport, alpha-blending, etc.
	//    Mask writes to the alpha channel.
	//    First pass: render age grid mask as a single quad covering viewport.
	//    Second pass:
	//      Set projection matrix using that of the quad tree node.
	//      Bind age grid coverage texture to texture unit 0.
	//      Set texgen/texture matrix state for texture unit using projection matrix
	//        of quad tree node (and any adjustments).
	//      Iterate over polygon meshes (whose age range covers the current time):
	//        Wrap a vertex array drawable around mesh triangles and polygon vertex array
	//          and add to the renderer.
	//      Enditerate
	//    Third pass:
	//      Unmask alpha channel.
	//      Setup multiplicative blending using GL_DST_COLOR.
	//      Render raster tile as a single quad covering viewport.
	//  Pop render target.
	//  Render same as below (case for no age grid) except render all polygon meshes
	//    regardless of age of appearance/disappearance.


	//
	// Render the three passes.
	//

	render_first_pass_age_masked_source_raster(
			renderer,
			age_grid_mask_texture,
			age_grid_partial_tile_coverage_texture_matrix,
			mask_colour_channels_state);

	render_second_pass_age_masked_source_raster(
			renderer,
			age_grid_coverage_texture,
			age_grid_partial_tile_coverage_texture_matrix,
			mask_colour_channels_state,
			*projection_transform,
			*view_transform,
			reconstructed_polygon_mesh_transform_groups,
			present_day_polygons_intersecting_tile,
			polygon_mesh_drawables);

	render_third_pass_age_masked_source_raster(
			renderer,
			source_raster_texture,
			source_raster_tile_coverage_texture_matrix);


	// Pop the viewport state set.
	renderer.pop_state_set();

	renderer.pop_render_target();


	//
	// Update the age mask source tile validity with respect to its input tiles.
	//

	// If we got here then we should have an age grid.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_age_grid_params,
			GPLATES_ASSERTION_SOURCE);

	// This tile texture is now update-to-date with the inputs used to generate it.
	d_raster_to_reconstruct->get_subject_token().update_observer(
			cube_quad_tree_node.get_element().source_raster_observer_token);

	d_age_grid_params->age_grid_mask_cube_raster->get_subject_token().update_observer(
			cube_quad_tree_node.get_element().age_grid_mask_observer_token);

	d_age_grid_params->age_grid_coverage_cube_raster->get_subject_token().update_observer(
			cube_quad_tree_node.get_element().age_grid_coverage_observer_token);
}


void
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::render_first_pass_age_masked_source_raster(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_to_const_type &age_grid_mask_texture,
		const GLMatrix &age_grid_partial_tile_coverage_texture_matrix,
		const GLMaskBuffersState::non_null_ptr_type &mask_colour_channels_state)
{
	//
	// First pass state
	//

	GLCompositeStateSet::non_null_ptr_type first_render_pass_state = GLCompositeStateSet::create();

	// Set the texture state for the age grid mask.
	GLUtils::set_full_screen_quad_texture_state(
			*first_render_pass_state,
			age_grid_mask_texture,
			0/*texture_unit*/,
			GL_REPLACE,
			age_grid_partial_tile_coverage_texture_matrix);

	first_render_pass_state->add_state_set(mask_colour_channels_state);

	//
	// Render the first render pass.
	//

	// NOTE: We leave the model-view and projection matrices as identity as that is what we
	// need to draw a full-screen quad.
	renderer.push_state_set(first_render_pass_state);
 	renderer.add_drawable(d_full_screen_quad_drawable);
	renderer.pop_state_set();
}


void
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::render_second_pass_age_masked_source_raster(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_to_const_type &age_grid_coverage_texture,
		const GLMatrix &age_grid_partial_tile_coverage_texture_matrix,
		const GLMaskBuffersState::non_null_ptr_type &mask_colour_channels_state,
		const GLTransform &projection_transform,
		const GLTransform &view_transform,
		const reconstructed_polygon_mesh_transform_groups_type &reconstructed_polygon_mesh_transform_groups,
		const present_day_polygon_mesh_membership_type &present_day_polygons_intersecting_tile,
		const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables)
{
	//
	// Second pass state
	//

	GLCompositeStateSet::non_null_ptr_type second_render_pass_state = GLCompositeStateSet::create();

	// Since we're rendering to a frustum (instead of a full-screen quad) we also need to convert
	// from clip coordinates ([-1,1]) to texture coordinates ([0,1]).
	GLMatrix age_grid_partial_tile_coverage_clip_space_to_texture_space_matrix(
			age_grid_partial_tile_coverage_texture_matrix);
	age_grid_partial_tile_coverage_clip_space_to_texture_space_matrix.gl_mult_matrix(
			GLUtils::get_clip_space_to_texture_space_transform());

	// Set the texture state for the age grid coverage.
	GLUtils::set_frustum_texture_state(
			*second_render_pass_state,
			age_grid_coverage_texture,
			projection_transform.get_matrix(),
			view_transform.get_matrix(),
			0/*texture_unit*/,
			GL_REPLACE,
			age_grid_partial_tile_coverage_clip_space_to_texture_space_matrix);

	second_render_pass_state->add_state_set(mask_colour_channels_state);

	// Alpha-blend state.
	GLBlendState::non_null_ptr_type second_pass_blend_state = GLBlendState::create();
	second_pass_blend_state->gl_enable(GL_TRUE).gl_blend_func(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	second_render_pass_state->add_state_set(second_pass_blend_state);

#if 0
	// Alpha-test state.
	GLAlphaTestState::non_null_ptr_type second_pass_alpha_test_state = GLAlphaTestState::create();
	second_pass_alpha_test_state->gl_enable(GL_TRUE).gl_alpha_func(GL_GREATER, GLclampf(0));
	second_render_pass_state->add_state_set(second_pass_alpha_test_state);
#endif

	//
	// Render the second render pass.
	//

	renderer.push_state_set(second_render_pass_state);

	renderer.push_transform(projection_transform);
	renderer.push_transform(view_transform);

	// Render all polygons covering the current subdivision tile - not just the polygons for
	// the current rotation group.
	//
	// Since we're using an age grid we're rendering the age grid mask combined
	// with the polygon mask (where there's no age grid coverage) and so we need to draw
	// all polygons that cover the current quad tree node - this is also because the results
	// get cached into the tile texture and other visits to this quad tree node in the same
	// scene render (but by different rotation groups) can use the cached texture.
	//
	// NOTE: We render the polygons at the current reconstruction time so that continental
	// polygons (ie, polygons outside the oceanic age mask) that don't exist anymore will get
	// removed from the combined age mask.

	// Of those polygons intersecting the current cube quad tree node find those that
	// are still active after being reconstructed to the current reconstruction time.
	// Any that are not active for the current reconstruction time will not get drawn.
	//
	// IMPORTANT: We need to draw *all* polygons that are active at the current reconstruction time
	// and not just those that are visible in the current view frustum - this is because the
	// age-masked quad tree node texture is cached and reused for other frames and the user might
	// rotate the globe bringing into view a cached tile texture that wasn't properly covered
	// with polygons thus leaving gaps in the end result.
	const boost::dynamic_bitset<> reconstructed_polygons_for_all_rotation_groups =
			present_day_polygons_intersecting_tile.get_polygon_meshes_membership() &
			reconstructed_polygon_mesh_transform_groups.get_all_present_day_polygon_meshes_for_active_reconstructions()
					.get_polygon_meshes_membership();

	render_polygon_drawables(
			renderer,
			reconstructed_polygons_for_all_rotation_groups,
			polygon_mesh_drawables);

	renderer.pop_transform();
	renderer.pop_transform();

	renderer.pop_state_set();
}


void
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::render_third_pass_age_masked_source_raster(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_to_const_type &source_raster_texture,
		const GLMatrix &source_raster_tile_coverage_texture_matrix)
{
	//
	// Third pass state
	//

	GLCompositeStateSet::non_null_ptr_type third_render_pass_state = GLCompositeStateSet::create();

	// Set the texture state for the source raster.
	GLUtils::set_full_screen_quad_texture_state(
			*third_render_pass_state,
			source_raster_texture,
			0/*texture_unit*/,
			GL_REPLACE,
			source_raster_tile_coverage_texture_matrix);

	// Alpha-blend state.
	GLBlendState::non_null_ptr_type third_pass_blend_state = GLBlendState::create();
	third_pass_blend_state->gl_enable(GL_TRUE).gl_blend_func(GL_DST_COLOR, GL_ZERO);
	third_render_pass_state->add_state_set(third_pass_blend_state);

	//
	// Render the third render pass.
	//

	// NOTE: We leave the model-view and projection matrices as identity as that is what we
	// need to draw a full-screen quad.
	renderer.push_state_set(third_render_pass_state);
	renderer.add_drawable(d_full_screen_quad_drawable);
	renderer.pop_state_set();
}


// See above.
ENABLE_GCC_WARNING("-Wshadow")


// We use macros in <GL/glew.h> that contain old-style casts.
DISABLE_GCC_WARNING("-Wold-style-cast")

void
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::create_age_masked_source_tile_texture(
		const GLTexture::shared_ptr_type &texture)
{
	// Bind the texture so its the current texture.
	// Here we actually make a direct OpenGL call to bind the texture to the currently
	// active texture unit. It doesn't matter what the current texture unit is because
	// the only reason we're binding the texture object is so we can set its state -
	// so that subsequent binds of this texture object, when we render the scene graph,
	// will set that state to OpenGL.
	texture->gl_bind_texture(GL_TEXTURE_2D);

	//
	// No mipmaps needed so we specify no mipmap filtering.
	// We're not using mipmaps because our cube mapping does not have much distortion
	// unlike global rectangular lat/lon rasters that squash near the poles.
	//
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	// Specify anisotropic filtering if it's supported since we are not using mipmaps
	// and any textures rendered near the edge of the globe will get squashed a bit due to
	// the angle we are looking at them and anisotropic filtering will help here.
	if (GLEW_EXT_texture_filter_anisotropic)
	{
		const GLfloat anisotropy = GLContext::get_texture_parameters().gl_texture_max_anisotropy_EXT;
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
	}

	// Create the texture but don't load any data into it.
	// Leave it uninitialised because we will be rendering into it to initialise it.
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
			d_tile_texel_dimension, d_tile_texel_dimension,
			0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	// Check there are no OpenGL errors.
	GLUtils::assert_no_gl_errors(GPLATES_ASSERTION_SOURCE);
}

ENABLE_GCC_WARNING("-Wold-style-cast")

// It seems some template instantiations happen at the end of the file.
// Disabling shadow warning caused by boost object_pool (used by GPlatesUtils::ObjectCache).
DISABLE_GCC_WARNING("-Wshadow")

