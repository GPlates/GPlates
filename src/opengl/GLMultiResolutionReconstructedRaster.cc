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

#include <cmath>

#include "global/CompilerWarnings.h"
PUSH_MSVC_WARNINGS
DISABLE_MSVC_WARNING(4503)
#include <map>
POP_MSVC_WARNINGS

#include <boost/cast.hpp>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Delaunay_mesher_2.h>
#include <CGAL/Delaunay_mesh_face_base_2.h>
#include <CGAL/Delaunay_mesh_size_criteria_2.h>
#include <CGAL/Cartesian.h>
#include <CGAL/Polygon_2.h>
/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>
#include <opengl/OpenGL.h>

#include "GLMultiResolutionReconstructedRaster.h"

#include "GLBindTextureState.h"
#include "GLBlendState.h"
#include "GLCompositeStateSet.h"
#include "GLContext.h"
#include "GLFragmentTestStates.h"
#include "GLIntersect.h"
#include "GLMatrix.h"
#include "GLPointLinePolygonState.h"
#include "GLRenderer.h"
#include "GLTextureEnvironmentState.h"
#include "GLTextureTransformState.h"
#include "GLTransformState.h"
#include "GLUtils.h"
#include "GLVertexArrayDrawable.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "gui/Colour.h"

#include "maths/UnitVector3D.h"
#include "maths/Vector3D.h"

#include "utils/Profile.h"


namespace
{
	/**
	 * Vertex used to render where texture coordinates are tex-gen'ed from the (x,y,z).
	 */
	struct Vertex
	{
		//! Vertex position.
		GLfloat x, y, z;
	};


	/**
	 * Vertex used to draw full-screen textured quads into a render texture.
	 */
	struct TextureVertex
	{
		//! Vertex position.
		GLfloat x, y, z;

		//! Vertex texture coordinates.
		GLfloat u, v;
	};


	//! The inverse of log(2.0).
	const float INVERSE_LOG2 = 1.0 / std::log(2.0);


	/**
	 * Projects a unit vector point onto the plane whose normal is @a plane_normal and
	 * returns normalised version of projected point.
	 */
	GPlatesMaths::UnitVector3D
	get_orthonormal_vector(
			const GPlatesMaths::UnitVector3D &point,
			const GPlatesMaths::UnitVector3D &plane_normal)
	{
		// The projection of 'point' in the direction of 'plane_normal'.
		GPlatesMaths::Vector3D proj = dot(point, plane_normal) * plane_normal;

		// The projection of 'point' perpendicular to the direction of
		// 'plane_normal'.
		return (GPlatesMaths::Vector3D(point) - proj).get_normalisation();
	}
}


GPlatesOpenGL::GLCubeSubdivision::non_null_ptr_to_const_type
GPlatesOpenGL::GLMultiResolutionReconstructedRaster::get_cube_subdivision()
{
	// This is the cube subdivision we are using.
	static GLCubeSubdivision::non_null_ptr_to_const_type s_cube_subdivision =
			GLCubeSubdivision::create();

	return s_cube_subdivision;
}


// The BOOST_FOREACH macro in versions of boost before 1.37 uses the same local
// variable name in each instantiation. Nested BOOST_FOREACH macros therefore
// cause GCC to warn about shadowed declarations.
DISABLE_GCC_WARNING("-Wshadow")

GPlatesOpenGL::GLMultiResolutionReconstructedRaster::GLMultiResolutionReconstructedRaster(
		const GLMultiResolutionCubeRaster::non_null_ptr_type &raster_to_reconstruct,
		const GPlatesAppLogic::ReconstructRasterPolygons::non_null_ptr_to_const_type &reconstructing_polygons,
		const GLTextureResourceManager::shared_ptr_type &texture_resource_manager,
		const boost::optional<GLMultiResolutionCubeRaster::non_null_ptr_type> &age_grid_mask_raster,
		const boost::optional<GLMultiResolutionCubeRaster::non_null_ptr_type> &age_grid_coverage_raster) :
	d_raster_to_reconstruct(raster_to_reconstruct),
	d_cube_subdivision(get_cube_subdivision()),
	d_tile_texel_dimension(d_cube_subdivision->get_tile_texel_dimension()),
	d_reconstructing_polygons(reconstructing_polygons),
	d_texture_resource_manager(texture_resource_manager),
	d_using_age_grid(false),
	d_age_grid_mask_raster(age_grid_mask_raster),
	d_age_grid_coverage_raster(age_grid_coverage_raster),
	d_clear_buffers_state(GLClearBuffersState::create()),
	d_clear_buffers(GLClearBuffers::create()),
	d_viewport(0, 0, d_tile_texel_dimension, d_tile_texel_dimension),
	d_viewport_state(GLViewportState::create(d_viewport)),
	d_full_screen_quad_vertex_array(GLVertexArray::create()),
	d_full_screen_quad_vertex_element_array(GLVertexElementArray::create()),
	d_first_age_mask_render_pass_state(GLCompositeStateSet::create()),
	d_second_age_mask_render_pass_state(GLCompositeStateSet::create()),
	d_third_age_mask_render_pass_state(GLCompositeStateSet::create())
{
	PROFILE_FUNC();

	// If we have both an age grid coverage raster and an age grid raster then set up for using them.
	// We'll either have neither or both since they're both sourced from a single proxied raster.
	if (d_age_grid_mask_raster && d_age_grid_coverage_raster)
	{
		d_using_age_grid = true;

		// Since the age grid mask changes dynamically as the reconstruction time changes
		// we don't need to worry about caching so much - just enough caching so that panning
		// the view doesn't mean every tile on screen needs to be regenerated - just the ones
		// near the edges.
		// This can be achieved by setting the cache size to one and just letting it grow as needed.
		d_age_masked_raster_texture_cache = create_texture_cache(1, texture_resource_manager);

		// Setup for clearing the render target colour buffer.
		// Clear colour to all ones.
		d_clear_buffers_state->gl_clear_color(1, 1, 1, 1);
		// Clear only the colour buffer.
		d_clear_buffers->gl_clear(GL_COLOR_BUFFER_BIT);

		// Initialise the vertex array for the full-screen quad.
		const TextureVertex quad_vertices[4] =
		{  //  x,  y, z, u, v
			{ -1, -1, 0, 0, 0 },
			{  1, -1, 0, 1, 0 },
			{  1,  1, 0, 1, 1 },
			{ -1,  1, 0, 0, 1 }
		};
		d_full_screen_quad_vertex_array->set_array_data(quad_vertices, quad_vertices + 4);
		d_full_screen_quad_vertex_array->gl_enable_client_state(GL_VERTEX_ARRAY);
		d_full_screen_quad_vertex_array->gl_enable_client_state(GL_TEXTURE_COORD_ARRAY);
		d_full_screen_quad_vertex_array->gl_vertex_pointer(3, GL_FLOAT, sizeof(TextureVertex), 0);
		d_full_screen_quad_vertex_array->gl_tex_coord_pointer(2, GL_FLOAT, sizeof(TextureVertex), 3 * sizeof(GLfloat));

		// Initialise the vertex element array for the full-screen quad.
		const GLushort quad_indices[4] = { 0, 1, 2, 3 };
		d_full_screen_quad_vertex_element_array->set_array_data(quad_indices, quad_indices + 4);
		d_full_screen_quad_vertex_element_array->gl_draw_range_elements_EXT(
				GL_QUADS, 0/*start*/, 3/*end*/, 4/*count*/, GL_UNSIGNED_SHORT/*type*/, 0 /*indices_offset*/);

		//
		// Setup rendering state for the three age grid mask render passes.
		//

		// Enable texturing and set the texture function.
		// It's the same for all three passes.
		GPlatesOpenGL::GLTextureEnvironmentState::non_null_ptr_type tex_env_state =
				GPlatesOpenGL::GLTextureEnvironmentState::create();
		tex_env_state->gl_enable_texture_2D(GL_TRUE);
		tex_env_state->gl_tex_env_mode(GL_REPLACE);
		d_first_age_mask_render_pass_state->add_state_set(tex_env_state);
		d_second_age_mask_render_pass_state->add_state_set(tex_env_state);
		d_third_age_mask_render_pass_state->add_state_set(tex_env_state);

#if 0
		// First pass alpha-test state.
		// This is used to write zeros when the fragment passes the alpha test.
		// The render target is initially all ones and the zeroes mask out areas that
		// should be hidden as determined by the age grid.
		GLAlphaTestState::non_null_ptr_type first_pass_alpha_test_state = GLAlphaTestState::create();
		first_pass_alpha_test_state->gl_enable(GL_TRUE).gl_alpha_func(GL_LEQUAL, GLclampf(0));
		d_first_age_mask_render_pass_state->add_state_set(first_pass_alpha_test_state);
#endif

		// Turns off colour channel writes for the first and second passes because
		// we're generated an alpha mask representing what should be drawn.
		GLMaskBuffersState::non_null_ptr_type mask_colour_channels_state = GLMaskBuffersState::create();
		mask_colour_channels_state->gl_color_mask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
		d_first_age_mask_render_pass_state->add_state_set(mask_colour_channels_state);
		d_second_age_mask_render_pass_state->add_state_set(mask_colour_channels_state);

		// Second pass alpha-blend state.
		GLBlendState::non_null_ptr_type second_pass_blend_state = GLBlendState::create();
		second_pass_blend_state->gl_enable(GL_TRUE).gl_blend_func(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		d_second_age_mask_render_pass_state->add_state_set(second_pass_blend_state);

		// Third pass alpha-blend state.
		GLBlendState::non_null_ptr_type third_pass_blend_state = GLBlendState::create();
		third_pass_blend_state->gl_enable(GL_TRUE).gl_blend_func(GL_DST_COLOR, GL_ZERO);
		d_third_age_mask_render_pass_state->add_state_set(third_pass_blend_state);
	}

	// Get the polygons grouped by rotation (plate id).
	// We do this so that polygon meshes associated with higher plate ids will get drawn
	// last and hence get drawn on top of other polygons in those cases where they overlap.
	std::vector<source_rotation_group_type::non_null_ptr_to_const_type> src_rotation_groups;
	reconstructing_polygons->get_rotation_groups_sorted_by_plate_id(src_rotation_groups);

	// Reserve space so we don't copy a lot when adding a new rotation group.
	// We may end up reserving more than we need but sizeof(RotationGroup) is not that big
	// so it should be fine.
	d_rotation_groups.reserve(src_rotation_groups.size());

	// Iterate over the source rotation groups in 'reconstructing_polygons'.
	BOOST_FOREACH(
			const source_rotation_group_type::non_null_ptr_to_const_type &src_rotation_group,
			src_rotation_groups)
	{
		// Add a new rotation group.
		d_rotation_groups.push_back(RotationGroup(src_rotation_group));
		RotationGroup &rotation_group = d_rotation_groups.back();

		// Iterate over the polygons in the source rotation group.
		BOOST_FOREACH(
				const source_polygon_region_type::non_null_ptr_to_const_type &src_polygon_region,
				src_rotation_group->polygon_regions)
		{
			// Clip the source polygon region to each face of the cube and then generate
			// a mesh for each cube face.
			generate_polygon_mesh(rotation_group, src_polygon_region);
		}
	}

	// Now that we've generated all the polygon meshes we can create a quad tree
	// for each face of the cube.
	initialise_cube_quad_trees();
}


unsigned int
GPlatesOpenGL::GLMultiResolutionReconstructedRaster::get_level_of_detail(
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
			2.0 / d_cube_subdivision->get_tile_texel_dimension();

	const float level_of_detail_factor = INVERSE_LOG2 *
			(std::log(max_lowest_resolution_texel_size_on_unit_sphere) -
					std::log(min_pixel_size_on_unit_sphere));

	// We need to round up instead of down and then clamp to zero.
	// We don't have an upper limit - as we traverse the quad tree to higher and higher
	// resolution nodes we might eventually reach the leaf nodes of the tree without
	// having satisified the requested level-of-detail resolution - in this case we'll
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
GPlatesOpenGL::GLMultiResolutionReconstructedRaster::render(
		GLRenderer &renderer)
{
	PROFILE_FUNC();

	// Make sure our cached version of the raster input's valid token is up to date
	// so our texture tiles can decide whether they need to re-render their texture caches.
	update_input_rasters_valid_tokens();

	// First make sure we've created our clip texture.
	// We do this here rather than in the constructor because we know we have an active
	// OpenGL context here - because we're rendering.
	if (!d_clip_texture)
	{
		create_clip_texture();
	}

	// Get the level-of-detail based on the size of viewport pixels projected onto the globe.
	// We'll try to render of this level of detail if our quad tree is deep enough.
	const unsigned int render_level_of_detail = get_level_of_detail(renderer.get_transform_state());

	const unsigned int num_rotation_groups = d_rotation_groups.size();

	// Iterate through the rotation groups.
	for (unsigned int rotation_group_index = 0;
		rotation_group_index < num_rotation_groups;
		++rotation_group_index)
	{
		const RotationGroup &rotation_group = d_rotation_groups[rotation_group_index];

		// Convert the rotation (based on plate id) from a unit quaternion to a matrix so
		// we can feed it to OpenGL.
		const GPlatesMaths::UnitQuaternion3D &quat_rotation = rotation_group.rotation->current_rotation;
		GLTransform::non_null_ptr_type rotation_transform = GLTransform::create(GL_MODELVIEW, quat_rotation);

		renderer.push_transform(*rotation_transform);

		// First get the view frustum planes.
		//
		// NOTE: We do this *after* pushing the above rotation transform because the
		// frustum planes are affected by the current model-view and projection transforms.
		// Out quad tree bounding boxes are in model space but the polygon meshes they
		// bound are rotating to new positions so we want to take that into account and map
		// the view frustum back to model space where we can test against our bounding boxes.
		//
		const GLTransformState::FrustumPlanes &frustum_planes =
				renderer.get_transform_state().get_current_frustum_planes_in_model_space();
		// There are six frustum planes initially active.
		const boost::uint32_t frustum_plane_mask = 0x3f; // 111111 in binary

		// Traverse the quad trees of the cube faces for the current rotation group.
		for (unsigned int face = 0; face < 6; ++face)
		{
			const QuadTree &quad_tree = d_cube.faces[face].quad_tree;

			if (quad_tree.root_node)
			{
				render_quad_tree(
						renderer,
						*quad_tree.root_node.get(),
						rotation_group_index,
						0/*level_of_detail*/,
						render_level_of_detail,
						frustum_planes,
						frustum_plane_mask);
			}
		}

		renderer.pop_transform();
	}
}

// See above
ENABLE_GCC_WARNING("-Wshadow")


void
GPlatesOpenGL::GLMultiResolutionReconstructedRaster::render_quad_tree(
		GLRenderer &renderer,
		QuadTreeNode &quad_tree_node,
		unsigned int rotation_group_index,
		unsigned int level_of_detail,
		unsigned int render_level_of_detail,
		const GLTransformState::FrustumPlanes &frustum_planes,
		boost::uint32_t frustum_plane_mask)
{
	// Get the rotation group that we are visiting.
	// The same quad tree is traversed once for each rotation group since each rotation group
	// has a different rotation and hence the view frustum culling will differ.
	const PartitionedRotationGroup::maybe_null_ptr_type &partitioned_rotation_group_opt =
			quad_tree_node.partitioned_rotation_groups[rotation_group_index];
	// If there are no partitioned meshes for the current rotation group and for this
	// current quad tree node then we can return without visiting out child nodes.
	if (!partitioned_rotation_group_opt)
	{
		return;
	}
	PartitionedRotationGroup &partitioned_rotation_group = *partitioned_rotation_group_opt;

	// If the frustum plane mask is zero then it means we are entirely inside the view frustum.
	// So only test for intersection if the mask is non-zero.
	if (frustum_plane_mask != 0)
	{
		// See if the OBB of the current OBB tree node intersects the view frustum.
		boost::optional<boost::uint32_t> out_frustum_plane_mask =
				GLIntersect::intersect_OBB_frustum(
						partitioned_rotation_group.bounding_box,
						frustum_planes.planes,
						frustum_plane_mask);
		if (!out_frustum_plane_mask)
		{
			// No intersection so OBB is outside the view frustum and we can cull it.
			return;
		}

		// Update the frustum plane mask so we only test against those planes that
		// the current bounding box intersects. The bounding box is entirely inside
		// the planes with a zero bit and so its child nodes are also entirely inside
		// those planes too and so they won't need to test against them.
		frustum_plane_mask = out_frustum_plane_mask.get();
	}

	// If we're at the right level of detail for rendering then do so and
	// return without traversing any child nodes.
	if (level_of_detail == render_level_of_detail)
	{
		render_quad_tree_node_tile(renderer, quad_tree_node, partitioned_rotation_group);

		return;
	}

	//
	// Iterate over the child subdivision regions and create if cover source raster.
	//

	bool have_child_nodes = false;
	for (unsigned int child_v_offset = 0; child_v_offset < 2; ++child_v_offset)
	{
		for (unsigned int child_u_offset = 0; child_u_offset < 2; ++child_u_offset)
		{
			const boost::optional<QuadTreeNode::non_null_ptr_type> &child_quad_tree_node =
					quad_tree_node.child_nodes[child_v_offset][child_u_offset];
			if (child_quad_tree_node)
			{
				have_child_nodes = true;

				render_quad_tree(
						renderer,
						*child_quad_tree_node.get(),
						rotation_group_index,
						level_of_detail + 1,
						render_level_of_detail,
						frustum_planes,
						frustum_plane_mask);
			}
		}
	}

	// If this quad tree node does not have any child nodes then it means we've been requested
	// to render at a resolution level that is too high for us and so we can only render at
	// the highest we can provide which is now.
	if (!have_child_nodes)
	{
		render_quad_tree_node_tile(renderer, quad_tree_node, partitioned_rotation_group);
	}
}


void
GPlatesOpenGL::GLMultiResolutionReconstructedRaster::render_quad_tree_node_tile(
		GLRenderer &renderer,
		QuadTreeNode &quad_tree_node,
		const PartitionedRotationGroup &partitioned_rotation_group)
{
	if (!d_using_age_grid)
	{
		// Get the source raster texture.
		// Since it's a cube texture it may, in turn, have to render its source raster
		// into its texture (which it then passes to us to use).
		GLTexture::shared_ptr_to_const_type source_raster_texture =
				d_raster_to_reconstruct->get_tile_texture(
						quad_tree_node.source_raster_tile, renderer);

		// Simply render the source raster to the scene.
		render_tile_to_scene(
				renderer, source_raster_texture, quad_tree_node, partitioned_rotation_group);

		return;
	}

	//
	// Get the texture for the tile - since we're using an age grid we need to cache the
	// results of age-masking the source raster to a tile texture before we can render
	// the tile to the main scene.
	//

	// See if we've generated our age masked tile texture and
	// see if it hasn't been recycled by the texture cache.
	boost::optional<GLTexture::shared_ptr_type> age_masked_tile_texture =
			quad_tree_node.age_masked_render_texture.get_object();
	if (!age_masked_tile_texture)
	{
		// We should have an age-masked texture cache if we're using age grids.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				d_age_masked_raster_texture_cache,
				GPLATES_ASSERTION_SOURCE);

		// We need to allocate a new texture from the texture cache and fill it with data.
		const std::pair<GLVolatileTexture, bool/*recycled*/> volatile_texture_result =
				d_age_masked_raster_texture_cache.get()->allocate_object();

		// Extract allocation results.
		quad_tree_node.age_masked_render_texture = volatile_texture_result.first;
		const bool texture_was_recycled = volatile_texture_result.second;

		// Get the tile texture again - this time it should have a valid texture.
		age_masked_tile_texture = quad_tree_node.age_masked_render_texture.get_object();
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				age_masked_tile_texture,
				GPLATES_ASSERTION_SOURCE);

		// If the texture is not recycled then its a newly allocated texture so we need
		// to create it in OpenGL before we can load data into it.
		if (!texture_was_recycled)
		{
			create_age_masked_tile_texture(age_masked_tile_texture.get());
		}

		// Render the source raster, age-masked, into our tile texture.
		render_age_masked_source_raster_into_tile(renderer, age_masked_tile_texture.get(), quad_tree_node);
	}

	// Our texture wasn't recycled but see if it's still valid in case
	// any of the input rasters changed underneath us.
	if (!quad_tree_node.source_texture_valid_token.is_still_valid(d_source_raster_valid_token) ||
		!quad_tree_node.age_grid_mask_texture_valid_token.is_still_valid(d_age_grid_mask_raster_valid_token) ||
		!quad_tree_node.age_grid_coverage_texture_valid_token.is_still_valid(d_age_grid_coverage_raster_valid_token))
	{
		// Render the source raster, age-masked, into our tile texture.
		render_age_masked_source_raster_into_tile(renderer, age_masked_tile_texture.get(), quad_tree_node);
	}

	// Now that we've got a texture that represents the age-masked source raster we can
	// render it to the scene.
	render_tile_to_scene(
			renderer, age_masked_tile_texture.get(), quad_tree_node, partitioned_rotation_group);
}


// The BOOST_FOREACH macro in versions of boost before 1.37 uses the same local
// variable name in each instantiation. Nested BOOST_FOREACH macros therefore
// cause GCC to warn about shadowed declarations.
DISABLE_GCC_WARNING("-Wshadow")

void
GPlatesOpenGL::GLMultiResolutionReconstructedRaster::render_age_masked_source_raster_into_tile(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_to_const_type &age_mask_tile_texture,
		QuadTreeNode &quad_tree_node)
{
	//                      Get raster tile (texture) - may involve GLMRCR pushing a render target if not cached.
	//                      Get age grid coverage tile (texture) - may involve GLMRCR pushing a render target.
	//                      Get age grid mask tile (texture) - may involve GLMRCR pushing a render target.
	//                      Push render target (blank OpenGL state)
	//                         Set viewport, alpha-blending, etc.
	//                         Mask writes to the alpha channel.
	//                         First pass: render age grid mask as a single quad covering viewport.
	//                         Second pass:
	//                            Set projection matrix using that of the quad tree node.
	//                            Bind age grid coverage texture to texture unit 0.
	//                            Set texgen/texture matrix state for texture unit using projection matrix
	//                              of quad tree node (and any adjustments).
	//                            Iterate over polygon meshes (regardless of polygon age):
	//                               Wrap a vertex array drawable around mesh triangles and polygon vertex array
	//                                 and add to the renderer.
	//                            Enditerate
	//                         Third pass:
	//                            Unmask alpha channel.
	//                            Setup multiplicative blending using GL_DST_COLOR.
	//                            Render raster tile as a single quad covering viewport.
	//                      Pop render target.
	//                      Render same as below (case for no age grid) except render all polygon meshes
	//                         regardless of age of appearance/disappearance.


	// Get the source raster texture.
	// Since it's a cube texture it may, in turn, have to render its source raster
	// into its texture (which it then passes to us to use).
	GLTexture::shared_ptr_to_const_type source_raster_texture =
			d_raster_to_reconstruct->get_tile_texture(
					quad_tree_node.source_raster_tile,
					renderer);

	// For now, if we're using an age grid then we should have an age grid tile
	// in every quad tree node - we would have terminated quad-tree creation otherwise.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			quad_tree_node.age_grid_mask_tile && quad_tree_node.age_grid_coverage_tile,
			GPLATES_ASSERTION_SOURCE);

	// Get the age grid mask texture.
	// Since it's a cube texture it may, in turn, have to render its source raster
	// into its texture (which it then passes to us to use).
	GLTexture::shared_ptr_to_const_type age_grid_mask_texture =
			d_age_grid_mask_raster.get()->get_tile_texture(
					quad_tree_node.age_grid_mask_tile.get(),
					renderer);

	// Get the age grid coverage texture.
	// Since it's a cube texture it may, in turn, have to render its source raster
	// into its texture (which it then passes to us to use).
	GLTexture::shared_ptr_to_const_type age_grid_coverage_texture =
			d_age_grid_coverage_raster.get()->get_tile_texture(
					quad_tree_node.age_grid_coverage_tile.get(),
					renderer);

	// Push a render target that will render to the tile texture.
	renderer.push_render_target(
			GLTextureRenderTargetType::create(
					age_mask_tile_texture, d_tile_texel_dimension, d_tile_texel_dimension));

	// Push the viewport state set.
	renderer.push_state_set(d_viewport_state);
	// Let the transform state know of the new viewport.
	renderer.get_transform_state().set_viewport(d_viewport);

	// Clear the colour buffer of the render target.
	renderer.push_state_set(d_clear_buffers_state);
	renderer.add_drawable(d_clear_buffers);
	renderer.pop_state_set();

	// NOTE: We leave the model-view and projection matrices as identity as that is what we
	// we need to draw a full-screen quad.

	// The full-screen quad drawable.
	GLVertexArrayDrawable::non_null_ptr_type full_screen_quad_drawable =
			GLVertexArrayDrawable::create(
					d_full_screen_quad_vertex_array, d_full_screen_quad_vertex_element_array);


	// Create a state set that binds the source raster texture to texture unit 0.
	GLBindTextureState::non_null_ptr_type bind_source_raster_texture = GLBindTextureState::create();
	bind_source_raster_texture->gl_bind_texture(GL_TEXTURE_2D, source_raster_texture);

	// Create a state set that binds the age grid mask texture to texture unit 0.
	GLBindTextureState::non_null_ptr_type bind_age_grid_mask_texture = GLBindTextureState::create();
	bind_age_grid_mask_texture->gl_bind_texture(GL_TEXTURE_2D, age_grid_mask_texture);

	// Create a state set that binds the age grid coverage texture to texture unit 0.
	GLBindTextureState::non_null_ptr_type bind_age_grid_coverage_texture = GLBindTextureState::create();
	bind_age_grid_coverage_texture->gl_bind_texture(GL_TEXTURE_2D, age_grid_coverage_texture);


	//
	// Set the state for the first render pass and render.
	//

	renderer.push_state_set(d_first_age_mask_render_pass_state);
	renderer.push_state_set(bind_age_grid_mask_texture);
 	renderer.add_drawable(full_screen_quad_drawable);
	renderer.pop_state_set();
	renderer.pop_state_set();

	//
	// Set the state for the second render pass and render.
	//

	renderer.push_state_set(d_second_age_mask_render_pass_state);
	renderer.push_state_set(bind_age_grid_coverage_texture);

	// Set up texture coordinate generation from the vertices (x,y,z) and
	// set up a texture matrix to perform the model-view and projection transforms
	// of the frustum of the current tile.
	GLTextureTransformState::non_null_ptr_type age_grid_coverage_texture_transform_state =
			GLTextureTransformState::create();
	age_grid_coverage_texture_transform_state->gl_active_texture_ARB(GLContext::TextureParameters::gl_texture0_ARB);
	GLTextureTransformState::TexGenCoordState age_grid_coverage_tex_gen_state_s;
	GLTextureTransformState::TexGenCoordState age_grid_coverage_tex_gen_state_t;
	GLTextureTransformState::TexGenCoordState age_grid_coverage_tex_gen_state_r;
	GLTextureTransformState::TexGenCoordState age_grid_coverage_tex_gen_state_q;
	age_grid_coverage_tex_gen_state_s.gl_enable_texture_gen(GL_TRUE);
	age_grid_coverage_tex_gen_state_t.gl_enable_texture_gen(GL_TRUE);
	age_grid_coverage_tex_gen_state_r.gl_enable_texture_gen(GL_TRUE);
	age_grid_coverage_tex_gen_state_q.gl_enable_texture_gen(GL_TRUE);
	age_grid_coverage_tex_gen_state_s.gl_tex_gen_mode(GL_OBJECT_LINEAR);
	age_grid_coverage_tex_gen_state_t.gl_tex_gen_mode(GL_OBJECT_LINEAR);
	age_grid_coverage_tex_gen_state_r.gl_tex_gen_mode(GL_OBJECT_LINEAR);
	age_grid_coverage_tex_gen_state_q.gl_tex_gen_mode(GL_OBJECT_LINEAR);
	const GLdouble age_grid_coverage_object_plane[4][4] =
	{
		{ 1, 0, 0, 0 },
		{ 0, 1, 0, 0 },
		{ 0, 0, 1, 0 },
		{ 0, 0, 0, 1 }
	};
	age_grid_coverage_tex_gen_state_s.gl_object_plane(age_grid_coverage_object_plane[0]);
	age_grid_coverage_tex_gen_state_t.gl_object_plane(age_grid_coverage_object_plane[1]);
	age_grid_coverage_tex_gen_state_r.gl_object_plane(age_grid_coverage_object_plane[2]);
	age_grid_coverage_tex_gen_state_q.gl_object_plane(age_grid_coverage_object_plane[3]);
	age_grid_coverage_texture_transform_state->set_tex_gen_coord_state(GL_S, age_grid_coverage_tex_gen_state_s);
	age_grid_coverage_texture_transform_state->set_tex_gen_coord_state(GL_T, age_grid_coverage_tex_gen_state_t);
	age_grid_coverage_texture_transform_state->set_tex_gen_coord_state(GL_R, age_grid_coverage_tex_gen_state_r);
	age_grid_coverage_texture_transform_state->set_tex_gen_coord_state(GL_Q, age_grid_coverage_tex_gen_state_q);
	GLMatrix age_grid_coverage_texture_matrix;
	// Convert the clip-space range (-1,1) to texture coord range (0, 1).
	age_grid_coverage_texture_matrix.gl_scale(0.5, 0.5, 1.0);
	age_grid_coverage_texture_matrix.gl_translate(1.0, 1.0, 0.0);
	age_grid_coverage_texture_matrix.gl_mult_matrix(quad_tree_node.projection_transform->get_matrix());
	age_grid_coverage_texture_matrix.gl_mult_matrix(quad_tree_node.view_transform->get_matrix());
	age_grid_coverage_texture_transform_state->gl_load_matrix(age_grid_coverage_texture_matrix);

	renderer.push_state_set(age_grid_coverage_texture_transform_state);

	renderer.push_transform(*quad_tree_node.projection_transform);
	renderer.push_transform(*quad_tree_node.view_transform);

	// Use the current reconstruction time to determine which polygons to draw based
	// on their time period.
	const GPlatesPropertyValues::GeoTimeInstant reconstruction_time(
			d_reconstructing_polygons->get_current_reconstruction_time());

	// Iterate over *all* polygon meshes for this quad tree node and draw them.
	// NOTE: Not just the polygon meshes for the current rotation group being rendered.
	//
	// Since we're using an age grid we're rendering the age grid mask combined
	// with the polygon mask (where there's no age grid coverage) and so we need to draw
	// all polygons that cover the current quad tree node - this is also because the results
	// get cached into the tile texture and other visits to this quad tree node in the same
	// scene render (by different rotation groups) can use the cached texture.
	//
	BOOST_FOREACH(
			const PartitionedRotationGroup::maybe_null_ptr_type &partitioned_rotation_group,
			quad_tree_node.partitioned_rotation_groups)
	{
		// Not all rotation groups will have polygon meshes covering the current quad tree node tile.
		if (!partitioned_rotation_group)
		{
			continue;
		}

		BOOST_FOREACH(const PartitionedMesh &partitioned_mesh, partitioned_rotation_group->partitioned_meshes)
		{
			const Polygon &polygon = *partitioned_mesh.polygon;

			// If the current reconstruction time is within the time period of the current
			// polygon then we can display it.
			if (polygon.time_of_appearance.is_earlier_than_or_coincident_with(reconstruction_time) &&
				reconstruction_time.is_earlier_than_or_coincident_with(polygon.time_of_disappearance))
			{
				// Add the drawable to the current render target.
				renderer.add_drawable(
						GLVertexArrayDrawable::create(
								polygon.vertex_array,
								partitioned_mesh.vertex_element_array));
			}
		}
	}

	renderer.pop_transform();
	renderer.pop_transform();

	renderer.pop_state_set(); // age_grid_coverage_texture_transform_state

	renderer.pop_state_set(); // bind_age_grid_coverage_texture
	renderer.pop_state_set(); // d_second_age_mask_render_pass_state

	//
	// Set the state for the third render pass and render.
	//

	renderer.push_state_set(d_third_age_mask_render_pass_state);
	renderer.push_state_set(bind_source_raster_texture);
	renderer.add_drawable(full_screen_quad_drawable);
	renderer.pop_state_set();
	renderer.pop_state_set();


	// Pop the viewport state set.
	renderer.pop_state_set();

	renderer.pop_render_target();

	// This tile texture is now update-to-date with the inputs used to generate it.
	quad_tree_node.source_texture_valid_token = d_source_raster_valid_token;
	quad_tree_node.age_grid_mask_texture_valid_token = d_age_grid_mask_raster_valid_token;
	quad_tree_node.age_grid_coverage_texture_valid_token = d_age_grid_coverage_raster_valid_token;
}

// See above.
ENABLE_GCC_WARNING("-Wshadow")


void
GPlatesOpenGL::GLMultiResolutionReconstructedRaster::render_tile_to_scene(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_to_const_type &source_raster_texture,
		QuadTreeNode &quad_tree_node,
		const PartitionedRotationGroup &partitioned_rotation_group)
{
	//
	// Bind clip texture to texture unit 0.
	// Bind raster texture to texture unit 1.
	// Set texture function to modulate.
	// Set texgen/texture matrix state for each texture unit using projection matrix
	//   of quad tree node (and any adjustments).
	//
	// If using age grid:
	//   Iterate over polygon meshes (regardless of polygon age):
	//     Wrap a vertex array drawable around mesh triangles and polygon vertex array
	//       and add to the renderer.
	//   Enditerate
	// Else:
	//   Iterate over polygon meshes:
	//     If older than current reconstruction time:
	//       Wrap a vertex array drawable around mesh triangles and polygon vertex array
	//         and add to the renderer.
	//     Endif
	//   Enditerate
	// Endif
	//

	// Create a container for a group of state sets.
	GLCompositeStateSet::non_null_ptr_type state_set = GLCompositeStateSet::create();

	// Create a state set that binds the clip texture to texture unit 0.
	GLBindTextureState::non_null_ptr_type bind_clip_texture = GLBindTextureState::create();
	bind_clip_texture->gl_active_texture_ARB(GLContext::TextureParameters::gl_texture0_ARB);
	bind_clip_texture->gl_bind_texture(GL_TEXTURE_2D, d_clip_texture);
	state_set->add_state_set(bind_clip_texture);

	// Set the texture environment state on texture unit 0.
	GLTextureEnvironmentState::non_null_ptr_type clip_texture_environment_state =
			GLTextureEnvironmentState::create();
	clip_texture_environment_state->gl_active_texture_ARB(GLContext::TextureParameters::gl_texture0_ARB);
	clip_texture_environment_state->gl_enable_texture_2D(GL_TRUE);
	clip_texture_environment_state->gl_tex_env_mode(GL_REPLACE);
	state_set->add_state_set(clip_texture_environment_state);

	// Set up texture coordinate generation from the vertices (x,y,z) and
	// set up a texture matrix to perform the model-view and projection transforms
	// of the frustum of the current tile.
	GLTextureTransformState::non_null_ptr_type clip_texture_transform_state =
			GLTextureTransformState::create();
	clip_texture_transform_state->gl_active_texture_ARB(GLContext::TextureParameters::gl_texture0_ARB);
	GLTextureTransformState::TexGenCoordState clip_tex_gen_state_s;
	GLTextureTransformState::TexGenCoordState clip_tex_gen_state_t;
	GLTextureTransformState::TexGenCoordState clip_tex_gen_state_r;
	GLTextureTransformState::TexGenCoordState clip_tex_gen_state_q;
	clip_tex_gen_state_s.gl_enable_texture_gen(GL_TRUE);
	clip_tex_gen_state_t.gl_enable_texture_gen(GL_TRUE);
	clip_tex_gen_state_r.gl_enable_texture_gen(GL_TRUE);
	clip_tex_gen_state_q.gl_enable_texture_gen(GL_TRUE);
	clip_tex_gen_state_s.gl_tex_gen_mode(GL_OBJECT_LINEAR);
	clip_tex_gen_state_t.gl_tex_gen_mode(GL_OBJECT_LINEAR);
	clip_tex_gen_state_r.gl_tex_gen_mode(GL_OBJECT_LINEAR);
	clip_tex_gen_state_q.gl_tex_gen_mode(GL_OBJECT_LINEAR);
	const GLdouble clip_object_plane[4][4] =
	{
		{ 1, 0, 0, 0 },
		{ 0, 1, 0, 0 },
		{ 0, 0, 1, 0 },
		{ 0, 0, 0, 1 }
	};
	clip_tex_gen_state_s.gl_object_plane(clip_object_plane[0]);
	clip_tex_gen_state_t.gl_object_plane(clip_object_plane[1]);
	clip_tex_gen_state_r.gl_object_plane(clip_object_plane[2]);
	clip_tex_gen_state_q.gl_object_plane(clip_object_plane[3]);
	clip_texture_transform_state->set_tex_gen_coord_state(GL_S, clip_tex_gen_state_s);
	clip_texture_transform_state->set_tex_gen_coord_state(GL_T, clip_tex_gen_state_t);
	clip_texture_transform_state->set_tex_gen_coord_state(GL_R, clip_tex_gen_state_r);
	clip_texture_transform_state->set_tex_gen_coord_state(GL_Q, clip_tex_gen_state_q);
	GLMatrix clip_texture_matrix;
	// Convert the clip-space range (-1,1) to texture coord range (0.25, 0.75) so that the
	// frustrum edges will map to the boundary of the interior 2x2 clip region of our
	// 4x4 clip texture.
	clip_texture_matrix.gl_translate(0.5, 0.5, 0.0);
	clip_texture_matrix.gl_scale(0.25, 0.25, 1.0);
	clip_texture_matrix.gl_mult_matrix(quad_tree_node.projection_transform->get_matrix());
	clip_texture_matrix.gl_mult_matrix(quad_tree_node.view_transform->get_matrix());
	clip_texture_transform_state->gl_load_matrix(clip_texture_matrix);
	state_set->add_state_set(clip_texture_transform_state);


	// Create a state set that binds the source raster tile texture to texture unit 1.
	GLBindTextureState::non_null_ptr_type bind_tile_texture = GLBindTextureState::create();
	bind_tile_texture->gl_active_texture_ARB(GLContext::TextureParameters::gl_texture0_ARB + 1);
	bind_tile_texture->gl_bind_texture(GL_TEXTURE_2D, source_raster_texture);
	state_set->add_state_set(bind_tile_texture);

	// Set the texture environment state on texture unit 1.
	// We want to modulate with the clip texture on unit 0.
	GLTextureEnvironmentState::non_null_ptr_type texture_environment_state =
			GLTextureEnvironmentState::create();
	texture_environment_state->gl_active_texture_ARB(GLContext::TextureParameters::gl_texture0_ARB + 1);
	texture_environment_state->gl_enable_texture_2D(GL_TRUE);
	texture_environment_state->gl_tex_env_mode(GL_MODULATE);
	state_set->add_state_set(texture_environment_state);

	// Set up texture coordinate generation from the vertices (x,y,z) and
	// set up a texture matrix to perform the model-view and projection transforms
	// of the frustum of the current tile.
	// Set it on same texture unit, ie texture unit 1.
	GLTextureTransformState::non_null_ptr_type texture_transform_state =
			GLTextureTransformState::create();
	texture_transform_state->gl_active_texture_ARB(GLContext::TextureParameters::gl_texture0_ARB + 1);
	GLTextureTransformState::TexGenCoordState tex_gen_state_s;
	GLTextureTransformState::TexGenCoordState tex_gen_state_t;
	GLTextureTransformState::TexGenCoordState tex_gen_state_r;
	GLTextureTransformState::TexGenCoordState tex_gen_state_q;
	tex_gen_state_s.gl_enable_texture_gen(GL_TRUE);
	tex_gen_state_t.gl_enable_texture_gen(GL_TRUE);
	tex_gen_state_r.gl_enable_texture_gen(GL_TRUE);
	tex_gen_state_q.gl_enable_texture_gen(GL_TRUE);
	tex_gen_state_s.gl_tex_gen_mode(GL_OBJECT_LINEAR);
	tex_gen_state_t.gl_tex_gen_mode(GL_OBJECT_LINEAR);
	tex_gen_state_r.gl_tex_gen_mode(GL_OBJECT_LINEAR);
	tex_gen_state_q.gl_tex_gen_mode(GL_OBJECT_LINEAR);
	const GLdouble object_plane[4][4] =
	{
		{ 1, 0, 0, 0 },
		{ 0, 1, 0, 0 },
		{ 0, 0, 1, 0 },
		{ 0, 0, 0, 1 }
	};
	tex_gen_state_s.gl_object_plane(object_plane[0]);
	tex_gen_state_t.gl_object_plane(object_plane[1]);
	tex_gen_state_r.gl_object_plane(object_plane[2]);
	tex_gen_state_q.gl_object_plane(object_plane[3]);
	texture_transform_state->set_tex_gen_coord_state(GL_S, tex_gen_state_s);
	texture_transform_state->set_tex_gen_coord_state(GL_T, tex_gen_state_t);
	texture_transform_state->set_tex_gen_coord_state(GL_R, tex_gen_state_r);
	texture_transform_state->set_tex_gen_coord_state(GL_Q, tex_gen_state_q);
	GLMatrix texture_matrix;
	texture_matrix.gl_scale(0.5, 0.5, 1.0);
	texture_matrix.gl_translate(1.0, 1.0, 0.0);
	texture_matrix.gl_mult_matrix(quad_tree_node.projection_transform->get_matrix());
	texture_matrix.gl_mult_matrix(quad_tree_node.view_transform->get_matrix());
	texture_transform_state->gl_load_matrix(texture_matrix);
	state_set->add_state_set(texture_transform_state);


	// Enable alpha-blending in case texture has partial transparency.
	GPlatesOpenGL::GLBlendState::non_null_ptr_type blend_state =
			GPlatesOpenGL::GLBlendState::create();
	blend_state->gl_enable(GL_TRUE).gl_blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	state_set->add_state_set(blend_state);

#if 0
	GLPolygonState::non_null_ptr_type polygon_state = GLPolygonState::create();
	polygon_state->gl_polygon_mode(GL_FRONT_AND_BACK, GL_LINE);
	state_set->add_state_set(polygon_state);
#endif

	// Push the state set onto the state graph.
	renderer.push_state_set(state_set);

	if (d_using_age_grid)
	{
		//
		// If we're using an age grid then we've already rendered the age grid mask combined
		// with the polygon mask (where there's no age grid coverage) and so we don't need
		// mask out polygons using their age (this has already been taken care of).
		//

		// Iterate over all polygon meshes for the current quad tree node and rotation group.
		BOOST_FOREACH(const PartitionedMesh &partitioned_mesh, partitioned_rotation_group.partitioned_meshes)
		{
			const Polygon &polygon = *partitioned_mesh.polygon;

			// Add the drawable to the current render target.
			renderer.add_drawable(
					GLVertexArrayDrawable::create(
							polygon.vertex_array,
							partitioned_mesh.vertex_element_array));
		}
	}
	else // not using age grid ...
	{
		//
		// We're not using an age grid so we have to use the polygon ages to mask out
		// regions that should not be drawn at a particular reconstruction time.
		// This is not as smooth as the per-pixel effect of the age grid.
		//

		// Use the current reconstruction time to determine which polygons to draw based
		// on their time period.
		const GPlatesPropertyValues::GeoTimeInstant reconstruction_time(
				d_reconstructing_polygons->get_current_reconstruction_time());

		// Iterate over the polygon meshes for the current quad tree node *and*
		// rotation group and draw them.
		BOOST_FOREACH(const PartitionedMesh &partitioned_mesh, partitioned_rotation_group.partitioned_meshes)
		{
			const Polygon &polygon = *partitioned_mesh.polygon;

			// If the current reconstruction time is within the time period of the current
			// polygon then we can display it.
			if (polygon.time_of_appearance.is_earlier_than_or_coincident_with(reconstruction_time) &&
				reconstruction_time.is_earlier_than_or_coincident_with(polygon.time_of_disappearance))
			{
				// Add the drawable to the current render target.
				renderer.add_drawable(
						GLVertexArrayDrawable::create(
								polygon.vertex_array,
								partitioned_mesh.vertex_element_array));
			}
		}
	}

	// Pop the state set.
	renderer.pop_state_set();
}


// We use macros in <GL/glew.h> that contain old-style casts.
DISABLE_GCC_WARNING("-Wold-style-cast")

void
GPlatesOpenGL::GLMultiResolutionReconstructedRaster::create_age_masked_tile_texture(
		const GLTexture::shared_ptr_type &texture)
{
	// Bind the texture so its the current texture.
	// Here we actually make a direct OpenGL call to bind the texture to the currently
	// active texture unit. It doesn't matter what the current texture unit is because
	// the only reason we're binding the texture object is so we can set its state =
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


void
GPlatesOpenGL::GLMultiResolutionReconstructedRaster::create_clip_texture()
{
	d_clip_texture = GLTexture::create(d_texture_resource_manager);

	// Bind the texture so its the current texture.
	// Here we actually make a direct OpenGL call to bind the texture to the currently
	// active texture unit. It doesn't matter what the current texture unit is because
	// the only reason we're binding the texture object is so we can set its state =
	// so that subsequent binds of this texture object, when we render the scene graph,
	// will set that state to OpenGL.
	d_clip_texture->gl_bind_texture(GL_TEXTURE_2D);

	//
	// We *must* use nearest neighbour filtering otherwise the clip texture won't work.
	// We are relying on the hard transition from white to black to clip for us.
	//
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	//
	// The clip texture is a 4x4 image where the centre 2x2 texels are 1.0
	// and the boundary texels are 0.0.
	// We will use the alpha channel for alpha-testing (to discard clipped regions)
	// and we'll use the colour channels to modulate
	//
	const GPlatesGui::rgba8_t mask_zero(0, 0, 0, 0);
	const GPlatesGui::rgba8_t mask_one(255, 255, 255, 255);
	const GPlatesGui::rgba8_t mask_image[16] =
	{
		mask_zero, mask_zero, mask_zero, mask_zero,
		mask_zero, mask_one,  mask_one,  mask_zero,
		mask_zero, mask_one,  mask_one,  mask_zero,
		mask_zero, mask_zero, mask_zero, mask_zero
	};

	// Create the texture and load the data into it.
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, mask_image);

	// Check there are no OpenGL errors.
	GLUtils::assert_no_gl_errors(GPLATES_ASSERTION_SOURCE);
}


// The BOOST_FOREACH macro in versions of boost before 1.37 uses the same local
// variable name in each instantiation. Nested BOOST_FOREACH macros therefore
// cause GCC to warn about shadowed declarations.
DISABLE_GCC_WARNING("-Wshadow")

void
GPlatesOpenGL::GLMultiResolutionReconstructedRaster::initialise_cube_quad_trees()
{
	PROFILE_FUNC();

	// Used to track partitioning of polygon meshes as we traverse down the quad tree.
	// There's one builder in the sequence for each rotation group.
	partitioned_rotation_group_builder_seq_type partitioned_rotation_group_builders;
	partitioned_rotation_group_builders.reserve(d_rotation_groups.size());

	// Iterate over the rotation groups.
	BOOST_FOREACH(RotationGroup &rotation_group, d_rotation_groups)
	{
		// Add a builder for the current rotation group.
		partitioned_rotation_group_builders.push_back(PartitionedRotationGroupBuilder());
		PartitionedRotationGroupBuilder &partitioned_rotation_group_builder =
				partitioned_rotation_group_builders.back();

		// Prepare some information about the polygons of the current rotation group,
		// such as the polygon mesh vertices and triangles, before we traverse down the
		// cube face quad trees and partition into smaller subdivisions.
		partitioned_mesh_builder_seq_type &partitioned_mesh_builders =
				partitioned_rotation_group_builder.partitioned_mesh_builders;
		partitioned_mesh_builders.reserve(rotation_group.polygons.size());

		BOOST_FOREACH(const Polygon::non_null_ptr_type &polygon, rotation_group.polygons)
		{
			// Add a builder for the current polygon.
			partitioned_mesh_builders.push_back(PartitionedMeshBuilder(polygon));
			PartitionedMeshBuilder &partitioned_mesh_builder = partitioned_mesh_builders.back();

			// Store the triangle indices for the current polygon.
			partitioned_mesh_builder.vertex_element_array_data = polygon->vertex_element_array_data;
		}
	}

	// Iterate over the faces of the cube and then traverse the quad tree of each face.
	for (unsigned int face = 0; face < 6; ++face)
	{
		GLCubeSubdivision::CubeFaceType cube_face =
				static_cast<GLCubeSubdivision::CubeFaceType>(face);

		// Start traversing the root of the quad tree of the same cube face in the source raster.
		boost::optional<GLMultiResolutionCubeRaster::QuadTreeNode> source_raster_quad_tree_root_node =
				d_raster_to_reconstruct->get_root_quad_tree_node(cube_face);
		if (!source_raster_quad_tree_root_node)
		{
			// Source raster does not cover the current cube face so
			// we don't need to generate a quad tree.
			continue;
		}

		boost::optional<GLMultiResolutionCubeRaster::QuadTreeNode> age_grid_mask_raster_quad_tree_root_node;
		boost::optional<GLMultiResolutionCubeRaster::QuadTreeNode> age_grid_coverage_raster_quad_tree_root_node;
		if (d_using_age_grid)
		{
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					d_age_grid_mask_raster && d_age_grid_coverage_raster,
					GPLATES_ASSERTION_SOURCE);

			// Get the root quad tree nodes from the age grid.
			age_grid_mask_raster_quad_tree_root_node =
					d_age_grid_mask_raster.get()->get_root_quad_tree_node(cube_face);
			age_grid_coverage_raster_quad_tree_root_node =
					d_age_grid_coverage_raster.get()->get_root_quad_tree_node(cube_face);

			// For now, we only proceed as far down the quad-tree as we have enough resolution
			// in *both* the age grid and the source raster. This limits the resolution to
			// the minimum of the two.
			// FIXME: Change this to proceed down the quad-tree as long as *either* the
			// age grid or source raster has enough resolution.
			if (!age_grid_mask_raster_quad_tree_root_node &&
				!age_grid_coverage_raster_quad_tree_root_node)
			{
				continue;
			}
		}

		// Recursively generate a quad tree for the current cube face.
		boost::optional<QuadTreeNode::non_null_ptr_type> quad_tree_root_node =
				create_quad_tree_node(
						cube_face,
						partitioned_rotation_group_builders,
						source_raster_quad_tree_root_node.get(),
						age_grid_mask_raster_quad_tree_root_node,
						age_grid_coverage_raster_quad_tree_root_node,
						0,/*level_of_detail*/
						0,/*tile_u_offset*/
						0/*tile_v_offset*/);

		QuadTree &quad_tree = d_cube.faces[face].quad_tree;
		quad_tree.root_node = quad_tree_root_node;
	}
}

// See above.
ENABLE_GCC_WARNING("-Wshadow")


boost::optional<GPlatesOpenGL::GLMultiResolutionReconstructedRaster::QuadTreeNode::non_null_ptr_type>
GPlatesOpenGL::GLMultiResolutionReconstructedRaster::create_quad_tree_node(
		GLCubeSubdivision::CubeFaceType cube_face,
		const partitioned_rotation_group_builder_seq_type &parent_partitioned_rotation_group_builders,
		const GLMultiResolutionCubeRaster::QuadTreeNode &source_raster_quad_tree_node,
		const boost::optional<GLMultiResolutionCubeRaster::QuadTreeNode> &age_grid_mask_raster_quad_tree_node,
		const boost::optional<GLMultiResolutionCubeRaster::QuadTreeNode> &age_grid_coverage_raster_quad_tree_node,
		unsigned int level_of_detail,
		unsigned int tile_u_offset,
		unsigned int tile_v_offset)
{
	// Create a transform state so we can get the clip planes of the cube subdivision
	// corresponding to the current quad tree node.
	GLTransformState::non_null_ptr_type transform_state = GLTransformState::create();

	const GLTransform::non_null_ptr_to_const_type projection_transform =
			d_cube_subdivision->get_projection_transform(
					cube_face, level_of_detail, tile_u_offset, tile_v_offset);
	transform_state->load_transform(*projection_transform);

	const GLTransform::non_null_ptr_to_const_type view_transform =
			d_cube_subdivision->get_view_transform(
					cube_face, level_of_detail, tile_u_offset, tile_v_offset);
	transform_state->load_transform(*view_transform);

	const GLTransformState::FrustumPlanes &frustum_planes =
			transform_state->get_current_frustum_planes_in_model_space();

	// The box bounding the meshes of this quad tree node.
	// Use the average of the left/right/bottom/top frustum plane normals as our
	// OBB z-axis. And use the average of the left and negative right plane normals
	// as our OBB y-axis.
	const GPlatesMaths::Vector3D left_plane_normal(
			frustum_planes.planes[GLTransformState::FrustumPlanes::LEFT_PLANE].get_normal().get_normalisation());
	const GPlatesMaths::Vector3D right_plane_normal(
			frustum_planes.planes[GLTransformState::FrustumPlanes::RIGHT_PLANE].get_normal().get_normalisation());
	const GPlatesMaths::Vector3D bottom_plane_normal(
			frustum_planes.planes[GLTransformState::FrustumPlanes::BOTTOM_PLANE].get_normal().get_normalisation());
	const GPlatesMaths::Vector3D top_plane_normal(
			frustum_planes.planes[GLTransformState::FrustumPlanes::TOP_PLANE].get_normal().get_normalisation());
	const GPlatesMaths::UnitVector3D obb_z_axis =
			(left_plane_normal + right_plane_normal + bottom_plane_normal + top_plane_normal).get_normalisation();
	const GPlatesMaths::Vector3D obb_y_axis = left_plane_normal - right_plane_normal;
	GLIntersect::OrientedBoundingBoxBuilder bounding_box_builder =
			GLIntersect::create_oriented_bounding_box_builder(obb_y_axis, obb_z_axis);
	// Add the extremal point along the z-axis which is just the z-axis point itself.
	bounding_box_builder.add(obb_z_axis);

	const unsigned int num_rotation_groups = parent_partitioned_rotation_group_builders.size();

	// This will contain the partitioned mesh builders to be used by our child quad tree nodes.
	partitioned_rotation_group_builder_seq_type partitioned_rotation_group_builders;
	partitioned_rotation_group_builders.reserve(num_rotation_groups);

	// This will contain the partitioned meshes to be stored in the our quad tree node.
	partitioned_rotation_group_seq_type partitioned_rotation_groups;
	partitioned_rotation_groups.reserve(num_rotation_groups);

	bool are_polygons_in_any_rotation_group = false;

	// Iterate over the rotation groups and refine the parent partitioning.
	// The refinement happens because the current quad tree node covers a smaller area of
	// the globe that its parent node.
	for (unsigned int rotation_group = 0; rotation_group < num_rotation_groups; ++rotation_group)
	{
		const PartitionedRotationGroupBuilder &parent_partitioned_rotation_group_builder =
				parent_partitioned_rotation_group_builders[rotation_group];

		// The current partitioned rotation group.
		PartitionedRotationGroup::maybe_null_ptr_type partitioned_rotation_group;

		// The current partitioned rotation group builder.
		partitioned_rotation_group_builders.push_back(PartitionedRotationGroupBuilder());
		PartitionedRotationGroupBuilder &partitioned_rotation_group_builder =
				partitioned_rotation_group_builders.back();

		// If there are partitioned meshes in the parent for the current rotation group...
		if (!parent_partitioned_rotation_group_builder.partitioned_mesh_builders.empty())
		{
			partitioned_rotation_group = partition_rotation_group(
					parent_partitioned_rotation_group_builder,
					partitioned_rotation_group_builder,
					frustum_planes,
					bounding_box_builder);
		}

		partitioned_rotation_groups.push_back(partitioned_rotation_group);

		if (partitioned_rotation_group)
		{
			are_polygons_in_any_rotation_group = true;
		}
	}
	// The size of the arrays should always equal the number of rotation groups.
	// This is because during rendering we need to be able to quickly index into
	// the partitioned rotation groups array to find the rotation group that's being rendered.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			partitioned_rotation_groups.size() == num_rotation_groups &&
			partitioned_rotation_group_builders.size() == num_rotation_groups,
			GPLATES_ASSERTION_SOURCE);

	// If no polygons in any rotation group cover the current
	// quad tree node's cube subdivision then we don't need to create a node.
	// Typically the polygons should cover the entire globe but there could be cases where
	// a user is only interested in a small region and only provides polygons for that region.
	if (!are_polygons_in_any_rotation_group)
	{
		return boost::none;
	}

	// Create a quad tree node.
	QuadTreeNode::non_null_ptr_type quad_tree_node = QuadTreeNode::create(
			source_raster_quad_tree_node.get_tile_handle(),
			projection_transform,
			view_transform);

	quad_tree_node->partitioned_rotation_groups.swap(partitioned_rotation_groups);

	if (d_using_age_grid)
	{
		// If we got here then we should have valid age grid quad tree nodes.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				age_grid_mask_raster_quad_tree_node &&
					age_grid_coverage_raster_quad_tree_node,
				GPLATES_ASSERTION_SOURCE);

		quad_tree_node->age_grid_mask_tile = age_grid_mask_raster_quad_tree_node->get_tile_handle();
		quad_tree_node->age_grid_coverage_tile = age_grid_coverage_raster_quad_tree_node->get_tile_handle();
	}

	// Build child quad tree nodes if necessary.
	for (unsigned int child_v_offset = 0; child_v_offset < 2; ++child_v_offset)
	{
		for (unsigned int child_u_offset = 0; child_u_offset < 2; ++child_u_offset)
		{
			// If the source raster does not having a child node then either the raster doesn't
			// cover that cube subdivision or it has a high enough resolution to, in turn,
			// reproduce its source raster - so we don't need to create a child node either.
			boost::optional<GLMultiResolutionCubeRaster::QuadTreeNode> source_raster_child_quad_tree_node =
					source_raster_quad_tree_node.get_child_node(child_v_offset, child_u_offset);
			if (!source_raster_child_quad_tree_node)
			{
				continue;
			}

			boost::optional<GLMultiResolutionCubeRaster::QuadTreeNode> age_grid_mask_raster_quad_tree_child_node;
			boost::optional<GLMultiResolutionCubeRaster::QuadTreeNode> age_grid_coverage_raster_quad_tree_child_node;
			if (d_using_age_grid)
			{
				// If we got here then we should have valid age grid quad tree nodes.
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						age_grid_mask_raster_quad_tree_node &&
							age_grid_coverage_raster_quad_tree_node,
						GPLATES_ASSERTION_SOURCE);

				// Get the age grid child quad tree nodes.
				age_grid_mask_raster_quad_tree_child_node =
						age_grid_mask_raster_quad_tree_node->get_child_node(child_v_offset, child_u_offset);
				age_grid_coverage_raster_quad_tree_child_node =
						age_grid_coverage_raster_quad_tree_node->get_child_node(child_v_offset, child_u_offset);

				// For now, we only proceed as far down the quad-tree as we have enough resolution
				// in *both* the age grid and the source raster. This limits the resolution to
				// the minimum of the two.
				// FIXME: Change this to proceed down the quad-tree as long as *either* the
				// age grid or source raster has enough resolution.
				if (!age_grid_mask_raster_quad_tree_child_node &&
					!age_grid_coverage_raster_quad_tree_child_node)
				{
					continue;
				}
			}

			quad_tree_node->child_nodes[child_v_offset][child_u_offset] =
					create_quad_tree_node(
							cube_face,
							partitioned_rotation_group_builders,
							source_raster_child_quad_tree_node.get(),
							age_grid_mask_raster_quad_tree_child_node,
							age_grid_coverage_raster_quad_tree_child_node,
							level_of_detail + 1,
							2 * tile_u_offset + child_u_offset,
							2 * tile_v_offset + child_v_offset);
		}
	}

	return quad_tree_node;
}


// The BOOST_FOREACH macro in versions of boost before 1.37 uses the same local
// variable name in each instantiation. Nested BOOST_FOREACH macros therefore
// cause GCC to warn about shadowed declarations.
DISABLE_GCC_WARNING("-Wshadow")

GPlatesOpenGL::GLMultiResolutionReconstructedRaster::PartitionedRotationGroup::maybe_null_ptr_type
GPlatesOpenGL::GLMultiResolutionReconstructedRaster::partition_rotation_group(
		const PartitionedRotationGroupBuilder &parent_partitioned_rotation_group_builder,
		// The partitioned mesh builders for the current partition (if any polygons intersect our partition).
		PartitionedRotationGroupBuilder &partitioned_rotation_group_builder,
		const GLTransformState::FrustumPlanes &frustum_planes,
		const GLIntersect::OrientedBoundingBoxBuilder &initial_bounding_box_builder)
{
	// The number of partitioned meshes in the parent quad tree node.
	const unsigned int num_parent_partitioned_meshes =
			parent_partitioned_rotation_group_builder.partitioned_mesh_builders.size();

	// Avoid excessive copying.
	partitioned_rotation_group_builder.partitioned_mesh_builders.reserve(num_parent_partitioned_meshes);

	// The partitioned meshes for the current partition (if any polygons intersect our partition).
	partitioned_mesh_seq_type partitioned_meshes;
	// Avoid excessive copying.
	partitioned_meshes.reserve(num_parent_partitioned_meshes);

	// Make a copy of the bounding box builder so we can add just the vertices for the
	// current rotation group.
	GLIntersect::OrientedBoundingBoxBuilder bounding_box_builder(initial_bounding_box_builder);

	// Iterate over the polygons and find the mesh triangles that cover the current
	// cube subdivision - in other words, that are not fully outside any frustum plane.
	// It's possible that a triangle does not intersect the frustum and is not fully
	// outside any frustum planes in which case we're including a triangle that does not
	// intersect the frustum - but the the CGAL mesher produces nice shaped triangles
	// (ie, not long skinny ones) and there won't be many of these (they'll only be
	// near where two frustum planes intersect).
	BOOST_FOREACH(
			const PartitionedMeshBuilder &parent_partitioned_mesh_builder,
			parent_partitioned_rotation_group_builder.partitioned_mesh_builders)
	{
		const std::vector<GPlatesMaths::UnitVector3D> &mesh_vertices =
				parent_partitioned_mesh_builder.polygon->mesh_points;

		const std::vector<GLuint> &parent_mesh_triangles =
				parent_partitioned_mesh_builder.vertex_element_array_data;

		// Any mesh triangles for the current quad tree node will go here.
		std::vector<GLuint> mesh_triangles;
		GLuint min_vertex_index = mesh_vertices.size();
		GLuint max_vertex_index = 0;

		const unsigned int num_parent_mesh_triangles = parent_mesh_triangles.size() / 3;
		for (unsigned int tri_index = 0; tri_index < num_parent_mesh_triangles; ++tri_index)
		{
			const unsigned int vertex_element_index = 3 * tri_index;

			const GLuint vertex_index0 = parent_mesh_triangles[vertex_element_index];
			const GLuint vertex_index1 = parent_mesh_triangles[vertex_element_index + 1];
			const GLuint vertex_index2 = parent_mesh_triangles[vertex_element_index + 2];

			const GPlatesMaths::UnitVector3D &tri_vertex0 = mesh_vertices[vertex_index0];
			const GPlatesMaths::UnitVector3D &tri_vertex1 = mesh_vertices[vertex_index1];
			const GPlatesMaths::UnitVector3D &tri_vertex2 = mesh_vertices[vertex_index2];

			// Test the current triangle against the frustum planes.
			bool is_triangle_outside_frustum = false;
			BOOST_FOREACH(const GLIntersect::Plane &plane, frustum_planes.planes)
			{
				// If all vertices of the triangle are outside a single plane then
				// the triangle is outside the frustum.
				if (plane.signed_distance(tri_vertex0) < 0 &&
					plane.signed_distance(tri_vertex1) < 0 &&
					plane.signed_distance(tri_vertex2) < 0)
				{
					is_triangle_outside_frustum = true;
					break;
				}
			}

			if (!is_triangle_outside_frustum)
			{
				// Add triangle to the list of triangles for the current quad tree node.
				mesh_triangles.push_back(vertex_index0);
				mesh_triangles.push_back(vertex_index1);
				mesh_triangles.push_back(vertex_index2);

				// Keep track of the minimum vertex index used by the current mesh.
				if (vertex_index0 < min_vertex_index)
				{
					min_vertex_index = vertex_index0;
				}
				if (vertex_index1 < min_vertex_index)
				{
					min_vertex_index = vertex_index1;
				}
				if (vertex_index2 < min_vertex_index)
				{
					min_vertex_index = vertex_index2;
				}
				// Keep track of the maximum vertex index used by the current mesh.
				if (vertex_index0 > max_vertex_index)
				{
					max_vertex_index = vertex_index0;
				}
				if (vertex_index1 > max_vertex_index)
				{
					max_vertex_index = vertex_index1;
				}
				if (vertex_index2 > max_vertex_index)
				{
					max_vertex_index = vertex_index2;
				}

				// Expand this quad tree node's bounding box to include the current triangle.
				bounding_box_builder.add(tri_vertex0);
				bounding_box_builder.add(tri_vertex1);
				bounding_box_builder.add(tri_vertex2);
			}
		}

		// If the current polygon has triangles that cover the current cube subdivision.
		if (!mesh_triangles.empty())
		{
			GLVertexElementArray::non_null_ptr_type vertex_element_array =
					GLVertexElementArray::create(mesh_triangles);

			// Tell it what to draw when the time comes to draw.
			vertex_element_array->gl_draw_range_elements_EXT(
					GL_TRIANGLES,
					min_vertex_index/*start*/,
					max_vertex_index/*end*/,
					mesh_triangles.size()/*count*/,
					GL_UNSIGNED_INT/*type*/,
					0 /*indices_offset*/);

			const PartitionedMesh partitioned_mesh(
					parent_partitioned_mesh_builder.polygon, vertex_element_array);
			partitioned_meshes.push_back(partitioned_mesh);

			// Add some information that our child nodes can use for partitioning.
			// We're effectively reducing the number of mesh triangles that children
			// have to test against since we know that triangles outside this
			// quad tree node will also be outside all child nodes.
			partitioned_rotation_group_builder.partitioned_mesh_builders.push_back(
					PartitionedMeshBuilder(parent_partitioned_mesh_builder.polygon));

			PartitionedMeshBuilder &partitioned_mesh_builder =
					partitioned_rotation_group_builder.partitioned_mesh_builders.back();

			partitioned_mesh_builder.vertex_element_array_data.swap(mesh_triangles);
		}
	}

	if (partitioned_meshes.empty())
	{
		// There were no meshes partitioned into the current quad tree node so return null.
		return PartitionedRotationGroup::maybe_null_ptr_type();
	}

	// Create a partitioned rotation group and return it.
	PartitionedRotationGroup::maybe_null_ptr_type partitioned_rotation_group =
			PartitionedRotationGroup::create(bounding_box_builder.get_oriented_bounding_box());
	partitioned_rotation_group->partitioned_meshes.swap(partitioned_meshes);

	return partitioned_rotation_group;
}

// See above.
ENABLE_GCC_WARNING("-Wshadow")


void
GPlatesOpenGL::GLMultiResolutionReconstructedRaster::update_input_rasters_valid_tokens()
{
	d_source_raster_valid_token = d_raster_to_reconstruct->get_current_valid_token();

	if (d_age_grid_mask_raster)
	{
		d_age_grid_mask_raster_valid_token =
				d_age_grid_mask_raster.get()->get_current_valid_token();
	}

	if (d_age_grid_coverage_raster)
	{
		d_age_grid_coverage_raster_valid_token =
				d_age_grid_coverage_raster.get()->get_current_valid_token();
	}
}


void
GPlatesOpenGL::GLMultiResolutionReconstructedRaster::generate_polygon_mesh(
		RotationGroup &rotation_group,
		const source_polygon_region_type::non_null_ptr_to_const_type &src_polygon_region)
{
	PROFILE_FUNC();

	typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
	typedef CGAL::Triangulation_vertex_base_2<K> Vb;
	typedef CGAL::Delaunay_mesh_face_base_2<K> Fb;
	typedef CGAL::Triangulation_data_structure_2<Vb, Fb> Tds;
	typedef CGAL::Constrained_Delaunay_triangulation_2<K, Tds> CDT;
	typedef CGAL::Delaunay_mesh_size_criteria_2<CDT> Criteria;

	typedef CGAL::Cartesian<double> KP;
	typedef KP::Point_2 Point;
	typedef CGAL::Polygon_2<KP> Polygon_2;

	// Clip each polygon to the current cube face.
	//
	// Instead, for now, just project onto an arbitrary plane.

	// Iterate through the polygon vertices and calculate the sum of vertex positions.
	GPlatesMaths::Vector3D summed_vertex_position(0,0,0);
	int num_vertices = 0;
	GPlatesMaths::PolygonOnSphere::vertex_const_iterator vertex_iter =
			src_polygon_region->exterior_polygon->vertex_begin();
	GPlatesMaths::PolygonOnSphere::vertex_const_iterator vertex_end =
			src_polygon_region->exterior_polygon->vertex_end();
	for ( ; vertex_iter != vertex_end; ++vertex_iter, ++num_vertices)
	{
		const GPlatesMaths::PointOnSphere &vertex = *vertex_iter;
		const GPlatesMaths::Vector3D point(vertex.position_vector());
		summed_vertex_position = summed_vertex_position + point;
	}

	// If the magnitude of the summed vertex position is zero then all the points averaged
	// to zero and hence we cannot get a plane normal to project onto.
	// This most likely happens when the vertices roughly form a great circle arc and hence
	// then are two possible projection directions and hence you could assign the orientation
	// to be either clockwise or counter-clockwise.
	// If this happens we'll just choose one orientation arbitrarily.
	if (summed_vertex_position.magSqrd() <= 0.0)
	{
		return;
	}

	// Calculate a unit vector from the sum to use as our plane normal.
	const GPlatesMaths::UnitVector3D proj_plane_normal =
			summed_vertex_position.get_normalisation();

	// First try starting with the global x axis - if it's too close to the plane normal
	// then choose the global y axis.
	GPlatesMaths::UnitVector3D proj_plane_x_axis_test_point(0, 0, 1); // global x-axis
	if (dot(proj_plane_x_axis_test_point, proj_plane_normal) > 1 - 1e-2)
	{
		proj_plane_x_axis_test_point = GPlatesMaths::UnitVector3D(0, 1, 0); // global y-axis
	}
	const GPlatesMaths::UnitVector3D proj_plane_axis_x = get_orthonormal_vector(
			proj_plane_x_axis_test_point, proj_plane_normal);

	// Determine the y axis of the plane.
	const GPlatesMaths::UnitVector3D proj_plane_axis_y(
			cross(proj_plane_normal, proj_plane_axis_x));

	Polygon_2 polygon_2;

	--vertex_end;
	// Iterate through the vertices again and project onto the plane.
	for (vertex_iter = src_polygon_region->exterior_polygon->vertex_begin();
		vertex_iter != vertex_end;
		++vertex_iter)
	{
		const GPlatesMaths::PointOnSphere &vertex = *vertex_iter;
		const GPlatesMaths::UnitVector3D &point = vertex.position_vector();

		const GPlatesMaths::real_t proj_point_z = dot(proj_plane_normal, point);
		// For now, if any point isn't localised on the plane then discard polygon.
		if (proj_point_z < 0.15)
		{
			std::cout << "Unable to project polygon - it's too big." << std::endl;
			return;
		}
		const GPlatesMaths::real_t inv_proj_point_z = 1.0 / proj_point_z;

		const GPlatesMaths::real_t proj_point_x = inv_proj_point_z * dot(proj_plane_axis_x, point);
		const GPlatesMaths::real_t proj_point_y = inv_proj_point_z * dot(proj_plane_axis_y, point);

		polygon_2.push_back(Point(proj_point_x.dval(), proj_point_y.dval()));
	}

	// For now, if the polygon is not simple (ie, it's self-intersecting) then discard polygon.
	if (!polygon_2.is_simple())
	{
		std::cout << "Unable to mesh polygon - it's self-intersecting." << std::endl;
		return;
	}

	// Use a set in case CGAL merges any vertices.
	std::map<CDT::Vertex_handle, unsigned int/*vertex index*/> unique_vertex_handles;
	std::vector<CDT::Vertex_handle> vertex_handles;
	CDT cdt;
	for (Polygon_2::Vertex_iterator vert_iter = polygon_2.vertices_begin();
		vert_iter != polygon_2.vertices_end();
		++vert_iter)
	{
		CDT::Vertex_handle vertex_handle = cdt.insert(CDT::Point(vert_iter->x(), vert_iter->y()));
		if (unique_vertex_handles.insert(
				std::map<CDT::Vertex_handle, unsigned int>::value_type(
						vertex_handle, vertex_handles.size())).second)
		{
			vertex_handles.push_back(vertex_handle);
		}
	}

	// For now, if the polygon has less than three vertices then discard it.
	// This can happen if CGAL determines two points are close enough to be merged.
	if (vertex_handles.size() < 3)
	{
		std::cout << "Polygon has less than 3 vertices after triangulation." << std::endl;
		return;
	}

	// Add the boundary constraints.
	for (std::size_t vert_index = 1; vert_index < vertex_handles.size(); ++vert_index)
	{
		cdt.insert_constraint(vertex_handles[vert_index - 1], vertex_handles[vert_index]);
	}
	cdt.insert_constraint(vertex_handles[vertex_handles.size() - 1], vertex_handles[0]);

	// Mesh the domain of the triangulation - the area bounded by constraints.
	PROFILE_BEGIN(cgal_refine_triangulation, "CGAL::refine_Delaunay_mesh_2");
	CGAL::refine_Delaunay_mesh_2(cdt, Criteria(0.125, 0.25));
	PROFILE_END(cgal_refine_triangulation);

	// The vertices of the vertex array for the polygon.
	std::vector<Vertex> vertex_array_data;
	std::vector<GPlatesMaths::UnitVector3D> mesh_points;
	// The triangles indices.
	std::vector<GLuint> vertex_element_array_data;

	// Iterate over the mesh triangles and collect the triangles belonging to the domain.
	typedef std::map<CDT::Vertex_handle, std::size_t/*vertex index*/> mesh_map_type;
	mesh_map_type mesh_vertex_handles;
	for (CDT::Finite_faces_iterator triangle_iter = cdt.finite_faces_begin();
		triangle_iter != cdt.finite_faces_end();
		++triangle_iter)
	{
		if (!triangle_iter->is_in_domain())
		{
			continue;
		}

		for (unsigned int tri_vert_index = 0; tri_vert_index < 3; ++tri_vert_index)
		{
			CDT::Vertex_handle vertex_handle = triangle_iter->vertex(tri_vert_index);

			std::pair<mesh_map_type::iterator, bool> p = mesh_vertex_handles.insert(
					mesh_map_type::value_type(
							vertex_handle, vertex_array_data.size()));

			const std::size_t mesh_vertex_index = p.first->second;
			if (p.second)
			{
				// Unproject the mesh point back onto the sphere.
				const CDT::Point &point2d = vertex_handle->point();
				const GPlatesMaths::UnitVector3D point3d =
					(GPlatesMaths::Vector3D(proj_plane_normal) +
					point2d.x() * proj_plane_axis_x +
					point2d.y() * proj_plane_axis_y).get_normalisation();

				mesh_points.push_back(point3d);

				const Vertex vertex = { point3d.x().dval(), point3d.y().dval(), point3d.z().dval() };
				vertex_array_data.push_back(vertex);
			}
			vertex_element_array_data.push_back(mesh_vertex_index);
		}
	}

	// If the polygon has no time of appearance then assume distant past.
	GPlatesPropertyValues::GeoTimeInstant time_of_appearance = src_polygon_region->time_of_appearance
			? src_polygon_region->time_of_appearance.get()
			: GPlatesPropertyValues::GeoTimeInstant::create_distant_past();

	// If the polygon has no time of disappearance then assume distant future.
	GPlatesPropertyValues::GeoTimeInstant time_of_disappearance = src_polygon_region->time_of_disappearance
			? src_polygon_region->time_of_disappearance.get()
			: GPlatesPropertyValues::GeoTimeInstant::create_distant_future();

	Polygon::non_null_ptr_type polygon = Polygon::create(
			time_of_appearance, time_of_disappearance);

	polygon->mesh_points.swap(mesh_points);

	polygon->vertex_array = GLVertexArray::create(vertex_array_data);
	// We only have (x,y,z) coordinates in our vertex array.
	polygon->vertex_array->gl_enable_client_state(GL_VERTEX_ARRAY);
	polygon->vertex_array->gl_vertex_pointer(3, GL_FLOAT, sizeof(Vertex), 0);

	polygon->vertex_element_array_data.swap(vertex_element_array_data);

	rotation_group.polygons.push_back(polygon);
}

// NOTE: Do not remove this.
// This is here to avoid CGAL related compile errors on MacOS.
// It seems we only need this at the end of the file - perhaps it's something to do with
// template instantiations happening at the end of the translation unit.
DISABLE_GCC_WARNING("-Wshadow")
