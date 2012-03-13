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
#include <QDebug>

#include "GLMultiResolutionStaticPolygonReconstructedRaster.h"

#include "GLContext.h"
#include "GLIntersect.h"
#include "GLMatrix.h"
#include "GLRenderer.h"
#include "GLProjectionUtils.h"
#include "GLShaderProgramUtils.h"
#include "GLUtils.h"
#include "GLVertex.h"
#include "GLViewport.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "gui/Colour.h"

#include "utils/Profile.h"


namespace
{
	//! The inverse of log(2.0).
	const float INVERSE_LOG2 = 1.0 / std::log(2.0);

	//! Vertex shader source code to render raster tiles to the scene.
	const char *RENDER_FLOATING_POINT_TILE_TO_SCENE_VERTEX_SHADER_SOURCE =
			"void main (void)\n"
			"{\n"

			"	// Ensure position is transformed exactly same as fixed-function pipeline due to multi-pass.\n"
			"	gl_Position = ftransform(); //gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"

			"	// Transform present-day position by cube map projection and\n"
			"	// any texture coordinate adjustments before accessing textures.\n"
			"	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_Vertex;\n"
			"	gl_TexCoord[1] = gl_TextureMatrix[1] * gl_Vertex;\n"

			"}\n";

	/**
	 * Fragment shader source code to render raster tiles to the scene.
	 *
	 * NOTE: This fragment shader is for floating-point textures which we set to 'nearest' filtering
	 * (due to earlier hardware lack of support) so we need to emulate bilinear filtering in the fragment shader.
	 */
	const char *RENDER_FLOATING_POINT_TILE_TO_SCENE_FRAGMENT_SHADER_SOURCE =
			"uniform sampler2D tile_texture_sampler;\n"
			"uniform sampler2D clip_texture_sampler;\n"
			"uniform vec4 tile_texture_dimensions;\n"

			"void main (void)\n"
			"{\n"

			"	// Discard the pixel if it has been clipped by the clip texture as an\n"
			"	// early rejection test since a lot of pixels will be outside the tile region.\n"
			"	float clip_mask = texture2DProj(clip_texture_sampler, gl_TexCoord[1]).a;\n"
			"	if (clip_mask == 0)\n"
			"		discard;\n"

			"	// Do the texture transform projective divide \n"
			"	vec2 tile_texture_coords = gl_TexCoord[0].st / gl_TexCoord[0].q;\n"

			"	// Bilinearly filter the tile texture.\n"
			"	vec4 tile_texture_bilinear = bilinearly_interpolate(\n"
			"	     tile_texture_sampler, tile_texture_coords, tile_texture_dimensions);\n"

			"	// We need to reject the pixel if its coverage/alpha is zero - this is because\n"
			"	// it might be an age masked tile - zero coverage, in this case, means don't draw\n"
			"	// because the age test failed - if we draw then we'd overwrite valid data.\n"
			"	// NOTE: For floating-point textures we've placed the coverage/alpha in the green channel.\n"
			"	if (tile_texture_bilinear.g == 0)\n"
			"		discard;\n"

			"	// Projective texturing to handle cube map projection.\n"
			"	gl_FragColor = tile_texture_bilinear;\n"

			"}\n";

	//! Vertex shader source code to render (the third pass of) an age-masked source raster tile.
	const char *RENDER_FLOATING_POINT_AGE_MASKED_SOURCE_RASTER_THIRD_PASS_VERTEX_SHADER_SOURCE =
			"uniform mat4 source_texture_transform;\n"

			"void main (void)\n"
			"{\n"

			"	// Ensure position is transformed exactly same as fixed-function pipeline due to multi-pass.\n"
			"	gl_Position = ftransform(); //gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"

			"	// Source texture has a transform but age-mask texture does not.\n"
			"	gl_TexCoord[0] = source_texture_transform * gl_MultiTexCoord0;\n"
			"	gl_TexCoord[1] = gl_MultiTexCoord0;\n"

			"}\n";

	/**
	 * Fragment shader source code to render (the third pass of) an age-masked source raster tile.
	 *
	 * NOTE: The source tile texture and age-mask tile texture are the same dimensions and we are
	 * doing a direct pixel-to-texel mapping so the 'nearest' filtering of source texture is fine
	 * here (recall that we set filtering on floating-point textures to 'nearest' due to earlier
	 * hardware lack of support and that bilinear filtering needs to be emulated in the fragment shader).
	 * So we don't need to emulate bilinear filtering in this fragment shader.
	 */
	const char *RENDER_FLOATING_POINT_AGE_MASKED_SOURCE_RASTER_THIRD_PASS_FRAGMENT_SHADER_SOURCE =
			"uniform sampler2D source_texture_sampler;\n"
			"uniform sampler2D age_mask_texture_sampler;\n"

			"void main (void)\n"
			"{\n"

			"	// Load the source texture.\n"
			"	// NOTE: Even though source texture coordinates have a texture transform,\n"
			"	//       the transform is not projective so we don't need 'texture2DProj'.\n"
			"	vec4 source_colour = texture2D(source_texture_sampler, gl_TexCoord[0].st);\n"

			"	// Get the age mask.\n"
			"	float age_mask = texture2D(age_mask_texture_sampler, gl_TexCoord[1].st).a;\n"

			"	// Modulate the source texture's coverage with the age-mask texture's coverage.\n"
			"	// NOTE: For floating-point textures we've placed the coverage/alpha in the green channel.\n"
			"	//       But the age-mask is still fixed-point and has its coverage in the alpha channel.\n"
			"	source_colour.g *= age_mask;\n"

			"	// We discard the current pixel if the coverage is zero so that the data value is also\n"
			"	// zero (the clear colour) - alternatively we could have set the data value to zero.\n"
			"	// Doing this ensures bilinear filtering of the age-masked source texture will give\n"
			"	// the correct results - this is also important for the raster co-registration since\n"
			"	// it normalises pixels (divides by coverage) in its 'minimum-value' operation.\n"
			"	if (source_colour.g == 0)\n"
			"		discard;\n"

			"	gl_FragColor = source_colour;\n"

			"}\n";
}


bool
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::is_supported(
		GLRenderer &renderer)
{
	// We currently need two texture units - one for the raster and another for the clip texture.
	if (GLContext::get_parameters().texture.gl_max_texture_units < 2)
	{
		// Only emit warning message once.
		static bool emitted_warning = false;
		if (!emitted_warning)
		{
			qWarning() <<
					"RECONSTRUCTED rasters NOT supported by this OpenGL system - requires two texture units.\n"
					"  Most graphics hardware for over a decade supports this -\n"
					"  most likely software renderer fallback has occurred - possibly via remote desktop software.";
			emitted_warning = true;
		}

		return false;
	}

	// Supported.
	return true;
}


GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::GLMultiResolutionStaticPolygonReconstructedRaster(
		GLRenderer &renderer,
		const GLMultiResolutionCubeRaster::non_null_ptr_type &raster_to_reconstruct,
		const GLReconstructedStaticPolygonMeshes::non_null_ptr_type &reconstructed_static_polygon_meshes,
		boost::optional<AgeGridRaster> age_grid_raster) :
	d_raster_to_reconstruct(raster_to_reconstruct),
	d_reconstructed_static_polygon_meshes(reconstructed_static_polygon_meshes),
	d_tile_texel_dimension(raster_to_reconstruct->get_tile_texel_dimension()),
	d_inverse_tile_texel_dimension(1.0f / raster_to_reconstruct->get_tile_texel_dimension()),
	d_tile_root_uv_transform(
			GLUtils::QuadTreeUVTransform::get_expand_tile_ratio(
					raster_to_reconstruct->get_tile_texel_dimension(),
					0.5/*tile_border_overlap_in_texels*/)),
	// Start with small size cache and just let the cache grow in size as needed if caching enabled...
	d_age_masked_source_tile_texture_cache(
			age_masked_source_tile_texture_cache_type::create(
					2/* GPU pipeline breathing room in case caching disabled*/)),
	d_xy_clip_texture(GLTextureUtils::create_xy_clip_texture_2D(renderer)),
	d_z_clip_texture(GLTextureUtils::create_z_clip_texture_2D(renderer)),
	d_xy_clip_texture_transform(GLTextureUtils::get_clip_texture_clip_space_to_texture_space_transform()),
	d_full_screen_quad_drawable(renderer.get_context().get_shared_state()->get_full_screen_2D_textured_quad(renderer)),
	d_cube_quad_tree(cube_quad_tree_type::create())
{
	// If using an age-grid then create the 'cube' age grid mask and coverage rasters.
	if (age_grid_raster)
	{
		// The age grid cube rasters need to have the same tile dimension as the source raster so that
		// the resolution matches and we don't undersample/oversample the age grid more than necessary...
		d_age_grid_cube_raster =
				AgeGridCubeRaster(renderer, age_grid_raster.get(), raster_to_reconstruct->get_tile_texel_dimension());
	}

	// If the source raster is floating-point (ie, not coloured as fixed-point for visual display)
	// then use a shader program instead of the fixed-function pipeline.
	if (GLTexture::is_format_floating_point(d_raster_to_reconstruct->get_tile_texture_internal_format()))
	{
		create_floating_point_shader_programs(renderer);
	}
}


const GPlatesUtils::SubjectToken &
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::get_subject_token() const
{
	//
	// This covers changes to the inputs that don't require completely re-creating the inputs.
	// That is beyond our scope and is detected and managed by our owners (and owners of our inputs).
	//

	// If the source raster has changed.
	if (!d_raster_to_reconstruct->get_subject_token().is_observer_up_to_date(
				d_raster_to_reconstruct_texture_observer_token))
	{
		d_subject_token.invalidate();

		d_raster_to_reconstruct->get_subject_token().update_observer(
				d_raster_to_reconstruct_texture_observer_token);
	}

	// If the reconstructed polygons have changed (a new reconstruction).
	if (!d_reconstructed_static_polygon_meshes->get_subject_token().is_observer_up_to_date(
				d_reconstructed_static_polygon_meshes_observer_token))
	{
		d_subject_token.invalidate();

		d_reconstructed_static_polygon_meshes->get_subject_token().update_observer(
				d_reconstructed_static_polygon_meshes_observer_token);
	}

	// If the age grid has changed (a new reconstruction time).
	if (d_age_grid_cube_raster)
	{
		if (!d_age_grid_cube_raster->age_grid_mask_cube_raster->get_subject_token().is_observer_up_to_date(
				d_age_grid_cube_raster->d_age_grid_mask_cube_raster_observer_token))
		{
			d_subject_token.invalidate();

			d_age_grid_cube_raster->age_grid_mask_cube_raster->get_subject_token().update_observer(
					d_age_grid_cube_raster->d_age_grid_mask_cube_raster_observer_token);
		}

		if (!d_age_grid_cube_raster->age_grid_coverage_cube_raster->get_subject_token().is_observer_up_to_date(
				d_age_grid_cube_raster->d_age_grid_coverage_cube_raster_observer_token))
		{
			d_subject_token.invalidate();

			d_age_grid_cube_raster->age_grid_coverage_cube_raster->get_subject_token().update_observer(
					d_age_grid_cube_raster->d_age_grid_coverage_cube_raster_observer_token);
		}
	}

	return d_subject_token;
}


float
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::get_level_of_detail(
		const GLMatrix &model_view_transform,
		const GLMatrix &projection_transform,
		const GLViewport &viewport,
		float level_of_detail_bias) const
{
	// Get the minimum size of a pixel in the current viewport when projected
	// onto the unit sphere (in model space).
	const double min_pixel_size_on_unit_sphere = GLProjectionUtils::get_min_pixel_size_on_unit_sphere(
				viewport, model_view_transform, projection_transform);

	//
	// Calculate the cube quad tree depth we need to traverse to satisfy the resolution of the view frustum.
	// This is the equivalent of:
	//
	//    t = t0 * 2 ^ (-depth)
	//
	// ...where 't0' is the texel size of the *highest* resolution level-of-detail and
	// 't' is the projected size of a pixel of the viewport.
	//

	// The maximum texel size of any texel projected onto the unit sphere occurs at the centre
	// of the cube faces. Not all cube subdivisions occur at the face centres but the projected
	// texel size will always be less than at the face centre so at least it's bounded and the
	// variation across the cube face is not that large so we shouldn't be using a level-of-detail
	// that is much higher than what we need.
	const float max_highest_resolution_texel_size_on_unit_sphere = 2.0 / d_tile_texel_dimension;

	const float cube_quad_tree_depth = INVERSE_LOG2 *
			(std::log(max_highest_resolution_texel_size_on_unit_sphere) -
					std::log(min_pixel_size_on_unit_sphere));

	// Need to convert cube quad tree depth to the level-of-detail recognised by the source raster.
	// They are the inverse of each other - the highest resolution source level-of-detail is zero
	// which corresponds to a cube quad tree depth of 'get_num_levels_of_detail() - 1' while
	// a cube quad tree depth of zero corresponds to the lowest (used) source level-of-detail of
	// 'get_num_levels_of_detail() - 1'.
	// Also note that 'level_of_detail' can be negative which means rendering at a resolution that is
	// actually higher than the source raster can supply (useful if age grid mask is even higher resolution).
	const float level_of_detail = (get_num_levels_of_detail() - 1) - cube_quad_tree_depth;

	// 'level_of_detail_bias' is used by clients to allow the largest texel in a drawn texture to be
	// larger than a pixel in the viewport (which can result in blockiness in places in the rendered scene).
	return level_of_detail + level_of_detail_bias;
}


int
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::clamp_level_of_detail(
		float level_of_detail_float) const
{
	// NOTE: A negative LOD is allowed and is useful when reconstructed raster uses an age grid mask
	// that is higher resolution than the source raster itself - this allows us to render the
	// source raster at a higher resolution than it supports but at a resolution still supported by
	// the age grid (so the raster will appear blurry but its age grid masking/modulation will not).
	if (level_of_detail_float < 0)
	{
		// We want to round *down* to the next negative integer but static_cast<int> rounds
		// *towards* zero - so we subtract one before truncating.
		return static_cast<int>(level_of_detail_float - 1);
	}

	// Truncate the level-of-detail.
	int level_of_detail = static_cast<int>(level_of_detail_float);

	const unsigned int num_levels_of_detail = get_num_levels_of_detail();

	// Clamp to lowest resolution level of detail.
	if (level_of_detail >= boost::numeric_cast<int>(num_levels_of_detail))
	{
		// If we get there then even our lowest resolution level of detail had too much resolution
		// for the specified level of detail - but this is pretty unlikely for all but the very
		// smallest of viewports.
		level_of_detail = num_levels_of_detail - 1;
	}

	return level_of_detail;
}


bool
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::render(
		GLRenderer &renderer,
		int level_of_detail,
		cache_handle_type &cache_handle)
{
	PROFILE_FUNC();

	// Convert the source raster level-of-detail to cube quad tree depth.
	// They are the inverse of each other - the highest resolution source level-of-detail is zero
	// which corresponds to a cube quad tree depth of 'get_num_levels_of_detail() - 1' while
	// a cube quad tree depth of zero corresponds to the lowest (used) source level-of-detail of
	// 'get_num_levels_of_detail() - 1'.
	// Also note that 'level_of_detail' can be negative which means rendering at a resolution that is
	// actually higher than the source raster can supply (useful if age grid mask is even higher resolution).
	int render_cube_quad_tree_depth = (get_num_levels_of_detail() - 1) - level_of_detail;
	// The GLMultiResolutionRasterInterface interface says an exception is thrown if level-of-detail
	// is outside the valid range.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			render_cube_quad_tree_depth >= 0,
			GPLATES_ASSERTION_SOURCE);

	// The polygon mesh drawables and polygon mesh cube quad tree node intersections.
	const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables =
			d_reconstructed_static_polygon_meshes->get_present_day_polygon_mesh_drawables();
	const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections =
			d_reconstructed_static_polygon_meshes->get_present_day_polygon_mesh_node_intersections();

	// Get the transform groups of reconstructed polygon meshes that are visible in the view frustum.
	reconstructed_polygon_mesh_transform_groups_type::non_null_ptr_to_const_type
			reconstructed_polygon_mesh_transform_groups =
					d_reconstructed_static_polygon_meshes->get_reconstructed_polygon_meshes(renderer);

	// Used to cache information (only during this 'render' method) that can be reused as we traverse
	// the source raster for each transform in the reconstructed polygon meshes.
	render_traversal_cube_quad_tree_type::non_null_ptr_type render_traversal_cube_quad_tree =
			render_traversal_cube_quad_tree_type::create();

	// Used to cache information to return to the client so that objects in our internal caches
	// don't get recycled prematurely (as we render the various tiles).
	client_cache_cube_quad_tree_type::non_null_ptr_type client_cache_cube_quad_tree =
			client_cache_cube_quad_tree_type::create();

	// Create a subdivision cube quad tree traversal.
	// Caching is required since we're likely visiting each subdivision node more than once.
	// Each transform group visits the source raster cube quad tree and there's likely overlap.
	// The frustum culling is where we access the same node in cube subdivision cache multiple times.
	//
	// Cube subdivision cache for half-texel-expanded projection transforms since that is what's used to
	// lookup the tile textures (the tile textures are bilinearly filtered and the centres of
	// border texels match up with adjacent tiles).
	cube_subdivision_cache_type::non_null_ptr_type
			cube_subdivision_cache =
					cube_subdivision_cache_type::create(
							GPlatesOpenGL::GLCubeSubdivision::create(
									GPlatesOpenGL::GLCubeSubdivision::get_expand_frustum_ratio(
											d_tile_texel_dimension,
											0.5/* half a texel */)),
									1024/*max_num_cached_elements*/);
	// Cube subdivision cache for the clip texture (no frustum expansion here).
	// Also no caching required since clipping only used to set scene-tile-compiled-draw-state
	// and that is cached across the transform groups.
	clip_cube_subdivision_cache_type::non_null_ptr_type
			clip_cube_subdivision_cache =
					clip_cube_subdivision_cache_type::create(
							GPlatesOpenGL::GLCubeSubdivision::create());

	// Keep track of how many tiles were rendered to the scene.
	// Currently this is just used to determine if we rendered anything.
	unsigned int num_tiles_rendered_to_scene = 0;

	// Iterate over the transform groups and traverse the cube quad tree separately for each transform.
	BOOST_FOREACH(
			const reconstructed_polygon_mesh_transform_group_type &reconstructed_polygon_mesh_transform_group,
			reconstructed_polygon_mesh_transform_groups->get_transform_groups())
	{
		// Make sure we leave the OpenGL state the way it was.
		// We do this for each iteration of the loop.
		GLRenderer::StateBlockScope save_restore_state(renderer);

		// Push the current finite rotation transform.
		renderer.gl_mult_matrix(
				GL_MODELVIEW,
				reconstructed_polygon_mesh_transform_group.get_finite_rotation()->get_matrix());

		// First get the view frustum planes.
		//
		// NOTE: We do this *after* multiplying the above rotation transform because the
		// frustum planes are affected by the current model-view and projection transforms.
		// Our quad tree bounding boxes are in model space but the polygon meshes they
		// bound are rotating to new positions so we want to take that into account and map
		// the view frustum back to model space where we can test against our bounding boxes.
		//
		const GLFrustum frustum_planes(
				renderer.gl_get_matrix(GL_MODELVIEW),
				renderer.gl_get_matrix(GL_PROJECTION));

		//
		// Traverse the source raster cube quad tree and the spatial partition of reconstructed polygon meshes.
		//

		// Traverse the quad trees of the cube faces.
		for (unsigned int face = 0; face < 6; ++face)
		{
			const GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face =
					static_cast<GPlatesMaths::CubeCoordinateFrame::CubeFaceType>(face);

			// Get the quad tree root node of the current cube face of the source raster.
			boost::optional<raster_quad_tree_node_type> source_raster_quad_tree_root =
					d_raster_to_reconstruct->get_quad_tree_root_node(cube_face);
			// If there is no source raster for the current cube face then continue to the next face.
			if (!source_raster_quad_tree_root)
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
						QuadTreeNode(d_age_masked_source_tile_texture_cache->allocate_volatile_object()));
			}

			// Get, or create, the root render traversal node.
			render_traversal_cube_quad_tree_type::node_type &render_traversal_cube_quad_tree_root =
					render_traversal_cube_quad_tree->get_or_create_quad_tree_root_node(cube_face);

			// Get, or create, the root client cache node.
			client_cache_cube_quad_tree_type::node_type &client_cache_cube_quad_tree_root =
					client_cache_cube_quad_tree->get_or_create_quad_tree_root_node(cube_face);

			// Get the root cube division cache node.
			const cube_subdivision_cache_type::node_reference_type
					cube_subdivision_cache_root_node =
							cube_subdivision_cache->get_quad_tree_root_node(cube_face);

			// Get the root clip cube subdivision cache node.
			const clip_cube_subdivision_cache_type::node_reference_type
					clip_cube_subdivision_cache_root_node =
							clip_cube_subdivision_cache->get_quad_tree_root_node(cube_face);

			// If we're using an age grid to smooth the reconstruction then perform a slightly
			// different quad tree traversal code path.
			if (d_age_grid_cube_raster)
			{
				// Get the quad tree root node of the current cube face of the age grid mask/coverage rasters.
				boost::optional<age_grid_mask_quad_tree_node_type> age_grid_mask_quad_tree_root =
						d_age_grid_cube_raster->age_grid_mask_cube_raster->get_quad_tree_root_node(cube_face);
				boost::optional<age_grid_coverage_quad_tree_node_type> age_grid_coverage_quad_tree_root =
						d_age_grid_cube_raster->age_grid_coverage_cube_raster->get_quad_tree_root_node(cube_face);

				// If there is an age grid mask/coverage raster for the current cube face then...
				if (age_grid_mask_quad_tree_root && age_grid_coverage_quad_tree_root)
				{
					render_quad_tree_source_raster_and_age_grid(
							renderer,
							*cube_quad_tree_root,
							*render_traversal_cube_quad_tree,
							render_traversal_cube_quad_tree_root,
							*client_cache_cube_quad_tree,
							client_cache_cube_quad_tree_root,
							*source_raster_quad_tree_root,
							*age_grid_mask_quad_tree_root,
							*age_grid_coverage_quad_tree_root,
							*reconstructed_polygon_mesh_transform_groups,
							reconstructed_polygon_mesh_transform_group,
							polygon_mesh_drawables,
							polygon_mesh_node_intersections,
							*intersections_quad_tree_root_node,
							*cube_subdivision_cache,
							cube_subdivision_cache_root_node,
							*clip_cube_subdivision_cache,
							clip_cube_subdivision_cache_root_node,
							0/*cube_quad_tree_depth*/,
							render_cube_quad_tree_depth,
							frustum_planes,
							// There are six frustum planes initially active
							GLFrustum::ALL_PLANES_ACTIVE_MASK,
							num_tiles_rendered_to_scene);

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
					*client_cache_cube_quad_tree,
					client_cache_cube_quad_tree_root,
					*source_raster_quad_tree_root,
					reconstructed_polygon_mesh_transform_group,
					polygon_mesh_drawables,
					polygon_mesh_node_intersections,
					*intersections_quad_tree_root_node,
					*cube_subdivision_cache,
					cube_subdivision_cache_root_node,
					*clip_cube_subdivision_cache,
					clip_cube_subdivision_cache_root_node,
					0/*cube_quad_tree_depth*/,
					render_cube_quad_tree_depth,
					frustum_planes,
					// There are six frustum planes initially active
					GLFrustum::ALL_PLANES_ACTIVE_MASK,
					num_tiles_rendered_to_scene);
		}
	}

	// Return the client cache cube quad tree to the caller.
	// The caller will cache this tile to keep objects in it from being prematurely recycled by caches.
	// Note that we also need to create a boost::shared_ptr from an intrusive pointer.
	cache_handle = cache_handle_type(make_shared_from_intrusive(client_cache_cube_quad_tree));

	//qDebug() << "GLMultiResolutionStaticPolygonReconstructedRaster: Num rendered tiles: " << num_tiles_rendered_to_scene;

	return num_tiles_rendered_to_scene > 0;
}


void
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::render_quad_tree_source_raster(
		GLRenderer &renderer,
		render_traversal_cube_quad_tree_type &render_traversal_cube_quad_tree,
		render_traversal_cube_quad_tree_type::node_type &render_traversal_cube_quad_tree_node,
		client_cache_cube_quad_tree_type &client_cache_cube_quad_tree,
		client_cache_cube_quad_tree_type::node_type &client_cache_cube_quad_tree_node,
		const raster_quad_tree_node_type &source_raster_quad_tree_node,
		const reconstructed_polygon_mesh_transform_group_type &reconstructed_polygon_mesh_transform_group,
		const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables,
		const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
		const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
		cube_subdivision_cache_type &cube_subdivision_cache,
		const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node,
		clip_cube_subdivision_cache_type &clip_cube_subdivision_cache,
		const clip_cube_subdivision_cache_type::node_reference_type &clip_cube_subdivision_cache_node,
		unsigned int cube_quad_tree_depth,
		unsigned int render_cube_quad_tree_depth,
		const GLFrustum &frustum_planes,
		boost::uint32_t frustum_plane_mask,
		unsigned int &num_tiles_rendered_to_scene)
{
	// If the current quad tree node does not intersect any polygon meshes in the current rotation
	// group or the current quad tree node is outside the view frustum then we can return early.
	if (cull_quad_tree(
			// We're rendering using only *active* reconstructed polygons.
			reconstructed_polygon_mesh_transform_group.get_visible_present_day_polygon_meshes_for_active_reconstructions(),
			polygon_mesh_node_intersections,
			intersections_quad_tree_node,
			cube_subdivision_cache,
			cube_subdivision_cache_node,
			frustum_planes,
			frustum_plane_mask))
	{
		return;
	}

	// If either:
	// - we're at the correct level of detail for rendering, or
	// - we've reached a leaf node of the source raster (highest resolution supplied by the source raster),
	// ...then render the current source raster tile.
	if (cube_quad_tree_depth == render_cube_quad_tree_depth ||
		source_raster_quad_tree_node.is_leaf_node())
	{
		render_source_raster_tile_to_scene(
				renderer,
				render_traversal_cube_quad_tree_node,
				client_cache_cube_quad_tree_node,
				source_raster_quad_tree_node,
				// Note that there's no uv scaling of the source raster because we're at the
				// source raster leaf node (and are not traversing deeper)...
				d_tile_root_uv_transform/*source_raster_uv_transform*/,
				reconstructed_polygon_mesh_transform_group,
				polygon_mesh_node_intersections,
				intersections_quad_tree_node,
				polygon_mesh_drawables,
				cube_subdivision_cache,
				cube_subdivision_cache_node,
				clip_cube_subdivision_cache,
				clip_cube_subdivision_cache_node,
				num_tiles_rendered_to_scene);

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
			boost::optional<raster_quad_tree_node_type> child_source_raster_quad_tree_node =
					d_raster_to_reconstruct->get_child_node(
							source_raster_quad_tree_node, child_u_offset, child_v_offset);

			// If there is no source raster for the current child then continue to the next child.
			// This happens if the current child is not covered by the source raster.
			// Note that if we get here then the current parent is not a leaf node.
			if (!child_source_raster_quad_tree_node)
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

			// Get, or create, the child client cache node.
			client_cache_cube_quad_tree_type::node_type &child_render_client_cache_quad_tree_node =
					client_cache_cube_quad_tree.get_or_create_child_node(
							client_cache_cube_quad_tree_node, child_u_offset, child_v_offset);

			// Get the child cube subdivision cache node.
			const cube_subdivision_cache_type::node_reference_type
					child_cube_subdivision_cache_node =
							cube_subdivision_cache.get_child_node(
									cube_subdivision_cache_node,
									child_u_offset,
									child_v_offset);

			// Get the child clip cube subdivision cache node.
			const clip_cube_subdivision_cache_type::node_reference_type
					child_clip_cube_subdivision_cache_node =
							clip_cube_subdivision_cache.get_child_node(
									clip_cube_subdivision_cache_node,
									child_u_offset,
									child_v_offset);

			render_quad_tree_source_raster(
					renderer,
					render_traversal_cube_quad_tree,
					child_render_traversal_cube_quad_tree_node,
					client_cache_cube_quad_tree,
					child_render_client_cache_quad_tree_node,
					*child_source_raster_quad_tree_node,
					reconstructed_polygon_mesh_transform_group,
					polygon_mesh_drawables,
					polygon_mesh_node_intersections,
					*child_intersections_quad_tree_node,
					cube_subdivision_cache,
					child_cube_subdivision_cache_node,
					clip_cube_subdivision_cache,
					child_clip_cube_subdivision_cache_node,
					cube_quad_tree_depth + 1,
					render_cube_quad_tree_depth,
					frustum_planes,
					frustum_plane_mask,
					num_tiles_rendered_to_scene);
		}
	}
}


void
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::render_quad_tree_scaled_source_raster_and_unscaled_age_grid(
		GLRenderer &renderer,
		cube_quad_tree_type::node_type &cube_quad_tree_node,
		render_traversal_cube_quad_tree_type &render_traversal_cube_quad_tree,
		render_traversal_cube_quad_tree_type::node_type &render_traversal_cube_quad_tree_node,
		client_cache_cube_quad_tree_type &client_cache_cube_quad_tree,
		client_cache_cube_quad_tree_type::node_type &client_cache_cube_quad_tree_node,
		const raster_quad_tree_node_type &source_raster_quad_tree_node,
		const GLUtils::QuadTreeUVTransform &source_raster_uv_transform,
		const age_grid_mask_quad_tree_node_type &age_grid_mask_quad_tree_node,
		const age_grid_coverage_quad_tree_node_type &age_grid_coverage_quad_tree_node,
		const reconstructed_polygon_mesh_transform_groups_type &reconstructed_polygon_mesh_transform_groups,
		const reconstructed_polygon_mesh_transform_group_type &reconstructed_polygon_mesh_transform_group,
		const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables,
		const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
		const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
		cube_subdivision_cache_type &cube_subdivision_cache,
		const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node,
		clip_cube_subdivision_cache_type &clip_cube_subdivision_cache,
		const clip_cube_subdivision_cache_type::node_reference_type &clip_cube_subdivision_cache_node,
		unsigned int cube_quad_tree_depth,
		unsigned int render_cube_quad_tree_depth,
		const GLFrustum &frustum_planes,
		boost::uint32_t frustum_plane_mask,
		unsigned int &num_tiles_rendered_to_scene)
{
	// If the current quad tree node does not intersect any polygon meshes in the current rotation
	// group or the current quad tree node is outside the view frustum then we can return early.
	if (cull_quad_tree(
			// We're rendering using active *and* inactive reconstructed polygons since the
			// age grid now decides the begin times of oceanic crust.
			reconstructed_polygon_mesh_transform_group.get_visible_present_day_polygon_meshes_for_active_or_inactive_reconstructions(),
			polygon_mesh_node_intersections,
			intersections_quad_tree_node,
			cube_subdivision_cache,
			cube_subdivision_cache_node,
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
			cube_quad_tree_depth == render_cube_quad_tree_depth ||
			(
				age_grid_mask_quad_tree_node.is_leaf_node() ||
				age_grid_coverage_quad_tree_node.is_leaf_node()
			)
		)
	{
		render_source_raster_and_age_grid_tile_to_scene(
				renderer,
				cube_quad_tree_node,
				render_traversal_cube_quad_tree_node,
				client_cache_cube_quad_tree_node,
				source_raster_quad_tree_node,
				source_raster_uv_transform,
				age_grid_mask_quad_tree_node,
				age_grid_coverage_quad_tree_node,
				// Note that there's no uv scaling of the age grid because we're at the
				// age grid leaf node (and are not traversing deeper)...
				d_tile_root_uv_transform/*age_grid_uv_transform*/,
				reconstructed_polygon_mesh_transform_groups,
				reconstructed_polygon_mesh_transform_group,
				polygon_mesh_node_intersections,
				intersections_quad_tree_node,
				polygon_mesh_drawables,
				cube_subdivision_cache,
				cube_subdivision_cache_node,
				clip_cube_subdivision_cache,
				clip_cube_subdivision_cache_node,
				num_tiles_rendered_to_scene);

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
						QuadTreeNode(d_age_masked_source_tile_texture_cache->allocate_volatile_object()));
			}

			// Get, or create, the child render traversal node.
			render_traversal_cube_quad_tree_type::node_type &child_render_traversal_cube_quad_tree_node =
					render_traversal_cube_quad_tree.get_or_create_child_node(
							render_traversal_cube_quad_tree_node, child_u_offset, child_v_offset);

			// Get, or create, the child client cache node.
			client_cache_cube_quad_tree_type::node_type &child_client_cache_cube_quad_tree_node =
					client_cache_cube_quad_tree.get_or_create_child_node(
							client_cache_cube_quad_tree_node, child_u_offset, child_v_offset);

			// Get the child cube subdivision cache node.
			const cube_subdivision_cache_type::node_reference_type
					child_cube_subdivision_cache_node =
							cube_subdivision_cache.get_child_node(
									cube_subdivision_cache_node,
									child_u_offset,
									child_v_offset);

			// Get the child clip cube subdivision cache node.
			const clip_cube_subdivision_cache_type::node_reference_type
					child_clip_cube_subdivision_cache_node =
							clip_cube_subdivision_cache.get_child_node(
									clip_cube_subdivision_cache_node,
									child_u_offset,
									child_v_offset);

			// UV scaling for the child source raster node.
			const GLUtils::QuadTreeUVTransform child_source_raster_uv_transform(
					source_raster_uv_transform, child_u_offset, child_v_offset);

			// Get the child node of the current age grid mask/coverage quad tree nodes.
			boost::optional<age_grid_mask_quad_tree_node_type> child_age_grid_mask_quad_tree_node =
					d_age_grid_cube_raster->age_grid_mask_cube_raster->get_child_node(
							age_grid_mask_quad_tree_node, child_u_offset, child_v_offset);
			boost::optional<age_grid_coverage_quad_tree_node_type> child_age_grid_coverage_quad_tree_node =
					d_age_grid_cube_raster->age_grid_coverage_cube_raster->get_child_node(
							age_grid_coverage_quad_tree_node, child_u_offset, child_v_offset);

			// If there is no age grid for the current child then render source raster without an age grid.
			// This happens if the current child is not covered by the age grid.
			// Note that if we get here then the current parent age grid node is not a leaf node.
			if (!child_age_grid_mask_quad_tree_node ||
				!child_age_grid_coverage_quad_tree_node)
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
						cube_subdivision_cache,
						child_cube_subdivision_cache_node,
						frustum_planes,
						frustum_plane_mask))
				{
					render_source_raster_tile_to_scene(
							renderer,
							child_render_traversal_cube_quad_tree_node,
							child_client_cache_cube_quad_tree_node,
							source_raster_quad_tree_node,
							child_source_raster_uv_transform,
							reconstructed_polygon_mesh_transform_group,
							polygon_mesh_node_intersections,
							*child_intersections_quad_tree_node,
							polygon_mesh_drawables,
							cube_subdivision_cache,
							child_cube_subdivision_cache_node,
							clip_cube_subdivision_cache,
							child_clip_cube_subdivision_cache_node,
							num_tiles_rendered_to_scene);
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
					client_cache_cube_quad_tree,
					client_cache_cube_quad_tree_node,
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
					cube_subdivision_cache,
					child_cube_subdivision_cache_node,
					clip_cube_subdivision_cache,
					child_clip_cube_subdivision_cache_node,
					cube_quad_tree_depth + 1,
					render_cube_quad_tree_depth,
					frustum_planes,
					frustum_plane_mask,
					num_tiles_rendered_to_scene);
		}
	}
}


void
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::render_quad_tree_unscaled_source_raster_and_scaled_age_grid(
		GLRenderer &renderer,
		cube_quad_tree_type::node_type &cube_quad_tree_node,
		render_traversal_cube_quad_tree_type &render_traversal_cube_quad_tree,
		render_traversal_cube_quad_tree_type::node_type &render_traversal_cube_quad_tree_node,
		client_cache_cube_quad_tree_type &client_cache_cube_quad_tree,
		client_cache_cube_quad_tree_type::node_type &client_cache_cube_quad_tree_node,
		const raster_quad_tree_node_type &source_raster_quad_tree_node,
		const age_grid_mask_quad_tree_node_type &age_grid_mask_quad_tree_node,
		const age_grid_coverage_quad_tree_node_type &age_grid_coverage_quad_tree_node,
		const GLUtils::QuadTreeUVTransform &age_grid_uv_transform,
		const reconstructed_polygon_mesh_transform_groups_type &reconstructed_polygon_mesh_transform_groups,
		const reconstructed_polygon_mesh_transform_group_type &reconstructed_polygon_mesh_transform_group,
		const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables,
		const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
		const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
		cube_subdivision_cache_type &cube_subdivision_cache,
		const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node,
		clip_cube_subdivision_cache_type &clip_cube_subdivision_cache,
		const clip_cube_subdivision_cache_type::node_reference_type &clip_cube_subdivision_cache_node,
		unsigned int cube_quad_tree_depth,
		unsigned int render_cube_quad_tree_depth,
		const GLFrustum &frustum_planes,
		boost::uint32_t frustum_plane_mask,
		unsigned int &num_tiles_rendered_to_scene)
{
	// If the current quad tree node does not intersect any polygon meshes in the current rotation
	// group or the current quad tree node is outside the view frustum then we can return early.
	if (cull_quad_tree(
			// We're rendering using active *and* inactive reconstructed polygons since the
			// age grid now decides the begin times of oceanic crust.
			reconstructed_polygon_mesh_transform_group.get_visible_present_day_polygon_meshes_for_active_or_inactive_reconstructions(),
			polygon_mesh_node_intersections,
			intersections_quad_tree_node,
			cube_subdivision_cache,
			cube_subdivision_cache_node,
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
	if (cube_quad_tree_depth == render_cube_quad_tree_depth ||
		source_raster_quad_tree_node.is_leaf_node())
	{
		render_source_raster_and_age_grid_tile_to_scene(
				renderer,
				cube_quad_tree_node,
				render_traversal_cube_quad_tree_node,
				client_cache_cube_quad_tree_node,
				source_raster_quad_tree_node,
				// Note that there's no uv scaling of the source raster...
				d_tile_root_uv_transform/*source_raster_uv_transform*/,
				age_grid_mask_quad_tree_node,
				age_grid_coverage_quad_tree_node,
				age_grid_uv_transform,
				reconstructed_polygon_mesh_transform_groups,
				reconstructed_polygon_mesh_transform_group,
				polygon_mesh_node_intersections,
				intersections_quad_tree_node,
				polygon_mesh_drawables,
				cube_subdivision_cache,
				cube_subdivision_cache_node,
				clip_cube_subdivision_cache,
				clip_cube_subdivision_cache_node,
				num_tiles_rendered_to_scene);

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
			boost::optional<raster_quad_tree_node_type> child_source_raster_quad_tree_node =
					d_raster_to_reconstruct->get_child_node(
							source_raster_quad_tree_node, child_u_offset, child_v_offset);

			// If there is no source raster for the current child then continue to the next child.
			// This happens if the current child is not covered by the source raster.
			// Note that if we get here then the current parent is not a leaf node.
			if (!child_source_raster_quad_tree_node)
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
						QuadTreeNode(d_age_masked_source_tile_texture_cache->allocate_volatile_object()));
			}

			// Get, or create, the child render traversal node.
			render_traversal_cube_quad_tree_type::node_type &child_render_traversal_cube_quad_tree_node =
					render_traversal_cube_quad_tree.get_or_create_child_node(
							render_traversal_cube_quad_tree_node, child_u_offset, child_v_offset);

			// Get, or create, the child client cache node.
			client_cache_cube_quad_tree_type::node_type &child_client_cache_cube_quad_tree_node =
					client_cache_cube_quad_tree.get_or_create_child_node(
							client_cache_cube_quad_tree_node, child_u_offset, child_v_offset);

			// Get the child cube subdivision cache node.
			const cube_subdivision_cache_type::node_reference_type
					child_cube_subdivision_cache_node =
							cube_subdivision_cache.get_child_node(
									cube_subdivision_cache_node,
									child_u_offset,
									child_v_offset);

			// Get the child clip cube subdivision cache node.
			const clip_cube_subdivision_cache_type::node_reference_type
					child_clip_cube_subdivision_cache_node =
							clip_cube_subdivision_cache.get_child_node(
									clip_cube_subdivision_cache_node,
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
					client_cache_cube_quad_tree,
					child_client_cache_cube_quad_tree_node,
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
					cube_subdivision_cache,
					child_cube_subdivision_cache_node,
					clip_cube_subdivision_cache,
					child_clip_cube_subdivision_cache_node,
					cube_quad_tree_depth + 1,
					render_cube_quad_tree_depth,
					frustum_planes,
					frustum_plane_mask,
					num_tiles_rendered_to_scene);
		}
	}
}


void
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::render_quad_tree_source_raster_and_age_grid(
		GLRenderer &renderer,
		cube_quad_tree_type::node_type &cube_quad_tree_node,
		render_traversal_cube_quad_tree_type &render_traversal_cube_quad_tree,
		render_traversal_cube_quad_tree_type::node_type &render_traversal_cube_quad_tree_node,
		client_cache_cube_quad_tree_type &client_cache_cube_quad_tree,
		client_cache_cube_quad_tree_type::node_type &client_cache_cube_quad_tree_node,
		const raster_quad_tree_node_type &source_raster_quad_tree_node,
		const age_grid_mask_quad_tree_node_type &age_grid_mask_quad_tree_node,
		const age_grid_coverage_quad_tree_node_type &age_grid_coverage_quad_tree_node,
		const reconstructed_polygon_mesh_transform_groups_type &reconstructed_polygon_mesh_transform_groups,
		const reconstructed_polygon_mesh_transform_group_type &reconstructed_polygon_mesh_transform_group,
		const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables,
		const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
		const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
		cube_subdivision_cache_type &cube_subdivision_cache,
		const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node,
		clip_cube_subdivision_cache_type &clip_cube_subdivision_cache,
		const clip_cube_subdivision_cache_type::node_reference_type &clip_cube_subdivision_cache_node,
		unsigned int cube_quad_tree_depth,
		unsigned int render_cube_quad_tree_depth,
		const GLFrustum &frustum_planes,
		boost::uint32_t frustum_plane_mask,
		unsigned int &num_tiles_rendered_to_scene)
{
	// If the current quad tree node does not intersect any polygon meshes in the current rotation
	// group or the current quad tree node is outside the view frustum then we can return early.
	if (cull_quad_tree(
			// We're rendering using active *and* inactive reconstructed polygons since the
			// age grid now decides the begin times of oceanic crust.
			reconstructed_polygon_mesh_transform_group.get_visible_present_day_polygon_meshes_for_active_or_inactive_reconstructions(),
			polygon_mesh_node_intersections,
			intersections_quad_tree_node,
			cube_subdivision_cache,
			cube_subdivision_cache_node,
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
			cube_quad_tree_depth == render_cube_quad_tree_depth ||
			(
				source_raster_quad_tree_node.is_leaf_node() &&
				(
					age_grid_mask_quad_tree_node.is_leaf_node() ||
					age_grid_coverage_quad_tree_node.is_leaf_node()
				)
			)
		)
	{
		render_source_raster_and_age_grid_tile_to_scene(
				renderer,
				cube_quad_tree_node,
				render_traversal_cube_quad_tree_node,
				client_cache_cube_quad_tree_node,
				source_raster_quad_tree_node,
				// Note that there's no uv scaling of the source raster...
				d_tile_root_uv_transform/*source_raster_uv_transform*/,
				age_grid_mask_quad_tree_node,
				age_grid_coverage_quad_tree_node,
				// Note that there's no uv scaling of the age grid...
				d_tile_root_uv_transform/*age_grid_uv_transform*/,
				reconstructed_polygon_mesh_transform_groups,
				reconstructed_polygon_mesh_transform_group,
				polygon_mesh_node_intersections,
				intersections_quad_tree_node,
				polygon_mesh_drawables,
				cube_subdivision_cache,
				cube_subdivision_cache_node,
				clip_cube_subdivision_cache,
				clip_cube_subdivision_cache_node,
				num_tiles_rendered_to_scene);

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
						QuadTreeNode(d_age_masked_source_tile_texture_cache->allocate_volatile_object()));
			}

			// Get, or create, the child render traversal node.
			render_traversal_cube_quad_tree_type::node_type &child_render_traversal_cube_quad_tree_node =
					render_traversal_cube_quad_tree.get_or_create_child_node(
							render_traversal_cube_quad_tree_node, child_u_offset, child_v_offset);

			// Get, or create, the child client cache node.
			client_cache_cube_quad_tree_type::node_type &child_client_cache_cube_quad_tree_node =
					client_cache_cube_quad_tree.get_or_create_child_node(
							client_cache_cube_quad_tree_node, child_u_offset, child_v_offset);

			// Get the child cube subdivision cache node.
			const cube_subdivision_cache_type::node_reference_type
					child_cube_subdivision_cache_node =
							cube_subdivision_cache.get_child_node(
									cube_subdivision_cache_node,
									child_u_offset,
									child_v_offset);

			// Get the child clip cube subdivision cache node.
			const clip_cube_subdivision_cache_type::node_reference_type
					child_clip_cube_subdivision_cache_node =
							clip_cube_subdivision_cache.get_child_node(
									clip_cube_subdivision_cache_node,
									child_u_offset,
									child_v_offset);

			// Get the child node of the current age grid mask/coverage quad tree nodes.
			// NOTE: These could be NULL.
			boost::optional<age_grid_mask_quad_tree_node_type> child_age_grid_mask_quad_tree_node =
					d_age_grid_cube_raster->age_grid_mask_cube_raster->get_child_node(
							age_grid_mask_quad_tree_node, child_u_offset, child_v_offset);
			boost::optional<age_grid_coverage_quad_tree_node_type> child_age_grid_coverage_quad_tree_node =
					d_age_grid_cube_raster->age_grid_coverage_cube_raster->get_child_node(
							age_grid_coverage_quad_tree_node, child_u_offset, child_v_offset);

			if (source_raster_quad_tree_node.is_leaf_node())
			{
				// If the source raster parent node is a leaf node then it has no children.
				// So we need to start scaling the source raster tile texture as our children
				// tiles get smaller and smaller the deeper we go.
				GLUtils::QuadTreeUVTransform child_source_raster_uv_transform(
						d_tile_root_uv_transform, child_u_offset, child_v_offset);

				// NOTE: The age grid has not yet reached a leaf node otherwise we wouldn't be here.
				// The age grid can however have one or more NULL children if the age grid does
				// not cover some child nodes.

				// If there is no age grid for the current child then render source raster without an age grid.
				// This happens if the current child is not covered by the age grid.
				// Note that if we get here then the current parent age grid node is *not* a leaf node
				// (because the parent source raster node is a leaf node and only one of source raster
				// or age grid can be a leaf node - otherwise we wouldn't be here).
				if (!child_age_grid_mask_quad_tree_node ||
					!child_age_grid_coverage_quad_tree_node)
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
							cube_subdivision_cache,
							child_cube_subdivision_cache_node,
							frustum_planes,
							frustum_plane_mask))
					{
						render_source_raster_tile_to_scene(
								renderer,
								child_render_traversal_cube_quad_tree_node,
								child_client_cache_cube_quad_tree_node,
								source_raster_quad_tree_node,
								child_source_raster_uv_transform,
								reconstructed_polygon_mesh_transform_group,
								polygon_mesh_node_intersections,
								*child_intersections_quad_tree_node,
								polygon_mesh_drawables,
								cube_subdivision_cache,
								child_cube_subdivision_cache_node,
								clip_cube_subdivision_cache,
								child_clip_cube_subdivision_cache_node,
								num_tiles_rendered_to_scene);
					}

					continue;
				}

				// Start uv scaling the source raster.
				render_quad_tree_scaled_source_raster_and_unscaled_age_grid(
						renderer,
						*child_cube_quad_tree_node,
						render_traversal_cube_quad_tree,
						child_render_traversal_cube_quad_tree_node,
						client_cache_cube_quad_tree,
						child_client_cache_cube_quad_tree_node,
						source_raster_quad_tree_node,
						child_source_raster_uv_transform,
						*child_age_grid_mask_quad_tree_node,
						*child_age_grid_coverage_quad_tree_node,
						reconstructed_polygon_mesh_transform_groups,
						reconstructed_polygon_mesh_transform_group,
						polygon_mesh_drawables,
						polygon_mesh_node_intersections,
						*child_intersections_quad_tree_node,
						cube_subdivision_cache,
						child_cube_subdivision_cache_node,
						clip_cube_subdivision_cache,
						child_clip_cube_subdivision_cache_node,
						cube_quad_tree_depth + 1,
						render_cube_quad_tree_depth,
						frustum_planes,
						frustum_plane_mask,
						num_tiles_rendered_to_scene);

				continue;
			}

			// Get the child node of the current source raster quad tree node.
			boost::optional<raster_quad_tree_node_type> child_source_raster_quad_tree_node =
					d_raster_to_reconstruct->get_child_node(
							source_raster_quad_tree_node, child_u_offset, child_v_offset);

			// If there is no source raster for the current child then continue to the next child.
			// This happens if the current child is not covered by the source raster.
			// Note that if we get here then the current parent is not a leaf node.
			if (!child_source_raster_quad_tree_node)
			{
				continue;
			}

			// Note that both mask and coverage nodes will be leaf nodes at the same time but
			// we'll test both just to make sure.
			if (age_grid_mask_quad_tree_node.is_leaf_node() ||
				age_grid_coverage_quad_tree_node.is_leaf_node())
			{
				// If the age grid parent node is a leaf node then it has no children.
				// So we need to start scaling the age grid tile textures as our children
				// tiles get smaller and smaller the deeper we go.
				GLUtils::QuadTreeUVTransform child_age_grid_uv_transform(
						d_tile_root_uv_transform, child_u_offset, child_v_offset);

				// NOTE: The source raster has not yet reached a leaf node otherwise we wouldn't be here.

				// Start uv scaling the age grid.
				render_quad_tree_unscaled_source_raster_and_scaled_age_grid(
						renderer,
						*child_cube_quad_tree_node,
						render_traversal_cube_quad_tree,
						child_render_traversal_cube_quad_tree_node,
						client_cache_cube_quad_tree,
						child_client_cache_cube_quad_tree_node,
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
						cube_subdivision_cache,
						child_cube_subdivision_cache_node,
						clip_cube_subdivision_cache,
						child_clip_cube_subdivision_cache_node,
						cube_quad_tree_depth + 1,
						render_cube_quad_tree_depth,
						frustum_planes,
						frustum_plane_mask,
						num_tiles_rendered_to_scene);

				continue;
			}

			// If there is no age grid for the current child then render source raster without an age grid.
			// This happens if the current child is not covered by the age grid because we've
			// already verified that the parent age grid node is *not* a leaf node.
			if (!child_age_grid_mask_quad_tree_node ||
				!child_age_grid_coverage_quad_tree_node)
			{
				// Note that we're *are* continuing to traverse the quad tree because the source raster
				// has *not* reached its highest resolution yet.
				render_quad_tree_source_raster(
						renderer,
						render_traversal_cube_quad_tree,
						child_render_traversal_cube_quad_tree_node,
						client_cache_cube_quad_tree,
						child_client_cache_cube_quad_tree_node,
						*child_source_raster_quad_tree_node,
						reconstructed_polygon_mesh_transform_group,
						polygon_mesh_drawables,
						polygon_mesh_node_intersections,
						*child_intersections_quad_tree_node,
						cube_subdivision_cache,
						child_cube_subdivision_cache_node,
						clip_cube_subdivision_cache,
						child_clip_cube_subdivision_cache_node,
						cube_quad_tree_depth,
						render_cube_quad_tree_depth,
						frustum_planes,
						frustum_plane_mask,
						num_tiles_rendered_to_scene);

				continue;
			}

			// Both the source raster and age grid rasters have not reached their maximum resolution
			// yet so keep traversing both cube quad trees without uv texture scaling.
			render_quad_tree_source_raster_and_age_grid(
					renderer,
					*child_cube_quad_tree_node,
					render_traversal_cube_quad_tree,
					child_render_traversal_cube_quad_tree_node,
					client_cache_cube_quad_tree,
					child_client_cache_cube_quad_tree_node,
					*child_source_raster_quad_tree_node,
					*child_age_grid_mask_quad_tree_node,
					*child_age_grid_coverage_quad_tree_node,
					reconstructed_polygon_mesh_transform_groups,
					reconstructed_polygon_mesh_transform_group,
					polygon_mesh_drawables,
					polygon_mesh_node_intersections,
					*child_intersections_quad_tree_node,
					cube_subdivision_cache,
					child_cube_subdivision_cache_node,
					clip_cube_subdivision_cache,
					child_clip_cube_subdivision_cache_node,
					cube_quad_tree_depth + 1,
					render_cube_quad_tree_depth,
					frustum_planes,
					frustum_plane_mask,
					num_tiles_rendered_to_scene);
		}
	}
}


bool
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::cull_quad_tree(
		const present_day_polygon_mesh_membership_type &reconstructed_polygon_mesh_membership,
		const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
		const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
		cube_subdivision_cache_type &cube_subdivision_cache,
		const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node,
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
		const GLIntersect::OrientedBoundingBox quad_tree_node_bounds =
				cube_subdivision_cache.get_oriented_bounding_box(
						cube_subdivision_cache_node);

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


GPlatesOpenGL::GLCompiledDrawState::non_null_ptr_to_const_type
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::create_scene_tile_compiled_draw_state(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_to_const_type &scene_tile_texture,
		const GLUtils::QuadTreeUVTransform &scene_tile_uv_transform,
		cube_subdivision_cache_type &cube_subdivision_cache,
		const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node,
		clip_cube_subdivision_cache_type &clip_cube_subdivision_cache,
		const clip_cube_subdivision_cache_type::node_reference_type &clip_cube_subdivision_cache_node)
{
	PROFILE_FUNC();

	// Get the view/projection transforms for the current cube quad tree node.

	// The view transform never changes within a cube face so it's the same across
	// an entire cube face quad tree (each cube face has its own quad tree).
	const GLTransform::non_null_ptr_to_const_type view_transform =
			cube_subdivision_cache.get_view_transform(
					cube_subdivision_cache_node);

	// Expand the source tile frustum by half a texel around the border of the frustum.
	// This causes the texel centres of the border tile texels to fall right on the edge
	// of the unmodified frustum which means adjacent tiles will have the same colour after
	// bilinear filtering and hence there will be no visible colour seams (or discontinuities
	// in the raster data if the source raster is floating-point).
	const GLTransform::non_null_ptr_to_const_type projection_transform =
			cube_subdivision_cache.get_projection_transform(
					cube_subdivision_cache_node);
	// The regular projection transform (unexpanded) used for the clip texture.
	const GLTransform::non_null_ptr_to_const_type clip_projection_transform =
			clip_cube_subdivision_cache.get_projection_transform(
					clip_cube_subdivision_cache_node);

	// Start compiling the scene tile state.
	GLRenderer::CompileDrawStateScope compile_draw_state_scope(renderer);

	// Used to transform texture coordinates to account for partial coverage of current tile.
	GLMatrix scene_tile_texture_matrix;
	scene_tile_uv_transform.inverse_transform(scene_tile_texture_matrix);
	scene_tile_texture_matrix.gl_mult_matrix(GLUtils::get_clip_space_to_texture_space_transform());
	// Set up the texture matrix to perform model-view and projection transforms of the frustum.
	scene_tile_texture_matrix.gl_mult_matrix(projection_transform->get_matrix());
	scene_tile_texture_matrix.gl_mult_matrix(view_transform->get_matrix());
	// Load texture transform into texture unit 0.
	renderer.gl_load_texture_matrix(GL_TEXTURE0, scene_tile_texture_matrix);

	// Bind the scene tile texture to texture unit 0.
	renderer.gl_bind_texture(scene_tile_texture, GL_TEXTURE0, GL_TEXTURE_2D);

	// Modulate the scene tile texture with the clip texture.
	//
	// Convert the clip-space range [-1, 1] to texture coord range [0.25, 0.75] so that the
	// frustrum edges will map to the boundary of the interior 2x2 clip region of our
	// 4x4 clip texture.
	//
	// NOTE: We don't adjust the clip texture matrix in case viewport only covers a part of the
	// tile texture - this is because the clip texture is always centred on the view frustum regardless.
	//
	// NOTE: We also do *not* expand the tile frustum since the clip texture uses nearest filtering
	// instead of bilinear filtering and hence we're not removing a seam between tiles
	// (instead we are clipping adjacent tiles).
	GLMatrix clip_texture_matrix(d_xy_clip_texture_transform);
	// Set up the texture matrix to perform model-view and projection transforms of the frustum.
	clip_texture_matrix.gl_mult_matrix(clip_projection_transform->get_matrix());
	clip_texture_matrix.gl_mult_matrix(view_transform->get_matrix());
	// Load texture transform into texture unit 1.
	renderer.gl_load_texture_matrix(GL_TEXTURE1, clip_texture_matrix);

	// Bind the clip texture to texture unit 1.
	renderer.gl_bind_texture(d_xy_clip_texture, GL_TEXTURE1, GL_TEXTURE_2D);

	// Use shader program (if supported), otherwise the fixed-function pipeline.
	// A valid shader program means we have a floating-point source raster.
	if (d_render_floating_point_tile_to_scene_program_object)
	{
		// Bind the shader program.
		renderer.gl_bind_program_object(d_render_floating_point_tile_to_scene_program_object.get());

		// Set the tile texture sampler to texture unit 0.
		d_render_floating_point_tile_to_scene_program_object.get()->gl_uniform1i(
				renderer, "tile_texture_sampler", 0/*texture unit*/);
		// Set the clip texture sampler to texture unit 1.
		d_render_floating_point_tile_to_scene_program_object.get()->gl_uniform1i(
				renderer, "clip_texture_sampler", 1/*texture unit*/);

		// We don't need to enable alpha-testing because the fragment shader uses 'discard' instead.
	}
	else // Fixed function...
	{
		// Enable texturing and set the texture function on texture unit 0.
		renderer.gl_enable_texture(GL_TEXTURE0, GL_TEXTURE_2D);
		renderer.gl_tex_env(GL_TEXTURE0, GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		// Set up texture coordinate generation from the vertices (x,y,z) on texture unit 0.
		GLUtils::set_object_linear_tex_gen_state(renderer, 0/*texture_unit*/);

		// NOTE: If two texture units are not supported then just don't clip to the tile.
		// The 'is_supported()' method should have been called to prevent us from getting here though.
		if (GLContext::get_parameters().texture.gl_max_texture_units >= 2)
		{
			// Enable texturing and set the texture function on texture unit 1.
			renderer.gl_enable_texture(GL_TEXTURE1, GL_TEXTURE_2D);
			renderer.gl_tex_env(GL_TEXTURE1, GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			// Set up texture coordinate generation from the vertices (x,y,z) on texture unit 1.
			GLUtils::set_object_linear_tex_gen_state(renderer, 1/*texture_unit*/);
		}

		// Alpha-test state.
		// This enables the alpha texture clipping (done by reconstructed raster renderer) to mask out
		// source regions as they're being rendered rather than writing RGBA of zero into the render-target
		// effectively overwriting previously rendered (from same render call) valid data - when the
		// render-target is used for rendering in turn as a texture it would come out as transparent
		// blocky regions where there should be valid reconstructed raster data instead.
		//
		// NOTE: This is needed not only for texture clipping but also in case the source raster
		// is age-masked in which case zero alpha also means don't draw due to age test.
		renderer.gl_enable(GL_ALPHA_TEST);
		renderer.gl_alpha_func(GL_GREATER, GLclampf(0));
	}

	// NOTE: We don't set alpha-blending state here because we
	// might not be rendering directly to the final render target and hence we don't
	// want to double-blend semi-transparent rasters - the alpha value is multiplied by
	// all channels including alpha during alpha blending (R,G,B,A) -> (A*R,A*G,A*B,A*A) -
	// the final render target would then have a source blending contribution of (3A*R,3A*G,3A*B,4A)
	// which is not what we want - we want (A*R,A*G,A*B,A*A).

#if 0
	// Used to render as wire-frame meshes instead of filled textured meshes for
	// visualising mesh density.
	renderer.gl_polygon_mode(GL_FRONT_AND_BACK, GL_LINE);
#endif

	return compile_draw_state_scope.get_compiled_draw_state();
}


void
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::render_source_raster_tile_to_scene(
		GLRenderer &renderer,
		render_traversal_cube_quad_tree_type::node_type &render_traversal_cube_quad_tree_node,
		client_cache_cube_quad_tree_type::node_type &client_cache_cube_quad_tree_node,
		const raster_quad_tree_node_type &source_raster_quad_tree_node,
		const GLUtils::QuadTreeUVTransform &source_raster_uv_transform,
		const reconstructed_polygon_mesh_transform_group_type &reconstructed_polygon_mesh_transform_group,
		const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
		const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
		const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables,
		cube_subdivision_cache_type &cube_subdivision_cache,
		const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node,
		clip_cube_subdivision_cache_type &clip_cube_subdivision_cache,
		const clip_cube_subdivision_cache_type::node_reference_type &clip_cube_subdivision_cache_node,
		unsigned int &num_tiles_rendered_to_scene)
{
	PROFILE_FUNC();

	// If we haven't yet cached a scene tile state set for the current quad tree node then do so.
	if (!render_traversal_cube_quad_tree_node.get_element().scene_tile_compiled_draw_state)
	{
		// Get the source raster texture.
		// Since it's a cube texture it may, in turn, have to render its source raster
		// into its texture (which it then passes to us to use).
		GLMultiResolutionCubeRaster::cache_handle_type source_raster_cache_handle;
		const boost::optional<GLTexture::shared_ptr_to_const_type> source_raster_texture =
				source_raster_quad_tree_node.get_tile_texture(
						renderer,
						source_raster_cache_handle);
		if (!source_raster_texture)
		{
			// Note that this should never happen since we're using a 'GLMultiResolutionCubeRaster'.
			return;
		}

		// Cache the source raster texture to return to our client.
		// This prevents the texture from being immediately recycled for another tile immediately
		// after we render the tile using it.
		// NOTE: This caching happens across render frames (thanks to the client).
		client_cache_cube_quad_tree_node.get_element().source_raster_cache_handle = source_raster_cache_handle;

		// Prepare for rendering the current scene tile.
		//
		// NOTE: This caching only happens within a render call (not across render frames).
		//
		// This caching is useful since this cube quad tree is traversed once for each transform group
		// but the state set for a specific cube quad tree node is the same for them all.
		render_traversal_cube_quad_tree_node.get_element().scene_tile_compiled_draw_state =
				create_scene_tile_compiled_draw_state(
						renderer,
						source_raster_texture.get(),
						source_raster_uv_transform,
						cube_subdivision_cache,
						cube_subdivision_cache_node,
						clip_cube_subdivision_cache,
						clip_cube_subdivision_cache_node);
	}

	render_tile_to_scene(
			renderer,
			render_traversal_cube_quad_tree_node.get_element().scene_tile_compiled_draw_state.get(),
			reconstructed_polygon_mesh_transform_group.get_visible_present_day_polygon_meshes_for_active_reconstructions(),
			polygon_mesh_node_intersections,
			intersections_quad_tree_node,
			polygon_mesh_drawables,
			num_tiles_rendered_to_scene);
}


void
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::render_tile_to_scene(
		GLRenderer &renderer,
		const GLCompiledDrawState::non_null_ptr_to_const_type &render_scene_tile_compiled_draw_state,
		const present_day_polygon_mesh_membership_type &reconstructed_polygon_mesh_membership,
		const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
		const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
		const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables,
		unsigned int &num_tiles_rendered_to_scene)
{
	PROFILE_FUNC();

	// Make sure we leave the OpenGL state the way it was.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Get the polygon mesh drawables to render for the current transform group that
	// intersect the current cube quad tree node.
	const boost::dynamic_bitset<> polygon_mesh_membership =
			reconstructed_polygon_mesh_membership.get_polygon_meshes_membership() &
			polygon_mesh_node_intersections.get_intersecting_polygon_meshes(
					intersections_quad_tree_node).get_polygon_meshes_membership();

	renderer.apply_compiled_draw_state(*render_scene_tile_compiled_draw_state);

	render_polygon_drawables(
			renderer,
			polygon_mesh_membership,
			polygon_mesh_drawables);

	// This is the only place we increment the rendered tile count.
	++num_tiles_rendered_to_scene;
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
		const boost::optional<GLReconstructedStaticPolygonMeshes::PolygonMeshDrawable> &polygon_mesh_drawable =
				polygon_mesh_drawables[polygon_mesh_index];

		if (polygon_mesh_drawable)
		{
			renderer.apply_compiled_draw_state(*polygon_mesh_drawable->drawable);
		}
	}
}


void
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::render_source_raster_and_age_grid_tile_to_scene(
		GLRenderer &renderer,
		cube_quad_tree_type::node_type &cube_quad_tree_node,
		render_traversal_cube_quad_tree_type::node_type &render_traversal_cube_quad_tree_node,
		client_cache_cube_quad_tree_type::node_type &client_cache_cube_quad_tree_node,
		const raster_quad_tree_node_type &source_raster_quad_tree_node,
		const GLUtils::QuadTreeUVTransform &source_raster_uv_transform,
		const age_grid_mask_quad_tree_node_type &age_grid_mask_quad_tree_node,
		const age_grid_coverage_quad_tree_node_type &age_grid_coverage_quad_tree_node,
		const GLUtils::QuadTreeUVTransform &age_grid_uv_transform,
		const reconstructed_polygon_mesh_transform_groups_type &reconstructed_polygon_mesh_transform_groups,
		const reconstructed_polygon_mesh_transform_group_type &reconstructed_polygon_mesh_transform_group,
		const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
		const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
		const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables,
		cube_subdivision_cache_type &cube_subdivision_cache,
		const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node,
		clip_cube_subdivision_cache_type &clip_cube_subdivision_cache,
		const clip_cube_subdivision_cache_type::node_reference_type &clip_cube_subdivision_cache_node,
		unsigned int &num_tiles_rendered_to_scene)
{
	PROFILE_FUNC();

	// If we haven't yet cached a scene tile state set for the current quad tree node then do so.
	if (!render_traversal_cube_quad_tree_node.get_element().scene_tile_compiled_draw_state)
	{
		// Render the current age-masked scene tile if it's not currently cached.
		const GLTexture::shared_ptr_to_const_type age_masked_source_tile_texture =
				get_age_masked_source_raster_tile(
						renderer,
						cube_quad_tree_node,
						client_cache_cube_quad_tree_node,
						source_raster_quad_tree_node,
						source_raster_uv_transform,
						age_grid_mask_quad_tree_node,
						age_grid_coverage_quad_tree_node,
						age_grid_uv_transform,
						reconstructed_polygon_mesh_transform_groups,
						polygon_mesh_node_intersections,
						intersections_quad_tree_node,
						polygon_mesh_drawables,
						cube_subdivision_cache,
						cube_subdivision_cache_node);

		// Prepare for rendering the current age-masked scene tile.
		//
		// NOTE: This caching only happens within a render call (not across render frames).
		//
		// This caching is useful since this cube quad tree is traversed once for each transform group
		// but the state set for a specific cube quad tree node is the same for them all.
		render_traversal_cube_quad_tree_node.get_element().scene_tile_compiled_draw_state =
				create_scene_tile_compiled_draw_state(
						renderer,
						age_masked_source_tile_texture,
						// Note that there's no uv scaling of the age grid here because we've
						// rendered it to fit exactly in the current cube quad tree node tile...
						d_tile_root_uv_transform/*scene_tile_uv_transform*/,
						cube_subdivision_cache,
						cube_subdivision_cache_node,
						clip_cube_subdivision_cache,
						clip_cube_subdivision_cache_node);
	}

	render_tile_to_scene(
			renderer,
			render_traversal_cube_quad_tree_node.get_element().scene_tile_compiled_draw_state.get(),
			// Rendering using active *and* inactive reconstructed polygons because the age grid
			// decides the begin time of oceanic crust not the polygons. We still need the polygons
			// but we just don't obey their begin times (we treat them as distant past begin times).
			reconstructed_polygon_mesh_transform_group.get_visible_present_day_polygon_meshes_for_active_or_inactive_reconstructions(),
			polygon_mesh_node_intersections,
			intersections_quad_tree_node,
			polygon_mesh_drawables,
			num_tiles_rendered_to_scene);
}


GPlatesOpenGL::GLTexture::shared_ptr_to_const_type
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::get_age_masked_source_raster_tile(
		GLRenderer &renderer,
		cube_quad_tree_type::node_type &cube_quad_tree_node,
		client_cache_cube_quad_tree_type::node_type &client_cache_cube_quad_tree_node,
		const raster_quad_tree_node_type &source_raster_quad_tree_node,
		const GLUtils::QuadTreeUVTransform &source_raster_uv_transform,
		const age_grid_mask_quad_tree_node_type &age_grid_mask_quad_tree_node,
		const age_grid_coverage_quad_tree_node_type &age_grid_coverage_quad_tree_node,
		const GLUtils::QuadTreeUVTransform &age_grid_uv_transform,
		const reconstructed_polygon_mesh_transform_groups_type &reconstructed_polygon_mesh_transform_groups,
		const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
		const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
		const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables,
		cube_subdivision_cache_type &cube_subdivision_cache,
		const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node)
{
	// See if we've generated our tile texture and
	// see if it hasn't been recycled by the texture cache.
	age_masked_source_tile_texture_cache_type::object_shared_ptr_type age_masked_source_tile_texture =
			cube_quad_tree_node.get_element().age_masked_source_tile_texture->get_cached_object();
	if (!age_masked_source_tile_texture)
	{
		age_masked_source_tile_texture = cube_quad_tree_node.get_element()
				.age_masked_source_tile_texture->recycle_an_unused_object();
		if (!age_masked_source_tile_texture)
		{
			// Create a new tile texture.
			age_masked_source_tile_texture =
					cube_quad_tree_node.get_element().age_masked_source_tile_texture->set_cached_object(
							std::auto_ptr<AgeMaskedSourceTileTexture>(new AgeMaskedSourceTileTexture(renderer)),
							// Called whenever tile texture is returned to the cache...
							boost::bind(&AgeMaskedSourceTileTexture::returned_to_cache, _1));

			// The texture was just allocated so we need to initialise it in OpenGL.
			// Since it's effectively the source raster (modulated by the age-mask) we want it to
			// have the same texture filtering etc as the source raster.
			d_raster_to_reconstruct->create_tile_texture(
					renderer,
					age_masked_source_tile_texture->texture,
					source_raster_quad_tree_node);

			//qDebug() << "GLMultiResolutionStaticPolygonReconstructedRaster: "
			//		<< d_age_masked_source_tile_texture_cache->get_current_num_objects_in_use();
		}
		else
		{
			// Since it's effectively the source raster (modulated by the age-mask) we want it to
			// have the same texture filtering etc as the source raster.
			d_raster_to_reconstruct->update_tile_texture(
					renderer,
					age_masked_source_tile_texture->texture,
					source_raster_quad_tree_node);
		}

		// Render the age-masked source raster into our tile texture.
		render_age_masked_source_raster_into_tile(
				renderer,
				*age_masked_source_tile_texture,
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
				cube_subdivision_cache,
				cube_subdivision_cache_node);
	}
	// Our texture wasn't recycled but see if it's still valid in case the source raster or
	// age grid rasters changed underneath us.
	else if (!d_raster_to_reconstruct->get_subject_token().is_observer_up_to_date(
			cube_quad_tree_node.get_element().source_raster_observer_token) ||
		!d_age_grid_cube_raster->age_grid_mask_cube_raster->get_subject_token().is_observer_up_to_date(
			cube_quad_tree_node.get_element().age_grid_mask_observer_token) ||
		!d_age_grid_cube_raster->age_grid_coverage_cube_raster->get_subject_token().is_observer_up_to_date(
			cube_quad_tree_node.get_element().age_grid_coverage_observer_token))
	{
		// Render the age-masked source raster into our tile texture.
		render_age_masked_source_raster_into_tile(
				renderer,
				*age_masked_source_tile_texture,
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
				cube_subdivision_cache,
				cube_subdivision_cache_node);
	}

	// Add to the client's cache.
	// This prevents the texture (and its sources) from being immediately recycled for another
	// tile immediately after we render this tile.
	// NOTE: This caching happens across render frames (thanks to the client).
	client_cache_cube_quad_tree_node.get_element().age_masked_source_tile_texture = age_masked_source_tile_texture;

	return age_masked_source_tile_texture->texture;
}


// The BOOST_FOREACH macro in versions of boost before 1.37 uses the same local
// variable name in each instantiation. Nested BOOST_FOREACH macros therefore
// cause GCC to warn about shadowed declarations.
DISABLE_GCC_WARNING("-Wshadow")

void
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::render_age_masked_source_raster_into_tile(
		GLRenderer &renderer,
		AgeMaskedSourceTileTexture &age_masked_source_tile_texture,
		cube_quad_tree_type::node_type &cube_quad_tree_node,
		const raster_quad_tree_node_type &source_raster_quad_tree_node,
		const GLUtils::QuadTreeUVTransform &source_raster_uv_transform,
		const age_grid_mask_quad_tree_node_type &age_grid_mask_quad_tree_node,
		const age_grid_coverage_quad_tree_node_type &age_grid_coverage_quad_tree_node,
		const GLUtils::QuadTreeUVTransform &age_grid_uv_transform,
		const reconstructed_polygon_mesh_transform_groups_type &reconstructed_polygon_mesh_transform_groups,
		const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
		const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
		const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables,
		cube_subdivision_cache_type &cube_subdivision_cache,
		const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node)
{
	PROFILE_FUNC();

	//
	// Obtain textures of the source, age grid mask/coverage tiles.
	//

	// Get the source raster texture.
	// Since it's a cube texture it may, in turn, have to render its source raster
	// into its texture (which it then passes to us to use).
	const boost::optional<GLTexture::shared_ptr_to_const_type> source_raster_texture =
			source_raster_quad_tree_node.get_tile_texture(
					renderer,
					age_masked_source_tile_texture.source_raster_cache_handle);
	if (!source_raster_texture)
	{
		// Note that this should never happen since we're using a 'GLMultiResolutionCubeRaster'.
		return;
	}

	// If we got here then we should have an age grid.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_age_grid_cube_raster,
			GPLATES_ASSERTION_SOURCE);

	// Get the age grid mask/coverage textures.
	// Since they are cube textures they may, in turn, have to render their source rasters
	// into their textures (which they then pass to us to use).
	const boost::optional<GLTexture::shared_ptr_to_const_type> age_grid_mask_texture =
			age_grid_mask_quad_tree_node.get_tile_texture(
					renderer,
					age_masked_source_tile_texture.age_grid_mask_cache_handle);
	const boost::optional<GLTexture::shared_ptr_to_const_type> age_grid_coverage_texture =
			age_grid_coverage_quad_tree_node.get_tile_texture(
					renderer,
					age_masked_source_tile_texture.age_grid_coverage_cache_handle);
	if (!age_grid_mask_texture || !age_grid_coverage_texture)
	{
		// Note that this should never happen since we're using a 'GLMultiResolutionCubeRaster'.
		return;
	}


	// Used to transform texture coordinates to account for partial coverage of current tile
	// by the source raster.
	// This can happen if we are rendering a source raster that is lower-resolution than the age grid.
	GLMatrix source_raster_tile_coverage_texture_matrix;
	source_raster_uv_transform.inverse_transform(source_raster_tile_coverage_texture_matrix);

	// Used to transform texture coordinates to account for partial coverage of current tile
	// by the age grid tiles.
	// This can happen if we are rendering a source raster that is higher-resolution than the age grid.
	GLMatrix age_grid_partial_tile_coverage_texture_matrix;
	age_grid_uv_transform.inverse_transform(age_grid_partial_tile_coverage_texture_matrix);

	// Get all present day polygons that intersect the current cube quad tree node.
	const present_day_polygon_mesh_membership_type &present_day_polygons_intersecting_tile =
			polygon_mesh_node_intersections.get_intersecting_polygon_meshes(intersections_quad_tree_node);

	// The view transform never changes within a cube face so it's the same across
	// an entire cube face quad tree (each cube face has its own quad tree).
	const GLTransform::non_null_ptr_to_const_type view_transform =
			cube_subdivision_cache.get_view_transform(
					cube_subdivision_cache_node);

	// Get the view/projection transforms for the current cube quad tree node.
	//
	// NOTE: We get the half-texel-expanded projection transform since that is what's used to
	// lookup the tile textures (the tile textures are bilinearly filtered and the centres of
	// border texels match up with adjacent tiles).
	const GLTransform::non_null_ptr_to_const_type half_texel_expanded_projection_transform =
			cube_subdivision_cache.get_projection_transform(
					cube_subdivision_cache_node);


	//
	// Begin rendering to age masked source tile texture.
	//

	// If we have a shader program for the third pass (modulate source with age-mask) then use it.
	// This means we have a floating-point source raster.
	if (d_render_floating_point_age_masked_source_raster_third_pass_program_object)
	{
		render_floating_point_age_masked_source_raster_into_tile(
				renderer,
				age_masked_source_tile_texture.texture,
				reconstructed_polygon_mesh_transform_groups,
				polygon_mesh_drawables,
				present_day_polygons_intersecting_tile,
				*half_texel_expanded_projection_transform,
				*view_transform,
				source_raster_tile_coverage_texture_matrix,
				age_grid_partial_tile_coverage_texture_matrix,
				source_raster_texture.get(),
				age_grid_mask_texture.get(),
				age_grid_coverage_texture.get());
	}
	else // fixed-function pipeline...
	{
		render_fixed_point_age_masked_source_raster_into_tile(
				renderer,
				age_masked_source_tile_texture.texture,
				reconstructed_polygon_mesh_transform_groups,
				polygon_mesh_drawables,
				present_day_polygons_intersecting_tile,
				*half_texel_expanded_projection_transform,
				*view_transform,
				source_raster_tile_coverage_texture_matrix,
				age_grid_partial_tile_coverage_texture_matrix,
				source_raster_texture.get(),
				age_grid_mask_texture.get(),
				age_grid_coverage_texture.get());
	}


	//
	// Update the age mask source tile validity with respect to its input tiles.
	//

	// If we got here then we should have an age grid.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_age_grid_cube_raster,
			GPLATES_ASSERTION_SOURCE);

	// This tile texture is now update-to-date with the inputs used to generate it.
	d_raster_to_reconstruct->get_subject_token().update_observer(
			cube_quad_tree_node.get_element().source_raster_observer_token);

	d_age_grid_cube_raster->age_grid_mask_cube_raster->get_subject_token().update_observer(
			cube_quad_tree_node.get_element().age_grid_mask_observer_token);

	d_age_grid_cube_raster->age_grid_coverage_cube_raster->get_subject_token().update_observer(
			cube_quad_tree_node.get_element().age_grid_coverage_observer_token);
}


void
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::render_floating_point_age_masked_source_raster_into_tile(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_type &age_mask_source_texture,
		const reconstructed_polygon_mesh_transform_groups_type &reconstructed_polygon_mesh_transform_groups,
		const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables,
		const present_day_polygon_mesh_membership_type &present_day_polygons_intersecting_tile,
		const GLTransform &half_texel_expanded_projection_transform,
		const GLTransform &view_transform,
		const GLMatrix &source_raster_tile_coverage_texture_matrix,
		const GLMatrix &age_grid_partial_tile_coverage_texture_matrix,
		const GLTexture::shared_ptr_to_const_type &source_raster_texture,
		const GLTexture::shared_ptr_to_const_type &age_grid_mask_texture,
		const GLTexture::shared_ptr_to_const_type &age_grid_coverage_texture)
{
	//
	// Use the same procedure as the fixed-function pipeline for the first two passes
	// (to generate the age-grid mask) but use a shader program for the third pass to
	// modulate this with the source raster (this is all done to avoid clamping of
	// the source raster if it's a floating-point raster).
	//

	// Get an intermediate RGBA8 texture to render the age-mask into.
	const GLTexture::shared_ptr_type intermediate_texture =
			renderer.get_context().get_shared_state()->acquire_texture(
					renderer,
					GL_TEXTURE_2D,
					GL_RGBA8,
					d_tile_texel_dimension,
					d_tile_texel_dimension);

	// 'acquire_texture' initialises the texture memory (to empty) but does not set the filtering
	// state when it creates a new texture.
	// Also even if the texture was cached it might have been used by another client that specified
	// different filtering settings for it.
	// So we set the filtering settings each time we acquire.
	intermediate_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	intermediate_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	// Turn off anisotropic filtering (don't need it).
	if (GLEW_EXT_texture_filter_anisotropic)
	{
		intermediate_texture->gl_tex_parameterf(renderer, GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
	}

	// Clamp texture coordinates to centre of edge texels -
	// it's easier for hardware to implement - and doesn't affect our calculations.
	if (GLEW_EXT_texture_edge_clamp || GLEW_SGIS_texture_edge_clamp)
	{
		intermediate_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		intermediate_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	else
	{
		intermediate_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		intermediate_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}

	{
		// Begin rendering to a 2D render target.
		GLRenderer::RenderTarget2DScope render_target_scope(
				renderer,
				intermediate_texture);

		// Clear colour to all ones.
		renderer.gl_clear_color(1, 1, 1, 1);
		// Clear only the colour buffer.
		renderer.gl_clear(GL_COLOR_BUFFER_BIT);

		// The render target viewport.
		renderer.gl_viewport(0, 0, d_tile_texel_dimension, d_tile_texel_dimension);

		//
		// Render two of the three fixed-function passes.
		//

		render_fixed_function_pipeline_first_pass_age_masked_source_raster(
				renderer,
				age_grid_mask_texture,
				age_grid_partial_tile_coverage_texture_matrix);

		render_fixed_function_pipeline_second_pass_age_masked_source_raster(
				renderer,
				age_grid_coverage_texture,
				age_grid_partial_tile_coverage_texture_matrix,
				half_texel_expanded_projection_transform,
				view_transform,
				reconstructed_polygon_mesh_transform_groups,
				present_day_polygons_intersecting_tile,
				polygon_mesh_drawables);
	}

	// The third pass uses a shader program.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_render_floating_point_age_masked_source_raster_third_pass_program_object,
			GPLATES_ASSERTION_SOURCE);

	// Begin rendering to a 2D render target.
	GLRenderer::RenderTarget2DScope render_target_scope(renderer, age_mask_source_texture);

	// The render target viewport.
	renderer.gl_viewport(0, 0, d_tile_texel_dimension, d_tile_texel_dimension);

	// Clear colour to all zeros.
	renderer.gl_clear_color();
	// Clear only the colour buffer.
	renderer.gl_clear(GL_COLOR_BUFFER_BIT);

	// Bind the shader program.
	renderer.gl_bind_program_object(d_render_floating_point_age_masked_source_raster_third_pass_program_object.get());

	// Bind the source raster texture to texture unit 0.
	renderer.gl_bind_texture(source_raster_texture, GL_TEXTURE0, GL_TEXTURE_2D);

	// Bind the age-mask texture to texture unit 1.
	renderer.gl_bind_texture(intermediate_texture, GL_TEXTURE1, GL_TEXTURE_2D);

	// Used to transform texture coordinates of *source* raster to account for partial coverage of current tile.
	d_render_floating_point_age_masked_source_raster_third_pass_program_object.get()->gl_uniform_matrix4x4f(
			renderer, "source_texture_transform", source_raster_tile_coverage_texture_matrix);

	// Set the source texture sampler to texture unit 0.
	d_render_floating_point_age_masked_source_raster_third_pass_program_object.get()->gl_uniform1i(
			renderer, "source_texture_sampler", 0/*texture unit*/);

	// Set the age mask texture sampler to texture unit 1.
	d_render_floating_point_age_masked_source_raster_third_pass_program_object.get()->gl_uniform1i(
			renderer, "age_mask_texture_sampler", 1/*texture unit*/);

	//
	// Render the third render pass.
	//

	// NOTE: We leave the model-view and projection matrices as identity as that is what we
	// need to draw a full-screen quad.
	renderer.apply_compiled_draw_state(*d_full_screen_quad_drawable);
}


void
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::render_fixed_point_age_masked_source_raster_into_tile(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_type &age_mask_source_texture,
		const reconstructed_polygon_mesh_transform_groups_type &reconstructed_polygon_mesh_transform_groups,
		const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables,
		const present_day_polygon_mesh_membership_type &present_day_polygons_intersecting_tile,
		const GLTransform &half_texel_expanded_projection_transform,
		const GLTransform &view_transform,
		const GLMatrix &source_raster_tile_coverage_texture_matrix,
		const GLMatrix &age_grid_partial_tile_coverage_texture_matrix,
		const GLTexture::shared_ptr_to_const_type &source_raster_texture,
		const GLTexture::shared_ptr_to_const_type &age_grid_mask_texture,
		const GLTexture::shared_ptr_to_const_type &age_grid_coverage_texture)
{
	// Begin rendering to a 2D render target.
	GLRenderer::RenderTarget2DScope render_target_scope(renderer, age_mask_source_texture);

	//
	// Setup common rendering state for some of the three age grid mask render passes.
	//

	// Clear colour to all ones.
	renderer.gl_clear_color(1, 1, 1, 1);
	// Clear only the colour buffer.
	renderer.gl_clear(GL_COLOR_BUFFER_BIT);

	// The render target viewport.
	renderer.gl_viewport(0, 0, d_tile_texel_dimension, d_tile_texel_dimension);


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

	render_fixed_function_pipeline_first_pass_age_masked_source_raster(
			renderer,
			age_grid_mask_texture,
			age_grid_partial_tile_coverage_texture_matrix);

	render_fixed_function_pipeline_second_pass_age_masked_source_raster(
			renderer,
			age_grid_coverage_texture,
			age_grid_partial_tile_coverage_texture_matrix,
			half_texel_expanded_projection_transform,
			view_transform,
			reconstructed_polygon_mesh_transform_groups,
			present_day_polygons_intersecting_tile,
			polygon_mesh_drawables);

	render_fixed_function_pipeline_third_pass_age_masked_source_raster(
			renderer,
			source_raster_texture,
			source_raster_tile_coverage_texture_matrix);
}


void
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::render_fixed_function_pipeline_first_pass_age_masked_source_raster(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_to_const_type &age_grid_mask_texture,
		const GLMatrix &age_grid_partial_tile_coverage_texture_matrix)
{
	// Make sure we leave the OpenGL state the way it was.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	//
	// First pass state
	//

	// Turns off colour channel writes because we're generating an alpha mask representing what should be drawn.
	renderer.gl_color_mask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);

	// Set the texture state for the age grid mask.
	GLUtils::set_full_screen_quad_texture_state(
			renderer,
			age_grid_mask_texture,
			0/*texture_unit*/,
			GL_REPLACE,
			age_grid_partial_tile_coverage_texture_matrix);

	//
	// Render the first render pass.
	//

	// NOTE: We leave the model-view and projection matrices as identity as that is what we
	// need to draw a full-screen quad.
 	renderer.apply_compiled_draw_state(*d_full_screen_quad_drawable);
}


void
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::render_fixed_function_pipeline_second_pass_age_masked_source_raster(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_to_const_type &age_grid_coverage_texture,
		const GLMatrix &age_grid_partial_tile_coverage_texture_matrix,
		const GLTransform &half_texel_expanded_projection_transform,
		const GLTransform &view_transform,
		const reconstructed_polygon_mesh_transform_groups_type &reconstructed_polygon_mesh_transform_groups,
		const present_day_polygon_mesh_membership_type &present_day_polygons_intersecting_tile,
		const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables)
{
	// Make sure we leave the OpenGL state the way it was.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	//
	// Second pass state
	//

	// Since we're rendering to a frustum (instead of a full-screen quad) we also need to convert
	// from clip coordinates ([-1,1]) to texture coordinates ([0,1]).
	GLMatrix age_grid_partial_tile_coverage_clip_space_to_texture_space_matrix(
			age_grid_partial_tile_coverage_texture_matrix);
	age_grid_partial_tile_coverage_clip_space_to_texture_space_matrix.gl_mult_matrix(
			GLUtils::get_clip_space_to_texture_space_transform());

	// Turns off colour channel writes because we're generating an alpha mask representing what should be drawn.
	renderer.gl_color_mask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);

	// Set the texture state for the age grid coverage.
	GLUtils::set_frustum_texture_state(
			renderer,
			age_grid_coverage_texture,
			half_texel_expanded_projection_transform.get_matrix(),
			view_transform.get_matrix(),
			0/*texture_unit*/,
			GL_REPLACE,
			age_grid_partial_tile_coverage_clip_space_to_texture_space_matrix);

	// Alpha-blend state.
	renderer.gl_enable(GL_BLEND);
	renderer.gl_blend_func(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

#if 0
	// Alpha-test state.
	renderer.gl_enable(GL_ALPHA_TEST);
	renderer.gl_alpha_func(GL_GREATER, GLclampf(0));
#endif

	renderer.gl_mult_matrix(GL_MODELVIEW, view_transform.get_matrix());
	renderer.gl_mult_matrix(GL_PROJECTION, half_texel_expanded_projection_transform.get_matrix());

	//
	// Render the second render pass.
	//

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
}


void
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::render_fixed_function_pipeline_third_pass_age_masked_source_raster(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_to_const_type &source_raster_texture,
		const GLMatrix &source_raster_tile_coverage_texture_matrix)
{
	// Make sure we leave the OpenGL state the way it was.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	//
	// Third pass state
	//

	// Set the texture state for the source raster.
	GLUtils::set_full_screen_quad_texture_state(
			renderer,
			source_raster_texture,
			0/*texture_unit*/,
			GL_REPLACE,
			source_raster_tile_coverage_texture_matrix);

	// Alpha-blend state.
	renderer.gl_enable(GL_BLEND);
	renderer.gl_blend_func(GL_DST_COLOR, GL_ZERO);

	//
	// Render the third render pass.
	//

	// NOTE: We leave the model-view and projection matrices as identity as that is what we
	// need to draw a full-screen quad.
	renderer.apply_compiled_draw_state(*d_full_screen_quad_drawable);
}


// See above.
ENABLE_GCC_WARNING("-Wshadow")


void
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::create_floating_point_shader_programs(
		GLRenderer &renderer)
{
	// If shader programs are supported then use them to render the raster tile.
	//
	// If floating-point textures are supported then shader programs should also be supported.
	// If they are not for some reason (pretty unlikely) then revert to using the fixed-function pipeline.
	//
	// NOTE: The reason for doing this (instead of just using the fixed-function pipeline always)
	// is to prevent clamping (to [0,1] range) of floating-point textures.
	// The raster texture might be rendered as floating-point (if we're being used for
	// data analysis instead of visualisation). The programmable pipeline has no clamping by default
	// whereas the fixed-function pipeline does (both clamping at the fragment output and internal
	// clamping in the texture environment stages). This clamping can be controlled by the
	// 'GL_ARB_color_buffer_float' extension (which means we could use the fixed-function pipeline
	// always) but that extension is not available on Mac OSX 10.5 (Leopard) on any hardware
	// (rectified in 10.6) so instead we'll just use the programmable pipeline whenever it's
	// available (and all platforms that support GL_ARB_texture_float should also support shaders).
	GLShaderProgramUtils::ShaderSource render_floating_point_tile_to_scene_fragment_shader_source;
	// Add the bilinear GLSL function first.
	render_floating_point_tile_to_scene_fragment_shader_source.add_shader_source(
			GLShaderProgramUtils::BILINEAR_FILTER_SHADER_SOURCE);
	// Then add the GLSL 'main()' function.
	render_floating_point_tile_to_scene_fragment_shader_source.add_shader_source(
			RENDER_FLOATING_POINT_TILE_TO_SCENE_FRAGMENT_SHADER_SOURCE);
	// Create the shader program.
	d_render_floating_point_tile_to_scene_program_object =
			GLShaderProgramUtils::compile_and_link_vertex_fragment_program(
					renderer,
					RENDER_FLOATING_POINT_TILE_TO_SCENE_VERTEX_SHADER_SOURCE,
					render_floating_point_tile_to_scene_fragment_shader_source);
	if (d_render_floating_point_tile_to_scene_program_object)
	{
		// Set the tile texture dimensions (and inverse dimensions).
		// This uniform is constant (only needs to be reloaded if shader program is re-linked).
		d_render_floating_point_tile_to_scene_program_object.get()->gl_uniform4f(
				renderer,
				"tile_texture_dimensions",
				d_tile_texel_dimension, d_tile_texel_dimension,
				d_inverse_tile_texel_dimension, d_inverse_tile_texel_dimension);
	}

	// A similar argument applies to the rendering of the age-masked source raster.
	//
	// An additional note is the third fixed-function pipeline pass for age-masking the source
	// raster tiles relies on framebuffer alpha-blending and earlier hardware supporting
	// floating-point textures does not support floating-point framebuffer blending.
	// So the age-mask modulation is then taken care of in the fragment shader directly.
	d_render_floating_point_age_masked_source_raster_third_pass_program_object =
			GLShaderProgramUtils::compile_and_link_vertex_fragment_program(
					renderer,
					RENDER_FLOATING_POINT_AGE_MASKED_SOURCE_RASTER_THIRD_PASS_VERTEX_SHADER_SOURCE,
					RENDER_FLOATING_POINT_AGE_MASKED_SOURCE_RASTER_THIRD_PASS_FRAGMENT_SHADER_SOURCE);
}


// It seems some template instantiations happen at the end of the file.
// Disabling shadow warning caused by boost object_pool (used by GPlatesUtils::ObjectCache).
DISABLE_GCC_WARNING("-Wshadow")

