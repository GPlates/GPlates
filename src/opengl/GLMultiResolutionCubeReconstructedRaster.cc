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
#include <boost/foreach.hpp>
/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>
#include <opengl/OpenGL.h>
#include <QDebug>

#include "GLMultiResolutionCubeReconstructedRaster.h"

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


const unsigned int GPlatesOpenGL::GLMultiResolutionCubeReconstructedRaster::MIN_TILE_TEXEL_DIMENSION = 256;


GPlatesOpenGL::GLMultiResolutionCubeReconstructedRaster::GLMultiResolutionCubeReconstructedRaster(
		GLRenderer &renderer,
		const GLMultiResolutionStaticPolygonReconstructedRaster::non_null_ptr_type &source_reconstructed_raster,
		bool cache_tile_textures) :
	d_reconstructed_raster(source_reconstructed_raster),
	d_level_of_detail_offset_for_scaled_tile_dimension(0),
	d_tile_texel_dimension(
			update_tile_texel_dimension(
					renderer,
					source_reconstructed_raster->get_tile_texel_dimension())),
	// Start with small size cache and just let the cache grow in size as needed (if caching enabled)...
	d_texture_cache(tile_texture_cache_type::create(2/* GPU pipeline breathing room in case caching disabled*/)),
	d_cache_tile_textures(cache_tile_textures),
	d_cube_subdivision(
			// Expand the tile frustums by half a texel around the border of each frustum.
			// This causes the texel centres of the border tile texels to fall right on the edge
			// of the unmodified frustum which means adjacent tiles will have the same colour after
			// bilinear filtering and hence there will be no visible colour seams (or discontinuities
			// in the raster data if the source raster is floating-point).
			// The nice thing is this works for both bilinear filtering and nearest neighbour filtering
			// (ie, there'll be no visible seams in nearest neighbour filtering either).
			GPlatesOpenGL::GLCubeSubdivision::create(
					GPlatesOpenGL::GLCubeSubdivision::get_expand_frustum_ratio(
							d_tile_texel_dimension,
							0.5/* half a texel */))),
	d_cube_quad_tree(cube_quad_tree_type::create())
{
}


unsigned int
GPlatesOpenGL::GLMultiResolutionCubeReconstructedRaster::update_tile_texel_dimension(
		GLRenderer &renderer,
		unsigned int tile_texel_dimension)
{
	const GLCapabilities &capabilities = renderer.get_capabilities();

	// If tile dimensions are too small then we end up requiring a lot more tiles to render since
	// there's no limit on how deep we can render (see 'QuadTreeNodeImp::is_leaf_node()' for more details).
	//
	// To fix this we keep doubling the tile dimensions until they exceed a minimum (and without
	// exceeding the maximum texture size).
	d_level_of_detail_offset_for_scaled_tile_dimension = 0;
	while (tile_texel_dimension < MIN_TILE_TEXEL_DIMENSION)
	{
		// Make sure the doubled tile dimension does not exceed the maximum texture size.
		// We're requiring the final multiplier to be a power-of-two so that the level-of-detail
		// adjustment is an integer (so we can render at an exact LOD level).
		if (2 * tile_texel_dimension > capabilities.texture.gl_max_texture_size)
		{
			break;
		}

		tile_texel_dimension = 2 * tile_texel_dimension;
		--d_level_of_detail_offset_for_scaled_tile_dimension;
	}

	return tile_texel_dimension;
}


void
GPlatesOpenGL::GLMultiResolutionCubeReconstructedRaster::set_world_transform(
		const GLMatrix &world_transform)
{
	// If the world transform has changed then set it, and mark all our texture tiles dirty.
	if (d_world_transform != world_transform)
	{
		d_world_transform = world_transform;

		// Iterate over all the nodes in the cube quad tree and reset the observer tokens.
		// This will force an update when the textures are subsequently requested.
		cube_quad_tree_type::iterator cube_quad_tree_node_iter = d_cube_quad_tree->get_iterator();
		for ( ; !cube_quad_tree_node_iter.finished(); cube_quad_tree_node_iter.next())
		{
			CubeQuadTreeNode &cube_quad_tree_node = cube_quad_tree_node_iter.get_element();
			cube_quad_tree_node.d_source_texture_observer_token.reset();
		}

		// Let any clients know that they're now out-of-date (since our cube map texture has a new orientation).
		d_subject_token.invalidate();
	}
}


const GPlatesUtils::SubjectToken &
GPlatesOpenGL::GLMultiResolutionCubeReconstructedRaster::get_subject_token() const
{
	//
	// This covers changes to the inputs that don't require completely re-creating the inputs.
	// That is beyond our scope and is detected and managed by our owners (and owners of our inputs).
	//

	// If the source raster has changed.
	if (!d_reconstructed_raster->get_subject_token().is_observer_up_to_date(
				d_reconstructed_raster_observer_token))
	{
		d_subject_token.invalidate();

		d_reconstructed_raster->get_subject_token().update_observer(
				d_reconstructed_raster_observer_token);
	}

	return d_subject_token;
}


boost::optional<GPlatesOpenGL::GLTexture::shared_ptr_to_const_type>
GPlatesOpenGL::GLMultiResolutionCubeReconstructedRaster::get_tile_texture(
		GLRenderer &renderer,
		const CubeQuadTreeNode &tile,
		cache_handle_type &cache_handle)
{
	//
	// Get the texture for the tile.
	//

	// Is true if reconstructed raster is visible in current node's frustum.
	// The default (a valid cached texture) is visible.
	bool visible = true;

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
					std::unique_ptr<TileTexture>(new TileTexture(renderer)),
					// Called whenever tile texture is returned to the cache...
					boost::bind(&TileTexture::returned_to_cache, boost::placeholders::_1));

			// The texture was just allocated so we need to create it in OpenGL.
			create_tile_texture(renderer, tile_texture->texture);

			//qDebug() << "GLMultiResolutionCubeReconstructedRaster: " << d_texture_cache->get_current_num_objects_in_use();
		}

		// Render the source raster into our tile texture.
		visible = render_raster_data_into_tile_texture(
				renderer,
				tile,
				*tile_texture);
	}
	// Our texture wasn't recycled but see if it's still valid in case the source
	// raster changed the data underneath us.
	else if (!d_reconstructed_raster->get_subject_token().is_observer_up_to_date(
				tile.d_source_texture_observer_token))
	{
		// Render the source raster into our tile texture.
		visible = render_raster_data_into_tile_texture(
				renderer,
				tile,
				*tile_texture);
	}

	// The caller will cache this tile to keep it from being prematurely recycled by our caches.
	// Also the cached data accumulated by the reconstructed raster renderer will be added to the cache.
	cache_handle = boost::shared_ptr<ClientCacheTile>(
			new ClientCacheTile(
					tile_texture,
					// Only cache the tile texture if the client has requested it...
					d_cache_tile_textures &&
						// If the nothing was rendered into the tile then we don't want to return the
						// unused texture to the caller for caching - this way it'll get returned to
						// the texture cache for reuse...
						visible));

	// If nothing was rendered then inform the caller.
	if (!visible)
	{
		return boost::none;
	}

	return GLTexture::shared_ptr_to_const_type(tile_texture->texture);
}


bool
GPlatesOpenGL::GLMultiResolutionCubeReconstructedRaster::render_raster_data_into_tile_texture(
		GLRenderer &renderer,
		const CubeQuadTreeNode &tile,
		TileTexture &tile_texture)
{
	PROFILE_FUNC();

	// Determine if anything was rendered.
	bool rendered = false;

	// We might do multiple source raster render calls (due to render target tiling).
	boost::shared_ptr<std::vector<GLMultiResolutionStaticPolygonReconstructedRaster::cache_handle_type> > tile_cache_handle(
			new std::vector<GLMultiResolutionStaticPolygonReconstructedRaster::cache_handle_type>());

	// Begin rendering to a 2D render target texture.
	GLRenderer::RenderTarget2DScope render_target_scope(
			renderer,
			tile_texture.texture,
			GLViewport(0, 0, d_tile_texel_dimension, d_tile_texel_dimension));

	// The render target tiling loop...
	do
	{
		// Begin the current render target tile - this also sets the viewport.
		GLTransform::non_null_ptr_to_const_type render_target_tile_projection = render_target_scope.begin_tile();

		// Set up the projection transform adjustment for the current render target tile.
		renderer.gl_load_matrix(GL_PROJECTION, render_target_tile_projection->get_matrix());
		// Multiply in the projection matrix.
		renderer.gl_mult_matrix(GL_PROJECTION, tile.d_projection_transform->get_matrix());

		// The model-view matrix.
		renderer.gl_load_matrix(GL_MODELVIEW, tile.d_view_transform->get_matrix());
		// Multiply in the requested world transform.
		renderer.gl_mult_matrix(GL_MODELVIEW, d_world_transform);

		renderer.gl_clear_color(); // Clear colour to all zeros.
		renderer.gl_clear(GL_COLOR_BUFFER_BIT); // Clear only the colour buffer.

		// If the render target is floating-point...
		if (tile_texture.texture->is_floating_point())
		{
			// A lot of graphics hardware does not support blending to floating-point targets so we don't enable it.
			// And a floating-point render target is used for data rasters (ie, not coloured as fixed-point
			// for visual display) - where the coverage (or alpha) is in the green channel instead of the alpha channel.
		}
		else // an RGBA render target...
		{
			// Set up alpha blending for pre-multiplied alpha.
			// This has (src,dst) blend factors of (1, 1-src_alpha) instead of (src_alpha, 1-src_alpha).
			// This is where the RGB channels have already been multiplied by the alpha channel.
			// See class GLVisualRasterSource for why this is done.
			renderer.gl_enable(GL_BLEND);
			renderer.gl_blend_func(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

			// Enable alpha testing as an optimisation for culling transparent raster pixels.
			renderer.gl_enable(GL_ALPHA_TEST);
			renderer.gl_alpha_func(GL_GREATER, GLclampf(0));
		}

		// Reconstruct source raster by rendering into the render target using the view frustum
		// we have provided and the level-of-detail we have calculated.
		GLMultiResolutionStaticPolygonReconstructedRaster::cache_handle_type source_cache_handle;
		if (d_reconstructed_raster->render(renderer, tile.d_level_of_detail, source_cache_handle))
		{
			rendered = true;
		}
		tile_cache_handle->push_back(source_cache_handle);
	}
	while (render_target_scope.end_tile());

	tile_texture.source_cache_handle = tile_cache_handle;

	// This tile texture is now update-to-date with respect to the source raster.
	d_reconstructed_raster->get_subject_token().update_observer(
			tile.d_source_texture_observer_token);

	// Return true if anything was rendered into the current node's tile.
	return rendered;
}


float
GPlatesOpenGL::GLMultiResolutionCubeReconstructedRaster::get_level_of_detail(
		unsigned int quad_tree_depth) const
{
	// Since our (cube map) tile dimension is the same as the reconstructed raster's input
	// source (cube map) tile dimension we will just render the (input) source (cube map) raster
	// as the same level-of-detail (which means the same quad tree depth).
	//
	// UPDATE: Our (cube map) tile dimension can now be a power-of-two multiple of the reconstructed
	// raster's input source (cube map) tile dimension if the latter is found to be too small.
	// We account for this by adding a LOD offset to the final level-of-detail.
	//
	// NOTE: Previously we did the usual thing of passing our tile's modelview/projection matrices
	// and viewport to the reconstructed raster which, in turn, determined the level-of-detail
	// (quad tree depth) to render at. However, due to the non-uniformity of pixels across a cube
	// map face (about a factor of two), we ended up rendering too high a resolution (sometimes
	// an extra two levels too deep) which just slowed things down significantly (mostly due to the
	// extra input raster data that needed to be converted to colours by palette lookup).
	// Now the size of source tiles is roughly the same in the globe and map views.
	const float cube_quad_tree_depth = quad_tree_depth;

	// Need to convert cube quad tree depth to the level-of-detail recognised by the source raster.
	// See 'GLMultiResolutionStaticPolygonReconstructedRaster::get_level_of_detail()' for more details.
	//
	// We add a LOD offset to account for possible power-of-two scaling of input source tile dimension.
	const float level_of_detail = d_level_of_detail_offset_for_scaled_tile_dimension +
			(d_reconstructed_raster->get_num_levels_of_detail() - 1) - cube_quad_tree_depth;

	// Return the clamped level-of-detail to ensure it is within a valid range.
	return d_reconstructed_raster->clamp_level_of_detail(level_of_detail);
}


boost::optional<GPlatesOpenGL::GLMultiResolutionCubeReconstructedRaster::quad_tree_node_type>
GPlatesOpenGL::GLMultiResolutionCubeReconstructedRaster::get_quad_tree_root_node(
		GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face)
{
	const GPlatesMaths::CubeQuadTreeLocation cube_quad_tree_location(cube_face);

	// See if we have a root cube quad tree node.
	cube_quad_tree_type::node_type *cube_root_node =
			d_cube_quad_tree->get_quad_tree_root_node(cube_face);

	// If not then just create one.
	if (cube_root_node == NULL)
	{
		// The view transform for the current cube face.
		const GLTransform::non_null_ptr_to_const_type view_transform =
				d_cube_subdivision->get_view_transform(cube_face);

		// The projection transform for the root cube quad tree node.
		const GLTransform::non_null_ptr_to_const_type projection_transform =
				d_cube_subdivision->get_projection_transform(
						0/*level_of_detail*/,
						0/*tile_u_offset*/,
						0/*tile_v_offset*/);

		// Create a root quad tree node.
		d_cube_quad_tree->set_quad_tree_root_node(
				cube_face,
				d_cube_quad_tree->create_node(
						CubeQuadTreeNode(
								view_transform,
								projection_transform,
								get_level_of_detail(
										cube_quad_tree_location.get_node_location()->quad_tree_depth),
								d_texture_cache->allocate_volatile_object())));

		cube_root_node = d_cube_quad_tree->get_quad_tree_root_node(cube_face);
	}

	return quad_tree_node_type(
			GPlatesUtils::non_null_intrusive_ptr<quad_tree_node_type::ImplInterface>(
					new QuadTreeNodeImpl(*cube_root_node, *this, cube_quad_tree_location)));
}


boost::optional<GPlatesOpenGL::GLMultiResolutionCubeReconstructedRaster::quad_tree_node_type>
GPlatesOpenGL::GLMultiResolutionCubeReconstructedRaster::get_child_node(
		const quad_tree_node_type &parent_node,
		unsigned int child_x_offset,
		unsigned int child_y_offset)
{
	// Get our internal cube quad tree parent node.
	QuadTreeNodeImpl &parent_node_impl = get_quad_tree_node_impl(parent_node);

	// Location of the current child node in the cube quad tree.
	const GPlatesMaths::CubeQuadTreeLocation child_cube_quad_tree_location(
			parent_node_impl.cube_quad_tree_location,
			child_x_offset,
			child_y_offset);

	// Get the internal cube quad tree child node.
	// See if we have a child cube quad tree node.
	cube_quad_tree_type::node_type *cube_child_node =
			parent_node_impl.cube_quad_tree_node.get_child_node(child_x_offset, child_y_offset);

	// If not then just create one.
	//
	// NOTE: After a while (with the user panning and zooming) we could end up with a lot
	// of nodes (because, unlike most situations, here there's no limit to how deep into the tree
	// the client can go - well, the limit would be how much viewport zoom is allowed in the GUI).
	// Note that we still recycle the tile textures though.
	//
	// TODO: We may want to periodically release nodes but that'll be hard because will first have
	// to determine which nodes are least-recently-used and also take into account that removing
	// an internal quad tree node will also remove its descendant nodes which may not be ready to go.
	// Probably not worth it though - memory usage is probably high enough to worry about.
	if (cube_child_node == NULL)
	{
		// The view transform for the current cube face.
		const GLTransform::non_null_ptr_to_const_type view_transform =
				d_cube_subdivision->get_view_transform(
						child_cube_quad_tree_location.get_node_location()->cube_face);

		// The projection transform for the current child cube quad tree node.
		const GLTransform::non_null_ptr_to_const_type projection_transform =
				d_cube_subdivision->get_projection_transform(
						child_cube_quad_tree_location.get_node_location()->quad_tree_depth/*level_of_detail*/,
						child_cube_quad_tree_location.get_node_location()->x_node_offset/*tile_u_offset*/,
						child_cube_quad_tree_location.get_node_location()->y_node_offset/*tile_v_offset*/);

		// Create a child quad tree node.
		d_cube_quad_tree->set_child_node(
				parent_node_impl.cube_quad_tree_node,
				child_x_offset,
				child_y_offset,
				d_cube_quad_tree->create_node(
						CubeQuadTreeNode(
								view_transform,
								projection_transform,
								get_level_of_detail(
										child_cube_quad_tree_location.get_node_location()->quad_tree_depth),
								d_texture_cache->allocate_volatile_object())));

		cube_child_node = parent_node_impl.cube_quad_tree_node.get_child_node(child_x_offset, child_y_offset);
	}

	return quad_tree_node_type(
			GPlatesUtils::non_null_intrusive_ptr<quad_tree_node_type::ImplInterface>(
					new QuadTreeNodeImpl(*cube_child_node, *this, child_cube_quad_tree_location)));
}


void
GPlatesOpenGL::GLMultiResolutionCubeReconstructedRaster::create_tile_texture(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_type &tile_texture)
{
	const GLCapabilities &capabilities = renderer.get_capabilities();

	const GLint internal_format = d_reconstructed_raster->get_target_texture_internal_format();
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
		// Binlinear filtering for GL_TEXTURE_MIN_FILTER and GL_TEXTURE_MAG_FILTER.
		tile_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		tile_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		// Specify anisotropic filtering if it's supported since we are not using mipmaps
		// and any textures rendered near the edge of the globe will get squashed a bit due to
		// the angle we are looking at them and anisotropic filtering will help here.
		//
		// NOTE: We don't enable anisotropic filtering for floating-point textures since earlier
		// hardware (that supports floating-point textures) only supports nearest filtering.
		if (capabilities.texture.gl_EXT_texture_filter_anisotropic)
		{
			const GLfloat anisotropy = capabilities.texture.gl_texture_max_anisotropy;
			tile_texture->gl_tex_parameterf(renderer, GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
		}
	}

	// Clamp texture coordinates to centre of edge texels -
	// it's easier for hardware to implement - and doesn't affect our calculations.
	if (capabilities.texture.gl_EXT_texture_edge_clamp ||
		capabilities.texture.gl_SGIS_texture_edge_clamp)
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
	GLUtils::check_gl_errors(GPLATES_ASSERTION_SOURCE);
}
