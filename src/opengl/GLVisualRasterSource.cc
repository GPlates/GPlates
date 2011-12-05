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

#include <boost/cast.hpp>
/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>
#include <opengl/OpenGL.h>
#include <QDebug>

#include "GLVisualRasterSource.h"

#include "GLContext.h"
#include "GLRenderer.h"
#include "GLTextureUtils.h"
#include "GLUtils.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "gui/Colour.h"

#include "property-values/ProxiedRasterResolver.h"
#include "property-values/RawRasterUtils.h"

#include "utils/Base2Utils.h"
#include "utils/Profile.h"


namespace GPlatesOpenGL
{
	namespace
	{
		/**
		 * The minimum number of textures in the raster cache, before any recycling can happen,
		 * is the number of objects in use in the cache multiplied by this factor.
		 *
		 * The minimum is constantly updated such that it is the maximum over all times.
		 *
		 * This gives the cache some breathing room and reduces the number of cache misses that
		 * force us to retrieve data from the proxied raster resolver.
		 */
		const float RASTER_CACHE_SIZE_FACTOR = 1.5f;
	}
}


boost::optional<GPlatesOpenGL::GLVisualRasterSource::non_null_ptr_type>
GPlatesOpenGL::GLVisualRasterSource::create(
		GLRenderer &renderer,
		const GPlatesPropertyValues::RawRaster::non_null_ptr_type &raster,
		const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &raster_colour_palette,
		const GPlatesGui::Colour &raster_modulate_colour,
		unsigned int tile_texel_dimension)
{
	boost::optional<GPlatesPropertyValues::ProxiedRasterResolver::non_null_ptr_type> proxy_resolver_opt =
			GPlatesPropertyValues::ProxiedRasterResolver::create(raster);
	if (!proxy_resolver_opt)
	{
		return boost::none;
	}

	// Get the raster dimensions.
	boost::optional<std::pair<unsigned int, unsigned int> > raster_dimensions =
			GPlatesPropertyValues::RawRasterUtils::get_raster_size(*raster);

	// If raster happens to be uninitialised then return false.
	if (!raster_dimensions)
	{
		return boost::none;
	}

	const unsigned int raster_width = raster_dimensions->first;
	const unsigned int raster_height = raster_dimensions->second;

	// Make sure our tile size does not exceed the maximum texture size...
	if (tile_texel_dimension > GLContext::get_parameters().texture.gl_max_texture_size)
	{
		tile_texel_dimension = GLContext::get_parameters().texture.gl_max_texture_size;
	}

	// Make sure tile_texel_dimension is a power-of-two.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			tile_texel_dimension > 0 && GPlatesUtils::Base2::is_power_of_two(tile_texel_dimension),
			GPLATES_ASSERTION_SOURCE);

	return non_null_ptr_type(new GLVisualRasterSource(
			renderer,
			proxy_resolver_opt.get(),
			raster_colour_palette,
			raster_modulate_colour,
			raster_width,
			raster_height,
			tile_texel_dimension));
}


GPlatesOpenGL::GLVisualRasterSource::GLVisualRasterSource(
		GLRenderer &renderer,
		const GPlatesGlobal::PointerTraits<GPlatesPropertyValues::ProxiedRasterResolver>::non_null_ptr_type &
				proxy_raster_resolver,
		const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &raster_colour_palette,
		const GPlatesGui::Colour &raster_modulate_colour,
		unsigned int raster_width,
		unsigned int raster_height,
		unsigned int tile_texel_dimension) :
	d_proxied_raster_resolver(proxy_raster_resolver),
	d_raster_colour_palette(raster_colour_palette),
	d_raster_width(raster_width),
	d_raster_height(raster_height),
	d_tile_texel_dimension(tile_texel_dimension),
	// Start with small size cache and just let the cache grow in size as needed...
	d_raster_texture_cache(GPlatesUtils::ObjectCache<GLTexture>::create()),
	d_raster_modulate_colour(raster_modulate_colour),
	d_full_screen_quad_drawable(
			GLUtils::create_full_screen_2D_coloured_textured_quad(
					renderer,
					GPlatesGui::Colour::to_rgba8(raster_modulate_colour))),
	d_logged_tile_load_failure_warning(false)
{
	initialise_level_of_detail_pyramid();
}


GPlatesOpenGL::GLVisualRasterSource::cache_handle_type
GPlatesOpenGL::GLVisualRasterSource::load_tile(
		unsigned int level,
		unsigned int texel_x_offset,
		unsigned int texel_y_offset,
		unsigned int texel_width,
		unsigned int texel_height,
		const GLTexture::shared_ptr_type &target_texture,
		GLRenderer &renderer)
{
	PROFILE_FUNC();

	// Get the tile corresponding to the request.
	Tile &tile = get_tile(level, texel_x_offset, texel_y_offset);

	// See if we've previously created our tile textures and
	// see if they haven't been recycled by the texture cache.
	
	GPlatesUtils::ObjectCache<GLTexture>::volatile_object_ptr_type &raster_volatile_texture =
			tile.get_raster_texture(*d_raster_texture_cache);
	GLTexture::shared_ptr_type raster_texture = raster_volatile_texture->get_cached_object();
	if (!raster_texture)
	{
		// See if we can update the cache's minimum number of objects before recycling occurs.
		const unsigned int min_num_cached_objects = static_cast<int>(
				RASTER_CACHE_SIZE_FACTOR * d_raster_texture_cache->get_current_num_objects_in_use());
		if (min_num_cached_objects > d_raster_texture_cache->get_min_num_objects())
		{
			d_raster_texture_cache->set_min_num_objects(min_num_cached_objects);
			//qDebug() << "Updated cache size: (" << this << ", " << min_num_cached_objects << ")";
		}

		// Attempt to recycle a texture.
		raster_texture = raster_volatile_texture->recycle_an_unused_object();
		if (!raster_texture)
		{
			raster_texture = raster_volatile_texture->set_cached_object(
					GLTexture::create_as_auto_ptr(renderer));

			// The texture was just allocated so we need to create it in OpenGL.
			create_tile_texture(renderer, raster_texture);

			//qDebug() << "GLVisualRasterSource: " << d_raster_texture_cache->get_current_num_objects_in_use();
		}

		// Load the proxied raster data into the texture.
		load_proxied_raster_data_into_raster_texture(
				level,
				texel_x_offset,
				texel_y_offset,
				texel_width,
				texel_height,
				raster_texture,
				tile,
				renderer);
	}
	// Our texture wasn't recycled but see if it's still valid in case the source raster data has changed.
	// This can happen when the raster itself changes (eg, time-dependent raster) or new colour palette.
	else if (!d_raster_data_subject_token.is_observer_up_to_date(tile.raster_data_observer_token))
	{
		// Load the proxied raster data into the texture.
		load_proxied_raster_data_into_raster_texture(
				level,
				texel_x_offset,
				texel_y_offset,
				texel_width,
				texel_height,
				raster_texture,
				tile,
				renderer);
	}

	// Copy, and modulate opacity/intensity, of the raster texture into the tile target texture.
	write_raster_texture_into_tile_target_texture(renderer, target_texture, raster_texture);

	// Keep the raster texture alive so it doesn't get recycled by other tiles.
	return raster_texture;
}


bool
GPlatesOpenGL::GLVisualRasterSource::change_raster(
		GLRenderer &renderer,
		const GPlatesPropertyValues::RawRaster::non_null_ptr_type &new_raw_raster,
		const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &raster_colour_palette)
{
	// Get the raster dimensions.
	boost::optional<std::pair<unsigned int, unsigned int> > new_raster_dimensions =
			GPlatesPropertyValues::RawRasterUtils::get_raster_size(*new_raw_raster);

	// If raster happens to be uninitialised then return false.
	if (!new_raster_dimensions)
	{
		return false;
	}

	// If the new raster dimensions don't match our current internal raster then return false.
	if (new_raster_dimensions->first != d_raster_width ||
		new_raster_dimensions->second != d_raster_height)
	{
		return false;
	}

	// Create a new proxied raster resolver to perform region queries for the new raster data.
	boost::optional<GPlatesPropertyValues::ProxiedRasterResolver::non_null_ptr_type> proxy_resolver_opt =
			GPlatesPropertyValues::ProxiedRasterResolver::create(new_raw_raster);
	if (!proxy_resolver_opt)
	{
		return false;
	}
	d_proxied_raster_resolver = proxy_resolver_opt.get();

	// New raster colour palette.
	d_raster_colour_palette = raster_colour_palette;

	// Invalidate any raster data that clients may have cached.
	invalidate();
	// Also invalidate our internal raster texture cache because we've got new raster data
	// (either a new raster or a new colour palette or both).
	d_raster_data_subject_token.invalidate();

	// Successfully changed to a new raster of the same dimensions as the previous one.
	return true;
}


void
GPlatesOpenGL::GLVisualRasterSource::change_modulate_colour(
		GLRenderer &renderer,
		const GPlatesGui::Colour &raster_modulate_colour)
{
	// If the colour hasn't changed then nothing to do.
	if (raster_modulate_colour == d_raster_modulate_colour)
	{
		return;
	}

	d_raster_modulate_colour = raster_modulate_colour;

	// New modulation colour - store it as the vertex colour of the full-screen quad.
	d_full_screen_quad_drawable =
			GLUtils::create_full_screen_2D_coloured_textured_quad(
					renderer,
					GPlatesGui::Colour::to_rgba8(raster_modulate_colour));

	// Invalidate any raster data that *clients* may have cached.
	//
	// But we don't need to invalidate our internal raster texture cache because
	// neither the raster data has changed nor has the raster colour palette - so any
	// raster data loaded from the proxied raster resolver (and cached) is still good.
	//
	// Although the source raster data has not changed the *modulated* raster data has.
	invalidate();
}


void
GPlatesOpenGL::GLVisualRasterSource::initialise_level_of_detail_pyramid()
{
	// The dimension of texels that contribute to a level-of-detail
	// (starting with the highest resolution level-of-detail).
	unsigned int lod_texel_width = d_raster_width;
	unsigned int lod_texel_height = d_raster_height;

	// Generate the levels of detail starting with the
	// highest resolution (original raster) at level 0.
	for (unsigned int lod_level = 0; ; ++lod_level)
	{
		// The number of tiles is rounded up because the last tile might only have one texel.
		const unsigned int num_x_tiles =
				(lod_texel_width + d_tile_texel_dimension - 1) / d_tile_texel_dimension;
		const unsigned int num_y_tiles =
				(lod_texel_height + d_tile_texel_dimension - 1) / d_tile_texel_dimension;

		// Create a level-of-detail.
		LevelOfDetail::non_null_ptr_type level_of_detail =
				LevelOfDetail::create(num_x_tiles, num_y_tiles);

		// Add to our level-of-detail pyramid.
		d_levels.push_back(level_of_detail);

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


GPlatesOpenGL::GLVisualRasterSource::Tile &
GPlatesOpenGL::GLVisualRasterSource::get_tile(
		unsigned int level,
		unsigned int texel_x_offset,
		unsigned int texel_y_offset)
{
	// Lookup the tile corresponding to the request.
	// The caller is required to have texel offsets start on a tile boundary.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			(texel_x_offset % d_tile_texel_dimension) == 0 &&
				(texel_y_offset % d_tile_texel_dimension) == 0,
			GPLATES_ASSERTION_SOURCE);

	const unsigned int tile_x_offset = texel_x_offset / d_tile_texel_dimension;
	const unsigned int tile_y_offset = texel_y_offset / d_tile_texel_dimension;

	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			level < d_levels.size() &&
				tile_y_offset < d_levels[level]->num_y_tiles &&
					tile_x_offset < d_levels[level]->num_x_tiles,
			GPLATES_ASSERTION_SOURCE);

	return d_levels[level]->get_tile(tile_x_offset, tile_y_offset);
}


void
GPlatesOpenGL::GLVisualRasterSource::load_proxied_raster_data_into_raster_texture(
		unsigned int level,
		unsigned int texel_x_offset,
		unsigned int texel_y_offset,
		unsigned int texel_width,
		unsigned int texel_height,
		const GLTexture::shared_ptr_type &raster_texture,
		Tile &tile,
		GLRenderer &renderer)
{
	PROFILE_BEGIN(proxy_raster, "get_coloured_region_from_level");
	// Get the region of the raster covered by this tile at the level-of-detail of this tile.
	boost::optional<GPlatesPropertyValues::Rgba8RawRaster::non_null_ptr_type> raster_region_opt =
			d_proxied_raster_resolver->get_coloured_region_from_level(
					level,
					texel_x_offset,
					texel_y_offset,
					texel_width,
					texel_height,
					d_raster_colour_palette);
	PROFILE_END(proxy_raster);

	if (raster_region_opt)
	{
		// Load the source data into the raster texture.
		GLTextureUtils::load_image_into_rgba8_texture_2D(
				renderer,
				raster_texture,
				raster_region_opt.get()->data(),
				texel_width,
				texel_height);
	}
	else
	{
		// There was an error accessing raster data so black out the texture and
		// render an error message into it.
		//
		// FIXME: We should probably deal with the error in a better way than this.
		// However it can be thought of as a visible assertion of sorts - the user sees the error message
		// clearly and it has already pointed us developers to the problem quickly on more than one occasion.
		render_error_text_into_texture(
				level,
				texel_x_offset,
				texel_y_offset,
				texel_width,
				texel_height,
				raster_texture,
				renderer);
	}

	// This raster texture tile is now update-to-date with respect to the raster data - even if an error occurred.
	d_raster_data_subject_token.update_observer(tile.raster_data_observer_token);
}


void
GPlatesOpenGL::GLVisualRasterSource::render_error_text_into_texture(
		unsigned int level,
		unsigned int texel_x_offset,
		unsigned int texel_y_offset,
		unsigned int texel_width,
		unsigned int texel_height,
		const GLTexture::shared_ptr_type &texture,
		GLRenderer &renderer)
{
	if (!d_logged_tile_load_failure_warning)
	{
		qWarning() << "Unable to load data into raster tile:";

		qWarning() << "  level, texel_x_offset, texel_y_offset, texel_width, texel_height: "
				<< level << ", "
				<< texel_x_offset << ", "
				<< texel_y_offset << ", "
				<< texel_width << ", "
				<< texel_height << ", ";

		d_logged_tile_load_failure_warning = true;
	}

	// Create a black raster to load into the texture and overlay an error message in red.
	// Create a different message depending on whether the level is zero or not.
	// This is because level zero goes through a different proxied raster resolver path
	// than levels greater than zero and different error messages help us narrow down the problem.
	const QString error_text = (level == 0)
			? "Error loading raster level 0"
			: "Error loading raster mipmap";
	QImage &error_text_image = (level == 0)
			? d_error_text_image_level_zero
			: d_error_text_image_mipmap_levels;

	// Only need to build once - reduces noticeable frame-rate hitches when zooming the view.
	if (error_text_image.isNull())
	{
		// Draw error message text into image.
		error_text_image = GLTextureUtils::draw_text_into_qimage(
				error_text,
				d_tile_texel_dimension, d_tile_texel_dimension,
				3.0f/*text scale*/,
				QColor(255, 0, 0, 255)/*red text*/);

		// Convert to ARGB32 format so it's easier to load into a texture.
		error_text_image.convertToFormat(QImage::Format_ARGB32);
	}

	// Most tiles will be the tile texel dimension - it's just the stragglers around the
	// edges of the raster.
	if (texel_width == d_tile_texel_dimension && texel_height == d_tile_texel_dimension)
	{
		// Load cached image into raster texture.
		GLTextureUtils::load_argb32_qimage_into_rgba8_texture_2D(
				renderer,
				texture,
				error_text_image,
				0, 0);
	}
	else
	{
		// Need to load clipped copy of error text image into raster texture.
		GLTextureUtils::load_argb32_qimage_into_rgba8_texture_2D(
				renderer,
				texture,
				error_text_image.copy(0, 0, texel_width, texel_height),
				0, 0);
	}
}


void
GPlatesOpenGL::GLVisualRasterSource::write_raster_texture_into_tile_target_texture(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_type &target_texture,
		const GLTexture::shared_ptr_type &raster_texture)
{
	//
	// Modulate the raster data with the opacity/intensity colour using the coloured full-screen quad drawable.
	//

	// Begin rendering to a 2D render target texture.
	GLRenderer::RenderTarget2DScope render_target_scope(renderer, target_texture);

	// Viewport that matches the tile texture size.
	renderer.gl_viewport(0, 0, d_tile_texel_dimension, d_tile_texel_dimension);

	// Clear only the colour buffer using default clear colour (all zeros).
	renderer.gl_clear(GL_COLOR_BUFFER_BIT);

	// Bind the raster texture to texture unit 0.
	renderer.gl_bind_texture(raster_texture, GL_TEXTURE0, GL_TEXTURE_2D);

	// Enable texturing on texture unit 0.
	renderer.gl_enable_texture(GL_TEXTURE0, GL_TEXTURE_2D);
	// We modulate the (interpolated) vertex colour with the texture on texture unit 0.
	// The modulation colour is in the vertices of the full-screen quad.
	renderer.gl_tex_env(GL_TEXTURE0, GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	// NOTE: We leave the model-view and projection matrices as identity as that is what we
	// we need to draw a full-screen quad.
	renderer.apply_compiled_draw_state(*d_full_screen_quad_drawable);
}


void
GPlatesOpenGL::GLVisualRasterSource::create_tile_texture(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_type &texture) const
{
	// No mipmaps needed or anisotropic filtering required.
	texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

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
	// We'll load image data into the texture later.
	texture->gl_tex_image_2D(renderer, GL_TEXTURE_2D, 0, GL_RGBA8,
			d_tile_texel_dimension, d_tile_texel_dimension,
			0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	// Check there are no OpenGL errors.
	GLUtils::assert_no_gl_errors(GPLATES_ASSERTION_SOURCE);
}