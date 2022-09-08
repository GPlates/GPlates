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

#include <algorithm>  // std::fill_n
#include <cmath>
#include <boost/cast.hpp>
#include <boost/scoped_array.hpp>
#include <QDebug>

#include "GLNormalMapSource.h"

#include "GLCapabilities.h"
#include "GLShader.h"
#include "GLShaderSource.h"
#include "GLTextureUtils.h"
#include "GLUtils.h"
#include "OpenGLException.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "maths/MathsUtils.h"

#include "property-values/ProxiedRasterResolver.h"

#include "utils/Base2Utils.h"
#include "utils/CallStackTracker.h"
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


boost::optional<GPlatesOpenGL::GLNormalMapSource::non_null_ptr_type>
GPlatesOpenGL::GLNormalMapSource::create(
		GL &gl,
		const GPlatesPropertyValues::RawRaster::non_null_ptr_type &height_field_raster,
		unsigned int tile_texel_dimension,
		float height_field_scale_factor)
{
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
	if (tile_texel_dimension > gl.get_capabilities().gl_max_texture_size)
	{
		tile_texel_dimension = gl.get_capabilities().gl_max_texture_size;
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
			gl,
			proxy_resolver_opt.get(),
			raster_width,
			raster_height,
			tile_texel_dimension,
			raster_statistics,
			height_field_scale_factor));
}


GPlatesOpenGL::GLNormalMapSource::GLNormalMapSource(
		GL &gl,
		const GPlatesGlobal::PointerTraits<GPlatesPropertyValues::ProxiedRasterResolver>::non_null_ptr_type &
				proxy_raster_resolver,
		unsigned int raster_width,
		unsigned int raster_height,
		unsigned int tile_texel_dimension,
		const GPlatesPropertyValues::RasterStatistics &raster_statistics,
		float height_field_scale_factor) :
	d_proxied_raster_resolver(proxy_raster_resolver),
	d_raster_width(raster_width),
	d_raster_height(raster_height),
	d_tile_texel_dimension(tile_texel_dimension),
	// The constant is arbitrary and empirically determined to work with some test rasters.
	// The user can adjust the final scale value so this just needs to provide a reasonably OK starting point...
	d_constant_height_field_scale_factor(GPlatesMaths::PI / 18),
	d_raster_statistics_height_field_scale_factor(1), // Default scale
	d_raster_resolution_height_field_scale_factor(1), // Default scale
	d_client_height_field_scale_factor(height_field_scale_factor),
	// These height tile textures get reused even inside a single rendering frame so we just need a small number
	// to give the graphics card some breathing room (ie, don't want to upload to same height texture that is
	// currently being used to render to a normal map tile and be forced to wait)...
	d_height_field_texture_cache(GPlatesUtils::ObjectCache<GLTexture>::create(2/*ping-pong between two height textures*/)),
	d_generate_normals_program(GLProgram::create(gl)),
	d_generate_normals_framebuffer(GLFramebuffer::create(gl)),
	d_full_screen_quad(GLUtils::create_full_screen_quad(gl)),
	d_have_checked_framebuffer_completeness_normal_map_generation(false),
	d_logged_tile_load_failure_warning(false)
{
	initialise_level_of_detail_dimensions();

	// Adjust the height field scale factor that's based on the raster statistics.
	initialise_raster_statistics_height_field_scale_factor(raster_statistics);

	// Create the shader program that generates normals from a height field.
	compile_link_normal_map_generation_shader_program(gl);

	// Allocate working data for the height data.
	const unsigned int height_map_texel_dimension = tile_texel_dimension + 2/*border pixels*/;
	// The tile height working data will be used to upload to a height map texture...
	const unsigned int num_floats_per_texel = 2/*RG format*/;
	d_tile_height_data_working_space.reset(
			new float[num_floats_per_texel * height_map_texel_dimension * height_map_texel_dimension]);
	// Zero the memory.
	std::fill_n(
			d_tile_height_data_working_space.get(),
			num_floats_per_texel * height_map_texel_dimension * height_map_texel_dimension,
			0.0f);
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
GPlatesOpenGL::GLNormalMapSource::initialise_raster_statistics_height_field_scale_factor(
		const GPlatesPropertyValues::RasterStatistics &raster_statistics)
{
	if (!raster_statistics.minimum || !raster_statistics.maximum)
	{
		// Leave height field scale as default value.
		d_raster_statistics_height_field_scale_factor = 1;
		return;
	}

	// Base the scale off the range of raster values.
	// The constant is arbitrary and empirically determined to work with some test rasters.
	// The user can adjust this value so it just needs to be a reasonably OK starting point.
	if (!GPlatesMaths::are_almost_exactly_equal(raster_statistics.maximum.get(), raster_statistics.minimum.get()))
	{
		d_raster_statistics_height_field_scale_factor = 1.0 / (raster_statistics.maximum.get() - raster_statistics.minimum.get());
	}
}


float
GPlatesOpenGL::GLNormalMapSource::get_height_field_scale() const
{
	// Multiple all the height field scale factors together.
	return d_constant_height_field_scale_factor *
		d_raster_statistics_height_field_scale_factor *
		d_raster_resolution_height_field_scale_factor *
		d_client_height_field_scale_factor;
}


void
GPlatesOpenGL::GLNormalMapSource::set_max_highest_resolution_texel_size_on_unit_sphere(
		const double &max_highest_resolution_texel_size_on_unit_sphere)
{
	// The smaller the texel size on the unit-sphere the larger the scale we need to apply to the heights
	// in order to keep the normals (slopes) the same as an equivalent lower (or higher) resolution raster.
	d_raster_resolution_height_field_scale_factor =
			1.0 / max_highest_resolution_texel_size_on_unit_sphere;
}


bool
GPlatesOpenGL::GLNormalMapSource::change_raster(
		GL &gl,
		const GPlatesPropertyValues::RawRaster::non_null_ptr_type &new_height_raster,
		float height_field_scale_factor)
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

	// The raster type is expected to contain numerical (height) data, not colour RGBA data.
	if (!GPlatesPropertyValues::RawRasterUtils::does_raster_contain_numerical_data(*new_height_raster))
	{
		return false;
	}

	boost::optional<GPlatesPropertyValues::ProxiedRasterResolver::non_null_ptr_type> proxy_resolver_opt =
			GPlatesPropertyValues::ProxiedRasterResolver::create(new_height_raster);
	if (!proxy_resolver_opt)
	{
		return false;
	}

	d_proxied_raster_resolver = proxy_resolver_opt.get();

	// Get the raster statistics (if any).
	GPlatesPropertyValues::RasterStatistics raster_statistics;
	if (GPlatesPropertyValues::RasterStatistics *raster_statistics_ptr =
			GPlatesPropertyValues::RawRasterUtils::get_raster_statistics(*new_height_raster))
	{
		raster_statistics = *raster_statistics_ptr;
	}

	// Adjust the heightfield scale based on raster statistics.
	initialise_raster_statistics_height_field_scale_factor(raster_statistics);

	d_client_height_field_scale_factor = height_field_scale_factor;

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
		GL &gl)
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

	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GL::StateScope save_restore_state(gl);

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
				gl);

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
				gl);

		// Nothing needs caching.
		return cache_handle_type();
	}

	// The division by 2^level is to adjust for the change in distance between pixels across
	// the different levels-of-detail. Each lower-resolution level (higher 'level' value) needs
	// to have its heights scaled down to compensate (otherwise the change in lighting is visible
	// when transitioning between levels).
	// The division by 6 accounts for the 3 slope calculations per u or v direction and the distance
	// of two pixels covered by each.
	const float lod_height_scale = (1.0f / 6) * get_height_field_scale() / (1 << level);

	// Offload the normal map generation to the GPU.
	// This really requires floating-point textures to get sufficient precision for the height field values.
	// Fixed-point 8-bit RGB textures are fine for the generated surface normals though.
	convert_height_field_to_normal_map(
			gl,
			target_texture,
			lod_height_scale,
			normal_map_texel_width,
			normal_map_texel_height);

	// Nothing needs caching.
	return cache_handle_type();
}


void
GPlatesOpenGL::GLNormalMapSource::convert_height_field_to_normal_map(
		GL &gl,
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
				GLTexture::create_unique(gl));

		// The texture was just allocated so we need to create it in OpenGL.
		create_height_tile_texture(gl, height_field_texture.get());
	}

	// The height map is a non-power-of-two texture (the normal map is power-of-two).
	const unsigned int height_map_texel_width = normal_map_texel_width + 2;
	const unsigned int height_map_texel_height = normal_map_texel_height + 2;

	// Bind height texture before uploading to it.
	gl.BindTexture(GL_TEXTURE_2D, height_field_texture.get());

	// Our client memory image buffers are byte aligned.
	// Not really needed since default alignment is 4 (which is alignment of GL_FLOAT anyway).
	gl.PixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// Load the height data into the floating-point texture.
	gl.TexSubImage2D(GL_TEXTURE_2D, 0/*level*/,
		0/*xoffset*/, 0/*yoffset*/, height_map_texel_width, height_map_texel_height,
		GL_RG, GL_FLOAT, d_tile_height_data_working_space.get());

	// Make sure we leave the OpenGL state the way it was.
	GL::StateScope save_restore_state(
			gl,
			// We're rendering to a render target so reset to the default OpenGL state...
			true/*reset_to_default_state*/);

	// Bind our framebuffer object for generating normals.
	gl.BindFramebuffer(GL_FRAMEBUFFER, d_generate_normals_framebuffer);

	// Begin rendering to the 2D target texture (containing normals).
	gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target_texture, 0/*level*/);

	// Check our framebuffer object for completeness (now that normal texture is attached to it).
	// We only need to do this once because, while the normal map tile texture changes, the
	// framebuffer configuration does not (ie, same texture internal format, dimensions, etc).
	if (!d_have_checked_framebuffer_completeness_normal_map_generation)
	{
		// Throw OpenGLException if not complete.
		// This should succeed since we're using GL_RGBA8 texture format (which is required by OpenGL 3.3 core).
		const GLenum completeness = gl.CheckFramebufferStatus(GL_FRAMEBUFFER);
		GPlatesGlobal::Assert<OpenGLException>(
				completeness == GL_FRAMEBUFFER_COMPLETE,
				GPLATES_ASSERTION_SOURCE,
				"Framebuffer not complete for rendering normals into normal map.");

		d_have_checked_framebuffer_completeness_normal_map_generation = true;
	}

	// Specify a viewport that matches the possibly partial tile dimensions and *not* necessarily always
	// the full tile dimensions. This happens for tiles near the bottom or right edge of the raster.
	gl.Viewport(0, 0, normal_map_texel_width, normal_map_texel_height);

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
	// NOTE: The clear is not limited to the viewport region specified above (since we've not
	//       enabled scissor testing) which is important for the above reason.
	gl.ClearColor(GLclampf(0.5), GLclampf(0.5), GLclampf(1), GLclampf(1));

	// Clear only the colour buffer.
	gl.Clear(GL_COLOR_BUFFER_BIT);

	// Bind the shader program for rendering light direction for the 2D map views.
	gl.UseProgram(d_generate_normals_program);

	// Bind the height field texture to texture unit 0.
	gl.ActiveTexture(GL_TEXTURE0);
	gl.BindTexture(GL_TEXTURE_2D, height_field_texture.get());
	// Set the height field texture sampler to texture unit 0.
	gl.Uniform1i(
			d_generate_normals_program->get_uniform_location(gl, "height_field_texture_sampler"),
			0);

	// The texture coordinates scale/translate the full-screen quad [0,1] (which maps to the
	// possibly partial tile region in the viewport) to the full-size square height field tile of
	// dimension (d_tile_texel_dimension + 2) x (d_tile_texel_dimension + 2).
	const float inverse_full_height_map_tile = 1.0f / (d_tile_texel_dimension + 2);
	const float u_scale = normal_map_texel_width * inverse_full_height_map_tile;
	const float v_scale = normal_map_texel_height * inverse_full_height_map_tile;
	gl.Uniform4f(
			d_generate_normals_program->get_uniform_location(gl, "height_field_parameters"),
			u_scale, // scale u
			v_scale, // scale v
			inverse_full_height_map_tile, // translate u and v
			lod_height_scale);

	// Bind the full screen quad.
	gl.BindVertexArray(d_full_screen_quad);

	// Draw the full screen quad.
	gl.DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}


void
GPlatesOpenGL::GLNormalMapSource::load_default_normal_map(
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
		qWarning() << "Unable to load floating-point height/coverage data into raster tile:";

		qWarning() << "  level, texel_x_offset, texel_y_offset, texel_width, texel_height: "
				<< level << ", "
				<< texel_x_offset << ", "
				<< texel_y_offset << ", "
				<< texel_width << ", "
				<< texel_height << ", ";

		d_logged_tile_load_failure_warning = true;
	}

	// Bind target texture before uploading to it.
	gl.BindTexture(GL_TEXTURE_2D, target_texture);

	// Our client memory image buffers are byte aligned.
	gl.PixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// The default normal is normal to the surface with (x,y,z) of (0,0,1).
	// We also need to convert the x and y components from the range [-1,1] to [0,255] and
	// the z component from the range [0,1] to [0,255].
	//
	// This produces the default lighting in the absence of a height field.
	const GPlatesGui::rgba8_t default_normal(128, 128, 255, 255);

	// Load default normal into target texture.
	boost::scoped_array<GPlatesGui::rgba8_t> default_normal_image_data(
			new GPlatesGui::rgba8_t[texel_width * texel_height]);
	std::fill_n(default_normal_image_data.get(), texel_width * texel_height, default_normal);
	gl.TexSubImage2D(GL_TEXTURE_2D, 0/*level*/,
			0/*xoffset*/, 0/*yoffset*/, texel_width, texel_height,
			GL_RGBA, GL_UNSIGNED_BYTE, default_normal_image_data.get());
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

	const unsigned int num_floats_per_texel = 2/*RG format*/;

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


void
GPlatesOpenGL::GLNormalMapSource::compile_link_normal_map_generation_shader_program(
		GL &gl)
{
	// Add this scope to the call stack trace printed if exception thrown in this scope (eg, failure to compile/link shader).
	TRACK_CALL_STACK();

	// Vertex shader source.
	GLShaderSource vertex_shader_source;
	vertex_shader_source.add_code_segment_from_file(GLShaderSource::UTILS_FILE_NAME);
	vertex_shader_source.add_code_segment_from_file(GENERATE_NORMAL_MAP_VERTEX_SHADER_SOURCE_FILE_NAME);

	// Vertex shader.
	GLShader::shared_ptr_type vertex_shader = GLShader::create(gl, GL_VERTEX_SHADER);
	vertex_shader->shader_source(gl, vertex_shader_source);
	vertex_shader->compile_shader(gl);

	// Fragment shader source.
	GLShaderSource fragment_shader_source;
	fragment_shader_source.add_code_segment_from_file(GLShaderSource::UTILS_FILE_NAME);
	fragment_shader_source.add_code_segment_from_file(GENERATE_NORMAL_MAP_FRAGMENT_SHADER_SOURCE_FILE_NAME);

	// Fragment shader.
	GLShader::shared_ptr_type fragment_shader = GLShader::create(gl, GL_FRAGMENT_SHADER);
	fragment_shader->shader_source(gl, fragment_shader_source);
	fragment_shader->compile_shader(gl);

	// Vertex-fragment program.
	d_generate_normals_program->attach_shader(gl, vertex_shader);
	d_generate_normals_program->attach_shader(gl, fragment_shader);
	d_generate_normals_program->link_program(gl);
}


void
GPlatesOpenGL::GLNormalMapSource::create_height_tile_texture(
		GL &gl,
		const GLTexture::shared_ptr_type &texture) const
{
	gl.BindTexture(GL_TEXTURE_2D, texture);

	// It's a floating-point texture so use nearest neighbour filtering and no anisotropic.
	gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Clamp texture coordinates to centre of edge texels -
	// it's easier for hardware to implement - and doesn't affect our calculations.
	gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// We use RG format (instead of RGBA) since it saves memory.
	const GLint internalformat = GL_RG32F;

	const unsigned int height_map_texel_dimension = d_tile_texel_dimension + 2;

	// Create the texture in OpenGL - this actually creates the texture without any data.
	//
	// NOTE: Since the image data is NULL it doesn't really matter what 'format' (and 'type') are so
	// we just use GL_RGBA (and GL_FLOAT).
	gl.TexImage2D(GL_TEXTURE_2D, 0, internalformat,
			height_map_texel_dimension, height_map_texel_dimension,
			0, GL_RGBA, GL_FLOAT, nullptr);

	// Check there are no OpenGL errors.
	GLUtils::check_gl_errors(gl, GPLATES_ASSERTION_SOURCE);
}
