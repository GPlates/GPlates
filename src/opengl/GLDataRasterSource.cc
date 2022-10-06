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

#include <algorithm>  // std::fill_n
#include <boost/cast.hpp>
#include <boost/scoped_array.hpp>
#include <QDebug>

#include "GLDataRasterSource.h"
#include "GLContext.h"
#include "GLUtils.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "property-values/ProxiedRasterResolver.h"
#include "property-values/RawRasterUtils.h"

#include "utils/Base2Utils.h"
#include "utils/Profile.h"


boost::optional<GPlatesOpenGL::GLDataRasterSource::non_null_ptr_type>
GPlatesOpenGL::GLDataRasterSource::create(
		GL &gl,
		const GPlatesPropertyValues::RawRaster::non_null_ptr_type &data_raster,
		unsigned int tile_texel_dimension)
{
	const GLCapabilities &capabilities = gl.get_capabilities();

	// The raster type is expected to contain numerical data, not colour RGBA data.
	if (!GPlatesPropertyValues::RawRasterUtils::does_raster_contain_numerical_data(*data_raster))
	{
		return boost::none;
	}

	boost::optional<GPlatesPropertyValues::ProxiedRasterResolver::non_null_ptr_type> proxy_resolver_opt =
			GPlatesPropertyValues::ProxiedRasterResolver::create(data_raster);
	if (!proxy_resolver_opt)
	{
		return boost::none;
	}

	// Get the raster dimensions.
	boost::optional<std::pair<unsigned int, unsigned int> > raster_dimensions =
			GPlatesPropertyValues::RawRasterUtils::get_raster_size(*data_raster);

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

	return non_null_ptr_type(
			new GLDataRasterSource(
					gl,
					proxy_resolver_opt.get(),
					raster_width,
					raster_height,
					tile_texel_dimension));
}


GPlatesOpenGL::GLDataRasterSource::GLDataRasterSource(
		GL &gl,
		const GPlatesGlobal::PointerTraits<GPlatesPropertyValues::ProxiedRasterResolver>::non_null_ptr_type &
				proxy_raster_resolver,
		unsigned int raster_width,
		unsigned int raster_height,
		unsigned int tile_texel_dimension) :
	d_proxied_raster_resolver(proxy_raster_resolver),
	d_raster_width(raster_width),
	d_raster_height(raster_height),
	d_tile_texel_dimension(tile_texel_dimension),
	d_tile_pack_working_space(new float[2/*RG format*/ * tile_texel_dimension * tile_texel_dimension]),
	d_logged_tile_load_failure_warning(false)
{
}


bool
GPlatesOpenGL::GLDataRasterSource::change_raster(
		GL &gl,
		const GPlatesPropertyValues::RawRaster::non_null_ptr_type &new_data_raster)
{
	// Get the raster dimensions.
	boost::optional<std::pair<unsigned int, unsigned int> > new_raster_dimensions =
			GPlatesPropertyValues::RawRasterUtils::get_raster_size(*new_data_raster);

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
			GPlatesPropertyValues::ProxiedRasterResolver::create(new_data_raster);
	if (!proxy_resolver_opt)
	{
		return false;
	}
	d_proxied_raster_resolver = proxy_resolver_opt.get();

	// Invalidate any raster data that clients may have cached.
	invalidate();

	// Successfully changed to a new raster of the same dimensions as the previous one.
	return true;
}


GLint
GPlatesOpenGL::GLDataRasterSource::get_tile_texture_internal_format() const
{
	// Our requirement of OpenGL 3.3 supports this format.
	return GL_RG32F;
}


GPlatesOpenGL::GLDataRasterSource::cache_handle_type
GPlatesOpenGL::GLDataRasterSource::load_tile(
		unsigned int level,
		unsigned int texel_x_offset,
		unsigned int texel_y_offset,
		unsigned int texel_width,
		unsigned int texel_height,
		const GLTexture::shared_ptr_type &target_texture,
		GL &gl)
{
	PROFILE_BEGIN(profile_proxy_raster_data, "GLDataRasterSource: get_region_from_level");
	// Get the region of the raster covered by this tile at the level-of-detail of this tile.
	boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type> raster_region_opt =
			d_proxied_raster_resolver->get_region_from_level(
					level,
					texel_x_offset,
					texel_y_offset,
					texel_width,
					texel_height);
	PROFILE_END(profile_proxy_raster_data);

	PROFILE_BEGIN(profile_proxy_raster_coverage, "GLDataRasterSource: get_coverage_from_level");
	// Get the region of the raster covered by this tile at the level-of-detail of this tile.
	boost::optional<GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type> raster_coverage_opt =
			d_proxied_raster_resolver->get_coverage_from_level(
					level,
					texel_x_offset,
					texel_y_offset,
					texel_width,
					texel_height);
	PROFILE_END(profile_proxy_raster_coverage);

	// If there was an error accessing raster data, or coverage, then zero the raster data/coverage values.
	if (!raster_region_opt || !raster_coverage_opt)
	{
		handle_error_loading_source_raster(
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

	// Pack the raster data/coverage values into the target texture.
	// This will fail if the raster is not a floating-point raster.
	if (!pack_raster_data_into_tile_working_space(
			raster_region_opt.get(),
			raster_coverage_opt.get(),
			texel_width,
			texel_height,
			gl))
	{
		handle_error_loading_source_raster(
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

	// Load the packed data into the texture.
	// Use RG-only format to pack raster data/coverage values.
	//
	// Note: We load the entire tile, not just the region. These can differ at the right and bottom edges
	//       of the raster (if the raster width or height is not an integer multiple of the tile dimension).
	//
	// Note: The default GL_UNPACK_ALIGNMENT of 4 works since our source texels (8 bytes) are a multiple of 4.
	gl.TextureSubImage2D(target_texture, 0/*level*/,
			0/*xoffset*/, 0/*yoffset*/, d_tile_texel_dimension, d_tile_texel_dimension,
			GL_RG, GL_FLOAT, d_tile_pack_working_space.get());

	// Nothing needs caching.
	return cache_handle_type();
}


void
GPlatesOpenGL::GLDataRasterSource::handle_error_loading_source_raster(
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
		qWarning() << "Unable to load floating-point data/coverage data into raster tile:";

		qWarning() << "  level, texel_x_offset, texel_y_offset, texel_width, texel_height: "
				<< level << ", "
				<< texel_x_offset << ", "
				<< texel_y_offset << ", "
				<< texel_width << ", "
				<< texel_height << ", ";

		d_logged_tile_load_failure_warning = true;
	}

	// Set the data/coverage values to zero for all pixels.
	// Use RG-only format.
	std::fill_n(d_tile_pack_working_space.get(), 2 * d_tile_texel_dimension * d_tile_texel_dimension, GLfloat(0));
	// Note: The default GL_UNPACK_ALIGNMENT of 4 works since our source texels (8 bytes) are a multiple of 4.
	gl.TextureSubImage2D(target_texture, 0/*level*/,
			0/*xoffset*/, 0/*yoffset*/, d_tile_texel_dimension, d_tile_texel_dimension,
			GL_RG, GL_FLOAT, d_tile_pack_working_space.get());
}


template <typename RealType>
void
GPlatesOpenGL::GLDataRasterSource::pack_raster_data_into_tile_working_space(
		const RealType *const region_data,
		const float *const coverage_data,
		unsigned int texel_width,
		unsigned int texel_height,
		GL &gl)
{
	// Fill the requested region (and premultiply coverage).
	for (unsigned int y = 0; y < texel_height; ++y)
	{
		const RealType *src_region_data_row = region_data + y * texel_width;
		const float *src_coverage_data_row = coverage_data + y * texel_width;

		// Use RG-only format to pack raster data/coverage values.
		//
		// Note: We use the tile dimension rather than the width of the region being loaded.
		//       Usually they're the same except at the right and bottom edges of entire raster.
		float *dst_row = d_tile_pack_working_space.get() + 2/*RG*/ * y * d_tile_texel_dimension;

		for (unsigned int x = 0; x < texel_width; ++x)
		{
			const float coverage_data_texel = src_coverage_data_row[x];
			// If we've sampled outside the coverage then we have no valid data value so set it to zero
			// instead of NaN (noting that the data value is premultiplied by coverage).
			//
			// Premultiply coverage so that GPU filtering (eg, bilinear) does the right thing
			// (filtered value is sum(Wi * Ci * Xi) where Wi is filter weight, Ci is coverage and Xi is data).
			const float region_data_texel = (coverage_data_texel > 0)
					? (coverage_data_texel * src_region_data_row[x])
					: 0;

			// Distribute the data/coverage values into the red/green channels.
			dst_row[2/*RG*/ * x] = region_data_texel;
			dst_row[2/*RG*/ * x + 1] = coverage_data_texel;
		}
	}

	//
	// If the region does not occupy the entire tile then it means we've reached the right edge
	// of the raster - we duplicate the last column of texels into all columns to the right of it to
	// ensure that subsequent sampling of the texture near at right edge of the last column of texels
	// will generate the texel colour at the edge texel centres for both nearest and bilinear filtering
	// (although only nearest filtering is used). Similarly for the bottom edge of the raster.
	// Normally, for a full tile, the OpenGL clamp-to-edge filter will handle this - however for
	// partially filled textures we need to emulate clamp-to-edge in a way that will work with wide
	// filters like anisotropic filtering.
	//

	// Duplicate right edge.
	if (texel_width < d_tile_texel_dimension)
	{
		for (unsigned int y = 0; y < texel_height; ++y)
		{
			float *dst_row = d_tile_pack_working_space.get() + 2/*RG*/ * y * d_tile_texel_dimension;
			const float *src_row = dst_row;

			const float *right_edge_texel = src_row + 2/*RG*/ * (texel_width - 1);
			for (unsigned int x = texel_width; x < d_tile_texel_dimension; ++x)
			{
				dst_row[2/*RG*/ * x] = right_edge_texel[0];
				dst_row[2/*RG*/ * x + 1] = right_edge_texel[1];
			}
		}
	}

	// Duplicate bottom edge.
	if (texel_height < d_tile_texel_dimension)
	{
		const float *src_row = d_tile_pack_working_space.get() +
				2/*RG*/ * (texel_height - 1) * d_tile_texel_dimension;
		for (unsigned int x = 0; x < texel_width; ++x)
		{
			const float *bottom_edge_texel = src_row + 2/*RG*/ * x;
			for (unsigned int y = texel_height; y < d_tile_texel_dimension; ++y)
			{
				float *dst_row = d_tile_pack_working_space.get() + 2/*RG*/ * y * d_tile_texel_dimension;
				dst_row[2/*RG*/ * x] = bottom_edge_texel[0];
				dst_row[2/*RG*/ * x + 1] = bottom_edge_texel[1];
			}
		}
	}

	// Duplicate bottom-right corner texel.
	if (texel_width < d_tile_texel_dimension &&
		texel_height < d_tile_texel_dimension)
	{
		const float *bottom_right_corner_texel = d_tile_pack_working_space.get() +
				2/*RG*/ * ((texel_height - 1) * d_tile_texel_dimension + texel_width - 1);

		for (unsigned int y = texel_height; y < d_tile_texel_dimension; ++y)
		{
			float *dst_row = d_tile_pack_working_space.get() + 2/*RG*/ * y * d_tile_texel_dimension;

			for (unsigned int x = texel_width; x < d_tile_texel_dimension; ++x)
			{
				dst_row[2/*RG*/ * x] = bottom_right_corner_texel[0];
				dst_row[2/*RG*/ * x + 1] = bottom_right_corner_texel[1];
			}
		}
	}
}


bool
GPlatesOpenGL::GLDataRasterSource::pack_raster_data_into_tile_working_space(
		const GPlatesPropertyValues::RawRaster::non_null_ptr_type &raster_region,
		const GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type &raster_coverage,
		unsigned int texel_width,
		unsigned int texel_height,
		GL &gl)
{
	boost::optional<GPlatesPropertyValues::FloatRawRaster::non_null_ptr_type> float_region_tile =
			GPlatesPropertyValues::RawRasterUtils::try_raster_cast<
					GPlatesPropertyValues::FloatRawRaster>(*raster_region);
	if (float_region_tile)
	{
		pack_raster_data_into_tile_working_space(
				float_region_tile.get()->data(),
				raster_coverage->data(),
				texel_width,
				texel_height,
				gl);

		return true;
	}

	boost::optional<GPlatesPropertyValues::DoubleRawRaster::non_null_ptr_type> double_region_tile =
			GPlatesPropertyValues::RawRasterUtils::try_raster_cast<
					GPlatesPropertyValues::DoubleRawRaster>(*raster_region);
	if (double_region_tile)
	{
		pack_raster_data_into_tile_working_space(
				double_region_tile.get()->data(),
				raster_coverage->data(),
				texel_width,
				texel_height,
				gl);

		return true;
	}

	boost::optional<GPlatesPropertyValues::Int8RawRaster::non_null_ptr_type> int8_region_tile =
			GPlatesPropertyValues::RawRasterUtils::try_raster_cast<
					GPlatesPropertyValues::Int8RawRaster>(*raster_region);
	if (int8_region_tile)
	{
		pack_raster_data_into_tile_working_space(
				int8_region_tile.get()->data(),
				raster_coverage->data(),
				texel_width,
				texel_height,
				gl);

		return true;
	}

	boost::optional<GPlatesPropertyValues::UInt8RawRaster::non_null_ptr_type> uint8_region_tile =
			GPlatesPropertyValues::RawRasterUtils::try_raster_cast<
					GPlatesPropertyValues::UInt8RawRaster>(*raster_region);
	if (uint8_region_tile)
	{
		pack_raster_data_into_tile_working_space(
				uint8_region_tile.get()->data(),
				raster_coverage->data(),
				texel_width,
				texel_height,
				gl);

		return true;
	}

	boost::optional<GPlatesPropertyValues::Int16RawRaster::non_null_ptr_type> int16_region_tile =
			GPlatesPropertyValues::RawRasterUtils::try_raster_cast<
					GPlatesPropertyValues::Int16RawRaster>(*raster_region);
	if (int16_region_tile)
	{
		pack_raster_data_into_tile_working_space(
				int16_region_tile.get()->data(),
				raster_coverage->data(),
				texel_width,
				texel_height,
				gl);

		return true;
	}

	boost::optional<GPlatesPropertyValues::UInt16RawRaster::non_null_ptr_type> uint16_region_tile =
			GPlatesPropertyValues::RawRasterUtils::try_raster_cast<
					GPlatesPropertyValues::UInt16RawRaster>(*raster_region);
	if (uint16_region_tile)
	{
		pack_raster_data_into_tile_working_space(
				uint16_region_tile.get()->data(),
				raster_coverage->data(),
				texel_width,
				texel_height,
				gl);

		return true;
	}

	boost::optional<GPlatesPropertyValues::Int32RawRaster::non_null_ptr_type> int32_region_tile =
			GPlatesPropertyValues::RawRasterUtils::try_raster_cast<
					GPlatesPropertyValues::Int32RawRaster>(*raster_region);
	if (int32_region_tile)
	{
		pack_raster_data_into_tile_working_space(
				int32_region_tile.get()->data(),
				raster_coverage->data(),
				texel_width,
				texel_height,
				gl);

		return true;
	}

	boost::optional<GPlatesPropertyValues::UInt32RawRaster::non_null_ptr_type> uint32_region_tile =
			GPlatesPropertyValues::RawRasterUtils::try_raster_cast<
					GPlatesPropertyValues::UInt32RawRaster>(*raster_region);
	if (uint32_region_tile)
	{
		pack_raster_data_into_tile_working_space(
				uint32_region_tile.get()->data(),
				raster_coverage->data(),
				texel_width,
				texel_height,
				gl);

		return true;
	}

	return false;
}
