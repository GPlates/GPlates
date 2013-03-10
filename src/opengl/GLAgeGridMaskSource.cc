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

#include <utility>
#include <boost/cast.hpp>
#include <boost/cstdint.hpp>
/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>
#include <opengl/OpenGL.h>
#include <QDebug>

#include "GLAgeGridMaskSource.h"

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


namespace
{
	/**
	 * A 4-component texture environment colour used to extract red channel when used with GL_ARB_texture_env_dot3.
	 */
	std::vector<GLfloat>
	create_dot3_extract_red_channel()
	{
		std::vector<GLfloat> vec(4);

		vec[0] = 1;
		vec[1] = 0.5f;
		vec[2] = 0.5f;
		vec[3] = 0;

		return vec;
	}
}


boost::optional<GPlatesOpenGL::GLAgeGridMaskSource::non_null_ptr_type>
GPlatesOpenGL::GLAgeGridMaskSource::create(
		GLRenderer &renderer,
		const double &reconstruction_time,
		const GPlatesPropertyValues::RawRaster::non_null_ptr_type &age_grid_raster,
		unsigned int tile_texel_dimension)
{
	// The raster type is expected to contain numerical data, not colour RGBA data.
	if (!GPlatesPropertyValues::RawRasterUtils::does_raster_contain_numerical_data(*age_grid_raster))
	{
		return boost::none;
	}

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

	double min_age_in_raster = 0;
	// One bit less than 16-bit (the size of age values in our two textures combined).
	double max_age_in_raster = (1 << 15);
	// Get the raster statistics.
	GPlatesPropertyValues::RasterStatistics * raster_statistics =
			GPlatesPropertyValues::RawRasterUtils::get_raster_statistics(*age_grid_raster);
	if (raster_statistics)
	{
		if (raster_statistics->minimum)
		{
			min_age_in_raster = raster_statistics->minimum.get();
		}
		if (raster_statistics->maximum)
		{
			max_age_in_raster = raster_statistics->maximum.get();
		}

		if (max_age_in_raster <= min_age_in_raster)
		{
			qWarning() << "Invalid age range in age grid raster.";
			return boost::none;
		}
	}

	// Make sure our tile size does not exceed the maximum texture size...
	if (tile_texel_dimension > renderer.get_context().get_capabilities().texture.gl_max_texture_size)
	{
		tile_texel_dimension = renderer.get_context().get_capabilities().texture.gl_max_texture_size;
	}

	// Make sure tile_texel_dimension is a power-of-two.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			tile_texel_dimension > 0 && GPlatesUtils::Base2::is_power_of_two(tile_texel_dimension),
			GPLATES_ASSERTION_SOURCE);

	return non_null_ptr_type(
			new GLAgeGridMaskSource(
					renderer,
					reconstruction_time,
					proxy_resolver_opt.get(),
					raster_width,
					raster_height,
					tile_texel_dimension,
					min_age_in_raster,
					max_age_in_raster));
}


GPlatesOpenGL::GLAgeGridMaskSource::GLAgeGridMaskSource(
		GLRenderer &renderer,
		const double &reconstruction_time,
		const GPlatesGlobal::PointerTraits<GPlatesPropertyValues::ProxiedRasterResolver>::non_null_ptr_type &
				proxy_raster_resolver,
		unsigned int raster_width,
		unsigned int raster_height,
		unsigned int tile_texel_dimension,
		double min_age_in_raster,
		double max_age_in_raster) :
	d_current_reconstruction_time(reconstruction_time),
	d_proxied_raster_resolver(proxy_raster_resolver),
	d_raster_width(raster_width),
	d_raster_height(raster_height),
	d_tile_texel_dimension(tile_texel_dimension),
	// Start with smallest size cache and just let the cache grow in size as needed...
	d_age_grid_texture_cache(GPlatesUtils::ObjectCache<GLTexture>::create()),
	// These textures get reused even inside a single rendering frame so we just need a small number
	// to give the graphics card some breathing room (in terms of render-texture dependencies)...
	d_intermediate_render_texture_cache(GPlatesUtils::ObjectCache<GLTexture>::create(2)),
	d_full_screen_quad_drawable(renderer.get_context().get_shared_state()->get_full_screen_2D_textured_quad(renderer)),
	d_first_render_pass_state(renderer.create_empty_compiled_draw_state()),
	d_second_render_pass_state(renderer.create_empty_compiled_draw_state()),
	d_third_render_pass_state(renderer.create_empty_compiled_draw_state()),
	d_raster_min_age(min_age_in_raster),
	d_raster_max_age(max_age_in_raster),
	// Factor to convert floating-point age range to 16-bit integer...
	d_raster_inv_age_range_factor(0xffff / (max_age_in_raster - min_age_in_raster)),
	d_age_high_byte_tile_working_space(new GPlatesGui::rgba8_t[tile_texel_dimension * tile_texel_dimension]),
	d_age_low_byte_tile_working_space(new GPlatesGui::rgba8_t[tile_texel_dimension * tile_texel_dimension]),
	d_logged_tile_load_failure_warning(false)
{
	// Set our current integer reconstruction time.
	convert_age_to_16_bit_integer(
			d_current_reconstruction_time.dval(),
			d_current_reconstruction_time_high_byte,
			d_current_reconstruction_time_low_byte);

	// Initialise high/low byte tile working space.
	const GPlatesGui::rgba8_t white(255, 255, 255, 255);
	GPlatesGui::rgba8_t *high_byte_tile_working_space = d_age_high_byte_tile_working_space.get();
	GPlatesGui::rgba8_t *low_byte_tile_working_space = d_age_low_byte_tile_working_space.get();
	for (unsigned int n = 0; n < d_tile_texel_dimension * d_tile_texel_dimension; ++n)
	{
		high_byte_tile_working_space[n] = white;
		low_byte_tile_working_space[n] = white;
	}

	//
	// Setup rendering state for the three age grid mask render passes.
	//

	{
		GLRenderer::CompileDrawStateScope compile_draw_state_scope(renderer, d_first_render_pass_state);

		renderer.gl_enable_texture(GL_TEXTURE0, GL_TEXTURE_2D);
		renderer.gl_tex_env(GL_TEXTURE0, GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		renderer.gl_enable(GL_BLEND);
		renderer.gl_blend_func(GL_ONE, GL_ZERO);
		renderer.gl_enable(GL_ALPHA_TEST);

		// Draw a full-screen quad.
		// NOTE: We leave the model-view and projection matrices as identity as that is what we
		// we need to draw a full-screen quad.
		renderer.apply_compiled_draw_state(*d_full_screen_quad_drawable);
	}

	{
		GLRenderer::CompileDrawStateScope compile_draw_state_scope(renderer, d_second_render_pass_state);

		renderer.gl_enable_texture(GL_TEXTURE0, GL_TEXTURE_2D);
		renderer.gl_tex_env(GL_TEXTURE0, GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		renderer.gl_enable(GL_BLEND);
		renderer.gl_blend_func(GL_ZERO, GL_ZERO);
		renderer.gl_enable(GL_ALPHA_TEST);

		// Draw a full-screen quad.
		// NOTE: We leave the model-view and projection matrices as identity as that is what we
		// we need to draw a full-screen quad.
		renderer.apply_compiled_draw_state(*d_full_screen_quad_drawable);
	}

	{
		GLRenderer::CompileDrawStateScope compile_draw_state_scope(renderer, d_third_render_pass_state);

		renderer.gl_enable_texture(GL_TEXTURE0, GL_TEXTURE_2D);
		renderer.gl_tex_env(GL_TEXTURE0, GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		renderer.gl_enable(GL_BLEND);
		renderer.gl_blend_func(GL_ONE, GL_ONE);
		renderer.gl_enable(GL_ALPHA_TEST);

		// Draw a full-screen quad.
		// NOTE: We leave the model-view and projection matrices as identity as that is what we
		// we need to draw a full-screen quad.
		renderer.apply_compiled_draw_state(*d_full_screen_quad_drawable);
	}

	initialise_level_of_detail_pyramid();
}


void
GPlatesOpenGL::GLAgeGridMaskSource::update_reconstruction_time(
		const double &reconstruction_time)
{
	// If the reconstruction time has changed then invalidate ourself as this
	// will inform connected clients that they need to refresh their texture caches.
	GPlatesMaths::real_t new_reconstruction_time(reconstruction_time);
	if (d_current_reconstruction_time != new_reconstruction_time)
	{
		d_current_reconstruction_time = new_reconstruction_time;
		invalidate();
	}

	// Set our current integer reconstruction time.
	convert_age_to_16_bit_integer(
			d_current_reconstruction_time.dval(),
			d_current_reconstruction_time_high_byte,
			d_current_reconstruction_time_low_byte);
}


GPlatesOpenGL::GLAgeGridMaskSource::cache_handle_type
GPlatesOpenGL::GLAgeGridMaskSource::load_tile(
		unsigned int level,
		unsigned int texel_x_offset,
		unsigned int texel_y_offset,
		unsigned int texel_width,
		unsigned int texel_height,
		const GLTexture::shared_ptr_type &target_texture,
		GLRenderer &renderer)
{
	// Get the tile corresponding to the request.
	Tile &tile = get_tile(level, texel_x_offset, texel_y_offset);

	// See if either our high or low byte age grid tile textures need reloading
	// from the input age grid raster (eg, if they were recycled by the texture cache).
	GLTexture::shared_ptr_type high_byte_age_texture, low_byte_age_texture;
	if (should_reload_high_and_low_byte_age_textures(
			renderer, tile, high_byte_age_texture, low_byte_age_texture))
	{
		PROFILE_BEGIN(profile_get_region_from_level, "GLAgeGridMaskSource: get_region_from_level");
		// Get the region of the raster covered by this tile at the level-of-detail of this tile.
		// These are the age grid *age* values.
		boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type> raster_region_opt =
				d_proxied_raster_resolver->get_region_from_level(
						level,
						texel_x_offset,
						texel_y_offset,
						texel_width,
						texel_height);
		PROFILE_END(profile_get_region_from_level);

		PROFILE_BEGIN(profile_get_coverage_from_level, "GLAgeGridMaskSource: get_coverage_from_level");
		// Get the region of the raster covered by this tile at the level-of-detail of this tile.
		// These are the age grid *coverage* values.
		boost::optional<GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type> raster_coverage_opt =
				d_proxied_raster_resolver->get_coverage_from_level(
						level,
						texel_x_offset,
						texel_y_offset,
						texel_width,
						texel_height);
		PROFILE_END(profile_get_coverage_from_level);

		// If there was an error accessing raster data then black out the texture to
		// indicate no age grid mask - the age grid coverage will come from the same raster
		// and that will fail too and it will set the appropriate mask to ensure the effect
		// is the same as if the age grid had not been connected.
		// TODO: Connect age grid mask source and age grid coverage source to the same
		// proxied raster resolver.
		if (!raster_region_opt || !raster_coverage_opt)
		{
			if (!d_logged_tile_load_failure_warning)
			{
				qWarning() << "Unable to load age grid data into raster tile:";

				qWarning() << "  level, texel_x_offset, texel_y_offset, texel_width, texel_height: "
						<< level << ", "
						<< texel_x_offset << ", "
						<< texel_y_offset << ", "
						<< texel_width << ", "
						<< texel_height << ", ";

				d_logged_tile_load_failure_warning = true;
			}

			// Create a black raster to load into the texture.
			const GPlatesGui::rgba8_t black(0, 0, 0, 0);
			GLTextureUtils::load_colour_into_rgba8_texture_2D(renderer, target_texture, black, texel_width, texel_height);

			return cache_handle_type();
		}

		// Convert the floating point age values into low and high byte textures so
		// we can do some alpha testing/blending to simulate the per-pixel comparison
		// of the reconstruction time with the age grid values and generate a binary
		// mask in the target texture.
		if (!load_age_grid_into_high_and_low_byte_tile(
				renderer,
				raster_region_opt.get(),
				raster_coverage_opt.get(),
				high_byte_age_texture,
				low_byte_age_texture,
				texel_width,
				texel_height))
		{
			// Create a black raster to load into the texture.
			const GPlatesGui::rgba8_t black(0, 0, 0, 0);
			GLTextureUtils::load_colour_into_rgba8_texture_2D(renderer, target_texture, black, texel_width, texel_height);

			return cache_handle_type();
		}
	}

	// So now we have up to date high and low byte textures so we can render
	// the age grid mask with them.
	render_age_grid_mask(renderer, target_texture, high_byte_age_texture, low_byte_age_texture);

	// Keep the high/low byte age textures alive so they don't get recycled by other tiles.
	return cache_handle_type(
			new std::pair<GLTexture::shared_ptr_type, GLTexture::shared_ptr_type>(
					high_byte_age_texture, low_byte_age_texture));
}


GPlatesOpenGL::GLAgeGridMaskSource::Tile &
GPlatesOpenGL::GLAgeGridMaskSource::get_tile(
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


bool
GPlatesOpenGL::GLAgeGridMaskSource::should_reload_high_and_low_byte_age_textures(
		GLRenderer &renderer,
		Tile &tile,
		GLTexture::shared_ptr_type &high_byte_age_texture,
		GLTexture::shared_ptr_type &low_byte_age_texture)
{
	bool should_reload = false;

	// See if we've previously created our tile textures and
	// see if they haven't been recycled by the texture cache.
	
	GPlatesUtils::ObjectCache<GLTexture>::volatile_object_ptr_type &high_byte_age_volatile_texture =
			tile.get_high_byte_age_texture(*d_age_grid_texture_cache);
	high_byte_age_texture = high_byte_age_volatile_texture->get_cached_object();
	if (!high_byte_age_texture)
	{
		should_reload = true;

		high_byte_age_texture = high_byte_age_volatile_texture->recycle_an_unused_object();
		if (!high_byte_age_texture)
		{
			high_byte_age_texture = high_byte_age_volatile_texture->set_cached_object(
					GLTexture::create_as_auto_ptr(renderer));

			// The texture was just allocated so we need to create it in OpenGL.
			create_tile_texture(renderer, high_byte_age_texture);
		}
	}

	GPlatesUtils::ObjectCache<GLTexture>::volatile_object_ptr_type &low_byte_age_volatile_texture =
			tile.get_low_byte_age_texture(*d_age_grid_texture_cache);
	low_byte_age_texture = low_byte_age_volatile_texture->get_cached_object();
	if (!low_byte_age_texture)
	{
		should_reload = true;

		low_byte_age_texture = low_byte_age_volatile_texture->recycle_an_unused_object();
		if (!low_byte_age_texture)
		{
			low_byte_age_texture = low_byte_age_volatile_texture->set_cached_object(
					GLTexture::create_as_auto_ptr(renderer));

			// The texture was just allocated so we need to create it in OpenGL.
			create_tile_texture(renderer, low_byte_age_texture);
		}
	}

	return should_reload;
}


void
GPlatesOpenGL::GLAgeGridMaskSource::create_tile_texture(
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
	// We'll be getting our raster source to load image data into the texture.
	texture->gl_tex_image_2D(renderer, GL_TEXTURE_2D, 0, GL_RGBA8,
			d_tile_texel_dimension, d_tile_texel_dimension,
			0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	// Check there are no OpenGL errors.
	GLUtils::assert_no_gl_errors(GPLATES_ASSERTION_SOURCE);
}


template <typename RealType>
void
GPlatesOpenGL::GLAgeGridMaskSource::load_age_grid_into_high_and_low_byte_tile(
		GLRenderer &renderer,
		const RealType *age_grid_age_tile,
		const float *age_grid_coverage_tile,
		GLTexture::shared_ptr_type &high_byte_age_texture,
		GLTexture::shared_ptr_type &low_byte_age_texture,
		unsigned int texel_width,
		unsigned int texel_height)
{
	GPlatesGui::rgba8_t *high_byte_tile_working_space = d_age_high_byte_tile_working_space.get();
	GPlatesGui::rgba8_t *low_byte_tile_working_space = d_age_low_byte_tile_working_space.get();

	const unsigned int num_texels = texel_width * texel_height;
	for (unsigned int texel = 0; texel < num_texels; ++texel)
	{
		const float coverage = age_grid_coverage_tile[texel];
		const bool has_coverage = (coverage > 0);

		// If we've sampled outside the coverage then we have no valid age grid value so set the age
		// to the minimum raster value - this ensures the age mask will be zero in regions not
		// covered by the age grid.
		const RealType age_grid_texel = has_coverage ? age_grid_age_tile[texel] : d_raster_min_age;

		// Convert floating-point age grid value to integer and store in the Alpha channels.
		convert_age_to_16_bit_integer(
				age_grid_texel,
				high_byte_tile_working_space[texel].alpha,
				low_byte_tile_working_space[texel].alpha);

		// Store the coverage in the Red channel.
		//
		// NOTE: We convert non-zero coverage values to 1.0 to avoid blending seams due to
		// partial alpha values. The render-target age mask values are also either 0.0 or 1.0 and
		// our clients use nearest-neighbour texture sampling with no anisotropic filtering so that
		// values of 0.0 or 1.0 remain as 0.0 or 1.0 (no inbetween values).
		// All of this ensures no alpha-blending artifacts since the final alpha used for blending
		// will always be either 0.0 or 1.0 (ie, either draw or no-draw).
		high_byte_tile_working_space[texel].red =
				low_byte_tile_working_space[texel].red =
						(has_coverage ? 255 : 0);
	}

	// Load the data into the high and low byte textures.
	GLTextureUtils::load_image_into_rgba8_texture_2D(
			renderer,
			high_byte_age_texture,
			high_byte_tile_working_space,
			texel_width,
			texel_height);
	GLTextureUtils::load_image_into_rgba8_texture_2D(
			renderer,
			low_byte_age_texture,
			low_byte_tile_working_space,
			texel_width,
			texel_height);
}


bool
GPlatesOpenGL::GLAgeGridMaskSource::load_age_grid_into_high_and_low_byte_tile(
		GLRenderer &renderer,
		const GPlatesPropertyValues::RawRaster::non_null_ptr_type &age_grid_age_tile,
		const GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type &age_grid_coverage_tile,
		GLTexture::shared_ptr_type &high_byte_age_texture,
		GLTexture::shared_ptr_type &low_byte_age_texture,
		unsigned int texel_width,
		unsigned int texel_height)
{
	PROFILE_FUNC();

	boost::optional<GPlatesPropertyValues::FloatRawRaster::non_null_ptr_type> float_age_grid_age_tile =
			GPlatesPropertyValues::RawRasterUtils::try_raster_cast<
					GPlatesPropertyValues::FloatRawRaster>(*age_grid_age_tile);
	if (float_age_grid_age_tile)
	{
		load_age_grid_into_high_and_low_byte_tile(
				renderer,
				float_age_grid_age_tile.get()->data(),
				age_grid_coverage_tile.get()->data(),
				high_byte_age_texture,
				low_byte_age_texture,
				texel_width,
				texel_height);

		return true;
	}

	boost::optional<GPlatesPropertyValues::DoubleRawRaster::non_null_ptr_type> double_age_grid_age_tile =
			GPlatesPropertyValues::RawRasterUtils::try_raster_cast<
					GPlatesPropertyValues::DoubleRawRaster>(*age_grid_age_tile);
	if (double_age_grid_age_tile)
	{
		load_age_grid_into_high_and_low_byte_tile(
				renderer,
				double_age_grid_age_tile.get()->data(),
				age_grid_coverage_tile.get()->data(),
				high_byte_age_texture,
				low_byte_age_texture,
				texel_width,
				texel_height);

		return true;
	}

	qWarning() << "Age grid raster does not have 'float' or 'double' values.";

	return false;
}


void
GPlatesOpenGL::GLAgeGridMaskSource::render_age_grid_mask(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_type &target_texture,
		const GLTexture::shared_ptr_type &high_byte_age_texture,
		const GLTexture::shared_ptr_type &low_byte_age_texture)
{
	// Simply allocate a new texture from the texture cache and fill it with data.
	// Get an unused tile texture from the cache if there is one.
	boost::optional<GLTexture::shared_ptr_type> intermediate_texture =
			d_intermediate_render_texture_cache->allocate_object();
	if (!intermediate_texture)
	{
		// No unused texture so create a new one...
		intermediate_texture = d_intermediate_render_texture_cache->allocate_object(
				GLTexture::create_as_auto_ptr(renderer));

		// The texture was just allocated so we need to create it in OpenGL.
		create_tile_texture(renderer, intermediate_texture.get());
	}

	// Render the high and low byte textures to the intermediate texture.
	render_age_grid_intermediate_mask(
			renderer, intermediate_texture.get(), high_byte_age_texture, low_byte_age_texture);


	// Begin rendering to a 2D render target texture.
	//
	// Viewport that matches the tile texture size even though some tiles (around boundary of age
	// grid raster) will not use the full tile.
	// The extra texels will be garbage and what we calculate with them will be garbage but those
	// texels won't be accessed when rendering *using* the age grid tile so it's ok.
	GLRenderer::Rgba8RenderTarget2DScope render_target_scope(
			renderer,
			target_texture,
			GLViewport(0, 0, d_tile_texel_dimension, d_tile_texel_dimension));

	// The render target tiling loop...
	do
	{
		// Begin the current render target tile - this also sets the viewport.
		GLTransform::non_null_ptr_to_const_type tile_projection = render_target_scope.begin_tile();

		// Set up the projection transform adjustment for the current render target tile.
		renderer.gl_load_matrix(GL_PROJECTION, tile_projection->get_matrix());

		renderer.gl_clear_color(1, 1, 1, 1);
		// Clear only the colour buffer.
		renderer.gl_clear(GL_COLOR_BUFFER_BIT);

		// Prevent writing to the RGB channels - RGB(1,1,1) is used for the default age grid mask.
		renderer.gl_color_mask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);

		// Bind the low byte age texture to texture unit 0 - the red channel contains coverage.
		renderer.gl_bind_texture(low_byte_age_texture, GL_TEXTURE0, GL_TEXTURE_2D);

		// Enable texturing and on texture unit 0.
		renderer.gl_enable_texture(GL_TEXTURE0, GL_TEXTURE_2D);

		// Use dot3 to convert RGB(1,*,*) to RGBA(*,*,*,1) or RGB(0,*,*) to RGBA(*,*,*,0).
		//
		// NOTE: This only works for extracting a value that's either 0.0 or 1.0 so nearest neighbour
		// filtering with no anisotropic should be used to prevent a value between 0 and 1.
		static const std::vector<GLfloat> dot3_extract_red_channel = create_dot3_extract_red_channel();
		renderer.gl_tex_env(GL_TEXTURE0, GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
		renderer.gl_tex_env(GL_TEXTURE0, GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_DOT3_RGBA_ARB);
		renderer.gl_tex_env(GL_TEXTURE0, GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
		renderer.gl_tex_env(GL_TEXTURE0, GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_CONSTANT_ARB);
		renderer.gl_tex_env(GL_TEXTURE0, GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, dot3_extract_red_channel);
		// The alpha channel is ignored since using GL_DOT3_RGBA_ARB instead of GL_DOT3_RGB_ARB.

		// NOTE: We leave the model-view and projection matrices as identity as that is what we
		// we need to draw a full-screen quad.
		renderer.apply_compiled_draw_state(*d_full_screen_quad_drawable);


		//
		// The initial alpha channel render target value is 1 from the above clear.
		// If an intermediate texture pixel has zero alpha then then zero is written to the render target,
		// otherwise it is left as 1.
		// The intermediate texture is either Ah or Al (the high or low byte of the age-grid age texture)
		// when the age mask should be 1 or 0 when it should be 0.
		// And, as noted in 'render_age_grid_intermediate_mask()', Ah or Al is always greater than zero.
		// So an alpha-test of A == 0 (combined with initial render target value of 1) transforms:
		//
		//   0           ->    0
		//   Ah or Al    ->    1
		//
		// ...so our final age mask values will be 0.0 or 1.0 and nothing in between.
		//

		//
		// Set the state converting the age grid intermediate mask to the full mask.
		//

		// Prevent writing to the Alpha channel (it contains our coverage).
		renderer.gl_color_mask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);

		// Bind the intermediate texture to texture unit 0.
		renderer.gl_bind_texture(intermediate_texture.get(), GL_TEXTURE0, GL_TEXTURE_2D);

		// Enable texturing and set the texture function to replace on texture unit 0.
		renderer.gl_enable_texture(GL_TEXTURE0, GL_TEXTURE_2D);
		renderer.gl_tex_env(GL_TEXTURE0, GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

		// Alpha-test state.
		renderer.gl_enable(GL_ALPHA_TEST);
		renderer.gl_alpha_func(GL_LEQUAL, GLclampf(0));

		// NOTE: We leave the model-view and projection matrices as identity as that is what we
		// we need to draw a full-screen quad.
		renderer.apply_compiled_draw_state(*d_full_screen_quad_drawable);
	}
	while (render_target_scope.end_tile());
}


void
GPlatesOpenGL::GLAgeGridMaskSource::render_age_grid_intermediate_mask(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_type &intermediate_texture,
		const GLTexture::shared_ptr_type &high_byte_age_texture,
		const GLTexture::shared_ptr_type &low_byte_age_texture)
{
	PROFILE_FUNC();

	// Begin rendering to a 2D render target texture.
	GLRenderer::Rgba8RenderTarget2DScope render_target_scope(
			renderer,
			intermediate_texture,
			GLViewport(0, 0, d_tile_texel_dimension, d_tile_texel_dimension));

	// The render target tiling loop...
	do
	{
		// Begin the current render target tile - this also sets the viewport.
		GLTransform::non_null_ptr_to_const_type tile_projection = render_target_scope.begin_tile();

		// Set up the projection transform adjustment for the current render target tile.
		renderer.gl_load_matrix(GL_PROJECTION, tile_projection->get_matrix());

		// Setup for clearing the render target colour buffer.
		// Clear RGB colour to all zeros - this will be used by 'render_age_grid_mask()'.
		// Clear the alpha channel to zero - we'll write a non-zero alpha value where
		// the age-grid age value is greater than the current reconstruction time.
		renderer.gl_clear_color(0, 0, 0, 0);

		// Clear the colour buffer of the render target.
		renderer.gl_clear(GL_COLOR_BUFFER_BIT);

		// Prevent writing to the colour channels - we want to keep RGB(0,0,0) for 'render_age_grid_mask()'.
		renderer.gl_color_mask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);


		//
		// The algorithm for a 16-bit age comparison in terms of an 8-bit comparison is...
		//
		//   Ah * 256 + Al > Th * 256 + Tl
		//
		// ...where Ah and Al are the high and low bytes of the 16-bit age-grid age value and
		// Th and Tl are the high and low bytes of the 16-bit current reconstruction time.
		// This is the same as...
		//
		//   (Ah > Th) || ((Ah == Th) && (Al > Tl))
		//
		// ...which can be implemented as three consecutive alpha-blending / alpha-testing passes...
		//
		//       src_blend  dst_blend  alpha-test
		//   (1)     1          0       Al >  Tl
		//   (2)     0          0       Ah != Th
		//   (3)     1          1       Ah >  Th
		//
		// ...which gives the following results for the alpha channel of the render target...
		//
		//   Ah > Th                           Ah        PASS
		//   Ah < Th                           0         FAIL
		//   (Ah == Th) && (Al > Tl)           Al        PASS
		//   (Ah == Th) && (Al <= Tl)          0         FAIL
		//
		// ...and note that Ah and Al can never be zero in the above because Ah > Th or Al > Tl means
		// that Ah > 0 or Al > 0 (since Th >= 0 or Tl >= 0).
		// Therefore the final alpha channel render target value is always non-zero for PASS and zero for FAIL.
		//


		//
		// Set the state for the first render pass and render.
		//

		// First pass alpha-test state.
		const GLclampf first_pass_alpha_ref = (1.0 / 255) * d_current_reconstruction_time_low_byte;
		renderer.gl_alpha_func(GL_GREATER, first_pass_alpha_ref);

		// Bind the low byte age texture to texture unit 0.
		renderer.gl_bind_texture(low_byte_age_texture, GL_TEXTURE0, GL_TEXTURE_2D);

		renderer.apply_compiled_draw_state(*d_first_render_pass_state);

		//
		// Set the state for the second render pass and render.
		//

		// Second pass alpha-test state.
		const GLclampf second_pass_alpha_ref = (1.0 / 255) * d_current_reconstruction_time_high_byte;
		renderer.gl_alpha_func(GL_NOTEQUAL, second_pass_alpha_ref);

		// Bind the high byte age texture to texture unit 0.
		renderer.gl_bind_texture(high_byte_age_texture, GL_TEXTURE0, GL_TEXTURE_2D);

		renderer.apply_compiled_draw_state(*d_second_render_pass_state);

		//
		// Set the state for the third render pass and render.
		//

		// Third pass alpha-test state.
		const GLclampf third_pass_alpha_ref = (1.0 / 255) * d_current_reconstruction_time_high_byte;
		renderer.gl_alpha_func(GL_GREATER, third_pass_alpha_ref);

		// Bind the high byte age texture to texture unit 0.
		renderer.gl_bind_texture(high_byte_age_texture, GL_TEXTURE0, GL_TEXTURE_2D);

		renderer.apply_compiled_draw_state(*d_third_render_pass_state);
	}
	while (render_target_scope.end_tile());
}


void
GPlatesOpenGL::GLAgeGridMaskSource::initialise_level_of_detail_pyramid()
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
