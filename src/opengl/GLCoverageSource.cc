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

#include "GLCoverageSource.h"
#include "GLContext.h"
#include "GLTextureUtils.h"
#include "GLUtils.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "property-values/ProxiedRasterResolver.h"
#include "property-values/RawRasterUtils.h"

#include "utils/Base2Utils.h"
#include "utils/Profile.h"


boost::optional<GPlatesOpenGL::GLCoverageSource::non_null_ptr_type>
GPlatesOpenGL::GLCoverageSource::create(
		const GPlatesPropertyValues::RawRaster::non_null_ptr_type &age_grid_raster,
		TextureDataFormat texture_data_format,
		unsigned int tile_texel_dimension)
{
	boost::optional<GPlatesPropertyValues::ProxiedRasterResolver::non_null_ptr_type> proxy_resolver_opt =
			GPlatesPropertyValues::ProxiedRasterResolver::create(age_grid_raster);
	if (!proxy_resolver_opt)
	{
		return boost::none;
	}

	// Get the raster dimensions.
	boost::optional<std::pair<unsigned int, unsigned int> > raster_dimensions =
			GPlatesPropertyValues::RawRasterUtils::get_raster_size(*age_grid_raster);

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

	return non_null_ptr_type(new GLCoverageSource(
			proxy_resolver_opt.get(),
			raster_width,
			raster_height,
			texture_data_format,
			tile_texel_dimension));
}


GPlatesOpenGL::GLCoverageSource::GLCoverageSource(
		const GPlatesGlobal::PointerTraits<GPlatesPropertyValues::ProxiedRasterResolver>::non_null_ptr_type &
				proxy_raster_resolver,
		unsigned int raster_width,
		unsigned int raster_height,
		TextureDataFormat texture_data_format,
		unsigned int tile_texel_dimension) :
	d_proxied_raster_resolver(proxy_raster_resolver),
	d_raster_width(raster_width),
	d_raster_height(raster_height),
	d_texture_data_format(texture_data_format),
	d_tile_texel_dimension(tile_texel_dimension),
	d_coverage_tile_working_space(new GPlatesGui::rgba8_t[tile_texel_dimension * tile_texel_dimension]),
	d_logged_tile_load_failure_warning(false)
{
	// If the texture data format is floating-point then check we have support for them.
	if (texture_data_format == TEXTURE_DATA_FORMAT_FLOATING_POINT_A32_COVERAGE)
	{
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				GPLATES_OPENGL_BOOL(GLEW_ARB_texture_float),
				GPLATES_ASSERTION_SOURCE);
	}
	else if (texture_data_format == TEXTURE_DATA_FORMAT_FLOATING_POINT_R32_COVERAGE)
	{
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				GPLATES_OPENGL_BOOL(GLEW_ARB_texture_float) && GPLATES_OPENGL_BOOL(GLEW_ARB_texture_rg),
				GPLATES_ASSERTION_SOURCE);
	}
}


GLint
GPlatesOpenGL::GLCoverageSource::get_target_texture_internal_format() const
{
	switch (d_texture_data_format)
	{
	case TEXTURE_DATA_FORMAT_FLOATING_POINT_A32_COVERAGE:
		return GL_ALPHA32F_ARB;

	case TEXTURE_DATA_FORMAT_FLOATING_POINT_R32_COVERAGE:
		return GL_R32F;

	case TEXTURE_DATA_FORMAT_FIXED_POINT_RGBA8_COVERAGE_A:
	case TEXTURE_DATA_FORMAT_FIXED_POINT_RGBA8_COVERAGE_INVERT_A:
	case TEXTURE_DATA_FORMAT_FIXED_POINT_RGBA8_COVERAGE_RGB:
	case TEXTURE_DATA_FORMAT_FIXED_POINT_RGBA8_COVERAGE_RGBA:
	default:
		return GL_RGBA8;
	}
}


GPlatesOpenGL::GLCoverageSource::cache_handle_type
GPlatesOpenGL::GLCoverageSource::load_tile(
		unsigned int level,
		unsigned int texel_x_offset,
		unsigned int texel_y_offset,
		unsigned int texel_width,
		unsigned int texel_height,
		const GLTexture::shared_ptr_type &target_texture,
		GLRenderer &renderer)
{
	PROFILE_FUNC();

	PROFILE_BEGIN(profile_proxy_raster, "GLCoverageSource: get_coverage_from_level");
	// Get the region of the raster covered by this tile at the level-of-detail of this tile.
	boost::optional<GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type> raster_coverage_opt =
			d_proxied_raster_resolver->get_coverage_from_level(
					level,
					texel_x_offset,
					texel_y_offset,
					texel_width,
					texel_height);
	PROFILE_END(profile_proxy_raster);

	// If there was an error accessing raster data then black out the texture to
	// indicate no age grid mask - the age grid coverage will come from the same raster
	// and that will fail too and it will set the appropriate mask to ensure the effect
	// is the same as if the age grid had not been connected.
	// TODO: Connect age grid mask source and age grid coverage source to the same
	// proxied raster resolver.
	if (!raster_coverage_opt)
	{
		if (!d_logged_tile_load_failure_warning)
		{
			qWarning() << "Unable to load age grid coverage data into raster tile:";

			qWarning() << "  level, texel_x_offset, texel_y_offset, texel_width, texel_height: "
					<< level << ", "
					<< texel_x_offset << ", "
					<< texel_y_offset << ", "
					<< texel_width << ", "
					<< texel_height << ", ";

			d_logged_tile_load_failure_warning = true;
		}

		// Set the coverage to zero for all pixels.
		switch (d_texture_data_format)
		{
		case TEXTURE_DATA_FORMAT_FLOATING_POINT_A32_COVERAGE:
			GLTextureUtils::fill_float_texture_2D(
					renderer, target_texture, 1.0f, GL_ALPHA, texel_width, texel_height);
			break;

		case TEXTURE_DATA_FORMAT_FLOATING_POINT_R32_COVERAGE:
			GLTextureUtils::fill_float_texture_2D(
					renderer, target_texture, 1.0f, GL_RED, texel_width, texel_height);
			break;

		case TEXTURE_DATA_FORMAT_FIXED_POINT_RGBA8_COVERAGE_A:
			GLTextureUtils::load_colour_into_rgba8_texture_2D(
					renderer, target_texture, GPlatesGui::rgba8_t(255, 255, 255, 0), texel_width, texel_height);
			break;

		case TEXTURE_DATA_FORMAT_FIXED_POINT_RGBA8_COVERAGE_INVERT_A:
			GLTextureUtils::load_colour_into_rgba8_texture_2D(
					renderer, target_texture, GPlatesGui::rgba8_t(255, 255, 255, 255), texel_width, texel_height);
			break;

		case TEXTURE_DATA_FORMAT_FIXED_POINT_RGBA8_COVERAGE_RGB:
			GLTextureUtils::load_colour_into_rgba8_texture_2D(
					renderer, target_texture, GPlatesGui::rgba8_t(0, 0, 0, 255), texel_width, texel_height);
			break;

		case TEXTURE_DATA_FORMAT_FIXED_POINT_RGBA8_COVERAGE_RGBA:
		default:
			GLTextureUtils::load_colour_into_rgba8_texture_2D(
					renderer, target_texture, GPlatesGui::rgba8_t(0, 0, 0, 0), texel_width, texel_height);
			break;
		}

		// Nothing needs caching.
		return cache_handle_type();
	}

	const float *const coverage_data = raster_coverage_opt.get()->data();
	GPlatesGui::rgba8_t *const coverage_tile_working_space = d_coverage_tile_working_space.get();

	// We only need to convert to fixed-point for the non-floating-point formats.
	if (!using_floating_point_texture())
	{
		const unsigned int num_texels = texel_width * texel_height;
		for (unsigned int texel = 0; texel < num_texels; ++texel)
		{
			const float coverage = coverage_data[texel];

			int coverage_int = static_cast<int>(coverage * 255);
			if (coverage_int < 0)
			{
				coverage_int = 0;
			}
			else if (coverage_int > 255)
			{
				coverage_int = 255;
			}
			const boost::uint8_t c = static_cast<boost::uint8_t>(coverage_int);

			// Distribute the coverage value into the pixel channels.
			switch (d_texture_data_format)
			{
			case TEXTURE_DATA_FORMAT_FIXED_POINT_RGBA8_COVERAGE_A:
				coverage_tile_working_space[texel] = GPlatesGui::rgba8_t(255, 255, 255, c);
				break;

			case TEXTURE_DATA_FORMAT_FIXED_POINT_RGBA8_COVERAGE_INVERT_A:
				coverage_tile_working_space[texel] = GPlatesGui::rgba8_t(255, 255, 255, 255 - c);
				break;

			case TEXTURE_DATA_FORMAT_FIXED_POINT_RGBA8_COVERAGE_RGB:
				coverage_tile_working_space[texel] = GPlatesGui::rgba8_t(c, c, c, 255);
				break;

			case TEXTURE_DATA_FORMAT_FIXED_POINT_RGBA8_COVERAGE_RGBA:
			default:
				coverage_tile_working_space[texel] = GPlatesGui::rgba8_t(c, c, c, c);
				break;
			}
		}
	}

	// Load the coverage data into the target texture.
	switch (d_texture_data_format)
	{
	case TEXTURE_DATA_FORMAT_FLOATING_POINT_A32_COVERAGE:
		GLTextureUtils::load_image_into_texture_2D(
				renderer,
				target_texture,
				coverage_data,
				GL_ALPHA,
				GL_FLOAT,
				texel_width,
				texel_height);
		break;
	case TEXTURE_DATA_FORMAT_FLOATING_POINT_R32_COVERAGE:
		GLTextureUtils::load_image_into_texture_2D(
				renderer,
				target_texture,
				coverage_data,
				GL_RED,
				GL_FLOAT,
				texel_width,
				texel_height);
		break;

	case TEXTURE_DATA_FORMAT_FIXED_POINT_RGBA8_COVERAGE_A:
	case TEXTURE_DATA_FORMAT_FIXED_POINT_RGBA8_COVERAGE_INVERT_A:
	case TEXTURE_DATA_FORMAT_FIXED_POINT_RGBA8_COVERAGE_RGB:
	case TEXTURE_DATA_FORMAT_FIXED_POINT_RGBA8_COVERAGE_RGBA:
	default:
		GLTextureUtils::load_image_into_rgba8_texture_2D(
				renderer,
				target_texture,
				coverage_tile_working_space,
				texel_width,
				texel_height);
		break;
	}

	// Nothing needs caching.
	return cache_handle_type();
}


bool
GPlatesOpenGL::GLCoverageSource::using_floating_point_texture() const
{
	return d_texture_data_format == TEXTURE_DATA_FORMAT_FLOATING_POINT_A32_COVERAGE ||
		d_texture_data_format == TEXTURE_DATA_FORMAT_FLOATING_POINT_R32_COVERAGE;
}
