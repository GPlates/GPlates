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

#include "GLProxiedRasterSource.h"

#include "GLContext.h"
#include "GLTextureUtils.h"
#include "GLUtils.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "gui/Colour.h"

#include "property-values/ProxiedRasterResolver.h"
#include "property-values/RawRasterUtils.h"

#include "utils/Profile.h"


boost::optional<GPlatesOpenGL::GLProxiedRasterSource::non_null_ptr_type>
GPlatesOpenGL::GLProxiedRasterSource::create(
		const GPlatesPropertyValues::RawRaster::non_null_ptr_type &raster,
		const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &raster_colour_palette,
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
	if (boost::numeric_cast<GLint>(tile_texel_dimension) >
		GLContext::get_texture_parameters().gl_max_texture_size)
	{
		tile_texel_dimension = GLContext::get_texture_parameters().gl_max_texture_size;
	}

	// Make sure tile_texel_dimension is a power-of-two.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			tile_texel_dimension > 0 &&
					(tile_texel_dimension & (tile_texel_dimension - 1)) == 0,
			GPLATES_ASSERTION_SOURCE);

	return non_null_ptr_type(new GLProxiedRasterSource(
			proxy_resolver_opt.get(),
			raster_colour_palette,
			raster_width,
			raster_height,
			tile_texel_dimension));
}


GPlatesOpenGL::GLProxiedRasterSource::GLProxiedRasterSource(
		const GPlatesGlobal::PointerTraits<GPlatesPropertyValues::ProxiedRasterResolver>::non_null_ptr_type &
				proxy_raster_resolver,
		const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &raster_colour_palette,
		unsigned int raster_width,
		unsigned int raster_height,
		unsigned int tile_texel_dimension) :
	d_proxied_raster_resolver(proxy_raster_resolver),
	d_raster_colour_palette(raster_colour_palette),
	d_raster_width(raster_width),
	d_raster_height(raster_height),
	d_tile_texel_dimension(tile_texel_dimension)
{
}


void
GPlatesOpenGL::GLProxiedRasterSource::load_tile(
		unsigned int level,
		unsigned int texel_x_offset,
		unsigned int texel_y_offset,
		unsigned int texel_width,
		unsigned int texel_height,
		const GLTexture::shared_ptr_type &target_texture,
		GLRenderer &renderer,
		GLRenderer::RenderTargetUsageType render_target_usage)
{
	PROFILE_FUNC();

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

	// If there was an error accessing raster data then black out the texture.
	if (!raster_region_opt)
	{
#if 0 // Too many warnings - besides already drawing error message on globe via textures.
		qWarning() << "Unable to load data into raster tile:";

		qWarning() << "  level, texel_x_offset, texel_y_offset, texel_width, texel_height: "
				<< level << ", "
				<< texel_x_offset << ", "
				<< texel_y_offset << ", "
				<< texel_width << ", "
				<< texel_height << ", ";
#endif

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
			// Load cached image into target texture.
			GLTextureUtils::load_argb32_qimage_into_texture(
					target_texture,
					error_text_image,
					0, 0);
		}
		else
		{
			// Need to load clipped copy of error text image into target texture.
			GLTextureUtils::load_argb32_qimage_into_texture(
					target_texture,
					error_text_image.copy(0, 0, texel_width, texel_height),
					0, 0);
		}

		return;
	}

	// Load the raster data into the texture.
	GLTextureUtils::load_rgba8_image_into_texture(
			target_texture,
			raster_region_opt.get()->data(),
			texel_width,
			texel_height);
}


bool
GPlatesOpenGL::GLProxiedRasterSource::change_raster(
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

	// Successfully changed to a new raster of the same dimensions as the previous one.
	return true;
}
