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
#include <new> // For placement new.
#include <boost/bind.hpp>
#include <boost/cast.hpp>
#include <boost/cstdint.hpp>
#include <boost/foreach.hpp>
#include <boost/scoped_array.hpp>

/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>
#include <opengl/OpenGL.h>
#include <QDebug>

#include "GLMultiResolutionRaster.h"

#include "GLContext.h"
#include "GLFrustum.h"
#include "GLIntersect.h"
#include "GLNormalMapSource.h"
#include "GLProjectionUtils.h"
#include "GLRenderer.h"
#include "GLScalarFieldDepthLayersSource.h"
#include "GLShaderProgramUtils.h"
#include "GLUtils.h"
#include "GLVertex.h"

#include "global/AssertionFailureException.h"
#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "utils/Profile.h"


namespace GPlatesOpenGL
{
	namespace
	{
		//! The inverse of log(2.0).
		const float INVERSE_LOG2 = 1.0 / std::log(2.0);

		/**
		 * Fragment shader source code to render a source raster as either a floating-point raster or
		 * a normal-map raster.
		 */
		const QString RENDER_RASTER_FRAGMENT_SHADER_SOURCE_FILE_NAME =
				":/opengl/multi_resolution_raster/render_raster_fragment_shader.glsl";
	}
}


bool
GPlatesOpenGL::GLMultiResolutionRaster::supports_normal_map_source(
		GLRenderer &renderer)
{
	static bool supported = false;

	// Only test for support the first time we're called.
	static bool tested_for_support = false;
	if (!tested_for_support)
	{
		tested_for_support = true;

		// Need support for GLNormalMapSource.
		if (!GLNormalMapSource::is_supported(renderer))
		{
			return false;
		}

		// Need vertex/fragment shader support.
		if (!GLContext::get_parameters().shader.gl_ARB_vertex_shader ||
			!GLContext::get_parameters().shader.gl_ARB_fragment_shader)
		{
			//qDebug() <<
			//		"GLMultiResolutionRaster: Disabling normal map raster lighting in OpenGL - requires vertex/fragment shader programs.";
			return false;
		}

		//
		// Try to compile our surface normals fragment shader program.
		// If that fails then it could be exceeding some resource limit on the runtime system
		// such as number of shader instructions allowed.
		// We do this test because we are promising to support normal maps in a shader
		// program regardless on the complexity of the shader.
		//

		GLShaderProgramUtils::ShaderSource fragment_shader_source;
		fragment_shader_source.add_shader_source("#define SURFACE_NORMALS\n");
		fragment_shader_source.add_shader_source_from_file(RENDER_RASTER_FRAGMENT_SHADER_SOURCE_FILE_NAME);

		// Attempt to create the test shader program.
		if (!GLShaderProgramUtils::compile_and_link_fragment_program(
				renderer, fragment_shader_source))
		{
			//qDebug() <<
			//		"GLMultiResolutionRaster: Disabling normal map raster lighting in OpenGL - unable to compile and link shader program.";
			return false;
		}

		// If we get this far then we have support.
		supported = true;
	}

	return supported;
}


bool
GPlatesOpenGL::GLMultiResolutionRaster::supports_scalar_field_depth_layers_source(
		GLRenderer &renderer)
{
	static bool supported = false;

	// Only test for support the first time we're called.
	static bool tested_for_support = false;
	if (!tested_for_support)
	{
		tested_for_support = true;

		// Need support for GLScalarFieldDepthLayersSource.
		if (!GLScalarFieldDepthLayersSource::is_supported(renderer))
		{
			return false;
		}

		// Need vertex/fragment shader support.
		if (!GLContext::get_parameters().shader.gl_ARB_vertex_shader ||
			!GLContext::get_parameters().shader.gl_ARB_fragment_shader)
		{
			return false;
		}

		//
		// Try to compile our scalar/gradient fragment shader program.
		// If that fails then it could be exceeding some resource limit on the runtime system
		// such as number of shader instructions allowed.
		// We do this test because we are promising support in a shader program regardless of the
		// complexity of the shader.
		//

		GLShaderProgramUtils::ShaderSource fragment_shader_source;
		fragment_shader_source.add_shader_source("#define SCALAR_GRADIENT\n");
		fragment_shader_source.add_shader_source_from_file(RENDER_RASTER_FRAGMENT_SHADER_SOURCE_FILE_NAME);

		// Attempt to create the test shader program.
		if (!GLShaderProgramUtils::compile_and_link_fragment_program(
				renderer, fragment_shader_source))
		{
			return false;
		}

		// If we get this far then we have support.
		supported = true;
	}

	return supported;
}


GPlatesOpenGL::GLMultiResolutionRaster::non_null_ptr_type
GPlatesOpenGL::GLMultiResolutionRaster::create(
		GLRenderer &renderer,
		const GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type &georeferencing,
		const GLMultiResolutionRasterSource::non_null_ptr_type &raster_source,
		FixedPointTextureFilterType fixed_point_texture_filter,
		CacheTileTexturesType cache_tile_textures,
		RasterScanlineOrderType raster_scanline_order)
{
	return non_null_ptr_type(
			new GLMultiResolutionRaster(
					renderer,
					georeferencing,
					raster_source,
					fixed_point_texture_filter,
					cache_tile_textures,
					raster_scanline_order));
}


GPlatesOpenGL::GLMultiResolutionRaster::GLMultiResolutionRaster(
		GLRenderer &renderer,
		const GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type &georeferencing,
		const GLMultiResolutionRasterSource::non_null_ptr_type &raster_source,
		FixedPointTextureFilterType fixed_point_texture_filter,
		CacheTileTexturesType cache_tile_textures,
		RasterScanlineOrderType raster_scanline_order) :
	d_georeferencing(georeferencing),
	d_raster_source(raster_source),
	// The raster dimensions (the highest resolution level-of-detail).
	d_raster_width(raster_source->get_raster_width()),
	d_raster_height(raster_source->get_raster_height()),
	d_raster_scanline_order(raster_scanline_order),
	d_fixed_point_texture_filter(fixed_point_texture_filter),
	d_tile_texel_dimension(raster_source->get_tile_texel_dimension()),
	d_num_texels_per_vertex(/*default*/MAX_NUM_TEXELS_PER_VERTEX << 16), // ...a 16:16 fixed-point type.
	// The parentheses around min are to prevent the windows min macros
	// from stuffing numeric_limits' min.
	d_max_highest_resolution_texel_size_on_unit_sphere((std::numeric_limits<float>::min)()),
	// Start with small size cache and just let the cache grow in size as needed if caching enabled...
	d_tile_texture_cache(tile_texture_cache_type::create(2/* GPU pipeline breathing room in case caching disabled*/)),
	d_cache_tile_textures(cache_tile_textures),
	// Start with smallest size cache and just let the cache grow in size as needed...
	d_tile_vertices_cache(tile_vertices_cache_type::create())
{
	// Determine number of texels between two adjacent vertices along a horizontal/vertical tile edge.
	// For most rasters this is the maximum texel density.
	// For very low resolution rasters a smaller texel density is needed to keep the mesh surface
	// looking smooth and curved instead of coarsely tessellated on the globe.
	d_num_texels_per_vertex = calculate_num_texels_per_vertex();

	// Create the levels of details and within each one create an oriented bounding box
	// tree (used to quickly find visible tiles) where the drawable tiles are in the
	// leaf nodes of the OBB tree.
	initialise_level_of_detail_pyramid();

	// If the source raster is a normal map then adjust its height field scale depending its resolution.
	if (dynamic_cast<GLNormalMapSource *>(raster_source.get()))
	{
		GPlatesUtils::dynamic_pointer_cast<GLNormalMapSource>(raster_source)
				->set_max_highest_resolution_texel_size_on_unit_sphere(
						d_max_highest_resolution_texel_size_on_unit_sphere);
	}

	// If the client has requested the entire level-of-detail pyramid be cached.
	// This does not consume memory until each individual tile is requested.
	// For example, if all level 0 tiles are accessed but none of the other levels then memory
	// will only be used for the level 0 tiles.
	if (d_cache_tile_textures == CACHE_TILE_TEXTURES_ENTIRE_LEVEL_OF_DETAIL_PYRAMID)
	{
		// This effectively disables any recycling that would otherwise happen in the cache.
		d_tile_texture_cache->set_min_num_objects(d_tiles.size());
		d_tile_vertices_cache->set_min_num_objects(d_tiles.size());
	}

	// Use a shader program for rendering a floating-point raster or a normal-map raster
	// (otherwise don't create a shader program and just use the fixed-function pipeline).
	create_shader_program_if_necessary(renderer);
}


float
GPlatesOpenGL::GLMultiResolutionRaster::get_level_of_detail(
		const GLMatrix &model_view_transform,
		const GLMatrix &projection_transform,
		const GLViewport &viewport,
		float level_of_detail_bias) const
{
	// Get the minimum size of a pixel in the current viewport when projected
	// onto the unit sphere (in model space).
	const double min_pixel_size_on_unit_sphere =
			GLProjectionUtils::get_min_pixel_size_on_unit_sphere(
					viewport, model_view_transform, projection_transform);

	// Calculate the level-of-detail.
	// This is the equivalent of:
	//
	//    t = t0 * 2 ^ (lod - lod_bias)
	//
	// ...where 't0' is the texel size of the *highest* resolution level-of-detail and
	// 't' is the projected size of a pixel of the viewport. And 'lod_bias' is used
	// by clients to allow the largest texel in a drawn texture to be larger than
	// a pixel in the viewport (which can result in blockiness in places in the rendered scene).
	//
	// Note: we return the un-clamped floating-point level-of-detail so clients of this class
	// can see if they need a higher resolution render-texture, for example, to render
	// our raster into - so in that case they'd increase their render-target resolution or
	// decrease their render target view frustum until the level-of-detail was zero.
	return level_of_detail_bias + INVERSE_LOG2 *
			(std::log(min_pixel_size_on_unit_sphere) -
					std::log(d_max_highest_resolution_texel_size_on_unit_sphere));
}


float
GPlatesOpenGL::GLMultiResolutionRaster::clamp_level_of_detail(
		float level_of_detail) const
{
	// Clamp to highest resolution level of detail.
	if (level_of_detail < 0)
	{
		// If we get here then even the highest resolution level-of-detail did not have enough
		// resolution for the specified level of detail but it'll have to do since its the
		// highest resolution we have to offer.
		// This is where the user will start to see magnification of the raster.
		return 0;
	}

	// Clamp to lowest resolution level of detail.
	if (level_of_detail > d_level_of_detail_pyramid.size() - 1)
	{
		// If we get there then even our lowest resolution level of detail had too much resolution
		// for the specified level of detail - but this is pretty unlikely for all but the very
		// smallest of viewports.
		//
		// Note that float can represent integers (up to 23 bits) exactly so returning as float is fine.
		return d_level_of_detail_pyramid.size() - 1;
	}

	return level_of_detail;
}


void
GPlatesOpenGL::GLMultiResolutionRaster::get_visible_tiles(
		std::vector<tile_handle_type> &visible_tiles,
		const GLMatrix &model_view_transform,
		const GLMatrix &projection_transform,
		float level_of_detail) const
{
	// There should be levels of detail and the specified level of detail should be in range.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			!d_level_of_detail_pyramid.empty() &&
				level_of_detail >= 0 &&
				level_of_detail <= d_level_of_detail_pyramid.size() - 1,
			GPLATES_ASSERTION_SOURCE);

	// Truncate floating-point level of detail down to an integer level-of-detail.
	const unsigned int pyramid_level = boost::numeric_cast<unsigned int>(level_of_detail);
	const LevelOfDetail &lod = *d_level_of_detail_pyramid[pyramid_level];

	//
	// Traverse the OBB tree of the level-of-detail and gather of list of tiles that
	// are visible in the view frustum.
	//

	// First get the view frustum planes.
	const GLFrustum frustum_planes(model_view_transform, projection_transform);

	// Get the root OBB tree node of the level-of-detail.
	const LevelOfDetail::OBBTreeNode &lod_root_obb_tree_node = lod.get_obb_tree_node(lod.obb_tree_root_node_index);

	// Recursively traverse the OBB tree to find visible tiles.
	get_visible_tiles(frustum_planes, GLFrustum::ALL_PLANES_ACTIVE_MASK, lod, lod_root_obb_tree_node, visible_tiles);
}


void
GPlatesOpenGL::GLMultiResolutionRaster::get_visible_tiles(
		const GLFrustum &frustum_planes,
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
						frustum_planes.get_planes(),
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


bool
GPlatesOpenGL::GLMultiResolutionRaster::render(
		GLRenderer &renderer,
		float level_of_detail,
		cache_handle_type &cache_handle)
{
	// The GLMultiResolutionRasterInterface interface says an exception is thrown if level-of-detail
	// is outside the valid range.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			level_of_detail >= 0 && level_of_detail <= get_num_levels_of_detail() - 1,
			GPLATES_ASSERTION_SOURCE);

	const GLMatrix &model_view_transform = renderer.gl_get_matrix(GL_MODELVIEW);
	const GLMatrix &projection_transform = renderer.gl_get_matrix(GL_PROJECTION);

	// Get the tiles visible in the view frustum of the render target in 'renderer'.
	std::vector<tile_handle_type> visible_tiles;
	get_visible_tiles(visible_tiles, model_view_transform, projection_transform, level_of_detail);

	// Return early if there are no tiles to render.
	if (visible_tiles.empty())
	{
		cache_handle = cache_handle_type();
		return false;
	}

	return render(renderer, visible_tiles, cache_handle);
}


bool
GPlatesOpenGL::GLMultiResolutionRaster::render(
		GLRenderer &renderer,
		const std::vector<tile_handle_type> &tiles,
		cache_handle_type &cache_handle)
{
	PROFILE_FUNC();

	// Make sure we leave the OpenGL state the way it was.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// Use shader program (if supported), otherwise the fixed-function pipeline.
	// A valid shader program means we have either a floating-point source raster or
	// a normal-map source raster (both of which require a shader program to render).
	unsigned int vertex_size = sizeof(vertex_type);
	if (d_render_raster_program_object)
	{
		// Bind the shader program.
		renderer.gl_bind_program_object(d_render_raster_program_object.get());
		// Set the raster texture sampler to texture unit 0.
		d_render_raster_program_object.get()->gl_uniform1i(renderer, "raster_texture_sampler", 0/*texture unit*/);

		// When rendering a normal map the vertex size is larger due to the per-vertex tangent-space frame.
		if (dynamic_cast<GLNormalMapSource *>(d_raster_source.get()))
		{
			vertex_size = sizeof(normal_map_vertex_type);
		}
		// ...or when rendering a scalar gradient map the vertex size is larger due to the per-vertex tangent-space frame.
		else if (dynamic_cast<GLScalarFieldDepthLayersSource *>(d_raster_source.get()))
		{
			vertex_size = sizeof(scalar_field_depth_layer_vertex_type);
		}
	}
	else // Fixed function...
	{
		// Use the fixed-function pipeline (available on all hardware) to render raster.
		// Enable texturing and set the texture function on texture unit 0.
		renderer.gl_enable_texture(GL_TEXTURE0, GL_TEXTURE_2D);
		renderer.gl_tex_env(GL_TEXTURE0, GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	}

#if 0
	// Used to render as wire-frame meshes instead of filled textured meshes for
	// visualising mesh density.
	renderer.gl_polygon_mode(GL_FRONT_AND_BACK, GL_LINE);
#endif

	// The cached view is a sequence of tiles for the caller to keep alive until the next frame.
	boost::shared_ptr<std::vector<ClientCacheTile> > cached_tiles(new std::vector<ClientCacheTile>());
	cached_tiles->reserve(tiles.size());

	// Render each tile.
	BOOST_FOREACH(GLMultiResolutionRaster::tile_handle_type tile_handle, tiles)
	{
		const Tile tile = get_tile(tile_handle, renderer);

		// Bind the tile texture to texture unit 0.
		renderer.gl_bind_texture(tile.tile_texture->texture, GL_TEXTURE0, GL_TEXTURE_2D);

		// Bind the current tile.
		tile.tile_vertices->vertex_array->gl_bind(renderer);

		const unsigned int num_vertices =
				tile.tile_vertices->vertex_buffer->get_buffer()->get_buffer_size() / vertex_size;
		const unsigned int num_indices =
				tile.tile_vertices->vertex_element_buffer->get_buffer()->get_buffer_size() / sizeof(vertex_element_type);

		// Draw the current tile.
		tile.tile_vertices->vertex_array->gl_draw_range_elements(
				renderer,
				GL_TRIANGLES,
				0/*start*/,
				num_vertices - 1/*end*/,
				num_indices/*count*/,
				GLVertexElementTraits<vertex_element_type>::type,
				0 /*indices_offset*/);

		// The caller will cache this tile to keep it from being prematurely recycled by our caches.
		//
		// Note that none of this has any effect if the client specified the entire level-of-detail
		// pyramid be cached (in the 'create()' method) in which case it'll get cached regardless.
		cached_tiles->push_back(ClientCacheTile(tile, d_cache_tile_textures));
	}

	// Return cached tiles to the caller.
	cache_handle = cached_tiles;

	return !tiles.empty();
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
	tile_texture_cache_type::object_shared_ptr_type tile_texture = get_tile_texture(renderer, lod_tile);

	// Get the vertices for the tile.
	tile_vertices_cache_type::object_shared_ptr_type tile_vertices = get_tile_vertices(renderer, lod_tile);

	// Return the tile to the caller.
	// Each tile has its own vertices and texture but shares the same triangles (vertex indices).
	return Tile(tile_vertices, tile_texture);
}


GPlatesOpenGL::GLMultiResolutionRaster::tile_texture_cache_type::object_shared_ptr_type
GPlatesOpenGL::GLMultiResolutionRaster::get_tile_texture(
		GLRenderer &renderer,
		const LevelOfDetailTile &lod_tile)
{
	// See if we've previously created our tile texture and
	// see if it hasn't been recycled by the texture cache.
	tile_texture_cache_type::object_shared_ptr_type tile_texture = lod_tile.tile_texture->get_cached_object();
	if (!tile_texture)
	{
		tile_texture = lod_tile.tile_texture->recycle_an_unused_object();
		if (!tile_texture)
		{
			// Create a new tile texture.
			tile_texture = lod_tile.tile_texture->set_cached_object(
					std::auto_ptr<TileTexture>(new TileTexture(renderer)),
					// Called whenever tile texture is returned to the cache...
					boost::bind(&TileTexture::returned_to_cache, _1));

			// The texture was just allocated so we need to create it in OpenGL.
			create_texture(renderer, tile_texture->texture);

			//qDebug() << "GLMultiResolutionRaster: " << d_tile_texture_cache->get_current_num_objects_in_use();
		}

		load_raster_data_into_tile_texture(
				lod_tile,
				*tile_texture,
				renderer);
	}
	// Our texture wasn't recycled but see if it's still valid in case the source
	// raster changed the data underneath us.
	else if (!d_raster_source->get_subject_token().is_observer_up_to_date(lod_tile.source_texture_observer_token))
	{
		// Load the data into the texture.
		load_raster_data_into_tile_texture(
				lod_tile,
				*tile_texture,
				renderer);
	}

	return tile_texture;
}


void
GPlatesOpenGL::GLMultiResolutionRaster::load_raster_data_into_tile_texture(
		const LevelOfDetailTile &lod_tile,
		TileTexture &tile_texture,
		GLRenderer &renderer)
{
	PROFILE_FUNC();

	// Get our source to load data into the texture.
	tile_texture.source_cache_handle =
			d_raster_source->load_tile(
					lod_tile.lod_level,
					lod_tile.u_lod_texel_offset,
					lod_tile.v_lod_texel_offset,
					lod_tile.num_u_lod_texels,
					lod_tile.num_v_lod_texels,
					tile_texture.texture,
					renderer);

	// This tile texture is now update-to-date.
	d_raster_source->get_subject_token().update_observer(lod_tile.source_texture_observer_token);
}


// We use macros in <GL/glew.h> that contain old-style casts.
DISABLE_GCC_WARNING("-Wold-style-cast")

void
GPlatesOpenGL::GLMultiResolutionRaster::create_texture(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_type &texture)
{
	const GLint internal_format = d_raster_source->get_target_texture_internal_format();

#if 0
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
	texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
#endif

	// No mipmap filter for the GL_TEXTURE_MAG_FILTER filter regardless.
	texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Specify anisotropic filtering if it's supported since rasters near the north or
	// south pole will exhibit squashing along the longitude, but not the latitude, direction.
	// Regular isotropic filtering will just reduce texel resolution equally along both
	// directions and reduce the visual sharpness that we want to retain in the latitude direction.
	//
	// NOTE: We don't enable anisotropic filtering for floating-point textures since earlier
	// hardware (that supports floating-point textures) only supports nearest filtering.
	if (!GLTexture::is_format_floating_point(internal_format) &&
		GLEW_EXT_texture_filter_anisotropic &&
		d_fixed_point_texture_filter == FIXED_POINT_TEXTURE_FILTER_ANISOTROPIC)
	{
		const GLfloat anisotropy = GLContext::get_parameters().texture.gl_texture_max_anisotropy;
		texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
	}

	// Clamp texture coordinates to centre of edge texels -
	// it's easier for hardware to implement - and doesn't affect our calculations.
	if (GLEW_EXT_texture_edge_clamp || GLEW_SGIS_texture_edge_clamp)
	{
		texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	else
	{
		texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}

	// Create the texture in OpenGL - this actually creates the texture without any data.
	// We'll be getting our raster source to load image data into the texture.
	//
	// NOTE: Since the image data is NULL it doesn't really matter what 'format' and 'type' are -
	// just use values that are compatible with all internal formats to avoid a possible error.
	texture->gl_tex_image_2D(
			renderer, GL_TEXTURE_2D, 0,
			internal_format,
			d_tile_texel_dimension, d_tile_texel_dimension,
			0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	// Check there are no OpenGL errors.
	GLUtils::assert_no_gl_errors(GPLATES_ASSERTION_SOURCE);
}

ENABLE_GCC_WARNING("-Wold-style-cast")


GPlatesOpenGL::GLMultiResolutionRaster::tile_vertices_cache_type::object_shared_ptr_type
GPlatesOpenGL::GLMultiResolutionRaster::get_tile_vertices(
		GLRenderer &renderer,
		const LevelOfDetailTile &lod_tile)
{
	// See if we've previously created our tile vertices and
	// see if they haven't been recycled by the tile vertices cache.
	tile_vertices_cache_type::object_shared_ptr_type tile_vertices = lod_tile.tile_vertices->get_cached_object();
	if (!tile_vertices)
	{
		tile_vertices = lod_tile.tile_vertices->recycle_an_unused_object();
		if (!tile_vertices)
		{
			tile_vertices = lod_tile.tile_vertices->set_cached_object(
					std::auto_ptr<TileVertices>(new TileVertices(renderer)));

			// Bind the new vertex buffer to the new vertex array.
			// This only needs to be done once since the vertex buffer and vertex array
			// are always created together.
			if (dynamic_cast<GLNormalMapSource *>(d_raster_source.get()))
			{
				// Normal-map vertices are larger due to per-vertex tangent-space frame.
				bind_vertex_buffer_to_vertex_array<normal_map_vertex_type>(
						renderer, *tile_vertices->vertex_array, tile_vertices->vertex_buffer);
			}
			else if (dynamic_cast<GLScalarFieldDepthLayersSource *>(d_raster_source.get()))
			{
				// Scalar-gradient-map vertices are larger due to per-vertex tangent-space frame.
				bind_vertex_buffer_to_vertex_array<scalar_field_depth_layer_vertex_type>(
						renderer, *tile_vertices->vertex_array, tile_vertices->vertex_buffer);
			}
			else
			{
				bind_vertex_buffer_to_vertex_array<vertex_type>(
						renderer, *tile_vertices->vertex_array, tile_vertices->vertex_buffer);
			}
		}

		// Get the vertex indices for this tile.
		// Since most tiles can share these indices we store them in a map keyed on
		// the number of vertices in each dimension.
		tile_vertices->vertex_element_buffer =
				get_vertex_element_buffer(renderer, lod_tile.x_num_vertices, lod_tile.y_num_vertices);

		// Bind the vertex element buffer for the current tile to the vertex array.
		// We have to do this each time we recycle (or create) a tile since the previous vertex
		// elements (indices) may not be appropriate for the current tile (due to partial boundary tiles).
		//
		// When we draw the vertex array it will use this vertex element buffer.
		tile_vertices->vertex_array->set_vertex_element_buffer(renderer, tile_vertices->vertex_element_buffer);

		// Load the tile vertices.
		load_vertices_into_tile_vertex_buffer(renderer, lod_tile, *tile_vertices);
	}

	return tile_vertices;
}


void
GPlatesOpenGL::GLMultiResolutionRaster::load_vertices_into_tile_vertex_buffer(
		GLRenderer &renderer,
		const LevelOfDetailTile &lod_tile,
		TileVertices &tile_vertices)
{
	PROFILE_FUNC();

	// Total number of vertices in this tile.
	const unsigned int num_vertices_in_tile =
			lod_tile.x_num_vertices * lod_tile.y_num_vertices;


	// Allocate memory for the geo-referenced vertex positions.
	// If we're rendering surface normals then we need extra positions around the border
	// of the tile so we can calculate tangent-space frames for each tile vertex.
	std::vector<GPlatesMaths::UnitVector3D> vertex_positions;
	vertex_positions.reserve(num_vertices_in_tile);

	// Set up some variables before initialising the geo-referenced vertex positions.
	const double inverse_x_num_quads = 1.0 / (lod_tile.x_num_vertices - 1);
	const double inverse_y_num_quads = 1.0 / (lod_tile.y_num_vertices - 1);
	const double x_pixels_per_quad =
			inverse_x_num_quads * (lod_tile.x_geo_end - lod_tile.x_geo_start);
	const double y_pixels_per_quad =
			inverse_y_num_quads * (lod_tile.y_geo_end - lod_tile.y_geo_start);

	// Calculate the geo-referenced vertex positions.
	unsigned int j;
	for (j = 0; j < lod_tile.y_num_vertices; ++j)
	{
		// NOTE: The positions of the last row of vertices should
		// match up identically with the adjacent tile otherwise
		// cracks will appear in the raster along tile edges and
		// missing pixels can show up intermittently.
		const double y = (j == lod_tile.y_num_vertices - 1)
				? lod_tile.y_geo_end
				: lod_tile.y_geo_start + j * y_pixels_per_quad;

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

			vertex_positions.push_back(vertex_position.position_vector());
		}
	}

	// Vertex size.
	unsigned int vertex_size = sizeof(vertex_type);
	// When rendering a normal map the vertex size is larger due to the per-vertex tangent-space frame.
	if (dynamic_cast<GLNormalMapSource *>(d_raster_source.get()))
	{
		vertex_size = sizeof(normal_map_vertex_type);
	}
	// ...or when rendering a scalar gradient map the vertex size is larger due to the per-vertex tangent-space frame.
	else if (dynamic_cast<GLScalarFieldDepthLayersSource *>(d_raster_source.get()))
	{
		vertex_size = sizeof(scalar_field_depth_layer_vertex_type);
	}

	// Allocate memory for the vertex array.
	const unsigned int vertex_buffer_size_in_bytes = num_vertices_in_tile * vertex_size;

	// The memory is allocated directly in the vertex buffer.
	//
	// NOTE: We could use USAGE_DYNAMIC_DRAW but that is useful if updating every few frames or so.
	// In our case we typically update much less frequently than that so it's better to use USAGE_STATIC_DRAW
	// to hint to the driver to store vertices in faster video memory rather than AGP memory.
	tile_vertices.vertex_buffer->get_buffer()->gl_buffer_data(
			renderer,
			GLBuffer::TARGET_ARRAY_BUFFER,
			vertex_buffer_size_in_bytes,
			NULL/*data*/, // We allocating memory but not initialising it yet.
			GLBuffer::USAGE_STATIC_DRAW);

	// Get access to the allocated buffer.
	// The buffer will be unmapped at scope exit.
	GLBuffer::MapBufferScope map_vertex_buffer_scope(
			renderer,
			*tile_vertices.vertex_buffer->get_buffer(),
			GLBuffer::TARGET_ARRAY_BUFFER);
	// NOTE: This is a write-only pointer - it might reference video memory - and cannot be read from.
	GLvoid *vertex_data_write_ptr =
			map_vertex_buffer_scope.gl_map_buffer_static(GLBuffer::ACCESS_WRITE_ONLY);

	//
	// Initialise the vertices
	//

	// Set up some variables before initialising the vertices.
	const double u_increment_per_quad = inverse_x_num_quads * (lod_tile.u_end - lod_tile.u_start);
	const double v_increment_per_quad = inverse_y_num_quads * (lod_tile.v_end - lod_tile.v_start);

	// Only needed if gradients are calculated.
	const double inv_num_texels_per_vertex = double(1 << 16) / d_num_texels_per_vertex;

	// Calculate the vertices.
	for (j = 0; j < lod_tile.y_num_vertices; ++j)
	{
		// The 'v' texture coordinate.
		const double v = lod_tile.v_start + j * v_increment_per_quad;

		for (unsigned int i = 0; i < lod_tile.x_num_vertices; ++i)
		{
			// Get the geo-referenced vertex position.
			const GPlatesMaths::UnitVector3D &vertex_position =
					vertex_positions[i + j * lod_tile.x_num_vertices];

			// The 'u' texture coordinate.
			const double u = lod_tile.u_start + i * u_increment_per_quad;

			if (dynamic_cast<GLNormalMapSource *>(d_raster_source.get()))
			{
				// Get the adjacent vertex positions.
				GPlatesMaths::UnitVector3D vertex_position01 = vertex_position;
				GPlatesMaths::UnitVector3D vertex_position21 = vertex_position;
				GPlatesMaths::UnitVector3D vertex_position10 = vertex_position;
				GPlatesMaths::UnitVector3D vertex_position12 = vertex_position;
				bool has_vertex_position01, has_vertex_position21, has_vertex_position10, has_vertex_position12;
				get_adjacent_vertex_positions(
						vertex_position01, has_vertex_position01,
						vertex_position21, has_vertex_position21,
						vertex_position10, has_vertex_position10,
						vertex_position12, has_vertex_position12,
						lod_tile, vertex_positions, i, j, x_pixels_per_quad, y_pixels_per_quad);

				// Calculate the tangent-space frame of the current vertex.
				const TangentSpaceFrame tangent_space_frame = calculate_tangent_space_frame(
						vertex_position,
						vertex_position01, vertex_position21, vertex_position10, vertex_position12);

				// Normal map vertex pointer into vertex buffer memory.
				normal_map_vertex_type *vertex =
						static_cast<normal_map_vertex_type *>(vertex_data_write_ptr);

				// Write directly into (un-initialised) memory in the vertex buffer.
				// NOTE: We cannot read this memory.
				new (vertex) normal_map_vertex_type(
						vertex_position,
						u, v,
						tangent_space_frame.tangent,
						tangent_space_frame.binormal,
						tangent_space_frame.normal);

				// Advance the vertex buffer write pointer.
				vertex_data_write_ptr = vertex + 1;
			}
			else if (dynamic_cast<GLScalarFieldDepthLayersSource *>(d_raster_source.get()))
			{
				// Get the adjacent vertex positions.
				GPlatesMaths::UnitVector3D vertex_position01 = vertex_position;
				GPlatesMaths::UnitVector3D vertex_position21 = vertex_position;
				GPlatesMaths::UnitVector3D vertex_position10 = vertex_position;
				GPlatesMaths::UnitVector3D vertex_position12 = vertex_position;
				bool has_vertex_position01, has_vertex_position21, has_vertex_position10, has_vertex_position12;
				get_adjacent_vertex_positions(
						vertex_position01, has_vertex_position01,
						vertex_position21, has_vertex_position21,
						vertex_position10, has_vertex_position10,
						vertex_position12, has_vertex_position12,
						lod_tile, vertex_positions, i, j, x_pixels_per_quad, y_pixels_per_quad);

				// Per-texel distance vector of constant 'u' and 'v'.
				GPlatesMaths::Vector3D delta_u =
						GPlatesMaths::Vector3D(vertex_position21) - GPlatesMaths::Vector3D(vertex_position01);
				GPlatesMaths::Vector3D delta_v =
						GPlatesMaths::Vector3D(vertex_position12) - GPlatesMaths::Vector3D(vertex_position10);

				// The inverse num texels makes the inverse distance a per-texel measure.
				if (has_vertex_position21 && has_vertex_position01)
				{
					delta_u = 0.5 * inv_num_texels_per_vertex * delta_u;
				}
				else // distance vector covers one vertex edge instead of two...
				{
					delta_u = inv_num_texels_per_vertex * delta_u;
				}
				if (has_vertex_position12 && has_vertex_position10)
				{
					delta_v = 0.5 * inv_num_texels_per_vertex * delta_v;
				}
				else // distance vector covers one vertex edge instead of two...
				{
					delta_v = inv_num_texels_per_vertex * delta_v;
				}

				// Per-texel inverse distance vector of constant 'u' and 'v'.
				// Using inverse magnitude squared since need one inverse magnitude is to normalise and
				// the other inverse magnitude is to generate the inverse distance part of gradient calculation.
				GPlatesMaths::Vector3D inv_delta_u_tangent(0,0,0);
				GPlatesMaths::Vector3D inv_delta_v_binormal(0,0,0);
				if (!are_almost_exactly_equal(delta_u.magSqrd(), 0))
				{
					inv_delta_u_tangent = (1.0 / delta_u.magSqrd()) * delta_u;
				}
				if (!are_almost_exactly_equal(delta_v.magSqrd(), 0))
				{
					inv_delta_v_binormal = (1.0 / delta_v.magSqrd()) * delta_v;
				}

				// The surface normal points outwards from the sphere regardless of tangent and binormal directions.
				const GPlatesMaths::Vector3D normal(vertex_position);

				// Scalar/gradient map vertex pointer into vertex buffer memory.
				scalar_field_depth_layer_vertex_type *vertex =
						static_cast<scalar_field_depth_layer_vertex_type *>(vertex_data_write_ptr);

				// Write directly into (un-initialised) memory in the vertex buffer.
				// NOTE: We cannot read this memory.
				new (vertex) scalar_field_depth_layer_vertex_type(
						vertex_position,
						u, v,
						inv_delta_u_tangent, inv_delta_v_binormal, normal);

				// Advance the vertex buffer write pointer.
				vertex_data_write_ptr = vertex + 1;
			}
			else // source raster is *not* a normal map...
			{
				// Write the vertex to vertex buffer memory.
				vertex_type *vertex = static_cast<vertex_type *>(vertex_data_write_ptr);

				// Write directly into (un-initialised) memory in the vertex buffer.
				// NOTE: We cannot read this memory.
				new (vertex) vertex_type(vertex_position, u, v);

				// Advance the vertex buffer write pointer.
				vertex_data_write_ptr = vertex + 1;
			}
		}
	}
}


void
GPlatesOpenGL::GLMultiResolutionRaster::get_adjacent_vertex_positions(
		GPlatesMaths::UnitVector3D &vertex_position01,
		bool &has_vertex_position01,
		GPlatesMaths::UnitVector3D &vertex_position21,
		bool &has_vertex_position21,
		GPlatesMaths::UnitVector3D &vertex_position10,
		bool &has_vertex_position10,
		GPlatesMaths::UnitVector3D &vertex_position12,
		bool &has_vertex_position12,
		const LevelOfDetailTile &lod_tile,
		const std::vector<GPlatesMaths::UnitVector3D> &vertex_positions,
		const unsigned int i,
		const unsigned int j,
		const double &x_pixels_per_quad,
		const double &y_pixels_per_quad)
{
	//
	// Calculate the vertex positions above/below/left/right of the current vertex.
	//

	if (i != 0) // vertex in tile
	{
		vertex_position01 = vertex_positions[i - 1 + j * lod_tile.x_num_vertices];
		has_vertex_position01 = true;
	}
	else if (lod_tile.x_geo_start != 0) // vertex outside tile but in raster
	{
		vertex_position01 = convert_pixel_coord_to_geographic_coord(
				lod_tile.x_geo_start - x_pixels_per_quad,
				lod_tile.y_geo_start + j * y_pixels_per_quad).position_vector();
		has_vertex_position01 = true;
	}
	else // vertex outside raster - just use raster edge
	{
		has_vertex_position01 = false;
	}

	if (i != lod_tile.x_num_vertices - 1) // vertex in tile
	{
		vertex_position21 = vertex_positions[i + 1 + j * lod_tile.x_num_vertices];
		has_vertex_position21 = true;
	}
	else if (lod_tile.x_geo_end != d_raster_width) // vertex outside tile but in raster
	{
		vertex_position21 = convert_pixel_coord_to_geographic_coord(
				lod_tile.x_geo_end + x_pixels_per_quad,
				lod_tile.y_geo_start + j * y_pixels_per_quad).position_vector();
		has_vertex_position21 = true;
	}
	else // vertex outside raster - just use raster edge
	{
		has_vertex_position21 = false;
	}

	if (j != 0) // vertex in tile
	{
		vertex_position10 = vertex_positions[i + (j - 1) * lod_tile.x_num_vertices];
		has_vertex_position10 = true;
	}
	else if (lod_tile.y_geo_start != 0) // vertex outside tile but in raster
	{
		vertex_position10 = convert_pixel_coord_to_geographic_coord(
							lod_tile.x_geo_start + i * x_pixels_per_quad,
							lod_tile.y_geo_start - y_pixels_per_quad).position_vector();
		has_vertex_position10 = true;
	}
	else // vertex outside raster - just use raster edge
	{
		has_vertex_position10 = false;
	}

	if (j != lod_tile.y_num_vertices - 1) // vertex in tile
	{
		vertex_position12 = vertex_positions[i + (j + 1) * lod_tile.x_num_vertices];
		has_vertex_position12 = true;
	}
	else if (lod_tile.y_geo_end != d_raster_height) // vertex outside tile but in raster
	{
		vertex_position12 = convert_pixel_coord_to_geographic_coord(
							lod_tile.x_geo_start + i * x_pixels_per_quad,
							lod_tile.y_geo_end + y_pixels_per_quad).position_vector();
		has_vertex_position12 = true;
	}
	else // vertex outside raster - just use raster edge
	{
		has_vertex_position12 = false;
	}
}


GPlatesOpenGL::GLMultiResolutionRaster::TangentSpaceFrame
GPlatesOpenGL::GLMultiResolutionRaster::calculate_tangent_space_frame(
		const GPlatesMaths::UnitVector3D &vertex_position,
		const GPlatesMaths::UnitVector3D &vertex_position01,
		const GPlatesMaths::UnitVector3D &vertex_position21,
		const GPlatesMaths::UnitVector3D &vertex_position10,
		const GPlatesMaths::UnitVector3D &vertex_position12)
{
	// Calculate the tangent-space frame of the specified vertex.
	//
	// NOTE: Depending on how the raster is geo-referenced onto the globe we could get
	// a left-handed or right-handed tangent-space coordinate system. In other words you
	// could imagine flipping a raster about one of its two geo-referenced coordinates
	// and this would change from left (or right) coordinate system to right (or left).
	//
	// NOTE: The tangent-space frame is not necessarily orthogonal and so the tangent and
	// binormal can be non-orthogonal to each other (but still orthogonal to the normal).
	// This depends on the geo-referencing and is fine - the shader program will still
	// normalise each (world-space) surface normal pixel.

	boost::optional<GPlatesMaths::UnitVector3D> tangent;
	boost::optional<GPlatesMaths::UnitVector3D> binormal;
	// The surface normal points outwards from the sphere regardless of tangent and binormal.
	const GPlatesMaths::UnitVector3D &normal = vertex_position;

	// Vector of constant 'u' coming into current vertex.
	GPlatesMaths::Vector3D delta_u10 =
			GPlatesMaths::Vector3D(vertex_position) - GPlatesMaths::Vector3D(vertex_position01);
	// Vector of constant 'u' going out of current vertex.
	GPlatesMaths::Vector3D delta_u21 =
			GPlatesMaths::Vector3D(vertex_position21) - GPlatesMaths::Vector3D(vertex_position);
	// Normalise, unless the length is zero (in which case it won't contribute).
	if (!are_almost_exactly_equal(delta_u10.magSqrd(), 0))
	{
		delta_u10 = (1.0 / delta_u10.magnitude()) * delta_u10;
	}
	if (!are_almost_exactly_equal(delta_u21.magSqrd(), 0))
	{
		delta_u21 = (1.0 / delta_u21.magnitude()) * delta_u21;
	}

	// The tangent is the average of the vectors of constant 'u'.
	GPlatesMaths::Vector3D delta_u = delta_u21 + delta_u10;
	// Normalise, unless the length is zero (in which case tangent could not be determined).
	if (!are_almost_exactly_equal(delta_u.magSqrd(), 0))
	{
		tangent = delta_u.get_normalisation();
	}

	// Vector of constant 'v' coming into current vertex.
	GPlatesMaths::Vector3D delta_v10 =
			GPlatesMaths::Vector3D(vertex_position) - GPlatesMaths::Vector3D(vertex_position10);
	// Vector of constant 'v' going out of current vertex.
	GPlatesMaths::Vector3D delta_v21 =
			GPlatesMaths::Vector3D(vertex_position12) - GPlatesMaths::Vector3D(vertex_position);
	// Normalise, unless the length is zero (in which case it won't contribute).
	if (!are_almost_exactly_equal(delta_v10.magSqrd(), 0))
	{
		delta_v10 = (1.0 / delta_v10.magnitude()) * delta_v10;
	}
	if (!are_almost_exactly_equal(delta_v21.magSqrd(), 0))
	{
		delta_v21 = (1.0 / delta_v21.magnitude()) * delta_v21;
	}

	// The tangent is the average of the vectors of constant 'v'.
	GPlatesMaths::Vector3D delta_v = delta_v21 + delta_v10;
	// Normalise, unless the length is zero (in which case binormal could not be determined).
	if (!are_almost_exactly_equal(delta_v.magSqrd(), 0))
	{
		binormal = delta_v.get_normalisation();
	}

	// If both tangent and binormal could not be determined then generate any arbitrary
	// orthonormal frame using 'normal'. This could happen near the north or south pole.
	// Typically the height pixels should all be the same when they're all bunched near
	// a pole like that and so the surface normals should all be normal to the surface
	// (ie, no tangent/binormal components) and hence the arbitrary tangent/binormal frame
	// won't get used in the shader program when converting surface normals to world-space.
	if (!tangent && !binormal)
	{
		tangent = generate_perpendicular(normal);
		// Cross-product produces very close to unit vector but not good enough for
		// UnitVector3D constructor so using 'get_normalisation()' instead.
		binormal = cross(normal, tangent.get()).get_normalisation();
	}
	else if (!tangent)
	{
		// Cross-product produces very close to unit vector but not good enough for
		// UnitVector3D constructor so using 'get_normalisation()' instead.
		tangent = cross(binormal.get(), normal).get_normalisation();
	}
	else if (!binormal)
	{
		// Cross-product produces very close to unit vector but not good enough for
		// UnitVector3D constructor so using 'get_normalisation()' instead.
		binormal = cross(normal, tangent.get()).get_normalisation();
	}

	return TangentSpaceFrame(tangent.get(), binormal.get(), normal);
}


GPlatesOpenGL::GLMultiResolutionRaster::texels_per_vertex_fixed_point_type
GPlatesOpenGL::GLMultiResolutionRaster::calculate_num_texels_per_vertex()
{
	// We're calculating the texel sampling density for the entire raster.
	const unsigned int x_geo_start = 0;
	const unsigned int x_geo_end = d_raster_width;
	const unsigned int y_geo_start = 0;
	const unsigned int y_geo_end = d_raster_height;

	// Centre point of the raster.
	const double x_geo_centre = 0.5 * (x_geo_start + x_geo_end);
	const double y_geo_centre = 0.5 * (y_geo_start + y_geo_end);

	// The nine boundary points including corners and midpoints and one centre point.
	const GPlatesMaths::PointOnSphere sample_points[3][3] =
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

	// Calculate the maximum angle spanned by the raster in the x direction.
	GPlatesMaths::real_t x_min_half_span = 1;

	// Iterate over the half segments and calculate dot products in the x direction.
	for (unsigned int i = 0; i < 3; ++i)
	{
		for (unsigned int j = 0; j < 2; ++j)
		{
			const GPlatesMaths::real_t x_half_span = dot(
					sample_points[i][j].position_vector(),
					sample_points[i][j+1].position_vector());
			if (x_half_span < x_min_half_span)
			{
				x_min_half_span = x_half_span;
			}
		}
	}

	// Calculate the maximum angle spanned by the raster in the y direction.
	GPlatesMaths::real_t y_min_half_span = 1;

	// Iterate over the half segments and calculate dot products in the y direction.
	for (unsigned int j = 0; j < 3; ++j)
	{
		for (unsigned int i = 0; i < 2; ++i)
		{
			const GPlatesMaths::real_t y_half_span = dot(
					sample_points[i][j].position_vector(),
					sample_points[i+1][j].position_vector());
			if (y_half_span < y_min_half_span)
			{
				y_min_half_span = y_half_span;
			}
		}
	}

	// Convert from dot product to angle.
	const GPlatesMaths::real_t x_max_spanned_angle_in_radians = 2 * acos(x_min_half_span);
	const GPlatesMaths::real_t y_max_spanned_angle_in_radians = 2 * acos(y_min_half_span);

	// Determine number of quads (segments) along each edge.
	const GPlatesMaths::real_t x_num_quads_based_on_distance_real =
			GPlatesMaths::convert_rad_to_deg(x_max_spanned_angle_in_radians) /
					MAX_ANGLE_IN_DEGREES_BETWEEN_VERTICES;
	const GPlatesMaths::real_t y_num_quads_based_on_distance_real =
			GPlatesMaths::convert_rad_to_deg(y_max_spanned_angle_in_radians) /
					MAX_ANGLE_IN_DEGREES_BETWEEN_VERTICES;

	// Determine the texel-per-vertex density along each edge.
	const GPlatesMaths::real_t x_num_texels_per_vertex =
			GPlatesMaths::real_t(d_raster_width) / x_num_quads_based_on_distance_real;
	const GPlatesMaths::real_t y_num_texels_per_vertex =
			GPlatesMaths::real_t(d_raster_height) / y_num_quads_based_on_distance_real;

	// Choose the minimum number of texels per vertex.
	// If the raster is very low resolution then it will need more vertices per texel to keep
	// the mesh tessellated finely enough (so it looks smooth and curve when drawn on the globe).
	const GPlatesMaths::real_t num_texels_per_vertex =
			(x_num_texels_per_vertex.dval() < y_num_texels_per_vertex.dval())
			? x_num_texels_per_vertex
			: y_num_texels_per_vertex;

	// Convert to 16:16 fixed-point format.
	texels_per_vertex_fixed_point_type num_texels_per_vertex_fixed_point = MAX_NUM_TEXELS_PER_VERTEX << 16;
	if (num_texels_per_vertex.dval() < MAX_NUM_TEXELS_PER_VERTEX)
	{
		num_texels_per_vertex_fixed_point =
				static_cast<texels_per_vertex_fixed_point_type>(
						num_texels_per_vertex.dval() * (1 << 16));

		// If, for some reason, the floating-point value is so low that we don't have enough
		// fixed-point precision (16-bits) to capture it then set it to the lowest fixed-point value.
		if (num_texels_per_vertex_fixed_point == 0)
		{
			num_texels_per_vertex_fixed_point = 1;
		}
	}

	//qDebug() << "texels/vertex: " << num_texels_per_vertex_fixed_point / 65536.0;

	return num_texels_per_vertex_fixed_point;
}


void
GPlatesOpenGL::GLMultiResolutionRaster::create_shader_program_if_necessary(
		GLRenderer &renderer)
{
	// If the source raster is a normal map then use a shader program instead of the fixed-function pipeline.
	// This converts the surface normals from tangent-space to world-space so they can be captured
	// in a cube raster (which is decoupled from the raster geo-referencing or tangent-space).
	if (dynamic_cast<GLNormalMapSource *>(d_raster_source.get()))
	{
		GLShaderProgramUtils::ShaderSource fragment_shader_source;

		// Configure shader for converting tangent-space surface normals to world-space.
		fragment_shader_source.add_shader_source("#define SURFACE_NORMALS\n");

		// Finally add the GLSL 'main()' function.
		fragment_shader_source.add_shader_source_from_file(RENDER_RASTER_FRAGMENT_SHADER_SOURCE_FILE_NAME);

		d_render_raster_program_object =
				GLShaderProgramUtils::compile_and_link_fragment_program(
						renderer,
						fragment_shader_source);

		// We should be able to compile/link the shader program since the client should have
		// called 'supports_normal_map_source()' which does a test compile/link.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				d_render_raster_program_object,
				GPLATES_ASSERTION_SOURCE);
	}
	else if (dynamic_cast<GLScalarFieldDepthLayersSource *>(d_raster_source.get()))
	{
		GLShaderProgramUtils::ShaderSource fragment_shader_source;

		// Configure shader for completing the gradient calculation for a scalar field.
		fragment_shader_source.add_shader_source("#define SCALAR_GRADIENT\n");

		// Finally add the GLSL 'main()' function.
		fragment_shader_source.add_shader_source_from_file(RENDER_RASTER_FRAGMENT_SHADER_SOURCE_FILE_NAME);

		d_render_raster_program_object =
				GLShaderProgramUtils::compile_and_link_fragment_program(
						renderer,
						fragment_shader_source);

		// We should be able to compile/link the shader program since the client should have
		// called 'supports_normal_map_source()' which does a test compile/link.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				d_render_raster_program_object,
				GPLATES_ASSERTION_SOURCE);
	}
	// Else if the source raster is floating-point (ie, not coloured as fixed-point for visual display)
	// then use a shader program instead of the fixed-function pipeline.
	else if (GLTexture::is_format_floating_point(d_raster_source->get_target_texture_internal_format()))
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

		GLShaderProgramUtils::ShaderSource fragment_shader_source;

		// Configure shader for floating-point rasters.
		fragment_shader_source.add_shader_source("#define SOURCE_RASTER_IS_FLOATING_POINT\n");

		// Finally add the GLSL 'main()' function.
		fragment_shader_source.add_shader_source_from_file(RENDER_RASTER_FRAGMENT_SHADER_SOURCE_FILE_NAME);

		d_render_raster_program_object =
				GLShaderProgramUtils::compile_and_link_fragment_program(
						renderer,
						fragment_shader_source);

		// The shader cannot get any simpler so if it fails to compile/link then something is wrong.
		// Also the client will have called 'GLDataRasterSource::is_supported()' which verifies
		// vertex/fragment shader support - so that should not be the reason for failure.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				d_render_raster_program_object,
				GPLATES_ASSERTION_SOURCE);
	}
	else
	{
		// Don't create a shader program - using fixed-function pipeline.
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

	// Determine the number of quads along each tile edge based on the texel resolution.
	// 'd_num_texels_per_vertex' is a 16:16 fixed-point type to allow fractional values without
	// floating-point precision issues potentially causing adjacent tiles to have different
	// tessellation (different number of vertices along common edge) and hence create gaps/cracks.
	//
	// Make sure we don't overflow the fixed-point calculation.
	// For tile dimensions less than 65,536 we should be fine.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			num_u_texels < (1 << 16) && num_v_texels < (1 << 16),
			GPLATES_ASSERTION_SOURCE);
	unsigned int x_num_quads = (num_u_texels << 16) / d_num_texels_per_vertex;
	unsigned int y_num_quads = (num_v_texels << 16) / d_num_texels_per_vertex;

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
	unsigned int x_num_vertices = x_num_quads + 1;
	unsigned int y_num_vertices = y_num_quads + 1;

	// Since we're using GLushort to store our vertex indices, we can't have
	// more than 65535 vertices per tile.
	if (x_num_vertices > (1 << (8 * sizeof(vertex_element_type) / 2/*sqrt*/)))
	{
		x_num_vertices = (1 << (8 * sizeof(vertex_element_type) / 2/*sqrt*/));
	}
	if (y_num_vertices > (1 << (8 * sizeof(vertex_element_type) / 2/*sqrt*/)))
	{
		y_num_vertices = (1 << (8 * sizeof(vertex_element_type) / 2/*sqrt*/));
	}

	//qDebug() << "Num vertices: " << x_num_vertices << ", " << y_num_vertices;

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
			*d_tile_vertices_cache,
			*d_tile_texture_cache);

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

	// Set up some variables before calculating the boundary vertices.
	const double x_pixels_per_quad =
			static_cast<double>(lod_tile.x_geo_end - lod_tile.x_geo_start) /
					(lod_tile.x_num_vertices - 1);
	const double y_pixels_per_quad =
			static_cast<double>(lod_tile.y_geo_end - lod_tile.y_geo_start) /
					(lod_tile.y_num_vertices - 1);

	// Expand the oriented bounding box to include all vertices of the current tile.
	// The value of '4' is because:
	//  1) the lowest resolution can wrap around the entire globe (for a global raster), and
	//  2) the second lowest resolution can also wrap around the entire globe if the dimension,
	//     in pixels, of this level-of-detail is slightly above the tile dimension, and
	//  3) the third lowest resolution can wrap around *half* the entire globe, and
	//  4) the fourth lowest resolution can wrap around a *quarter* of the entire globe.
	// ...so, for the fourth lowest resolution (and higher resolutions), it is fine to exclude interior points.
	if (lod_tile.lod_level + 4 <= d_level_of_detail_pyramid.size())
	{
		// For high enough resolutions we only need the boundary vertices to accomplish this because
		// the interior vertices will then be bounded along the OBB's x and y axes (due to the boundary
		// vertices) and the z-axis will be bounded along its negative direction (due to the boundary
		// vertices) and the z-axis will be bounded along its positive direction due to the extremal
		// point already added in 'create_oriented_bounding_box_builder()'.

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
	}
	else // The lowest resolution levels of detail...
	{
		// Bound the tile's interior and exterior points since the level-of-detail is a low enough
		// resolution that (for a global raster) it could wrap around the globe more than 90 degrees.
		// This means we cannot exclude the interior points.
		for (unsigned int j = 0; j < lod_tile.y_num_vertices; ++j)
		{
			// Bound the tile's top and bottom edge vertices.
			for (unsigned int i = 0; i < lod_tile.x_num_vertices; ++i)
			{
				obb_builder.add(convert_pixel_coord_to_geographic_coord(
						lod_tile.x_geo_start + i * x_pixels_per_quad,
						lod_tile.y_geo_start + j * y_pixels_per_quad));
			}
		}
	}

	return obb_builder.get_oriented_bounding_box();
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

			// Sample plus one texel in x direction.
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

			// Sample plus one texel in y direction.
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


GPlatesOpenGL::GLVertexElementBuffer::shared_ptr_to_const_type
GPlatesOpenGL::GLMultiResolutionRaster::get_vertex_element_buffer(
		GLRenderer &renderer,
		const unsigned int num_vertices_along_tile_x_edge,
		const unsigned int num_vertices_along_tile_y_edge)
{
	// Should have at least two vertices along each edge of the tile.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			num_vertices_along_tile_x_edge > 1 && num_vertices_along_tile_y_edge > 1,
			GPLATES_ASSERTION_SOURCE);

	// Lookup our map of vertex element buffers to see if we've already created one
	// with the specified vertex dimensions.
	vertex_element_buffer_map_type::const_iterator vertex_element_buffer_iter =
			d_vertex_element_buffers.find(
					vertex_element_buffer_map_type::key_type(
							num_vertices_along_tile_x_edge,
							num_vertices_along_tile_y_edge));
	if (vertex_element_buffer_iter != d_vertex_element_buffers.end())
	{
		return vertex_element_buffer_iter->second;
	}

	//
	// We haven't already created a vertex element buffer with the specified vertex dimensions
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
			num_vertices_per_tile < (1 << (8 * sizeof(vertex_element_type))),
			GPLATES_ASSERTION_SOURCE);

	// The array to store the vertex indices.
	boost::scoped_array<vertex_element_type> buffer_data(new vertex_element_type[num_indices_per_tile]);

	// Initialise the vertex indices.
	vertex_element_type *indices = buffer_data.get();
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

	// Set up the vertex element buffer.
	GLBuffer::shared_ptr_type vertex_element_buffer_data = GLBuffer::create(renderer);
	vertex_element_buffer_data->gl_buffer_data(
			renderer,
			GLBuffer::TARGET_ELEMENT_ARRAY_BUFFER,
			num_indices_per_tile * sizeof(vertex_element_type),
			buffer_data.get(),
			// Indices written to buffer only once...
			GLBuffer::USAGE_STATIC_DRAW);
	GLVertexElementBuffer::shared_ptr_type vertex_element_buffer =
			GLVertexElementBuffer::create(renderer, vertex_element_buffer_data);

	// Add to our map of vertex element arrays.
	const vertex_element_buffer_map_type::key_type vertex_dimensions(
			num_vertices_along_tile_x_edge, num_vertices_along_tile_y_edge);
	d_vertex_element_buffers.insert(vertex_element_buffer_map_type::value_type(
			vertex_dimensions, vertex_element_buffer));

	return vertex_element_buffer;
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
		tile_vertices_cache_type &tile_vertices_cache_,
		tile_texture_cache_type &tile_texture_cache_) :
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
	tile_vertices(tile_vertices_cache_.allocate_volatile_object()),
	tile_texture(tile_texture_cache_.allocate_volatile_object())
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
