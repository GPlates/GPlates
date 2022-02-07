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

#include <boost/cast.hpp>
/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>
#include <opengl/OpenGL.h>
#include <QDebug>

#include "GLDataRasterSource.h"
#include "GLContext.h"
#include "GLRenderer.h"
#include "GLTextureUtils.h"
#include "GLUtils.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "property-values/ProxiedRasterResolver.h"
#include "property-values/RawRasterUtils.h"

#include "utils/Base2Utils.h"
#include "utils/Profile.h"


bool
GPlatesOpenGL::GLDataRasterSource::is_supported(
		GLRenderer &renderer)
{
	static bool supported = false;

	// Only test for support the first time we're called.
	static bool tested_for_support = false;
	if (!tested_for_support)
	{
		tested_for_support = true;

		const GLCapabilities &capabilities = renderer.get_capabilities();

		// Need floating-point texture support.
		// Also need vertex/fragment shader support in various other classes to render floating-point rasters.
		//
		// NOTE: The reason for doing this (instead of just using the fixed-function pipeline always)
		// is to prevent clamping (to [0,1] range) of floating-point textures.
		// The raster texture might be rendered as floating-point (if we're being used for
		// data analysis instead of visualisation). The programmable pipeline has no clamping by default
		// whereas the fixed-function pipeline does (both clamping at the fragment output and internal
		// clamping in the texture environment stages). This clamping can be controlled by the
		// 'GL_ARB_color_buffer_float' extension (which means we could use the fixed-function pipeline
		// always) but that extension is not available on Mac OSX 10.5 (Leopard) on any hardware
		// (rectified in 10.6) so instead we'll just use the programmable pipeline whenever it's available.
		if (!capabilities.texture.gl_ARB_texture_float ||
			!capabilities.shader.gl_ARB_vertex_shader ||
			!capabilities.shader.gl_ARB_fragment_shader)
		{
			// Any system with floating-point textures will typically also have shaders so only
			// need to mention lack of floating-point texture support.
			qWarning() <<
					"GLDataRasterSource: Floating-point OpenGL texture support 'GL_ARB_texture_float' is required.\n";
			return false;
		}

		// If we get this far then we have support.
		supported = true;
	}

	return supported;
}


boost::optional<GPlatesOpenGL::GLDataRasterSource::non_null_ptr_type>
GPlatesOpenGL::GLDataRasterSource::create(
		GLRenderer &renderer,
		const GPlatesPropertyValues::RawRaster::non_null_ptr_type &data_raster,
		unsigned int tile_texel_dimension)
{
	const GLCapabilities &capabilities = renderer.get_capabilities();

	if (!is_supported(renderer))
	{
		return boost::none;
	}

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
	if (tile_texel_dimension > capabilities.texture.gl_max_texture_size)
	{
		tile_texel_dimension = capabilities.texture.gl_max_texture_size;
	}

	// Make sure tile_texel_dimension is a power-of-two.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			tile_texel_dimension > 0 && GPlatesUtils::Base2::is_power_of_two(tile_texel_dimension),
			GPLATES_ASSERTION_SOURCE);

	return non_null_ptr_type(
			new GLDataRasterSource(
					renderer,
					proxy_resolver_opt.get(),
					raster_width,
					raster_height,
					tile_texel_dimension));
}


GPlatesOpenGL::GLDataRasterSource::GLDataRasterSource(
		GLRenderer &renderer,
		const GPlatesGlobal::PointerTraits<GPlatesPropertyValues::ProxiedRasterResolver>::non_null_ptr_type &
				proxy_raster_resolver,
		unsigned int raster_width,
		unsigned int raster_height,
		unsigned int tile_texel_dimension) :
	d_proxied_raster_resolver(proxy_raster_resolver),
	d_raster_width(raster_width),
	d_raster_height(raster_height),
	d_tile_texture_internal_format(
			// We use RG format where possible since it saves memory.
			// NOTE: Otherwise we use RGBA (instead of RGB) because hardware typically uses
			// four channels for RGB formats anyway and uploading to the hardware should be faster
			// since driver doesn't need to be involved (consuming CPU cycles to convert RGB to RGBA).
			renderer.get_capabilities().texture.gl_ARB_texture_rg ? GL_RG32F : GL_RGBA32F_ARB),
	d_tile_texel_dimension(tile_texel_dimension),
	d_tile_pack_working_space(
			new float[
					renderer.get_capabilities().texture.gl_ARB_texture_rg
					// RG format...
					? 2 * tile_texel_dimension * tile_texel_dimension
					// RGBA format...
					: 4 * tile_texel_dimension * tile_texel_dimension]),
	d_tile_edge_working_space(
			new float[
					renderer.get_capabilities().texture.gl_ARB_texture_rg
					// RG format...
					? 2 * tile_texel_dimension
					// RGBA format...
					: 4 * tile_texel_dimension]),
	d_logged_tile_load_failure_warning(false)
{
}


bool
GPlatesOpenGL::GLDataRasterSource::change_raster(
		GLRenderer &renderer,
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
GPlatesOpenGL::GLDataRasterSource::get_target_texture_internal_format() const
{
	return d_tile_texture_internal_format;
}


GPlatesOpenGL::GLDataRasterSource::cache_handle_type
GPlatesOpenGL::GLDataRasterSource::load_tile(
		unsigned int level,
		unsigned int texel_x_offset,
		unsigned int texel_y_offset,
		unsigned int texel_width,
		unsigned int texel_height,
		const GLTexture::shared_ptr_type &target_texture,
		GLRenderer &renderer)
{
	const GLCapabilities &capabilities = renderer.get_capabilities();

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
				renderer);

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
			renderer))
	{
		handle_error_loading_source_raster(
				level,
				texel_x_offset,
				texel_y_offset,
				texel_width,
				texel_height,
				target_texture,
				renderer);

		// Nothing needs caching.
		return cache_handle_type();
	}

	// Load the packed data into the texture.
	if (capabilities.texture.gl_ARB_texture_rg)
	{
		// Use RG-only format to pack raster data/coverage values.
		GLTextureUtils::load_image_into_texture_2D(
				renderer,
				target_texture,
				d_tile_pack_working_space.get(),
				GL_RG,
				GL_FLOAT,
				texel_width,
				texel_height);
	}
	else
	{
		// Use RGBA format to pack raster data/coverage values.
		GLTextureUtils::load_image_into_texture_2D(
				renderer,
				target_texture,
				d_tile_pack_working_space.get(),
				GL_RGBA,
				GL_FLOAT,
				texel_width,
				texel_height);
	}

	// If the region does not occupy the entire tile then it means we've reached the right edge
	// of the raster - we duplicate the last column of texels into the adjacent column to ensure
	// that subsequent sampling of the texture at the right edge of the last column of texels
	// will generate the texel colour at the texel centres (for both nearest and bilinear filtering).
	// This sampling happens when rendering a raster into a multi-resolution cube map that has
	// an cube frustum overlap of half a texel - normally, for a full tile, the OpenGL clamp-to-edge
	// filter will ensure the texture border colour is not used - however for partially filled
	// textures we need to duplicate the edge to achieve the same effect otherwise numerical precision
	// in the graphics hardware and nearest neighbour filtering could sample a garbage texel.
	//
	// Note: We don't use anisotropic filtering for floating-point textures (like we do for fixed-point)
	// and so we don't have to worry about a anisotropic filter width sampling texels beyond our
	// duplicated single row/column of border texels.
	if (texel_width < d_tile_texel_dimension)
	{
		const unsigned int texel_size_in_floats = capabilities.texture.gl_ARB_texture_rg ? 2 : 4;
		// Copy the right edge of the region into the working space.
		float *working_space = d_tile_edge_working_space.get();
		// The last texel in the first row of the region.
		const float *region_last_column = d_tile_pack_working_space.get() + texel_size_in_floats * (texel_width - 1);
		for (unsigned int y = 0; y < texel_height; ++y)
		{
			for (unsigned int component = 0; component < texel_size_in_floats; ++component)
			{
				working_space[component] = region_last_column[component];
			}
			working_space += texel_size_in_floats;
			region_last_column += texel_size_in_floats * texel_width;
		}

		// Load the one-texel wide column of data from column 'texel_width-1' into column 'texel_width'.
		GLTextureUtils::load_image_into_texture_2D(
				renderer,
				target_texture,
				d_tile_edge_working_space.get(),
				capabilities.texture.gl_ARB_texture_rg ? GL_RG : GL_RGBA,
				GL_FLOAT,
				1/*image_width*/,
				texel_height,
				texel_width/*texel_u_offset*/);
	}

	// Same applies if we've reached the bottom edge of raster (and the raster height is not an
	// integer multiple of the tile texel dimension).
	if (texel_height < d_tile_texel_dimension)
	{
		const unsigned int texel_size_in_floats = capabilities.texture.gl_ARB_texture_rg ? 2 : 4;
		// Copy the bottom edge of the region into the working space.
		float *working_space = d_tile_edge_working_space.get();
		// The first texel in the last row of the region.
		const float *region_last_row =
				d_tile_pack_working_space.get() + texel_size_in_floats * (texel_height - 1) * texel_width;
		for (unsigned int x = 0; x < texel_width; ++x)
		{
			for (unsigned int component = 0; component < texel_size_in_floats; ++component)
			{
				working_space[component] = region_last_row[component];
			}
			working_space += texel_size_in_floats;
			region_last_row += texel_size_in_floats;
		}
		// Copy the corner texel, we want it to get copied to the texel at column 'texel_width' and row 'texel_height'.
		unsigned int texels_in_last_row = texel_width;
		if (texel_width < d_tile_texel_dimension)
		{
			const float *corner_texel = region_last_row - texel_size_in_floats;
			for (unsigned int component = 0; component < texel_size_in_floats; ++component)
			{
				working_space[component] = corner_texel[component];
			}
			++texels_in_last_row;
		}

		// Load the one-texel wide row of data from row 'texel_height-1' into row 'texel_height'.
		GLTextureUtils::load_image_into_texture_2D(
				renderer,
				target_texture,
				d_tile_edge_working_space.get(),
				capabilities.texture.gl_ARB_texture_rg ? GL_RG : GL_RGBA,
				GL_FLOAT,
				texels_in_last_row/*image_width*/,
				1/*image_height*/,
				0/*texel_u_offset*/,
				texel_height/*texel_v_offset*/);
	}

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
		GLRenderer &renderer)
{
	const GLCapabilities &capabilities = renderer.get_capabilities();

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
	if (capabilities.texture.gl_ARB_texture_rg)
	{
		// Use RG-only format.
		GLTextureUtils::fill_float_texture_2D(
				renderer, target_texture, 0.0f, 0.0f, GL_RG, texel_width, texel_height);
	}
	else
	{
		// Use RGBA format.
		GLTextureUtils::fill_float_texture_2D(
				renderer, target_texture, 0.0f, 0.0f, 0.0f, 0.0f, texel_width, texel_height);
	}
}


template <typename RealType>
void
GPlatesOpenGL::GLDataRasterSource::pack_raster_data_into_tile_working_space(
		const RealType *const region_data,
		const float *const coverage_data,
		unsigned int texel_width,
		unsigned int texel_height,
		GLRenderer &renderer)
{
	const GLCapabilities &capabilities = renderer.get_capabilities();

	float *tile_pack_working_space = d_tile_pack_working_space.get();
	const unsigned int num_texels = texel_width * texel_height;

	if (capabilities.texture.gl_ARB_texture_rg)
	{
		// Use RG-only format to pack raster data/coverage values.
		for (unsigned int texel = 0; texel < num_texels; ++texel)
		{
			// If we've sampled outside the coverage then we have no valid data value so
			// just set it a valid floating-point value (ie, not NAN) so that the graphics hardware
			// can still do valid operations on it - this makes it easy to do coverage-weighted
			// calculations in order to avoid having to explicitly check that a texel coverage
			// value is zero (multiplying by zero effectively disables it but multiplying zero
			// will give NAN instead of zero). An example of this kind of calculation is
			// mean which is M = sum(Ci * Xi) / sum(Ci) where Ci is coverage and Xi is data value.
			const float coverage_data_texel = coverage_data[texel];
			const RealType region_data_texel = (coverage_data_texel > 0) ? region_data[texel] : 0;

			// Distribute the data/coverage values into the red/green channels.
			tile_pack_working_space[0] = static_cast<GLfloat>(region_data_texel);
			tile_pack_working_space[1] = coverage_data_texel;

			tile_pack_working_space += 2;
		}
	}
	else
	{
		// Use RGBA format to pack raster data/coverage values.
		for (unsigned int texel = 0; texel < num_texels; ++texel)
		{
			// See comment above.
			const float coverage_data_texel = coverage_data[texel];
			const RealType region_data_texel = (coverage_data_texel > 0) ? region_data[texel] : 0;

			// Distribute the data/coverage values into the red/green channels.
			tile_pack_working_space[0] = static_cast<GLfloat>(region_data_texel);
			tile_pack_working_space[1] = coverage_data_texel;
			tile_pack_working_space[2] = 0.0f; // This channel unused/ignored.
			tile_pack_working_space[3] = 0.0f; // This channel unused/ignored.

			tile_pack_working_space += 4;
		}
	}
}


bool
GPlatesOpenGL::GLDataRasterSource::pack_raster_data_into_tile_working_space(
		const GPlatesPropertyValues::RawRaster::non_null_ptr_type &raster_region,
		const GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type &raster_coverage,
		unsigned int texel_width,
		unsigned int texel_height,
		GLRenderer &renderer)
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
				renderer);

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
				renderer);

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
				renderer);

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
				renderer);

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
				renderer);

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
				renderer);

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
				renderer);

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
				renderer);

		return true;
	}

	return false;
}
