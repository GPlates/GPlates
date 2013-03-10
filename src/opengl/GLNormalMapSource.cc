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
#include <boost/cast.hpp>
/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>
#include <opengl/OpenGL.h>
#include <QDebug>

#include "GLNormalMapSource.h"

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


namespace GPlatesOpenGL
{
	namespace
	{
		/**
		 * Vertex shader source to generate normals from a height field.
		 */
		const QString GENERATE_NORMAL_MAP_VERTEX_SHADER_SOURCE_FILE_NAME =
				":/opengl/normal_map_source/generate_normal_map_vertex_shader.glsl";

		/**
		 * Fragment shader source to generate normals from a height field.
		 */
		const QString GENERATE_NORMAL_MAP_FRAGMENT_SHADER_SOURCE_FILE_NAME =
				":/opengl/normal_map_source/generate_normal_map_fragment_shader.glsl";
	}
}


bool
GPlatesOpenGL::GLNormalMapSource::is_supported(
		GLRenderer &renderer)
{
	static bool supported = false;

	// Only test for support the first time we're called.
	static bool tested_for_support = false;
	if (!tested_for_support)
	{
		tested_for_support = true;

		// Floating-point textures and non-power-of-two textures are used if available but not required.
		// However vertex/fragment shader programs are required.
		// Actually they're not required in this class since we can generate normals on the CPU
		// but they are required when normals are used for lighting elsewhere.
		if (!renderer.get_capabilities().shader.gl_ARB_vertex_shader ||
			!renderer.get_capabilities().shader.gl_ARB_fragment_shader)
		{
			//qDebug() <<
			//		"GLNormalMapSource: Disabling normal map raster lighting in OpenGL - requires vertex/fragment shader programs.";

			return false;
		}

		// If we get this far then we have support.
		supported = true;
	}

	return supported;
}


boost::optional<GPlatesOpenGL::GLNormalMapSource::non_null_ptr_type>
GPlatesOpenGL::GLNormalMapSource::create(
		GLRenderer &renderer,
		const GPlatesPropertyValues::RawRaster::non_null_ptr_type &height_field_raster,
		unsigned int tile_texel_dimension)
{
	if (!is_supported(renderer))
	{
		return boost::none;
	}

	// The raster type is expected to contain numerical (height) data, not colour RGBA data.
	if (!GPlatesPropertyValues::RawRasterUtils::does_raster_contain_numerical_data(*height_field_raster))
	{
		return boost::none;
	}

	boost::optional<GPlatesPropertyValues::ProxiedRasterResolver::non_null_ptr_type> proxy_resolver_opt =
			GPlatesPropertyValues::ProxiedRasterResolver::create(height_field_raster);
	if (!proxy_resolver_opt)
	{
		return boost::none;
	}

	// Get the raster dimensions.
	boost::optional<std::pair<unsigned int, unsigned int> > raster_dimensions =
			GPlatesPropertyValues::RawRasterUtils::get_raster_size(*height_field_raster);

	// If raster happens to be uninitialised then return false.
	if (!raster_dimensions)
	{
		return boost::none;
	}

	const unsigned int raster_width = raster_dimensions->first;
	const unsigned int raster_height = raster_dimensions->second;

	// Make sure our tile size does not exceed the maximum texture size...
	if (tile_texel_dimension > renderer.get_capabilities().texture.gl_max_texture_size)
	{
		tile_texel_dimension = renderer.get_capabilities().texture.gl_max_texture_size;
	}

	// Make sure tile_texel_dimension is a power-of-two.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			tile_texel_dimension > 0 && GPlatesUtils::Base2::is_power_of_two(tile_texel_dimension),
			GPLATES_ASSERTION_SOURCE);

	// Get the raster statistics (if any).
	GPlatesPropertyValues::RasterStatistics raster_statistics;
	if (GPlatesPropertyValues::RasterStatistics *raster_statistics_ptr =
			GPlatesPropertyValues::RawRasterUtils::get_raster_statistics(*height_field_raster))
	{
		raster_statistics = *raster_statistics_ptr;
	}

	return non_null_ptr_type(new GLNormalMapSource(
			renderer,
			proxy_resolver_opt.get(),
			raster_width,
			raster_height,
			tile_texel_dimension,
			raster_statistics));
}


GPlatesOpenGL::GLNormalMapSource::GLNormalMapSource(
		GLRenderer &renderer,
		const GPlatesGlobal::PointerTraits<GPlatesPropertyValues::ProxiedRasterResolver>::non_null_ptr_type &
				proxy_raster_resolver,
		unsigned int raster_width,
		unsigned int raster_height,
		unsigned int tile_texel_dimension,
		const GPlatesPropertyValues::RasterStatistics &raster_statistics) :
	d_proxied_raster_resolver(proxy_raster_resolver),
	d_raster_width(raster_width),
	d_raster_height(raster_height),
	d_tile_texel_dimension(tile_texel_dimension),
	d_height_field_scale(1), // Default scale
	// Generating normals on GPU requires uploading height field as a floating-point texture with
	// non-power-of-two dimension (tile_texel_dimension + 2) x (tile_texel_dimension + 2) ...
	d_generate_normal_map_on_gpu(
			GLEW_ARB_texture_float &&
			GLEW_ARB_texture_non_power_of_two &&
			GLEW_EXT_framebuffer_object/*needed when rendering to floating-point targets*/),
	// These textures get reused even inside a single rendering frame so we just need a small number
	// to give the graphics card some breathing room (in terms of render-texture dependencies)...
	d_height_field_texture_cache(GPlatesUtils::ObjectCache<GLTexture>::create(2)),
	d_full_screen_quad_drawable(renderer.get_context().get_shared_state()->get_full_screen_2D_textured_quad(renderer)),
	d_logged_tile_load_failure_warning(false)
{
	initialise_level_of_detail_dimensions();

	// Adjust the height field scale based on the raster statistics.
	initialise_height_field_scale(raster_statistics);

	if (d_generate_normal_map_on_gpu)
	{
		// Create the shader program that generates normals from a height field.
		if (!create_normal_map_generation_shader_program(renderer))
		{
			// Failed to create a shader program so resort to generating normals on the CPU.
			d_generate_normal_map_on_gpu = false;
		}
	}

	// Allocate working data for the height data (and normal data for CPU generated normals).
	const unsigned int height_map_texel_dimension = tile_texel_dimension + 2/*border pixels*/;
	if (d_generate_normal_map_on_gpu)
	{
		// If we generate normals on the GPU then the tile height working data will be used to
		// upload to a height map texture...
		const unsigned int num_floats_per_texel = GLEW_ARB_texture_rg ? 2/*RG*/ : 4/*RGBA*/;
		d_tile_height_data_working_space.reset(
				new float[num_floats_per_texel * height_map_texel_dimension * height_map_texel_dimension]);
		// Zero the memory.
		for (unsigned int n = 0; n < num_floats_per_texel * height_map_texel_dimension * height_map_texel_dimension; ++n)
		{
			d_tile_height_data_working_space.get()[n] = 0.0f;
		}
	}
	else // will be generating normals on the CPU...
	{
		d_tile_height_data_working_space.reset(
				new float[2 * height_map_texel_dimension * height_map_texel_dimension]);
		// Zero the memory.
		for (unsigned int n = 0; n < 2 * height_map_texel_dimension * height_map_texel_dimension; ++n)
		{
			d_tile_height_data_working_space.get()[n] = 0.0f;
		}

		// The normal data working space will be used to upload to a normal map texture...
		d_tile_normal_data_working_space.reset(
				new GPlatesGui::rgba8_t[tile_texel_dimension * tile_texel_dimension]);
	}
}


void
GPlatesOpenGL::GLNormalMapSource::initialise_level_of_detail_dimensions()
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
GPlatesOpenGL::GLNormalMapSource::initialise_height_field_scale(
		const GPlatesPropertyValues::RasterStatistics &raster_statistics)
{
	if (!raster_statistics.minimum || !raster_statistics.maximum)
	{
		// Leave height field scale as default value.
		return;
	}

	// Base the scale off the range of raster values.
	// The constant is arbitrary and empirically determined to work with some test rasters.
	// The user can adjust this value so it just needs to be a reasonably OK starting point.
	d_height_field_scale = 100.0 / (raster_statistics.maximum.get() - raster_statistics.minimum.get());
}


void
GPlatesOpenGL::GLNormalMapSource::set_max_highest_resolution_texel_size_on_unit_sphere(
		const double &texel_size)
{
	// The smaller the texel size on the unit-sphere the larger the scale we need to apply to the heights
	// in order to keep the normals (slopes) the same as an equivalent lower (or higher) resolution raster.
	// The constant is arbitrary and empirically determined to work with some test rasters.
	// The user can adjust this value so it just needs to be a reasonably OK starting point.
	d_height_field_scale *= (GPlatesMaths::PI / 1800) / texel_size;
}


bool
GPlatesOpenGL::GLNormalMapSource::change_raster(
		GLRenderer &renderer,
		const GPlatesPropertyValues::RawRaster::non_null_ptr_type &new_height_raster)
{
	// Get the raster dimensions.
	boost::optional<std::pair<unsigned int, unsigned int> > new_raster_dimensions =
			GPlatesPropertyValues::RawRasterUtils::get_raster_size(*new_height_raster);

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
			GPlatesPropertyValues::ProxiedRasterResolver::create(new_height_raster);
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


GPlatesOpenGL::GLNormalMapSource::cache_handle_type
GPlatesOpenGL::GLNormalMapSource::load_tile(
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

	const unsigned int normal_map_texel_x_offset = texel_x_offset;
	const unsigned int normal_map_texel_y_offset = texel_y_offset;
	const unsigned int normal_map_texel_width = texel_width;
	const unsigned int normal_map_texel_height = texel_height;

	const int dst_height_map_texel_x_offset = normal_map_texel_x_offset - 1;
	const int dst_height_map_texel_y_offset = normal_map_texel_y_offset - 1;
	const unsigned int dst_height_map_texel_width = normal_map_texel_width + 2;
	const unsigned int dst_height_map_texel_height = normal_map_texel_height + 2;

	// Expand the tile region by one pixel around its boundary.
	// We need the adjacent height values, at border pixels, in order to calculate normals.
	unsigned int src_height_map_texel_x_offset = normal_map_texel_x_offset;
	unsigned int src_height_map_texel_y_offset = normal_map_texel_y_offset;
	unsigned int src_height_map_texel_width = normal_map_texel_width;
	unsigned int src_height_map_texel_height = normal_map_texel_height;
	// Expand the src normal map to read from proxied raster by one texel around border to
	// get height map except near edges of raster where that's not possible.
	if (normal_map_texel_x_offset > 0)
	{
		--src_height_map_texel_x_offset;
		++src_height_map_texel_width;
	}
	if (normal_map_texel_x_offset + normal_map_texel_width < lod_texel_width)
	{
		++src_height_map_texel_width;
	}
	if (normal_map_texel_y_offset > 0)
	{
		--src_height_map_texel_y_offset;
		++src_height_map_texel_height;
	}
	if (normal_map_texel_y_offset + normal_map_texel_height < lod_texel_height)
	{
		++src_height_map_texel_height;
	}

	PROFILE_BEGIN(profile_proxy_raster_data, "GLNormalMapSource: get_region_from_level");
	// Get the region of the raster covered by this tile at the level-of-detail of this tile.
	boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type> raster_region_opt =
			d_proxied_raster_resolver->get_region_from_level(
					level,
					src_height_map_texel_x_offset,
					src_height_map_texel_y_offset,
					src_height_map_texel_width,
					src_height_map_texel_height);
	PROFILE_END(profile_proxy_raster_data);

	PROFILE_BEGIN(profile_proxy_raster_coverage, "GLNormalMapSource: get_coverage_from_level");
	// Get the region of the raster covered by this tile at the level-of-detail of this tile.
	boost::optional<GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type> raster_coverage_opt =
			d_proxied_raster_resolver->get_coverage_from_level(
					level,
					src_height_map_texel_x_offset,
					src_height_map_texel_y_offset,
					src_height_map_texel_width,
					src_height_map_texel_height);
	PROFILE_END(profile_proxy_raster_coverage);

	// If there was an error accessing raster data or coverage then use default values for the normal map.
	if (!raster_region_opt || !raster_coverage_opt)
	{
		load_default_normal_map(
				level,
				normal_map_texel_x_offset,
				normal_map_texel_y_offset,
				normal_map_texel_width,
				normal_map_texel_height,
				target_texture,
				renderer);

		// Nothing needs caching.
		return cache_handle_type();
	}

	// Pack the raster height/coverage values into the working space.
	if (!pack_height_data_into_tile_working_space(
			raster_region_opt.get(),
			raster_coverage_opt.get(),
			// Offsets of source height data within destination height map...
			src_height_map_texel_x_offset - dst_height_map_texel_x_offset,
			src_height_map_texel_y_offset - dst_height_map_texel_y_offset,
			src_height_map_texel_width,
			src_height_map_texel_height,
			dst_height_map_texel_width,
			dst_height_map_texel_height))
	{
		load_default_normal_map(
				level,
				normal_map_texel_x_offset,
				normal_map_texel_y_offset,
				normal_map_texel_width,
				normal_map_texel_height,
				target_texture,
				renderer);

		// Nothing needs caching.
		return cache_handle_type();
	}

	// The division by 2^level is to adjust for the change in distance between pixels across
	// the different levels-of-detail. Each lower-resolution level (higher 'level' value) needs
	// to have its heights scaled down to compensate (otherwise the change in lighting is visible
	// when transitioning between levels).
	// The division by 6 accounts for the 3 slope calculations per u or v direction and the distance
	// of two pixels covered by each.
	const float lod_height_scale = (1.0f / 6) * d_height_field_scale / (1 << level);

	// If we can offload the normal map generation to the GPU then do so.
	// This really requires floating-point textures to get sufficient precision for the height field values.
	// Fixed-point 8-bit RGB textures are fine for the generated surface normals though.
	if (d_generate_normal_map_on_gpu)
	{
		gpu_convert_height_field_to_normal_map(
				renderer,
				target_texture,
				lod_height_scale,
				normal_map_texel_width,
				normal_map_texel_height);
	}
	else
	{
		cpu_convert_height_field_to_normal_map(
				renderer,
				target_texture,
				lod_height_scale,
				normal_map_texel_width,
				normal_map_texel_height);
	}

	// Nothing needs caching.
	return cache_handle_type();
}


void
GPlatesOpenGL::GLNormalMapSource::gpu_convert_height_field_to_normal_map(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_type &target_texture,
		float lod_height_scale,
		unsigned int normal_map_texel_width,
		unsigned int normal_map_texel_height)
{
	PROFILE_FUNC();

	// Simply allocate a new texture from the texture cache and fill it with height data.
	// Get an unused tile texture from the cache if there is one.
	boost::optional<GLTexture::shared_ptr_type> height_field_texture =
			d_height_field_texture_cache->allocate_object();
	if (!height_field_texture)
	{
		// No unused texture so create a new one...
		height_field_texture = d_height_field_texture_cache->allocate_object(
				GLTexture::create_as_auto_ptr(renderer));

		// The texture was just allocated so we need to create it in OpenGL.
		create_height_tile_texture(renderer, height_field_texture.get());
	}

	// The height map is a non-power-of-two texture (the normal map is power-of-two).
	const unsigned int height_map_texel_width = normal_map_texel_width + 2;
	const unsigned int height_map_texel_height = normal_map_texel_height + 2;

	// Load the height data into the floating-point texture.
	GLTextureUtils::load_image_into_texture_2D(
			renderer,
			height_field_texture.get(),
			d_tile_height_data_working_space.get(),
			(GLEW_ARB_texture_rg ? GL_RG : GL_RGBA),
			GL_FLOAT,
			height_map_texel_width,
			height_map_texel_height);

	// Begin rendering to the 2D render target normal map texture.
	//
	// Specify a viewport that matches the possibly partial tile dimensions and *not* necessarily always
	// the full tile dimensions. This happens for tiles near the bottom or right edge of the raster.
	GLRenderer::RenderTarget2DScope render_target_scope(
			renderer,
			target_texture,
			GLViewport(0, 0, normal_map_texel_width, normal_map_texel_height));

	// The render target tiling loop...
	do
	{
		// Begin the current render target tile - this also sets the viewport.
		GLTransform::non_null_ptr_to_const_type tile_projection = render_target_scope.begin_tile();

		// Set up the projection transform adjustment for the current render target tile.
		renderer.gl_load_matrix(GL_PROJECTION, tile_projection->get_matrix());

		// The default normal is normal to the surface with (x,y,z) of (0,0,1).
		// We also need to convert the x and y components from the signed range [-1,1] to unsigned range [0,1].
		// This is because our normal map texture is unsigned 8-bit RGB.
		// It'll get converted back to the signed range when lighting is applied in a shader program.
		//
		// The default normal is useful because if the region does not occupy the entire tile then it
		// means we've reached the right or bottom edge of the raster and it's possible that our generated
		// normal map could be sampled outside its valid region due to the fact that it's partially filled
		// and contains undefined values outside the region.
		// In this case the default normal will be sampled to give the same lighting results as
		// non-normal-mapped regions of the globe.
		// This also enables us to use discard in the shader program when the coverage is zero in order
		// to use the default normal.
		// NOTE: The clear is not limited to the viewport region (specified above) which is important
		// for the above reason.
		renderer.gl_clear_color(GLclampf(0.5), GLclampf(0.5), GLclampf(1), GLclampf(1));

		// Clear only the colour buffer.
		renderer.gl_clear(GL_COLOR_BUFFER_BIT);

		// Bind the shader program.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				d_generate_normals_program_object,
				GPLATES_ASSERTION_SOURCE);
		renderer.gl_bind_program_object(d_generate_normals_program_object.get());

		// Bind the height field texture to texture unit 0.
		renderer.gl_bind_texture(height_field_texture.get(), GL_TEXTURE0, GL_TEXTURE_2D);

		// Set the height field texture sampler to texture unit 0.
		d_generate_normals_program_object.get()->gl_uniform1i(
				renderer, "height_field_texture_sampler", 0);

		// Set the texture coordinates scale/translate to convert from [0,1] in the possibly partial
		// tile region in the viewport to the full-size square height field tile of dimension
		//    (d_tile_texel_dimension + 2) x (d_tile_texel_dimension + 2).
		const float inverse_full_height_map_tile = 1.0f / (d_tile_texel_dimension + 2);
		const float u_scale = normal_map_texel_width * inverse_full_height_map_tile;
		const float v_scale = normal_map_texel_height * inverse_full_height_map_tile;
		d_generate_normals_program_object.get()->gl_uniform4f(
				renderer,
				"height_field_parameters",
				u_scale, // scale u
				v_scale, // scale v
				inverse_full_height_map_tile, // translate u and v
				lod_height_scale);

		// Draw a full-screen quad.
		renderer.apply_compiled_draw_state(*d_full_screen_quad_drawable);
	}
	while (render_target_scope.end_tile());
}


void
GPlatesOpenGL::GLNormalMapSource::cpu_convert_height_field_to_normal_map(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_type &target_texture,
		float lod_height_scale,
		unsigned int normal_map_texel_width,
		unsigned int normal_map_texel_height)
{
	PROFILE_FUNC();

	float *const height_data_working_space = d_tile_height_data_working_space.get();
	const unsigned int height_map_texel_width = normal_map_texel_width + 2;

	// Each height data texel is a height value followed by a coverage value.
	const unsigned int num_floats_per_height_texel = 2;

	GPlatesGui::rgba8_t *const normal_data_working_space = d_tile_normal_data_working_space.get();

	// The default normal is normal to the surface with (x,y,z) of (0,0,1).
	// We also need to convert the x and y components from the range [-1,1] to [0,255] and
	// the z component from the range [0,1] to [0,255].
	const GPlatesGui::rgba8_t default_normal(128, 128, 255, 255);

	// Generate the normals.
	for (unsigned int y = 0; y < normal_map_texel_height; ++y)
	{
		float *height_data_row_working_space = height_data_working_space +
				num_floats_per_height_texel * ((y + 1) * height_map_texel_width + 1);

		unsigned int normal_map_texel_offset = y * normal_map_texel_width;

		for (unsigned int x = 0;
			x < normal_map_texel_width;
			++x, ++normal_map_texel_offset, height_data_row_working_space += num_floats_per_height_texel)
		{
			// Pixels with zero coverage won't have their height data accessed so there's no
			// need to zero them out (eg, if they are Nan).

			// The centre texel of the 3x3 height texels used to generate the normal for
			// the current normal texel.
			const float *const height_texel11 = height_data_row_working_space;
			const float coverage = height_texel11[1]; // Index 1 is coverage
			if (GPlatesMaths::are_almost_exactly_equal(coverage, 0))
			{
				normal_data_working_space[normal_map_texel_offset] = default_normal;
				continue;
			}

			// All texels in 3x3 height map pixels (except centre pixel).
			const float *const height_texel00 = height_texel11 -
					(height_map_texel_width + 1) * num_floats_per_height_texel;
			const float *const height_texel10 = height_texel11 -
					(height_map_texel_width + 0) * num_floats_per_height_texel;
			const float *const height_texel20 = height_texel11 -
					(height_map_texel_width - 1) * num_floats_per_height_texel;
			const float *const height_texel01 = height_texel11 -
					1 * num_floats_per_height_texel;
			const float *const height_texel21 = height_texel11 +
					1 * num_floats_per_height_texel;
			const float *const height_texel02 = height_texel11 +
					(height_map_texel_width - 1) * num_floats_per_height_texel;
			const float *const height_texel12 = height_texel11 +
					(height_map_texel_width + 0) * num_floats_per_height_texel;
			const float *const height_texel22 = height_texel11 +
					(height_map_texel_width + 1) * num_floats_per_height_texel;

			// Index 1 into texel is coverage.
			const bool have_coverage00 = !GPlatesMaths::are_almost_exactly_equal(height_texel00[1], 0);
			const bool have_coverage10 = !GPlatesMaths::are_almost_exactly_equal(height_texel10[1], 0);
			const bool have_coverage20 = !GPlatesMaths::are_almost_exactly_equal(height_texel20[1], 0);
			const bool have_coverage01 = !GPlatesMaths::are_almost_exactly_equal(height_texel01[1], 0);
			const bool have_coverage21 = !GPlatesMaths::are_almost_exactly_equal(height_texel21[1], 0);
			const bool have_coverage02 = !GPlatesMaths::are_almost_exactly_equal(height_texel02[1], 0);
			const bool have_coverage12 = !GPlatesMaths::are_almost_exactly_equal(height_texel12[1], 0);
			const bool have_coverage22 = !GPlatesMaths::are_almost_exactly_equal(height_texel22[1], 0);

			// Index 0 into texel is height.

			float du = 0;
			if (have_coverage00 && have_coverage20)
			{
				du += height_texel20[0] - height_texel00[0];
			}
			if (have_coverage01 && have_coverage21)
			{
				du += height_texel21[0] - height_texel01[0];
			}
			if (have_coverage02 && have_coverage22)
			{
				du += height_texel22[0] - height_texel02[0];
			}

			float dv = 0;
			if (have_coverage00 && have_coverage02)
			{
				dv += height_texel02[0] - height_texel00[0];
			}
			if (have_coverage10 && have_coverage12)
			{
				dv += height_texel12[0] - height_texel10[0];
			}
			if (have_coverage20 && have_coverage22)
			{
				dv += height_texel22[0] - height_texel20[0];
			}

			du *= lod_height_scale;
			dv *= lod_height_scale;

			const float inv_mag = 1.0f / std::sqrt(1 + du * du + dv * dv);
			const float normal_x = -du * inv_mag;
			const float normal_y = -dv * inv_mag;
			const float normal_z = inv_mag;

			// Store the normal as fixed-point unsigned 8-bit RGB.
			normal_data_working_space[normal_map_texel_offset] =
					GPlatesGui::rgba8_t(
							// Convert x and y components from range [-1,1] to [0,255]...
							static_cast<int>((1.0f + normal_x) * 127.5f),
							static_cast<int>((1.0f + normal_y) * 127.5f),
							// Leave z component as is since it's always positive...
							static_cast<int>(normal_z * 255.0f),
							255);
		}
	}

	// Load the generated normals into the RGBA texture.
	GLTextureUtils::load_image_into_texture_2D(
			renderer,
			target_texture,
			d_tile_normal_data_working_space.get(),
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			normal_map_texel_width,
			normal_map_texel_height);

	// If the region does not occupy the entire tile then it means we've reached the right edge
	// of the raster - we set the last column of texels to the default normal to ensure
	// reasonable values if it happens to get sampled due to numerical precision.
	if (normal_map_texel_width < d_tile_texel_dimension)
	{
		GPlatesGui::rgba8_t *working_space = d_tile_normal_data_working_space.get();
		for (unsigned int y = 0; y < normal_map_texel_height; ++y)
		{
			*working_space++ = default_normal;
		}

		// Load the one-pixel wide column of default normal data into adjacent column.
		GLTextureUtils::load_image_into_texture_2D(
				renderer,
				target_texture,
				d_tile_normal_data_working_space.get(),
				GL_RGBA,
				GL_UNSIGNED_BYTE,
				1/*image_width*/,
				normal_map_texel_height,
				normal_map_texel_width/*texel_u_offset*/);
	}

	// Same applies if we've reached the bottom edge of raster (where the raster height is not an
	// integer multiple of the tile texel dimension).
	if (normal_map_texel_height < d_tile_texel_dimension)
	{
		GPlatesGui::rgba8_t *working_space = d_tile_normal_data_working_space.get();
		for (unsigned int y = 0; y < normal_map_texel_width; ++y)
		{
			*working_space++ = default_normal;
		}

		// Load the one-pixel wide row of default normal data into adjacent row.
		GLTextureUtils::load_image_into_texture_2D(
				renderer,
				target_texture,
				d_tile_normal_data_working_space.get(),
				GL_RGBA,
				GL_UNSIGNED_BYTE,
				normal_map_texel_width,
				1/*image_height*/,
				0/*texel_u_offset*/,
				normal_map_texel_height/*texel_v_offset*/);
	}
}


void
GPlatesOpenGL::GLNormalMapSource::load_default_normal_map(
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
		qWarning() << "Unable to load floating-point height/coverage data into raster tile:";

		qWarning() << "  level, texel_x_offset, texel_y_offset, texel_width, texel_height: "
				<< level << ", "
				<< texel_x_offset << ", "
				<< texel_y_offset << ", "
				<< texel_width << ", "
				<< texel_height << ", ";

		d_logged_tile_load_failure_warning = true;
	}

	// The default normal is normal to the surface with (x,y,z) of (0,0,1).
	// We also need to convert the x and y components from the range [-1,1] to [0,255] and
	// the z component from the range [0,1] to [0,255].
	//
	// This produces the default lighting in the absence of a height field.
	const GPlatesGui::rgba8_t default_normal(128, 128, 255, 255);

	GLTextureUtils::load_colour_into_rgba8_texture_2D(
			renderer, target_texture, default_normal, texel_width, texel_height);
}


template <typename RealType>
void
GPlatesOpenGL::GLNormalMapSource::pack_height_data_into_tile_working_space(
		const RealType *const src_region_data,
		const float *const src_coverage_data,
		unsigned int src_texel_x_offset,
		unsigned int src_texel_y_offset,
		unsigned int src_texel_width,
		unsigned int src_texel_height,
		unsigned int dst_texel_width,
		unsigned int dst_texel_height)
{
	float *const dst_working_space = d_tile_height_data_working_space.get();

	const unsigned int num_floats_per_texel =
			// Generating normals on GPU using RGBA format for height data...
			(d_generate_normal_map_on_gpu && !GLEW_ARB_texture_rg)
			? 4
			// Generating normals on CPU, or generating on GPU using RG format for height data...
			: 2;

	// Copy the source height field into the destination height field.
	// They are the same except the source may be missing boundary height samples.
	for (unsigned int src_y = 0; src_y < src_texel_height; ++src_y)
	{
		const unsigned int dst_y = src_texel_y_offset + src_y;
		const unsigned int dst_x = src_texel_x_offset;
		float *dst_row_working_space = dst_working_space +
				num_floats_per_texel * (dst_y * dst_texel_width + dst_x);

		unsigned int src_texel_offset = src_y * src_texel_width;

		for (unsigned int src_x = 0; src_x < src_texel_width; ++src_x, ++src_texel_offset)
		{
			// Pixels with zero coverage won't have their height data accessed so there's no
			// need to zero them out (eg, if they are Nan).

			dst_row_working_space[0] = static_cast<GLfloat>(src_region_data[src_texel_offset]);
			dst_row_working_space[1] = src_coverage_data[src_texel_offset];
			// There might be entries at index 2 and 3 for RGBA but we leave them as zero.

			dst_row_working_space += num_floats_per_texel;
		}
	}

	// If there's no height data in the bottom edge then set its coverage to zero so it won't be sampled.
	if (src_texel_y_offset > 0)
	{
		float *dst_row_working_space = dst_working_space;

		for (unsigned int dst_x = 0; dst_x < dst_texel_width; ++dst_x)
		{
			dst_row_working_space[0] = 0; // height
			dst_row_working_space[1] = 0; // coverage
			// There might be entries at index 2 and 3 for RGBA but we leave them as zero.

			dst_row_working_space += num_floats_per_texel;
		}
	}

	// If there's no height data in the top edge then set its coverage to zero so it won't be sampled.
	if (src_texel_y_offset + src_texel_height < dst_texel_height)
	{
		float *dst_row_working_space = dst_working_space +
				num_floats_per_texel * (dst_texel_height - 1) * dst_texel_width;

		for (unsigned int dst_x = 0; dst_x < dst_texel_width; ++dst_x)
		{
			dst_row_working_space[0] = 0; // height
			dst_row_working_space[1] = 0; // coverage
			// There might be entries at index 2 and 3 for RGBA but we leave them as zero.

			dst_row_working_space += num_floats_per_texel;
		}
	}

	// If there's no height data in the left edge then set its coverage to zero so it won't be sampled.
	if (src_texel_x_offset > 0)
	{
		float *dst_col_working_space = dst_working_space;

		for (unsigned int dst_y = 0; dst_y < dst_texel_height; ++dst_y)
		{
			dst_col_working_space[0] = 0; // height
			dst_col_working_space[1] = 0; // coverage
			// There might be entries at index 2 and 3 for RGBA but we leave them as zero.

			dst_col_working_space += num_floats_per_texel * dst_texel_width;
		}
	}

	// If there's no height data in the right edge then set its coverage to zero so it won't be sampled.
	if (src_texel_x_offset + src_texel_width < dst_texel_width)
	{
		float *dst_col_working_space = dst_working_space +
				num_floats_per_texel * (dst_texel_width - 1);

		for (unsigned int dst_y = 0; dst_y < dst_texel_height; ++dst_y)
		{
			dst_col_working_space[0] = 0; // height
			dst_col_working_space[1] = 0; // coverage
			// There might be entries at index 2 and 3 for RGBA but we leave them as zero.

			dst_col_working_space += num_floats_per_texel * dst_texel_width;
		}
	}
}


bool
GPlatesOpenGL::GLNormalMapSource::pack_height_data_into_tile_working_space(
		const GPlatesPropertyValues::RawRaster::non_null_ptr_type &src_raster_region,
		const GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type &src_raster_coverage,
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
		pack_height_data_into_tile_working_space(
				float_region_tile.get()->data(),
				src_raster_coverage->data(),
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
		pack_height_data_into_tile_working_space(
				double_region_tile.get()->data(),
				src_raster_coverage->data(),
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
		pack_height_data_into_tile_working_space(
				int8_region_tile.get()->data(),
				src_raster_coverage->data(),
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
		pack_height_data_into_tile_working_space(
				uint8_region_tile.get()->data(),
				src_raster_coverage->data(),
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
		pack_height_data_into_tile_working_space(
				int16_region_tile.get()->data(),
				src_raster_coverage->data(),
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
		pack_height_data_into_tile_working_space(
				uint16_region_tile.get()->data(),
				src_raster_coverage->data(),
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
		pack_height_data_into_tile_working_space(
				int32_region_tile.get()->data(),
				src_raster_coverage->data(),
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
		pack_height_data_into_tile_working_space(
				uint32_region_tile.get()->data(),
				src_raster_coverage->data(),
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


bool
GPlatesOpenGL::GLNormalMapSource::create_normal_map_generation_shader_program(
		GLRenderer &renderer)
{
	GLShaderProgramUtils::ShaderSource generate_normal_map_vertex_shader_source;
	GLShaderProgramUtils::ShaderSource generate_normal_map_fragment_shader_source;

	// Finally add the GLSL 'main()' function.
	generate_normal_map_vertex_shader_source.add_shader_source_from_file(
			GENERATE_NORMAL_MAP_VERTEX_SHADER_SOURCE_FILE_NAME);

	// Finally add the GLSL 'main()' function.
	generate_normal_map_fragment_shader_source.add_shader_source_from_file(
			GENERATE_NORMAL_MAP_FRAGMENT_SHADER_SOURCE_FILE_NAME);

	// Create the shader program for *inactive* polygons.
	d_generate_normals_program_object =
			GLShaderProgramUtils::compile_and_link_vertex_fragment_program(
					renderer,
					generate_normal_map_vertex_shader_source,
					generate_normal_map_fragment_shader_source);

	// Return false if unable to create a shader program.
	if (!d_generate_normals_program_object)
	{
		return false;
	}

	return true;
}


void
GPlatesOpenGL::GLNormalMapSource::create_height_tile_texture(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_type &texture) const
{
	// It's a floating-point texture so use nearest neighbour filtering and no anisotropic.
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

	// We use RG format where possible since it saves memory.
	// NOTE: Otherwise we use RGBA (instead of RGB) because hardware typically uses
	// four channels for RGB formats anyway and uploading to the hardware should be faster
	// since driver doesn't need to be involved (consuming CPU cycles to convert RGB to RGBA).
	const GLint internalformat = GLEW_ARB_texture_rg ? GL_RG32F : GL_RGBA32F_ARB;

	// The height map is a non-power-of-two texture (the normal map is the power-of-two tile dimension).
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			GPLATES_OPENGL_BOOL(GLEW_ARB_texture_non_power_of_two),
			GPLATES_ASSERTION_SOURCE);
	const unsigned int height_map_texel_dimension = d_tile_texel_dimension + 2;

	// Create the texture in OpenGL - this actually creates the texture without any data.
	//
	// NOTE: Since the image data is NULL it doesn't really matter what 'format' (and 'type') are so
	// we just use GL_RGBA (and GL_FLOAT).
	texture->gl_tex_image_2D(renderer, GL_TEXTURE_2D, 0, internalformat,
			height_map_texel_dimension, height_map_texel_dimension,
			0, GL_RGBA, GL_FLOAT, NULL);

	// Check there are no OpenGL errors.
	GLUtils::assert_no_gl_errors(GPLATES_ASSERTION_SOURCE);
}
