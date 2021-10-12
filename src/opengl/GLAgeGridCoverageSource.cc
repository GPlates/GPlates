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

#include "GLAgeGridCoverageSource.h"
#include "GLContext.h"
#include "GLTextureUtils.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "property-values/RawRasterUtils.h"

#include "utils/Profile.h"


boost::optional<GPlatesOpenGL::GLAgeGridCoverageSource::non_null_ptr_type>
GPlatesOpenGL::GLAgeGridCoverageSource::create(
		const GPlatesPropertyValues::RawRaster::non_null_ptr_type &age_grid_raster,
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

	return non_null_ptr_type(new GLAgeGridCoverageSource(
			proxy_resolver_opt.get(),
			raster_width,
			raster_height,
			tile_texel_dimension));
}


GPlatesOpenGL::GLAgeGridCoverageSource::GLAgeGridCoverageSource(
		const GPlatesPropertyValues::ProxiedRasterResolver::non_null_ptr_type &proxy_raster_resolver,
		unsigned int raster_width,
		unsigned int raster_height,
		unsigned int tile_texel_dimension) :
	d_proxied_raster_resolver(proxy_raster_resolver),
	d_raster_width(raster_width),
	d_raster_height(raster_height),
	d_tile_texel_dimension(tile_texel_dimension),
	d_age_grid_coverage_tile_working_space(new GPlatesGui::rgba8_t[tile_texel_dimension * tile_texel_dimension])
{
	// Initialise age grid coverage tile working space.
	const GPlatesGui::rgba8_t white(255, 255, 255, 255);
	GPlatesGui::rgba8_t *coverage_tile_working_space = d_age_grid_coverage_tile_working_space.get();
	for (unsigned int n = 0; n < d_tile_texel_dimension * d_tile_texel_dimension; ++n)
	{
		coverage_tile_working_space[n] = white;
	}
}


void
GPlatesOpenGL::GLAgeGridCoverageSource::load_tile(
		unsigned int level,
		unsigned int texel_x_offset,
		unsigned int texel_y_offset,
		unsigned int texel_width,
		unsigned int texel_height,
		const GLTexture::shared_ptr_type &target_texture,
		GLRenderer &renderer)
{
	PROFILE_BEGIN(proxy_raster, "get_coverage_from_level");
	// Get the region of the raster covered by this tile at the level-of-detail of this tile.
	boost::optional<GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type> raster_region_opt =
			d_proxied_raster_resolver->get_coverage_from_level(
					level,
					texel_x_offset,
					texel_y_offset,
					texel_width,
					texel_height);
	PROFILE_END(proxy_raster);

	// If there was an error accessing raster data then black out the texture to
	// indicate no age grid mask - the age grid coverage will come from the same raster
	// and that will fail too and it will set the appropriate mask to ensure the effect
	// is the same as if the age grid had not been connected.
	// TODO: Connect age grid mask source and age grid coverage source to the same
	// proxied raster resolver.
	if (!raster_region_opt)
	{
		qWarning() << "Unable to load age grid coverage data into raster tile:";

		qWarning() << "  level, texel_x_offset, texel_y_offset, texel_width, texel_height: "
				<< level << ", "
				<< texel_x_offset << ", "
				<< texel_y_offset << ", "
				<< texel_width << ", "
				<< texel_height << ", ";

		// Create a black raster to load into the texture.
		const GPlatesGui::rgba8_t black(0, 0, 0, 0);
		GLTextureUtils::load_colour_into_texture(target_texture, black, texel_width, texel_height);

		return;
	}

	//
	// Keep the RGB channels as white but write the float-point coverage,
	// converted to 8-bit integer, into the alpha channel of the texture.
	//

	const float *const coverage_data = raster_region_opt.get()->data();

	GPlatesGui::rgba8_t *const coverage_tile_working_space =
			d_age_grid_coverage_tile_working_space.get();

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

		// Update: Actually storing the inverse coverage makes it easier down the line.
		coverage_tile_working_space[texel].alpha = static_cast<boost::uint8_t>(255 - coverage_int);
	}

	// Load the coverage data into the target texture.
	GLTextureUtils::load_rgba8_image_into_texture(
			target_texture,
			coverage_tile_working_space,
			texel_width,
			texel_height);
}
