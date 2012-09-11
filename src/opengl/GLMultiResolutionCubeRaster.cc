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
#include <boost/cast.hpp>
#include <boost/foreach.hpp>
/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>
#include <opengl/OpenGL.h>

#include "GLMultiResolutionCubeRaster.h"

#include "GLContext.h"
#include "GLProjectionUtils.h"
#include "GLRenderer.h"
#include "GLTexture.h"
#include "GLUtils.h"
#include "GLViewport.h"

#include "global/AssertionFailureException.h"
#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "utils/Base2Utils.h"
#include "utils/Profile.h"


GPlatesOpenGL::GLMultiResolutionCubeRaster::GLMultiResolutionCubeRaster(
		GLRenderer &renderer,
		const GLMultiResolutionRaster::non_null_ptr_type &multi_resolution_raster,
		std::size_t tile_texel_dimension,
		bool adapt_tile_dimension_to_source_resolution,
		FixedPointTextureFilterType fixed_point_texture_filter,
		CacheTileTexturesType cache_tile_textures) :
	d_multi_resolution_raster(multi_resolution_raster),
	d_tile_texel_dimension(tile_texel_dimension),
	d_fixed_point_texture_filter(fixed_point_texture_filter),
	// Start with small size cache and just let the cache grow in size as needed (if caching enabled)...
	d_texture_cache(tile_texture_cache_type::create(2/* GPU pipeline breathing room in case caching disabled*/)),
	d_cache_tile_textures(cache_tile_textures),
	d_cube_quad_tree(cube_quad_tree_type::create()),
	d_num_source_levels_of_detail_used(1)
{
	// Adjust the tile dimension to the source raster resolution if non-power-of-two textures are supported.
	// The number of levels of detail returned might not be all levels of the source raster -
	// just the ones used by this cube map raster (the lowest resolutions might get left off).
	adjust_tile_texel_dimension(adapt_tile_dimension_to_source_resolution);

	// The, possibly adapted, tile dimension should be a power-of-two *if*
	// 'GL_ARB_texture_non_power_of_two' is *not* supported.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			GPLATES_OPENGL_BOOL(GLEW_ARB_texture_non_power_of_two) ||
				GPlatesUtils::Base2::is_power_of_two(d_tile_texel_dimension),
			GPLATES_ASSERTION_SOURCE);
	// Make sure the, possibly adapted, tile dimension does not exceed the maximum texture size...
	if (d_tile_texel_dimension > GLContext::get_parameters().texture.gl_max_texture_size)
	{
		d_tile_texel_dimension = GLContext::get_parameters().texture.gl_max_texture_size;
	}

	initialise_cube_quad_trees();

	// If the client has requested the entire cube quad tree be cached...
	if (d_cache_tile_textures == CACHE_TILE_TEXTURES_ENTIRE_CUBE_QUAD_TREE)
	{
		// This does not consume memory until each individual tile is requested.
		// This effectively disables any recycling that would otherwise happen in the cache.
		d_texture_cache->set_min_num_objects(d_cube_quad_tree->size());
	}
}


// We use macros in <GL/glew.h> that contain old-style casts.
DISABLE_GCC_WARNING("-Wold-style-cast")

void
GPlatesOpenGL::GLMultiResolutionCubeRaster::adjust_tile_texel_dimension(
		bool adapt_tile_dimension_to_source_resolution)
{
	// We don't worry about half-texel expansion of the projection frustums here because
	// we just need to determine viewport dimensions. There will be a slight error by neglecting
	// the half texel but it's already an approximation anyway.
	// Besides, the half texel depends on the tile texel dimension and we're going to change that below.
	GLCubeSubdivision::non_null_ptr_to_const_type cube_subdivision = GLCubeSubdivision::create();

	// Get the projection transforms of an entire cube face (the lowest resolution level-of-detail).
	const GLTransform::non_null_ptr_to_const_type projection_transform =
			cube_subdivision->get_projection_transform(
					0/*level_of_detail*/, 0/*tile_u_offset*/, 0/*tile_v_offset*/);

	// Get the view transform - it doesn't matter which cube face we choose because, although
	// the view transform are different, it won't matter to us since we're projecting onto
	// a spherical globe from its centre and all faces project the same way.
	const GLTransform::non_null_ptr_to_const_type view_transform =
			cube_subdivision->get_view_transform(
					GPlatesMaths::CubeCoordinateFrame::POSITIVE_X);

	// Determine the scale factor for our viewport dimensions required to capture the resolution
	// of the highest level of detail (level 0) of the source raster into an entire cube face.
	double viewport_dimension_scale = d_multi_resolution_raster->get_viewport_dimension_scale(
			view_transform->get_matrix(),
			projection_transform->get_matrix(),
			// Start off with a tile-sized viewport - we'll adjust its width and height shortly...
			GLViewport(0, 0, d_tile_texel_dimension, d_tile_texel_dimension),
			0/*level_of_detail*/);

	// The source raster level-of-detail (and hence viewport dimension scale) is determined such
	// that a pixel on the globe covers no more than one pixel in the cube map. However the
	// variation in cube map projection from face centre to face corner is approximately
	// a factor of two (or one level-of-detail difference). This means two pixels on the globe
	// can fit into one pixel in the cube map at a face centre. This is fine for each level-of-detail
	// because it ensures plenty enough detail is rendered into a cube map tile. However it does
	// mean that we can get one resolution level than expected. This means we can zoom further
	// into the globe before it reaches full resolution and starts to get blurry.
	// The factor is sqrt(3) * (1 / cos(A)); where sin(A) = (1 / sqrt(3)).
	// This is the same as 3 / sqrt(2).
	// The sqrt(3) is length of cube half-diagonal (divided by unit-length globe radius).
	// The cos(A) is a 35 degree angle between the cube face and globe tangent plane at cube corner
	// (globe tangent calculated at position on globe that cube corner projects onto).
	// This factor is how much a pixel on the globe expands in size when projected to a pixel on the
	// cube face at its corner (and is close to a factor of two).
	viewport_dimension_scale *= 3.0 / std::sqrt(2.0);

	// Since our cube quad tree is an *integer* power-of-two subdivision of tiles find out the
	// *non-integer* power-of-two scale factor - next we'll adjust this so it becomes an *integer*
	// power-of-two (by adjusting the tile texel dimension to be a non-power-of-two).
	double log2_viewport_dimension_scale = std::log(viewport_dimension_scale) / std::log(2.0);

	// If we can only have power-of-two dimensions then we will need to adapt the tile texel dimension
	// to the next power-of-two if it isn't already a power-of-two.
	//
	// NOTE: We do this even if the client did *not* request the tile texel dimension be adapted.
	if (!GLEW_ARB_texture_non_power_of_two)
	{
		// Round up to the next power-of-two.
		// If it's already a power-of-two then it won't change.
		const std::size_t new_tile_texel_dimension = GPlatesUtils::Base2::next_power_of_two(d_tile_texel_dimension);

		// If the tile texel dimension changed then it affects the viewport dimension scale.
		if (new_tile_texel_dimension != d_tile_texel_dimension)
		{
			// Adjust the viewport dimension scale since it determines the number of levels of detail.
			viewport_dimension_scale *= double(d_tile_texel_dimension) / new_tile_texel_dimension;
			log2_viewport_dimension_scale = std::log(viewport_dimension_scale) / std::log(2.0);

			d_tile_texel_dimension = new_tile_texel_dimension;
		}

		// If the new viewport dimension is less than the original tile dimension then there will only
		// be one level (the root) in the cube quad tree.
		if (log2_viewport_dimension_scale < 0)
		{
			// Only one level of detail needed.
			d_num_source_levels_of_detail_used = 1;
			return;
		}

		// Return number of levels of detail used.
		// The first '+1' rounds up to the next integer level-of-detail.
		// The second '+1' converts from integer level-of-detail to number of levels of detail.
		d_num_source_levels_of_detail_used = static_cast<int>(log2_viewport_dimension_scale + 1) + 1;

		return;
	}

	// We can have non-power-of-two texture dimensions but we've been asked not to adapt the tile dimension.
	if (!adapt_tile_dimension_to_source_resolution)
	{
		if (log2_viewport_dimension_scale < 0)
		{
			// Only one level of detail needed.
			d_num_source_levels_of_detail_used = 1;
			return;
		}

		// Return number of levels of detail used.
		// The first '+1' rounds up to the next integer level-of-detail.
		// The second '+1' converts from integer level-of-detail to number of levels of detail.
		d_num_source_levels_of_detail_used = static_cast<int>(log2_viewport_dimension_scale + 1) + 1;

		return;
	}

	// If the new viewport dimension is less than the original tile dimension then there will only
	// be one level (the root) in the cube quad tree.
	if (log2_viewport_dimension_scale < 0)
	{
		// Scale the viewport dimension down and round up to the nearest integer.
		// The scale is in the range (0,1).
		d_tile_texel_dimension = static_cast<int>(viewport_dimension_scale * d_tile_texel_dimension + 1);

		// Only one level of detail needed.
		d_num_source_levels_of_detail_used = 1;
		return;
	}
	// The viewport dimension scale is greater than 1...

	// We want to keep the tile texel dimension in the range:
	//
	//    [d_tile_texel_dimension, 2 * d_tile_texel_dimension)
	//
	// ...just because that's a reasonable range that doesn't deviate too much from the
	// client's requested size and provides close to the desired granularity of tiles.
	//
	// Remove the *integer* power-of-two part of the scale factor.
	// That will get taken care of by our power-of-two recursion into the cube quad tree.
	double log2_viewport_dimension_scale_int;
	const double log2_viewport_dimension_scale_fract =
			std::modf(log2_viewport_dimension_scale, &log2_viewport_dimension_scale_int);

	// This scale factor is now in the range [1, 2).
	const double tile_texel_dimension_scale = std::pow(2.0, log2_viewport_dimension_scale_fract);

	// Increase the tile texel dimension from a power-of-two value to a non-power-of-two value.
	d_tile_texel_dimension = static_cast<int>(tile_texel_dimension_scale * d_tile_texel_dimension + 1);

	// Return number of levels of detail used.
	// Convert to integer (it's already integral - the 0.5 is just to avoid numerical precision issues).
	// The '+1' converts from integer level-of-detail to number of levels of detail.
	d_num_source_levels_of_detail_used = static_cast<int>(log2_viewport_dimension_scale_int + 0.5) + 1;
}

ENABLE_GCC_WARNING("-Wold-style-cast")


void
GPlatesOpenGL::GLMultiResolutionCubeRaster::initialise_cube_quad_trees()
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_num_source_levels_of_detail_used > 0,
			GPLATES_ASSERTION_SOURCE);

	// Create a projection transforms cube quad tree cache so we can determine the view frustum
	// to render each spatial partition node into.
	// We're only visiting each subdivision node once so there's no need for caching.
	//
	// Expand the tile frustums by half a texel around the border of each frustum.
	// This causes the texel centres of the border tile texels to fall right on the edge
	// of the unmodified frustum which means adjacent tiles will have the same colour after
	// bilinear filtering and hence there will be no visible colour seams (or discontinuities
	// in the raster data if the source raster is floating-point).
	// The nice thing is this works for both bilinear filtering and nearest neighbour filtering
	// (ie, there'll be no visible seams in nearest neighbour filtering either).
	cube_subdivision_cache_type::non_null_ptr_type
			cube_subdivision_cache =
					cube_subdivision_cache_type::create(
							GPlatesOpenGL::GLCubeSubdivision::create(
									GPlatesOpenGL::GLCubeSubdivision::get_expand_frustum_ratio(
											d_tile_texel_dimension,
											0.5/* half a texel */)));

	for (unsigned int face = 0; face < 6; ++face)
	{
		GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face =
				static_cast<GPlatesMaths::CubeCoordinateFrame::CubeFaceType>(face);

		// The viewport is the same for all subdivisions since they use the same tile texture dimension.
		const GLViewport viewport(0, 0, d_tile_texel_dimension, d_tile_texel_dimension);

		// Get the cube subdivision cache root node.
		const cube_subdivision_cache_type::node_reference_type
				cube_subdivision_cache_root_node =
						cube_subdivision_cache->get_quad_tree_root_node(cube_face);

		// Recursively generate a quad tree for the current cube face.
		boost::optional<cube_quad_tree_type::node_type::ptr_type> quad_tree_root_node =
				create_quad_tree_node(
						viewport,
						*cube_subdivision_cache,
						cube_subdivision_cache_root_node,
						// Start out at the lowest resolution level of detail supported by
						// the root level of our quad tree...
						d_num_source_levels_of_detail_used - 1/*source_level_of_detail*/);

		// If the source raster covered any part of the current cube face.
		if (quad_tree_root_node)
		{
			d_cube_quad_tree->set_quad_tree_root_node(cube_face, quad_tree_root_node.get());
		}
	}
}


boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRaster::cube_quad_tree_type::node_type::ptr_type>
GPlatesOpenGL::GLMultiResolutionCubeRaster::create_quad_tree_node(
		const GLViewport &viewport,
		cube_subdivision_cache_type &cube_subdivision_cache,
		const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node,
		const unsigned int source_level_of_detail)
{
	//
	// Setup the model-view and projection matrices and setup the viewport.
	// These are required when the source raster looks for tiles visible in the view frustum
	// of our quad tree subdivision cell (the viewport is used to determine the correct
	// level-of-detail tiles to use).
	//

	// Set the view transform.
	const GLTransform::non_null_ptr_to_const_type view_transform =
			cube_subdivision_cache.get_view_transform(cube_subdivision_cache_node);

	// Set the projection transform.
	// Get the expanded tile frustum (by half a texel around the border of the frustum).
	// This causes the texel centres of the border tile texels to fall right on the edge
	// of the unmodified frustum which means adjacent tiles will have the same colour after
	// bilinear filtering and hence there will be no visible colour seams (or discontinuities
	// in the raster data if the source raster is floating-point).
	const GLTransform::non_null_ptr_to_const_type half_texel_expanded_projection_transform =
			cube_subdivision_cache.get_projection_transform(
					cube_subdivision_cache_node);

	// Determine the source raster level-of-detail from the current viewport and view/projection matrices.
	// Only to see if we can use a lower resolution than pre-calculated.
	// Level-of-detail will get clamped such that it's ">= 0" (and hence can be represented as unsigned).
	unsigned int tile_level_of_detail = boost::numeric_cast<unsigned int>(
			d_multi_resolution_raster->clamp_level_of_detail(
					d_multi_resolution_raster->get_level_of_detail(
							view_transform->get_matrix(),
							half_texel_expanded_projection_transform->get_matrix(),
							viewport)));
	// If we can use a lower-resolution level-of-detail just for this tile then might as well
	// otherwise just use the pre-calculated level-of-detail.
	// If the tile texel dimension has been adjusted to the optimal non-power-of-two value
	// then this is not likely to happen.
	// However if we're forced to use power-of-two dimensions then, as we approach tiles near
	// the centre of the cube face, we might be able to use lower-resolution level-of-details
	// due to the texel resolution of those tiles being lower due to the cube map projection.
	//
	// However don't go to a higher resolution than pre-calculated - this might happen due to numerical
	// precision and the fact that the viewport dimensions have been adjusted to give almost
	// exactly integer level-of-detail values (and might get level-of-detail slightly less than
	// the integer which would get truncated to the next higher resolution LOD).
	if (tile_level_of_detail < source_level_of_detail)
	{
		// Need to clamp because 'source_level_of_detail' might be a lower resolution than
		// supported by the source raster.
		tile_level_of_detail = boost::numeric_cast<unsigned int>(
				d_multi_resolution_raster->clamp_level_of_detail(source_level_of_detail));
	}

	// Get the source tiles that are visible in the current view frustum.
	std::vector<GLMultiResolutionRaster::tile_handle_type> source_raster_tile_handles;
	d_multi_resolution_raster->get_visible_tiles(
			source_raster_tile_handles,
			view_transform->get_matrix(),
			half_texel_expanded_projection_transform->get_matrix(),
			tile_level_of_detail);

	// If there are no tiles it means the source raster does not have global extents
	// and we are looking at a part of the globe not covered by it.
	if (source_raster_tile_handles.empty())
	{
		return boost::none;
	}

	// Create a quad tree node.
	cube_quad_tree_type::node_type::ptr_type quad_tree_node =
			d_cube_quad_tree->create_node(
					CubeQuadTreeNode(
							tile_level_of_detail,
							view_transform,
							half_texel_expanded_projection_transform,
							d_texture_cache->allocate_volatile_object()));

	// Mark the tile as up-to-date with respect to the world transform.
	d_world_transform_subject.update_observer(
			quad_tree_node->get_element().d_visible_source_tiles_observer_token);

	// Initialise the quad tree node's source tiles.
	quad_tree_node->get_element().d_src_raster_tiles.swap(source_raster_tile_handles);

	// If we have reached the highest resolution level of detail in the source raster
	// then we don't need to create any child quad tree nodes.
	if (source_level_of_detail == 0)
	{
		return quad_tree_node;
	}


	//
	// Iterate over the child subdivision regions and create if cover source raster.
	//

	for (unsigned int child_v_offset = 0; child_v_offset < 2; ++child_v_offset)
	{
		for (unsigned int child_u_offset = 0; child_u_offset < 2; ++child_u_offset)
		{
			// Get the project transforms child quad tree node.
			const cube_subdivision_cache_type::node_reference_type
					child_cube_subdivision_cache_node =
							cube_subdivision_cache.get_child_node(
									cube_subdivision_cache_node,
									child_u_offset,
									child_v_offset);

			// Returns a non-null child if child quad tree node is covered by source raster.
			const boost::optional<cube_quad_tree_type::node_type::ptr_type> child_node =
					create_quad_tree_node(
							viewport,
							cube_subdivision_cache,
							child_cube_subdivision_cache_node,
							// Child is at a higher resolution level-of-detail...
							source_level_of_detail - 1);

			// Add the child node if one was created.
			if (child_node)
			{
				// Add the node as a child.
				d_cube_quad_tree->set_child_node(
						*quad_tree_node, child_u_offset, child_v_offset, child_node.get());

				// Since we've given the quad tree node a child it is now an internal node.
				quad_tree_node->get_element().d_is_leaf_node = false;
			}
		}
	}

	return quad_tree_node;
}


void
GPlatesOpenGL::GLMultiResolutionCubeRaster::set_world_transform(
		const GLMatrix &world_transform)
{
	d_world_transform = world_transform;

	// Iterate over all the nodes in the cube quad tree and reset the observer tokens.
	// This will force an update when the textures are subsequently requested.
	cube_quad_tree_type::iterator cube_quad_tree_node_iter = d_cube_quad_tree->get_iterator();
	for ( ; !cube_quad_tree_node_iter.finished(); cube_quad_tree_node_iter.next())
	{
		CubeQuadTreeNode &cube_quad_tree_node = cube_quad_tree_node_iter.get_element();

		// Force the texture to be regenerated.
		cube_quad_tree_node.d_source_texture_observer_token.reset();
	}

	// Force the visible source tiles to be recalculated for each cube quad tree tile.
	// This is necessary because the source tiles that contribute to each destination tile are now different.
	d_world_transform_subject.invalidate();
}


GPlatesOpenGL::GLTexture::shared_ptr_to_const_type
GPlatesOpenGL::GLMultiResolutionCubeRaster::get_tile_texture(
		GLRenderer &renderer,
		const CubeQuadTreeNode &tile,
		cache_handle_type &cache_handle)
{
	//
	// See if a new set of visible source tiles, contributing to this tile, needs to be calculated.
	//

	if (!d_world_transform_subject.is_observer_up_to_date(tile.d_visible_source_tiles_observer_token))
	{
		// Clear the old set of source tiles.
		tile.d_src_raster_tiles.clear();

		// Multiply the current world transform into the model-view matrix.
		GLMatrix world_model_view_transform(tile.d_view_transform->get_matrix());
		world_model_view_transform.gl_mult_matrix(d_world_transform);

		// Get the new set of source tiles that are visible in the current tile's frustum.
		d_multi_resolution_raster->get_visible_tiles(
				tile.d_src_raster_tiles,
				world_model_view_transform,
				tile.d_projection_transform->get_matrix(),
				tile.d_tile_level_of_detail);

		// The tile is now up-to-date with respect to the world transform.
		d_world_transform_subject.update_observer(tile.d_visible_source_tiles_observer_token);
	}

	//
	// Get the texture for the tile.
	//

	// See if we've generated our tile texture and
	// see if it hasn't been recycled by the texture cache.
	tile_texture_cache_type::object_shared_ptr_type tile_texture = tile.d_tile_texture->get_cached_object();
	if (!tile_texture)
	{
		tile_texture = tile.d_tile_texture->recycle_an_unused_object();
		if (!tile_texture)
		{
			// Create a new tile texture.
			tile_texture = tile.d_tile_texture->set_cached_object(
					std::auto_ptr<TileTexture>(new TileTexture(renderer)),
					// Called whenever tile texture is returned to the cache...
					boost::bind(&TileTexture::returned_to_cache, _1));

			// The texture was just allocated so we need to create it in OpenGL.
			create_tile_texture(renderer, tile_texture->texture, tile);

			//qDebug() << "GLMultiResolutionCubeRaster: " << d_texture_cache->get_current_num_objects_in_use();
		}
		else
		{
			// The filtering depends on whether the current tile is a leaf node or not.
			// So update the texture's filtering options.
			update_tile_texture(renderer, tile_texture->texture, tile);
		}

		// Render the source raster into our tile texture.
		render_raster_data_into_tile_texture(
				renderer,
				tile,
				*tile_texture);
	}
	// Our texture wasn't recycled but see if it's still valid in case the source
	// raster changed the data underneath us.
	else if (!d_multi_resolution_raster->get_subject_token().is_observer_up_to_date(
				tile.d_source_texture_observer_token))
	{
		// Render the source raster into our tile texture.
		render_raster_data_into_tile_texture(
				renderer,
				tile,
				*tile_texture);
	}

	// The caller will cache this tile to keep it from being prematurely recycled by our caches.
	//
	// Note that none of this has any effect if the client specified the entire cube quad tree
	// be cached (in the 'create()' method) in which case it'll get cached regardless.
	cache_handle = boost::shared_ptr<ClientCacheTile>(
			new ClientCacheTile(tile_texture, d_cache_tile_textures));

	return tile_texture->texture;
}


void
GPlatesOpenGL::GLMultiResolutionCubeRaster::render_raster_data_into_tile_texture(
		GLRenderer &renderer,
		const CubeQuadTreeNode &tile,
		TileTexture &tile_texture)
{
	PROFILE_FUNC();

	// Begin rendering to a 2D render target texture.
	GLRenderer::RenderTarget2DScope render_target_scope(renderer, tile_texture.texture);

	renderer.gl_viewport(0, 0, d_tile_texel_dimension, d_tile_texel_dimension);

	renderer.gl_clear_color(); // Clear colour to all zeros.
	renderer.gl_clear(GL_COLOR_BUFFER_BIT); // Clear only the colour buffer.

	// The model-view matrix.
	renderer.gl_load_matrix(GL_MODELVIEW, tile.d_view_transform->get_matrix());
	// Multiply in the requested world transform.
	renderer.gl_mult_matrix(GL_MODELVIEW, d_world_transform);

	// The projection matrix.
	renderer.gl_load_matrix(GL_PROJECTION, tile.d_projection_transform->get_matrix());

	// If the render target is floating-point...
	if (tile_texture.texture->is_floating_point())
	{
		// A lot of graphics hardware does not support blending to floating-point targets so we don't enable it.
		// And a floating-point render target is used for data rasters (ie, not coloured as fixed-point
		// for visual display) - where the coverage (or alpha) is in the green channel instead of the alpha channel.
	}
	else // an RGBA render target...
	{
#if 0 // We don't really need alpha blending since the source raster tiles don't overlap...
		// Set up alpha blending for pre-multiplied alpha.
		// This has (src,dst) blend factors of (1, 1-src_alpha) instead of (src_alpha, 1-src_alpha).
		// This is where the RGB channels have already been multiplied by the alpha channel.
		renderer.gl_enable(GL_BLEND);
		renderer.gl_blend_func(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
#endif

		// Enable alpha testing as an optimisation for culling transparent raster pixels.
		renderer.gl_enable(GL_ALPHA_TEST);
		renderer.gl_alpha_func(GL_GREATER, GLclampf(0));
	}

	// Get the source raster to render into the render target using the view frustum
	// we have provided. We have already pre-calculated the list of visible source raster tiles
	// that need to be rendered into our frustum to save it a bit of culling work.
	// And that's why we also don't need to test the return value to see if anything was rendered.
	d_multi_resolution_raster->render(
			renderer,
			tile.d_src_raster_tiles,
			tile_texture.source_cache_handle);

	// This tile texture is now update-to-date with respect to the source multi-resolution raster.
	d_multi_resolution_raster->get_subject_token().update_observer(
			tile.d_source_texture_observer_token);
}


boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRaster::quad_tree_node_type>
GPlatesOpenGL::GLMultiResolutionCubeRaster::get_quad_tree_root_node(
		GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face)
{
	const cube_quad_tree_type::node_type *cube_root_node =
			d_cube_quad_tree->get_quad_tree_root_node(cube_face);
	if (cube_root_node == NULL)
	{
		return boost::none;
	}

	return quad_tree_node_type(
			GPlatesUtils::non_null_intrusive_ptr<quad_tree_node_type::ImplInterface>(
					new QuadTreeNodeImpl(*cube_root_node, *this)));
}


boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRaster::quad_tree_node_type>
GPlatesOpenGL::GLMultiResolutionCubeRaster::get_child_node(
		const quad_tree_node_type &parent_node,
		unsigned int child_x_offset,
		unsigned int child_y_offset)
{
	// Get our internal cube quad tree parent node.
	const cube_quad_tree_type::node_type &cube_parent_node = get_cube_quad_tree_node(parent_node);

	// Get the internal cube quad tree child node.
	const cube_quad_tree_type::node_type *cube_child_node =
			cube_parent_node.get_child_node(child_x_offset, child_y_offset);
	if (cube_child_node == NULL)
	{
		return boost::none;
	}

	return quad_tree_node_type(
			GPlatesUtils::non_null_intrusive_ptr<quad_tree_node_type::ImplInterface>(
					new QuadTreeNodeImpl(*cube_child_node, *this)));
}


// We use macros in <GL/glew.h> that contain old-style casts.
DISABLE_GCC_WARNING("-Wold-style-cast")

void
GPlatesOpenGL::GLMultiResolutionCubeRaster::create_tile_texture(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_type &tile_texture,
		const CubeQuadTreeNode &tile)
{
	//PROFILE_FUNC();

	// Use the same texture format as the source raster.
	const GLint internal_format = d_multi_resolution_raster->get_target_texture_internal_format();

	//
	// No mipmaps needed so we specify no mipmap filtering.
	// We're not using mipmaps because our cube mapping does not have much distortion
	// unlike global rectangular lat/lon rasters that squash near the poles.
	//
	// We do enable bilinear filtering if the texture is a fixed-point format.
	// The client needs to emulate bilinear filtering in a fragment shader if the format is floating-point.
	if (GLTexture::is_format_floating_point(internal_format))
	{
		tile_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		tile_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	else // fixed-point...
	{
		// Use nearest filtering for the 'min' filter since it displays a visually crisper image.
		// This is for fixed-point data (used for visual display) - for floating-point data clients
		// will still use bilinear filtering (in their shader program).
		tile_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		// Set the magnification filter.
		update_fixed_point_tile_texture_mag_filter(renderer, tile_texture, tile);

		// Specify anisotropic filtering if it's supported since we are not using mipmaps
		// and any textures rendered near the edge of the globe will get squashed a bit due to
		// the angle we are looking at them and anisotropic filtering will help here.
		//
		// NOTE: We don't enable anisotropic filtering for floating-point textures since earlier
		// hardware (that supports floating-point textures) only supports nearest filtering.
		if (GLEW_EXT_texture_filter_anisotropic &&
			(d_fixed_point_texture_filter == FIXED_POINT_TEXTURE_FILTER_MAG_NEAREST_ANISOTROPIC ||
				d_fixed_point_texture_filter == FIXED_POINT_TEXTURE_FILTER_MAG_LINEAR_ANISOTROPIC))
		{
			const GLfloat anisotropy = GLContext::get_parameters().texture.gl_texture_max_anisotropy;
			tile_texture->gl_tex_parameterf(renderer, GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
		}
	}

	// Clamp texture coordinates to centre of edge texels -
	// it's easier for hardware to implement - and doesn't affect our calculations.
	if (GLEW_EXT_texture_edge_clamp || GLEW_SGIS_texture_edge_clamp)
	{
		tile_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		tile_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	else
	{
		tile_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		tile_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}

	// Create the texture but don't load any data into it.
	// Leave it uninitialised because we will be rendering into it to initialise it.
	//
	// NOTE: Since the image data is NULL it doesn't really matter what 'format' and 'type' are -
	// just use values that are compatible with all internal formats to avoid a possible error.
	tile_texture->gl_tex_image_2D(renderer, GL_TEXTURE_2D, 0,
			internal_format,
			d_tile_texel_dimension, d_tile_texel_dimension,
			0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	// Check there are no OpenGL errors.
	GLUtils::assert_no_gl_errors(GPLATES_ASSERTION_SOURCE);
}


void
GPlatesOpenGL::GLMultiResolutionCubeRaster::update_tile_texture(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_type &tile_texture,
		const CubeQuadTreeNode &tile)
{
	// Use the same texture format as the source raster.
	const boost::optional<GLenum> internal_format = tile_texture->get_internal_format();
	// The texture should have been initialised.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			internal_format,
			GPLATES_ASSERTION_SOURCE);

	// Nothing to do if it's a floating-point texture (only supports nearest filtering)...
	if (GLTexture::is_format_floating_point(internal_format.get()))
	{
		return;
	}

	update_fixed_point_tile_texture_mag_filter(renderer, tile_texture, tile);
}


void
GPlatesOpenGL::GLMultiResolutionCubeRaster::update_fixed_point_tile_texture_mag_filter(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_type &tile_texture,
		const CubeQuadTreeNode &tile)
{
	// Set the magnification filter.
	switch (d_fixed_point_texture_filter)
	{
	case FIXED_POINT_TEXTURE_FILTER_MAG_NEAREST:
	case FIXED_POINT_TEXTURE_FILTER_MAG_NEAREST_ANISOTROPIC:
		tile_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		break;

	case FIXED_POINT_TEXTURE_FILTER_MAG_LINEAR:
	case FIXED_POINT_TEXTURE_FILTER_MAG_LINEAR_ANISOTROPIC:
		// Only if it's a leaf node do we specify bilinear filtering since only then
		// will the texture start to magnify (the bilinear makes it smooth instead of pixelated).
		if (tile.d_is_leaf_node)
		{
			tile_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
		else
		{
			tile_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}
		break;

	default:
		// Unsupported texture filter type.
		GPlatesGlobal::Abort(GPLATES_EXCEPTION_SOURCE);
		break;
	}
}

ENABLE_GCC_WARNING("-Wold-style-cast")
