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

#include <opengl/OpenGL3.h>  // Should be included at TOP of ".cc" file.

#include <boost/cast.hpp>
#include <QDebug>

#include "GLVisualRasterSource.h"

#include "GL.h"
#include "GLCapabilities.h"
#include "GLContext.h"
#include "GLImageUtils.h"
#include "GLTextureUtils.h"
#include "GLUtils.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "gui/Colour.h"

#include "property-values/ProxiedRasterResolver.h"
#include "property-values/RawRasterUtils.h"

#include "utils/Base2Utils.h"
#include "utils/Profile.h"


boost::optional<GPlatesOpenGL::GLVisualRasterSource::non_null_ptr_type>
GPlatesOpenGL::GLVisualRasterSource::create(
		GL &gl,
		const GPlatesPropertyValues::RawRaster::non_null_ptr_type &raster,
		const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &raster_colour_palette,
		const GPlatesGui::Colour &raster_modulate_colour,
		unsigned int tile_texel_dimension)
{
	const GLCapabilities &capabilities = gl.get_capabilities();

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
	if (tile_texel_dimension > capabilities.gl_max_texture_size)
	{
		tile_texel_dimension = capabilities.gl_max_texture_size;
	}

	// Make sure tile_texel_dimension is a power-of-two.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			tile_texel_dimension > 0 && GPlatesUtils::Base2::is_power_of_two(tile_texel_dimension),
			GPLATES_ASSERTION_SOURCE);

	return non_null_ptr_type(new GLVisualRasterSource(
			gl,
			proxy_resolver_opt.get(),
			raster_colour_palette,
			raster_modulate_colour,
			raster_width,
			raster_height,
			tile_texel_dimension));
}


GPlatesOpenGL::GLVisualRasterSource::GLVisualRasterSource(
		GL &gl,
		const GPlatesGlobal::PointerTraits<GPlatesPropertyValues::ProxiedRasterResolver>::non_null_ptr_type &proxy_raster_resolver,
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
	d_raster_modulate_colour(raster_modulate_colour),
	d_tile_edge_working_space(new GPlatesGui::rgba8_t[tile_texel_dimension]),
	d_logged_tile_load_failure_warning(false)
{
}


GPlatesOpenGL::GLVisualRasterSource::cache_handle_type
GPlatesOpenGL::GLVisualRasterSource::load_tile(
		unsigned int level,
		unsigned int texel_x_offset,
		unsigned int texel_y_offset,
		unsigned int texel_width,
		unsigned int texel_height,
		const GLTexture::shared_ptr_type &target_texture,
		GL &gl)
{
	PROFILE_BEGIN(profile_proxy_raster, "GLVisualRasterSource: get_coloured_region_from_level");
	// Get the region of the raster covered by this tile at the level-of-detail of this tile.
	boost::optional<GPlatesPropertyValues::Rgba8RawRaster::non_null_ptr_type> raster_region_opt =
			d_proxied_raster_resolver->get_coloured_region_from_level(
					level,
					texel_x_offset,
					texel_y_offset,
					texel_width,
					texel_height,
					d_raster_colour_palette);
	PROFILE_END(profile_proxy_raster);

	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	// Bind texture before uploading to it.
	gl.BindTexture(GL_TEXTURE_2D, target_texture);

	// Our client memory image buffers are byte aligned.
	gl.PixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// If there was an error accessing raster colours...
	if (!raster_region_opt)
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
				target_texture,
				gl);

		// Nothing needs caching.
		return cache_handle_type();
	}

	// Load the colours into the texture.
	glTexSubImage2D(GL_TEXTURE_2D, 0/*level*/,
			0/*xoffset*/, 0/*yoffset*/, texel_width, texel_height,
			GL_RGBA, GL_UNSIGNED_BYTE, raster_region_opt.get()->data());

	// If the region does not occupy the entire tile then it means we've reached the right edge
	// of the raster - we duplicate the last column of texels into the adjacent column to ensure
	// that subsequent sampling of the texture at the right edge of the last column of texels
	// will generate the texel colour at the texel centres (for both nearest and bilinear filtering).
	// This sampling happens when rendering a raster into a multi-resolution cube map that has
	// a cube frustum overlap of half a texel - normally, for a full tile, the OpenGL clamp-to-edge
	// filter will handle this - however for partially filled textures we need to duplicate the edge
	// to achieve the same effect otherwise numerical precision in the graphics hardware and
	// nearest neighbour filtering could sample a garbage texel.
	if (texel_width < d_tile_texel_dimension ||
		texel_height < d_tile_texel_dimension)
	{
		unsigned int duplication_size = 1;

		// Anisotropic filtering can have a filter width greater than one (even for nearest neighbour filtering).
		// And so we need to extend the duplicated region according to the maximum anisotropy.
		const GLCapabilities &capabilities = gl.get_capabilities();
		if (capabilities.gl_EXT_texture_filter_anisotropic)
		{
			duplication_size = static_cast<unsigned int>(
					// The '1 - 1e-4' rounds up to the next integer...
					capabilities.gl_texture_max_anisotropy + 1 - 1e-4);
		}

		// Duplicate the last column into an extra 'duplication_size' columns.
		unsigned int padded_texel_width = texel_width + duplication_size;
		if (padded_texel_width > d_tile_texel_dimension)
		{
			padded_texel_width = d_tile_texel_dimension;
		}

		// See if we've reached the right edge of raster (and the raster width is not an
		// integer multiple of the tile texel dimension).
		if (texel_width < padded_texel_width)
		{
			// Copy the right edge of the region into the working space.
			GPlatesGui::rgba8_t *const working_space = d_tile_edge_working_space.get();
			// The last texel in the first row of the region.
			const GPlatesGui::rgba8_t *region_last_column = raster_region_opt.get()->data() + texel_width - 1;
			for (unsigned int y = 0; y < texel_height; ++y)
			{
				working_space[y] = *region_last_column;
				region_last_column += texel_width;
			}

			for (unsigned int texel_u_offset = texel_width;
				texel_u_offset < padded_texel_width;
				++texel_u_offset)
			{
				// Load the one-texel wide column of data from column 'texel_width-1' into column 'texel_u_offset'.
				glTexSubImage2D(
						GL_TEXTURE_2D, 0/*level*/,
						texel_u_offset/*xoffset*/, 0/*yoffset*/, 1/*width*/, texel_height/*height*/,
						GL_RGBA, GL_UNSIGNED_BYTE, d_tile_edge_working_space.get());
			}
		}

		// Duplicate the last row into an extra 'duplication_size' rows.
		unsigned int padded_texel_height = texel_height + duplication_size;
		if (padded_texel_height > d_tile_texel_dimension)
		{
			padded_texel_height = d_tile_texel_dimension;
		}

		// See if we've reached the bottom edge of raster (and the raster height is not an
		// integer multiple of the tile texel dimension).
		if (texel_height < padded_texel_height)
		{
			// Copy the bottom edge of the region into the working space.
			GPlatesGui::rgba8_t *const working_space = d_tile_edge_working_space.get();
			// The first texel in the last row of the region.
			const GPlatesGui::rgba8_t *const region_last_row =
					raster_region_opt.get()->data() + (texel_height - 1) * texel_width;
			unsigned int x = 0;
			for ( ; x < texel_width; ++x)
			{
				working_space[x] = region_last_row[x];
			}
			// Also copy the corner texel to the right to cover the empty texels where:
			//
			//   texel_width  <= x < padded_texel_width
			//   texel_height <= y < padded_texel_height
			//
			// ...this will ultimately duplicate the corner texel across that entire region.
			const GPlatesGui::rgba8_t corner_texel = working_space[texel_width - 1];
			for ( ; x < padded_texel_width; ++x)
			{
				working_space[x] = corner_texel;
			}

			for (unsigned int texel_v_offset = texel_height;
				texel_v_offset < padded_texel_height;
				++texel_v_offset)
			{
				// Load the one-texel wide row of data from row 'texel_height-1' into row 'texel_v_offset'.
				glTexSubImage2D(
						GL_TEXTURE_2D, 0/*level*/,
						0/*xoffset*/, texel_v_offset/*yoffset*/, padded_texel_width/*width*/, 1/*height*/,
						GL_RGBA, GL_UNSIGNED_BYTE, d_tile_edge_working_space.get());
			}
		}
	}

	// Nothing needs caching.
	return cache_handle_type();
}


bool
GPlatesOpenGL::GLVisualRasterSource::change_raster(
		GL &gl,
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


void
GPlatesOpenGL::GLVisualRasterSource::change_modulate_colour(
		GL &gl,
		const GPlatesGui::Colour &raster_modulate_colour)
{
	// If the colour hasn't changed then nothing to do.
	if (raster_modulate_colour == d_raster_modulate_colour)
	{
		return;
	}

	d_raster_modulate_colour = raster_modulate_colour;

	// Invalidate any raster data that *clients* may have cached.
	invalidate();
}


const GPlatesGui::Colour &
GPlatesOpenGL::GLVisualRasterSource::get_modulate_colour() const
{
	return d_raster_modulate_colour;
}


void
GPlatesOpenGL::GLVisualRasterSource::render_error_text_into_texture(
		unsigned int level,
		unsigned int texel_x_offset,
		unsigned int texel_y_offset,
		unsigned int texel_width,
		unsigned int texel_height,
		const GLTexture::shared_ptr_type &target_texture,
		GL &gl)
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
	QImage &error_text_image_argb32 = (level == 0)
			? d_error_text_image_level_zero
			: d_error_text_image_mipmap_levels;

	// Only need to build once - reduces noticeable frame-rate hitches when zooming the view.
	if (error_text_image_argb32.isNull())
	{
		// Draw error message text into image.
		error_text_image_argb32 = GLImageUtils::draw_text_into_qimage(
				error_text,
				d_tile_texel_dimension, d_tile_texel_dimension,
				3.0f/*text scale*/,
				QColor(255, 0, 0, 255)/*red text*/);

		// Convert to ARGB32 format so it's easier to load into a texture.
		error_text_image_argb32 = error_text_image_argb32.convertToFormat(QImage::Format_ARGB32);
	}

	boost::scoped_array<GPlatesGui::rgba8_t> error_text_rgba8_array;

	// Most tiles will be the tile texel dimension - it's just the stragglers around the edges of the raster.
	if (texel_width == d_tile_texel_dimension &&
		texel_height == d_tile_texel_dimension)
	{
		// Convert ARGB32 format to RGBA8.
		GLTextureUtils::load_argb32_qimage_into_rgba8_array(
				error_text_image_argb32,
				error_text_rgba8_array);
	}
	else
	{
		// Convert ARGB32 format to RGBA8.
		GLTextureUtils::load_argb32_qimage_into_rgba8_array(
				// Need to load clipped copy of error text image into raster texture...
				error_text_image_argb32.copy(0, 0, texel_width, texel_height),
				error_text_rgba8_array);
	}

	gl.BindTexture(GL_TEXTURE_2D, target_texture);

	// Load cached image into tile texture.
	glTexSubImage2D(
			GL_TEXTURE_2D, 0/*level*/,
			0/*xoffset*/, 0/*yoffset*/, texel_width, texel_height,
			GL_RGBA, GL_UNSIGNED_BYTE, error_text_rgba8_array.get());
}
