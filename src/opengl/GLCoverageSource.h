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
 
#ifndef GPLATES_OPENGL_GLCOVERAGESOURCE_H
#define GPLATES_OPENGL_GLCOVERAGESOURCE_H

#include <boost/optional.hpp>
#include <boost/scoped_array.hpp>
	
#include "GLMultiResolutionRasterSource.h"

#include "gui/Colour.h"
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
	 * An arbitrary dimension source of coverage data extracted from a raster.
	 *
	 * NOTE: The inverse of the coverage is returned - this makes it easier to implement
	 * the combining of age masking (for ocean regions) with polygon masking (for continent regions).
	 *
	 * The age grid raster itself is input via a proxied raster.
	 */
	class GLCoverageSource :
			public GLMultiResolutionRasterSource
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLCoverageSource.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLCoverageSource> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLCoverageSource.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLCoverageSource> non_null_ptr_to_const_type;


		/**
		 * Specifies how each coverage pixel is stored in the target texture.
		 *
		 * Includes fixed-point and floating-point textures, which channels contain coverage
		 * and if coverage is inverted (ie, '1 - coverage' is stored instead of 'coverage').
		 */
		enum TextureDataFormat
		{
			//
			// Fixed-point formats...
			//

			// RGBA (8-bit) texture storing (C, C, C, C)...
			TEXTURE_DATA_FORMAT_FIXED_POINT_RGBA8_COVERAGE_RGBA,

			// RGBA (8-bit) texture storing (C, C, C, 1.0)...
			TEXTURE_DATA_FORMAT_FIXED_POINT_RGBA8_COVERAGE_RGB,

			// RGBA (8-bit) texture storing (1.0, 1.0, 1.0, C)...
			TEXTURE_DATA_FORMAT_FIXED_POINT_RGBA8_COVERAGE_A,

			// RGBA (8-bit) texture storing (1.0, 1.0, 1.0, 1.0 - C)...
			TEXTURE_DATA_FORMAT_FIXED_POINT_RGBA8_COVERAGE_INVERT_A,

			//
			// Floating-point formats...
			//
			// For the following floating-point formats only one channel is supplied
			// and inversion, etc not needed since any hardware with floating-point textures
			// will also supports shaders and the coverage can be swizzled, inverted, etc in
			// the shader if it's not in the exact desired format.
			//

			// Red (32-bit floating-point) texture storing coverage...
			//
			// NOTE: Requires GL_ARB_texture_float *and* GL_ARB_texture_rg extensions
			// with GL_ARB_texture_rg being less commonly available.
			//
			// NOTE: The RGBA values are (C, 0.0, 0.0, 1.0).
			TEXTURE_DATA_FORMAT_FLOATING_POINT_R32_COVERAGE,

			// Alpha (32-bit floating-point) texture storing coverage...
			// NOTE: Requires only the GL_ARB_texture_float extension.
			//
			// NOTE: The RGBA values are (0.0, 0.0, 0.0, C).
			TEXTURE_DATA_FORMAT_FLOATING_POINT_A32_COVERAGE
		};


		/**
		 * Creates a @a GLCoverageSource object.
		 *
		 * @a texture_data_format determine how a coverage value is distributed into a pixel's
		 * channels and the storage format.
		 * NOTE: Each *floating-point* format has OpenGL extension requirements.
		 *
		 * @a tile_texel_dimension must be a power-of-two - it is the OpenGL square texture
		 * dimension to use for the tiled textures that represent the multi-resolution raster.
		 *
		 * If @a tile_texel_dimension is greater than the maximum texture size supported
		 * by the run-time system then it will be reduced to the maximum texture size.
		 *
		 * Returns false if @a raster is not a proxy raster or if it's uninitialised.
		 */
		static
		boost::optional<non_null_ptr_type>
		create(
				const GPlatesPropertyValues::RawRaster::non_null_ptr_type &age_grid_raster,
				TextureDataFormat texture_data_format = TEXTURE_DATA_FORMAT_FIXED_POINT_RGBA8_COVERAGE_RGBA,
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
		 * The proxied raster resolver to get coverage floating-point data from the raster.
		 */
		GPlatesGlobal::PointerTraits<GPlatesPropertyValues::ProxiedRasterResolver>::non_null_ptr_type
				d_proxied_raster_resolver;

		//! Original raster width.
		unsigned int d_raster_width;

		//! Original raster height.
		unsigned int d_raster_height;

		//! How a coverage value is distributed into a pixel's channels and the storage format.
		TextureDataFormat d_texture_data_format;

		/**
		 * The number of texels along a tiles edge (horizontal or vertical since it's square).
		 */
		unsigned int d_tile_texel_dimension;

		/**
		 * Uses as temporary space to convert float coverage values to alpha before loading texture.
		 */
		boost::scoped_array<GPlatesGui::rgba8_t> d_coverage_tile_working_space;

		/**
		 * We log a load-tile-failure warning message only once for each coverage source.
		 */
		bool d_logged_tile_load_failure_warning;


		GLCoverageSource(
				const GPlatesGlobal::PointerTraits<GPlatesPropertyValues::ProxiedRasterResolver>::non_null_ptr_type &
						proxy_raster_resolver,
				unsigned int raster_width,
				unsigned int raster_height,
				TextureDataFormat texture_data_format,
				unsigned int tile_texel_dimension);


		//! Returns true if using floating-point textures.
		bool
		using_floating_point_texture() const;
	};
}

#endif // GPLATES_OPENGL_GLCOVERAGESOURCE_H
