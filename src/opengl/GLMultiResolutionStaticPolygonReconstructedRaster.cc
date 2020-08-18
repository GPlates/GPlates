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

#include <cmath>
#include <string>

#include "global/CompilerWarnings.h"

#include <boost/cast.hpp>
#include <boost/foreach.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <QDebug>

#include "GLMultiResolutionStaticPolygonReconstructedRaster.h"

#include "GLContext.h"
#include "GLDataRasterSource.h"
#include "GLIntersect.h"
#include "GLLight.h"
#include "GLMatrix.h"
#include "GLMultiResolutionRaster.h"
#include "GLRenderer.h"
#include "GLShaderProgramUtils.h"
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

#include "utils/Profile.h"


// Temporarily remove directional lighting for normal maps until GPlates 1.4 (when introduce light canvas tool)...
#define TEMPORARY_HACK_NO_DIRECTIONAL_LIGHT_FOR_NORMAL_MAPS


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

	/**
	 * A 4-component texture environment colour used to extract red channel when used with GL_ARB_texture_env_dot3.
	 */
	std::vector<GLfloat>
	create_dot3_extract_red_channel()
	{
		std::vector<GLfloat> vec(4);

		vec[0] = 1;
		vec[1] = 0.5f;
		vec[2] = 0.5f;
		vec[3] = 0;

		return vec;
	}
}


bool
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::is_supported(
		GLRenderer &renderer)
{
	static bool supported = false;

	// Only test for support the first time we're called.
	static bool tested_for_support = false;
	if (!tested_for_support)
	{
		tested_for_support = true;

		const GLCapabilities &capabilities = renderer.get_capabilities();

		// We currently need four texture units and GL_ARB_texture_env_combine and GL_ARB_texture_env_dot3.
		// For regular raster reconstruction - one for the raster and another for the clip texture.
		// For age-grid enhanced raster reconstruction - another two for the age grid coverage/mask texture.
		if (capabilities.texture.gl_max_texture_units < 4 ||
			!capabilities.texture.gl_ARB_texture_env_combine ||
			!capabilities.texture.gl_ARB_texture_env_dot3)
		{
			qWarning() <<
					"RECONSTRUCTED rasters NOT supported by this graphics hardware - requires four texture units.\n"
					"  Most graphics hardware for over a decade supports this -\n"
					"  most likely software renderer fallback has occurred - possibly via remote desktop software.";

			return false;
		}

		// If we get this far then we have support.
		supported = true;
	}

	return supported;
}


bool
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::supports_floating_point_source_raster(
		GLRenderer &renderer)
{
	static bool supported = false;

	// Only test for support the first time we're called.
	static bool tested_for_support = false;
	if (!tested_for_support)
	{
		tested_for_support = true;

		// We use GLRenderer to render to render targets so it must support rendering to
		// floating-point render targets.
		supported =
				renderer.supports_floating_point_render_target_2D() &&
					// Don't really need to check for this but will anyway in case caller expects us to...
					renderer.get_capabilities().texture.gl_ARB_texture_float;
	}

	return supported;
}


bool
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::supports_age_mask_generation(
		GLRenderer &renderer)
{
	static bool supported = false;

	// Only test for support the first time we're called.
	static bool tested_for_support = false;
	if (!tested_for_support)
	{
		tested_for_support = true;

		const GLCapabilities &capabilities = renderer.get_capabilities();

		// Need floating-point textures to support age grid in GLDataRasterSource format and
		// GLMultiResolutionCubeRaster (attached to the age-grid GLDataRasterSource) must
		// support rendering to floating-point targets.
		// Also need vertex/fragment shader support.
		if (!GLDataRasterSource::is_supported(renderer) ||
			!GLMultiResolutionCubeRaster::supports_floating_point_source_raster(renderer) ||
			!capabilities.shader.gl_ARB_vertex_shader ||
			!capabilities.shader.gl_ARB_fragment_shader)
		{
			return false;
		}

		//
		// Try to compile our most complex fragment shader program with surface lighting and
		// with normal maps - because it would be nice to have everything including normal maps.
		// If we can't have age mask generation with normal maps then there's the slower alternative
		// of pre-generated age masks with normal maps (see 'supports_normal_map()') which is
		// visually better than age mask generation with no normal maps.
		//
		// If this fails then it could be exceeding some resource limit on the runtime system
		// such as number of shader instructions allowed.
		//

		// Not configuring shader for floating-point source raster since lighting does not apply to it.
		const char *shader_defines =
			// Configure shader for using an age grid.
			"#define USING_AGE_GRID\n"
			// Configure shader for generating age grid mask.
			"#define GENERATE_AGE_MASK\n"
			// Configure shader for active polygons as that increases complexity.
			"#define ACTIVE_POLYGONS\n"
			// Configure shader for using normal maps as that increases complexity.
			"#define USING_NORMAL_MAP\n"
			// Configure shader for using map view surface lighting as that increases complexity.
			"#define SURFACE_LIGHTING\n"
			"#define MAP_VIEW\n";

		GLShaderSource fragment_shader_source;
		fragment_shader_source.add_code_segment_from_file(GLShaderSource::UTILS_FILE_NAME);
		fragment_shader_source.add_code_segment(shader_defines);
		fragment_shader_source.add_code_segment_from_file(RENDER_TILE_TO_SCENE_FRAGMENT_SHADER_SOURCE_FILE_NAME);

		GLShaderSource vertex_shader_source;
		vertex_shader_source.add_code_segment_from_file(GLShaderSource::UTILS_FILE_NAME);
		vertex_shader_source.add_code_segment(shader_defines);
		vertex_shader_source.add_code_segment_from_file(RENDER_TILE_TO_SCENE_VERTEX_SHADER_SOURCE_FILE_NAME);

		// Attempt to create the test shader program.
		if (!GLShaderProgramUtils::compile_and_link_vertex_fragment_program(
				renderer, vertex_shader_source, fragment_shader_source))
		{
			return false;
		}

		// If we get this far then we have support.
		supported = true;
	}

	return supported;
}


bool
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::supports_normal_map(
		GLRenderer &renderer)
{
	static bool supported = false;

	// Only test for support the first time we're called.
	static bool tested_for_support = false;
	if (!tested_for_support)
	{
		tested_for_support = true;

		const GLCapabilities &capabilities = renderer.get_capabilities();

		// Test input raster for normal map support.
		// Also need vertex/fragment shader support.
		if (!GLMultiResolutionRaster::supports_normal_map_source(renderer) ||
			!capabilities.shader.gl_ARB_vertex_shader ||
			!capabilities.shader.gl_ARB_fragment_shader)
		{
			return false;
		}

		//
		// Try to compile our most complex fragment shader program but without age mask generation
		// that's not necessary (has the alternative of pre-compiled age masks) and
		// 'supports_age_mask_generation()' will test for age mask generation *and* normal maps.
		//
		// If this fails then it could be exceeding some resource limit on the runtime system
		// such as number of shader instructions allowed.
		//

		// Not configuring shader for floating-point source raster since lighting does not apply to it.
		const char *shader_defines =
			// Configure shader for using normal maps.
			"#define USING_NORMAL_MAP\n"
			// Configure shader for using map view surface lighting as that increases complexity.
			"#define SURFACE_LIGHTING\n"
			"#define MAP_VIEW\n"
			// Configure shader for using an age grid as that increases complexity.
			// But we don't configure for age mask generation as mentioned above.
			"#define USING_AGE_GRID\n"
			// Configure shader for active polygons as that increases complexity.
			"#define ACTIVE_POLYGONS\n";

		GLShaderSource fragment_shader_source;
		fragment_shader_source.add_code_segment_from_file(GLShaderSource::UTILS_FILE_NAME);
		fragment_shader_source.add_code_segment(shader_defines);
		fragment_shader_source.add_code_segment_from_file(RENDER_TILE_TO_SCENE_FRAGMENT_SHADER_SOURCE_FILE_NAME);
		
		GLShaderSource vertex_shader_source;
		vertex_shader_source.add_code_segment_from_file(GLShaderSource::UTILS_FILE_NAME);
		vertex_shader_source.add_code_segment(shader_defines);
		vertex_shader_source.add_code_segment_from_file(RENDER_TILE_TO_SCENE_VERTEX_SHADER_SOURCE_FILE_NAME);

		// Attempt to create the test shader program.
		if (!GLShaderProgramUtils::compile_and_link_vertex_fragment_program(
				renderer, vertex_shader_source, fragment_shader_source))
		{
			return false;
		}

		// If we get this far then we have support.
		supported = true;
	}

	return supported;
}


GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::GLMultiResolutionStaticPolygonReconstructedRaster(
		GLRenderer &renderer,
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
	d_xy_clip_texture(GLTextureUtils::create_xy_clip_texture_2D(renderer)),
	d_z_clip_texture(GLTextureUtils::create_z_clip_texture_2D(renderer)),
	d_xy_clip_texture_transform(GLTextureUtils::get_clip_texture_clip_space_to_texture_space_transform()),
	d_full_screen_quad_drawable(renderer.get_context().get_shared_state()->get_full_screen_2D_textured_quad(renderer)),
	d_light(light)
#if 0	// Not needed anymore but keeping in case needed in the future...
	d_cube_quad_tree(cube_quad_tree_type::create())
#endif
{
	// We are either reconstructing the raster (using static polygon meshes) or not (instead using
	// the multi-resolution cube mesh), so one (and only one) must be valid.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			// Essentially the equivalent of boolean XOR (the extra NOT's ensure boolean comparison)...
			d_reconstructed_static_polygon_meshes.empty() != !d_multi_resolution_cube_mesh,
			GPLATES_ASSERTION_SOURCE);

	if (d_light)
	{
		// Cannot have lighting for a floating-point source raster (it's not meant to be visualised).
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				!GLTexture::is_format_floating_point(d_source_raster->get_tile_texture_internal_format()),
				GPLATES_ASSERTION_SOURCE);
	}

	// If using an age-grid...
	if (age_grid_raster)
	{
		d_age_grid_cube_raster = AgeGridCubeRaster(age_grid_raster.get());
	}

	// If using a normal map...
	if (normal_map_raster)
	{
		d_normal_map_cube_raster = NormalMapCubeRaster(normal_map_raster.get());
	}

	// Use shader programs for rendering if available (otherwise resort to fixed-function pipeline).
	// NOTE: This needs to be called *after* initialising the age grid and normal map rasters.
	create_shader_programs(renderer);
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
		const GLMatrix &model_view_transform,
		const GLMatrix &projection_transform,
		const GLViewport &viewport,
		float level_of_detail_bias) const
{
	// Get the minimum size of a pixel in the current viewport when projected
	// onto the unit sphere (in model space).
	GLViewProjection view_projection(viewport, model_view_transform, projection_transform);
	const double min_pixel_size_on_unit_sphere = view_projection.get_min_pixel_size_on_unit_sphere();

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
		GLRenderer &renderer,
		float level_of_detail,
		cache_handle_type &cache_handle)
{
	PROFILE_FUNC();

	// Make sure we leave the OpenGL state the way it was.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Make sure the source cube map raster has not been re-oriented via a non-identity world transform.
	// This class assumes the cube map is in its default orientation (with identity world transform).
	// If the cube map raster's world transform is already identity then this call does nothing.
	// This is necessary since the cube map raster might have be re-oriented to align it with a non-zero
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
							GPlatesOpenGL::GLCubeSubdivision::create(
									GPlatesOpenGL::GLCubeSubdivision::get_expand_frustum_ratio(
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
						GPlatesOpenGL::GLCubeSubdivision::create(
								GPlatesOpenGL::GLCubeSubdivision::get_expand_frustum_ratio(
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
						GPlatesOpenGL::GLCubeSubdivision::create(
								GPlatesOpenGL::GLCubeSubdivision::get_expand_frustum_ratio(
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
							GPlatesOpenGL::GLCubeSubdivision::create());

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
		for (unsigned int reconstructed_static_polygon_meshes_index = 0;
			reconstructed_static_polygon_meshes_index < d_reconstructed_static_polygon_meshes.size();
			++reconstructed_static_polygon_meshes_index)
		{
			const GLReconstructedStaticPolygonMeshes::non_null_ptr_type &reconstructed_static_polygon_meshes =
					d_reconstructed_static_polygon_meshes[reconstructed_static_polygon_meshes_index];

			// Get the transform groups of reconstructed polygon meshes that are visible in the view frustum.
			reconstructed_polygon_mesh_transform_groups_type::non_null_ptr_to_const_type
					reconstructed_polygon_mesh_transform_groups =
							reconstructed_static_polygon_meshes->get_reconstructed_polygon_meshes(renderer);

			// The polygon mesh drawables and polygon mesh cube quad tree node intersections.
			const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables =
					reconstructed_static_polygon_meshes->get_present_day_polygon_mesh_drawables();
			const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections =
					reconstructed_static_polygon_meshes->get_present_day_polygon_mesh_node_intersections();

			// Iterate over the transform groups and traverse the cube quad tree separately for each transform.
			BOOST_FOREACH(
					const reconstructed_polygon_mesh_transform_group_type &reconstructed_polygon_mesh_transform_group,
					reconstructed_polygon_mesh_transform_groups->get_transform_groups())
			{
				// Make sure we leave the OpenGL state the way it was.
				// We do this for each iteration of the transform groups loop.
				GLRenderer::StateBlockScope save_restore_state_transform_group(renderer);

				// Push the current finite rotation transform.
				const GLMatrix finite_rotation(reconstructed_polygon_mesh_transform_group.get_finite_rotation());
				renderer.gl_mult_matrix(GL_MODELVIEW, finite_rotation);

				render_transform_group(
						renderer,
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

		// There is only one transform group to render and that's essentially the identity transform
		// since there is no reconstruction here.
		render_transform_group(
				renderer,
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
		GLRenderer &renderer,
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
				renderer,
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
		GLRenderer &renderer,
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
				renderer,
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
					renderer,
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
		GLRenderer &renderer,
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
	//PROFILE_FUNC();

	// If we are not reconstructing the raster then the code path is simpler since we do not need
	// to cache state across transform groups, and using the age grid is simpler.
	if (!reconstructing_raster())
	{
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				cube_mesh_quad_tree_node,
				GPLATES_ASSERTION_SOURCE);

		// Make sure we leave the OpenGL state the way it was.
		GLRenderer::StateBlockScope save_restore_state(renderer);

		// Get the tile textures (source texture and optionally age grid and normal map).
		boost::optional<GLTexture::shared_ptr_to_const_type> source_raster_texture;
		boost::optional<GLTexture::shared_ptr_to_const_type> age_grid_mask_texture;
		boost::optional<GLTexture::shared_ptr_to_const_type> normal_map_texture;
		if (!get_tile_textures(
				renderer,
				source_raster_texture,
				age_grid_mask_texture,
				normal_map_texture,
				client_cache_cube_quad_tree_node,
				source_raster_quad_tree_node,
				age_grid_mask_quad_tree_node,
				normal_map_quad_tree_node))
		{
			// Note that this should never happen since we're using a 'GLMultiResolutionCubeRaster's.
			return;
		}

		// The scene tile draw state.
		// Note that we use the 'active polygons' code path since that renders opaque outside
		// the age grid regions, and transparent inside the age grid where the age test fails.
		const RenderQuadTreeNode::TileDrawState render_scene_tile_draw_state =
				create_scene_tile_draw_state(
						renderer,
						// Determine which shader program to use (if any are supported) based on
						// the configuration of the current tile and lighting...
						get_shader_program_for_tile(
								GLTexture::is_format_floating_point(d_source_raster->get_tile_texture_internal_format()),
								static_cast<bool>(age_grid_mask_quad_tree_node),
								static_cast<bool>(normal_map_quad_tree_node),
								true/*active_polygons*/),
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
						clip_cube_subdivision_cache_node,
						true/*active_polygons*/);

		// Apply the necessary tile state.
		// Note that the plate rotation is the identity rotation since we are not reconstructing.
		apply_tile_state(
				renderer,
				render_scene_tile_draw_state,
				GPlatesMaths::UnitQuaternion3D::create_identity_rotation());

		// Draw the cube mesh covering the current quad tree node tile.
		cube_mesh_quad_tree_node->render_mesh_drawable(renderer);

		return;
	}

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
	if (!render_traversal_cube_quad_tree_node.get_element().scene_tile_draw_state)
	{
		// Get the tile textures (source texture and optionally age grid and normal map).
		boost::optional<GLTexture::shared_ptr_to_const_type> source_raster_texture;
		boost::optional<GLTexture::shared_ptr_to_const_type> age_grid_mask_texture;
		boost::optional<GLTexture::shared_ptr_to_const_type> normal_map_texture;
		if (!get_tile_textures(
				renderer,
				source_raster_texture,
				age_grid_mask_texture,
				normal_map_texture,
				client_cache_cube_quad_tree_node,
				source_raster_quad_tree_node,
				age_grid_mask_quad_tree_node,
				normal_map_quad_tree_node))
		{
			// Note that this should never happen since we're using a 'GLMultiResolutionCubeRaster's.
			return;
		}

		//
		// Prepare for rendering the current scene tile.
		//
		// NOTE: This caching only happens within a render call (not across render frames).
		//

		const bool is_floating_point_source_raster =
				GLTexture::is_format_floating_point(d_source_raster->get_tile_texture_internal_format());

		if (age_grid_mask_quad_tree_node)
		{
			render_traversal_cube_quad_tree_node.get_element().scene_tile_draw_state =
					std::vector<RenderQuadTreeNode::TileDrawState>();

			// Now handle the active polygons first.
			bool active_polygons = true;

			// Determine which shader program to use (if any are supported) based on the configuration
			// of the current tile and lighting.
			boost::optional<ProgramObject> active_polygons_program_object =
					get_shader_program_for_tile(
							is_floating_point_source_raster,
							static_cast<bool>(age_grid_mask_quad_tree_node),
							static_cast<bool>(normal_map_quad_tree_node),
							active_polygons);
			// Draw state for *active* polygons...
			const RenderQuadTreeNode::TileDrawState active_polygons_draw_state =
					create_scene_tile_draw_state(
							renderer,
							active_polygons_program_object,
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
							clip_cube_subdivision_cache_node,
							active_polygons);
			// Cache the state.
			render_traversal_cube_quad_tree_node.get_element().scene_tile_draw_state->push_back(
					active_polygons_draw_state);

			// Now handle the inactive polygons.
			active_polygons = false;

			// Determine which shader program to use (if any are supported) based on the configuration
			// of the current tile and lighting.
			boost::optional<ProgramObject> inactive_polygons_program_object =
					get_shader_program_for_tile(
							is_floating_point_source_raster,
							static_cast<bool>(age_grid_mask_quad_tree_node),
							static_cast<bool>(normal_map_quad_tree_node),
							active_polygons);
			// Draw state for *inactive* polygons...
			const RenderQuadTreeNode::TileDrawState inactive_polygons_draw_state =
					create_scene_tile_draw_state(
							renderer,
							inactive_polygons_program_object,
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
							clip_cube_subdivision_cache_node,
							active_polygons);
			// Cache the state.
			render_traversal_cube_quad_tree_node.get_element().scene_tile_draw_state->push_back(
					inactive_polygons_draw_state);
		}
		else // not using age grid...
		{
			// Determine which shader program to use (if any are supported) based on the configuration
			// of the current tile and lighting.
			boost::optional<ProgramObject> program_object =
					get_shader_program_for_tile(
							is_floating_point_source_raster,
							static_cast<bool>(age_grid_mask_quad_tree_node),
							static_cast<bool>(normal_map_quad_tree_node),
							true/*active_polygons*/);
			// Draw state...
			const RenderQuadTreeNode::TileDrawState draw_state =
					create_scene_tile_draw_state(
							renderer,
							program_object,
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
							clip_cube_subdivision_cache_node,
							true/*active_polygons*/);
			// Cache the state.
			render_traversal_cube_quad_tree_node.get_element().scene_tile_draw_state =
					std::vector<RenderQuadTreeNode::TileDrawState>(1, draw_state);
		}
	}

	//
	// Render the current scene tile.
	//

	if (age_grid_mask_quad_tree_node)
	{
		// We render using active *and* inactive reconstructed polygons because the age grid
		// decides the begin time of oceanic crust not the polygons.
		// We render the active reconstructed polygons first followed by the inactive.
		// For the active polygons we render the source texture where the age grid coverage is zero and
		// where both the age grid coverage *and* mask is one - here the polygon begin times are respected.
		// For the inactive polygons we only render the source texture where the age grid coverage *and*
		// mask is one.
		render_tile_polygon_drawables(
				renderer,
				render_traversal_cube_quad_tree_node.get_element().scene_tile_draw_state->at(0),
				reconstructed_polygon_mesh_transform_group->get_finite_rotation(),
				reconstructed_polygon_mesh_transform_group->get_visible_present_day_polygon_meshes_for_active_reconstructions(),
				polygon_mesh_node_intersections.get(),
				intersections_quad_tree_node.get(),
				polygon_mesh_drawables.get());
		render_tile_polygon_drawables(
				renderer,
				render_traversal_cube_quad_tree_node.get_element().scene_tile_draw_state->at(1),
				reconstructed_polygon_mesh_transform_group->get_finite_rotation(),
				reconstructed_polygon_mesh_transform_group->get_visible_present_day_polygon_meshes_for_inactive_reconstructions(),
				polygon_mesh_node_intersections.get(),
				intersections_quad_tree_node.get(),
				polygon_mesh_drawables.get());
	}
	else // not using age grid...
	{
		render_tile_polygon_drawables(
				renderer,
				render_traversal_cube_quad_tree_node.get_element().scene_tile_draw_state->front(),
				reconstructed_polygon_mesh_transform_group->get_finite_rotation(),
				reconstructed_polygon_mesh_transform_group->get_visible_present_day_polygon_meshes_for_active_reconstructions(),
				polygon_mesh_node_intersections.get(),
				intersections_quad_tree_node.get(),
				polygon_mesh_drawables.get());
	}
}


bool
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::get_tile_textures(
		GLRenderer &renderer,
		boost::optional<GLTexture::shared_ptr_to_const_type> &source_raster_texture,
		boost::optional<GLTexture::shared_ptr_to_const_type> &age_grid_mask_texture,
		boost::optional<GLTexture::shared_ptr_to_const_type> &normal_map_texture,
		client_cache_cube_quad_tree_type::node_type &client_cache_cube_quad_tree_node,
		const source_raster_quad_tree_node_type &source_raster_quad_tree_node,
		const boost::optional<age_grid_mask_quad_tree_node_type> &age_grid_mask_quad_tree_node,
		const boost::optional<age_grid_mask_quad_tree_node_type> &normal_map_quad_tree_node)
{
	// Get the source raster texture.
	// Since it's a cube texture it may, in turn, have to render its source raster
	// into its texture (which it then passes to us to use).
	GLMultiResolutionCubeRaster::cache_handle_type source_raster_cache_handle;
	source_raster_texture = source_raster_quad_tree_node.get_tile_texture(renderer, source_raster_cache_handle);
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
				age_grid_mask_quad_tree_node->get_tile_texture(renderer, age_grid_mask_cache_handle);
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
				normal_map_quad_tree_node->get_tile_texture(renderer, normal_map_cache_handle);
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


boost::optional<GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::ProgramObject>
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::get_shader_program_for_tile(
		bool source_raster_is_floating_point,
		bool using_age_grid_tile,
		bool using_normal_map_tile,
		bool active_polygons)
{
	//
	// Determine which shader program to use or return boost::none to use fixed-function pipeline.
	//
	// This effectively mirrors the creation of shader programs in 'create_shader_programs()'.
	//

	if (source_raster_is_floating_point)
	{
		if (using_age_grid_tile)
		{
			return active_polygons
					? d_render_floating_point_with_age_grid_with_active_polygons_program_object
					: d_render_floating_point_with_age_grid_with_inactive_polygons_program_object;
		}

		// No age grid on current tile...
		return d_render_floating_point_program_object;
	}

	// Source raster is fixed-point ...

	if (d_light)
	{
		// The surface lighting depends on whether in a 3D globe view or a 2D map view.
		const ViewType view = d_light.get()->get_map_projection() ? MAP_VIEW : GLOBE_VIEW;

		if (using_normal_map_tile)
		{
			// Note that we'll only get a normal map texture if surface relief lighting is enabled
			// but we'll check for both surface lighting and a normal map texture just to be certain.
			if (d_light.get()->get_scene_lighting_parameters().is_lighting_enabled(
				GPlatesGui::SceneLightingParameters::LIGHTING_RASTER))
			{
				if (using_age_grid_tile)
				{
					return active_polygons
							? d_render_fixed_point_with_age_grid_with_active_polygons_with_normal_map_with_surface_lighting_program_object[view]
							: d_render_fixed_point_with_age_grid_with_inactive_polygons_with_normal_map_with_surface_lighting_program_object[view];
				}

				return d_render_fixed_point_with_normal_map_with_surface_lighting_program_object[view];
			}

			// No surface relief *directional* lighting...

			if (using_age_grid_tile)
			{
				return active_polygons
						? d_render_fixed_point_with_age_grid_with_active_polygons_with_normal_map_program_object[view]
						: d_render_fixed_point_with_age_grid_with_inactive_polygons_with_normal_map_program_object[view];
			}

			return d_render_fixed_point_with_normal_map_program_object[view];
		}

		// No normal map...

		if (d_light.get()->get_scene_lighting_parameters().is_lighting_enabled(
				GPlatesGui::SceneLightingParameters::LIGHTING_RASTER))
		{
			if (using_age_grid_tile)
			{
				return active_polygons
						? d_render_fixed_point_with_age_grid_with_active_polygons_with_surface_lighting_program_object[view]
						: d_render_fixed_point_with_age_grid_with_inactive_polygons_with_surface_lighting_program_object[view];
			}

			return d_render_fixed_point_with_surface_lighting_program_object[view];
		}
	}

	// Not using any surface lighting...

	if (using_age_grid_tile)
	{
		return active_polygons
				? d_render_fixed_point_with_age_grid_with_active_polygons_program_object
				: d_render_fixed_point_with_age_grid_with_inactive_polygons_program_object;
	}

	// Neither age grid nor surface lighting applied to the current tile ...
	return d_render_fixed_point_program_object;
}


GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::RenderQuadTreeNode::TileDrawState
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::create_scene_tile_draw_state(
		GLRenderer &renderer,
		boost::optional<ProgramObject> render_tile_to_scene_program_object,
		const GLTexture::shared_ptr_to_const_type &source_raster_tile_texture,
		const GLUtils::QuadTreeUVTransform &source_raster_uv_transform,
		boost::optional<GLTexture::shared_ptr_to_const_type> age_grid_tile_texture,
		boost::optional<const GLUtils::QuadTreeUVTransform &> age_grid_uv_transform,
		boost::optional<GLTexture::shared_ptr_to_const_type> normal_map_tile_texture,
		boost::optional<const GLUtils::QuadTreeUVTransform &> normal_map_uv_transform,
		source_raster_cube_subdivision_cache_type &source_raster_cube_subdivision_cache,
		const source_raster_cube_subdivision_cache_type::node_reference_type &source_raster_cube_subdivision_cache_node,
		boost::optional<age_grid_cube_subdivision_cache_type::non_null_ptr_type> &age_grid_cube_subdivision_cache,
		const boost::optional<age_grid_cube_subdivision_cache_type::node_reference_type> &age_grid_cube_subdivision_cache_node,
		boost::optional<age_grid_cube_subdivision_cache_type::non_null_ptr_type> &normal_map_cube_subdivision_cache,
		const boost::optional<age_grid_cube_subdivision_cache_type::node_reference_type> &normal_map_cube_subdivision_cache_node,
		clip_cube_subdivision_cache_type &clip_cube_subdivision_cache,
		const clip_cube_subdivision_cache_type::node_reference_type &clip_cube_subdivision_cache_node,
		bool active_polygons)
{
	//PROFILE_FUNC();

	// Create an empty compiled draw state to start off with.
	GLCompiledDrawState::non_null_ptr_type compiled_draw_state = renderer.create_empty_compiled_draw_state();

	// NOTE: If OpenGL requirements are not supported then return an empty compiled draw state that sets no state.
	// The 'is_supported()' method should have been called to prevent us from getting here though.
	if (!is_supported(renderer))
	{
		return RenderQuadTreeNode::TileDrawState(compiled_draw_state);
	}

	// The tile draw state includes the compiled state (calls to GLRenderer) and the shader program state.
	RenderQuadTreeNode::TileDrawState tile_draw_state(compiled_draw_state, render_tile_to_scene_program_object);

	// Start compiling the scene tile state.
	// Compiled state is any call to GLRenderer.
	GLRenderer::CompileDrawStateScope compile_draw_state_scope(renderer, compiled_draw_state);

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

	// If using an age grid then it will need a different projection transform due to it, most likely, having
	// a different tile dimension than the source raster (and hence different half-texel expanded frustum).
	GLMatrix age_grid_tile_texture_matrix;
	if (age_grid_tile_texture)
	{
		const GLTransform::non_null_ptr_to_const_type age_grid_projection_transform =
				age_grid_cube_subdivision_cache.get()->get_projection_transform(
						age_grid_cube_subdivision_cache_node.get());

		// Used to transform texture coordinates to account for partial coverage of current age grid tile.
		// This can happen if we are rendering a source raster that is higher-resolution than the age grid.
		age_grid_uv_transform->inverse_transform(age_grid_tile_texture_matrix);
		age_grid_tile_texture_matrix.gl_mult_matrix(GLUtils::get_clip_space_to_texture_space_transform());
		// Set up the texture matrix to perform model-view and projection transforms of the frustum.
		age_grid_tile_texture_matrix.gl_mult_matrix(age_grid_projection_transform->get_matrix());
		age_grid_tile_texture_matrix.gl_mult_matrix(view_transform->get_matrix());
	}

	// If using a normal map then it will need a different projection transform due to it, most likely, having
	// a different tile dimension than the source raster (and hence different half-texel expanded frustum).
	GLMatrix normal_map_tile_texture_matrix;
	if (normal_map_tile_texture)
	{
		const GLTransform::non_null_ptr_to_const_type normal_map_projection_transform =
				normal_map_cube_subdivision_cache.get()->get_projection_transform(
						normal_map_cube_subdivision_cache_node.get());

		// Used to transform texture coordinates to account for partial coverage of current normal map tile.
		// This can happen if we are rendering a source raster that is higher-resolution than the normal map.
		normal_map_uv_transform->inverse_transform(normal_map_tile_texture_matrix);
		normal_map_tile_texture_matrix.gl_mult_matrix(GLUtils::get_clip_space_to_texture_space_transform());
		// Set up the texture matrix to perform model-view and projection transforms of the frustum.
		normal_map_tile_texture_matrix.gl_mult_matrix(normal_map_projection_transform->get_matrix());
		normal_map_tile_texture_matrix.gl_mult_matrix(view_transform->get_matrix());
	}

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

	// Use shader program (if supported), otherwise the fixed-function pipeline.
	if (render_tile_to_scene_program_object)
	{
		// Bind the shader program.
		renderer.gl_bind_program_object(render_tile_to_scene_program_object->shader_program_object);
	}

	unsigned int current_texture_unit = 0;

	//
	// Age grid texture
	//

	// If using an age grid for the current tile...
	if (age_grid_tile_texture)
	{
		// Only need one texture unit for shader programs to access age mask and coverage together.
		// Fixed function pipeline requires two units though.
		if (render_tile_to_scene_program_object)
		{
			if (render_tile_to_scene_program_object->uniforms.uses_age_grid_texture_transform)
			{
				// Load the age grid texture transform.
				tile_draw_state.age_grid_texture_transform = age_grid_tile_texture_matrix;
			}

			if (render_tile_to_scene_program_object->uniforms.uses_age_grid_texture_sampler)
			{
				// Bind the age grid texture to the current texture unit.
				renderer.gl_bind_texture(age_grid_tile_texture.get(), GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_2D);

				// Set the age grid texture sampler to the current texture unit.
				tile_draw_state.age_grid_texture_sampler = current_texture_unit;

				// Move to the next texture unit.
				++current_texture_unit;
			}

			if (render_tile_to_scene_program_object->uniforms.uses_reconstruction_time)
			{
				// If we're generating the age mask in the shader program then it needs to know the reconstruction time.
				tile_draw_state.reconstruction_time = d_reconstruction_time;
			}
		}
		else // Fixed function...
		{
			//
			// Age grid mask texture (to access mask and copy from RGB to Alpha channel)
			//

			// Load texture transform into the current texture unit.
			renderer.gl_load_texture_matrix(GL_TEXTURE0 + current_texture_unit, age_grid_tile_texture_matrix);
			// Bind the age grid mask texture to the current texture unit.
			renderer.gl_bind_texture(age_grid_tile_texture.get(), GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_2D);
			// Enable texturing and set the texture function on the current texture unit.
			renderer.gl_enable_texture(GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_2D);
			// We only need to set parameters that are not default parameters...
			// Use dot3 to convert RGB(1,*,*) to RGBA(1,1,1,1) or RGB(0,*,*) to RGBA(0,0,0,0).
			// This gets the red channel into the alpha channel (we need it there for alpha-testing).
			//
			// NOTE: This only works for extracting a value that's either 0.0 or 1.0 so nearest neighbour
			// filtering with no anisotropic should be used to prevent a value between 0 and 1.
			static const std::vector<GLfloat> dot3_extract_red_channel = create_dot3_extract_red_channel();
			renderer.gl_tex_env(GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
			renderer.gl_tex_env(GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_DOT3_RGBA_ARB);
			renderer.gl_tex_env(GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
			renderer.gl_tex_env(GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_CONSTANT_ARB);
			renderer.gl_tex_env(GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, dot3_extract_red_channel);
			// The alpha channel is ignored since using GL_DOT3_RGBA_ARB instead of GL_DOT3_RGB_ARB.
			// Set up texture coordinate generation from the vertices (x,y,z) on the current texture unit.
			GLUtils::set_object_linear_tex_gen_state(renderer, current_texture_unit);

			// Move to the next texture unit.
			++current_texture_unit;

			//
			// Age grid mask texture (to access mask coverage)
			//

			// Load texture transform into the current texture unit.
			renderer.gl_load_texture_matrix(GL_TEXTURE0 + current_texture_unit, age_grid_tile_texture_matrix);
			// Bind the age grid mask texture to the current texture unit.
			renderer.gl_bind_texture(age_grid_tile_texture.get(), GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_2D);
			// Enable texturing and set the texture function on the current texture unit.
			renderer.gl_enable_texture(GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_2D);
			renderer.gl_tex_env(GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
			// For RGB channels set to (1,1,1).
			renderer.gl_tex_env(GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_REPLACE);
			renderer.gl_tex_env(GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PRIMARY_COLOR_ARB);
			if (active_polygons)
			{
				// Interpolate with the age grid mask in the previous texture unit (the age grid coverage interpolates).
				// For alpha channel we interpolate (1-Ac) + Ac * Am where Ac is coverage and Am is mask.
				// INTERPOLATE_ARB --> Arg0 * (Arg2) + Arg1 * (1-Arg2)
				// We only need to set parameters that are not default parameters...
				renderer.gl_tex_env(GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_INTERPOLATE_ARB);
				renderer.gl_tex_env(GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS_ARB);
				renderer.gl_tex_env(GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_PRIMARY_COLOR_ARB);
				renderer.gl_tex_env(GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_ENV, GL_SOURCE2_ALPHA_ARB, GL_TEXTURE);
			}
			else // inactive polygons...
			{
				// Just multiple the age grid coverage with the age grid mask.
				renderer.gl_tex_env(GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_MODULATE);
				renderer.gl_tex_env(GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
				renderer.gl_tex_env(GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_PREVIOUS_ARB);
			}
			// Set up texture coordinate generation from the vertices (x,y,z) on the current texture unit.
			GLUtils::set_object_linear_tex_gen_state(renderer, current_texture_unit);

			// Move to the next texture unit.
			++current_texture_unit;
		}
	}

	//
	// Surface lighting and normal map raster texture
	//

	// There's no fixed-function pipeline fallback for surface lighting.
	// If the shader program is not available on runtime system then fixed-function pipeline
	// will be used *without* surface lighting.
	if (render_tile_to_scene_program_object)
	{
		if (render_tile_to_scene_program_object->uniforms.uses_normal_map_texture_transform)
		{
			// Load the normal map texture transform.
			tile_draw_state.normal_map_texture_transform = normal_map_tile_texture_matrix;
		}

		if (render_tile_to_scene_program_object->uniforms.uses_normal_map_texture_sampler)
		{
			// Bind the normal map texture to the current texture unit.
			renderer.gl_bind_texture(normal_map_tile_texture.get(), GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_2D);

			// Set the normal map texture sampler to the current texture unit.
			tile_draw_state.normal_map_texture_sampler = current_texture_unit;

			// Move to the next texture unit.
			++current_texture_unit;
		}

		if (render_tile_to_scene_program_object->uniforms.uses_world_space_light_direction)
		{
			// Set the world space light direction.
			tile_draw_state.world_space_light_direction =
					d_light.get()->get_globe_view_light_direction(renderer);
		}

		if (render_tile_to_scene_program_object->uniforms.uses_light_direction_cube_texture_sampler)
		{
			// Bind the light direction cube texture to the current texture unit.
			renderer.gl_bind_texture(
					d_light.get()->get_map_view_light_direction_cube_map_texture(renderer),
					GL_TEXTURE0 + current_texture_unit,
					GL_TEXTURE_CUBE_MAP_ARB);

			// Set the light direction cube texture sampler to the current texture unit.
			tile_draw_state.light_direction_cube_texture_sampler = current_texture_unit;

			// Move to the next texture unit.
			++current_texture_unit;
		}

		if (render_tile_to_scene_program_object->uniforms.uses_ambient_and_diffuse_lighting)
		{
			tile_draw_state.ambient_and_diffuse_lighting =
					d_light.get()->get_map_view_constant_lighting(renderer);
		}

		if (render_tile_to_scene_program_object->uniforms.uses_light_ambient_contribution)
		{
			// Set the ambient light contribution.
			tile_draw_state.light_ambient_contribution =
					d_light.get()->get_scene_lighting_parameters().get_ambient_light_contribution();
		}
	}

	//
	// Source raster texture
	//

	if (render_tile_to_scene_program_object)
	{
		if (render_tile_to_scene_program_object->uniforms.uses_source_raster_texture_transform)
		{
			// Load the source raster texture transform.
			tile_draw_state.source_raster_texture_transform = source_raster_tile_texture_matrix;
		}

		if (render_tile_to_scene_program_object->uniforms.uses_source_texture_sampler)
		{
			// Bind the source raster tile texture to the current texture unit.
			renderer.gl_bind_texture(source_raster_tile_texture, GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_2D);

			// Set the source tile texture sampler to the current texture unit.
			tile_draw_state.source_texture_sampler = current_texture_unit;

			// Move to the next texture unit.
			++current_texture_unit;
		}
	}
	else // Fixed function...
	{
		// Load texture transform into the current texture unit.
		renderer.gl_load_texture_matrix(GL_TEXTURE0 + current_texture_unit, source_raster_tile_texture_matrix);
		// Bind the source raster tile texture to the current texture unit.
		renderer.gl_bind_texture(source_raster_tile_texture, GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_2D);
		// Enable texturing and set the texture function on the current texture unit.
		renderer.gl_enable_texture(GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_2D);
		// Modulate with the age grid mask (if using age grid)...
		if (age_grid_tile_texture)
		{
			// NOTE: All raster data has pre-multiplied alpha (for alpha-blending to render targets).
			// The source raster already has pre-multiplied alpha (see GLVisualRasterSource).
			// However we still need to pre-multiply the age mask (alpha).
			// (RGB * A, A)  ->  (RGB * A * age_mask, A * age_mask).
			renderer.gl_tex_env(GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
			// RGB * A  ->  RGB * A * age_mask
			renderer.gl_tex_env(GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
			renderer.gl_tex_env(GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
			renderer.gl_tex_env(GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
			renderer.gl_tex_env(GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB);
			renderer.gl_tex_env(GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_ALPHA);
			// A  ->  A * age_mask
			renderer.gl_tex_env(GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_MODULATE);
			renderer.gl_tex_env(GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
			renderer.gl_tex_env(GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_PREVIOUS_ARB);
		}
		else
		{
			renderer.gl_tex_env(GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		}
		// Set up texture coordinate generation from the vertices (x,y,z) on the current texture unit.
		GLUtils::set_object_linear_tex_gen_state(renderer, current_texture_unit);

		// Move to the next texture unit.
		++current_texture_unit;
	}

	//
	// Clip texture
	//

	// NOTE: For reconstructed static polygon meshes we always need clipping, but when there's
	// no reconstruction we then use GLMultiResolutionCubeMesh which only requires clipping when
	// we reach (zoom into) the deepest levels of its cube quad tree (because otherwise it can
	// draw a mesh exactly within the current quad tree node's boundary).
	// However, in order to keep things simple (in this code and in the shader programs) we will
	// leave clipping on always.

	if (render_tile_to_scene_program_object)
	{
		if (render_tile_to_scene_program_object->uniforms.uses_clip_texture_transform)
		{
			// Load the clip texture transform.
			tile_draw_state.clip_texture_transform = clip_texture_matrix;
		}

		if (render_tile_to_scene_program_object->uniforms.uses_clip_texture_sampler)
		{
			// Bind the clip texture to the current texture unit.
			renderer.gl_bind_texture(d_xy_clip_texture, GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_2D);

			// Set the clip texture sampler to the current texture unit.
			tile_draw_state.clip_texture_sampler = current_texture_unit;

			// Move to the next texture unit.
			++current_texture_unit;
		}
	}
	else // Fixed function...
	{
		// Load texture transform into the current texture unit.
		renderer.gl_load_texture_matrix(GL_TEXTURE0 + current_texture_unit, clip_texture_matrix);
		// Bind the clip texture to the current texture unit.
		renderer.gl_bind_texture(d_xy_clip_texture, GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_2D);
		// Enable texturing and set the texture function on the current texture unit.
		renderer.gl_enable_texture(GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_2D);
		renderer.gl_tex_env(GL_TEXTURE0 + current_texture_unit, GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		// Set up texture coordinate generation from the vertices (x,y,z) on the current texture unit.
		GLUtils::set_object_linear_tex_gen_state(renderer, current_texture_unit);

		// Move to the next texture unit.
		++current_texture_unit;
	}

	if (render_tile_to_scene_program_object)
	{
		// We don't need to enable alpha-testing because the fragment shader uses 'discard' instead.
	}
	else // Fixed function...
	{
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

#if 0
	// Used to render as wire-frame meshes instead of filled textured meshes for
	// visualising mesh density.
	renderer.gl_polygon_mode(GL_FRONT_AND_BACK, GL_LINE);
#endif

	return tile_draw_state;
}


void
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::render_tile_polygon_drawables(
		GLRenderer &renderer,
		const RenderQuadTreeNode::TileDrawState &render_scene_tile_draw_state,
		const GPlatesMaths::UnitQuaternion3D &reconstructed_polygon_mesh_rotation,
		const present_day_polygon_mesh_membership_type &reconstructed_polygon_mesh_membership,
		const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
		const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
		const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables)
{
	//PROFILE_FUNC();

	// Make sure we leave the OpenGL state the way it was.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Apply the necessary tile state.
	apply_tile_state(renderer, render_scene_tile_draw_state, reconstructed_polygon_mesh_rotation);

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
			renderer.apply_compiled_draw_state(*polygon_mesh_drawable->drawable);
		}
	}
}


void
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::apply_tile_state(
		GLRenderer &renderer,
		const RenderQuadTreeNode::TileDrawState &tile_draw_state,
		const GPlatesMaths::UnitQuaternion3D &plate_rotation)
{
	// Apply the pre-compiled draw state for the current tile.
	// This state is used by all rotation groups visiting this tile.
	renderer.apply_compiled_draw_state(*tile_draw_state.compiled_draw_state);

	// Set any shader program object state associated with the current tile.
	// This state is not compiled into the compiled draw state because that only compiles calls to GLRenderer.
	// And also a shader program object is shared across multiple tiles.
	apply_tile_shader_program_state(renderer, tile_draw_state);

	// If scene lighting is enabled then we need to apply some non-cached draw state.
	// This state was not cached with the tile because it varies with rotation group and hence
	// changes as each rotation group visits the same tile.
	if (tile_draw_state.program_object &&
		tile_draw_state.program_object->uniforms.uses_plate_rotation_quaternion)
	{
		// Set the plate rotation of the current rotation group.
		tile_draw_state.program_object->shader_program_object->gl_uniform4f(
				renderer,
				"plate_rotation_quaternion",
				plate_rotation);
	}
}


void
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::apply_tile_shader_program_state(
		GLRenderer &renderer,
		const RenderQuadTreeNode::TileDrawState &tile_draw_state)
{
	// Return early if no shader program to apply state to.
	if (!tile_draw_state.program_object)
	{
		return;
	}

	const GLProgramObject::shared_ptr_type tile_program_object =
			tile_draw_state.program_object->shader_program_object;

	// Note: We only set the tile state here.
	// Any state that varies with the rotation group is not cached in the tile and is handled
	// in 'apply_tile_state()'.

	// Load the source raster texture transform (if specified).
	if (tile_draw_state.source_raster_texture_transform)
	{
		tile_program_object->gl_uniform_matrix4x4f(
				renderer,
				"source_raster_texture_transform",
				tile_draw_state.source_raster_texture_transform.get());
	}
	// Set the source raster texture sampler.
	if (tile_draw_state.source_texture_sampler)
	{
		tile_program_object->gl_uniform1i(
				renderer,
				"source_texture_sampler",
				tile_draw_state.source_texture_sampler.get());
	}

	// Load the clip texture transform (if specified).
	if (tile_draw_state.clip_texture_transform)
	{
		tile_program_object->gl_uniform_matrix4x4f(
				renderer,
				"clip_texture_transform",
				tile_draw_state.clip_texture_transform.get());
	}
	// Set the clip texture sampler.
	if (tile_draw_state.clip_texture_sampler)
	{
		tile_program_object->gl_uniform1i(
				renderer,
				"clip_texture_sampler",
				tile_draw_state.clip_texture_sampler.get());
	}

	// Load the age grid texture transform (if specified).
	if (tile_draw_state.age_grid_texture_transform)
	{
		tile_program_object->gl_uniform_matrix4x4f(
				renderer,
				"age_grid_texture_transform",
				tile_draw_state.age_grid_texture_transform.get());
	}
	// Set the age grid texture sampler.
	if (tile_draw_state.age_grid_texture_sampler)
	{
		tile_program_object->gl_uniform1i(
				renderer,
				"age_grid_texture_sampler",
				tile_draw_state.age_grid_texture_sampler.get());
	}
	// Set the reconstruction time for the age mask generation.
	if (tile_draw_state.reconstruction_time)
	{
		tile_program_object->gl_uniform1f(
				renderer,
				"reconstruction_time",
				tile_draw_state.reconstruction_time.get());
	}

	// Load the normal map texture transform (if specified).
	if (tile_draw_state.normal_map_texture_transform)
	{
		tile_program_object->gl_uniform_matrix4x4f(
				renderer,
				"normal_map_texture_transform",
				tile_draw_state.normal_map_texture_transform.get());
	}
	// Set the normal map texture sampler.
	if (tile_draw_state.normal_map_texture_sampler)
	{
		tile_program_object->gl_uniform1i(
				renderer,
				"normal_map_texture_sampler",
				tile_draw_state.normal_map_texture_sampler.get());
	}

	// Set the (ambient+diffuse) lighting.
	if (tile_draw_state.ambient_and_diffuse_lighting)
	{
		tile_program_object->gl_uniform1f(
				renderer,
				"ambient_and_diffuse_lighting",
				tile_draw_state.ambient_and_diffuse_lighting.get());
	}

	// Set the ambient light contribution.
	if (tile_draw_state.light_ambient_contribution)
	{
		tile_program_object->gl_uniform1f(
				renderer,
				"light_ambient_contribution",
				tile_draw_state.light_ambient_contribution.get());
	}
	// Set the world space light direction.
	if (tile_draw_state.world_space_light_direction)
	{
		tile_program_object->gl_uniform3f(
				renderer,
				"world_space_light_direction",
				tile_draw_state.world_space_light_direction.get());
	}
	// Set the light direction cube texture sampler.
	if (tile_draw_state.light_direction_cube_texture_sampler)
	{
		tile_program_object->gl_uniform1i(
				renderer,
				"light_direction_cube_texture_sampler",
				tile_draw_state.light_direction_cube_texture_sampler.get());
	}
}


void
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::create_shader_programs(
		GLRenderer &renderer)
{
	// If shader programs are supported then use them to render the raster tile.

	const bool is_floating_point_source_raster =
			GLTexture::is_format_floating_point(d_source_raster->get_tile_texture_internal_format());

	// If the source raster is floating-point (ie, not coloured as fixed-point for visual display)
	// then use a shader program instead of the fixed-function pipeline.
	if (is_floating_point_source_raster)
	{
		// We shouldn't have a floating-point raster if it's not supported (the client should check).
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				supports_floating_point_source_raster(renderer),
				GPLATES_ASSERTION_SOURCE);

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

		d_render_floating_point_program_object =
				create_shader_program(
						renderer,
						true/*define_source_raster_is_floating_point*/,
						false/*define_using_age_grid*/,
						false/*define_generate_age_mask*/,
						false/*define_active_polygons*/,
						false/*define_surface_lighting*/,
						false/*define_using_normal_map*/,
						false/*define_no_directional_light_for_normal_maps*/,
						false/*define_map_view*/);
		// If cannot compile program object then will resort to fixed-function pipeline.
		if (!d_render_floating_point_program_object)
		{
			qDebug() << "Unable to compile shader program to reconstruct floating-point raster.\n"
				"  Clamping of values to [0,1] range may result.";
		}

		// If using an age grid...
		if (d_age_grid_cube_raster)
		{
			d_render_floating_point_with_age_grid_with_active_polygons_program_object =
					create_shader_program(
							renderer,
							true/*define_source_raster_is_floating_point*/,
							true/*define_using_age_grid*/,
							true/*define_generate_age_mask*/,
							true/*define_active_polygons*/,
							false/*define_surface_lighting*/,
							false/*define_using_normal_map*/,
							false/*define_no_directional_light_for_normal_maps*/,
							false/*define_map_view*/);
			d_render_floating_point_with_age_grid_with_inactive_polygons_program_object =
					create_shader_program(
							renderer,
							true/*define_source_raster_is_floating_point*/,
							true/*define_using_age_grid*/,
							true/*define_generate_age_mask*/,
							false/*define_active_polygons*/,
							false/*define_surface_lighting*/,
							false/*define_using_normal_map*/,
							false/*define_no_directional_light_for_normal_maps*/,
							false/*define_map_view*/);
			// If cannot compile program object then will resort to fixed-function pipeline.
			if (!d_render_floating_point_with_age_grid_with_active_polygons_program_object ||
				!d_render_floating_point_with_age_grid_with_inactive_polygons_program_object)
			{
				qDebug() << "Unable to compile shader program to reconstruct floating-point raster with age grid.\n"
					"  Clamping of values to [0,1] range may result.";
			}
		}

		// No surface lighting gets applied to floating-point rasters - they are not for visualisation.

	}
	else // a fixed-point source raster ...
	{
		// If cannot compile then will resort to fixed-function pipeline without any loss.
		d_render_fixed_point_program_object = create_shader_program(
				renderer,
				false/*define_source_raster_is_floating_point*/,
				false/*define_using_age_grid*/,
				false/*define_generate_age_mask*/,
				false/*define_active_polygons*/,
				false/*define_surface_lighting*/,
				false/*define_using_normal_map*/,
				false/*define_no_directional_light_for_normal_maps*/,
				false/*define_map_view*/);

		// For rendering with surface lighting (but without a normal map).
		d_render_fixed_point_with_surface_lighting_program_object[GLOBE_VIEW] =
				create_shader_program(
						renderer,
						false/*define_source_raster_is_floating_point*/,
						false/*define_using_age_grid*/,
						false/*define_generate_age_mask*/,
						false/*define_active_polygons*/,
						true/*define_surface_lighting*/,
						false/*define_using_normal_map*/,
						false/*define_no_directional_light_for_normal_maps*/,
						false/*define_map_view*/);
		d_render_fixed_point_with_surface_lighting_program_object[MAP_VIEW] =
				create_shader_program(
						renderer,
						false/*define_source_raster_is_floating_point*/,
						false/*define_using_age_grid*/,
						false/*define_generate_age_mask*/,
						false/*define_active_polygons*/,
						true/*define_surface_lighting*/,
						false/*define_using_normal_map*/,
						false/*define_no_directional_light_for_normal_maps*/,
						true/*define_map_view*/);
		// If cannot compile program object then will resort to fixed-function pipeline.
		if (!d_render_fixed_point_with_surface_lighting_program_object[GLOBE_VIEW] ||
			!d_render_fixed_point_with_surface_lighting_program_object[MAP_VIEW])
		{
			qDebug() << "Unable to compile shader program to reconstruct raster with surface lighting.\n"
				"  Raster will lack surface lighting.";
		}

		// If using an age grid...
		if (d_age_grid_cube_raster)
		{
			// If 'supports_age_mask_generation()' is true then this will not fail to compile.
			// If 'supports_age_mask_generation()' is false then this can fail to compile in which
			// case we'll resort to fixed-function pipeline without any loss (using GLAgeGridMaskSource).
			d_render_fixed_point_with_age_grid_with_active_polygons_program_object =
					create_shader_program(
							renderer,
							false/*define_source_raster_is_floating_point*/,
							true/*define_using_age_grid*/,
							true/*define_generate_age_mask*/,
							true/*define_active_polygons*/,
							false/*define_surface_lighting*/,
							false/*define_using_normal_map*/,
							false/*define_no_directional_light_for_normal_maps*/,
							false/*define_map_view*/);
			d_render_fixed_point_with_age_grid_with_inactive_polygons_program_object =
					create_shader_program(
							renderer,
							false/*define_source_raster_is_floating_point*/,
							true/*define_using_age_grid*/,
							true/*define_generate_age_mask*/,
							false/*define_active_polygons*/,
							false/*define_surface_lighting*/,
							false/*define_using_normal_map*/,
							false/*define_no_directional_light_for_normal_maps*/,
							false/*define_map_view*/);

			// For rendering with age grid and surface lighting (but without a normal map).
			//
			// If 'supports_age_mask_generation()' is true then this will not fail to compile.
			// If 'supports_age_mask_generation()' is false then this can fail to compile in which
			// case we'll resort to fixed-function pipeline but will lose surface lighting.
			d_render_fixed_point_with_age_grid_with_active_polygons_with_surface_lighting_program_object[GLOBE_VIEW] =
					create_shader_program(
							renderer,
							false/*define_source_raster_is_floating_point*/,
							true/*define_using_age_grid*/,
							true/*define_generate_age_mask*/,
							true/*define_active_polygons*/,
							true/*define_surface_lighting*/,
							false/*define_using_normal_map*/,
							false/*define_no_directional_light_for_normal_maps*/,
							false/*define_map_view*/);
			d_render_fixed_point_with_age_grid_with_active_polygons_with_surface_lighting_program_object[MAP_VIEW] =
					create_shader_program(
							renderer,
							false/*define_source_raster_is_floating_point*/,
							true/*define_using_age_grid*/,
							true/*define_generate_age_mask*/,
							true/*define_active_polygons*/,
							true/*define_surface_lighting*/,
							false/*define_using_normal_map*/,
							false/*define_no_directional_light_for_normal_maps*/,
							true/*define_map_view*/);
			d_render_fixed_point_with_age_grid_with_inactive_polygons_with_surface_lighting_program_object[GLOBE_VIEW] =
					create_shader_program(
							renderer,
							false/*define_source_raster_is_floating_point*/,
							true/*define_using_age_grid*/,
							true/*define_generate_age_mask*/,
							false/*define_active_polygons*/,
							true/*define_surface_lighting*/,
							false/*define_using_normal_map*/,
							false/*define_no_directional_light_for_normal_maps*/,
							false/*define_map_view*/);
			d_render_fixed_point_with_age_grid_with_inactive_polygons_with_surface_lighting_program_object[MAP_VIEW] =
					create_shader_program(
							renderer,
							false/*define_source_raster_is_floating_point*/,
							true/*define_using_age_grid*/,
							true/*define_generate_age_mask*/,
							false/*define_active_polygons*/,
							true/*define_surface_lighting*/,
							false/*define_using_normal_map*/,
							false/*define_no_directional_light_for_normal_maps*/,
							true/*define_map_view*/);
			// If cannot compile program object then will resort to fixed-function pipeline.
			if (!d_render_fixed_point_with_age_grid_with_active_polygons_with_surface_lighting_program_object[GLOBE_VIEW] ||
				!d_render_fixed_point_with_age_grid_with_active_polygons_with_surface_lighting_program_object[MAP_VIEW] ||
				!d_render_fixed_point_with_age_grid_with_inactive_polygons_with_surface_lighting_program_object[GLOBE_VIEW] ||
				!d_render_fixed_point_with_age_grid_with_inactive_polygons_with_surface_lighting_program_object[MAP_VIEW])
			{
				qDebug() << "Unable to compile shader program to reconstruct raster with age grid and surface lighting.\n"
					"  Raster will lack surface lighting.";
			}
		}

		// If using a normal map...
		if (d_normal_map_cube_raster)
		{
			// We shouldn't have a normal map if it's not supported (the client should check).
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					supports_normal_map(renderer),
					GPLATES_ASSERTION_SOURCE);

			// For rendering with a normal map.
			d_render_fixed_point_with_normal_map_program_object[GLOBE_VIEW] =
					create_shader_program(
							renderer,
							false/*define_source_raster_is_floating_point*/,
							false/*define_using_age_grid*/,
							false/*define_generate_age_mask*/,
							false/*define_active_polygons*/,
							true/*define_surface_lighting*/,
							true/*define_using_normal_map*/,
							true/*define_no_directional_light_for_normal_maps*/,
							false/*define_map_view*/);
			d_render_fixed_point_with_normal_map_program_object[MAP_VIEW] =
					create_shader_program(
							renderer,
							false/*define_source_raster_is_floating_point*/,
							false/*define_using_age_grid*/,
							false/*define_generate_age_mask*/,
							false/*define_active_polygons*/,
							true/*define_surface_lighting*/,
							true/*define_using_normal_map*/,
							true/*define_no_directional_light_for_normal_maps*/,
							true/*define_map_view*/);
			// Since 'supports_normal_map()' is true then should not fail to compile.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					d_render_fixed_point_with_normal_map_program_object[GLOBE_VIEW] &&
						d_render_fixed_point_with_normal_map_program_object[MAP_VIEW],
					GPLATES_ASSERTION_SOURCE);

			// For rendering surface lighting with a normal map.
			d_render_fixed_point_with_normal_map_with_surface_lighting_program_object[GLOBE_VIEW] =
					create_shader_program(
							renderer,
							false/*define_source_raster_is_floating_point*/,
							false/*define_using_age_grid*/,
							false/*define_generate_age_mask*/,
							false/*define_active_polygons*/,
							true/*define_surface_lighting*/,
							true/*define_using_normal_map*/,
							false/*define_no_directional_light_for_normal_maps*/,
							false/*define_map_view*/);
			d_render_fixed_point_with_normal_map_with_surface_lighting_program_object[MAP_VIEW] =
					create_shader_program(
							renderer,
							false/*define_source_raster_is_floating_point*/,
							false/*define_using_age_grid*/,
							false/*define_generate_age_mask*/,
							false/*define_active_polygons*/,
							true/*define_surface_lighting*/,
							true/*define_using_normal_map*/,
							false/*define_no_directional_light_for_normal_maps*/,
							true/*define_map_view*/);
			// Since 'supports_normal_map()' is true then should not fail to compile.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					d_render_fixed_point_with_normal_map_with_surface_lighting_program_object[GLOBE_VIEW] &&
						d_render_fixed_point_with_normal_map_with_surface_lighting_program_object[MAP_VIEW],
					GPLATES_ASSERTION_SOURCE);
		}

		// If using an age grid and a normal map...
		if (d_age_grid_cube_raster && d_normal_map_cube_raster)
		{
			// We shouldn't have a normal map if it's not supported (the client should check).
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					supports_normal_map(renderer),
					GPLATES_ASSERTION_SOURCE);

			// For rendering with age grid and with normal map.
			d_render_fixed_point_with_age_grid_with_active_polygons_with_normal_map_program_object[GLOBE_VIEW] =
					create_shader_program(
							renderer,
							false/*define_source_raster_is_floating_point*/,
							true/*define_using_age_grid*/,
							true/*define_generate_age_mask*/,
							true/*define_active_polygons*/,
							true/*define_surface_lighting*/,
							true/*define_using_normal_map*/,
							true/*define_no_directional_light_for_normal_maps*/,
							false/*define_map_view*/);
			d_render_fixed_point_with_age_grid_with_active_polygons_with_normal_map_program_object[MAP_VIEW] =
					create_shader_program(
							renderer,
							false/*define_source_raster_is_floating_point*/,
							true/*define_using_age_grid*/,
							true/*define_generate_age_mask*/,
							true/*define_active_polygons*/,
							true/*define_surface_lighting*/,
							true/*define_using_normal_map*/,
							true/*define_no_directional_light_for_normal_maps*/,
							true/*define_map_view*/);
			d_render_fixed_point_with_age_grid_with_inactive_polygons_with_normal_map_program_object[GLOBE_VIEW] =
					create_shader_program(
							renderer,
							false/*define_source_raster_is_floating_point*/,
							true/*define_using_age_grid*/,
							true/*define_generate_age_mask*/,
							false/*define_active_polygons*/,
							true/*define_surface_lighting*/,
							true/*define_using_normal_map*/,
							true/*define_no_directional_light_for_normal_maps*/,
							false/*define_map_view*/);
			d_render_fixed_point_with_age_grid_with_inactive_polygons_with_normal_map_program_object[MAP_VIEW] =
					create_shader_program(
							renderer,
							false/*define_source_raster_is_floating_point*/,
							true/*define_using_age_grid*/,
							true/*define_generate_age_mask*/,
							false/*define_active_polygons*/,
							true/*define_surface_lighting*/,
							true/*define_using_normal_map*/,
							true/*define_no_directional_light_for_normal_maps*/,
							true/*define_map_view*/);

			// For rendering with age grid and surface lighting with a normal map.
			d_render_fixed_point_with_age_grid_with_active_polygons_with_normal_map_with_surface_lighting_program_object[GLOBE_VIEW] =
					create_shader_program(
							renderer,
							false/*define_source_raster_is_floating_point*/,
							true/*define_using_age_grid*/,
							true/*define_generate_age_mask*/,
							true/*define_active_polygons*/,
							true/*define_surface_lighting*/,
							true/*define_using_normal_map*/,
							false/*define_no_directional_light_for_normal_maps*/,
							false/*define_map_view*/);
			d_render_fixed_point_with_age_grid_with_active_polygons_with_normal_map_with_surface_lighting_program_object[MAP_VIEW] =
					create_shader_program(
							renderer,
							false/*define_source_raster_is_floating_point*/,
							true/*define_using_age_grid*/,
							true/*define_generate_age_mask*/,
							true/*define_active_polygons*/,
							true/*define_surface_lighting*/,
							true/*define_using_normal_map*/,
							false/*define_no_directional_light_for_normal_maps*/,
							true/*define_map_view*/);
			d_render_fixed_point_with_age_grid_with_inactive_polygons_with_normal_map_with_surface_lighting_program_object[GLOBE_VIEW] =
					create_shader_program(
							renderer,
							false/*define_source_raster_is_floating_point*/,
							true/*define_using_age_grid*/,
							true/*define_generate_age_mask*/,
							false/*define_active_polygons*/,
							true/*define_surface_lighting*/,
							true/*define_using_normal_map*/,
							false/*define_no_directional_light_for_normal_maps*/,
							false/*define_map_view*/);
			d_render_fixed_point_with_age_grid_with_inactive_polygons_with_normal_map_with_surface_lighting_program_object[MAP_VIEW] =
					create_shader_program(
							renderer,
							false/*define_source_raster_is_floating_point*/,
							true/*define_using_age_grid*/,
							true/*define_generate_age_mask*/,
							false/*define_active_polygons*/,
							true/*define_surface_lighting*/,
							true/*define_using_normal_map*/,
							false/*define_no_directional_light_for_normal_maps*/,
							true/*define_map_view*/);

			// Since 'supports_normal_map()' is true then should not fail to compile.
			// Note that normal maps *with* age mask generation are only compiled if
			// 'supports_age_mask_generation()' returns true, otherwise age masking is still
			// used but it's just not as fast and not as good quality.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					d_render_fixed_point_with_age_grid_with_active_polygons_with_normal_map_program_object[GLOBE_VIEW] &&
						d_render_fixed_point_with_age_grid_with_active_polygons_with_normal_map_program_object[MAP_VIEW] &&
						d_render_fixed_point_with_age_grid_with_inactive_polygons_with_normal_map_program_object[GLOBE_VIEW] &&
						d_render_fixed_point_with_age_grid_with_inactive_polygons_with_normal_map_program_object[MAP_VIEW] &&
						d_render_fixed_point_with_age_grid_with_active_polygons_with_normal_map_with_surface_lighting_program_object[GLOBE_VIEW] &&
						d_render_fixed_point_with_age_grid_with_active_polygons_with_normal_map_with_surface_lighting_program_object[MAP_VIEW] &&
						d_render_fixed_point_with_age_grid_with_inactive_polygons_with_normal_map_with_surface_lighting_program_object[GLOBE_VIEW] &&
						d_render_fixed_point_with_age_grid_with_inactive_polygons_with_normal_map_with_surface_lighting_program_object[MAP_VIEW],
					GPLATES_ASSERTION_SOURCE);
		}
	}
}


boost::optional<GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::ProgramObject>
GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::create_shader_program(
		GLRenderer &renderer,
		bool define_source_raster_is_floating_point,
		bool define_using_age_grid,
		bool define_generate_age_mask,
		bool define_active_polygons,
		bool define_surface_lighting,
		bool define_using_normal_map,
		bool define_no_directional_light_for_normal_maps,
		bool define_map_view)
{
	// FIXME: A temporary prevention of normal map lighting in the 2D map views that does not
	// result in a constant light direction in the 2D map space. Eventually the light canvas tool
	// could support changing a lighting direction in 2D map space.
	if (define_surface_lighting && define_using_normal_map && define_map_view)
	{
		define_no_directional_light_for_normal_maps = true;
	}

	// Add the requested vertex/fragment shader defines.
	std::string vertex_and_fragment_shader_defines;
	if (define_source_raster_is_floating_point)
	{
		vertex_and_fragment_shader_defines += "#define SOURCE_RASTER_IS_FLOATING_POINT\n";
	}
	if (define_using_age_grid)
	{
		vertex_and_fragment_shader_defines += "#define USING_AGE_GRID\n";
	}
	if (define_generate_age_mask)
	{
		// Do we do the age comparison (with current reconstruction time) in the shader...
		if (supports_age_mask_generation(renderer))
		{
			vertex_and_fragment_shader_defines += "#define GENERATE_AGE_MASK\n";
		}
		else
		{
			define_generate_age_mask = false;
		}
	}
	if (define_active_polygons)
	{
		vertex_and_fragment_shader_defines += "#define ACTIVE_POLYGONS\n";
	}
	if (define_surface_lighting)
	{
		vertex_and_fragment_shader_defines += "#define SURFACE_LIGHTING\n";
	}
	if (define_using_normal_map)
	{
		vertex_and_fragment_shader_defines += "#define USING_NORMAL_MAP\n";
	}
	if (define_no_directional_light_for_normal_maps)
	{
		vertex_and_fragment_shader_defines += "#define NO_DIRECTIONAL_LIGHT_FOR_NORMAL_MAPS\n";
	}
	if (define_map_view)
	{
		vertex_and_fragment_shader_defines += "#define MAP_VIEW\n";
	}

	// Vertex shader source.
	GLShaderSource vertex_shader_source;
	// Add the '#define' statements first.
	vertex_shader_source.add_code_segment(vertex_and_fragment_shader_defines.c_str());
	// Then add the GLSL function to rotate by quaternion.
	vertex_shader_source.add_code_segment_from_file(GLShaderSource::UTILS_FILE_NAME);
	// Then add the GLSL 'main()' function.
	vertex_shader_source.add_code_segment_from_file(RENDER_TILE_TO_SCENE_VERTEX_SHADER_SOURCE_FILE_NAME);

	GLShaderSource fragment_shader_source;
	// Add the '#define' statements first.
	fragment_shader_source.add_code_segment(vertex_and_fragment_shader_defines.c_str());
	// Add the bilinear GLSL function for bilinear texture filtering and rotation by quaternion.
	fragment_shader_source.add_code_segment_from_file(GLShaderSource::UTILS_FILE_NAME);
	// Then add the GLSL 'main()' function.
	fragment_shader_source.add_code_segment_from_file(RENDER_TILE_TO_SCENE_FRAGMENT_SHADER_SOURCE_FILE_NAME);

	// Link the shader program.
	boost::optional<GLProgramObject::shared_ptr_type> shader_program_object =
			GLShaderProgramUtils::compile_and_link_vertex_fragment_program(
					renderer,
					vertex_shader_source,
					fragment_shader_source);
	if (!shader_program_object)
	{
		return boost::none;
	}

	ProgramObject::Uniforms uniforms;

	//
	// Add the uniforms used by the current shader program.
	//
	// NOTE: This can only be determined by inspecting the shader program source code itself.
	//

	//
	// Mirror the vertex shader source code uniform variable declarations.
	//

	uniforms.uses_source_raster_texture_transform = true;
	uniforms.uses_clip_texture_transform = true;

	if (define_using_age_grid)
	{
		uniforms.uses_age_grid_texture_transform = true;
	}
	if (define_surface_lighting)
	{
		if (define_using_normal_map || !define_map_view)
		{
			uniforms.uses_plate_rotation_quaternion = true;
			if (define_using_normal_map)
			{
				uniforms.uses_normal_map_texture_transform = true;
				if (!define_map_view)
				{
					if (!define_no_directional_light_for_normal_maps)
					{
						uniforms.uses_world_space_light_direction = true;
					}
				}
			}
		}
	}

	//
	// Mirror the fragment shader source code uniform variable declarations.
	//

	uniforms.uses_source_texture_sampler = true;
	uniforms.uses_clip_texture_sampler = true;

	if (define_using_age_grid)
	{
		uniforms.uses_age_grid_texture_sampler = true;
		uniforms.uses_age_grid_texture_dimensions = true;
		if (define_generate_age_mask)
		{
			uniforms.uses_reconstruction_time = true;
		}
	}
	if (define_source_raster_is_floating_point)
	{
		uniforms.uses_source_texture_dimensions = true;
	}
	if (define_surface_lighting)
	{
		if (define_map_view && !define_using_normal_map)
		{
			uniforms.uses_ambient_and_diffuse_lighting = true;
		}
		else
		{
			uniforms.uses_light_ambient_contribution = true;
			if (define_using_normal_map)
			{
				uniforms.uses_normal_map_texture_sampler = true;
				if (define_map_view)
				{
					uniforms.uses_plate_rotation_quaternion = true;
					if (!define_no_directional_light_for_normal_maps)
					{
						uniforms.uses_light_direction_cube_texture_sampler = true;
					}
				}
				else
				{
					if (!define_no_directional_light_for_normal_maps)
					{
						uniforms.uses_world_space_light_direction = true;
					}
				}
			}
			else
			{
				uniforms.uses_world_space_light_direction = true;
			}
		}
	}


	const ProgramObject program_object(shader_program_object.get(), uniforms);

	//
	// Set some shader program constants that don't change.
	//

	if (program_object.uniforms.uses_source_texture_dimensions)
	{
		// Set the source tile texture dimensions (and inverse dimensions).
		// This uniform is constant (only needs to be reloaded if shader program is re-linked).
		program_object.shader_program_object->gl_uniform4f(
				renderer,
				"source_texture_dimensions",
				d_source_raster_tile_texel_dimension, d_source_raster_tile_texel_dimension,
				d_source_raster_inverse_tile_texel_dimension, d_source_raster_inverse_tile_texel_dimension);
	}

	if (program_object.uniforms.uses_age_grid_texture_dimensions &&
		d_age_grid_cube_raster)
	{
		// Set the age grid tile texture dimensions (and inverse dimensions).
		// This uniform is constant (only needs to be reloaded if shader program is re-linked).
		program_object.shader_program_object->gl_uniform4f(
				renderer,
				"age_grid_texture_dimensions",
				d_age_grid_cube_raster->tile_texel_dimension, d_age_grid_cube_raster->tile_texel_dimension,
				d_age_grid_cube_raster->inverse_tile_texel_dimension, d_age_grid_cube_raster->inverse_tile_texel_dimension);
	}

	return program_object;
}


GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::ProgramObject::Uniforms::Uniforms()
{
	// Default to false - each shader program must set to true if it uses the uniform variable.
	uses_source_raster_texture_transform = false;
	uses_source_texture_sampler = false;
	uses_source_texture_dimensions = false;
	uses_clip_texture_transform = false;
	uses_clip_texture_sampler = false;
	uses_age_grid_texture_transform = false;
	uses_age_grid_texture_sampler = false;
	uses_age_grid_texture_dimensions = false;
	uses_normal_map_texture_transform = false;
	uses_normal_map_texture_sampler = false;
	uses_reconstruction_time = false;
	uses_plate_rotation_quaternion = false;
	uses_world_space_light_direction = false;
	uses_light_direction_cube_texture_sampler = false;
	uses_ambient_and_diffuse_lighting = false;
	uses_light_ambient_contribution = false;
}


// It seems some template instantiations happen at the end of the file.
// Disabling shadow warning caused by boost object_pool (used by GPlatesUtils::ObjectCache).
DISABLE_GCC_WARNING("-Wshadow")

