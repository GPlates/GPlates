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

#include <boost/scoped_array.hpp>

#include "GLRasterProxy.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "gui/ColourRawRaster.h"
#include "gui/RasterColourPalette.h"

#include "property-values/RawRasterUtils.h"


GPlatesOpenGL::GLRasterProxy::GLRasterProxy(
		const GPlatesPropertyValues::RawRaster::non_null_ptr_type &raw_raster,
		std::size_t tile_texel_dimension) :
	d_max_dimension_for_lowest_res_mipmap(tile_texel_dimension)
{
	// Get the raster dimensions.
	boost::optional<std::pair<unsigned int, unsigned int> > raster_dimensions =
			GPlatesPropertyValues::RawRasterUtils::get_raster_size(*raw_raster);

	// If raster happens to be uninitialised then throw an exception.
	if (!raster_dimensions)
	{
		throw UninitialisedRasterException(GPLATES_EXCEPTION_SOURCE);
	}

	// Convert to a rgba8 raster if it's not already one.
	GPlatesPropertyValues::Rgba8RawRaster::non_null_ptr_type rgba8_raster =
			convert_to_rgba8_raster(raw_raster);

	// Store the raster as the highest level mipmap.
	const Mipmap highest_res_mipmap(
			raster_dimensions->first, raster_dimensions->second, rgba8_raster);
	d_mipmap_pyramid.push_back(highest_res_mipmap);

	// Generate the mipmap levels.
	//
	// Ideally we should only filter the rgba texels if the original raster was in rgba format.
	// For floating-point rasters we should filter the raster values and then lookup the rgba
	// colour from a colour palette.
	// But this is temporary anyway since Enoch is creating better raster proxy.
	generate_mipmaps();
}


unsigned int
GPlatesOpenGL::GLRasterProxy::get_texel_width(
		unsigned int level_of_detail) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			level_of_detail < d_mipmap_pyramid.size(),
			GPLATES_ASSERTION_SOURCE);

	return d_mipmap_pyramid[level_of_detail].width;
}


unsigned int
GPlatesOpenGL::GLRasterProxy::get_texel_height(
		unsigned int level_of_detail) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			level_of_detail < d_mipmap_pyramid.size(),
			GPLATES_ASSERTION_SOURCE);

	return d_mipmap_pyramid[level_of_detail].height;
}


GPlatesPropertyValues::Rgba8RawRaster::non_null_ptr_type
GPlatesOpenGL::GLRasterProxy::get_raster_region(
		unsigned int level_of_detail,
		unsigned int x_texel_offset,
		unsigned int num_x_texels,
		unsigned int y_texel_offset,
		unsigned int num_y_texels) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			level_of_detail < d_mipmap_pyramid.size(),
			GPLATES_ASSERTION_SOURCE);

	const Mipmap &mipmap = d_mipmap_pyramid[level_of_detail];

	// Make sure request is within bounds.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			x_texel_offset + num_x_texels <= mipmap.width &&
					y_texel_offset + num_y_texels <= mipmap.height,
			GPLATES_ASSERTION_SOURCE);

	// Create a raster for the region we're returning to the caller.
	GPlatesPropertyValues::Rgba8RawRaster::non_null_ptr_type rgba8_raster_region =
			GPlatesPropertyValues::Rgba8RawRaster::create(num_x_texels, num_y_texels);

	// Copy the raster region.
	const GPlatesGui::rgba8_t *raster_src_row = mipmap.rgba8_raster->data()
			+ y_texel_offset * mipmap.width + x_texel_offset;
	GPlatesGui::rgba8_t *raster_dst_row = rgba8_raster_region->data();
	for (unsigned int j = 0; j < num_y_texels; ++j)
	{
		for (unsigned int i = 0; i < num_x_texels; ++i)
		{
			raster_dst_row[i] = raster_src_row[i];
		}

		// Move to the next row of pixels.
		raster_src_row += mipmap.width;
		raster_dst_row += num_x_texels;
	}

	return rgba8_raster_region;
}


GPlatesPropertyValues::Rgba8RawRaster::non_null_ptr_type
GPlatesOpenGL::GLRasterProxy::convert_to_rgba8_raster(
		const GPlatesPropertyValues::RawRaster::non_null_ptr_type &raw_raster)
{
	// See whether it's an Rgba8RawRaster already.
	boost::optional<GPlatesPropertyValues::Rgba8RawRaster::non_null_ptr_type> rgba8_raster_opt =
		GPlatesPropertyValues::RawRasterUtils::try_rgba8_raster_cast(*raw_raster);
	if (rgba8_raster_opt)
	{
		return rgba8_raster_opt.get();
	}

	// Colour using the default raster colour palette instead.

	// Get statistics.
	GPlatesPropertyValues::RasterStatistics *statistics_ptr =
		GPlatesPropertyValues::RawRasterUtils::get_raster_statistics(*raw_raster);
	if (!statistics_ptr)
	{
		throw RasterHasNoStatisticsException(GPLATES_EXCEPTION_SOURCE);
	}

	GPlatesPropertyValues::RasterStatistics &statistics = *statistics_ptr;
	if (!statistics.mean || !statistics.standard_deviation)
	{
		throw RasterHasNoStatisticsException(GPLATES_EXCEPTION_SOURCE);
	}
	double mean = *statistics.mean;
	double std_dev = *statistics.standard_deviation;

	// Create default raster colour palette.
	GPlatesGui::DefaultRasterColourPalette::non_null_ptr_type rgba8_palette =
			GPlatesGui::DefaultRasterColourPalette::create(mean, std_dev);

	// Convert the non-RGBA8 RawRaster into a RGBA8 RawRaster.
	rgba8_raster_opt = GPlatesGui::ColourRawRaster::colour_raw_raster<double>(
			*raw_raster, rgba8_palette);
	if (!rgba8_raster_opt)
	{
		throw ColourPaletteNotSuitableForRasterException(GPLATES_EXCEPTION_SOURCE);
	}

	return rgba8_raster_opt.get();
}


void
GPlatesOpenGL::GLRasterProxy::generate_mipmaps()
{
	// The mipmap we will filter is the lowest resolution mipmap generated so far.
	const Mipmap &src_mipmap = d_mipmap_pyramid.back();

	// If the lowest resolution mipmap is already under the maximum dimensions
	// then return early as there's nothing to do.
	if (src_mipmap.width <= d_max_dimension_for_lowest_res_mipmap &&
		src_mipmap.height <= d_max_dimension_for_lowest_res_mipmap)
	{
		return;
	}

	const unsigned int src_mipmap_width = src_mipmap.width;
	const unsigned int src_mipmap_height = src_mipmap.height;
	const GPlatesPropertyValues::Rgba8RawRaster::non_null_ptr_type &src_mipmap_raster =
			src_mipmap.rgba8_raster;

	const unsigned int dst_mipmap_width = (src_mipmap_width + 1) / 2;
	const unsigned int dst_mipmap_height = (src_mipmap_height + 1) / 2;

	// Create a raster for the generated mipmap.
	GPlatesPropertyValues::Rgba8RawRaster::non_null_ptr_type dst_mipmap_raster =
			GPlatesPropertyValues::Rgba8RawRaster::create(dst_mipmap_width, dst_mipmap_height);

	filter_mipmap(
			src_mipmap_raster,
			src_mipmap_width,
			src_mipmap_height,
			dst_mipmap_raster,
			dst_mipmap_width,
			dst_mipmap_height);

	// Add the generated mipmap to our pyramid.
	const Mipmap dst_mipmap(dst_mipmap_width, dst_mipmap_height, dst_mipmap_raster);
	d_mipmap_pyramid.push_back(dst_mipmap);

	// Recursively generate a lower resolution mipmap from the one just generated.
	generate_mipmaps();
}


void
GPlatesOpenGL::GLRasterProxy::filter_mipmap(
		const GPlatesPropertyValues::Rgba8RawRaster::non_null_ptr_type &src_mipmap_raster,
		const unsigned int src_mipmap_width,
		const unsigned int src_mipmap_height,
		const GPlatesPropertyValues::Rgba8RawRaster::non_null_ptr_type &dst_mipmap_raster,
		const unsigned int dst_mipmap_width,
		const unsigned int dst_mipmap_height)
{
	// 4x4 box filter the rgba8 texels.
	//
	// We'll filter a square 64x64 blocks of texels to take better advantage of the CPU memory cache.
	const unsigned int dst_block_dimension = 32;
	const unsigned int src_block_dimension = 2 * dst_block_dimension;
	// Number of blocks including partial blocks.
	const unsigned int num_dst_blocks_x =
			(dst_mipmap_width + dst_block_dimension - 1) / dst_block_dimension;
	const unsigned int num_dst_blocks_y =
			(dst_mipmap_height + dst_block_dimension - 1) / dst_block_dimension;

	// Iterate over the raster blocks.
	for (unsigned int y_block = 0; y_block < num_dst_blocks_y; ++y_block)
	{
		for (unsigned int x_block = 0; x_block < num_dst_blocks_x; ++x_block)
		{
			const GPlatesGui::rgba8_t *src_block = src_mipmap_raster->data()
					+ y_block * src_block_dimension * src_mipmap_width
					+ x_block * src_block_dimension;
			GPlatesGui::rgba8_t *dst_block = dst_mipmap_raster->data()
					+ y_block * dst_block_dimension * dst_mipmap_width
					+ x_block * dst_block_dimension;

			unsigned int dst_block_width = dst_mipmap_width - x_block * dst_block_dimension;
			if (dst_block_width > dst_block_dimension)
			{
				dst_block_width = dst_block_dimension;
			}
			unsigned int dst_block_height = dst_mipmap_height - y_block * dst_block_dimension;
			if (dst_block_height > dst_block_dimension)
			{
				dst_block_height = dst_block_dimension;
			}

			// If the current block will access src mipmap texels out-of-bounds, due to
			// src mipmap having either dimension an odd number, then copy the block
			// out of the src mipmap and pad the missing texels.
			unsigned int src_block_width_remaining = src_mipmap_width - 2 * x_block * dst_block_dimension;
			if (src_block_width_remaining > 2 * dst_block_width)
			{
				src_block_width_remaining = 2 * dst_block_width;
			}
			unsigned int src_block_height_remaining = src_mipmap_height - 2 * y_block * dst_block_dimension;
			if (src_block_height_remaining > 2 * dst_block_height)
			{
				src_block_height_remaining = 2 * dst_block_height;
			}
			if (2 * dst_block_width > src_block_width_remaining ||
				2 * dst_block_height > src_block_height_remaining)
			{
				// Allocate a new source block.
				const unsigned int src_block_padded_width = 2 * dst_block_width;
				const unsigned int src_block_padded_height = 2 * dst_block_height;
				boost::scoped_array<GPlatesGui::rgba8_t> src_block_padded(
						new GPlatesGui::rgba8_t[src_block_padded_width * src_block_padded_height]);

				copy_and_pad_block(
						src_mipmap_width,
						src_block,
						src_block_width_remaining,
						src_block_height_remaining,
						src_block_padded.get(),
						src_block_padded_width,
						src_block_padded_height);

				// Filter the copied and padded block.
				filter_block(
						src_block_padded.get(),
						src_block_padded_width,
						dst_block,
						dst_block_width,
						dst_block_height,
						dst_mipmap_width);

				continue;
			}

			// Filter the current block.
			filter_block(
					src_block,
					src_mipmap_width,
					dst_block,
					dst_block_width,
					dst_block_height,
					dst_mipmap_width);
		}
	}
}


void
GPlatesOpenGL::GLRasterProxy::copy_and_pad_block(
		const unsigned int src_mipmap_width,
		const GPlatesGui::rgba8_t *const src_block,
		const unsigned int src_block_width,
		const unsigned int src_block_height,
		GPlatesGui::rgba8_t *const src_block_padded,
		const unsigned int src_block_padded_width,
		const unsigned int src_block_padded_height)
{
	// Copy the src raster block.
	unsigned int y;
	for (y = 0; y < src_block_height; ++y)
	{
		const GPlatesGui::rgba8_t *src = src_block + y * src_mipmap_width;
		GPlatesGui::rgba8_t *src_padded = src_block_padded + y * src_block_padded_width;

		unsigned int x;
		for (x = 0; x < src_block_width; ++x)
		{
			src_padded[x] = src[x];
		}
		// If need to pad in the x-direction...
		if (x < src_block_padded_width)
		{
			// Pads the last texel by duplicating it.
			src_padded[x] = src[x - 1];
		}
	}

	// If need to pad in the y-direction...
	if (y < src_block_padded_height)
	{
		const GPlatesGui::rgba8_t *src = src_block + (y - 1) * src_mipmap_width;
		GPlatesGui::rgba8_t *src_padded = src_block_padded + y * src_block_padded_width;

		unsigned int x;
		for (x = 0; x < src_block_width; ++x)
		{
			src_padded[x] = src[x];
		}
		// If need to pad in the x-direction...
		if (x < src_block_padded_width)
		{
			// Pads the last texel by duplicating it.
			src_padded[x] = src[x - 1];
		}
	}
}


void
GPlatesOpenGL::GLRasterProxy::filter_block(
		const GPlatesGui::rgba8_t *const src_block,
		const unsigned int src_mipmap_width,
		GPlatesGui::rgba8_t *const dst_block,
		const unsigned int dst_block_width,
		const unsigned int dst_block_height,
		const unsigned int dst_mipmap_width)
{
	// Filter the current block.
	for (unsigned int y = 0; y < dst_block_height; ++y)
	{
		const GPlatesGui::rgba8_t *src_row = src_block + 2 * y * src_mipmap_width;
		GPlatesGui::rgba8_t *dst_row = dst_block + y * dst_mipmap_width;

		// Iterate through the colour components.
		// Filter one row at a time for each of four channels.
		for (unsigned int c = 0; c < 4; ++c)
		{
			const boost::uint8_t *const src_row_channel0 =
					reinterpret_cast<const boost::uint8_t *>(src_row) + c;
			const boost::uint8_t *const src_row_channel1 =
					reinterpret_cast<const boost::uint8_t *>(src_row + src_mipmap_width) + c;

			boost::uint8_t *const dst_row_channel =
					reinterpret_cast<boost::uint8_t *>(dst_row) + c;

			for (unsigned int x = 0; x < dst_block_width; ++x)
			{
				const unsigned int src00 = src_row_channel0[2 * 4 * x];
				const unsigned int src01 = src_row_channel0[2 * 4 * x + 4];
				const unsigned int src10 = src_row_channel1[2 * 4 * x];
				const unsigned int src11 = src_row_channel1[2 * 4 * x + 4];

				// 4x4 box filter
				const unsigned int dst = (src00 + src01 + src10 + src11) >> 2;
				dst_row_channel[4 * x] = static_cast<boost::uint8_t>(dst);
			}
		}
	}
}
