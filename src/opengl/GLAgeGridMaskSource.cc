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

#include "GLAgeGridMaskSource.h"

#include "GLBindTextureState.h"
#include "GLBlendState.h"
#include "GLContext.h"
#include "GLFragmentTestStates.h"
#include "GLRenderer.h"
#include "GLRenderTargetType.h"
#include "GLTextureEnvironmentState.h"
#include "GLTextureUtils.h"
#include "GLUtils.h"
#include "GLVertexArrayDrawable.h"
#include "GLViewport.h"
#include "GLViewportState.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "property-values/ProxiedRasterResolver.h"
#include "property-values/RawRasterUtils.h"

#include "utils/Profile.h"


boost::optional<GPlatesOpenGL::GLAgeGridMaskSource::non_null_ptr_type>
GPlatesOpenGL::GLAgeGridMaskSource::create(
		const double &reconstruction_time,
		const GPlatesPropertyValues::RawRaster::non_null_ptr_type &age_grid_raster,
		const GLTextureResourceManager::shared_ptr_type &texture_resource_manager,
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

	return non_null_ptr_type(new GLAgeGridMaskSource(
			reconstruction_time,
			proxy_resolver_opt.get(),
			raster_width,
			raster_height,
			tile_texel_dimension,
			min_age_in_raster,
			max_age_in_raster,
			texture_resource_manager));
}


GPlatesOpenGL::GLAgeGridMaskSource::GLAgeGridMaskSource(
		const double &reconstruction_time,
		const GPlatesGlobal::PointerTraits<GPlatesPropertyValues::ProxiedRasterResolver>::non_null_ptr_type &
				proxy_raster_resolver,
		unsigned int raster_width,
		unsigned int raster_height,
		unsigned int tile_texel_dimension,
		double min_age_in_raster,
		double max_age_in_raster,
		const GLTextureResourceManager::shared_ptr_type &texture_resource_manager) :
	d_current_reconstruction_time(reconstruction_time),
	d_proxied_raster_resolver(proxy_raster_resolver),
	d_raster_width(raster_width),
	d_raster_height(raster_height),
	d_tile_texel_dimension(tile_texel_dimension),
	d_texture_resource_manager(texture_resource_manager),
	// For the actual age grid values themselves we'll use a bigger cache since these
	// values don't change as the reconstruction time changes (unlike the age grid mask).
	d_age_grid_texture_cache(GPlatesUtils::ObjectCache<GLTexture>::create(200)),
	// These textures don't really need a cache as we don't reuse their contents but it's
	// easy to manage them with a cache.
	d_intermediate_render_texture_cache(GPlatesUtils::ObjectCache<GLTexture>::create()),
	d_clear_buffers_state(GLClearBuffersState::create()),
	d_clear_buffers(GLClearBuffers::create()),
	d_mask_alpha_channel_state(GLMaskBuffersState::create()),
	// Create a state set for the viewport - we'll a viewport that matches the tile texture size
	// even though some tiles (around boundary of age grid raster) will use the full tile.
	// The extra texels will be garbage and what we calculate with them will be garbage but those
	// texels won't be accessed when rendering *using* the age grid tile so it's ok.
	d_viewport(0, 0, tile_texel_dimension, tile_texel_dimension),
	d_viewport_state(GLViewportState::create(d_viewport)),
	d_full_screen_quad_drawable(GLUtils::create_full_screen_2D_textured_quad()),
	d_first_render_pass_state(GLCompositeStateSet::create()),
	d_second_render_pass_state(GLCompositeStateSet::create()),
	d_third_render_pass_state(GLCompositeStateSet::create()),
	d_raster_min_age(min_age_in_raster),
	d_raster_max_age(max_age_in_raster),
	// Factor to convert floating-point age range to 16-bit integer...
	d_raster_inv_age_range_factor(0xffff / (max_age_in_raster - min_age_in_raster)),
	d_age_high_byte_tile_working_space(new GPlatesGui::rgba8_t[tile_texel_dimension * tile_texel_dimension]),
	d_age_low_byte_tile_working_space(new GPlatesGui::rgba8_t[tile_texel_dimension * tile_texel_dimension])
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

	// Setup for clearing the render target colour buffer.
	// Clear colour to all ones and alpha channel to zero.
	d_clear_buffers_state->gl_clear_color(1, 1, 1, 0);
	// Clear only the colour buffer.
	d_clear_buffers->gl_clear(GL_COLOR_BUFFER_BIT);
	// Mask out writing to the colour channels when rendering the age grid mask.
	d_mask_alpha_channel_state->gl_color_mask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);

	//
	// Setup rendering state for the three age grid mask render passes.
	//

	// Enable texturing and set the texture function.
	// It's the same for all three passes.
	GPlatesOpenGL::GLTextureEnvironmentState::non_null_ptr_type tex_env_state =
			GPlatesOpenGL::GLTextureEnvironmentState::create();
	tex_env_state->gl_enable_texture_2D(GL_TRUE);
	tex_env_state->gl_tex_env_mode(GL_REPLACE);

	d_first_render_pass_state->add_state_set(tex_env_state);
	d_second_render_pass_state->add_state_set(tex_env_state);
	d_third_render_pass_state->add_state_set(tex_env_state);

	// First pass alpha-blend state.
	GLBlendState::non_null_ptr_type first_pass_blend_state = GLBlendState::create();
	first_pass_blend_state->gl_enable(GL_TRUE).gl_blend_func(GL_ONE, GL_ZERO);
	d_first_render_pass_state->add_state_set(first_pass_blend_state);

	// Second pass alpha-blend state.
	GLBlendState::non_null_ptr_type second_pass_blend_state = GLBlendState::create();
	second_pass_blend_state->gl_enable(GL_TRUE).gl_blend_func(GL_ZERO, GL_ZERO);
	d_second_render_pass_state->add_state_set(second_pass_blend_state);

	// Third pass alpha-blend state.
	GLBlendState::non_null_ptr_type third_pass_blend_state = GLBlendState::create();
	third_pass_blend_state->gl_enable(GL_TRUE).gl_blend_func(GL_ONE, GL_ONE);
	d_third_render_pass_state->add_state_set(third_pass_blend_state);

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


void
GPlatesOpenGL::GLAgeGridMaskSource::load_tile(
		unsigned int level,
		unsigned int texel_x_offset,
		unsigned int texel_y_offset,
		unsigned int texel_width,
		unsigned int texel_height,
		const GLTexture::shared_ptr_type &target_texture,
		GLRenderer &renderer,
		GLRenderer::RenderTargetUsageType render_target_usage)
{
	// Get the tile corresponding to the request.
	Tile &tile = get_tile(level, texel_x_offset, texel_y_offset);

	// See if either our high or low byte age grid tile textures need reloading
	// from the input age grid raster (eg, if they were recycled by the texture cache).
	GLTexture::shared_ptr_type high_byte_age_texture, low_byte_age_texture;
	if (should_reload_high_and_low_byte_age_textures(
			tile, high_byte_age_texture, low_byte_age_texture))
	{
		PROFILE_BEGIN(proxy_raster, "get_region_from_level");
		// Get the region of the raster covered by this tile at the level-of-detail of this tile.
		boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type> raster_region_opt =
				d_proxied_raster_resolver->get_region_from_level(
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
			qWarning() << "Unable to load age grid data into raster tile:";

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

		// Convert the floating point age values into low and high byte textures so
		// we can do some alpha testing/blending to simulate the per-pixel comparison
		// of the reconstruction time with the age grid values and generate a binary
		// mask in the target texture.
		if (!load_age_grid_into_high_and_low_byte_tile(
				raster_region_opt.get(),
				high_byte_age_texture, low_byte_age_texture,
				texel_width, texel_height))
		{
			// Create a black raster to load into the texture.
			const GPlatesGui::rgba8_t black(0, 0, 0, 0);
			GLTextureUtils::load_colour_into_texture(target_texture, black, texel_width, texel_height);

			return;
		}
	}

	// So now we have up to date high and low byte textures so we can render
	// the age grid mask with them.
	render_age_grid_mask(
			target_texture, high_byte_age_texture, low_byte_age_texture,
			renderer, render_target_usage);
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
					GLTexture::create_as_auto_ptr(d_texture_resource_manager));

			// The texture was just allocated so we need to create it in OpenGL.
			create_tile_texture(high_byte_age_texture);
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
					GLTexture::create_as_auto_ptr(d_texture_resource_manager));

			// The texture was just allocated so we need to create it in OpenGL.
			create_tile_texture(low_byte_age_texture);
		}
	}

	return should_reload;
}


void
GPlatesOpenGL::GLAgeGridMaskSource::create_tile_texture(
		const GLTexture::shared_ptr_type &texture)
{
	//
	// NOTE: Direct calls to OpenGL are made below.
	//
	// Below we make direct calls to OpenGL once we've bound the texture.
	// This is in contrast to storing OpenGL commands in the render graph to be
	// executed later. The reason for this is the calls we make affect the
	// bound texture object's state and not the general OpenGL state. Texture object's
	// are one of those places in OpenGL where you can set state and then subsequent
	// binds of that texture object will set all that state into OpenGL.
	//

	// Bind the texture so its the current texture.
	// Here we actually make a direct OpenGL call to bind the texture to the currently
	// active texture unit. It doesn't matter what the current texture unit is because
	// the only reason we're binding the texture object is so we can set its state =
	// so that subsequent binds of this texture object, when we render the scene graph,
	// will set that state to OpenGL.
	texture->gl_bind_texture(GL_TEXTURE_2D);

	// No mipmaps needed or anisotropic filtering required.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	// Create the texture in OpenGL - this actually creates the texture without any data.
	// We'll be getting our raster source to load image data into the texture.
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
			d_tile_texel_dimension, d_tile_texel_dimension,
			0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	// Check there are no OpenGL errors.
	GLUtils::assert_no_gl_errors(GPLATES_ASSERTION_SOURCE);
}


template <typename RealType>
void
GPlatesOpenGL::GLAgeGridMaskSource::load_age_grid_into_high_and_low_byte_tile(
		const RealType *age_grid_tile,
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
		
		// Convert floating-point age grid value to integer.
		convert_age_to_16_bit_integer(
				age_grid_tile[texel],
				high_byte_tile_working_space[texel].alpha,
				low_byte_tile_working_space[texel].alpha);
	}

	// Load the data into the high and low byte textures.
	GLTextureUtils::load_rgba8_image_into_texture(
			high_byte_age_texture,
			high_byte_tile_working_space,
			texel_width,
			texel_height);
	GLTextureUtils::load_rgba8_image_into_texture(
			low_byte_age_texture,
			low_byte_tile_working_space,
			texel_width,
			texel_height);
}


bool
GPlatesOpenGL::GLAgeGridMaskSource::load_age_grid_into_high_and_low_byte_tile(
		const GPlatesPropertyValues::RawRaster::non_null_ptr_type &age_grid_tile,
		GLTexture::shared_ptr_type &high_byte_age_texture,
		GLTexture::shared_ptr_type &low_byte_age_texture,
		unsigned int texel_width,
		unsigned int texel_height)
{
	PROFILE_FUNC();

	boost::optional<GPlatesPropertyValues::FloatRawRaster::non_null_ptr_type> float_age_grid_tile =
			GPlatesPropertyValues::RawRasterUtils::try_raster_cast<
					GPlatesPropertyValues::FloatRawRaster>(*age_grid_tile);
	if (float_age_grid_tile)
	{
		load_age_grid_into_high_and_low_byte_tile(
				float_age_grid_tile.get()->data(),
				high_byte_age_texture, low_byte_age_texture,
				texel_width, texel_height);

		return true;
	}

	boost::optional<GPlatesPropertyValues::DoubleRawRaster::non_null_ptr_type> double_age_grid_tile =
			GPlatesPropertyValues::RawRasterUtils::try_raster_cast<
					GPlatesPropertyValues::DoubleRawRaster>(*age_grid_tile);
	if (double_age_grid_tile)
	{
		load_age_grid_into_high_and_low_byte_tile(
				double_age_grid_tile.get()->data(),
				high_byte_age_texture, low_byte_age_texture,
				texel_width, texel_height);

		return true;
	}

	qWarning() << "Age grid raster does not have 'float' or 'double' values.";

	return false;
}


void
GPlatesOpenGL::GLAgeGridMaskSource::render_age_grid_mask(
		const GLTexture::shared_ptr_type &target_texture,
		const GLTexture::shared_ptr_type &high_byte_age_texture,
		const GLTexture::shared_ptr_type &low_byte_age_texture,
		GLRenderer &renderer,
		GLRenderer::RenderTargetUsageType render_target_usage)
{
	// Push a render target that will render to the tile texture.
	renderer.push_render_target(
			GLTextureRenderTargetType::create(
					target_texture, d_tile_texel_dimension, d_tile_texel_dimension),
			render_target_usage);

	// Push the viewport state set.
	renderer.push_state_set(d_viewport_state);
	// Let the transform state know of the new viewport.
	renderer.get_transform_state().set_viewport(d_viewport);

	// Clear the colour buffer of the render target.
	GLClearBuffersState::non_null_ptr_type clear_buffers_state = GLClearBuffersState::create();
	clear_buffers_state->gl_clear_color(1, 1, 1, 1);
	renderer.push_state_set(clear_buffers_state);
	renderer.add_drawable(d_clear_buffers);
	renderer.pop_state_set();

	// Mask writing to the alpha channel.
	renderer.push_state_set(d_mask_alpha_channel_state);

	//
	// Set the state converting the age grid intermediate mask to the full mask.
	//

	// Simply allocate a new texture from the texture cache and fill it with data.
	// Get an unused tile texture from the cache if there is one.
	boost::optional< boost::shared_ptr<GLTexture> > intermediate_texture =
			d_intermediate_render_texture_cache->allocate_object();
	if (!intermediate_texture)
	{
		// No unused texture so create a new one...
		intermediate_texture = d_intermediate_render_texture_cache->allocate_object(
				GLTexture::create_as_auto_ptr(d_texture_resource_manager));

		// The texture was just allocated so we need to create it in OpenGL.
		create_tile_texture(intermediate_texture.get());
	}

	// Render the high and low byte textures to the intermediate texture.
	render_age_grid_intermediate_mask(
			intermediate_texture.get(), high_byte_age_texture, low_byte_age_texture, renderer);


	// Create a state set that binds the intermediate texture to texture unit 0.
	GLBindTextureState::non_null_ptr_type bind_age_grid_intermediate_texture = GLBindTextureState::create();
	bind_age_grid_intermediate_texture->gl_bind_texture(GL_TEXTURE_2D, intermediate_texture.get());

	GLCompositeStateSet::non_null_ptr_type state_set = GLCompositeStateSet::create();

	// Enable texturing and set the texture function.
	GPlatesOpenGL::GLTextureEnvironmentState::non_null_ptr_type tex_env_state =
			GPlatesOpenGL::GLTextureEnvironmentState::create();
	tex_env_state->gl_enable_texture_2D(GL_TRUE);
	tex_env_state->gl_tex_env_mode(GL_REPLACE);
	state_set->add_state_set(tex_env_state);

	// Alpha-test state.
	GLAlphaTestState::non_null_ptr_type alpha_test_state = GLAlphaTestState::create();
	alpha_test_state->gl_enable(GL_TRUE).gl_alpha_func(GL_LEQUAL, GLclampf(0));
	state_set->add_state_set(alpha_test_state);

	// NOTE: We leave the model-view and projection matrices as identity as that is what we
	// we need to draw a full-screen quad.
	renderer.push_state_set(state_set);
	renderer.push_state_set(bind_age_grid_intermediate_texture);
	renderer.add_drawable(d_full_screen_quad_drawable);
	renderer.pop_state_set();
	renderer.pop_state_set();

	// Pop the mask buffers state set.
	renderer.pop_state_set();

	// Pop the viewport state set.
	renderer.pop_state_set();

	renderer.pop_render_target();
}


void
GPlatesOpenGL::GLAgeGridMaskSource::render_age_grid_intermediate_mask(
		const GLTexture::shared_ptr_type &intermediate_texture,
		const GLTexture::shared_ptr_type &high_byte_age_texture,
		const GLTexture::shared_ptr_type &low_byte_age_texture,
		GLRenderer &renderer)
{
	PROFILE_FUNC();

	// Push a render target that will render to the tile texture.
	// We can render to the target in parallel because we're caching the intermediate texture.
	renderer.push_render_target(
			GLTextureRenderTargetType::create(
					intermediate_texture, d_tile_texel_dimension, d_tile_texel_dimension),
			GLRenderer::RENDER_TARGET_USAGE_PARALLEL);

	// Push the viewport state set.
	renderer.push_state_set(d_viewport_state);
	// Let the transform state know of the new viewport.
	renderer.get_transform_state().set_viewport(d_viewport);

	// Clear the colour buffer of the render target.
	renderer.push_state_set(d_clear_buffers_state);
	renderer.add_drawable(d_clear_buffers);
	renderer.pop_state_set();

	// Mask writing to the alpha channel.
	renderer.push_state_set(d_mask_alpha_channel_state);


	// Create a state set that binds the low byte age texture to texture unit 0.
	GLBindTextureState::non_null_ptr_type bind_low_byte_age_texture = GLBindTextureState::create();
	bind_low_byte_age_texture->gl_bind_texture(GL_TEXTURE_2D, low_byte_age_texture);

	// Create a state set that binds the high byte age texture to texture unit 0.
	GLBindTextureState::non_null_ptr_type bind_high_byte_age_texture = GLBindTextureState::create();
	bind_high_byte_age_texture->gl_bind_texture(GL_TEXTURE_2D, high_byte_age_texture);


	//
	// Set the state for the first render pass and render.
	//

	// First pass alpha-test state.
	GLAlphaTestState::non_null_ptr_type first_pass_alpha_test_state = GLAlphaTestState::create();
	first_pass_alpha_test_state->gl_enable(GL_TRUE);
	const GLclampf first_pass_alpha_ref = (1.0 / 255) * d_current_reconstruction_time_low_byte;
	first_pass_alpha_test_state->gl_alpha_func(GL_GREATER, first_pass_alpha_ref);
	renderer.push_state_set(first_pass_alpha_test_state);

	// NOTE: We leave the model-view and projection matrices as identity as that is what we
	// we need to draw a full-screen quad.
	renderer.push_state_set(d_first_render_pass_state);
	renderer.push_state_set(bind_low_byte_age_texture);
	renderer.add_drawable(d_full_screen_quad_drawable);
	renderer.pop_state_set();
	renderer.pop_state_set();

	renderer.pop_state_set();

	//
	// Set the state for the second render pass and render.
	//

	// Second pass alpha-test state.
	GLAlphaTestState::non_null_ptr_type second_pass_alpha_test_state = GLAlphaTestState::create();
	second_pass_alpha_test_state->gl_enable(GL_TRUE);
	const GLclampf second_pass_alpha_ref = (1.0 / 255) * d_current_reconstruction_time_high_byte;
	second_pass_alpha_test_state->gl_alpha_func(GL_NOTEQUAL, second_pass_alpha_ref);
	renderer.push_state_set(second_pass_alpha_test_state);

	// NOTE: We leave the model-view and projection matrices as identity as that is what we
	// we need to draw a full-screen quad.
	renderer.push_state_set(d_second_render_pass_state);
	renderer.push_state_set(bind_high_byte_age_texture);
	renderer.add_drawable(d_full_screen_quad_drawable);
	renderer.pop_state_set();
	renderer.pop_state_set();

	renderer.pop_state_set();

	//
	// Set the state for the third render pass and render.
	//

	// Third pass alpha-test state.
	GLAlphaTestState::non_null_ptr_type third_pass_alpha_test_state = GLAlphaTestState::create();
	third_pass_alpha_test_state->gl_enable(GL_TRUE);
	const GLclampf third_pass_alpha_ref = (1.0 / 255) * d_current_reconstruction_time_high_byte;
	third_pass_alpha_test_state->gl_alpha_func(GL_GREATER, third_pass_alpha_ref);
	renderer.push_state_set(third_pass_alpha_test_state);

	// NOTE: We leave the model-view and projection matrices as identity as that is what we
	// we need to draw a full-screen quad.
	renderer.push_state_set(d_third_render_pass_state);
	renderer.push_state_set(bind_high_byte_age_texture);
	renderer.add_drawable(d_full_screen_quad_drawable);
	renderer.pop_state_set();
	renderer.pop_state_set();

	renderer.pop_state_set();


	// Pop the mask buffers state set.
	renderer.pop_state_set();

	// Pop the viewport state set.
	renderer.pop_state_set();

	renderer.pop_render_target();
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
