/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#include <cmath>
#include <utility>
#include <boost/bind.hpp>
#include <boost/cast.hpp>
/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>
#include <opengl/OpenGL.h>
#include <QDebug>

#include "GLScalarFieldDepthLayersSource.h"

#include "GLContext.h"
#include "GLRenderer.h"
#include "GLShaderProgramUtils.h"
#include "GLTextureUtils.h"
#include "GLUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "maths/MathsUtils.h"

#include "property-values/ProxiedRasterResolver.h"

#include "utils/Base2Utils.h"
#include "utils/Profile.h"

#include <boost/foreach.hpp>

bool
GPlatesOpenGL::GLScalarFieldDepthLayersSource::is_supported(
		GLRenderer &renderer)
{
	static bool supported = false;

	// Only test for support the first time we're called.
	static bool tested_for_support = false;
	if (!tested_for_support)
	{
		tested_for_support = true;

		// Floating-point textures and non-power-of-two textures are required.
		// Vertex/fragment shader programs are required.
		if (!GLContext::get_parameters().texture.gl_ARB_texture_float ||
			!GLContext::get_parameters().texture.gl_ARB_texture_non_power_of_two ||
			!GLContext::get_parameters().shader.gl_ARB_vertex_shader ||
			!GLContext::get_parameters().shader.gl_ARB_fragment_shader)
		{
			return false;
		}

		// If we get this far then we have support.
		supported = true;
	}

	return supported;
}


boost::optional<GPlatesOpenGL::GLScalarFieldDepthLayersSource::non_null_ptr_type>
GPlatesOpenGL::GLScalarFieldDepthLayersSource::create(
		GLRenderer &renderer,
		const depth_layer_seq_type &depth_layers,
		unsigned int tile_texel_dimension)
{
	if (!is_supported(renderer))
	{
		return boost::none;
	}

	if (depth_layers.empty())
	{
		return boost::none;
	}

	boost::optional<std::pair<unsigned int, unsigned int> > raster_dimensions;

	proxied_depth_layer_seq_type proxied_depth_layers;

	// Create a resolver for each proxied raster depth layer.
	BOOST_FOREACH(const DepthLayer &depth_layer, depth_layers)
	{
		// The raster type is expected to contain numerical (height) data, not colour RGBA data.
		if (!GPlatesPropertyValues::RawRasterUtils::does_raster_contain_numerical_data(*depth_layer.depth_layer_raster))
		{
			return boost::none;
		}

		boost::optional<GPlatesPropertyValues::ProxiedRasterResolver::non_null_ptr_type> depth_layer_resolver =
				GPlatesPropertyValues::ProxiedRasterResolver::create(depth_layer.depth_layer_raster);
		if (!depth_layer_resolver)
		{
			return boost::none;
		}

		// Get the raster dimensions.
		boost::optional<std::pair<unsigned int, unsigned int> > depth_layer_dimensions =
				GPlatesPropertyValues::RawRasterUtils::get_raster_size(*depth_layer.depth_layer_raster);

		// If raster happens to be uninitialised then return false.
		if (!depth_layer_dimensions)
		{
			return boost::none;
		}

		// Make sure all depth layers have the same dimensions.
		if (raster_dimensions)
		{
			if (raster_dimensions != depth_layer_dimensions)
			{
				return boost::none;
			}
		}
		else
		{
			raster_dimensions = depth_layer_dimensions;
		}

		proxied_depth_layers.push_back(
				ProxiedDepthLayer(
						depth_layer_resolver.get(),
						depth_layer.depth_radius));
	}

	// Sort the depth layers from low to high radius.
	std::sort(proxied_depth_layers.begin(), proxied_depth_layers.end(),
			boost::bind(&ProxiedDepthLayer::depth_radius, _1) <
			boost::bind(&ProxiedDepthLayer::depth_radius, _2));

	// Make sure our tile size does not exceed the maximum texture size...
	if (tile_texel_dimension > GLContext::get_parameters().texture.gl_max_texture_size)
	{
		tile_texel_dimension = GLContext::get_parameters().texture.gl_max_texture_size;
	}

	// Make sure tile_texel_dimension is a power-of-two.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			tile_texel_dimension > 0 &&
				GPlatesUtils::Base2::is_power_of_two(tile_texel_dimension),
			GPLATES_ASSERTION_SOURCE);

	return non_null_ptr_type(
			new GLScalarFieldDepthLayersSource(
					renderer,
					proxied_depth_layers,
					raster_dimensions->first,
					raster_dimensions->second,
					tile_texel_dimension));
}


GPlatesOpenGL::GLScalarFieldDepthLayersSource::GLScalarFieldDepthLayersSource(
		GLRenderer &renderer,
		const proxied_depth_layer_seq_type &proxied_depth_layers,
		unsigned int raster_width,
		unsigned int raster_height,
		unsigned int tile_texel_dimension) :
	d_proxied_depth_layers(proxied_depth_layers),
	d_raster_width(raster_width),
	d_raster_height(raster_height),
	d_num_depth_layers(proxied_depth_layers.size()),
	d_tile_texel_dimension(tile_texel_dimension),
	d_tile_edge_working_space(new float[4 * tile_texel_dimension]),
	d_logged_tile_load_failure_warning(false),
	d_current_depth_layer_index(0)
{
	initialise_level_of_detail_dimensions();

	// Allocate working space for the scalar data.
	// It has a one pixel wide boundary around the regular tile so can calculate finite differences.
	const unsigned int scalar_data_texel_dimension = tile_texel_dimension + 2/*border pixels*/;
	const unsigned int num_floats_per_scalar_data_texel = 2/*scalar value and coverage*/;
	// Zero the memory.
	for (unsigned int layer = 0; layer < 3; ++layer)
	{
		d_tile_scalar_data_working_space[layer].reset(
				new float[num_floats_per_scalar_data_texel * scalar_data_texel_dimension * scalar_data_texel_dimension]);
		for (unsigned int pixel = 0;
			pixel < num_floats_per_scalar_data_texel * scalar_data_texel_dimension * scalar_data_texel_dimension;
			++pixel)
		{
			d_tile_scalar_data_working_space[layer].get()[pixel] = 0.0f;
		}
	}

	// Allocate working space for the scalar/gradient data.
	const unsigned int num_floats_per_scalar_gradient_data_texel = 4/*scalar value and gradient*/;
	// Zero the memory.
	d_tile_scalar_gradient_data_working_space.reset(
			new float[num_floats_per_scalar_gradient_data_texel * tile_texel_dimension * tile_texel_dimension]);
	for (unsigned int pixel = 0;
		pixel < num_floats_per_scalar_gradient_data_texel * tile_texel_dimension * tile_texel_dimension;
		++pixel)
	{
		d_tile_scalar_gradient_data_working_space.get()[pixel] = 0.0f;
	}
}


void
GPlatesOpenGL::GLScalarFieldDepthLayersSource::initialise_level_of_detail_dimensions()
{
	// The dimension of texels that contribute to a level-of-detail
	// (starting with the highest resolution level-of-detail).
	unsigned int lod_texel_width = d_raster_width;
	unsigned int lod_texel_height = d_raster_height;

	for (unsigned int lod_level = 0; ; ++lod_level)
	{
		d_level_of_detail_dimensions.push_back(std::make_pair(lod_texel_width, lod_texel_height));

		// Continue through the level-of-details until the width and height
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


void
GPlatesOpenGL::GLScalarFieldDepthLayersSource::set_depth_layer(
		GLRenderer &renderer,
		unsigned int depth_layer_index)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			depth_layer_index < d_num_depth_layers,
			GPLATES_ASSERTION_SOURCE);

	if (depth_layer_index == d_current_depth_layer_index)
	{
		return;
	}

	d_current_depth_layer_index = depth_layer_index;

	// Invalidate any raster data that clients may have cached since we are targeting a different depth layer.
	invalidate();
}


GLint
GPlatesOpenGL::GLScalarFieldDepthLayersSource::get_target_texture_internal_format() const
{
	return GL_RGBA32F_ARB;
}


GPlatesOpenGL::GLScalarFieldDepthLayersSource::cache_handle_type
GPlatesOpenGL::GLScalarFieldDepthLayersSource::load_tile(
		unsigned int level,
		unsigned int texel_x_offset,
		unsigned int texel_y_offset,
		unsigned int texel_width,
		unsigned int texel_height,
		const GLTexture::shared_ptr_type &target_texture,
		GLRenderer &renderer)
{
	PROFILE_FUNC();

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			level < d_level_of_detail_dimensions.size(),
			GPLATES_ASSERTION_SOURCE);

	// The dimensions of the current level of detail of the entire raster.
	const unsigned int lod_texel_width = d_level_of_detail_dimensions[level].first; 
	const unsigned int lod_texel_height = d_level_of_detail_dimensions[level].second; 

	const int dst_scalar_map_texel_x_offset = texel_x_offset - 1;
	const int dst_scalar_map_texel_y_offset = texel_y_offset - 1;
	const unsigned int dst_scalar_map_texel_width = texel_width + 2;
	const unsigned int dst_scalar_map_texel_height = texel_height + 2;

	// Expand the tile region by one pixel around its boundary.
	// We need the adjacent height values, at border pixels, in order to calculate finite differences.
	unsigned int src_scalar_map_texel_x_offset = texel_x_offset;
	unsigned int src_scalar_map_texel_y_offset = texel_y_offset;
	unsigned int src_scalar_map_texel_width = texel_width;
	unsigned int src_scalar_map_texel_height = texel_height;
	// Expand the src scalar map to read from proxied raster by one texel around border to
	// get scalar map except near edges of raster where that's not possible.
	if (texel_x_offset > 0)
	{
		--src_scalar_map_texel_x_offset;
		++src_scalar_map_texel_width;
	}
	if (texel_x_offset + texel_width < lod_texel_width)
	{
		++src_scalar_map_texel_width;
	}
	if (texel_y_offset > 0)
	{
		--src_scalar_map_texel_y_offset;
		++src_scalar_map_texel_height;
	}
	if (texel_y_offset + texel_height < lod_texel_height)
	{
		++src_scalar_map_texel_height;
	}

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_current_depth_layer_index < d_num_depth_layers,
			GPLATES_ASSERTION_SOURCE);

	// The central depth layer is always available but its adjacent layers are not always.
	// This happens if the targeted depth layer is either the first or last depth layer.
	bool working_space_layer_loaded[3] = { false, false, false };

	// The depth radius of each depth layer.
	float depth_layer_radius[3] = { 0, 0, 0 };

	// To calculate central differences in the radial (depth) direction we need the current depth layer
	// and its two adjacent depth layers (if available).
	for (int depth_layer_offset = -1; depth_layer_offset <= 1; ++depth_layer_offset)
	{
		// The working space layer index in the range [0,2].
		const unsigned int working_space_layer_index = depth_layer_offset + 1;

		// The depth layer index in the range [0, num_depth_layers - 1].
		int depth_layer_index = d_current_depth_layer_index + depth_layer_offset;

		// If the depth layer index is outside the range of depth layers then we won't load it.
		// This happens if the targeted depth layer is either the first or last depth layer.
		if (depth_layer_index < 0 ||
			depth_layer_index >= boost::numeric_cast<int>(d_num_depth_layers))
		{
			continue;
		}

		const ProxiedDepthLayer &proxied_depth_layer = d_proxied_depth_layers[depth_layer_index];
		GPlatesPropertyValues::ProxiedRasterResolver::non_null_ptr_type proxied_depth_layer_resolver =
				proxied_depth_layer.depth_layer_resolver;

		// Load the region of the depth layer.
		if (!load_depth_layer_into_tile_working_space(
				*proxied_depth_layer_resolver,
				working_space_layer_index,
				level,
				src_scalar_map_texel_x_offset,
				src_scalar_map_texel_y_offset,
				src_scalar_map_texel_width,
				src_scalar_map_texel_height,
				dst_scalar_map_texel_x_offset,
				dst_scalar_map_texel_y_offset,
				dst_scalar_map_texel_width,
				dst_scalar_map_texel_height))
		{
			// If there was an error accessing raster data or coverage then
			// use default values for the scalar and gradient values.
			load_default_scalar_gradient_values(
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

		// Specify the radius of the depth radius.
		depth_layer_radius[working_space_layer_index] = proxied_depth_layer.depth_radius;

		// Mark the working space layer as loaded.
		working_space_layer_loaded[working_space_layer_index] = true;
	}

	generate_scalar_gradient_values(
			renderer,
			target_texture,
			texel_width,
			texel_height,
			depth_layer_radius,
			working_space_layer_loaded);

	// Nothing needs caching.
	return cache_handle_type();
}


void
GPlatesOpenGL::GLScalarFieldDepthLayersSource::generate_scalar_gradient_values(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_type &target_texture,
		unsigned int texel_width,
		unsigned int texel_height,
		float depth_layer_radius[3],
		bool working_space_layer_loaded[3])
{
	PROFILE_FUNC();

	// The targeted depth layer should always be available.
	// Only the lower/upper adjacent layers can be missing if targeted layer is first or last depth layer.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			working_space_layer_loaded[1],
			GPLATES_ASSERTION_SOURCE);

	// The inverse of the radius of the targeted depth layer.
	const double inv_radius = 1.0 / depth_layer_radius[1];

	double inv_delta_radius_lower = 0;
	double inv_delta_radius_upper = 0;
	if (working_space_layer_loaded[0])
	{
		const double delta_radius_lower = depth_layer_radius[1] - depth_layer_radius[0];
		if (!GPlatesMaths::are_almost_exactly_equal(delta_radius_lower, 0))
		{
			inv_delta_radius_lower = 1.0 / delta_radius_lower;
		}
	}
	if (working_space_layer_loaded[2])
	{
		const double delta_radius_upper = depth_layer_radius[2] - depth_layer_radius[1];
		if (!GPlatesMaths::are_almost_exactly_equal(delta_radius_upper, 0))
		{
			inv_delta_radius_upper = 1.0 / delta_radius_upper;
		}
	}

	const float *const lower_depth_scalar_data_working_space = d_tile_scalar_data_working_space[0].get();
	const float *const current_depth_scalar_data_working_space = d_tile_scalar_data_working_space[1].get();
	const float *const upper_depth_scalar_data_working_space = d_tile_scalar_data_working_space[2].get();
	const unsigned int scalar_map_texel_width = texel_width + 2;

	// Each input data texel is a scalar value followed by a coverage value.
	const unsigned int num_floats_per_scalar_data_texel = 2/*scalar value and coverage*/;

	// Working space for the scalar/gradient data.
	const unsigned int num_floats_per_scalar_gradient_data_texel = 4/*scalar value and gradient*/;
	float *const scalar_gradient_data_working_space = d_tile_scalar_gradient_data_working_space.get();

	// Generate the finite differences.
	for (unsigned int y = 0; y < texel_height; ++y)
	{
		const unsigned int scalar_texel_offset =
				num_floats_per_scalar_data_texel * ((y + 1) * scalar_map_texel_width + 1);

		const float *lower_depth_scalar_data_row_working_space =
				lower_depth_scalar_data_working_space + scalar_texel_offset;
		const float *current_depth_scalar_data_row_working_space =
				current_depth_scalar_data_working_space + scalar_texel_offset;
		const float *upper_depth_scalar_data_row_working_space =
				upper_depth_scalar_data_working_space + scalar_texel_offset;

		unsigned int scalar_gradient_texel_offset = y * texel_width;

		for (unsigned int x = 0;
			x < texel_width;
			++x,
			++scalar_gradient_texel_offset,
			lower_depth_scalar_data_row_working_space += num_floats_per_scalar_data_texel,
			current_depth_scalar_data_row_working_space += num_floats_per_scalar_data_texel,
			upper_depth_scalar_data_row_working_space += num_floats_per_scalar_data_texel)
		{
			// Pixels with zero coverage won't have their scalar data accessed so there's no
			// need to zero them out (eg, if they are Nan).

			// The centre texel from which the central differences are calculated relative to.
			const float *const scalar_texel_111 = current_depth_scalar_data_row_working_space;
			const float coverage = scalar_texel_111[1]; // Index 1 is coverage
			if (GPlatesMaths::are_almost_exactly_equal(coverage, 0))
			{
				// Set scalar and gradient to all zeros.
				for (unsigned int component = 0; component < num_floats_per_scalar_gradient_data_texel; ++component)
				{
					scalar_gradient_data_working_space[component] = 0;
				}
				continue;
			}

			// Index 0 into each texel is the scalar value.

			const float scalar = scalar_texel_111[0];

			//
			// Calculate the du and dv finite differences.
			//

			// Four adjacent texels in 3x3 neighbourhood (in current depth layer) of the centre pixel.
			const float *const scalar_texel_101 = scalar_texel_111 -
					(scalar_map_texel_width + 0) * num_floats_per_scalar_data_texel;
			const float *const scalar_texel_011 = scalar_texel_111 -
					1 * num_floats_per_scalar_data_texel;
			const float *const scalar_texel_211 = scalar_texel_111 +
					1 * num_floats_per_scalar_data_texel;
			const float *const scalar_texel_121 = scalar_texel_111 +
					(scalar_map_texel_width + 0) * num_floats_per_scalar_data_texel;

			// Index 1 into each texel is coverage.
			const bool have_coverage_101 = !GPlatesMaths::are_almost_exactly_equal(scalar_texel_101[1], 0);
			const bool have_coverage_011 = !GPlatesMaths::are_almost_exactly_equal(scalar_texel_011[1], 0);
			const bool have_coverage_211 = !GPlatesMaths::are_almost_exactly_equal(scalar_texel_211[1], 0);
			const bool have_coverage_121 = !GPlatesMaths::are_almost_exactly_equal(scalar_texel_121[1], 0);

			double du;
			if (have_coverage_211)
			{
				if (have_coverage_011)
				{
					// Central difference...
					du = 0.5 * (scalar_texel_211[0] - scalar_texel_011[0]);
				}
				else
				{
					// Forward difference...
					du = scalar_texel_211[0] - scalar;
				}
			}
			else if (have_coverage_011)
			{
				// Backward difference...
				du = scalar - scalar_texel_011[0];
			}
			else
			{
				// No difference...
				du = 0;
			}
			// This is a gradient adjustment to account for the fact that GLMultiResolutionRaster
			// (which completes the gradient calculation) assumes a radius of one.
			du *= inv_radius;

			double dv;
			if (have_coverage_121)
			{
				if (have_coverage_101)
				{
					// Central difference...
					dv = 0.5 * (scalar_texel_121[0] - scalar_texel_101[0]);
				}
				else
				{
					// Forward difference...
					dv = scalar_texel_121[0] - scalar;
				}
			}
			else if (have_coverage_101)
			{
				// Backward difference...
				dv = scalar - scalar_texel_101[0];
			}
			else
			{
				// No difference...
				dv = 0;
			}
			// This is a gradient adjustment to account for the fact that GLMultiResolutionRaster
			// (which completes the gradient calculation) assumes a radius of one.
			dv *= inv_radius;

			//
			// Calculate the radial finite difference.
			//
			// NOTE: Unlike the du and dv differences the radial difference includes
			// the radial distance. This makes it a gradient magnitude along the radial direction.
			// The du and dv differences will also become gradient magnitude in their respective
			// directions once they go through GLMultiResolutionRaster (which calculates the
			// distance along a texel in the u and v directions).

			// The centre texels from lower and upper depth layers.
			const float *const scalar_texel_110 = lower_depth_scalar_data_row_working_space;
			const float *const scalar_texel_112 = upper_depth_scalar_data_row_working_space;

			// Index 1 into each texel is coverage.
			const bool have_coverage_110 =
					working_space_layer_loaded[0] &&
						!GPlatesMaths::are_almost_exactly_equal(scalar_texel_110[1], 0);
			const bool have_coverage_112 =
					working_space_layer_loaded[2] &&
						!GPlatesMaths::are_almost_exactly_equal(scalar_texel_112[1], 0);

			double dr;
			if (have_coverage_112)
			{
				if (have_coverage_110)
				{
					// Asymmetric central difference...
					dr = 0.5 * inv_delta_radius_upper * (scalar_texel_112[0] - scalar) +
						0.5 * inv_delta_radius_lower * (scalar - scalar_texel_110[0]);
				}
				else
				{
					// Forward difference...
					dr = inv_delta_radius_upper * (scalar_texel_112[0] - scalar);
				}
			}
			else if (have_coverage_110)
			{
				// Backward difference...
				dr = inv_delta_radius_lower * (scalar - scalar_texel_110[0]);
			}
			else
			{
				// No difference...
				dr = 0;
			}

			//
			// Store the scalar value and the finite differences.
			//

			// The scalar gradient RGBA texel.
			float *scalar_gradient_data_texel = scalar_gradient_data_working_space +
					scalar_gradient_texel_offset * num_floats_per_scalar_gradient_data_texel;

			// Store the scalar value in the red channel.
			scalar_gradient_data_texel[0] = scalar;

			// Store the du finite difference in the green channel.
			scalar_gradient_data_texel[1] = du;

			// Store the dv finite difference in the blue channel.
			scalar_gradient_data_texel[2] = dv;

			// Store the radial finite difference in the alpha channel.
			scalar_gradient_data_texel[3] = dr;
		}
	}

	// Load the finite differences into the RGBA texture.
	GLTextureUtils::load_image_into_texture_2D(
			renderer,
			target_texture,
			d_tile_scalar_gradient_data_working_space.get(),
			GL_RGBA,
			GL_FLOAT,
			texel_width,
			texel_height);

	// If the region does not occupy the entire tile then it means we've reached the right edge
	// of the raster - we duplicate the last column of texels into the adjacent column to ensure
	// that subsequent sampling of the texture at the right edge of the last column of texels
	// will generate the texel colour at the texel centres (for both nearest and bilinear filtering).
	if (texel_width < d_tile_texel_dimension)
	{
		// Copy the right edge of the region into the working space.
		float *working_space = d_tile_edge_working_space.get();
		// The last pixel in the first row of the region.
		const float *region_last_column = d_tile_scalar_gradient_data_working_space.get() +
				num_floats_per_scalar_gradient_data_texel * (texel_width - 1);
		for (unsigned int y = 0; y < texel_height; ++y)
		{
			for (unsigned int component = 0; component < num_floats_per_scalar_gradient_data_texel; ++component)
			{
				working_space[component] = region_last_column[component];
			}
			working_space += num_floats_per_scalar_gradient_data_texel;
			region_last_column += num_floats_per_scalar_gradient_data_texel * texel_width;
		}

		// Load the one-pixel wide column of default normal data into adjacent column.
		GLTextureUtils::load_image_into_texture_2D(
				renderer,
				target_texture,
				d_tile_edge_working_space.get(),
				GL_RGBA,
				GL_FLOAT,
				1/*image_width*/,
				texel_height,
				texel_width/*texel_u_offset*/);
	}

	// Same applies if we've reached the bottom edge of raster (where the raster height is not an
	// integer multiple of the tile texel dimension).
	if (texel_height < d_tile_texel_dimension)
	{
		// Copy the bottom edge of the region into the working space.
		float *working_space = d_tile_edge_working_space.get();
		// The first pixel in the last row of the region.
		const float *region_last_row = d_tile_scalar_gradient_data_working_space.get() +
				num_floats_per_scalar_gradient_data_texel * (texel_height - 1) * texel_width;
		for (unsigned int x = 0; x < texel_width; ++x)
		{
			for (unsigned int component = 0; component < num_floats_per_scalar_gradient_data_texel; ++component)
			{
				working_space[component] = region_last_row[component];
			}
			working_space += num_floats_per_scalar_gradient_data_texel;
			region_last_row += num_floats_per_scalar_gradient_data_texel;
		}

		// Load the one-pixel wide row of default normal data into adjacent row.
		GLTextureUtils::load_image_into_texture_2D(
				renderer,
				target_texture,
				d_tile_edge_working_space.get(),
				GL_RGBA,
				GL_FLOAT,
				texel_width,
				1/*image_height*/,
				0/*texel_u_offset*/,
				texel_height/*texel_v_offset*/);
	}
}


bool
GPlatesOpenGL::GLScalarFieldDepthLayersSource::load_depth_layer_into_tile_working_space(
		GPlatesPropertyValues::ProxiedRasterResolver &proxied_depth_layer_resolver,
		unsigned int working_space_layer_index,
		unsigned int level,
		unsigned int src_scalar_map_texel_x_offset,
		unsigned int src_scalar_map_texel_y_offset,
		unsigned int src_scalar_map_texel_width,
		unsigned int src_scalar_map_texel_height,
		unsigned int dst_scalar_map_texel_x_offset,
		unsigned int dst_scalar_map_texel_y_offset,
		unsigned int dst_scalar_map_texel_width,
		unsigned int dst_scalar_map_texel_height)
{
	PROFILE_BEGIN(profile_proxy_raster_data, "GLScalarFieldDepthLayersSource: get_region_from_level");
	// Get the region of the raster covered by this tile at the level-of-detail of this tile.
	boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type> raster_region_opt =
			proxied_depth_layer_resolver.get_region_from_level(
					level,
					src_scalar_map_texel_x_offset,
					src_scalar_map_texel_y_offset,
					src_scalar_map_texel_width,
					src_scalar_map_texel_height);
	PROFILE_END(profile_proxy_raster_data);

	if (!raster_region_opt)
	{
		return false;
	}

	PROFILE_BEGIN(profile_proxy_raster_coverage, "GLScalarFieldDepthLayersSource: get_coverage_from_level");
	// Get the region of the raster covered by this tile at the level-of-detail of this tile.
	boost::optional<GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type> raster_coverage_opt =
			proxied_depth_layer_resolver.get_coverage_from_level(
					level,
					src_scalar_map_texel_x_offset,
					src_scalar_map_texel_y_offset,
					src_scalar_map_texel_width,
					src_scalar_map_texel_height);
	PROFILE_END(profile_proxy_raster_coverage);

	if (!raster_coverage_opt)
	{
		return false;
	}

	// Pack the scalar/coverage values into the working space.
	if (!pack_scalar_data_into_tile_working_space(
			raster_region_opt.get(),
			raster_coverage_opt.get(),
			working_space_layer_index,
			// Offsets of source scalar data within destination scalar map...
			src_scalar_map_texel_x_offset - dst_scalar_map_texel_x_offset,
			src_scalar_map_texel_y_offset - dst_scalar_map_texel_y_offset,
			src_scalar_map_texel_width,
			src_scalar_map_texel_height,
			dst_scalar_map_texel_width,
			dst_scalar_map_texel_height))
	{
		return false;
	}

	return true;
}


void
GPlatesOpenGL::GLScalarFieldDepthLayersSource::load_default_scalar_gradient_values(
		unsigned int level,
		unsigned int texel_x_offset,
		unsigned int texel_y_offset,
		unsigned int texel_width,
		unsigned int texel_height,
		const GLTexture::shared_ptr_type &target_texture,
		GLRenderer &renderer)
{
	if (!d_logged_tile_load_failure_warning)
	{
		qWarning() << "Unable to load floating-point scalar/coverage data into depth layer tile:";

		qWarning() << "  level, texel_x_offset, texel_y_offset, texel_width, texel_height: "
				<< level << ", "
				<< texel_x_offset << ", "
				<< texel_y_offset << ", "
				<< texel_width << ", "
				<< texel_height << ", ";

		d_logged_tile_load_failure_warning = true;
	}

	// Set the default scalar and gradient (R,GBA) to all zeros.
	const GPlatesGui::Colour default_scalar_gradient(0, 0, 0, 0);

	GLTextureUtils::load_colour_into_rgba32f_texture_2D(
			renderer, target_texture, default_scalar_gradient, texel_width, texel_height);
}


template <typename RealType>
void
GPlatesOpenGL::GLScalarFieldDepthLayersSource::pack_scalar_data_into_tile_working_space(
		const RealType *const src_region_data,
		const float *const src_coverage_data,
		unsigned int working_space_layer_index,
		unsigned int src_texel_x_offset,
		unsigned int src_texel_y_offset,
		unsigned int src_texel_width,
		unsigned int src_texel_height,
		unsigned int dst_texel_width,
		unsigned int dst_texel_height)
{
	float *const dst_working_space = d_tile_scalar_data_working_space[working_space_layer_index].get();

	const unsigned int num_floats_per_scalar_data_texel = 2/*scalar value and coverage*/;

	// Copy the source scalar field into the destination scalar field.
	// They are the same except the source may be missing boundary scalar samples.
	for (unsigned int src_y = 0; src_y < src_texel_height; ++src_y)
	{
		const unsigned int dst_y = src_texel_y_offset + src_y;
		const unsigned int dst_x = src_texel_x_offset;
		float *dst_row_working_space = dst_working_space +
				num_floats_per_scalar_data_texel * (dst_y * dst_texel_width + dst_x);

		unsigned int src_texel_offset = src_y * src_texel_width;

		for (unsigned int src_x = 0; src_x < src_texel_width; ++src_x, ++src_texel_offset)
		{
			// Pixels with zero coverage won't have their scalar data accessed so there's no
			// need to zero them out (eg, if they are Nan).

			dst_row_working_space[0] = static_cast<GLfloat>(src_region_data[src_texel_offset]);
			dst_row_working_space[1] = src_coverage_data[src_texel_offset];

			dst_row_working_space += num_floats_per_scalar_data_texel;
		}
	}

	// If there's no scalar data in the bottom edge then set its coverage to zero so it won't be sampled.
	if (src_texel_y_offset > 0)
	{
		float *dst_row_working_space = dst_working_space;

		for (unsigned int dst_x = 0; dst_x < dst_texel_width; ++dst_x)
		{
			dst_row_working_space[0] = 0; // scalar
			dst_row_working_space[1] = 0; // coverage

			dst_row_working_space += num_floats_per_scalar_data_texel;
		}
	}

	// If there's no scalar data in the top edge then set its coverage to zero so it won't be sampled.
	if (src_texel_y_offset + src_texel_height < dst_texel_height)
	{
		float *dst_row_working_space = dst_working_space +
				num_floats_per_scalar_data_texel * (dst_texel_height - 1) * dst_texel_width;

		for (unsigned int dst_x = 0; dst_x < dst_texel_width; ++dst_x)
		{
			dst_row_working_space[0] = 0; // scalar
			dst_row_working_space[1] = 0; // coverage

			dst_row_working_space += num_floats_per_scalar_data_texel;
		}
	}

	// If there's no scalar data in the left edge then set its coverage to zero so it won't be sampled.
	if (src_texel_x_offset > 0)
	{
		float *dst_col_working_space = dst_working_space;

		for (unsigned int dst_y = 0; dst_y < dst_texel_height; ++dst_y)
		{
			dst_col_working_space[0] = 0; // scalar
			dst_col_working_space[1] = 0; // coverage

			dst_col_working_space += num_floats_per_scalar_data_texel * dst_texel_width;
		}
	}

	// If there's no scalar data in the right edge then set its coverage to zero so it won't be sampled.
	if (src_texel_x_offset + src_texel_width < dst_texel_width)
	{
		float *dst_col_working_space = dst_working_space +
				num_floats_per_scalar_data_texel * (dst_texel_width - 1);

		for (unsigned int dst_y = 0; dst_y < dst_texel_height; ++dst_y)
		{
			dst_col_working_space[0] = 0; // scalar
			dst_col_working_space[1] = 0; // coverage

			dst_col_working_space += num_floats_per_scalar_data_texel * dst_texel_width;
		}
	}
}


bool
GPlatesOpenGL::GLScalarFieldDepthLayersSource::pack_scalar_data_into_tile_working_space(
		const GPlatesPropertyValues::RawRaster::non_null_ptr_type &src_raster_region,
		const GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type &src_raster_coverage,
		unsigned int working_space_layer_index,
		unsigned int src_texel_x_offset,
		unsigned int src_texel_y_offset,
		unsigned int src_texel_width,
		unsigned int src_texel_height,
		unsigned int dst_texel_width,
		unsigned int dst_texel_height)
{
	boost::optional<GPlatesPropertyValues::FloatRawRaster::non_null_ptr_type> float_region_tile =
			GPlatesPropertyValues::RawRasterUtils::try_raster_cast<
					GPlatesPropertyValues::FloatRawRaster>(*src_raster_region);
	if (float_region_tile)
	{
		pack_scalar_data_into_tile_working_space(
				float_region_tile.get()->data(),
				src_raster_coverage->data(),
				working_space_layer_index,
				src_texel_x_offset,
				src_texel_y_offset,
				src_texel_width,
				src_texel_height,
				dst_texel_width,
				dst_texel_height);

		return true;
	}

	boost::optional<GPlatesPropertyValues::DoubleRawRaster::non_null_ptr_type> double_region_tile =
			GPlatesPropertyValues::RawRasterUtils::try_raster_cast<
					GPlatesPropertyValues::DoubleRawRaster>(*src_raster_region);
	if (double_region_tile)
	{
		pack_scalar_data_into_tile_working_space(
				double_region_tile.get()->data(),
				src_raster_coverage->data(),
				working_space_layer_index,
				src_texel_x_offset,
				src_texel_y_offset,
				src_texel_width,
				src_texel_height,
				dst_texel_width,
				dst_texel_height);

		return true;
	}

	boost::optional<GPlatesPropertyValues::Int8RawRaster::non_null_ptr_type> int8_region_tile =
			GPlatesPropertyValues::RawRasterUtils::try_raster_cast<
					GPlatesPropertyValues::Int8RawRaster>(*src_raster_region);
	if (int8_region_tile)
	{
		pack_scalar_data_into_tile_working_space(
				int8_region_tile.get()->data(),
				src_raster_coverage->data(),
				working_space_layer_index,
				src_texel_x_offset,
				src_texel_y_offset,
				src_texel_width,
				src_texel_height,
				dst_texel_width,
				dst_texel_height);

		return true;
	}

	boost::optional<GPlatesPropertyValues::UInt8RawRaster::non_null_ptr_type> uint8_region_tile =
			GPlatesPropertyValues::RawRasterUtils::try_raster_cast<
					GPlatesPropertyValues::UInt8RawRaster>(*src_raster_region);
	if (uint8_region_tile)
	{
		pack_scalar_data_into_tile_working_space(
				uint8_region_tile.get()->data(),
				src_raster_coverage->data(),
				working_space_layer_index,
				src_texel_x_offset,
				src_texel_y_offset,
				src_texel_width,
				src_texel_height,
				dst_texel_width,
				dst_texel_height);

		return true;
	}

	boost::optional<GPlatesPropertyValues::Int16RawRaster::non_null_ptr_type> int16_region_tile =
			GPlatesPropertyValues::RawRasterUtils::try_raster_cast<
					GPlatesPropertyValues::Int16RawRaster>(*src_raster_region);
	if (int16_region_tile)
	{
		pack_scalar_data_into_tile_working_space(
				int16_region_tile.get()->data(),
				src_raster_coverage->data(),
				working_space_layer_index,
				src_texel_x_offset,
				src_texel_y_offset,
				src_texel_width,
				src_texel_height,
				dst_texel_width,
				dst_texel_height);

		return true;
	}

	boost::optional<GPlatesPropertyValues::UInt16RawRaster::non_null_ptr_type> uint16_region_tile =
			GPlatesPropertyValues::RawRasterUtils::try_raster_cast<
					GPlatesPropertyValues::UInt16RawRaster>(*src_raster_region);
	if (uint16_region_tile)
	{
		pack_scalar_data_into_tile_working_space(
				uint16_region_tile.get()->data(),
				src_raster_coverage->data(),
				working_space_layer_index,
				src_texel_x_offset,
				src_texel_y_offset,
				src_texel_width,
				src_texel_height,
				dst_texel_width,
				dst_texel_height);

		return true;
	}

	boost::optional<GPlatesPropertyValues::Int32RawRaster::non_null_ptr_type> int32_region_tile =
			GPlatesPropertyValues::RawRasterUtils::try_raster_cast<
					GPlatesPropertyValues::Int32RawRaster>(*src_raster_region);
	if (int32_region_tile)
	{
		pack_scalar_data_into_tile_working_space(
				int32_region_tile.get()->data(),
				src_raster_coverage->data(),
				working_space_layer_index,
				src_texel_x_offset,
				src_texel_y_offset,
				src_texel_width,
				src_texel_height,
				dst_texel_width,
				dst_texel_height);

		return true;
	}

	boost::optional<GPlatesPropertyValues::UInt32RawRaster::non_null_ptr_type> uint32_region_tile =
			GPlatesPropertyValues::RawRasterUtils::try_raster_cast<
					GPlatesPropertyValues::UInt32RawRaster>(*src_raster_region);
	if (uint32_region_tile)
	{
		pack_scalar_data_into_tile_working_space(
				uint32_region_tile.get()->data(),
				src_raster_coverage->data(),
				working_space_layer_index,
				src_texel_x_offset,
				src_texel_y_offset,
				src_texel_width,
				src_texel_height,
				dst_texel_width,
				dst_texel_height);

		return true;
	}

	return false;
}


GPlatesOpenGL::GLScalarFieldDepthLayersSource::ProxiedDepthLayer::ProxiedDepthLayer(
		const GPlatesPropertyValues::ProxiedRasterResolver::non_null_ptr_type &depth_layer_resolver_,
		double depth_radius_) :
	depth_layer_resolver(depth_layer_resolver_),
	depth_radius(depth_radius_)
{
}
