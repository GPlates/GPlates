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

#ifndef GPLATES_OPENGL_GLDATARASTERSOURCE_H
#define GPLATES_OPENGL_GLDATARASTERSOURCE_H

#include <boost/optional.hpp>
#include <boost/scoped_array.hpp>
	
#include "GLMultiResolutionRasterSource.h"

#include "property-values/RawRaster.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesPropertyValues
{
	class ProxiedRasterResolver;
}

namespace GPlatesOpenGL
{
	class GLRenderer;

	/**
	 * An arbitrary dimension source of floating-point data made accessible by a proxied raster.
	 *
	 * However, in contrast to @a GLVisualRasterSource, this raster is meant for data analysis and
	 * *not* for visual display. It expects a raster with a raster band containing floating-point
	 * pixel data. There is no usage of colour palettes or standard colour formats such as JPEG as
	 * those are all for visual display purposes. Note that a floating-point raster can also be used
	 * with @a GLVisualRasterSource but in that case a colour palette is applied to convert each pixel
	 * from a floating-point value to an RGBA8 fixed-point colour. This class does not do that.
	 *
	 * The texture format of the data is 32-bit floating-point with the red channel containing the
	 * raster data value and the green channel containing the raster coverage value (the value that
	 * specifies, at each pixel, how much of that pixel is not the sentinel value in the source raster).
	 *
	 * NOTE: The 'GL_ARB_texture_float' extension is required in which case the texture format is 'GL_RGBA32F_ARB'
	 * (note that RGB could have been used but hardware typically still takes up four channels anyway).
	 * However if the 'GL_ARB_texture_rg' extension is also supported then the 'GL_RG32F' texture
	 * format is used instead to reduce memory usage (by a half).
	 * In either case the red and green channels are used and any extra channels are ignored.
	 */
	class GLDataRasterSource :
			public GLMultiResolutionRasterSource
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLDataRasterSource.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLDataRasterSource> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLDataRasterSource.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLDataRasterSource> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLDataRasterSource object.
		 *
		 * @a tile_texel_dimension must be a power-of-two - it is the OpenGL square texture
		 * dimension to use for the tiled textures that represent the multi-resolution raster.
		 *
		 * If @a tile_texel_dimension is greater than the maximum texture size supported
		 * by the run-time system then it will be reduced to the maximum texture size.
		 *
		 * Returns false if @a raster is not a proxy raster or if it's uninitialised.
		 *
		 * NOTE: The raster is expected to be floating-point - if it's not then @a load_tile
		 * will load zero valued data.
		 */
		static
		boost::optional<non_null_ptr_type>
		create(
				const GPlatesPropertyValues::RawRaster::non_null_ptr_type &data_raster,
				unsigned int tile_texel_dimension = DEFAULT_TILE_TEXEL_DIMENSION);


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
		get_target_texture_internal_format() const;


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

	private:
		/**
		 * The proxied raster resolver to get floating-point data (and coverage) from the raster.
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
		 * Uses as temporary space to pack data and coverage into red/green channels before loading texture.
		 */
		boost::scoped_array<float> d_tile_pack_working_space;

		/**
		 * We log a load-tile-failure warning message only once for each data raster source.
		 */
		bool d_logged_tile_load_failure_warning;


		GLDataRasterSource(
				const GPlatesGlobal::PointerTraits<GPlatesPropertyValues::ProxiedRasterResolver>::non_null_ptr_type &
						proxy_raster_resolver,
				unsigned int raster_width,
				unsigned int raster_height,
				unsigned int tile_texel_dimension);


		/**
		 * Emits warning to log and loads zero data/coverage values into target texture.
		 */
		void
		handle_error_loading_source_raster(
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
		 * Returns false if raw raster is not a floating-point raster.
		 */
		bool
		pack_raster_data_into_tile_working_space(
				const GPlatesPropertyValues::RawRaster::non_null_ptr_type &raster_region,
				const GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type &raster_coverage,
				unsigned int texel_width,
				unsigned int texel_height);


		/**
		 * Handles packing of data/coverage values where data is either 'float' or 'double'.
		 */
		template <typename RealType>
		void
		pack_raster_data_into_tile_working_space(
				const RealType *const region_data,
				const float *const coverage_data,
				unsigned int texel_width,
				unsigned int texel_height);
	};
}

#endif // GPLATES_OPENGL_GLDATARASTERSOURCE_H
