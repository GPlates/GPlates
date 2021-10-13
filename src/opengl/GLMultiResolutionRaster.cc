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
#include <limits>
#include <boost/cast.hpp>
#include <boost/foreach.hpp>
/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>
#include <opengl/OpenGL.h>

#include "GLMultiResolutionRaster.h"

#include "GLBindTextureState.h"
#include "GLBlendState.h"
#include "GLCompositeStateSet.h"
#include "GLContext.h"
#include "GLFragmentTestStates.h"
#include "GLIntersect.h"
#include "GLMaskBuffersState.h"
#include "GLRenderer.h"
#include "GLTextureEnvironmentState.h"
#include "GLTransformState.h"
#include "GLUtils.h"
#include "GLVertexArrayDrawable.h"

#include "global/AssertionFailureException.h"
#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "utils/Profile.h"

namespace
{
	/**
	 * Vertex information stored in an OpenGL vertex array.
	 */
	struct Vertex
	{
		//! Vertex position.
		GLfloat x, y, z;

		//! Vertex texture coordinates.
		GLfloat u, v;
	};

	//! The inverse of log(2.0).
	const float INVERSE_LOG2 = 1.0 / std::log(2.0);
}


GPlatesOpenGL::GLMultiResolutionRaster::non_null_ptr_type
GPlatesOpenGL::GLMultiResolutionRaster::create(
		const GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type &georeferencing,
		const GLMultiResolutionRasterSource::non_null_ptr_type &raster_source,
		const GLTextureResourceManager::shared_ptr_type &texture_resource_manager,
		RasterScanlineOrderType raster_scanline_order)
{
	return non_null_ptr_type(new GLMultiResolutionRaster(
			georeferencing,
			raster_source,
			texture_resource_manager,
			raster_scanline_order));
}


GPlatesOpenGL::GLMultiResolutionRaster::GLMultiResolutionRaster(
		const GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type &georeferencing,
		const GLMultiResolutionRasterSource::non_null_ptr_type &raster_source,
		const GLTextureResourceManager::shared_ptr_type &texture_resource_manager,
		RasterScanlineOrderType raster_scanline_order) :
	d_georeferencing(georeferencing),
	d_raster_source(raster_source),
	d_raster_source_valid_token(raster_source->get_current_valid_token()),
	// The raster dimensions (the highest resolution level-of-detail).
	d_raster_width(raster_source->get_raster_width()),
	d_raster_height(raster_source->get_raster_height()),
	d_raster_scanline_order(raster_scanline_order),
	d_tile_texel_dimension(raster_source->get_tile_texel_dimension()),
	// The parentheses around min are to prevent the windows min macros
	// from stuffing numeric_limits' min.
	d_max_highest_resolution_texel_size_on_unit_sphere((std::numeric_limits<float>::min)()),
	// FIXME: Change the max number cached textures to a value that reflects
	// the tile dimensions and the viewport dimensions - the latter will vary so
	// we'll need to have the ability to change the max number cached textures dynamically
	// or perhaps just support the maximum screen resolution of any monitor (not a good idea).
	// For now it's 200 which is about 50Mb...
	d_texture_cache(create_texture_cache(200, texture_resource_manager)),
	// FIXME: Change the max number of cached vertex arrays to a value that reflects the
	// memory used to stored vertices for a tile - should also balance memory used for
	// textures with memory used for vertices.
	// For now it's 400 which is about 9Mb...
	d_vertex_array_cache(create_vertex_array_cache(400))
{
	// Create the levels of details and within each one create an oriented bounding box
	// tree (used to quickly find visible tiles) where the drawable tiles are in the
	// leaf nodes of the OBB tree.
	initialise_level_of_detail_pyramid();
}


GPlatesOpenGL::GLTextureUtils::ValidToken
GPlatesOpenGL::GLMultiResolutionRaster::get_current_valid_token()
{
	update_raster_source_valid_token();

	// We'll just use the valid token of the raster source - if they don't change then neither do we.
	// If we had two inputs sources then we'd have to have our own valid token.
	return d_raster_source_valid_token;
}


void
GPlatesOpenGL::GLMultiResolutionRaster::update_raster_source_valid_token()
{
	d_raster_source_valid_token = d_raster_source->get_current_valid_token();
}


void
GPlatesOpenGL::GLMultiResolutionRaster::render(
		GLRenderer &renderer,
		float level_of_detail_bias)
{
	// Get the tiles visible in the view frustum of the render target in 'renderer'.
	std::vector<tile_handle_type> visible_tiles;
	get_visible_tiles(renderer.get_transform_state(), visible_tiles, level_of_detail_bias);

	render(renderer, visible_tiles);
}


void
GPlatesOpenGL::GLMultiResolutionRaster::render(
		GLRenderer &renderer,
		const std::vector<tile_handle_type> &tiles)
{
	// Make sure our cached version of the raster source's valid token is up to date
	// so our texture tiles can decide whether they need to reload their caches.
	update_raster_source_valid_token();

	GPlatesOpenGL::GLCompositeStateSet::non_null_ptr_type state_set =
			GPlatesOpenGL::GLCompositeStateSet::create();

	// Enable texturing and set the texture function on texture unit 0.
	GPlatesOpenGL::GLTextureEnvironmentState::non_null_ptr_type tex_env_state =
			GPlatesOpenGL::GLTextureEnvironmentState::create();
	tex_env_state->gl_enable_texture_2D(GL_TRUE).gl_tex_env_mode(GL_REPLACE);
	state_set->add_state_set(tex_env_state);

	// Set the alpha-blend state in case texture has transparency.
	GPlatesOpenGL::GLBlendState::non_null_ptr_type blend_state =
			GPlatesOpenGL::GLBlendState::create();
	blend_state->gl_enable(GL_TRUE).gl_blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	state_set->add_state_set(blend_state);

	renderer.push_state_set(state_set);

	// Create a render operation for each visible tile.
	BOOST_FOREACH(GLMultiResolutionRaster::tile_handle_type tile_handle, tiles)
	{
		Tile tile = get_tile(tile_handle, renderer);

		// Create a state set that binds the tile texture to texture unit 0.
		GLBindTextureState::non_null_ptr_type bind_tile_texture = GLBindTextureState::create();
		bind_tile_texture->gl_bind_texture(GL_TEXTURE_2D, tile.get_texture());

		// Push the state set onto the state graph.
		renderer.push_state_set(bind_tile_texture);

		// Add the drawable to the current render target.
		renderer.add_drawable(tile.get_drawable());

		// Pop the state set.
		renderer.pop_state_set();
	}

	renderer.pop_state_set();
}


float
GPlatesOpenGL::GLMultiResolutionRaster::get_visible_tiles(
		const GLTransformState &transform_state,
		std::vector<tile_handle_type> &visible_tiles,
		float level_of_detail_bias) const
{
	// There should be levels of detail.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			!d_level_of_detail_pyramid.empty(),
			GPLATES_ASSERTION_SOURCE);

	// Get the level-of-detail based on the size of viewport pixels projected onto the globe.
	float level_of_detail_factor;
	const unsigned int level_of_detail =
			get_level_of_detail(transform_state, level_of_detail_bias, level_of_detail_factor);
	const LevelOfDetail &lod = *d_level_of_detail_pyramid[level_of_detail];

	//
	// Traverse the OBB tree of the level-of-detail and gather of list of tiles that
	// are visible in the view frustum (as defined by 'transform_state').
	//

	// First get the view frustum planes.
	const GLTransformState::FrustumPlanes &frustum_planes =
			transform_state.get_current_frustum_planes_in_model_space();
	// There are six frustum planes initially active.
	const boost::uint32_t frustum_plane_mask = 0x3f; // 111111 in binary

	// Get the root OBB tree node of the level-of-detail.
	const LevelOfDetail::OBBTreeNode &lod_root_obb_tree_node =
			lod.get_obb_tree_node(lod.obb_tree_root_node_index);

	// Recursively traverse the OBB tree to find visible tiles.
	get_visible_tiles(frustum_planes, frustum_plane_mask, lod, lod_root_obb_tree_node, visible_tiles);

	return level_of_detail_factor;
}


unsigned int
GPlatesOpenGL::GLMultiResolutionRaster::get_level_of_detail(
		const GLTransformState &transform_state,
		float level_of_detail_bias,
		float &level_of_detail_factor) const
{
	// Get the minimum size of a pixel in the current viewport when projected
	// onto the unit sphere (in model space).
	const double min_pixel_size_on_unit_sphere =
			transform_state.get_min_pixel_size_on_unit_sphere();

	// Calculate the level-of-detail.
	// This is the equivalent of:
	//
	//    t = t0 * 2 ^ (lod - lod_bias)
	//
	// ...where 't0' is the texel size of the *highest* resolution level-of-detail and
	// 't' is the projected size of a pixel of the viewport. And 'lod_bias' is used
	// by clients to allow a the largest texel in a drawn texture to be larger than
	// a pixel in the viewport (which can result in blockiness in places in the rendered scene).
	//
	// Note: we return the un-clamped floating-point level-of-detail so clients of this class
	// can see if they need a higher resolution render-texture, for example, to render
	// our raster into - so in that case they'd keep increasing their render-target
	// resolution or decreasing their render target view frustum until the level-of-detail
	// became negative.
	level_of_detail_factor = level_of_detail_bias + INVERSE_LOG2 *
			(std::log(min_pixel_size_on_unit_sphere) -
					std::log(d_max_highest_resolution_texel_size_on_unit_sphere));

	// Truncate level-of-detail factor and clamp to the range of our levels of details.
	int level_of_detail = static_cast<int>(level_of_detail_factor);
	// Clamp to highest resolution level of detail.
	if (level_of_detail < 0)
	{
		// LOD bias aside, if we get here then even the highest resolution level-of-detail did not
		// have enough resolution for the current viewport and projection but it'll have to do
		// since its the highest resolution we have to offer.
		// This is where the user will start to see magnification of the raster.
		level_of_detail = 0;
	}
	// Clamp to lowest resolution level of detail.
	if (level_of_detail >= boost::numeric_cast<int>(d_level_of_detail_pyramid.size()))
	{
		// LOD bias aside, if we get there then even our lowest resolution level of detail
		// had too much resolution - but this is pretty unlikely for all but the very
		// smallest of viewports.
		level_of_detail = d_level_of_detail_pyramid.size() - 1;
	}

	return level_of_detail;
}


void
GPlatesOpenGL::GLMultiResolutionRaster::get_visible_tiles(
		const GLTransformState::FrustumPlanes &frustum_planes,
		boost::uint32_t frustum_plane_mask,
		const LevelOfDetail &lod,
		const LevelOfDetail::OBBTreeNode &obb_tree_node,
		std::vector<tile_handle_type> &visible_tiles) const
{
	// If the frustum plane mask is zero then it means we are entirely inside the view frustum.
	// So only test for intersection if the mask is non-zero.
	if (frustum_plane_mask != 0)
	{
		// See if the OBB of the current OBB tree node intersects the view frustum.
		boost::optional<boost::uint32_t> out_frustum_plane_mask =
				GLIntersect::intersect_OBB_frustum(
						obb_tree_node.bounding_box,
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

	// See if it's a OBB tree *leaf* node.
	if (obb_tree_node.is_leaf_node)
	{
		// This leaf node is visible in the view frustum so
		// add its tile to the list of visible tiles.
		visible_tiles.push_back(obb_tree_node.tile);
		return;
	}
	// It's an *internal* OBB tree node.

	// Traverse the child nodes.
	get_visible_tiles(
			frustum_planes,
			frustum_plane_mask,
			lod,
			lod.get_obb_tree_node(obb_tree_node.child_node_indices[0]),
			visible_tiles);
	get_visible_tiles(
			frustum_planes,
			frustum_plane_mask,
			lod,
			lod.get_obb_tree_node(obb_tree_node.child_node_indices[1]),
			visible_tiles);
}


GPlatesOpenGL::GLMultiResolutionRaster::Tile
GPlatesOpenGL::GLMultiResolutionRaster::get_tile(
		tile_handle_type tile_handle,
		GLRenderer &renderer)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			tile_handle < d_tiles.size(),
			GPLATES_ASSERTION_SOURCE);

	const LevelOfDetailTile &lod_tile = *d_tiles[tile_handle];

	// Get the texture for the tile.
	GLTexture::shared_ptr_to_const_type texture =
			get_tile_texture(lod_tile, renderer);

	// Get the vertices for the tile.
	GLVertexArray::shared_ptr_to_const_type vertex_array = get_tile_vertex_array(lod_tile);

	// Return the tile to the caller.
	// Each tile has its own vertices and texture but shares the same triangles (vertex indices).
	return Tile(vertex_array, lod_tile.vertex_element_array, texture);
}


GPlatesOpenGL::GLTexture::shared_ptr_to_const_type
GPlatesOpenGL::GLMultiResolutionRaster::get_tile_texture(
		const LevelOfDetailTile &lod_tile,
		GLRenderer &renderer)
{
	// See if we've previously created our tile texture and
	// see if it hasn't been recycled by the texture cache.
	boost::optional<GLTexture::shared_ptr_type> tile_texture = lod_tile.texture.get_object();
	if (!tile_texture)
	{
		// We need to allocate a new texture from the texture cache and fill it with data.
		const std::pair<GLVolatileTexture, bool/*recycled*/> volatile_texture_result =
				d_texture_cache->allocate_object();

		// Extract allocation results.
		lod_tile.texture = volatile_texture_result.first;
		const bool texture_was_recycled = volatile_texture_result.second;

		// Get the tile texture again - this time it should have a valid texture.
		tile_texture = lod_tile.texture.get_object();
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				tile_texture,
				GPLATES_ASSERTION_SOURCE);

		// If the texture is not recycled then its a newly allocated texture so we need
		// to create it in OpenGL before we can load data into it.
		if (!texture_was_recycled)
		{
			create_tile_texture(tile_texture.get());
		}

		load_raster_data_into_tile_texture(
				lod_tile,
				tile_texture.get(),
				renderer);

		return tile_texture.get();
	}

	// Our texture wasn't recycled but see if it's still valid in case the source
	// raster changed the data underneath us.
	if (!lod_tile.source_texture_valid_token.is_still_valid(d_raster_source_valid_token))
	{
		// Load the data into the texture.
		load_raster_data_into_tile_texture(
				lod_tile, tile_texture.get(),
				renderer);
	}

	return tile_texture.get();
}


void
GPlatesOpenGL::GLMultiResolutionRaster::load_raster_data_into_tile_texture(
		const LevelOfDetailTile &lod_tile,
		const GLTexture::shared_ptr_type &texture,
		GLRenderer &renderer)
{
	// Get our source to load data into the texture.
	d_raster_source->load_tile(
			lod_tile.lod_level,
			lod_tile.u_lod_texel_offset,
			lod_tile.v_lod_texel_offset,
			lod_tile.num_u_lod_texels,
			lod_tile.num_v_lod_texels,
			texture,
			renderer);

	// This tile texture is now update-to-date.
	lod_tile.source_texture_valid_token = d_raster_source_valid_token;
}


// We use macros in <GL/glew.h> that contain old-style casts.
DISABLE_GCC_WARNING("-Wold-style-cast")

void
GPlatesOpenGL::GLMultiResolutionRaster::create_tile_texture(
		const GLTexture::shared_ptr_type &texture)
{
	//
	// NOTE: Direct calls to OpenGL are made below.
	//
	// Below we make direct calls to OpenGL once we've bound the texture.
	// This is in contrast to storing OpenGL commands in the render graph to be
	// executed later. The reason for this is the calls we make affect the
	// bound texture object's state and not the general OpenGL state. Texture object's
	// are one of those places in OpenGL where you can set state and then subsequent
	// binds of that texture object will set all that state into OpenGL.
	//

	// Bind the texture so its the current texture.
	// Here we actually make a direct OpenGL call to bind the texture to the currently
	// active texture unit. It doesn't matter what the current texture unit is because
	// the only reason we're binding the texture object is so we can set its state =
	// so that subsequent binds of this texture object, when we render the scene graph,
	// will set that state to OpenGL.
	texture->gl_bind_texture(GL_TEXTURE_2D);

	// If the auto-generate mipmaps OpenGL extension is supported then have mipmaps generated
	// automatically for us and specify a mipmap minification filter,
	// otherwise don't use mipmaps (and instead specify a non-mipmap minification filter).
	// A lot of cards have support for this extension.
	//
	// UPDATE: Generating mipmaps is causing problems when the input source is an age grid mask.
	// This is probably because that input is not a regularly loaded texture (loaded from CPU).
	// Instead it is a texture that's been rendered to by the GPU (via a render target).
	// In this case the auto generation of mipmaps is probably a little less clear since it
	// interacts with other specifications on mipmap rendering such as the frame buffer object
	// extension (used by GPlates where possible for render targets) which has its own
	// mipmap support.
	// Best to avoid auto generation of mipmaps - we don't really need it anyway since
	// our texture already matches pretty closely texel-to-pixel (texture -> viewport) since
	// we have our own mipmapped raster tiles via proxied rasters. Also we turn on anisotropic
	// filtering which will reduce any aliasing near the horizon of the globe.
	// Turning off auto-mipmap-generation will also give us a small speed boost.
#if 0
	if (GLEW_SGIS_generate_mipmap)
	{
		// Mipmaps will be generated automatically when the level 0 image is modified.
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}
#else
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
#endif

	// No mipmap filter for the GL_TEXTURE_MAG_FILTER filter regardless.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Specify anisotropic filtering if it's supported since rasters near the north or
	// south pole will exhibit squashing along the longitude, but not the latitude, direction.
	// Regular isotropic filtering will just reduce texel resolution equally along both
	// directions and reduce the visual sharpness that we want to retain in the latitude direction.
	if (GLEW_EXT_texture_filter_anisotropic)
	{
		const GLfloat anisotropy = GLContext::get_texture_parameters().gl_texture_max_anisotropy_EXT;
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	// Create the texture in OpenGL - this actually creates the texture without any data.
	// We'll be getting our raster source to load image data into the texture.
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
			d_tile_texel_dimension, d_tile_texel_dimension,
			0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	// Check there are no OpenGL errors.
	GLUtils::assert_no_gl_errors(GPLATES_ASSERTION_SOURCE);
}

ENABLE_GCC_WARNING("-Wold-style-cast")


GPlatesOpenGL::GLVertexArray::shared_ptr_to_const_type
GPlatesOpenGL::GLMultiResolutionRaster::get_tile_vertex_array(
		const LevelOfDetailTile &lod_tile)
{
	// See if we've previously created our tile vertices and
	// see if they haven't been recycled by the vertex array cache.
	boost::optional<GLVertexArray::shared_ptr_type> tile_vertices =
			lod_tile.vertex_array.get_object();
	if (!tile_vertices)
	{
		// We need to allocate a new vertex array from the cache and fill it with data.
		const std::pair<GLVolatileVertexArray, bool/*recycled*/> volatile_vertex_array_result =
				d_vertex_array_cache->allocate_object();

		// Extract allocation results.
		lod_tile.vertex_array = volatile_vertex_array_result.first;
		const bool vertex_array_was_recycled = volatile_vertex_array_result.second;

		// Get the tile vertices again - this time it should have a valid vertex array.
		tile_vertices = lod_tile.vertex_array.get_object();
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				tile_vertices,
				GPLATES_ASSERTION_SOURCE);

		load_vertices_into_tile_vertex_array(lod_tile, tile_vertices.get(), vertex_array_was_recycled);
	}

	return tile_vertices.get();
}


void
GPlatesOpenGL::GLMultiResolutionRaster::load_vertices_into_tile_vertex_array(
		const LevelOfDetailTile &lod_tile,
		const GLVertexArray::shared_ptr_type &vertex_array,
		bool vertex_array_was_recycled)
{
	// Total number of vertices in this tile.
	const unsigned int num_vertices_in_tile =
			lod_tile.x_num_vertices * lod_tile.y_num_vertices;

	// Allocate memory for the vertex array.
	boost::shared_array<Vertex> vertex_array_data(new Vertex[num_vertices_in_tile]);

	//
	// Initialise the vertices
	//

	// A write pointer to the vertex array.
	Vertex *vertices = vertex_array_data.get();

	// Set up some variables before initialising the vertices.
	const double inverse_x_num_quads = 1.0 / (lod_tile.x_num_vertices - 1);
	const double inverse_y_num_quads = 1.0 / (lod_tile.y_num_vertices - 1);
	const double x_pixels_per_quad =
			inverse_x_num_quads * (lod_tile.x_geo_end - lod_tile.x_geo_start);
	const double y_pixels_per_quad =
			inverse_y_num_quads * (lod_tile.y_geo_end - lod_tile.y_geo_start);
	const double u_increment_per_quad = inverse_x_num_quads * (lod_tile.u_end - lod_tile.u_start);
	const double v_increment_per_quad = inverse_y_num_quads * (lod_tile.v_end - lod_tile.v_start);

	// Calculate the vertices.
	for (unsigned int j = 0; j < lod_tile.y_num_vertices; ++j)
	{
		// NOTE: The positions of the last row of vertices should
		// match up identically with the adjacent tile otherwise
		// cracks will appear in the raster along tile edges and
		// missing pixels can show up intermittently.
		const double y = (j == lod_tile.y_num_vertices - 1)
				? lod_tile.y_geo_end
				: lod_tile.y_geo_start + j * y_pixels_per_quad;

		// The 'v' texture coordinate.
		const double v = lod_tile.v_start + j * v_increment_per_quad;

		for (unsigned int i = 0; i < lod_tile.x_num_vertices; ++i)
		{
			// NOTE: The positions of the last column of vertices should
			// match up identically with the adjacent tile otherwise
			// cracks will appear in the raster along tile edges and
			// missing pixels can show up intermittently.
			const double x = (i == lod_tile.x_num_vertices - 1)
					? lod_tile.x_geo_end
					: lod_tile.x_geo_start + i * x_pixels_per_quad;

			// Convert from pixel coordinates to a position on the unit globe.
			const GPlatesMaths::PointOnSphere vertex_position =
					convert_pixel_coord_to_geographic_coord(x, y);

			// Store the vertex position in the vertex array.
			vertices->x = vertex_position.position_vector().x().dval();
			vertices->y = vertex_position.position_vector().y().dval();
			vertices->z = vertex_position.position_vector().z().dval();

			// The 'u' texture coordinate.
			const double u = lod_tile.u_start + i * u_increment_per_quad;

			// Store the texture coordinates in the vertex array.
			vertices->u = u;
			vertices->v = v;

			// Move to the next vertex.
			++vertices;
		}
	}

	// Set the vertex array.
	vertex_array->set_array_data(vertex_array_data);

	// If the vertex array was not recycled then it was created for the first time and
	// we need to set the vertex array pointers and enable the appropriate client state.
	// We don't need to do this for a recycled vertex array because it's already been done.
	if (!vertex_array_was_recycled)
	{
		vertex_array->gl_enable_client_state(GL_VERTEX_ARRAY);
		vertex_array->gl_enable_client_state(GL_TEXTURE_COORD_ARRAY); // For texture unit zero

		vertex_array->gl_vertex_pointer(3, GL_FLOAT, sizeof(Vertex), 0);
		vertex_array->gl_tex_coord_pointer(2, GL_FLOAT, sizeof(Vertex), 3 * sizeof(GLfloat));
	}
}


void
GPlatesOpenGL::GLMultiResolutionRaster::initialise_level_of_detail_pyramid()
{
	// The dimension of texels that contribute to a level-of-detail
	// (starting with the highest resolution level-of-detail).
	unsigned int lod_texel_width = d_raster_width;
	unsigned int lod_texel_height = d_raster_height;

	// Generate the levels of detail starting with the
	// highest resolution (original raster) at level 0.
	for (unsigned int lod_level = 0; ; ++lod_level)
	{
		// Create a level-of-detail.
		LevelOfDetail::non_null_ptr_type level_of_detail = create_level_of_detail(lod_level);

		// Add to our level-of-detail pyramid.
		d_level_of_detail_pyramid.push_back(level_of_detail);

		// Keep generating coarser level-of-details until the width and height
		// fit within a square tile of size:
		//   'tile_texel_dimension' x 'tile_texel_dimension'
		if (lod_texel_width <= d_tile_texel_dimension &&
			lod_texel_height <= d_tile_texel_dimension)
		{
			break;
		}

		// Get the raster dimensions of the next level-of-detail.
		// The '+1' is to ensure the texels of the next level-of-detail
		// cover the texels of the current level-of-detail.
		// This can mean that the next level-of-detail texels actually
		// cover a slightly larger area on the globe than the current level-of-detail.
		//
		// For example:
		// Level 0: 5x5
		// Level 1: 3x3 (covers equivalent of 6x6 level 0 texels)
		// Level 2: 2x2 (covers equivalent of 4x4 level 1 texels or 8x8 level 0 texels)
		// Level 3: 1x1 (covers same area as level 2)
		//
		lod_texel_width = (lod_texel_width + 1) / 2;
		lod_texel_height = (lod_texel_height + 1) / 2;
	}
}


GPlatesOpenGL::GLMultiResolutionRaster::LevelOfDetail::non_null_ptr_type
GPlatesOpenGL::GLMultiResolutionRaster::create_level_of_detail(
		const unsigned int lod_level)
{
	// Allocate on the heap to avoid lots of copying when it's put in a std::vector.
	LevelOfDetail::non_null_ptr_type level_of_detail = LevelOfDetail::create(lod_level);

	/*
	 * Can generate OBB tree by starting at root node and dividing all tiles into two groups of tiles
	 * such that each group is still a rectangular arrangement of tiles and recursively descending.
	 * For example, if we start out with 5.6 x 10.9 tiles then take the dimension with
	 * the largest number of tiles and divide that, for example:
	 *                           5.6x10.8
	 *                        /           \
	 *                 5.6x5                 5.6x5.8
	 *              /       \               /      \
	 *         3x5           2.6x5      5.6x3       5.6x2.8
	 *        /   \         /    \      /   \       /     \
	 *     3x3     3x2  2.6x3  2.6x2  3x3  2.6x3  3x2.8   2.6x2.8
	 *
	 * The second level-of-detail looks like this:
	 *
	 *                           2.8x5.4
	 *                        /           \
	 *                 2.8x3                 2.8x2.4
	 *              /       \               /      \
	 *         1x3           1.8x3      1x2.4       1.8x2.4
	 *
	 * ...and hence needs a different OBB tree since the tiles at the leaves of the tree
	 * are a different size for each level-of-detail and so the partitioning line between
	 * two child nodes (of any parent internal node) will differ.
	 */

	// The root OBB tree node covers the entire raster.
	const unsigned int x_geo_start = 0;
	const unsigned int x_geo_end = d_raster_width;
	const unsigned int y_geo_start = 0;
	const unsigned int y_geo_end = d_raster_height;

	// Recursively create an OBB tree starting at the root.
	level_of_detail->obb_tree_root_node_index = create_obb_tree(
			*level_of_detail, x_geo_start, x_geo_end, y_geo_start, y_geo_end);

	return level_of_detail;
}


std::size_t
GPlatesOpenGL::GLMultiResolutionRaster::create_obb_tree(
		LevelOfDetail &level_of_detail,
		const unsigned int x_geo_start,
		const unsigned int x_geo_end,
		const unsigned int y_geo_start,
		const unsigned int y_geo_end)
{
	// Level-of-detail.
	const unsigned int lod_level = level_of_detail.lod_level;

	// Texels in this level-of-detail have dimensions 'lod_factor' times larger than the
	// original raster pixels when projected on the globe (they cover a larger area on the globe).
	const unsigned int lod_factor = 1 << lod_level;

	// The size of a tile (at the current level-of-detail) in units of pixels of
	// the original raster. Pixels and geo coordinates are the same.
	const unsigned int tile_geo_dimension = d_tile_texel_dimension * lod_factor;

	// The start x coordinate should be an integer multiple of the tile dimension.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			(x_geo_start % tile_geo_dimension) == 0,
			GPLATES_ASSERTION_SOURCE);
	if (d_raster_scanline_order == TOP_TO_BOTTOM)
	{
		// The start y coordinate should be an integer multiple of the tile dimension.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				(y_geo_start % tile_geo_dimension) == 0,
				GPLATES_ASSERTION_SOURCE);
	}
	else // BOTTOM_TO_TOP ...
	{
		// The raster height minus the end y coordinate should be also be an integer multiple.
		// The inverted y is a result of the geo coordinates starting at the top-left but
		// the raster data starting at the bottom-left.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				((d_raster_height - y_geo_end) % tile_geo_dimension) == 0,
				GPLATES_ASSERTION_SOURCE);
	}

	// The width and height of this node.
	const unsigned int node_geo_width = x_geo_end - x_geo_start;
	const unsigned int node_geo_height = y_geo_end - y_geo_start;

	// Determine if this node should be a leaf node (referencing a tile).
	if (node_geo_width <= tile_geo_dimension &&
		node_geo_height <= tile_geo_dimension)
	{
		// Return the node index so the parent node can reference this node.
		return create_obb_tree_leaf_node(level_of_detail,
				x_geo_start, x_geo_end, y_geo_start, y_geo_end);
	}

	//
	// When we reach the leaf nodes of the tree we can calculate the tile OBBs and then
	// as we traverse back up the tree (returning from recursion) we can generate the OBBs
	// of the internal nodes from the OBBs of child nodes (or optionally for a possibly
	// tighter fit, from the OBBs of all the tiles bounded by each internal node).
	//

	// Indices of child OBB tree nodes.
	std::size_t child_node_indices[2];

	// Divide this node into two child nodes along the raster x or y direction
	// depending on whether this node is longer (in units of pixels) along
	// the x or y direction.
	if (node_geo_width > node_geo_height)
	{
		// Divide along the x direction.
		//
		// Determine how many tiles (at the current level-of-detail) to
		// give to each child node. Round up the number of tiles (possibly non-integer)
		// covered by this node and then divide by two and round down (truncate) -
		// this gives the most even balance across the two child nodes.
		const unsigned int num_tiles_in_left_child =
				(node_geo_width + tile_geo_dimension - 1) / tile_geo_dimension / 2;

		// Left child node.
		child_node_indices[0] = create_obb_tree(
				level_of_detail,
				x_geo_start,
				x_geo_start + num_tiles_in_left_child * tile_geo_dimension,
				y_geo_start,
				y_geo_end);

		// 'node_geo_width' is greater than 'tile_geo_dimension' so we should
		// have texels remaining for the right child node.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				x_geo_end > x_geo_start + num_tiles_in_left_child * tile_geo_dimension,
				GPLATES_ASSERTION_SOURCE);
		// Right child node.
		child_node_indices[1] = create_obb_tree(
				level_of_detail,
				x_geo_start + num_tiles_in_left_child * tile_geo_dimension,
				x_geo_end,
				y_geo_start,
				y_geo_end);
	}
	else
	{
		// Divide along the y direction.
		//
		// Determine how many tiles (at the current level-of-detail) to
		// give to each child node. Round up the number of tiles (possibly non-integer)
		// covered by this node and then divide by two and round down (truncate) -
		// this gives the most even balance across the two child nodes.

		if (d_raster_scanline_order == TOP_TO_BOTTOM)
		{
			const unsigned int num_tiles_in_top_child =
					(node_geo_height + tile_geo_dimension - 1) / tile_geo_dimension / 2;

			// Top child node.
			child_node_indices[0] = create_obb_tree(
					level_of_detail,
					x_geo_start,
					x_geo_end,
					y_geo_start,
					y_geo_start + num_tiles_in_top_child * tile_geo_dimension);

			// 'node_geo_height' is greater than 'tile_geo_dimension' so we should
			// have texels remaining for the bottom child node.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					y_geo_end > y_geo_start + num_tiles_in_top_child * tile_geo_dimension,
					GPLATES_ASSERTION_SOURCE);
			// Bottom child node.
			child_node_indices[1] = create_obb_tree(
					level_of_detail,
					x_geo_start,
					x_geo_end,
					y_geo_start + num_tiles_in_top_child * tile_geo_dimension,
					y_geo_end);
		}
		else // BOTTOM_TO_TOP ...
		{
			const unsigned int num_tiles_in_bottom_child =
					(node_geo_height + tile_geo_dimension - 1) / tile_geo_dimension / 2;

			// 'node_geo_height' is greater than 'tile_geo_dimension' so we should
			// have texels remaining for the top child node.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					y_geo_start < y_geo_end - num_tiles_in_bottom_child * tile_geo_dimension,
					GPLATES_ASSERTION_SOURCE);
			// Top child node.
			child_node_indices[0] = create_obb_tree(
					level_of_detail,
					x_geo_start,
					x_geo_end,
					y_geo_start,
					y_geo_end - num_tiles_in_bottom_child * tile_geo_dimension);

			// Bottom child node.
			child_node_indices[1] = create_obb_tree(
					level_of_detail,
					x_geo_start,
					x_geo_end,
					y_geo_end - num_tiles_in_bottom_child * tile_geo_dimension,
					y_geo_end);
		}
	}

	// Each OBB in the tree has one axis oriented radially outward from the globe at the
	// centre point of its bounding domain as this should generate the tightest bounding box.
	const double x_geo_centre = 0.5 * (x_geo_start + x_geo_end);
	const double y_geo_centre = 0.5 * (y_geo_start + y_geo_end);

	GLIntersect::OrientedBoundingBoxBuilder obb_builder =
			create_oriented_bounding_box_builder(x_geo_centre, y_geo_centre);

	// Expand the oriented bounding box to include the child node bounding boxes.
	obb_builder.add(
			level_of_detail.get_obb_tree_node(child_node_indices[0]).bounding_box);
	obb_builder.add(
			level_of_detail.get_obb_tree_node(child_node_indices[1]).bounding_box);

	// Create an OBB tree node.
	LevelOfDetail::OBBTreeNode obb_node(
			obb_builder.get_oriented_bounding_box(),
			false/*is_leaf_node*/);

	// Set the child node indices.
	obb_node.child_node_indices[0] = child_node_indices[0];
	obb_node.child_node_indices[1] = child_node_indices[1];

	// Add the node to the level-of-detail and get it's array index.
	const std::size_t obb_node_index = level_of_detail.obb_tree_nodes.size();
	level_of_detail.obb_tree_nodes.push_back(obb_node);

	// Return the node index so the parent node can reference this node.
	return obb_node_index;
}


std::size_t
GPlatesOpenGL::GLMultiResolutionRaster::create_obb_tree_leaf_node(
		LevelOfDetail &level_of_detail,
		const unsigned int x_geo_start,
		const unsigned int x_geo_end,
		const unsigned int y_geo_start,
		const unsigned int y_geo_end)
{
	// Create the level-of-detail tile that this OBB tree leaf node will reference.
	const tile_handle_type lod_tile_handle = create_level_of_detail_tile(
			level_of_detail, x_geo_start, x_geo_end, y_geo_start, y_geo_end);

	// Get the level-of-detail tile structure just created.
	const LevelOfDetailTile &lod_tile = *d_tiles[lod_tile_handle];

	// Create an oriented bounding box around the vertices of the level-of-detail tile.
	const GLIntersect::OrientedBoundingBox obb = bound_level_of_detail_tile(lod_tile);

	// Get the maximum size of any texel in the level-of-detail tile.
	// We only really need to do this for the highest resolution level because
	// the maximum texel size of the lower resolution levels will be very close to a
	// power-of-two factor of the highest resolution level (not exactly a power-of-two
	// because warping due to the map projection but it'll be close enough for our
	// purpose of level-of-detail selection).
	if (level_of_detail.lod_level == 0)
	{
		const float max_texel_size_on_unit_sphere =
				calc_max_texel_size_on_unit_sphere(level_of_detail.lod_level, lod_tile);

		// The maximum texel size for the entire original raster is the maximum texel of all
		// its highest resolution tiles.
		if (max_texel_size_on_unit_sphere > d_max_highest_resolution_texel_size_on_unit_sphere)
		{
			d_max_highest_resolution_texel_size_on_unit_sphere = max_texel_size_on_unit_sphere;
		}
	}

	// Create an OBB tree node.
	LevelOfDetail::OBBTreeNode obb_node(obb, true/*is_leaf_node*/);

	// Set the level-of-detail tile for this OBB node.
	obb_node.tile = lod_tile_handle;

	// Add the node to the level-of-detail and get it's array index.
	const std::size_t obb_node_index = level_of_detail.obb_tree_nodes.size();
	level_of_detail.obb_tree_nodes.push_back(obb_node);

	// Return the node index so the parent node can reference this node.
	return obb_node_index;
}


GPlatesOpenGL::GLMultiResolutionRaster::tile_handle_type
GPlatesOpenGL::GLMultiResolutionRaster::create_level_of_detail_tile(
		LevelOfDetail &level_of_detail,
		const unsigned int x_geo_start,
		const unsigned int x_geo_end,
		const unsigned int y_geo_start,
		const unsigned int y_geo_end)
{
	// Level-of-detail.
	const unsigned int lod_level = level_of_detail.lod_level;

	// Texels in this level-of-detail have dimensions 'lod_factor' times larger than the
	// original raster pixels when projected on the globe (they cover a larger area on the globe).
	const unsigned int lod_factor = 1 << lod_level;

	// The size of a tile (at the current level-of-detail) in units of pixels of
	// the original raster. Pixels and geo coordinates are the same.
	const unsigned int tile_geo_dimension = d_tile_texel_dimension * lod_factor;

	//
	// In each tile store enough information to be able to generate the
	// vertex and texture data as needed when rendering the raster.
	//

	// Make sure neighbouring tiles, of the same resolution level, have exactly
	// matching boundaries to avoid cracks appearing between adjacent tiles.
	// We do this by making the corner geo (pixel) coordinates of the tile match those
	// of adjacent tiles - this is no problem since we're using integer geo coordinates.
	// They will get converted to floating-point when georeferenced but as long as they go
	// through the same code path for all tiles then the final positions of the unit sphere
	// should match up identically (ie, bitwise equality of the floating-point xyz coordinates).

	// The start of the tile should be inside the raster.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			x_geo_start < d_raster_width && y_geo_start < d_raster_height,
			GPLATES_ASSERTION_SOURCE);

	// The start x coordinate should be an integer multiple of the tile dimension.
	// The raster height minus the end y coordinate should be also be an integer multiple.
	// The inverted y is a result of the geo coordinates starting at the top-left but
	// the raster data starting at the bottom-left.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			(x_geo_start % tile_geo_dimension) == 0,
			GPLATES_ASSERTION_SOURCE);
	if (d_raster_scanline_order == TOP_TO_BOTTOM)
	{
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				(y_geo_start % tile_geo_dimension) == 0,
				GPLATES_ASSERTION_SOURCE);
	}
	else // BOTTOM_TO_TOP ...
	{
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				((d_raster_height - y_geo_end) % tile_geo_dimension) == 0,
				GPLATES_ASSERTION_SOURCE);
	}

	// The number of texels needed to cover the tile.
	// Round this up so that the level-of-details texels cover the range of geo coordinates.
	unsigned int num_u_texels = (x_geo_end - x_geo_start + lod_factor - 1) / lod_factor;
	unsigned int num_v_texels = (y_geo_end - y_geo_start + lod_factor - 1) / lod_factor;

	// The texel offsets into the raster data.
	// Note we need to invert in the 'v' or 'y' direction because
	// our georeferencing starts at the top-left of the image but our
	// raster data starts at the bottom-left.
	// Both of the divisions here are exactly divisible.
	const unsigned int u_lod_texel_offset = x_geo_start / lod_factor;
	const unsigned int v_lod_texel_offset = (d_raster_scanline_order == TOP_TO_BOTTOM)
			? y_geo_start / lod_factor
			: (d_raster_height - y_geo_end) / lod_factor;

	const float u_start = 0; // x_geo_start begins exactly on a texel boundary
	const float u_end = static_cast<float>(x_geo_end - x_geo_start) / tile_geo_dimension;

	float v_start, v_end;
	if (d_raster_scanline_order == TOP_TO_BOTTOM)
	{
		v_start = 0; // y_geo_start begins exactly on a texel boundary
		v_end = static_cast<float>(y_geo_end - y_geo_start) / tile_geo_dimension;
	}
	else // BOTTOM_TO_TOP ...
	{
		v_start = static_cast<float>(y_geo_end - y_geo_start) / tile_geo_dimension;
		v_end = 0; // y_geo_end begins exactly on a texel boundary
	}

	// Determine the number of vertices required for the tile.
	const std::pair<unsigned int, unsigned int> num_vertices_along_tile_edges =
			calculate_num_vertices_along_tile_edges(
					x_geo_start, x_geo_end,
					y_geo_start, y_geo_end,
					num_u_texels, num_v_texels);
	const unsigned int x_num_vertices = num_vertices_along_tile_edges.first;
	const unsigned int y_num_vertices = num_vertices_along_tile_edges.second;

	// Get the vertex indices for this tile.
	// Since most tiles can share these indices we store them in a map keyed on
	// the number of vertices in each dimension.
	GLVertexElementArray::non_null_ptr_to_const_type vertex_element_array =
			get_vertex_element_array(x_num_vertices, y_num_vertices);

	// Create the level-of-detail tile now that we have all the information we need.
	LevelOfDetailTile::non_null_ptr_type lod_tile = LevelOfDetailTile::create(
			lod_level,
			x_geo_start, x_geo_end,
			y_geo_start, y_geo_end,
			x_num_vertices, y_num_vertices,
			u_start, u_end,
			v_start, v_end,
			u_lod_texel_offset, v_lod_texel_offset,
			num_u_texels, num_v_texels,
			vertex_element_array);

	// Add the tile to the sequence of all tiles.
	const tile_handle_type tile_handle = d_tiles.size();
	d_tiles.push_back(lod_tile);

	// Return the tile handle.
	return tile_handle;
}


GPlatesOpenGL::GLIntersect::OrientedBoundingBox
GPlatesOpenGL::GLMultiResolutionRaster::bound_level_of_detail_tile(
		const LevelOfDetailTile &lod_tile) const
{
	// Generate the oriented axes for an OBB for this raster tile.
	//
	// Each OBB in the tree has one axis oriented radially outward from the globe at the
	// centre point of its bounding domain as this should generate the tightest bounding box.
	const double tile_centre_x_geo_coord = 0.5 * (lod_tile.x_geo_start + lod_tile.x_geo_end);
	const double tile_centre_y_geo_coord = 0.5 * (lod_tile.y_geo_start + lod_tile.y_geo_end);

	GLIntersect::OrientedBoundingBoxBuilder obb_builder = create_oriented_bounding_box_builder(
			tile_centre_x_geo_coord, tile_centre_y_geo_coord);

	// Expand the oriented bounding box to include all vertices of the current tile.
	// We only need the boundary vertices to accomplish this because the interior
	// vertices will then be bounded along the OBB's x and y axes (due to the boundary vertices)
	// and the z-axis will be bounded along its negative direction (due to the boundary vertices)
	// and the z-axis will be bounded along its positive direction due to the extremal point
	// already added in 'create_oriented_bounding_box_builder()'.

	// Set up some variables before calculating the boundary vertices.
	const double x_pixels_per_quad =
			static_cast<double>(lod_tile.x_geo_end - lod_tile.x_geo_start) /
					(lod_tile.x_num_vertices - 1);
	const double y_pixels_per_quad =
			static_cast<double>(lod_tile.y_geo_end - lod_tile.y_geo_start) /
					(lod_tile.y_num_vertices - 1);

	// Bound the tile's top and bottom edge vertices.
	for (unsigned int i = 0; i < lod_tile.x_num_vertices; ++i)
	{
		obb_builder.add(convert_pixel_coord_to_geographic_coord(
				lod_tile.x_geo_start + i * x_pixels_per_quad,
				lod_tile.y_geo_start));
		obb_builder.add(convert_pixel_coord_to_geographic_coord(
				lod_tile.x_geo_start + i * x_pixels_per_quad,
				lod_tile.y_geo_end));
	}
	// Bound the tile's left and right edge vertices (excluding corner points already bounded).
	for (unsigned int j = 1; j < lod_tile.y_num_vertices - 1; ++j)
	{
		obb_builder.add(convert_pixel_coord_to_geographic_coord(
				lod_tile.x_geo_start,
				lod_tile.y_geo_start + j * y_pixels_per_quad));
		obb_builder.add(convert_pixel_coord_to_geographic_coord(
				lod_tile.x_geo_end,
				lod_tile.y_geo_start + j * y_pixels_per_quad));
	}

	return obb_builder.get_oriented_bounding_box();
}


const std::pair<unsigned int, unsigned int>
GPlatesOpenGL::GLMultiResolutionRaster::calculate_num_vertices_along_tile_edges(
		const unsigned int x_geo_start,
		const unsigned int x_geo_end,
		const unsigned int y_geo_start,
		const unsigned int y_geo_end,
		const unsigned int num_u_texels,
		const unsigned int num_v_texels)
{
	// Centre point of the tile.
	const double x_geo_centre = 0.5 * (x_geo_start + x_geo_end);
	const double y_geo_centre = 0.5 * (y_geo_start + y_geo_end);

	// The nine boundary points including corners and midpoints and one centre point.
	const GPlatesMaths::PointOnSphere tile_points[3][3] =
	{
		{
			convert_pixel_coord_to_geographic_coord(x_geo_start, y_geo_start),
			convert_pixel_coord_to_geographic_coord(x_geo_centre, y_geo_start),
			convert_pixel_coord_to_geographic_coord(x_geo_end, y_geo_start)
		},
		{
			convert_pixel_coord_to_geographic_coord(x_geo_start, y_geo_centre),
			convert_pixel_coord_to_geographic_coord(x_geo_centre, y_geo_centre),
			convert_pixel_coord_to_geographic_coord(x_geo_end, y_geo_centre)
		},
		{
			convert_pixel_coord_to_geographic_coord(x_geo_start, y_geo_end),
			convert_pixel_coord_to_geographic_coord(x_geo_centre, y_geo_end),
			convert_pixel_coord_to_geographic_coord(x_geo_end, y_geo_end)
		}
	};

	// Calculate the maximum angle spanned by the current tile in the x direction.
	GPlatesMaths::real_t x_tile_min_half_span = 1;

	// Iterate over the half segments and calculate dot products in the x direction.
	for (unsigned int i = 0; i < 3; ++i)
	{
		for (unsigned int j = 0; j < 2; ++j)
		{
			GPlatesMaths::real_t x_tile_half_span = dot(
					tile_points[i][j].position_vector(), tile_points[i][j+1].position_vector());
			if (x_tile_half_span < x_tile_min_half_span)
			{
				x_tile_min_half_span = x_tile_half_span;
			}
		}
	}

	// Calculate the maximum angle spanned by the current tile in the y direction.
	GPlatesMaths::real_t y_tile_min_half_span = 1;

	// Iterate over the half segments and calculate dot products in the y direction.
	for (unsigned int j = 0; j < 3; ++j)
	{
		for (unsigned int i = 0; i < 2; ++i)
		{
			GPlatesMaths::real_t y_tile_half_span = dot(
					tile_points[i][j].position_vector(), tile_points[i+1][j].position_vector());
			if (y_tile_half_span < y_tile_min_half_span)
			{
				y_tile_min_half_span = y_tile_half_span;
			}
		}
	}

	// Convert from dot product to angle.
	const GPlatesMaths::real_t x_tile_max_spanned_angle_in_radians = 2 * acos(x_tile_min_half_span);
	const GPlatesMaths::real_t y_tile_max_spanned_angle_in_radians = 2 * acos(y_tile_min_half_span);

	// Determine number of quads (segments) along each edge.
	const GPlatesMaths::real_t x_num_quads_based_on_distance_real =
			GPlatesMaths::convert_rad_to_deg(x_tile_max_spanned_angle_in_radians) /
					MAX_ANGLE_IN_DEGREES_BETWEEN_VERTICES;
	const GPlatesMaths::real_t y_num_quads_based_on_distance_real =
			GPlatesMaths::convert_rad_to_deg(y_tile_max_spanned_angle_in_radians) /
					MAX_ANGLE_IN_DEGREES_BETWEEN_VERTICES;

	// Determine the number of quads along each tile edge based on the texel resolution.
	const unsigned int x_num_quads_based_on_texels = num_u_texels / NUM_TEXELS_PER_VERTEX;
	const unsigned int y_num_quads_based_on_texels = num_v_texels / NUM_TEXELS_PER_VERTEX;

#if 1
	unsigned int x_num_quads = x_num_quads_based_on_texels;
	unsigned int y_num_quads = y_num_quads_based_on_texels;
#else
	// Rounded up to integer.
	const unsigned int x_num_quads_based_on_distance =
			static_cast<unsigned int>(x_num_quads_based_on_distance_real.dval() + 0.99);
	const unsigned int y_num_quads_based_on_distance =
			static_cast<unsigned int>(y_num_quads_based_on_distance_real.dval() + 0.99);

	unsigned int x_num_quads = (std::max)(x_num_quads_based_on_distance, x_num_quads_based_on_texels);
	unsigned int y_num_quads = (std::max)(y_num_quads_based_on_distance, y_num_quads_based_on_texels);
#endif

	// Make sure non-zero.
	if (x_num_quads == 0)
	{
		x_num_quads = 1;
	}
	if (y_num_quads == 0)
	{
		y_num_quads = 1;
	}

	// The number of vertices each edge is the number of quads along each edge "+1".
	//
	// -------
	// | | | |
	// -------
	// | | | |
	// -------
	// | | | |
	// -------
	//
	// ...the above shows 3x3=9 quads but there's 4x4=16 vertices.
	//
	const unsigned int x_num_vertices = x_num_quads + 1;
	const unsigned int y_num_vertices = y_num_quads + 1;

	//std::cout << "Num vertices: " << x_num_vertices << ", " << y_num_vertices << std::endl;

	return std::make_pair(x_num_vertices, y_num_vertices);
}


float
GPlatesOpenGL::GLMultiResolutionRaster::calc_max_texel_size_on_unit_sphere(
		const unsigned int lod_level,
		const LevelOfDetailTile &lod_tile) const
{
	//
	// Sample roughly 8x8 points of a 256x256 texel tile and at each point calculate the
	// size of a texel (at the level-of-detail of the tile).
	//
	// We could sample more densely to get a more accurate result but it could be
	// expensive. For example a 10,000 x 5,000 raster image will have ~800 tiles
	// at the highest resolution level-of-detail (tile size 256x256 texels).
	// If we sampled each tile at 32 x 32 (ie roughly the sampling of mesh vertices)
	// and it took 4000 CPU clock cycles per sample that would take:
	//   (800 x 32 x 32 x 4000 / 3e9) seconds
	// ...on a 3Ghz machine, which is 1.0 seconds (doesn't include other levels of detail).
	// This is only done once when the raster is first setup but still that's a noticeable delay.
	//
	// Set to 8x8 samples on a 256x256 texel tile (256 / 8 = 32).
	static const unsigned int NUM_TEXELS_PER_SAMPLE_POINT = 32;

	// Determine the number of sample points along each tile edge.
	unsigned int num_sample_segments_along_tile_x_edge =
			lod_tile.num_u_lod_texels / NUM_TEXELS_PER_SAMPLE_POINT;
	if (num_sample_segments_along_tile_x_edge == 0)
	{
		num_sample_segments_along_tile_x_edge = 1;
	}
	unsigned int num_sample_segments_along_tile_y_edge =
			lod_tile.num_v_lod_texels / NUM_TEXELS_PER_SAMPLE_POINT;
	if (num_sample_segments_along_tile_y_edge == 0)
	{
		num_sample_segments_along_tile_y_edge = 1;
	}

	// Set up some variables before calculating sample positions on the globe.
	const double x_pixels_per_quad =
			static_cast<double>(lod_tile.x_geo_end - lod_tile.x_geo_start) /
					num_sample_segments_along_tile_x_edge;
	const double y_pixels_per_quad =
			static_cast<double>(lod_tile.y_geo_end - lod_tile.y_geo_start) /
					num_sample_segments_along_tile_y_edge;

	// Number of samples along each tile edge.
	const unsigned int num_samples_along_tile_x_edge = num_sample_segments_along_tile_x_edge + 1;
	const unsigned int num_samples_along_tile_y_edge = num_sample_segments_along_tile_y_edge + 1;

	GPlatesMaths::real_t min_dot_product_texel_size(1);
	const double texel_size_in_pixels = (1 << lod_level);

	// Iterate over the sample points.
	for (unsigned int j = 0; j < num_samples_along_tile_y_edge; ++j)
	{
		const double y = lod_tile.y_geo_start + j * y_pixels_per_quad;

		// We try to avoid sampling outside the raster pixel range because we don't
		// know if that will cause problems with the inverse map projection (if any
		// was specified for the raster).
		const double y_plus_one_texel = y +
				((j == 0) ? texel_size_in_pixels : -texel_size_in_pixels);

		for (unsigned int i = 0; i < num_samples_along_tile_x_edge; ++i)
		{
			const double x = lod_tile.x_geo_start + i * x_pixels_per_quad;

			// The main sample point.
			const GPlatesMaths::PointOnSphere sample_point =
					convert_pixel_coord_to_geographic_coord(x, y);

			// We try to avoid sampling outside the raster pixel range because we don't
			// know if that will cause problems with the inverse map projection (if any
			// was specified for the raster).
			const double x_plus_one_texel = x +
					((i == 0) ? texel_size_in_pixels : -texel_size_in_pixels);

			// Sample point one texel in x direction.
			const GPlatesMaths::PointOnSphere sample_point_plus_one_texel_x =
					convert_pixel_coord_to_geographic_coord(x_plus_one_texel, y);

			// The dot product can be converted to arc distance on unit sphere but we can
			// delay that expensive operation until we've compared all samples.
			const GPlatesMaths::real_t dot_product_texel_size_x = dot(
						sample_point.position_vector(),
						sample_point_plus_one_texel_x.position_vector());
			// We want the maximum projected texel size which means minimum dot product.
			if (dot_product_texel_size_x < min_dot_product_texel_size)
			{
				min_dot_product_texel_size = dot_product_texel_size_x;
			}

			// Sample point one texel in y direction.
			const GPlatesMaths::PointOnSphere sample_point_plus_one_texel_y =
					convert_pixel_coord_to_geographic_coord(x, y_plus_one_texel);

			// The dot product can be converted to arc distance on unit sphere but we can
			// delay that expensive operation until we've compared all samples.
			const GPlatesMaths::real_t dot_product_texel_size_y = dot(
						sample_point.position_vector(),
						sample_point_plus_one_texel_y.position_vector());
			// We want the maximum projected texel size which means minimum dot product.
			if (dot_product_texel_size_y < min_dot_product_texel_size)
			{
				min_dot_product_texel_size = dot_product_texel_size_y;
			}
		}
	}

	// Convert from dot product to arc distance on the unit sphere.
	return acos(min_dot_product_texel_size).dval();
}


GPlatesOpenGL::GLIntersect::OrientedBoundingBoxBuilder
GPlatesOpenGL::GLMultiResolutionRaster::create_oriented_bounding_box_builder(
		const double &x_geo_coord,
		const double &y_geo_coord) const
{
	// Convert the pixel coordinates to a point on the sphere.
	const GPlatesMaths::PointOnSphere point_on_sphere =
			convert_pixel_coord_to_geographic_coord(x_geo_coord, y_geo_coord);

	// The OBB z-axis is the vector from globe origin to point on sphere.
	const GPlatesMaths::UnitVector3D &obb_z_axis = point_on_sphere.position_vector();

	// Calculate the OBB x axis by taking the centre pixel coordinate and doing a finite
	// difference in the x direction.
	// The delta value just needs to be small enough to get a nearly tangential vector
	// to the raster at the specified pixel coordinate.
	//
	// The reason for getting the OBB x-axis tangential to the raster is so the OBB
	// will align with, and hence bound, the raster tile(s) tightly.
	const double delta = 1; // Make it +/- one pixel of delta.
	const GPlatesMaths::PointOnSphere centre_point_minus_x_delta =
			convert_pixel_coord_to_geographic_coord(x_geo_coord - delta, y_geo_coord);
	const GPlatesMaths::PointOnSphere centre_point_plus_x_delta =
			convert_pixel_coord_to_geographic_coord(x_geo_coord + delta, y_geo_coord);

	// The vector difference between these two points is the x-axis.
	GPlatesMaths::Vector3D obb_x_axis_unnormalised =
			GPlatesMaths::Vector3D(centre_point_minus_x_delta.position_vector()) -
					GPlatesMaths::Vector3D(centre_point_plus_x_delta.position_vector());

	// Return a builder using the orthonormal axes.
	// We're using our x-axis as a y-axis in this function call but it doesn't matter -
	// just an axis label really.
	GLIntersect::OrientedBoundingBoxBuilder obb_builder =
			GLIntersect::create_oriented_bounding_box_builder(obb_x_axis_unnormalised, obb_z_axis);

	// The point of sphere of the pixel coordinates is the most extremal point along
	// the OBB's z-axis so add it to the OBB to set the maximum extent of the OBB
	// along its z-axis.
	obb_builder.add(point_on_sphere);

	// We'll still need to add more points before we get a usable OBB though.
	// It's up to the caller to do this.
	return obb_builder;
}


GPlatesOpenGL::GLVertexElementArray::non_null_ptr_to_const_type
GPlatesOpenGL::GLMultiResolutionRaster::get_vertex_element_array(
		const unsigned int num_vertices_along_tile_x_edge,
		const unsigned int num_vertices_along_tile_y_edge)
{
	// Should have at least two vertices along each edge of the tile.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			num_vertices_along_tile_x_edge > 1 && num_vertices_along_tile_y_edge > 1,
			GPLATES_ASSERTION_SOURCE);

	// Lookup our map of vertex element arrays to see if we've already created one
	// with the specified vertex dimensions.
	vertex_element_array_map_type::const_iterator vertex_element_array_iter =
			d_vertex_element_arrays.find(
					vertex_element_array_map_type::key_type(
							num_vertices_along_tile_x_edge,
							num_vertices_along_tile_y_edge));
	if (vertex_element_array_iter != d_vertex_element_arrays.end())
	{
		return vertex_element_array_iter->second;
	}

	//
	// We haven't already created a vertex element array with the specified vertex dimensions
	// so create a new one.
	//

	// Number of quads along each tile edge.
	const unsigned int num_quads_along_tile_x_edge = num_vertices_along_tile_x_edge - 1;
	const unsigned int num_quads_along_tile_y_edge = num_vertices_along_tile_y_edge - 1;

	// Total number of quads in the tile.
	const unsigned int num_quads_per_tile =
			num_quads_along_tile_x_edge * num_quads_along_tile_y_edge;

	// Total number of vertices in the tile.
	const unsigned int num_vertices_per_tile =
			num_vertices_along_tile_x_edge * num_vertices_along_tile_y_edge;

	// Total number of triangles and vertex indices in the tile.
	const unsigned int num_triangles_per_tile = 2 * num_quads_per_tile;
	const unsigned int num_indices_per_tile = 3 * num_triangles_per_tile;

	// Since we're using GLushort to store our vertex indices, we can't have
	// more than 65535 vertices per tile - we're probably using about a thousand
	// per tile so should be no problem.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			num_vertices_per_tile < (1 << (8 * sizeof(GLushort))),
			GPLATES_ASSERTION_SOURCE);

	// The array to store the vertex indices.
	boost::shared_array<GLushort> vertex_element_array_data(new GLushort[num_indices_per_tile]);

	// Initialise the vertex indices.
	GLushort *indices = vertex_element_array_data.get();
	for (unsigned int y = 0; y < num_quads_along_tile_y_edge; ++y)
	{
		for (unsigned int x = 0; x < num_quads_along_tile_x_edge; ++x)
		{
			//
			// These are the two triangles per quad:
			//
			// ----
			// |\ |
			// | \|
			// ----
			//

			// The lower triangle in the above diagram.
			// Front facing triangles are counter-clockwise.
			indices[0] = y * num_vertices_along_tile_x_edge + x;
			indices[1] = (y + 1) * num_vertices_along_tile_x_edge + x;
			indices[2] = (y + 1) * num_vertices_along_tile_x_edge + x + 1;
			indices += 3;

			// The upper triangle in the above diagram.
			// Front facing triangles are counter-clockwise.
			indices[0] = (y + 1) * num_vertices_along_tile_x_edge + x + 1;
			indices[1] = y * num_vertices_along_tile_x_edge + x + 1;
			indices[2] = y * num_vertices_along_tile_x_edge + x;
			indices += 3;
		}
	}

	// Create a vertex element array and set the index data array.
	GLVertexElementArray::non_null_ptr_type vertex_element_array =
			GLVertexElementArray::create(vertex_element_array_data);

	// Tell it what to draw when the time comes to draw.
	vertex_element_array->gl_draw_range_elements_EXT(
			GL_TRIANGLES,
			0/*start*/,
			num_vertices_per_tile - 1/*end*/,
			num_indices_per_tile/*count*/,
			GL_UNSIGNED_SHORT/*type*/,
			0 /*indices_offset*/);

	// Add to our map of vertex element arrays.
	const vertex_element_array_map_type::key_type vertex_dimensions(
			num_vertices_along_tile_x_edge, num_vertices_along_tile_y_edge);
	d_vertex_element_arrays.insert(vertex_element_array_map_type::value_type(
			vertex_dimensions, vertex_element_array));

	return vertex_element_array;
}


GPlatesMaths::PointOnSphere
GPlatesOpenGL::GLMultiResolutionRaster::convert_pixel_coord_to_geographic_coord(
		const double &x_pixel_coord,
		const double &y_pixel_coord) const
{
	// Get the georeferencing parameters.
	const GPlatesPropertyValues::Georeferencing::parameters_type &georef =
			d_georeferencing->parameters();

	// Use the georeferencing information to convert
	// from pixel coordinates to geographic coordinates.
	double x_geo =
			x_pixel_coord * georef.x_component_of_pixel_width +
			y_pixel_coord * georef.x_component_of_pixel_height +
			georef.top_left_x_coordinate;
	double y_geo =
			x_pixel_coord * georef.y_component_of_pixel_width +
			y_pixel_coord * georef.y_component_of_pixel_height +
			georef.top_left_y_coordinate;

	// TODO: This is where the inverse map projection will go when we add
	// the map projection to the georeferencing information.
	// It will transform from map coordinates (x_geo, y_geo) to (longitude, latitude).
	// Right now we assume no map projection in which case (x_geo, y_geo)
	// are already in (longitude, latitude).

	// Sometimes due to numerical precision the latitude is slightly less then -90 degrees
	// or slightly greater than 90 degrees.
	// If it's only slightly outside the valid range then we'll be lenient and correct it.
	// Otherwise we'll do nothing and let GPlatesMaths::LatLonPoint throw an exception.
	// UPDATE: Actually we'll hard clamp it - there's no guarantee that the georeferencing
	// is correct in which case the raster will just be displayed incorrectly.
	if (y_geo < -90)
	{
		y_geo = -90;
	}
	else if (y_geo > 90)
	{
		y_geo = 90;
	}
	if (x_geo < -360)
	{
		x_geo = -360;
	}
	else if (x_geo > 360)
	{
		x_geo = 360;
	}

	// Finally convert from (longitude, latitude) to cartesian (x,y,z).
	const GPlatesMaths::LatLonPoint x_lat_lon(y_geo, x_geo);

	return make_point_on_sphere(x_lat_lon);
}


GPlatesOpenGL::GLMultiResolutionRaster::Tile::Tile(
		const GLVertexArray::shared_ptr_to_const_type &vertex_array,
		const GLVertexElementArray::non_null_ptr_to_const_type &vertex_element_array,
		const GLTexture::shared_ptr_to_const_type &texture) :
	d_drawable(GLVertexArrayDrawable::create(vertex_array, vertex_element_array)),
	d_texture(texture)
{
}


GPlatesOpenGL::GLMultiResolutionRaster::LevelOfDetailTile::LevelOfDetailTile(
		unsigned int lod_level_,
		unsigned int x_geo_start_,
		unsigned int x_geo_end_,
		unsigned int y_geo_start_,
		unsigned int y_geo_end_,
		unsigned int x_num_vertices_,
		unsigned int y_num_vertices_,
		float u_start_,
		float u_end_,
		float v_start_,
		float v_end_,
		unsigned int u_lod_texel_offset_,
		unsigned int v_lod_texel_offset_,
		unsigned int num_u_lod_texels_,
		unsigned int num_v_lod_texels_,
		const GLVertexElementArray::non_null_ptr_to_const_type &vertex_element_array_) :
	lod_level(lod_level_),
	x_geo_start(x_geo_start_),
	x_geo_end(x_geo_end_),
	y_geo_start(y_geo_start_),
	y_geo_end(y_geo_end_),
	x_num_vertices(x_num_vertices_),
	y_num_vertices(y_num_vertices_),
	u_start(u_start_),
	u_end(u_end_),
	v_start(v_start_),
	v_end(v_end_),
	u_lod_texel_offset(u_lod_texel_offset_),
	v_lod_texel_offset(v_lod_texel_offset_),
	num_u_lod_texels(num_u_lod_texels_),
	num_v_lod_texels(num_v_lod_texels_),
	vertex_element_array(vertex_element_array_)
{
}


GPlatesOpenGL::GLMultiResolutionRaster::LevelOfDetail::LevelOfDetail(
		unsigned int lod_level_) :
	lod_level(lod_level_),
	obb_tree_root_node_index(0)
{
}


const GPlatesOpenGL::GLMultiResolutionRaster::LevelOfDetail::OBBTreeNode &
GPlatesOpenGL::GLMultiResolutionRaster::LevelOfDetail::get_obb_tree_node(
		std::size_t node_index) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			node_index < obb_tree_nodes.size(),
			GPLATES_ASSERTION_SOURCE);

	return obb_tree_nodes[node_index];
}


GPlatesOpenGL::GLMultiResolutionRaster::LevelOfDetail::OBBTreeNode &
GPlatesOpenGL::GLMultiResolutionRaster::LevelOfDetail::get_obb_tree_node(
		std::size_t node_index)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			node_index < obb_tree_nodes.size(),
			GPLATES_ASSERTION_SOURCE);

	return obb_tree_nodes[node_index];
}
