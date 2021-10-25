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

#ifndef GPLATES_OPENGL_GLSCALARFIELDDEPTHLAYERSSOURCE_H
#define GPLATES_OPENGL_GLSCALARFIELDDEPTHLAYERSSOURCE_H

#include <vector>
#include <utility>
#include <boost/optional.hpp>
#include <boost/scoped_array.hpp>
	
#include "GLMultiResolutionRasterSource.h"

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
	 * A raster source that contains depth layers for generating the scalar values and gradients
	 * for a 3D scalar field.
	 *
	 * The floating-point RGBA output matches the format of GPlatesFileIO::ScalarField3DFileFormat::FieldDataSample.
	 * With red channel containing the scalar value and the GBA channels containing the field gradient.
	 * 
	 *
	 * NOTE: The 'GL_ARB_texture_float' extension is required (along with GL_ARB_vertex_shader and
	 * GL_ARB_fragment_shader) in which case the texture format is 'GL_RGBA32F_ARB'.
	 */
	class GLScalarFieldDepthLayersSource :
			public GLMultiResolutionRasterSource
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLScalarFieldDepthLayersSource.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLScalarFieldDepthLayersSource> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLScalarFieldDepthLayersSource.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLScalarFieldDepthLayersSource> non_null_ptr_to_const_type;


		/**
		 * A single depth layer contributing to the 3D scalar field.
		 */
		struct DepthLayer
		{
			// Note that @a depth_radius_ is normalised [0,1] sphere radius.
			DepthLayer(
					const GPlatesPropertyValues::RawRaster::non_null_ptr_type &depth_layer_raster_,
					double depth_radius_) :
				depth_layer_raster(depth_layer_raster_),
				depth_radius(depth_radius_)
			{  }

			GPlatesPropertyValues::RawRaster::non_null_ptr_type depth_layer_raster;
			float depth_radius;
		};

		//! Typedef for a sequence of depth layers.
		typedef std::vector<DepthLayer> depth_layer_seq_type;



		/**
		 * Returns true if @a GLScalarFieldDepthLayersSource is supported on the runtime system.
		 *
		 * The runtime system requires the OpenGL extension 'GL_ARB_texture_float' and
		 * vertex/fragment shader programs (GL_ARB_vertex_shader and GL_ARB_fragment_shader).
		 */
		static
		bool
		is_supported(
				GLRenderer &renderer);


		/**
		 * Creates a @a GLScalarFieldDepthLayersSource object from the specified depth layer rasters.
		 *
		 * @a tile_texel_dimension must be a power-of-two - it is the OpenGL square texture
		 * dimension to use for the tiled textures that represent the multi-resolution raster.
		 *
		 * If @a tile_texel_dimension is greater than the maximum texture size supported
		 * by the run-time system then it will be reduced to the maximum texture size.
		 *
		 * Returns false if any depth layer raster in the sequence:
		 *  - is not a proxy raster, or
		 *  - is uninitialised, or
		 *  - do not contain numerical floating-point or integer data (ie, contains colour RGBA pixels),
		 * ...or not all rasters have same dimensions, or
		 * if @a is_supported returns false.
		 * NOTE: The raster is expected to be floating-point (or integer), otherwise boost::none is returned.
		 *
		 * NOTE: The depth layers do not need to be sorted by depth - that will be handled by this function.
		 */
		static
		boost::optional<non_null_ptr_type>
		create(
				GLRenderer &renderer,
				const depth_layer_seq_type &depth_layers,
				unsigned int tile_texel_dimension = DEFAULT_TILE_TEXEL_DIMENSION);


		/**
		 * Sets the current depth layer that the output scalar values and gradients are generated from.
		 *
		 * @a depth_layer_index is the index into the depth layers passed into @a create.
		 */
		void
		set_depth_layer(
				GLRenderer &renderer,
				unsigned int depth_layer_index);


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
		 * A single depth layer with a proxied raw raster resolver to access the scalar field values.
		 */
		struct ProxiedDepthLayer
		{
			ProxiedDepthLayer(
					const GPlatesGlobal::PointerTraits<GPlatesPropertyValues::ProxiedRasterResolver>::non_null_ptr_type &depth_layer_resolver_,
					double depth_radius_);

			GPlatesGlobal::PointerTraits<GPlatesPropertyValues::ProxiedRasterResolver>::non_null_ptr_type depth_layer_resolver;
			float depth_radius;
		};

		//! Typedef for a sequence of proxied depth layer raster resolvers.
		typedef std::vector<ProxiedDepthLayer> proxied_depth_layer_seq_type;


		/**
		 * The proxied raster resolvers to get floating-point (or integer) data (and coverage) from the depth layers.
		 */
		proxied_depth_layer_seq_type d_proxied_depth_layers;

		//! Raster width.
		unsigned int d_raster_width;

		//! Raster height.
		unsigned int d_raster_height;

		//! Number of depth layers.
		unsigned int d_num_depth_layers;

		/**
		 * The number of texels along a tiles edge (horizontal or vertical since it's square).
		 */
		unsigned int d_tile_texel_dimension;

		/**
		 * The dimensions of the different levels of detail.
		 */
		std::vector< std::pair<unsigned int, unsigned int> > d_level_of_detail_dimensions;

		/**
		 * Used as temporary space for scalar data (and coverage).
		 *
		 * There are three arrays, one for the targeted depth layer and one for each adjacent depth layer.
		 */
		boost::scoped_array<float> d_tile_scalar_data_working_space[3];

		/**
		 * Used as temporary space for scalar+gradient data.
		 */
		boost::scoped_array<float> d_tile_scalar_gradient_data_working_space;

		/**
		 * Used as temporary space to duplicate a tile's vertical or horizontal edge when the data in
		 * the tile does not consume the full @a d_tile_texel_dimension x @a d_tile_texel_dimension area.
		 */
		boost::scoped_array<float> d_tile_edge_working_space;

		/**
		 * We log a load-tile-failure warning message only once for each data raster source.
		 */
		bool d_logged_tile_load_failure_warning;

		/**
		 * The current depth layer we are using as a source.
		 */
		unsigned int d_current_depth_layer_index;


		GLScalarFieldDepthLayersSource(
				GLRenderer &renderer,
				const proxied_depth_layer_seq_type &proxied_depth_layers,
				unsigned int raster_width,
				unsigned int raster_height,
				unsigned int tile_texel_dimension);


		void
		initialise_level_of_detail_dimensions();

		void
		generate_scalar_gradient_values(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_type &target_texture,
				unsigned int texel_width,
				unsigned int texel_height,
				float depth_layer_radius[3],
				bool working_space_layer_loaded[3]);

		bool
		load_depth_layer_into_tile_working_space(
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
				unsigned int dst_scalar_map_texel_height);

		void
		load_default_scalar_gradient_values(
				unsigned int level,
				unsigned int texel_x_offset,
				unsigned int texel_y_offset,
				unsigned int texel_width,
				unsigned int texel_height,
				const GLTexture::shared_ptr_type &target_texture,
				GLRenderer &renderer);

		template <typename RealType>
		void
		pack_scalar_data_into_tile_working_space(
				const RealType *const src_region_data,
				const float *const src_coverage_data,
				unsigned int working_space_layer_index,
				unsigned int src_texel_x_offset,
				unsigned int src_texel_y_offset,
				unsigned int src_texel_width,
				unsigned int src_texel_height,
				unsigned int dst_texel_width,
				unsigned int dst_texel_height);

		bool
		pack_scalar_data_into_tile_working_space(
				const GPlatesPropertyValues::RawRaster::non_null_ptr_type &src_raster_region,
				const GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type &src_raster_coverage,
				unsigned int working_space_layer_index,
				unsigned int src_texel_x_offset,
				unsigned int src_texel_y_offset,
				unsigned int src_texel_width,
				unsigned int src_texel_height,
				unsigned int dst_texel_width,
				unsigned int dst_texel_height);
	};
}

#endif // GPLATES_OPENGL_GLSCALARFIELDDEPTHLAYERSSOURCE_H
