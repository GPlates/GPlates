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

#ifndef GPLATES_OPENGL_GLNORMALMAPSOURCE_H
#define GPLATES_OPENGL_GLNORMALMAPSOURCE_H

#include <vector>
#include <utility>
#include <boost/optional.hpp>
#include <boost/scoped_array.hpp>
	
#include "GLMultiResolutionRasterSource.h"

#include "GLCompiledDrawState.h"
#include "GLProgramObject.h"

#include "gui/Colour.h"

#include "property-values/RawRaster.h"
#include "property-values/RawRasterUtils.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ObjectCache.h"
#include "utils/ReferenceCount.h"


namespace GPlatesPropertyValues
{
	class ProxiedRasterResolver;
}

namespace GPlatesOpenGL
{
	class GLRenderer;

	/**
	 * A raster source that converts a floating-point raster into a tangent-space normal map for surface lighting.
	 *
	 * The input floating-point raster is treated like a height field but can be any scalar field,
	 * such as gravity, that the user desires to view as if it was a height field.
	 *
	 * The texture format of the normals is 8-bit fixed-point RGBA with the red and green channels
	 * containing the x and y components of the tangent-space surface normal
	 * (converted from [-1.0, 1.0] to [0, 255]) and the blue channel containing the positive z-component
	 * (converted from [0.0, 1.0] to [0, 255]).
	 */
	class GLNormalMapSource :
			public GLMultiResolutionRasterSource
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLNormalMapSource.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLNormalMapSource> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLNormalMapSource.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLNormalMapSource> non_null_ptr_to_const_type;


		/**
		 * Returns true if @a GLNormalMapSource is supported on the runtime system.
		 *
		 * The runtime system requires vertex/fragment shader programs
		 * (GL_ARB_vertex_shader and GL_ARB_fragment_shader).
		 */
		static
		bool
		is_supported(
				GLRenderer &renderer);


		/**
		 * Creates a @a GLNormalMapSource object.
		 *
		 * @a tile_texel_dimension must be a power-of-two - it is the OpenGL square texture
		 * dimension to use for the tiled textures that represent the multi-resolution raster.
		 *
		 * If @a tile_texel_dimension is greater than the maximum texture size supported
		 * by the run-time system then it will be reduced to the maximum texture size.
		 *
		 * @a height_field_scale_factor is an adjustment to the internally determined height field
		 * scale based on the raster statistics (among other things).
		 *
		 * Returns false if @a raster is not a proxy raster or if it's uninitialised or if it doesn't
		 * contain numerical floating-point or integer data (ie, contains colour RGBA pixels) or
		 * if @a is_supported returns false.
		 * NOTE: The raster is expected to be floating-point (or integer), otherwise boost::none is returned.
		 */
		static
		boost::optional<non_null_ptr_type>
		create(
				GLRenderer &renderer,
				const GPlatesPropertyValues::RawRaster::non_null_ptr_type &height_field_raster,
				unsigned int tile_texel_dimension = DEFAULT_TILE_TEXEL_DIMENSION,
				float height_field_scale_factor = 1);


		/**
		 * Change to a new (height) raster of the same dimensions as the current internal raster.
		 *
		 * @a height_field_scale_factor is an adjustment to the internally determined height field
		 * scale based on the raster statistics (among other things).
		 *
		 * This method is useful for time-dependent rasters sharing the same georeferencing
		 * and raster dimensions.
		 *
		 * Returns false if @a raster has different dimensions than the current internal raster.
		 * In this case you'll need to create a new @a GLNormalMapSource.
		 *
		 * NOTE: The opposite, changing the georeferencing without changing the raster,
		 * will require creating a new @a GLMultiResolutionRaster object.
		 */
		bool
		change_raster(
				GLRenderer &renderer,
				const GPlatesPropertyValues::RawRaster::non_null_ptr_type &height_raster,
				float height_field_scale_factor = 1);


		virtual
		unsigned int
		get_raster_width() const
		{
			return d_raster_width;
		}


		virtual
		unsigned int
		get_raster_height() const
		{
			return d_raster_height;
		}


		virtual
		unsigned int
		get_tile_texel_dimension() const
		{
			return d_tile_texel_dimension;
		}


		virtual
		GLint
		get_target_texture_internal_format() const
		{
			// Fixed-point 8-bit texture containing the surface normals in RGB components.
			return GL_RGBA8;
		}


		virtual
		cache_handle_type
		load_tile(
				unsigned int level,
				unsigned int texel_x_offset,
				unsigned int texel_y_offset,
				unsigned int texel_width,
				unsigned int texel_height,
				const GLTexture::shared_ptr_type &target_texture,
				GLRenderer &renderer);


		/**
		 * This is called by @a GLMultiResolutionRaster so that the normals in the highest resolution
		 * normal map can be scaled based on arc distance between two pixels.
		 */
		void
		set_max_highest_resolution_texel_size_on_unit_sphere(
				const double &max_highest_resolution_texel_size_on_unit_sphere);

	private:
		/**
		 * The proxied raster resolver to get floating-point (or integer) data (and coverage) from the raster.
		 */
		GPlatesGlobal::PointerTraits<GPlatesPropertyValues::ProxiedRasterResolver>::non_null_ptr_type
				d_proxied_raster_resolver;

		//! Original raster width.
		unsigned int d_raster_width;

		//! Original raster height.
		unsigned int d_raster_height;

		/**
		 * The number of texels along a tiles edge (horizontal or vertical since it's square).
		 */
		unsigned int d_tile_texel_dimension;

		/**
		 * The empirically determined constant height field scale factor that gives reasonable
		 * results for some test rasters.
		 */
		float d_constant_height_field_scale_factor;

		/**
		 * Height field scale factor based on the heightfield raster statistics (min/max).
		 */
		float d_raster_statistics_height_field_scale_factor;

		/**
		 * Height field scale factor based on the heightfield raster resolution (on the sphere).
		 */
		float d_raster_resolution_height_field_scale_factor;

		/**
		 * Height field scale factor provided by the caller/client.
		 */
		float d_client_height_field_scale_factor;

		/**
		 * If true then normals are generated on the GPU instead of CPU.
		 */
		bool d_generate_normal_map_on_gpu;

		/**
		 * The dimensions of the different levels of detail.
		 */
		std::vector< std::pair<unsigned int, unsigned int> > d_level_of_detail_dimensions;

		/**
		 * Used as temporary space for height data (and coverage).
		 */
		boost::scoped_array<float> d_tile_height_data_working_space;

		/**
		 * Used as temporary space for normal map data.
		 *
		 * NOTE: This is only used if normal maps are generated on the CPU (instead of GPU).
		 */
		boost::scoped_array<GPlatesGui::rgba8_t> d_tile_normal_data_working_space;

		/**
		 * Used to allocate temporary height field textures when generating normals on the GPU.
		 *
		 * NOTE: This is not used when generating normals on the CPU.
		 */
		GPlatesUtils::ObjectCache<GLTexture>::shared_ptr_type d_height_field_texture_cache;

		/**
		 * Shader program to generate normals on the GPU.
		 *
		 * Is boost::none if generating normals on the CPU.
		 */
		boost::optional<GLProgramObject::shared_ptr_type> d_generate_normals_program_object;

		// Used to draw a textured full-screen quad into render texture.
		GLCompiledDrawState::non_null_ptr_to_const_type d_full_screen_quad_drawable;

		/**
		 * We log a load-tile-failure warning message only once for each data raster source.
		 */
		bool d_logged_tile_load_failure_warning;


		GLNormalMapSource(
				GLRenderer &renderer,
				const GPlatesGlobal::PointerTraits<GPlatesPropertyValues::ProxiedRasterResolver>::non_null_ptr_type &
						proxy_raster_resolver,
				unsigned int raster_width,
				unsigned int raster_height,
				unsigned int tile_texel_dimension,
				const GPlatesPropertyValues::RasterStatistics &raster_statistics,
				float height_field_scale_factor);


		void
		initialise_level_of_detail_dimensions();


		void
		initialise_raster_statistics_height_field_scale_factor(
				const GPlatesPropertyValues::RasterStatistics &raster_statistics);


		/**
		 * Returns the height combined field scale combined from all the contributing scale factors.
		 *
		 * Used to vertically exaggerate the height field to make the surface normals more pronounced.
		 */
		float
		get_height_field_scale() const;


		void
		gpu_convert_height_field_to_normal_map(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_type &target_texture,
				float lod_height_scale,
				unsigned int normal_map_texel_width,
				unsigned int normal_map_texel_height);


		void
		cpu_convert_height_field_to_normal_map(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_type &target_texture,
				float lod_height_scale,
				unsigned int normal_map_texel_width,
				unsigned int normal_map_texel_height);


		/**
		 * Emits warning to log and loads the default normal (0,0,1) into target texture.
		 */
		void
		load_default_normal_map(
				unsigned int level,
				unsigned int texel_x_offset,
				unsigned int texel_y_offset,
				unsigned int texel_width,
				unsigned int texel_height,
				const GLTexture::shared_ptr_type &target_texture,
				GLRenderer &renderer);


		/**
		 * Packs raster data/coverage values into target texture.
		 *
		 * Returns false if raw raster is not a floating-point raster (or integer).
		 */
		bool
		pack_height_data_into_tile_working_space(
				const GPlatesPropertyValues::RawRaster::non_null_ptr_type &src_raster_region,
				const GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type &src_raster_coverage,
				unsigned int src_texel_x_offset,
				unsigned int src_texel_y_offset,
				unsigned int src_texel_width,
				unsigned int src_texel_height,
				unsigned int dst_texel_width,
				unsigned int dst_texel_height,
				GLRenderer &renderer);


		/**
		 * Handles packing of data/coverage values where data is either 'float' or 'double'.
		 */
		template <typename RealType>
		void
		pack_height_data_into_tile_working_space(
				const RealType *const src_region_data,
				const float *const src_coverage_data,
				unsigned int src_texel_x_offset,
				unsigned int src_texel_y_offset,
				unsigned int src_texel_width,
				unsigned int src_texel_height,
				unsigned int dst_texel_width,
				unsigned int dst_texel_height,
				GLRenderer &renderer);


		bool
		create_normal_map_generation_shader_program(
				GLRenderer &renderer);


		void
		create_height_tile_texture(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_type &texture) const;
	};
}

#endif // GPLATES_OPENGL_GLNORMALMAPSOURCE_H
