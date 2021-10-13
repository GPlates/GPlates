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
		const GLVertexBufferResourceManager::shared_ptr_type &vertex_buffer_resource_manager) :
	d_raster_to_reconstruct(raster_to_reconstruct),
	d_reconstructed_static_polygon_meshes(reconstructed_static_polygon_meshes),
	d_cube_subdivision_projection_transforms_cache(cube_subdivision_projection_transforms_cache),
	d_cube_subdivision_bounds_cache(cube_subdivision_bounds_cache),
	d_texture_resource_manager(texture_resource_manager),
	d_vertex_buffer_resource_manager(vertex_buffer_resource_manager),
	d_xy_clip_texture_transform(GLTextureUtils::get_clip_texture_clip_space_to_texture_space_transform()),
	d_full_screen_quad_drawable(GLUtils::create_full_screen_2D_textured_quad())
{
	PROFILE_FUNC();
}


void
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::create_z_clip_texture()
{
	d_z_clip_texture = GLTexture::create(d_texture_resource_manager);

	// Bind the texture so its the current texture.
	// Here we actually make a direct OpenGL call to bind the texture to the currently
	// active texture unit. It doesn't matter what the current texture unit is because
	// the only reason we're binding the texture object is so we can set its state =
	// so that subsequent binds of this texture object, when we render the scene graph,
	// will set that state to OpenGL.
	d_z_clip_texture->gl_bind_texture(GL_TEXTURE_2D);

	//
	// We *must* use nearest neighbour filtering otherwise the clip texture won't work.
	// We are relying on the hard transition from white to black to clip for us.
	//
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	//
	// The clip texture is a 2x1 image where the one texel is white and the other black.
	// We will use the alpha channel for alpha-testing (to discard clipped regions).
	//
	const GPlatesGui::rgba8_t mask_zero(0, 0, 0, 0);
	const GPlatesGui::rgba8_t mask_one(255, 255, 255, 255);
	const GPlatesGui::rgba8_t mask_image[2] = { mask_zero, mask_one };

	// Create the texture and load the data into it.
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 2, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, mask_image);

	// Check there are no OpenGL errors.
	GLUtils::assert_no_gl_errors(GPLATES_ASSERTION_SOURCE);
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
	PROFILE_FUNC();

	// First make sure we've created our clip textures.
	// We do this here rather than in the constructor because we know we have an active
	// OpenGL context here - because we're rendering.
	if (!d_xy_clip_texture)
	{
		d_xy_clip_texture = GLTextureUtils::create_xy_clip_texture(d_texture_resource_manager);
	}
	if (!d_z_clip_texture)
	{
		create_z_clip_texture();
	}

	// Get the level-of-detail based on the size of viewport pixels projected onto the globe.
	// We'll try to render of this level of detail if our quad tree is deep enough.
	const unsigned int render_level_of_detail = get_level_of_detail(renderer.get_transform_state());

	// The polygon mesh drawables and polygon mesh cube quad tree node intersections.
	const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables =
			d_reconstructed_static_polygon_meshes->get_present_day_polygon_mesh_drawables();
	const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections =
			d_reconstructed_static_polygon_meshes->get_present_day_polygon_mesh_node_intersections();

	// Get the transform groups of reconstructed polygon meshes that are visible in the view frustum.
	reconstructed_polygon_mesh_transform_group_seq_type reconstructed_polygon_mesh_transform_groups;
	d_reconstructed_static_polygon_meshes->get_visible_reconstructed_polygon_meshes(
			reconstructed_polygon_mesh_transform_groups,
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
			reconstructed_polygon_mesh_transform_groups)
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

			// Get the root projection transforms node.
			const cube_subdivision_projection_transforms_cache_type::node_reference_type
					projection_transforms_cache_root_node =
							d_cube_subdivision_projection_transforms_cache->get_quad_tree_root_node(cube_face);

			// Get the root bounds node.
			const cube_subdivision_bounds_cache_type::node_reference_type
					bounds_cache_root_node =
							d_cube_subdivision_bounds_cache->get_quad_tree_root_node(cube_face);

			render_quad_tree(
					renderer,
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
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::render_quad_tree(
		GLRenderer &renderer,
		const raster_cube_quad_tree_type::node_type &source_raster_quad_tree_node,
		const reconstructed_polygon_mesh_transform_group_type &reconstructed_polygon_mesh_transform_group,
		const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables,
		const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
		const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
		const cube_subdivision_projection_transforms_cache_type::node_reference_type &
				projection_transforms_cache_node,
		const cube_subdivision_bounds_cache_type::node_reference_type &bounds_cache_node,
		unsigned int level_of_detail,
		unsigned int render_level_of_detail,
		const GLFrustum &frustum_planes,
		boost::uint32_t frustum_plane_mask)
{
	// Of all the present day polygon meshes that intersect the current quad tree node
	// see if any belong to the current transform group.
	//
	// This is done by seeing if there's any membership flag (for a present day polygon mesh)
	// that's true in both:
	// - the polygon meshes in the current transform group, and
	// - the polygon meshes that can possibly intersect the current quad tree node.
	if (!reconstructed_polygon_mesh_transform_group.get_present_day_polygon_meshes()
			.get_polygon_meshes_membership().intersects(
					polygon_mesh_node_intersections.get_intersecting_polygon_meshes(
							intersections_quad_tree_node).get_polygon_meshes_membership()))
	{
		// We can return early since none of the present day polygon meshes in the current
		// transform group intersect the current quad tree node.
		return;
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
			return;
		}

		// Update the frustum plane mask so we only test against those planes that
		// the current quad tree render node intersects. The node is entirely inside
		// the planes with a zero bit and so its child nodes are also entirely inside
		// those planes too and so they won't need to test against them.
		frustum_plane_mask = out_frustum_plane_mask.get();
	}

	// If either:
	// - we've reached a leaf node of the source raster (highest resolution supplied the source raster), or
	// - we're at the correct level of detail for rendering,
	// ...then render the current source raster tile.
	if (source_raster_quad_tree_node.get_element().is_leaf_node() ||
		level_of_detail == render_level_of_detail)
	{
		render_tile_to_scene(
				renderer,
				source_raster_quad_tree_node,
				reconstructed_polygon_mesh_transform_group,
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
			const raster_cube_quad_tree_type::node_type *source_raster_child_quad_tree_node =
					source_raster_quad_tree_node.get_child_node(child_u_offset, child_v_offset);

			// If there is no source raster for the current child then continue to the next child.
			// This happens if the current child is not covered by the source raster.
			// Note that if we get here then the current parent is not a leaf node since it
			// has at least one child that is covered (and that is non-NULL).
			if (source_raster_child_quad_tree_node == NULL)
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

			render_quad_tree(
					renderer,
					*source_raster_child_quad_tree_node,
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


GPlatesOpenGL::GLStateSet::non_null_ptr_to_const_type
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::create_scene_tile_state_set(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_to_const_type &scene_tile_texture,
		const cube_subdivision_projection_transforms_cache_type::node_reference_type &projection_transforms_cache_node)
{
	PROFILE_FUNC();

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

	// Modulate the scene tile texture with the clip texture.
	GLUtils::set_frustum_texture_state(
			*scene_tile_state_set,
			scene_tile_texture,
			projection_transform->get_matrix(),
			view_transform->get_matrix(),
			0/*texture_unit*/,
			GL_REPLACE);

	// Convert the clip-space range [-1, 1] to texture coord range [0.25, 0.75] so that the
	// frustrum edges will map to the boundary of the interior 2x2 clip region of our
	// 4x4 clip texture.
	// NOTE: We don't adjust the clip texture matrix in case viewport only covers a
	// part of the source raster texture - this is because the clip texture is always
	// centred on the view frustum regardless.
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
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::render_tile_to_scene(
		GLRenderer &renderer,
		const raster_cube_quad_tree_type::node_type &source_raster_quad_tree_node,
		const reconstructed_polygon_mesh_transform_group_type &reconstructed_polygon_mesh_transform_group,
		const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables,
		const cube_subdivision_projection_transforms_cache_type::node_reference_type &projection_transforms_cache_node)
{
	PROFILE_FUNC();

	// Get the source raster texture.
	// Since it's a cube texture it may, in turn, have to render its source raster
	// into its texture (which it then passes to us to use).
	const GLTexture::shared_ptr_to_const_type source_raster_texture =
			d_raster_to_reconstruct->get_tile_texture(
					source_raster_quad_tree_node.get_element(), renderer);

	// Prepare for rendering the current scene tile.
	const GLStateSet::non_null_ptr_to_const_type render_scene_tile_state_set =
			create_scene_tile_state_set(
					renderer,
					source_raster_texture,
					projection_transforms_cache_node);

	// Push the scene tile state set onto the state graph.
	renderer.push_state_set(render_scene_tile_state_set);

	// Get the polygon mesh drawables to render for the current transform group.
	const boost::dynamic_bitset<> &polygon_mesh_membership =
			reconstructed_polygon_mesh_transform_group.get_present_day_polygon_meshes()
					.get_polygon_meshes_membership();

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

	// Pop the state set.
	renderer.pop_state_set();
}
