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

namespace
{
	/**
	 * Adjusts the specified tile texel dimension according to the OpenGL capabilities of the run-time system.
	 */
	std::size_t
	initialise_tile_texel_dimension(
			std::size_t tile_texel_dimension)
	{
		// The tile dimension should be a power-of-two *if* 'GL_ARB_texture_non_power_of_two' is *not* supported.
		if (!GLEW_ARB_texture_non_power_of_two)
		{
			tile_texel_dimension = GPlatesUtils::Base2::next_power_of_two(tile_texel_dimension);
		}

		// And the tile dimension should not be larger than the maximum texture dimension.
		if (tile_texel_dimension > GPlatesOpenGL::GLContext::get_parameters().texture.gl_max_texture_size)
		{
			tile_texel_dimension = GPlatesOpenGL::GLContext::get_parameters().texture.gl_max_texture_size;
		}

		return tile_texel_dimension;
	}
}


std::size_t
GPlatesOpenGL::GLMultiResolutionCubeReconstructedRaster::get_default_tile_texel_dimension()
{
	return GLContext::get_parameters().framebuffer.gl_EXT_framebuffer_object ? 512 : 256;
}


GPlatesOpenGL::GLMultiResolutionCubeReconstructedRaster::GLMultiResolutionCubeReconstructedRaster(
		GLRenderer &renderer,
		const GLMultiResolutionStaticPolygonReconstructedRaster::non_null_ptr_type &source_reconstructed_raster,
		std::size_t tile_texel_dimension,
		bool cache_tile_textures) :
	d_reconstructed_raster(source_reconstructed_raster),
	d_tile_texel_dimension(initialise_tile_texel_dimension(tile_texel_dimension)),
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


void
GPlatesOpenGL::GLMultiResolutionCubeReconstructedRaster::set_world_transform(
		const GLMatrix &world_transform)
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
					std::auto_ptr<TileTexture>(new TileTexture(renderer)),
					// Called whenever tile texture is returned to the cache...
					boost::bind(&TileTexture::returned_to_cache, _1));

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
	// we have provided.
	const bool rendered = d_reconstructed_raster->render(renderer, tile_texture.source_cache_handle);

	// This tile texture is now update-to-date with respect to the source raster.
	d_reconstructed_raster->get_subject_token().update_observer(
			tile.d_source_texture_observer_token);

	// Return true if anything was rendered into the current node's tile.
	return rendered;
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
	//
	// TODO: We may want to periodically release nodes but that'll be hard because will first have
	// to determine which nodes are least-recently-used and also take into account that removing
	// an internal quad tree node will also remove its descendant nodes which may not be ready to go.
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
								d_texture_cache->allocate_volatile_object())));

		cube_child_node = parent_node_impl.cube_quad_tree_node.get_child_node(child_x_offset, child_y_offset);
	}

	return quad_tree_node_type(
			GPlatesUtils::non_null_intrusive_ptr<quad_tree_node_type::ImplInterface>(
					new QuadTreeNodeImpl(*cube_child_node, *this, child_cube_quad_tree_location)));
}


// We use macros in <GL/glew.h> that contain old-style casts.
DISABLE_GCC_WARNING("-Wold-style-cast")

void
GPlatesOpenGL::GLMultiResolutionCubeReconstructedRaster::create_tile_texture(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_type &tile_texture)
{
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
		if (GLEW_EXT_texture_filter_anisotropic)
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

ENABLE_GCC_WARNING("-Wold-style-cast")
