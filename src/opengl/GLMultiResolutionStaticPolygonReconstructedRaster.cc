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
#include <string>

#include "global/CompilerWarnings.h"

#include <boost/cast.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <QDebug>

#include "GLMultiResolutionStaticPolygonReconstructedRaster.h"

#include "GLContext.h"
#include "GLDataRasterSource.h"
#include "GLIntersect.h"
#include "GLLight.h"
#include "GLMatrix.h"
#include "GLMultiResolutionRaster.h"
#include "GLShader.h"
#include "GLShaderSource.h"
#include "GLUtils.h"
#include "GLVertexUtils.h"
#include "GLViewport.h"
#include "GLViewProjection.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "gui/Colour.h"

#include "maths/MathsUtils.h"
#include "maths/UnitQuaternion3D.h"

#include "utils/CallStackTracker.h"
#include "utils/Profile.h"


namespace
{
	//! The inverse of log(2.0).
	const float INVERSE_LOG2 = 1.0 / std::log(2.0);

	//! Vertex shader source code to render raster tiles to the scene.
	const QString RENDER_TILE_TO_SCENE_VERTEX_SHADER_SOURCE_FILE_NAME =
			":/opengl/multi_resolution_static_polygon_reconstructed_raster/render_tile_to_scene_vertex_shader.glsl";

	//! Fragment shader source code to render raster tiles to the scene.
	const QString RENDER_TILE_TO_SCENE_FRAGMENT_SHADER_SOURCE_FILE_NAME =
			":/opengl/multi_resolution_static_polygon_reconstructed_raster/render_tile_to_scene_fragment_shader.glsl";
}


GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::GLMultiResolutionStaticPolygonReconstructedRaster(
		GL &gl,
		const double &reconstruction_time,
		const GLMultiResolutionCubeRaster::non_null_ptr_type &source_raster,
		const std::vector<GLReconstructedStaticPolygonMeshes::non_null_ptr_type> &reconstructed_static_polygon_meshes,
		boost::optional<GLMultiResolutionCubeMesh::non_null_ptr_to_const_type> multi_resolution_cube_mesh,
		boost::optional<GLMultiResolutionCubeRaster::non_null_ptr_type> age_grid_raster,
		boost::optional<GLMultiResolutionCubeRaster::non_null_ptr_type> normal_map_raster,
		boost::optional<GLLight::non_null_ptr_type> light) :
	d_reconstruction_time(reconstruction_time),
	d_source_raster(source_raster),
	d_reconstructed_static_polygon_meshes(reconstructed_static_polygon_meshes),
	d_reconstructed_static_polygon_meshes_observer_tokens(reconstructed_static_polygon_meshes.size()),
	d_multi_resolution_cube_mesh(multi_resolution_cube_mesh),
	d_source_raster_tile_texel_dimension(source_raster->get_tile_texel_dimension()),
	d_source_raster_inverse_tile_texel_dimension(1.0f / source_raster->get_tile_texel_dimension()),
	d_source_raster_tile_root_uv_transform(
			GLUtils::QuadTreeUVTransform::get_expand_tile_ratio(
					source_raster->get_tile_texel_dimension(),
					0.5/*tile_border_overlap_in_texels*/)),
	d_xy_clip_texture(GLTextureUtils::create_xy_clip_texture_2D(gl)),
	d_z_clip_texture(GLTextureUtils::create_z_clip_texture_2D(gl)),
	d_xy_clip_texture_transform(GLTextureUtils::get_clip_texture_clip_space_to_texture_space_transform()),
	d_render_tile_to_scene_program(GLProgram::create(gl)),
	d_light(light)
#if 0	// Not needed anymore but keeping in case needed in the future...
	d_cube_quad_tree(cube_quad_tree_type::create())
#endif
{
	// We are either reconstructing the raster (using static polygon meshes) or not (instead using
	// the multi-resolution cube mesh), so one (and only one) must be valid.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			// Essentially the equivalent of boolean XOR...
			(d_reconstructed_static_polygon_meshes.empty() && d_multi_resolution_cube_mesh) ||
				(!d_reconstructed_static_polygon_meshes.empty() && !d_multi_resolution_cube_mesh),
			GPLATES_ASSERTION_SOURCE);

	if (d_light)
	{
		// Cannot have lighting for a data/numerical raster (it's not meant to be visualised).
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				d_source_raster->tile_texture_is_visual(),
				GPLATES_ASSERTION_SOURCE);
	}

	// If using an age-grid...
	if (age_grid_raster)
	{
		d_age_grid_cube_raster = AgeGridCubeRaster(age_grid_raster.get());
	}

	// If using a normal map (and have a light)...
	//
	// A normal map raster is only affected by lighting (so if there's no light then we don't use a normal map).
	// Note: Whether lighting is used also depends on the state of SceneLightingParameters.

	if (normal_map_raster && d_light)
	{
		d_normal_map_cube_raster = NormalMapCubeRaster(normal_map_raster.get());
	}

	// Note: This needs to be called *after* initialising the age grid and normal map rasters.
	compile_link_shader_program(gl);
}


void
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::update(
		const double &reconstruction_time)
{
	// If the reconstruction time has changed then let clients know that we have changed.
	if (!GPlatesMaths::are_almost_exactly_equal(d_reconstruction_time, reconstruction_time))
	{
		d_reconstruction_time = reconstruction_time;
		d_subject_token.invalidate();
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
	if (!d_source_raster->get_subject_token().is_observer_up_to_date(
				d_source_raster_texture_observer_token))
	{
		d_subject_token.invalidate();

		d_source_raster->get_subject_token().update_observer(
				d_source_raster_texture_observer_token);
	}

	// If the reconstructed polygons have changed (a new reconstruction).
	if (!d_reconstructed_static_polygon_meshes.empty())
	{
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				d_reconstructed_static_polygon_meshes.size() == d_reconstructed_static_polygon_meshes_observer_tokens.size(),
				GPLATES_ASSERTION_SOURCE);

		for (unsigned int n = 0; n < d_reconstructed_static_polygon_meshes.size(); ++n)
		{
			if (!d_reconstructed_static_polygon_meshes[n]->get_subject_token().is_observer_up_to_date(
						d_reconstructed_static_polygon_meshes_observer_tokens[n]))
			{
				d_subject_token.invalidate();

				d_reconstructed_static_polygon_meshes[n]->get_subject_token().update_observer(
						d_reconstructed_static_polygon_meshes_observer_tokens[n]);
			}
		}
	}

	// If the age grid has changed (a new reconstruction time).
	if (d_age_grid_cube_raster)
	{
		if (!d_age_grid_cube_raster->cube_raster->get_subject_token().is_observer_up_to_date(
				d_age_grid_cube_raster->cube_raster_observer_token))
		{
			d_subject_token.invalidate();

			d_age_grid_cube_raster->cube_raster->get_subject_token().update_observer(
					d_age_grid_cube_raster->cube_raster_observer_token);
		}
	}

	// If the normal map raster has changed.
	//
	// Note: We only use/have a normal map if we also have a light.
	if (d_normal_map_cube_raster)
	{
		if (!d_normal_map_cube_raster->cube_raster->get_subject_token().is_observer_up_to_date(
				d_normal_map_cube_raster->cube_raster_observer_token))
		{
			d_subject_token.invalidate();

			d_normal_map_cube_raster->cube_raster->get_subject_token().update_observer(
					d_normal_map_cube_raster->cube_raster_observer_token);
		}
	}

	// If the light has changed.
	if (d_light)
	{
		if (!d_light.get()->get_subject_token().is_observer_up_to_date(d_light_observer_token))
		{
			d_subject_token.invalidate();

			d_light.get()->get_subject_token().update_observer(d_light_observer_token);
		}
	}

	return d_subject_token;
}


float
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::get_level_of_detail(
		const GLViewProjection& view_projection,
		float level_of_detail_bias) const
{
	// Get the minimum size of a pixel in the current viewport when projected
	// onto the unit sphere (in model space).
	const double min_pixel_size_on_unit_sphere = view_projection.get_min_max_pixel_size_on_globe().first/*min*/;

	//
	// Calculate the cube quad tree depth we need to traverse to satisfy the resolution of the view frustum.
	// This is the equivalent of:
	//
	//    t = t0 * 2 ^ (-depth)
	//
	// ...where 't0' is the texel size of the *lowest* resolution level-of-detail and
	// 't' is the projected size of a pixel of the viewport.
	//

	// The maximum texel size of any texel projected onto the unit sphere occurs at the centre
	// of the cube faces. Not all cube subdivisions occur at the face centres but the projected
	// texel size will always be less than at the face centre so at least it's bounded and the
	// variation across the cube face is not that large so we shouldn't be using a level-of-detail
	// that is much higher than what we need.
	const float max_lowest_resolution_texel_size_on_unit_sphere = 2.0 / d_source_raster_tile_texel_dimension;

	const float cube_quad_tree_depth = INVERSE_LOG2 *
			(std::log(max_lowest_resolution_texel_size_on_unit_sphere) -
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


float
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::clamp_level_of_detail(
		float level_of_detail) const
{
	// NOTE: A negative LOD is allowed and is useful when reconstructed raster uses an age grid mask
	// that is higher resolution than the source raster itself - this allows us to render the
	// source raster at a higher resolution than it supports but at a resolution still supported by
	// the age grid (so the raster will appear blurry but its age grid masking/modulation will not).

	const unsigned int num_levels_of_detail = get_num_levels_of_detail();

	// Clamp to lowest resolution level of detail.
	if (level_of_detail > num_levels_of_detail - 1)
	{
		// If we get here then even our lowest resolution level of detail had too much resolution
		// for the specified level of detail - but this is pretty unlikely for all but the very
		// smallest of viewports.
		//
		// Note that float can represent integers (up to 23 bits) exactly so returning as float is fine.
		return num_levels_of_detail - 1;
	}

	return level_of_detail;
}


bool
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::render(
		GL &gl,
		const GLMatrix &view_projection_transform,
		float level_of_detail,
		cache_handle_type &cache_handle)
{
	PROFILE_FUNC();

	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	// We only have one shader program (for rendering each tile to the scene).
	// Bind it early since we'll be setting state in it as we traverse the cube quad tree.
	gl.UseProgram(d_render_tile_to_scene_program);

	// The shader program outputs colours for a visual source raster (or data and coverage for a data source raster)
	// that have premultiplied alpha (or coverage) because the source raster itself has premultiplied alpha (or coverage).
	//
	// So we want the alpha blending source factor to be one instead of alpha (or coverage):
	//
	// For visual rasters this means:
	//
	//   RGB =     1 * RGB_src + (1-A_src) * RGB_dst
	//     A =     1 *   A_src + (1-A_src) *   A_dst
	//
	// ..and for data rasters this means:
	//
	//   data(R)     =     1 * data_src(R)     + (1-coverage_src(A=G)) * data_dst(R)
	//   coverage(G) =     1 * coverage_src(G) + (1-coverage_src(A=G)) * coverage_dst(G)
	//
	// Note: Data rasters use the 2-channel RG texture format with data in Red and coverage in Green.
	//       Since there's no Alpha channel, the texture swizzle (set in texture object) copies coverage in the
	//       Green channel into Alpha channel where the alpha blender can access it (with GL_ONE_MINUS_SRC_ALPHA).
	//       The alpha blender output can then only store RG channels (if rendering a render target, such as cube map,
	//       and not the final/main framebuffer) and so Alpha gets discarded (but still used by alpha-blender),
	//       however that's OK since coverage is still in the Green channel.
	//
	// Also note that while the shader program can take as input a normal map raster and an age grid raster
	// (which is actually a data raster) the output is always visual (RGBA) or data (data + coverage).
	//
	gl.Enable(GL_BLEND);
	gl.BlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	// Make sure the source cube map raster has not been re-oriented via a non-identity world transform.
	// This class assumes the cube map is in its default orientation (with identity world transform).
	// If the cube map raster's world transform is already identity then this call does nothing.
	// This is necessary since the cube map raster might have been re-oriented to align it with a non-zero
	// central meridian in a map-projection (for the 2D map view or raster export).
	d_source_raster->set_world_transform(GLMatrix());

	const unsigned int num_levels_of_detail = get_num_levels_of_detail();

	// The GLMultiResolutionRasterInterface interface says an exception is thrown if level-of-detail
	// is outside the valid range, ie, if client didn't use 'clamp_level_of_detail()'.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			level_of_detail <= num_levels_of_detail - 1,
			GPLATES_ASSERTION_SOURCE);

	// Convert the level-of-detail to the source raster cube quad tree depth.
	// They are the inverse of each other - the highest resolution source level-of-detail is zero
	// which corresponds to a cube quad tree depth of 'get_num_levels_of_detail() - 1' while
	// a cube quad tree depth of zero corresponds to the lowest (used) source level-of-detail of
	// 'get_num_levels_of_detail() - 1'.
	//
	// Note that 'level_of_detail' can be negative which means rendering at a resolution that is
	// actually higher than the source raster can supply (useful if age grid mask is even higher resolution).
	// Also note that the cube quad tree depth cannot be negative due to the above assertion.
	const float source_raster_render_cube_quad_tree_depth_float = (num_levels_of_detail - 1) - level_of_detail;

	// Round to the closest deeper integer depth.
	// Using 0.9999f instead of 1.0 to avoid converting 0.0 to 1.0 and going deeper than necessary
	// when the lowest resolution (depth zero) is plenty sufficient in some cases.
	const unsigned int source_raster_render_cube_quad_tree_depth =
			boost::numeric_cast<unsigned int>(source_raster_render_cube_quad_tree_depth_float + 0.9999f);

	// If we're using an age grid then determine its render depth.
	// It can be different to the source raster if its tile dimension is different.
	unsigned int age_grid_render_cube_quad_tree_depth = 0;
	if (d_age_grid_cube_raster)
	{
		// Adjust the source raster render depth based on the ratio of tile dimensions.
		// This effectively adjusts the calculation done in 'get_level_of_detail()'.
		float age_grid_render_cube_quad_tree_depth_float =
				source_raster_render_cube_quad_tree_depth_float +
					INVERSE_LOG2 * std::log(
						d_source_raster_tile_texel_dimension *
							d_age_grid_cube_raster->inverse_tile_texel_dimension);

		// Ensure a non-negative render depth.
		if (age_grid_render_cube_quad_tree_depth_float < 0)
		{
			age_grid_render_cube_quad_tree_depth_float = 0;
		}

		// Round to the closest deeper integer depth.
		age_grid_render_cube_quad_tree_depth =
				boost::numeric_cast<unsigned int>(age_grid_render_cube_quad_tree_depth_float + 0.9999f);
	}

	// If we're using a normal map then determine its render depth.
	// It can be different to the source raster if its tile dimension is different.
	unsigned int normal_map_render_cube_quad_tree_depth = 0;
	if (d_normal_map_cube_raster)
	{
		// Adjust the source raster render depth based on the ratio of tile dimensions.
		// This effectively adjusts the calculation done in 'get_level_of_detail()'.
		float normal_map_render_cube_quad_tree_depth_float =
				source_raster_render_cube_quad_tree_depth_float +
					INVERSE_LOG2 * std::log(
						d_source_raster_tile_texel_dimension *
							d_normal_map_cube_raster->inverse_tile_texel_dimension);

		// Ensure a non-negative render depth.
		if (normal_map_render_cube_quad_tree_depth_float < 0)
		{
			normal_map_render_cube_quad_tree_depth_float = 0;
		}

		// Round to the closest deeper integer depth.
		normal_map_render_cube_quad_tree_depth =
				boost::numeric_cast<unsigned int>(normal_map_render_cube_quad_tree_depth_float + 0.9999f);
	}

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
	source_raster_cube_subdivision_cache_type::non_null_ptr_type
			source_raster_cube_subdivision_cache =
					source_raster_cube_subdivision_cache_type::create(
							GLCubeSubdivision::create(
									GLCubeSubdivision::get_expand_frustum_ratio(
											d_source_raster_tile_texel_dimension,
											0.5/* half a texel */)),
									1024/*max_num_cached_elements*/);

	// If using an age grid then it needs a different subdivision cache because it, most likely, has a
	// different tile dimension than the source raster (giving it a different half-texel frustum offset).
	boost::optional<age_grid_cube_subdivision_cache_type::non_null_ptr_type> age_grid_cube_subdivision_cache;
	if (d_age_grid_cube_raster)
	{
		age_grid_cube_subdivision_cache =
				age_grid_cube_subdivision_cache_type::create(
						GLCubeSubdivision::create(
								GLCubeSubdivision::get_expand_frustum_ratio(
										d_age_grid_cube_raster->tile_texel_dimension,
										0.5/* half a texel */)),
								1024/*max_num_cached_elements*/);
	}

	// If using a normal map then it needs a different subdivision cache because it, most likely, has a
	// different tile dimension than the source raster (giving it a different half-texel frustum offset).
	boost::optional<normal_map_cube_subdivision_cache_type::non_null_ptr_type> normal_map_cube_subdivision_cache;
	if (d_normal_map_cube_raster)
	{
		normal_map_cube_subdivision_cache =
				normal_map_cube_subdivision_cache_type::create(
						GLCubeSubdivision::create(
								GLCubeSubdivision::get_expand_frustum_ratio(
										d_normal_map_cube_raster->tile_texel_dimension,
										0.5/* half a texel */)),
								1024/*max_num_cached_elements*/);
	}

	// Cube subdivision cache for the clip texture (no frustum expansion here).
	// Also no caching required since clipping only used to set scene-tile-compiled-draw-state
	// and that is already cached across the transform groups.
	clip_cube_subdivision_cache_type::non_null_ptr_type
			clip_cube_subdivision_cache =
					clip_cube_subdivision_cache_type::create(
							GLCubeSubdivision::create());

	// Used to cache information (only during this 'render' method) that can be reused as we traverse
	// the source raster for each transform in the reconstructed polygon meshes.
	// Note that this is not actually needed for present day rendering (when no reconstructed polygon meshes).
	render_traversal_cube_quad_tree_type::non_null_ptr_type render_traversal_cube_quad_tree =
			render_traversal_cube_quad_tree_type::create();

	// Keep track of how many tiles were rendered to the scene.
	// Currently this is just used to determine if we rendered anything.
	unsigned int num_tiles_rendered_to_scene = 0;

	// If we're reconstructing the raster (using static polygon meshes) ...
	if (reconstructing_raster())
	{
		// Iterate over the reconstructed static polygon meshes (that came from different app-logic layers).
		for (auto reconstructed_static_polygon_meshes : d_reconstructed_static_polygon_meshes)
		{
			// Get the transform groups of reconstructed polygon meshes that are visible in the view frustum.
			reconstructed_polygon_mesh_transform_groups_type::non_null_ptr_to_const_type
					reconstructed_polygon_mesh_transform_groups =
							reconstructed_static_polygon_meshes->get_reconstructed_polygon_meshes(
									gl,
									view_projection_transform);

			// The polygon mesh drawables and polygon mesh cube quad tree node intersections.
			const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables =
					reconstructed_static_polygon_meshes->get_present_day_polygon_mesh_drawables();
			const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections =
					reconstructed_static_polygon_meshes->get_present_day_polygon_mesh_node_intersections();

			// Iterate over the transform groups and traverse the cube quad tree separately for each transform.
			for (const auto &reconstructed_polygon_mesh_transform_group :
					reconstructed_polygon_mesh_transform_groups->get_transform_groups())
			{
				// Post-multiply the current finite rotation transform into the view-projection transform.
				const GPlatesMaths::UnitQuaternion3D &transform_group_finite_rotation =
						reconstructed_polygon_mesh_transform_group.get_finite_rotation();

				// Set the rotation for the current rotation group.
				gl.Uniform4f(
						d_render_tile_to_scene_program->get_uniform_location(gl, "plate_rotation_quaternion"),
						transform_group_finite_rotation.x().dval(),
						transform_group_finite_rotation.y().dval(),
						transform_group_finite_rotation.z().dval(),
						transform_group_finite_rotation.w().dval());

				GLMatrix view_projection_transform_group(view_projection_transform);
				view_projection_transform_group.gl_mult_matrix(transform_group_finite_rotation);

				// Set view projection matrix for the current rotation group.
				GLfloat view_projection_transform_group_float_matrix[16];
				view_projection_transform_group.get_float_matrix(view_projection_transform_group_float_matrix);
				gl.UniformMatrix4fv(
						d_render_tile_to_scene_program->get_uniform_location(gl, "view_projection"),
						1, GL_FALSE/*transpose*/, view_projection_transform_group_float_matrix);

				render_transform_group(
						gl,
						view_projection_transform_group,
						*render_traversal_cube_quad_tree,
						*client_cache_cube_quad_tree,
						reconstructed_polygon_mesh_transform_group,
						polygon_mesh_drawables,
						polygon_mesh_node_intersections,
						*source_raster_cube_subdivision_cache,
						age_grid_cube_subdivision_cache,
						normal_map_cube_subdivision_cache,
						*clip_cube_subdivision_cache,
						source_raster_render_cube_quad_tree_depth,
						age_grid_render_cube_quad_tree_depth,
						normal_map_render_cube_quad_tree_depth,
						num_tiles_rendered_to_scene);
			}
		}
	}
	else // *not* reconstructing raster...
	{
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				d_multi_resolution_cube_mesh,
				GPLATES_ASSERTION_SOURCE);

		// There is only one transform group to render and that's an identity rotation
		// for the sole transform group since there is no reconstruction here.
		const GPlatesMaths::UnitQuaternion3D identity_finite_rotation =
				GPlatesMaths::UnitQuaternion3D::create_identity_rotation();
		gl.Uniform4f(
				d_render_tile_to_scene_program->get_uniform_location(gl, "plate_rotation_quaternion"),
				identity_finite_rotation.x().dval(),
				identity_finite_rotation.y().dval(),
				identity_finite_rotation.z().dval(),
				identity_finite_rotation.w().dval());

		// Set view projection matrix.
		GLfloat view_projection_float_matrix[16];
		view_projection_transform.get_float_matrix(view_projection_float_matrix);
		gl.UniformMatrix4fv(
				d_render_tile_to_scene_program->get_uniform_location(gl, "view_projection"),
				1, GL_FALSE/*transpose*/, view_projection_float_matrix);

		render_transform_group(
				gl,
				view_projection_transform,
				*render_traversal_cube_quad_tree,
				*client_cache_cube_quad_tree,
				boost::none/*reconstructed_polygon_mesh_transform_group*/,
				boost::none/*polygon_mesh_drawables*/,
				boost::none/*polygon_mesh_node_intersections*/,
				*source_raster_cube_subdivision_cache,
				age_grid_cube_subdivision_cache,
				normal_map_cube_subdivision_cache,
				*clip_cube_subdivision_cache,
				source_raster_render_cube_quad_tree_depth,
				age_grid_render_cube_quad_tree_depth,
				normal_map_render_cube_quad_tree_depth,
				num_tiles_rendered_to_scene);
	}

	// Return the client cache cube quad tree to the caller.
	// The caller will cache this tile to keep objects in it from being prematurely recycled by caches.
	// Note that we also need to create a boost::shared_ptr from an intrusive pointer.
	cache_handle = cache_handle_type(make_shared_from_intrusive(client_cache_cube_quad_tree));

	//qDebug() << "GLMultiResolutionStaticPolygonReconstructedRaster: Num rendered tiles: " << num_tiles_rendered_to_scene;

	return num_tiles_rendered_to_scene > 0;
}


void
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::render_transform_group(
		GL &gl,
		const GLMatrix &view_projection_transform_group,
		render_traversal_cube_quad_tree_type &render_traversal_cube_quad_tree,
		client_cache_cube_quad_tree_type &client_cache_cube_quad_tree,
		boost::optional<const reconstructed_polygon_mesh_transform_group_type &> reconstructed_polygon_mesh_transform_group,
		boost::optional<const present_day_polygon_mesh_drawables_seq_type &> polygon_mesh_drawables,
		boost::optional<const present_day_polygon_meshes_node_intersections_type &> polygon_mesh_node_intersections,
		source_raster_cube_subdivision_cache_type &source_raster_cube_subdivision_cache,
		boost::optional<age_grid_cube_subdivision_cache_type::non_null_ptr_type> &age_grid_cube_subdivision_cache,
		boost::optional<age_grid_cube_subdivision_cache_type::non_null_ptr_type> &normal_map_cube_subdivision_cache,
		clip_cube_subdivision_cache_type &clip_cube_subdivision_cache,
		const unsigned int source_raster_render_cube_quad_tree_depth,
		const unsigned int age_grid_render_cube_quad_tree_depth,
		const unsigned int normal_map_render_cube_quad_tree_depth,
		unsigned int &num_tiles_rendered_to_scene)
{
	// First get the view frustum planes.
	//
	// NOTE: The frustum planes are affected by the current model-view-projection transform,
	// which includes the rotation of the current transform group.
	// Our quad tree bounding boxes are in model space but the polygon meshes they
	// bound are rotating to new positions so we want to take that into account and map
	// the view frustum back to model space where we can test against our bounding boxes.
	//
	const GLFrustum frustum_planes(view_projection_transform_group);

	//
	// Traverse the source raster cube quad tree and the spatial partition of reconstructed polygon meshes.
	//

	// Traverse the quad trees of the cube faces.
	for (unsigned int face = 0; face < 6; ++face)
	{
		const GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face =
				static_cast<GPlatesMaths::CubeCoordinateFrame::CubeFaceType>(face);

		// If we're reconstructing the raster (using static polygon meshes) ...
		boost::optional<const present_day_polygon_meshes_intersection_partition_type::node_type &>
				intersections_quad_tree_root_node;
		if (reconstructing_raster())
		{
			// This is used to find those nodes that intersect the present day polygon meshes of
			// the reconstructed polygon meshes in the current transform group.
			// This is so we know which polygon meshes to draw for each source raster tile.
			const present_day_polygon_meshes_intersection_partition_type::node_type *
					intersections_quad_tree_root_node_ptr =
							polygon_mesh_node_intersections->get_quad_tree_root_node(cube_face);
			if (!intersections_quad_tree_root_node_ptr)
			{
				// No polygon meshes intersect the current cube face.
				continue;
			}
			intersections_quad_tree_root_node = *intersections_quad_tree_root_node_ptr;
		}

		// If we're *not* reconstructing the raster...
		boost::optional<cube_mesh_quad_tree_node_type> cube_mesh_quad_tree_root_node;
		if (!reconstructing_raster())
		{
			// Get the quad tree root node of the current cube face of the cube mesh.
			cube_mesh_quad_tree_root_node =
					d_multi_resolution_cube_mesh.get()->get_quad_tree_root_node(cube_face);
		}

		// Get the quad tree root node of the current cube face of the source raster.
		boost::optional<source_raster_quad_tree_node_type> source_raster_quad_tree_root =
				d_source_raster->get_quad_tree_root_node(cube_face);
		// If there is no source raster for the current cube face then continue to the next face.
		if (!source_raster_quad_tree_root)
		{
			continue;
		}

		// Get the source raster root cube division cache node.
		const source_raster_cube_subdivision_cache_type::node_reference_type
				source_raster_cube_subdivision_cache_root_node =
						source_raster_cube_subdivision_cache.get_quad_tree_root_node(cube_face);

		// If we're using an age grid to smooth the reconstruction...
		boost::optional<age_grid_mask_quad_tree_node_type> age_grid_mask_quad_tree_root;
		boost::optional<const GLUtils::QuadTreeUVTransform &> age_grid_root_uv_transform;
		boost::optional<age_grid_cube_subdivision_cache_type::node_reference_type> age_grid_cube_subdivision_cache_root_node;
		if (d_age_grid_cube_raster)
		{
			// Get the quad tree root node of the current cube face of the age grid mask raster.
			age_grid_mask_quad_tree_root =
					d_age_grid_cube_raster->cube_raster->get_quad_tree_root_node(cube_face);
			// Get the root uv transform.
			age_grid_root_uv_transform = d_age_grid_cube_raster->tile_root_uv_transform;
			// Get the root age grid cube division cache node.
			age_grid_cube_subdivision_cache_root_node =
					age_grid_cube_subdivision_cache.get()->get_quad_tree_root_node(cube_face);
		}
		// Note that we still render if there's no age grid for the current cube face.

		// If we're using a normal map...
		boost::optional<normal_map_quad_tree_node_type> normal_map_quad_tree_root;
		boost::optional<const GLUtils::QuadTreeUVTransform &> normal_map_root_uv_transform;
		boost::optional<normal_map_cube_subdivision_cache_type::node_reference_type> normal_map_cube_subdivision_cache_root_node;
		if (d_normal_map_cube_raster)
		{
			// Get the quad tree root node of the current cube face of the normal map raster.
			normal_map_quad_tree_root =
					d_normal_map_cube_raster->cube_raster->get_quad_tree_root_node(cube_face);
			// Get the root uv transform.
			normal_map_root_uv_transform = d_normal_map_cube_raster->tile_root_uv_transform;
			// Get the root normal map cube division cache node.
			normal_map_cube_subdivision_cache_root_node =
					normal_map_cube_subdivision_cache.get()->get_quad_tree_root_node(cube_face);
		}
		// Note that we still render if there's no normal map for the current cube face.

#if 0	// Not needed anymore but keeping in case needed in the future...
		// Get, or create, the root cube quad tree node.
		cube_quad_tree_type::node_type *cube_quad_tree_root = d_cube_quad_tree->get_quad_tree_root_node(cube_face);
		if (!cube_quad_tree_root)
		{
			cube_quad_tree_root = &d_cube_quad_tree->set_quad_tree_root_node(
					cube_face,
					QuadTreeNode());
		}
#endif

		// Get, or create, the root render traversal node.
		render_traversal_cube_quad_tree_type::node_type &render_traversal_cube_quad_tree_root =
				render_traversal_cube_quad_tree.get_or_create_quad_tree_root_node(cube_face);

		// Get, or create, the root client cache node.
		client_cache_cube_quad_tree_type::node_type &client_cache_cube_quad_tree_root =
				client_cache_cube_quad_tree.get_or_create_quad_tree_root_node(cube_face);

		// Get the root clip cube subdivision cache node.
		const clip_cube_subdivision_cache_type::node_reference_type
				clip_cube_subdivision_cache_root_node =
						clip_cube_subdivision_cache.get_quad_tree_root_node(cube_face);

		// Easier to allocate quad tree UV transforms on the heap instead of the runtime stack.
		boost::object_pool<GLUtils::QuadTreeUVTransform> pool_quad_tree_uv_transforms;

		// Render the quad tree.
		render_quad_tree(
				gl,
				pool_quad_tree_uv_transforms,
				render_traversal_cube_quad_tree,
				render_traversal_cube_quad_tree_root,
				client_cache_cube_quad_tree,
				client_cache_cube_quad_tree_root,
				*source_raster_quad_tree_root,
				d_source_raster_tile_root_uv_transform,
				age_grid_mask_quad_tree_root,
				age_grid_root_uv_transform,
				normal_map_quad_tree_root,
				normal_map_root_uv_transform,
				reconstructed_polygon_mesh_transform_group,
				polygon_mesh_drawables,
				polygon_mesh_node_intersections,
				intersections_quad_tree_root_node,
				cube_mesh_quad_tree_root_node,
				source_raster_cube_subdivision_cache,
				source_raster_cube_subdivision_cache_root_node,
				age_grid_cube_subdivision_cache,
				age_grid_cube_subdivision_cache_root_node,
				normal_map_cube_subdivision_cache,
				normal_map_cube_subdivision_cache_root_node,
				clip_cube_subdivision_cache,
				clip_cube_subdivision_cache_root_node,
				0/*cube_quad_tree_depth*/,
				source_raster_render_cube_quad_tree_depth,
				age_grid_render_cube_quad_tree_depth,
				normal_map_render_cube_quad_tree_depth,
				frustum_planes,
				// There are six frustum planes initially active
				GLFrustum::ALL_PLANES_ACTIVE_MASK,
				num_tiles_rendered_to_scene);
	}
}


void
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::render_quad_tree(
		GL &gl,
		boost::object_pool<GLUtils::QuadTreeUVTransform> &pool_quad_tree_uv_transforms,
		render_traversal_cube_quad_tree_type &render_traversal_cube_quad_tree,
		render_traversal_cube_quad_tree_type::node_type &render_traversal_cube_quad_tree_node,
		client_cache_cube_quad_tree_type &client_cache_cube_quad_tree,
		client_cache_cube_quad_tree_type::node_type &client_cache_cube_quad_tree_node,
		const source_raster_quad_tree_node_type &source_raster_quad_tree_node,
		const GLUtils::QuadTreeUVTransform &source_raster_uv_transform,
		const boost::optional<age_grid_mask_quad_tree_node_type> &age_grid_mask_quad_tree_node,
		boost::optional<const GLUtils::QuadTreeUVTransform &> age_grid_uv_transform,
		const boost::optional<age_grid_mask_quad_tree_node_type> &normal_map_quad_tree_node,
		boost::optional<const GLUtils::QuadTreeUVTransform &> normal_map_uv_transform,
		boost::optional<const reconstructed_polygon_mesh_transform_group_type &> reconstructed_polygon_mesh_transform_group,
		boost::optional<const present_day_polygon_mesh_drawables_seq_type &> polygon_mesh_drawables,
		boost::optional<const present_day_polygon_meshes_node_intersections_type &> polygon_mesh_node_intersections,
		boost::optional<const present_day_polygon_meshes_intersection_partition_type::node_type &> intersections_quad_tree_node,
		const boost::optional<cube_mesh_quad_tree_node_type> &cube_mesh_quad_tree_node,
		source_raster_cube_subdivision_cache_type &source_raster_cube_subdivision_cache,
		const source_raster_cube_subdivision_cache_type::node_reference_type &source_raster_cube_subdivision_cache_node,
		boost::optional<age_grid_cube_subdivision_cache_type::non_null_ptr_type> &age_grid_cube_subdivision_cache,
		const boost::optional<age_grid_cube_subdivision_cache_type::node_reference_type> &age_grid_cube_subdivision_cache_node,
		boost::optional<age_grid_cube_subdivision_cache_type::non_null_ptr_type> &normal_map_cube_subdivision_cache,
		const boost::optional<age_grid_cube_subdivision_cache_type::node_reference_type> &normal_map_cube_subdivision_cache_node,
		clip_cube_subdivision_cache_type &clip_cube_subdivision_cache,
		const clip_cube_subdivision_cache_type::node_reference_type &clip_cube_subdivision_cache_node,
		unsigned int cube_quad_tree_depth,
		const unsigned int source_raster_render_cube_quad_tree_depth,
		const unsigned int age_grid_render_cube_quad_tree_depth,
		const unsigned int normal_map_render_cube_quad_tree_depth,
		const GLFrustum &frustum_planes,
		boost::uint32_t frustum_plane_mask,
		unsigned int &num_tiles_rendered_to_scene)
{
	// If the current quad tree node does not intersect any polygon meshes in the current rotation
	// group or the current quad tree node is outside the view frustum then we can return early.
	if (reconstructing_raster())
	{
		const present_day_polygon_mesh_membership_type &reconstructed_polygon_mesh_membership =
				age_grid_mask_quad_tree_node
					// Using active *and* inactive reconstructed polygons for age grid...
					? reconstructed_polygon_mesh_transform_group
							->get_visible_present_day_polygon_meshes_for_active_or_inactive_reconstructions()
					// Use only *active* reconstructed polygons when not using age grid...
					: reconstructed_polygon_mesh_transform_group
							->get_visible_present_day_polygon_meshes_for_active_reconstructions();

		// Of all the present day polygon meshes that intersect the current quad tree node
		// see if any belong to the current transform group.
		//
		// This is done by seeing if there's any membership flag (for a present day polygon mesh)
		// that's true in both:
		// - the polygon meshes in the current transform group, and
		// - the polygon meshes that can possibly intersect the current quad tree node.
		if (!reconstructed_polygon_mesh_membership.get_polygon_meshes_membership().intersects(
						polygon_mesh_node_intersections->get_intersecting_polygon_meshes(
								intersections_quad_tree_node.get()).get_polygon_meshes_membership()))
		{
			// We can return early since none of the present day polygon meshes in the current
			// transform group intersect the current quad tree node.
			return;
		}
	}

	// If the frustum plane mask is zero then it means we are entirely inside the view frustum.
	// So only test for intersection if the mask is non-zero.
	if (frustum_plane_mask != 0)
	{
		// NOTE: We are using the source raster cube subdivision cache (instead of age grid) for
		// frustum culling because it's always there (unlike the age grid) and the slight difference
		// in frustum expansions between source and age grid (due to differing tile dimensions) is not
		// that important - they both cover the unexpanded frustum and that's all that's really necessary.
		const GLIntersect::OrientedBoundingBox quad_tree_node_bounds =
				source_raster_cube_subdivision_cache.get_oriented_bounding_box(
						source_raster_cube_subdivision_cache_node);

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

	// The current quad tree node is potentially visible in the frustum so we can't cull it.

	// The source raster is ready to contribute to rendering if:
	// - we're at, or exceeded, the correct level of detail for rendering source raster, or
	// - we've reached a leaf node of the source raster (highest resolution supplied).
	const bool render_source_raster_tile =
			(cube_quad_tree_depth >= source_raster_render_cube_quad_tree_depth) ||
				source_raster_quad_tree_node.is_leaf_node();

	// The age grid raster is ready to contribute to rendering if:
	// - we're not using an age grid (then it's effectively ignored), or
	// - we're at, or exceeded, the correct level of detail for rendering age grid, or
	// - we've reached a leaf node of the age grid (highest resolution supplied).
	const bool render_age_grid_tile =
			!age_grid_mask_quad_tree_node ||
				(cube_quad_tree_depth >= age_grid_render_cube_quad_tree_depth) ||
					age_grid_mask_quad_tree_node->is_leaf_node();

	// The normal map raster is ready to contribute to rendering if:
	// - we're not using a normal map (then it's effectively ignored), or
	// - we're at, or exceeded, the correct level of detail for rendering normal map, or
	// - we've reached a leaf node of the normal map (highest resolution supplied).
	const bool render_normal_map_tile =
			!normal_map_quad_tree_node ||
				(cube_quad_tree_depth >= normal_map_render_cube_quad_tree_depth) ||
					normal_map_quad_tree_node->is_leaf_node();

	// If source raster and age grid (if any) and normal map (if any) are ready to render then
	// draw the current tile to the scene.
	if (render_source_raster_tile && render_age_grid_tile && render_normal_map_tile)
	{
		render_tile_to_scene(
				gl,
				render_traversal_cube_quad_tree_node,
				client_cache_cube_quad_tree_node,
				source_raster_quad_tree_node,
				source_raster_uv_transform,
				age_grid_mask_quad_tree_node,
				age_grid_uv_transform,
				normal_map_quad_tree_node,
				normal_map_uv_transform,
				reconstructed_polygon_mesh_transform_group,
				polygon_mesh_node_intersections,
				intersections_quad_tree_node,
				polygon_mesh_drawables,
				cube_mesh_quad_tree_node,
				source_raster_cube_subdivision_cache,
				source_raster_cube_subdivision_cache_node,
				age_grid_cube_subdivision_cache,
				age_grid_cube_subdivision_cache_node,
				normal_map_cube_subdivision_cache,
				normal_map_cube_subdivision_cache_node,
				clip_cube_subdivision_cache,
				clip_cube_subdivision_cache_node);

		++num_tiles_rendered_to_scene;

		return;
	}

	//
	// Iterate over the child quad tree nodes.
	//

	for (unsigned int child_v_offset = 0; child_v_offset < 2; ++child_v_offset)
	{
		for (unsigned int child_u_offset = 0; child_u_offset < 2; ++child_u_offset)
		{
			// If we're reconstructing the raster (using static polygon meshes) ...
			boost::optional<const present_day_polygon_meshes_intersection_partition_type::node_type &>
					child_intersections_quad_tree_node;
			if (reconstructing_raster())
			{
				// This is used to find those nodes that intersect the present day polygon meshes of
				// the reconstructed polygon meshes in the current transform group.
				// This is so we know which polygon meshes to draw for each source raster tile.
				const present_day_polygon_meshes_intersection_partition_type::node_type *
						child_intersections_quad_tree_node_ptr =
								polygon_mesh_node_intersections->get_child_node(
										intersections_quad_tree_node.get(),
										child_u_offset,
										child_v_offset);
				if (!child_intersections_quad_tree_node_ptr)
				{
					// No polygon meshes intersect the current quad tree node.
					continue;
				}
				child_intersections_quad_tree_node = *child_intersections_quad_tree_node_ptr;
			}

			// If we're *not* reconstructing the raster...
			boost::optional<cube_mesh_quad_tree_node_type> child_cube_mesh_quad_tree_node;
			if (!reconstructing_raster())
			{
				// Get the child node of the current cube mesh quad tree node.
				child_cube_mesh_quad_tree_node =
						d_multi_resolution_cube_mesh.get()->get_child_node(
								cube_mesh_quad_tree_node.get(),
								child_u_offset,
								child_v_offset);
			}

			//
			// Handle child source raster tile
			//

			boost::optional<source_raster_quad_tree_node_type> child_source_raster_quad_tree_node;
			boost::optional<const GLUtils::QuadTreeUVTransform &> child_source_raster_uv_transform;

			// If the source raster is ready to contribute to rendering then we use the current
			// source raster tile (instead of child tiles) and we start UV scaling it as we continue
			// to traverse the quad tree.
			if (render_source_raster_tile)
			{
				// Just propagate the current node down the tree.
				child_source_raster_quad_tree_node = source_raster_quad_tree_node;

				// UV scale the child tiles to correctly index into current source raster tile.
				child_source_raster_uv_transform =
						*pool_quad_tree_uv_transforms.construct(
								source_raster_uv_transform, child_u_offset, child_v_offset);
			}
			else
			{
				// Get the child node of the current source raster quad tree node.
				// Note that the current source raster node is not a leaf node (therefore has children).
				child_source_raster_quad_tree_node =
						d_source_raster->get_child_node(
								source_raster_quad_tree_node, child_u_offset, child_v_offset);

				// If there is no source raster for the current child then continue to the next child.
				// This happens if the current child is not covered by the source raster.
				if (!child_source_raster_quad_tree_node)
				{
					continue;
				}

				// Just propagate the current UV scaling down the tree.
				child_source_raster_uv_transform = source_raster_uv_transform;
			}

			// Get the child source raster cube subdivision cache node.
			const source_raster_cube_subdivision_cache_type::node_reference_type
					child_source_raster_cube_subdivision_cache_node =
							source_raster_cube_subdivision_cache.get_child_node(
									source_raster_cube_subdivision_cache_node,
									child_u_offset,
									child_v_offset);

			//
			// Handle child age grid tile
			//

			boost::optional<age_grid_mask_quad_tree_node_type> child_age_grid_mask_quad_tree_node;
			boost::optional<const GLUtils::QuadTreeUVTransform &> child_age_grid_uv_transform;
			boost::optional<age_grid_cube_subdivision_cache_type::node_reference_type> child_age_grid_cube_subdivision_cache_node;

			// If using age grid on the current tile.
			if (age_grid_mask_quad_tree_node)
			{
				// If the age grid is ready to contribute to rendering then we use the current
				// age grid tile (instead of child tiles) and we start UV scaling it as we continue
				// to traverse the quad tree.
				if (render_age_grid_tile)
				{
					// Just propagate the current node down the tree.
					child_age_grid_mask_quad_tree_node = age_grid_mask_quad_tree_node;

					// UV scale the child tiles to correctly index into current age grid tile.
					child_age_grid_uv_transform =
							*pool_quad_tree_uv_transforms.construct(
									age_grid_uv_transform.get(), child_u_offset, child_v_offset);
				}
				else
				{
					// Get the child node of the current age grid quad tree node.
					// Note that the current age grid node is not a leaf node (therefore has children).
					child_age_grid_mask_quad_tree_node =
							d_age_grid_cube_raster->cube_raster->get_child_node(
									age_grid_mask_quad_tree_node.get(), child_u_offset, child_v_offset);

					// If there is no age grid for the current child then we still continue rendering.
					// This happens if the current child is not covered by the age grid.
					// It just means the source raster tile will get rendered without using an age grid.

					// Just propagate the current UV scaling down the tree.
					child_age_grid_uv_transform = age_grid_uv_transform;
				}

				// Get the child age grid cube subdivision cache node.
				child_age_grid_cube_subdivision_cache_node =
						age_grid_cube_subdivision_cache.get()->get_child_node(
								age_grid_cube_subdivision_cache_node.get(),
								child_u_offset,
								child_v_offset);
			}

			//
			// Handle child normal map tile
			//

			boost::optional<normal_map_quad_tree_node_type> child_normal_map_quad_tree_node;
			boost::optional<const GLUtils::QuadTreeUVTransform &> child_normal_map_uv_transform;
			boost::optional<normal_map_cube_subdivision_cache_type::node_reference_type> child_normal_map_cube_subdivision_cache_node;

			// If using normal map on the current tile.
			if (normal_map_quad_tree_node)
			{
				// If the normal map is ready to contribute to rendering then we use the current
				// normal map tile (instead of child tiles) and we start UV scaling it as we continue
				// to traverse the quad tree.
				if (render_normal_map_tile)
				{
					// Just propagate the current node down the tree.
					child_normal_map_quad_tree_node = normal_map_quad_tree_node;

					// UV scale the child tiles to correctly index into current normal map tile.
					child_normal_map_uv_transform =
							*pool_quad_tree_uv_transforms.construct(
									normal_map_uv_transform.get(), child_u_offset, child_v_offset);
				}
				else
				{
					// Get the child node of the current normal map quad tree node.
					// Note that the current normal map node is not a leaf node (therefore has children).
					child_normal_map_quad_tree_node =
							d_normal_map_cube_raster->cube_raster->get_child_node(
									normal_map_quad_tree_node.get(), child_u_offset, child_v_offset);

					// If there is no normal map for the current child then we still continue rendering.
					// This happens if the current child is not covered by the normal map.
					// It just means the source raster tile will get rendered without using a normal map.

					// Just propagate the current UV scaling down the tree.
					child_normal_map_uv_transform = normal_map_uv_transform;
				}

				// Get the child normal map cube subdivision cache node.
				child_normal_map_cube_subdivision_cache_node =
						normal_map_cube_subdivision_cache.get()->get_child_node(
								normal_map_cube_subdivision_cache_node.get(),
								child_u_offset,
								child_v_offset);
			}

			// Get, or create, the child render traversal node.
			render_traversal_cube_quad_tree_type::node_type &child_render_traversal_cube_quad_tree_node =
					render_traversal_cube_quad_tree.get_or_create_child_node(
							render_traversal_cube_quad_tree_node, child_u_offset, child_v_offset);

			// Get, or create, the child client cache node.
			client_cache_cube_quad_tree_type::node_type &child_render_client_cache_quad_tree_node =
					client_cache_cube_quad_tree.get_or_create_child_node(
							client_cache_cube_quad_tree_node, child_u_offset, child_v_offset);

			// Get the child clip cube subdivision cache node.
			const clip_cube_subdivision_cache_type::node_reference_type
					child_clip_cube_subdivision_cache_node =
							clip_cube_subdivision_cache.get_child_node(
									clip_cube_subdivision_cache_node,
									child_u_offset,
									child_v_offset);

			render_quad_tree(
					gl,
					pool_quad_tree_uv_transforms,
					render_traversal_cube_quad_tree,
					child_render_traversal_cube_quad_tree_node,
					client_cache_cube_quad_tree,
					child_render_client_cache_quad_tree_node,
					child_source_raster_quad_tree_node.get(),
					child_source_raster_uv_transform.get(),
					child_age_grid_mask_quad_tree_node,
					child_age_grid_uv_transform,
					child_normal_map_quad_tree_node,
					child_normal_map_uv_transform,
					reconstructed_polygon_mesh_transform_group,
					polygon_mesh_drawables,
					polygon_mesh_node_intersections,
					child_intersections_quad_tree_node,
					child_cube_mesh_quad_tree_node,
					source_raster_cube_subdivision_cache,
					child_source_raster_cube_subdivision_cache_node,
					age_grid_cube_subdivision_cache,
					child_age_grid_cube_subdivision_cache_node,
					normal_map_cube_subdivision_cache,
					child_normal_map_cube_subdivision_cache_node,
					clip_cube_subdivision_cache,
					child_clip_cube_subdivision_cache_node,
					cube_quad_tree_depth + 1,
					source_raster_render_cube_quad_tree_depth,
					age_grid_render_cube_quad_tree_depth,
					normal_map_render_cube_quad_tree_depth,
					frustum_planes,
					frustum_plane_mask,
					num_tiles_rendered_to_scene);
		}
	}
}


void
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::render_tile_to_scene(
		GL &gl,
		render_traversal_cube_quad_tree_type::node_type &render_traversal_cube_quad_tree_node,
		client_cache_cube_quad_tree_type::node_type &client_cache_cube_quad_tree_node,
		const source_raster_quad_tree_node_type &source_raster_quad_tree_node,
		const GLUtils::QuadTreeUVTransform &source_raster_uv_transform,
		const boost::optional<age_grid_mask_quad_tree_node_type> &age_grid_mask_quad_tree_node,
		boost::optional<const GLUtils::QuadTreeUVTransform &> age_grid_uv_transform,
		const boost::optional<age_grid_mask_quad_tree_node_type> &normal_map_quad_tree_node,
		boost::optional<const GLUtils::QuadTreeUVTransform &> normal_map_uv_transform,
		boost::optional<const reconstructed_polygon_mesh_transform_group_type &> reconstructed_polygon_mesh_transform_group,
		boost::optional<const present_day_polygon_meshes_node_intersections_type &> polygon_mesh_node_intersections,
		boost::optional<const present_day_polygon_meshes_intersection_partition_type::node_type &> intersections_quad_tree_node,
		boost::optional<const present_day_polygon_mesh_drawables_seq_type &> polygon_mesh_drawables,
		const boost::optional<cube_mesh_quad_tree_node_type> &cube_mesh_quad_tree_node,
		source_raster_cube_subdivision_cache_type &source_raster_cube_subdivision_cache,
		const source_raster_cube_subdivision_cache_type::node_reference_type &source_raster_cube_subdivision_cache_node,
		boost::optional<age_grid_cube_subdivision_cache_type::non_null_ptr_type> &age_grid_cube_subdivision_cache,
		const boost::optional<age_grid_cube_subdivision_cache_type::node_reference_type> &age_grid_cube_subdivision_cache_node,
		boost::optional<age_grid_cube_subdivision_cache_type::non_null_ptr_type> &normal_map_cube_subdivision_cache,
		const boost::optional<age_grid_cube_subdivision_cache_type::node_reference_type> &normal_map_cube_subdivision_cache_node,
		clip_cube_subdivision_cache_type &clip_cube_subdivision_cache,
		const clip_cube_subdivision_cache_type::node_reference_type &clip_cube_subdivision_cache_node)
{
	// If we haven't yet cached a scene tile state set for the current quad tree node then do so.
	//
	// This caching is useful since this cube quad tree is traversed once for each rotation group but
	// the state set for a specific cube quad tree node is the same for them all so it can be reused.
	//
	// NOTE: This caching only happens per-raster-render so there's no chance of a tile getting
	// stuck in the wrong state *across* render calls (eg, if lighting is toggled).
	//
	// WARNING: We need to be careful not to cache anything specific to the current rotation group
	// since the caching is reused by all transform groups that visit the current tile.
	// This basically means anything dependent on the rotation should not be cached.
	if (!render_traversal_cube_quad_tree_node.get_element().common_tile_draw_state)
	{
		// Get the tile textures (source texture and optionally age grid and normal map).
		boost::optional<GLTexture::shared_ptr_type> source_raster_texture;
		boost::optional<GLTexture::shared_ptr_type> age_grid_mask_texture;
		boost::optional<GLTexture::shared_ptr_type> normal_map_texture;
		if (!get_tile_textures(
				gl,
				source_raster_texture,
				age_grid_mask_texture,
				normal_map_texture,
				client_cache_cube_quad_tree_node,
				source_raster_quad_tree_node,
				age_grid_mask_quad_tree_node,
				normal_map_quad_tree_node))
		{
			// Note that this should never happen since we're using 'GLMultiResolutionCubeRaster's.
			return;
		}

		//
		// Prepare for rendering the current scene tile.
		//
		// NOTE: This caching only happens within a render call (not across render frames).
		//

		// The common tile state used by all transform groups when they render this tile.
		//
		// Note that we use the 'active polygons' code path since that renders opaque outside
		// the age grid regions, and transparent inside the age grid where the age test fails.
		render_traversal_cube_quad_tree_node.get_element().common_tile_draw_state =
				create_common_tile_draw_state(
						gl,
						source_raster_texture.get(),
						source_raster_uv_transform,
						age_grid_mask_texture,
						age_grid_uv_transform,
						normal_map_texture,
						normal_map_uv_transform,
						source_raster_cube_subdivision_cache,
						source_raster_cube_subdivision_cache_node,
						age_grid_cube_subdivision_cache,
						age_grid_cube_subdivision_cache_node,
						normal_map_cube_subdivision_cache,
						normal_map_cube_subdivision_cache_node,
						clip_cube_subdivision_cache,
						clip_cube_subdivision_cache_node);
	}

	const RenderQuadTreeNode::CommonTileDrawState &common_tile_draw_state =
			render_traversal_cube_quad_tree_node.get_element().common_tile_draw_state.get();

	//
	// Render the current scene tile.
	//

	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	// Apply the necessary tile state.
	set_tile_state(gl, common_tile_draw_state);

	// Whether we're using the age grid for the current tile (might not be covered by age grid).
	const bool using_age_grid = static_cast<bool>(age_grid_mask_quad_tree_node);

	// If we are not reconstructing the raster then we render use the cube mesh tile drawable
	// (instead of reconstructed static polygons).
	//
	// Note that we don't really need to cache and re-use the common tile draw state since that's
	// meant for re-use across the different transform groups, but that only applies when rendering
	// reconstructed static polygons. Here we're only visiting each cube quad tree tile once.
	// However we'll use the cache since it simplifies things (and also its cube quad tree nodes are
	// already being created during cube quad tree traversal).
	if (!reconstructing_raster())
	{
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				cube_mesh_quad_tree_node,
				GPLATES_ASSERTION_SOURCE);

		if (using_age_grid)
		{
			// We only need the active polygons pass (not inactive polygons pass) because the cube mesh covers the
			// entire tile and it so happens that enabling the active polygons path in the shader program does what
			// we want here - which is to test the age grid values against the reconstruction time where there are
			// age grid values (ie, ocean crust) and to always render outside the age grid (ie, continental crust).
			//
			// Note: Only need to set this variable when 'using_age_grid' shader variable is true.
			gl.Uniform1i(
					d_render_tile_to_scene_program->get_uniform_location(gl, "using_age_grid_active_polygons"),
					true);
		}

		// Draw the cube mesh covering the current quad tree node tile.
		cube_mesh_quad_tree_node->render_mesh_drawable(gl);

		return;
	}

	if (using_age_grid)
	{
		// We render using active *and* inactive reconstructed polygons because the age grid
		// decides the begin time of oceanic crust not the polygons.
		//
		// We render the active reconstructed polygons first followed by the inactive.

		// Note: Only need to set this variable when 'using_age_grid' shader variable is true.
		gl.Uniform1i(
				d_render_tile_to_scene_program->get_uniform_location(gl, "using_age_grid_active_polygons"),
				true);
		render_tile_polygon_drawables(
				gl,
				reconstructed_polygon_mesh_transform_group->get_visible_present_day_polygon_meshes_for_active_reconstructions(),
				polygon_mesh_node_intersections.get(),
				intersections_quad_tree_node.get(),
				polygon_mesh_drawables.get());

		// Note: Only need to set this variable when 'using_age_grid' shader variable is true.
		gl.Uniform1i(
				d_render_tile_to_scene_program->get_uniform_location(gl, "using_age_grid_active_polygons"),
				false);
		render_tile_polygon_drawables(
				gl,
				reconstructed_polygon_mesh_transform_group->get_visible_present_day_polygon_meshes_for_inactive_reconstructions(),
				polygon_mesh_node_intersections.get(),
				intersections_quad_tree_node.get(),
				polygon_mesh_drawables.get());
	}
	else // not using age grid...
	{
		render_tile_polygon_drawables(
				gl,
				reconstructed_polygon_mesh_transform_group->get_visible_present_day_polygon_meshes_for_active_reconstructions(),
				polygon_mesh_node_intersections.get(),
				intersections_quad_tree_node.get(),
				polygon_mesh_drawables.get());
	}
}


bool
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::get_tile_textures(
		GL &gl,
		boost::optional<GLTexture::shared_ptr_type> &source_raster_texture,
		boost::optional<GLTexture::shared_ptr_type> &age_grid_mask_texture,
		boost::optional<GLTexture::shared_ptr_type> &normal_map_texture,
		client_cache_cube_quad_tree_type::node_type &client_cache_cube_quad_tree_node,
		const source_raster_quad_tree_node_type &source_raster_quad_tree_node,
		const boost::optional<age_grid_mask_quad_tree_node_type> &age_grid_mask_quad_tree_node,
		const boost::optional<age_grid_mask_quad_tree_node_type> &normal_map_quad_tree_node)
{
	// Get the source raster texture.
	// Since it's a cube texture it may, in turn, have to render its source raster
	// into its texture (which it then passes to us to use).
	GLMultiResolutionCubeRaster::cache_handle_type source_raster_cache_handle;
	source_raster_texture = source_raster_quad_tree_node.get_tile_texture(gl, source_raster_cache_handle);
	if (!source_raster_texture)
	{
		// Note that this should never happen since we're using a 'GLMultiResolutionCubeRaster'.
		return false;
	}

	// Cache the source raster texture to return to our client.
	//
	// This also prevents the texture from being immediately recycled for another tile immediately
	// after we render the tile using it (when rendering multiple transform groups - in multiple passes).
	// Not doing this would cause errors because we're caching the compiled draw state which references
	// the source texture so when we come back to use the compiled draw state again (for another
	// transform group) the texture it references might have been recycled and overwritten -
	// effectively drawing the wrong texture.
	//
	// NOTE: This caching happens across render frames (thanks to the client).
	client_cache_cube_quad_tree_node.get_element().source_raster_cache_handle = source_raster_cache_handle;

	// Get the age grid mask raster texture.
	// Since it's a cube texture it may, in turn, have to render its source raster
	// into its texture (which it then passes to us to use).
	if (age_grid_mask_quad_tree_node)
	{
		// Get the age grid mask texture.
		// Since it's a cube texture it may, in turn, have to render its source raster
		// into its texture (which it then passes to us to use).
		GLMultiResolutionCubeRaster::cache_handle_type age_grid_mask_cache_handle;
		age_grid_mask_texture =
				age_grid_mask_quad_tree_node->get_tile_texture(gl, age_grid_mask_cache_handle);
		if (!age_grid_mask_texture)
		{
			// Note that this should never happen since we're using a 'GLMultiResolutionCubeRaster'.
			return false;
		}

		// Cache the source raster texture and age grid mask to return to our client.
		// As mentioned above this prevents the textures from being immediately recycled for another
		// tile immediately after we render the tile using it.
		// NOTE: This caching happens across render frames (thanks to the client).
		client_cache_cube_quad_tree_node.get_element().age_grid_mask_cache_handle = age_grid_mask_cache_handle;
	}

	// Get the normal map raster texture.
	// Since it's a cube texture it may, in turn, have to render its source raster
	// into its texture (which it then passes to us to use).
	if (normal_map_quad_tree_node)
	{
		// Get the normal map texture.
		// Since it's a cube texture it may, in turn, have to render its source raster
		// into its texture (which it then passes to us to use).
		GLMultiResolutionCubeRaster::cache_handle_type normal_map_cache_handle;
		normal_map_texture =
				normal_map_quad_tree_node->get_tile_texture(gl, normal_map_cache_handle);
		if (!normal_map_texture)
		{
			// Note that this should never happen since we're using a 'GLMultiResolutionCubeRaster'.
			return false;
		}

		// Cache the source raster texture and normal map to return to our client.
		// As mentioned above this prevents the textures from being immediately recycled for another
		// tile immediately after we render the tile using it.
		// NOTE: This caching happens across render frames (thanks to the client).
		client_cache_cube_quad_tree_node.get_element().normal_map_cache_handle = normal_map_cache_handle;
	}

	return true;
}


GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::RenderQuadTreeNode::CommonTileDrawState
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::create_common_tile_draw_state(
		GL &gl,
		const GLTexture::shared_ptr_type &source_raster_tile_texture,
		const GLUtils::QuadTreeUVTransform &source_raster_uv_transform,
		boost::optional<GLTexture::shared_ptr_type> age_grid_tile_texture,
		boost::optional<const GLUtils::QuadTreeUVTransform &> age_grid_uv_transform,
		boost::optional<GLTexture::shared_ptr_type> normal_map_tile_texture,
		boost::optional<const GLUtils::QuadTreeUVTransform &> normal_map_uv_transform,
		source_raster_cube_subdivision_cache_type &source_raster_cube_subdivision_cache,
		const source_raster_cube_subdivision_cache_type::node_reference_type &source_raster_cube_subdivision_cache_node,
		boost::optional<age_grid_cube_subdivision_cache_type::non_null_ptr_type> &age_grid_cube_subdivision_cache,
		const boost::optional<age_grid_cube_subdivision_cache_type::node_reference_type> &age_grid_cube_subdivision_cache_node,
		boost::optional<age_grid_cube_subdivision_cache_type::non_null_ptr_type> &normal_map_cube_subdivision_cache,
		const boost::optional<age_grid_cube_subdivision_cache_type::node_reference_type> &normal_map_cube_subdivision_cache_node,
		clip_cube_subdivision_cache_type &clip_cube_subdivision_cache,
		const clip_cube_subdivision_cache_type::node_reference_type &clip_cube_subdivision_cache_node)
{
	// Get the view/projection transforms for the current cube quad tree node.

	// The view transform never changes within a cube face so it's the same across
	// an entire cube face quad tree (each cube face has its own quad tree).
	const GLTransform::non_null_ptr_to_const_type view_transform =
			source_raster_cube_subdivision_cache.get_view_transform(
					source_raster_cube_subdivision_cache_node);

	// Expand the source tile frustum by half a texel around the border of the frustum.
	// This causes the texel centres of the border tile texels to fall right on the edge
	// of the unmodified frustum which means adjacent tiles will have the same colour after
	// bilinear filtering and hence there will be no visible colour seams (or discontinuities
	// in the raster data if the source raster is floating-point).
	const GLTransform::non_null_ptr_to_const_type source_raster_projection_transform =
			source_raster_cube_subdivision_cache.get_projection_transform(
					source_raster_cube_subdivision_cache_node);

	// Used to transform texture coordinates to account for partial coverage of current tile.
	// This can happen if we are rendering a source raster that is lower-resolution than the age grid.
	GLMatrix source_raster_tile_texture_matrix;
	source_raster_uv_transform.inverse_transform(source_raster_tile_texture_matrix);
	source_raster_tile_texture_matrix.gl_mult_matrix(GLUtils::get_clip_space_to_texture_space_transform());
	// Set up the texture matrix to perform model-view and projection transforms of the frustum.
	source_raster_tile_texture_matrix.gl_mult_matrix(source_raster_projection_transform->get_matrix());
	source_raster_tile_texture_matrix.gl_mult_matrix(view_transform->get_matrix());

	// The regular projection transform (unexpanded) used for the clip texture.
	const GLTransform::non_null_ptr_to_const_type clip_projection_transform =
			clip_cube_subdivision_cache.get_projection_transform(
					clip_cube_subdivision_cache_node);

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

	// The common tile state used by all transform groups when they render this tile.
	RenderQuadTreeNode::CommonTileDrawState common_tile_draw_state(
			source_raster_tile_texture,
			source_raster_tile_texture_matrix,
			clip_texture_matrix);

	//
	// Age grid texture
	//

	// If using an age grid for the current tile...
	if (age_grid_tile_texture)
	{
		// Age grid will need a different projection transform due to it, most likely, having a different
		// tile dimension than the source raster (and hence different half-texel expanded frustum).
		const GLTransform::non_null_ptr_to_const_type age_grid_projection_transform =
				age_grid_cube_subdivision_cache.get()->get_projection_transform(
						age_grid_cube_subdivision_cache_node.get());

		// Used to transform texture coordinates to account for partial coverage of current age grid tile.
		// This can happen if we are rendering a source raster that is higher-resolution than the age grid.
		GLMatrix age_grid_tile_texture_matrix;
		age_grid_uv_transform->inverse_transform(age_grid_tile_texture_matrix);
		age_grid_tile_texture_matrix.gl_mult_matrix(GLUtils::get_clip_space_to_texture_space_transform());
		// Set up the texture matrix to perform model-view and projection transforms of the frustum.
		age_grid_tile_texture_matrix.gl_mult_matrix(age_grid_projection_transform->get_matrix());
		age_grid_tile_texture_matrix.gl_mult_matrix(view_transform->get_matrix());

		// Cache the age grid texture transform and texture.
		common_tile_draw_state.age_grid_texture_transform = age_grid_tile_texture_matrix;
		common_tile_draw_state.age_grid_texture = age_grid_tile_texture;
	}

	//
	// Surface lighting and normal map raster texture
	//

	if (normal_map_tile_texture)
	{
		// Normal map will need a different projection transform due to it, most likely, having a different
		// tile dimension than the source raster (and hence different half-texel expanded frustum).
		const GLTransform::non_null_ptr_to_const_type normal_map_projection_transform =
				normal_map_cube_subdivision_cache.get()->get_projection_transform(
						normal_map_cube_subdivision_cache_node.get());

		// Used to transform texture coordinates to account for partial coverage of current normal map tile.
		// This can happen if we are rendering a source raster that is higher-resolution than the normal map.
		GLMatrix normal_map_tile_texture_matrix;
		normal_map_uv_transform->inverse_transform(normal_map_tile_texture_matrix);
		normal_map_tile_texture_matrix.gl_mult_matrix(GLUtils::get_clip_space_to_texture_space_transform());
		// Set up the texture matrix to perform model-view and projection transforms of the frustum.
		normal_map_tile_texture_matrix.gl_mult_matrix(normal_map_projection_transform->get_matrix());
		normal_map_tile_texture_matrix.gl_mult_matrix(view_transform->get_matrix());

		// Cache the normal map texture transform and texture.
		common_tile_draw_state.normal_map_texture_transform = normal_map_tile_texture_matrix;
		common_tile_draw_state.normal_map_texture = normal_map_tile_texture;
	}

	return common_tile_draw_state;
}


void
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::set_tile_state(
		GL &gl,
		const RenderQuadTreeNode::CommonTileDrawState &common_tile_draw_state)
{
	//
	// Source raster.
	//

	// Bind source texture to texture unit 0.
	gl.ActiveTexture(GL_TEXTURE0);
	gl.BindTexture(GL_TEXTURE_2D, common_tile_draw_state.source_raster_texture);

	// Source raster texture transform.
	GLfloat source_raster_texture_float_matrix[16];
	common_tile_draw_state.source_raster_texture_transform.get_float_matrix(source_raster_texture_float_matrix);
	gl.UniformMatrix4fv(
			d_render_tile_to_scene_program->get_uniform_location(gl, "source_texture_transform"),
			1, GL_FALSE/*transpose*/, source_raster_texture_float_matrix);

	//
	// Clip texture.
	//
	// NOTE: For reconstructed static polygon meshes we always need clipping, but when there's
	// no reconstruction we then use GLMultiResolutionCubeMesh which only requires clipping when
	// we reach (zoom into) the deepest levels of its cube quad tree (because otherwise it can
	// draw a mesh exactly within the current quad tree node's boundary).
	// However, in order to keep things simple (in this code and in the shader programs) we will
	// leave clipping on always.

	// Bind clip texture to texture unit 1.
	gl.ActiveTexture(GL_TEXTURE1);
	gl.BindTexture(GL_TEXTURE_2D, d_xy_clip_texture);

	// Clip texture transform.
	GLfloat clip_texture_float_matrix[16];
	common_tile_draw_state.clip_texture_transform.get_float_matrix(clip_texture_float_matrix);
	gl.UniformMatrix4fv(
			d_render_tile_to_scene_program->get_uniform_location(gl, "clip_texture_transform"),
			1, GL_FALSE/*transpose*/, clip_texture_float_matrix);

	//
	// Age grid (if used for current tile).
	//

	// Whether we're using the age grid for the current tile (might not be covered by age grid).
	const bool using_age_grid = static_cast<bool>(common_tile_draw_state.age_grid_texture);

	gl.Uniform1i(
			d_render_tile_to_scene_program->get_uniform_location(gl, "using_age_grid"),
			using_age_grid);

	if (using_age_grid)
	{
		// Bind age grid texture to texture unit 2.
		gl.ActiveTexture(GL_TEXTURE2);
		gl.BindTexture(GL_TEXTURE_2D, common_tile_draw_state.age_grid_texture.get());

		// Age grid texture transform.
		GLfloat age_grid_texture_float_matrix[16];
		common_tile_draw_state.age_grid_texture_transform->get_float_matrix(age_grid_texture_float_matrix);
		gl.UniformMatrix4fv(
				d_render_tile_to_scene_program->get_uniform_location(gl, "age_grid_texture_transform"),
				1, GL_FALSE/*transpose*/, age_grid_texture_float_matrix);

		// Set the reconstruction time for the age grid test.
		gl.Uniform1f(
				d_render_tile_to_scene_program->get_uniform_location(gl, "reconstruction_time"),
				d_reconstruction_time);
	}

	//
	// Normal map (if used for current tile).
	//

	// Whether we're using the normal map for the current tile (might not be covered by normal map).
	const bool using_normal_map = static_cast<bool>(common_tile_draw_state.normal_map_texture);

	gl.Uniform1i(
			d_render_tile_to_scene_program->get_uniform_location(gl, "using_surface_lighting_normal_map"),
			using_normal_map);

	if (using_normal_map)
	{
		// Bind normal map to texture unit 3.
		gl.ActiveTexture(GL_TEXTURE3);
		gl.BindTexture(GL_TEXTURE_2D, common_tile_draw_state.normal_map_texture.get());

		// Normal map texture transform.
		GLfloat normal_map_texture_float_matrix[16];
		common_tile_draw_state.normal_map_texture_transform->get_float_matrix(normal_map_texture_float_matrix);
		gl.UniformMatrix4fv(
				d_render_tile_to_scene_program->get_uniform_location(gl, "normal_map_texture_transform"),
				1, GL_FALSE/*transpose*/, normal_map_texture_float_matrix);
	}

	//
	// Surface lighting (if we have a light and lighting is enabled for rasters).
	//

	// Whether we're using surface lighting.
	const bool using_surface_lighting = static_cast<bool>(d_light);

	gl.Uniform1i(
			d_render_tile_to_scene_program->get_uniform_location(gl, "using_surface_lighting"),
			using_surface_lighting);

	// If we have a light and lighting is enabled for rasters...
	if (using_surface_lighting)
	{
		// Set the ambient light contribution.
		//
		// Note that it's unused in the map view when not using normal maps (but all other shader paths use it).
		gl.Uniform1f(
				d_render_tile_to_scene_program->get_uniform_location(gl, "ambient_lighting"),
				d_light.get()->get_scene_lighting_parameters().get_ambient_light_contribution());

		// Whether we're using surface lighting in map view.
		const bool using_surface_lighting_in_map_view =
				static_cast<bool>(d_light.get()->get_map_projection())/*in map view*/;

		// Note: Only need to set this variable when 'using_surface_lighting' shader variable is true.
		gl.Uniform1i(
				d_render_tile_to_scene_program->get_uniform_location(gl, "using_surface_lighting_in_map_view"),
				using_surface_lighting_in_map_view);

		if (using_normal_map)
		{
			// Whether we're using a *directional* light.
			const bool using_surface_lighting_normal_map_with_directional_light =
					d_light.get()->get_scene_lighting_parameters().is_lighting_enabled(
						GPlatesGui::SceneLightingParameters::LIGHTING_RASTER);
			// Whether we're using the radial sphere normal as the pseudo light direction
			// (instead of a directional light).
			const bool using_surface_lighting_normal_map_with_no_directional_light =
					!using_surface_lighting_normal_map_with_directional_light;

			// Note: Only need to set this variable when 'using_surface_lighting_in_map_view' shader variable is true.
			gl.Uniform1i(
					d_render_tile_to_scene_program->get_uniform_location(gl, "using_surface_lighting_normal_map_with_no_directional_light"),
					using_surface_lighting_normal_map_with_no_directional_light);

			if (using_surface_lighting_in_map_view)  // map view...
			{
				// Bind the light direction cube texture to texture unit 4.
				gl.ActiveTexture(GL_TEXTURE4);
				gl.BindTexture(GL_TEXTURE_2D, d_light.get()->get_map_view_light_direction_cube_map_texture());
			}
			else // globe view...
			{
				// Set the world space light direction.
				const GPlatesMaths::UnitVector3D &world_space_light_direction = d_light.get()->get_globe_view_light_direction();
				gl.Uniform3f(
						d_render_tile_to_scene_program->get_uniform_location(gl, "world_space_light_direction_in_globe_view"),
						world_space_light_direction.x().dval(),
						world_space_light_direction.y().dval(),
						world_space_light_direction.z().dval());
			}
		}
		else // not using normal map...
		{
			if (using_surface_lighting_in_map_view)  // map view...
			{
				gl.Uniform1f(
						d_render_tile_to_scene_program->get_uniform_location(gl, "ambient_and_diffuse_lighting_in_map_view_with_no_normal_map"),
						d_light.get()->get_map_view_constant_lighting());
			}
			else // globe view...
			{
				// Set the world space light direction.
				const GPlatesMaths::UnitVector3D &world_space_light_direction = d_light.get()->get_globe_view_light_direction();
				gl.Uniform3f(
						d_render_tile_to_scene_program->get_uniform_location(gl, "world_space_light_direction_in_globe_view"),
						world_space_light_direction.x().dval(),
						world_space_light_direction.y().dval(),
						world_space_light_direction.z().dval());
			}
		}
	}

#if 0
	// Used to render as wire-frame meshes instead of filled textured meshes for visualising mesh density.
	gl.PolygonMode(GL_FRONT_AND_BACK, GL_LINE);
#endif
}


void
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::render_tile_polygon_drawables(
		GL &gl,
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

	// Iterate over the present day polygon mesh drawables and render them.
	for (boost::dynamic_bitset<>::size_type polygon_mesh_index = polygon_mesh_membership.find_first();
		polygon_mesh_index != boost::dynamic_bitset<>::npos;
		polygon_mesh_index = polygon_mesh_membership.find_next(polygon_mesh_index))
	{
		const boost::optional<GLReconstructedStaticPolygonMeshes::PolygonMeshDrawable> &polygon_mesh_drawable =
				polygon_mesh_drawables[polygon_mesh_index];

		if (polygon_mesh_drawable)
		{
			polygon_mesh_drawable->render(gl);
		}
	}
}


void
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::compile_link_shader_program(
		GL &gl)
{
	// Add this scope to the call stack trace printed if exception thrown in this scope (eg, failure to compile/link shader).
	TRACK_CALL_STACK();

	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	// Vertex shader source.
	GLShaderSource vertex_shader_source;
	vertex_shader_source.add_code_segment_from_file(GLShaderSource::UTILS_FILE_NAME);
	vertex_shader_source.add_code_segment_from_file(RENDER_TILE_TO_SCENE_VERTEX_SHADER_SOURCE_FILE_NAME);

	// Vertex shader.
	GLShader::shared_ptr_type vertex_shader = GLShader::create(gl, GL_VERTEX_SHADER);
	vertex_shader->shader_source(gl, vertex_shader_source);
	vertex_shader->compile_shader(gl);

	// Fragment shader source.
	GLShaderSource fragment_shader_source;
	fragment_shader_source.add_code_segment_from_file(GLShaderSource::UTILS_FILE_NAME);
	fragment_shader_source.add_code_segment_from_file(RENDER_TILE_TO_SCENE_FRAGMENT_SHADER_SOURCE_FILE_NAME);

	// Fragment shader.
	GLShader::shared_ptr_type fragment_shader = GLShader::create(gl, GL_FRAGMENT_SHADER);
	fragment_shader->shader_source(gl, fragment_shader_source);
	fragment_shader->compile_shader(gl);

	// Vertex-fragment program.
	d_render_tile_to_scene_program->attach_shader(gl, vertex_shader);
	d_render_tile_to_scene_program->attach_shader(gl, fragment_shader);
	d_render_tile_to_scene_program->link_program(gl);

	//
	// Set some shader program constants that don't change.
	//

	gl.UseProgram(d_render_tile_to_scene_program);

	// Use texture unit 0 for source texture.
	gl.Uniform1i(
			d_render_tile_to_scene_program->get_uniform_location(gl, "source_texture_sampler"),
			0/*texture unit*/);
	// Use texture unit 1 for clip texture.
	gl.Uniform1i(
			d_render_tile_to_scene_program->get_uniform_location(gl, "clip_texture_sampler"),
			1/*texture unit*/);
	// Use texture unit 2 for age grid.
	gl.Uniform1i(
			d_render_tile_to_scene_program->get_uniform_location(gl, "age_grid_texture_sampler"),
			2/*texture unit*/);
	// Use texture unit 3 for normal map.
	gl.Uniform1i(
			d_render_tile_to_scene_program->get_uniform_location(gl, "normal_map_texture_sampler"),
			3/*texture unit*/);
	// Use texture unit 4 for light direction cube map (in map view when using normal maps).
	gl.Uniform1i(
			d_render_tile_to_scene_program->get_uniform_location(gl, "light_direction_cube_texture_sampler_in_map_view_with_normal_map"),
			4/*texture unit*/);
}


// It seems some template instantiations happen at the end of the file.
// Disabling shadow warning caused by boost object_pool (used by GPlatesUtils::ObjectCache).
DISABLE_GCC_WARNING("-Wshadow")

