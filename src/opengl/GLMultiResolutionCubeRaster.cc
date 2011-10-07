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
#include "GLRenderer.h"
#include "GLProjectionUtils.h"
#include "GLUtils.h"
#include "GLViewport.h"

#include "global/AssertionFailureException.h"
#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "utils/Profile.h"


GPlatesOpenGL::GLMultiResolutionCubeRaster::GLMultiResolutionCubeRaster(
		const GLMultiResolutionRaster::non_null_ptr_type &multi_resolution_raster,
		const cube_subdivision_projection_transforms_cache_type::non_null_ptr_type &cube_subdivision_projection_transforms_cache,
		float source_raster_level_of_detail_bias) :
	d_multi_resolution_raster(multi_resolution_raster),
	d_cube_subdivision_projection_transforms_cache(cube_subdivision_projection_transforms_cache),
	d_tile_texel_dimension(cube_subdivision_projection_transforms_cache->get_tile_texel_dimension()),
	d_source_raster_level_of_detail_bias(source_raster_level_of_detail_bias),
	// Start with smallest size cache and just let the cache grow in size as needed...
	d_texture_cache(tile_texture_cache_type::create()),
	d_cube_quad_tree(cube_quad_tree_type::create())
{
	initialise_cube_quad_trees();
}


GPlatesOpenGL::GLTexture::shared_ptr_to_const_type
GPlatesOpenGL::GLMultiResolutionCubeRaster::get_tile_texture(
		GLRenderer &renderer,
		const QuadTreeNode &tile,
		cache_handle_type &cache_handle)
{
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
			create_texture(renderer, tile_texture->texture);
		}

		// Render the source raster into our tile texture.
		render_raster_data_into_tile_texture(
				tile,
				*tile_texture,
				renderer);
	}
	// Our texture wasn't recycled but see if it's still valid in case the source
	// raster changed the data underneath us.
	else if (!d_multi_resolution_raster->get_subject_token().is_observer_up_to_date(
				tile.d_source_texture_observer_token))
	{
		// Render the source raster into our tile texture.
		render_raster_data_into_tile_texture(
				tile,
				*tile_texture,
				renderer);
	}

	// The caller will cache this tile to keep it from being prematurely recycled by our caches.
	// Note that we can assign directly since the tile texture is already a boost::shared_ptr.
	cache_handle = tile_texture;

	return tile_texture->texture;
}


void
GPlatesOpenGL::GLMultiResolutionCubeRaster::render_raster_data_into_tile_texture(
		const QuadTreeNode &tile,
		TileTexture &tile_texture,
		GLRenderer &renderer)
{
	PROFILE_FUNC();

	// Begin rendering to a 2D render target texture.
	GLRenderer::Rgba8RenderTarget2DScope render_target_scope(renderer, tile_texture.texture);

	renderer.gl_viewport(0, 0, d_tile_texel_dimension, d_tile_texel_dimension);

	renderer.gl_clear_color(); // Clear colour to all zeros.
	renderer.gl_clear(GL_COLOR_BUFFER_BIT); // Clear only the colour buffer.

	renderer.gl_load_matrix(GL_MODELVIEW, tile.d_view_transform->get_matrix());
	renderer.gl_load_matrix(GL_PROJECTION, tile.d_projection_transform->get_matrix());

	// Get the source raster to render into the render target using the view frustum
	// we have provided. We have already cached the visible source raster tiles that need to be
	// rendered into our frustum to save it a bit of culling work.
	tile_texture.source_cache_handle =
			d_multi_resolution_raster->render(
					renderer,
					tile.d_src_raster_tiles);

	// This tile texture is now update-to-date with respect to the source multi-resolution raster.
	d_multi_resolution_raster->get_subject_token().update_observer(
			tile.d_source_texture_observer_token);
}


// We use macros in <GL/glew.h> that contain old-style casts.
DISABLE_GCC_WARNING("-Wold-style-cast")

void
GPlatesOpenGL::GLMultiResolutionCubeRaster::create_texture(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_type &texture)
{
	PROFILE_FUNC();

	//
	// No mipmaps needed so we specify no mipmap filtering.
	// We're not using mipmaps because our cube mapping does not have much distortion
	// unlike global rectangular lat/lon rasters that squash near the poles.
	//
	texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Clamp texture coordinates to centre of edge texels -
	// it's easier for hardware to implement - and doesn't affect our calculations.
	if (GLEW_EXT_texture_edge_clamp)
	{
		texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	else
	{
		texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}

	// Specify anisotropic filtering if it's supported since we are not using mipmaps
	// and any textures rendered near the edge of the globe will get squashed a bit due to
	// the angle we are looking at them and anisotropic filtering will help here.
	if (GLEW_EXT_texture_filter_anisotropic)
	{
		const GLfloat anisotropy = GLContext::get_parameters().texture.gl_texture_max_anisotropy;
		texture->gl_tex_parameterf(renderer, GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
	}

	// Create the texture but don't load any data into it.
	// Leave it uninitialised because we will be rendering into it to initialise it.
	texture->gl_tex_image_2D(renderer, GL_TEXTURE_2D, 0, GL_RGBA8,
			d_tile_texel_dimension, d_tile_texel_dimension,
			0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	// Check there are no OpenGL errors.
	GLUtils::assert_no_gl_errors(GPLATES_ASSERTION_SOURCE);
}

ENABLE_GCC_WARNING("-Wold-style-cast")


void
GPlatesOpenGL::GLMultiResolutionCubeRaster::initialise_cube_quad_trees()
{
	for (unsigned int face = 0; face < 6; ++face)
	{
		GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face =
				static_cast<GPlatesMaths::CubeCoordinateFrame::CubeFaceType>(face);

		// The viewport is the same for all subdivisions since they use the same tile texture dimension.
		const GLViewport viewport(0, 0, d_tile_texel_dimension, d_tile_texel_dimension);

		// Get the loose bounds quad tree root node.
		const cube_subdivision_projection_transforms_cache_type::node_reference_type
				projection_transform_quad_tree_root =
						d_cube_subdivision_projection_transforms_cache->get_quad_tree_root_node(cube_face);

		// Recursively generate a quad tree for the current cube face.
		boost::optional<cube_quad_tree_type::node_type::ptr_type> quad_tree_root_node =
				create_quad_tree_node(
						cube_face,
						viewport,
						projection_transform_quad_tree_root);

		// If the source raster covered any part of the current cube face.
		if (quad_tree_root_node)
		{
			d_cube_quad_tree->set_quad_tree_root_node(cube_face, quad_tree_root_node.get());
		}
	}
}


boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRaster::cube_quad_tree_type::node_type::ptr_type>
GPlatesOpenGL::GLMultiResolutionCubeRaster::create_quad_tree_node(
		GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face,
		const GLViewport &viewport,
		const cube_subdivision_projection_transforms_cache_type::node_reference_type &projection_transform_quad_tree_node)
{
	//
	// Setup the model-view and projection matrices and setup the viewport.
	// These are required when the source raster looks for tiles visible in the view frustum
	// of our quad tree subdivision cell (the viewport is used to determine the correct
	// level-of-detail tiles to use).
	//

	// Set the projection transform.
	const GLTransform::non_null_ptr_to_const_type projection_transform =
			d_cube_subdivision_projection_transforms_cache->get_cached_element(projection_transform_quad_tree_node)
					->get_projection_transform();

	// Set the view transform.
	const GLTransform::non_null_ptr_to_const_type view_transform =
			d_cube_subdivision_projection_transforms_cache->get_view_transform(cube_face);

	// Get the source tiles that are visible in the current view frustum.
	std::vector<GLMultiResolutionRaster::tile_handle_type> source_raster_tile_handles;
	const float source_raster_level_of_detail = d_multi_resolution_raster->get_visible_tiles(
			source_raster_tile_handles,
			viewport,
			view_transform->get_matrix(),
			projection_transform->get_matrix(),
			d_source_raster_level_of_detail_bias);

	// If there are no tiles it means the source raster does not have global extents
	// and we are looking at a part of the globe not covered by it.
	if (source_raster_tile_handles.empty())
	{
		return boost::none;
	}

	// Create a quad tree node.
	cube_quad_tree_type::node_type::ptr_type quad_tree_node =
			d_cube_quad_tree->create_node(
					QuadTreeNode(
							projection_transform,
							view_transform,
							d_texture_cache->allocate_volatile_object()));

	// Initialise the quad tree node's source tiles.
	quad_tree_node->get_element().d_src_raster_tiles.swap(source_raster_tile_handles);

	// If we have reached the highest resolution level of detail in the source raster
	// then we don't need to create any child quad tree nodes.
	// When the LOD first goes negative it means we have a render texture / render frustum
	// that has enough resolution to contain the highest resolution the source raster can provide.
	if (source_raster_level_of_detail <= 0)
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
			const cube_subdivision_projection_transforms_cache_type::node_reference_type
					projection_transform_child_quad_tree_node =
							d_cube_subdivision_projection_transforms_cache->get_child_node(
									projection_transform_quad_tree_node,
									child_u_offset,
									child_v_offset);

			// Returns a non-null child if child quad tree node is covered by source raster.
			const boost::optional<cube_quad_tree_type::node_type::ptr_type> child_node =
					create_quad_tree_node(
							cube_face,
							viewport,
							projection_transform_child_quad_tree_node);

			// Add the child node if one was created.
			if (child_node)
			{
				// Add the node as a child.
				d_cube_quad_tree->set_child_node(
						*quad_tree_node, child_u_offset, child_v_offset, child_node.get());

				// Since we've given the quad tree node a child it is now an internal node.
				quad_tree_node->get_element().set_internal_node();
			}
		}
	}

	return quad_tree_node;
}
