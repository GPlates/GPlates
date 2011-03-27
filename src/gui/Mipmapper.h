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
 
#ifndef GPLATES_GUI_MIPMAPPER_H
#define GPLATES_GUI_MIPMAPPER_H

#include <algorithm>
#include <cmath>
#include <functional>
#include <utility>
#include <boost/optional.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits.hpp>

#include "Colour.h"

#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"

#include "maths/MathsUtils.h"

#include "property-values/RawRaster.h"
#include "property-values/RawRasterUtils.h"

#include "utils/Profile.h"


namespace GPlatesGui
{
	namespace MipmapperInternals
	{
		/**
		 * Creates a coverage raster from a raster.
		 */
		template<class RawRasterType, bool has_no_data_value>
		class CreateCoverageRawRaster
		{
		public:

			static
			boost::optional<GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type>
			create_coverage_raster(
					const RawRasterType &raster)
			{
				// has_no_data_value = false case handled here.
				// No work to do, because our raster can't have sentinel values anyway.
				return boost::none;
			}
		};


		template<class RawRasterType>
		class CreateCoverageRawRaster<RawRasterType, true>
		{
			class CoverageFunctor :
					public std::unary_function<typename RawRasterType::element_type,
					GPlatesPropertyValues::CoverageRawRaster::element_type>
			{
			public:

				typedef boost::function<bool (typename RawRasterType::element_type)> is_no_data_value_function_type;

				CoverageFunctor(
						is_no_data_value_function_type is_no_data_value) :
					d_is_no_data_value(is_no_data_value)
				{
				}

				GPlatesPropertyValues::CoverageRawRaster::element_type
				operator()(
						typename RawRasterType::element_type value)
				{
					static const GPlatesPropertyValues::CoverageRawRaster::element_type NO_DATA_COVERAGE_VALUE = 0.0f;
					static const GPlatesPropertyValues::CoverageRawRaster::element_type DATA_PRESENT_VALUE = 1.0f;

					if (d_is_no_data_value(value))
					{
						return NO_DATA_COVERAGE_VALUE;
					}
					else
					{
						return DATA_PRESENT_VALUE;
					}
				}

			private:

				is_no_data_value_function_type d_is_no_data_value;
			};

		public:

			static
			boost::optional<GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type>
			create_coverage_raster(
					const RawRasterType &raster)
			{
				GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type coverage =
					GPlatesPropertyValues::CoverageRawRaster::create(raster.width(), raster.height());

				std::transform(
						raster.data(),
						raster.data() + raster.width() * raster.height(),
						coverage->data(),
						CoverageFunctor(
							boost::bind(
								&RawRasterType::is_no_data_value,
								boost::cref(raster),
								_1)));

				return coverage;
			}
		};


		/**
		 * Determines if a raster has a no-data value and is not fully opaque.
		 */
		template<class RawRasterType, bool has_no_data_value>
		class DoesRasterContainANoDataValue
		{
		public:
			static
			bool
			does_raster_contain_a_no_data_value(
					const RawRasterType &raster)
			{
				// has_no_data_value = false case handled here.
				// No work to do, because our raster can't have sentinel values anyway.
				return false;
			}
		};


		template<class RawRasterType>
		class DoesRasterContainANoDataValue<RawRasterType, true/*has_no_data_value*/>
		{
		public:
			static
			bool
			does_raster_contain_a_no_data_value(
					const RawRasterType &raster)
			{
				// Iterate over the pixels and see if any are the sentinel value meaning
				// that that pixel is transparent.
				const unsigned int raster_width = raster.width();
				const unsigned int raster_height = raster.height();
				for (unsigned int j = 0; j < raster_height; ++j)
				{
					const typename RawRasterType::element_type *const row =
							raster.data() + j * raster_width;

					for (unsigned int i = 0; i < raster_width; ++i)
					{
						if (raster.is_no_data_value(row[i]))
						{
							// Raster contains a sentinel value so it's not fully opaque.
							return true;
						}
					}
				}

				// Raster contains no sentinel values, hence it is fully opaque.
				return false;
			};
		};


		/**
		 * Returns coverage raster that is fully opaque (all pixels are 1.0).
		 */
		GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_to_const_type
		get_opaque_coverage_raster(
				unsigned int width,
				unsigned int height);

		/**
		 * Returns coverage raster representing initial fractions of pixels in source raste.
		 *
		 * Initially all pixels in source raster are in source raster!
		 */
		inline
		GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_to_const_type
		get_initial_fraction_in_source_raster(
				unsigned int width,
				unsigned int height)
		{
			return get_opaque_coverage_raster(width, height);
		}


		/**
		 * BasicMipmapper contains the basic outlines of a mipmapper.
		 *
		 * BasicMipmapper is the base class of the various template specialisations of
		 * the Mipmapper class.
		 */
		template<class RawRasterType, class MipmapperType>
		class BasicMipmapper
		{
		public:
			/**
			 * The type of the raster that is produced as a result of the mipmapping process.
			 */
			typedef RawRasterType output_raster_type;

			/**
			 * The element type of the output raster type.
			 */
			typedef typename output_raster_type::element_type element_type;

			/**
			 * The coverage raster element type.
			 */
			typedef GPlatesPropertyValues::CoverageRawRaster::element_type coverage_element_type;


			/**
			 * Information about each level generated by this mipmapper.
			 */
			struct LevelInfo
			{
				unsigned int width;
				unsigned int height;
				quint64 num_bytes_main_mipmap;
				quint64 num_bytes_coverage_mipmap; // Is zero if coverage not being generated.
			};


			/**
			 * Returns the number of mipmap levels in total needed for a source raster of
			 * the specified dimensions.
			 */
			static
			unsigned int
			get_number_of_levels(
					const unsigned int threshold_size,
					const unsigned int source_raster_width,
					const unsigned int source_raster_height)
			{
				unsigned int num_levels = 0;

				unsigned int width = source_raster_width;
				unsigned int height = source_raster_height;

				while (width > threshold_size || height > threshold_size)
				{
					width = (width >> 1) + (width & 1);
					height = (height >> 1) + (height & 1);

					++num_levels;
				}

				return num_levels;
			}


			/**
			 * Returns information for all the mipmap levels in the mipmap pyramid.
			 */
			static
			std::vector<LevelInfo>
			get_level_infos(
					const unsigned int threshold_size,
					const unsigned int source_raster_width,
					const unsigned int source_raster_height,
					const bool generate_coverage)
			{
				std::vector<LevelInfo> level_infos;

				unsigned int width = source_raster_width;
				unsigned int height = source_raster_height;

				while (width > threshold_size || height > threshold_size)
				{
					width = (width >> 1) + (width & 1);
					height = (height >> 1) + (height & 1);

					LevelInfo level_info;
					level_info.width = width;
					level_info.height = height;
					level_info.num_bytes_main_mipmap = width * height * sizeof(element_type);
					level_info.num_bytes_coverage_mipmap =
							generate_coverage
							? width * height * sizeof(coverage_element_type)
							: 0;

					level_infos.push_back(level_info);
				}

				return level_infos;
			}


			/**
			 * Generates the next mipmap in the sequence of mipmaps.
			 *
			 * NOTE: It is up to the caller to ensure the next mipmap can actually be generated -
			 * use @a get_number_of_levels for this purpose.
			 *
			 * Also note that this method should be called before each call to
			 * @a get_current_mipmap and @a get_current_coverage.
			 */
			void
			generate_next()
			{
				do_generate_next();
			}

			/**
			 * Returns the current mipmap held by this Mipmapper.
			 */
			typename output_raster_type::non_null_ptr_to_const_type
			get_current_mipmap() const
			{
				// Make sure the client called 'generate_next' and the derived classes
				// actually calculated a mipmap.
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						d_current_mipmap,
						GPLATES_ASSERTION_SOURCE);

				return *d_current_mipmap;
			}

			/**
			 * Returns the current coverage raster that corresponds to the current mipmap.
			 *
			 * Returns boost::none if coverages have not been requested or
			 * make no sense for the raster type (eg, RGBA rasters don't have a coverage raster
			 * because the coverage is already inside the alpha channel).
			 */
			boost::optional<GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_to_const_type>
			get_current_coverage() const
			{
				return d_current_coverage;
			}

		protected:
			/**
			 * Creates a BasicMipmapper.
			 *
			 * The purpose of this class is to progressively create mipmaps from
			 * @a source_raster until the largest dimension in the mipmap is less than or
			 * equal to the @a threshold_size.
			 *
			 * @a threshold_size must be greater than or equal to 1.
			 *
			 * You can use @a does_raster_contain_a_no_data_value
			 * to determine the value of @a generate_coverage.
			 *
			 * Note that you must call @a generate_next before retrieving the first
			 * mipmap using @a get_current_mipmap.
			 */
			BasicMipmapper()
			{  }


			virtual
			~BasicMipmapper()
			{
			}


			/**
			 * Generate the next mipmap and optionally the coverage mipmap.
			 */
			virtual
			void
			do_generate_next() = 0;


			/**
			 * The raster at the current mipmap level.
			 */
			boost::optional<typename output_raster_type::non_null_ptr_to_const_type> d_current_mipmap;

			/**
			 * The coverage raster, if requested, corresponding to the current mipmap.
			 */
			boost::optional<GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_to_const_type> d_current_coverage;
		};

		/**
		 * Extends @a source_raster to the right and down by one pixel if its width
		 * and height are not multiples of two, respectively.
		 *
		 * If @a fill_value is set:
		 *
		 * When the raster is extended in a particular direction, this means that the
		 * new row and/or column is filled with the fill_value.
		 *
		 * If @a fill_value is boost::none (default value):
		 * 
		 * When the raster is extended in a particular direction, this means that the
		 * row or column at the edge of the @a source_raster is copied to the new row
		 * or column. New corner points take on the value of the corresponding corner
		 * point in the @a source_raster.
		 * e.g. a 5x6 raster is extended to become a 6x6 raster by copying the last
		 * row of pixels into the new, sixth row.
		 *
		 * Returns a pointer to @a source_raster if its dimensions are already even.
		 */
		template<class RawRasterType>
		typename RawRasterType::non_null_ptr_to_const_type
		extend_raster(
				const RawRasterType &source_raster,
				boost::optional<typename RawRasterType::element_type> fill_value = boost::none,
				// Can only be called if this type of raster has data.
				typename boost::enable_if_c<RawRasterType::has_data>::type *dummy = 0);


		/**
		 * Mipmaps the coverage raster @a coverage_raster and the raster @a fraction_in_source_raster
		 * containing the fraction of each pixel that lies within the original source raster.
		 *
		 * Returns both mipmapped coverage and mipmapped fraction-in-source rasters.
		 *
		 * Note that 'CoverageRasterType' is used for the coverage raster instead of
		 * a 'CoverageRawRaster' raster because for RGBA rasters the coverage is actually
		 * a 'FloatRawRaster'.
		 */
		template<typename CoverageRasterType>
		std::pair<
				// Mipmapped coverage...
				typename CoverageRasterType::non_null_ptr_to_const_type,
				// Mipmapped fraction in source raster...
				GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_to_const_type>
		mipmap_coverage_raster(
				const CoverageRasterType &coverage_raster,
				const GPlatesPropertyValues::CoverageRawRaster &fraction_in_source_raster);


		/**
		 * Mipmaps a floating-point raster @a raster.
		 *
		 * Uses @a coverage_raster and @a fraction_in_source_raster during mipmapping.
		 *
		 * Both @a coverage_raster and @a fraction_in_source_raster should be at the same
		 * mipmap level as @a raster.
		 *
		 * Note that 'CoverageRasterType' is used for the coverage raster instead of
		 * a 'CoverageRawRaster' raster because for RGBA rasters the coverage is actually
		 * a 'FloatRawRaster'.
		 */
		template<class RawRasterType, class CoverageRasterType>
		typename RawRasterType::non_null_ptr_to_const_type
		mipmap_main_raster(
				const RawRasterType &raster,
				const CoverageRasterType &coverage_raster,
				const GPlatesPropertyValues::CoverageRawRaster &fraction_in_source_raster,
				typename boost::enable_if_c<RawRasterType::has_no_data_value>::type *dummy = 0);
	}


	/**
	 * Returns true if the specified raster has a no-data sentinel value in the raster.
	 *
	 * The requirement of a no-data value is really just to rule out
	 * RGBA rasters which have an alpha-channel and hence can be transparent but
	 * do not have a no-data value (because of the alpha-channel).
	 */
	template<class RawRasterType>
	bool
	does_raster_contain_a_no_data_value(
			const RawRasterType &raster)
	{
		return MipmapperInternals::DoesRasterContainANoDataValue<
				RawRasterType, RawRasterType::has_no_data_value>
						::does_raster_contain_a_no_data_value(raster);
	}


	/**
	 * Mipmapper takes a raster of type RawRasterType and produces a sequence of
	 * mipmaps of successively smaller size.
	 *
	 * For the public interface of the various Mipmapper template specialisations,
	 * @see MipmapperInternals::BasicMipmapper. In addition, all specialisations
	 * have a public constructor that takes a RawRasterType::non_null_ptr_to_const_type
	 * as first parameter and an optional boolean (generate coverage) as their second parameter.
	 *
	 * Also all specialisations must have a static method:
	 *   float
	 *   get_max_memory_bytes_amplification_required_to_mipmap();
	 */
	template<class RawRasterType, class Enable = void>
	class Mipmapper;
		// This is intentionally not defined.


	/**
	 * This specialisation is for rasters that have an element_type of rgba8_t
	 * and are without a no-data value.
	 *
	 * This version uses ImageMagick for the downsampling; the default Lanczos filter is used.
	 */
	template<class RawRasterType>
	class Mipmapper<RawRasterType,
		typename boost::enable_if_c<!RawRasterType::has_no_data_value &&
		boost::is_same<typename RawRasterType::element_type, GPlatesGui::rgba8_t>::value>::type
	> :
			public MipmapperInternals::BasicMipmapper<RawRasterType, Mipmapper<RawRasterType> >
	{
		typedef MipmapperInternals::BasicMipmapper<RawRasterType, Mipmapper<RawRasterType> > base_type;

	public:
		typedef typename base_type::output_raster_type output_raster_type;
		typedef typename base_type::coverage_element_type coverage_element_type;

		/**
		 * Returns the maximum amount of memory, as a multiplier of the source raster width times
		 * height, that will be allocated in order to perform mipmapping.
		 *
		 * This does *not* include the memory of the source raster being mipmapped (even if it's stored).
		 */
		static
		float
		get_max_memory_bytes_amplification_required_to_mipmap()
		{
			// Calculate memory usage for generating top-level mipmap since it will use
			// the most memory.
			return
					// Linear R,G,B and A floating-point rasters...
					4 * sizeof(float) +
					// Fraction-in-source raster...
					1 * sizeof(float) +
					// Extending a raster to even dimensions allocates another raster...
					1 * sizeof(float);
			// No need to account for mipmapped 32bpp RGBA mipmap since the extended raster
			// will be deallocated before it's created so it will reuse that memory.
		}


		/**
		 * @see MipmapperInternals::BasicMipmapper::BasicMipmapper.
		 */
		Mipmapper(
				const typename RawRasterType::non_null_ptr_to_const_type &source_raster) :
			d_fraction_in_source_raster(
				MipmapperInternals::get_initial_fraction_in_source_raster(
					source_raster->width(), source_raster->height()))
		{
			initialise_linear_rgba_channels(source_raster);

			// Note: we're not initialised 'd_current_mipmap' since the call to 'do_generate_next'
			// will initialise it for us - provided the client calls it first !

			// Note: no need to generate coverages for RGBA rasters - the coverage is already
			// in the alpha channel.
		}

	private:
		virtual
		void
		do_generate_next()
		{
			PROFILE_FUNC();

			// Make sure the dimensions are even. After this call, the dimensions of
			// the mipmaps may very well have changed.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					d_linear_red_raster && d_linear_green_raster && d_linear_blue_raster && d_linear_alpha_raster,
					GPLATES_ASSERTION_SOURCE);
			d_linear_red_raster = MipmapperInternals::extend_raster(*d_linear_red_raster.get());
			d_linear_green_raster = MipmapperInternals::extend_raster(*d_linear_green_raster.get());
			d_linear_blue_raster = MipmapperInternals::extend_raster(*d_linear_blue_raster.get());
			d_linear_alpha_raster = MipmapperInternals::extend_raster(*d_linear_alpha_raster.get());

			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					d_fraction_in_source_raster,
					GPLATES_ASSERTION_SOURCE);
			d_fraction_in_source_raster = MipmapperInternals::extend_raster(
					*d_fraction_in_source_raster,
					boost::optional<coverage_element_type>(coverage_element_type() /* 0.0 */));

			//
			// NOTE: We mipmap the R,G,B channel before the alpha channel since they
			// use the previous level's alpha channel (coverage) and fraction-in-source rasters.
			//
			d_linear_red_raster = MipmapperInternals::mipmap_main_raster(
					*d_linear_red_raster.get(),
					*d_linear_alpha_raster.get(),
					*d_fraction_in_source_raster);
			d_linear_green_raster = MipmapperInternals::mipmap_main_raster(
					*d_linear_green_raster.get(),
					*d_linear_alpha_raster.get(),
					*d_fraction_in_source_raster);
			d_linear_blue_raster = MipmapperInternals::mipmap_main_raster(
					*d_linear_blue_raster.get(),
					*d_linear_alpha_raster.get(),
					*d_fraction_in_source_raster);

			// Note that the alpha channel is mipmapped as a *coverage* raster.
			// NOTE: We do this after mimapping the R,G,B channels.
			const std::pair<
				GPlatesPropertyValues::FloatRawRaster::non_null_ptr_to_const_type,
				GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_to_const_type>
					mipmapped_coverage_and_fraction_in_source_rasters =
						MipmapperInternals::mipmap_coverage_raster(
								*d_linear_alpha_raster.get(),
								*d_fraction_in_source_raster);
			d_linear_alpha_raster = mipmapped_coverage_and_fraction_in_source_rasters.first;
			d_fraction_in_source_raster = mipmapped_coverage_and_fraction_in_source_rasters.second;

			// Creates gamma-corrected RGBA raster using the current linear R,G,B,A rasters.
			d_current_mipmap = create_gamma_corrected_rgba_raster();

			// Note that the 'd_current_coverage' mipmap raster is left uninitialised because
			// it's not actually used by our clients - the coverage is already in the alpha channel.
		}

		/**
		 * Converts each presumably gamma-corrected R,G,B,A channel of source raster to a
		 * linear intensity floating-point raster channel (each channel in a separate float raster).
		 */
		void
		initialise_linear_rgba_channels(
				const typename RawRasterType::non_null_ptr_to_const_type &source_raster)
		{
			PROFILE_FUNC();

			const unsigned int source_raster_width = source_raster->width();
			const unsigned int source_raster_height = source_raster->height();

			// Create the R,G,B,A float rasters containing *linear* colour values.
			GPlatesPropertyValues::FloatRawRaster::non_null_ptr_type linear_red_raster =
					GPlatesPropertyValues::FloatRawRaster::create(
							source_raster_width, source_raster_height);
			GPlatesPropertyValues::FloatRawRaster::non_null_ptr_type linear_green_raster =
					GPlatesPropertyValues::FloatRawRaster::create(
							source_raster_width, source_raster_height);
			GPlatesPropertyValues::FloatRawRaster::non_null_ptr_type linear_blue_raster =
					GPlatesPropertyValues::FloatRawRaster::create(
							source_raster_width, source_raster_height);
			GPlatesPropertyValues::FloatRawRaster::non_null_ptr_type linear_alpha_raster =
					GPlatesPropertyValues::FloatRawRaster::create(
							source_raster_width, source_raster_height);

			// Used to convert 8-bit unsigned integer to [0,1] range.
			static const double inv_255 = 1 / 255.0;

			// Iterate over the source raster pixels.
			for (unsigned int j = 0; j < source_raster_height; ++j)
			{
				const GPlatesGui::rgba8_t *const source_row_ptr = source_raster->data() + j * source_raster_width;

				float *const red_row_ptr = linear_red_raster->data() + j * source_raster_width;
				float *const green_row_ptr = linear_green_raster->data() + j * source_raster_width;
				float *const blue_row_ptr = linear_blue_raster->data() + j * source_raster_width;
				float *const alpha_row_ptr = linear_alpha_raster->data() + j * source_raster_width;

				for (unsigned int i = 0; i < source_raster_width; ++i)
				{
					const GPlatesGui::rgba8_t &source_pixel = source_row_ptr[i];

					// Each 8-bit colour channel is presumably already gamma-corrected.
					// This is the case for JPEG images.
					// Most images are stored this way so that they appear correctly on a
					// monitor with a gamma of 2.2 - it also turns out to be a good way to reduce
					// banding at low intensities due to 8-bit integer quantisation - in other
					// words low values of each 8-bit integer channel represent a smaller range
					// of linear intensities and hence less banding is visible - this is due to
					// the compression of colour values due to the gamma-correction curve.
					//
					// Using the faster square power instead of 2.2 since this code
					// is a bottleneck at the moment.
#if 1
					red_row_ptr[i] = inv_255 * source_pixel.red;
					red_row_ptr[i] *= red_row_ptr[i];
					green_row_ptr[i] = inv_255 * source_pixel.green;
					green_row_ptr[i] *= green_row_ptr[i];
					blue_row_ptr[i] = inv_255 * source_pixel.blue;
					blue_row_ptr[i] *= blue_row_ptr[i];
					alpha_row_ptr[i] = inv_255 * source_pixel.alpha;
					alpha_row_ptr[i] *= alpha_row_ptr[i];
#else
					red_row_ptr[i] = std::pow(inv_255 * source_pixel.red, 2.2);
					green_row_ptr[i] = std::pow(inv_255 * source_pixel.green, 2.2);
					blue_row_ptr[i] = std::pow(inv_255 * source_pixel.blue, 2.2);
					alpha_row_ptr[i] = std::pow(inv_255 * source_pixel.alpha, 2.2);
#endif
				}
			}

			// Store the linear R,G,B,A rasters.
			// We will mipmap these since they are in linear space.
			d_linear_red_raster = linear_red_raster;
			d_linear_green_raster = linear_green_raster;
			d_linear_blue_raster = linear_blue_raster;
			d_linear_alpha_raster = linear_alpha_raster;
		}


		/**
		 * Gamma-corrects the current linear R,G,B,A float rasters and stores into
		 * 8-bit channels - this is the actual mipmap used by our clients.
		 *
		 * The linear intensity channel rasters are temporary.
		 */
		typename output_raster_type::non_null_ptr_to_const_type
		create_gamma_corrected_rgba_raster()
		{
			PROFILE_FUNC();

			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					d_linear_red_raster && d_linear_green_raster && d_linear_blue_raster && d_linear_alpha_raster,
					GPLATES_ASSERTION_SOURCE);

			const unsigned int raster_width = d_linear_red_raster.get()->width();
			const unsigned int raster_height = d_linear_red_raster.get()->height();

			// Create new gamma-corrected RGBA raster for the current mipmap level.
			typename output_raster_type::non_null_ptr_type rgba_raster =
					output_raster_type::create(raster_width, raster_height);

			// Iterate over the source raster pixels.
			for (unsigned int j = 0; j < raster_height; ++j)
			{
				GPlatesGui::rgba8_t *const rgba_row_ptr = rgba_raster->data() + j * raster_width;

				const float *const red_row_ptr = d_linear_red_raster.get()->data() + j * raster_width;
				const float *const green_row_ptr = d_linear_green_raster.get()->data() + j * raster_width;
				const float *const blue_row_ptr = d_linear_blue_raster.get()->data() + j * raster_width;
				const float *const alpha_row_ptr = d_linear_alpha_raster.get()->data() + j * raster_width;

				for (unsigned int i = 0; i < raster_width; ++i)
				{
					GPlatesGui::rgba8_t &rgba_pixel = rgba_row_ptr[i];

					// Each 8-bit colour channel is presumably already gamma-corrected.
					// This is the case for JPEG images.
					// Most images are stored this way so that they appear correctly on a
					// monitor with a gamma of 2.2 - it also turns out to be a good way to reduce
					// banding at low intensities due to 8-bit integer quantisation - in other
					// words low values of each 8-bit integer channel represent a smaller range
					// of linear intensities and hence less banding is visible - this is due to
					// the compression of colour values due to the gamma-correction curve.

					// All linear colour values should be in the range [0,1].
					// To convert to 8-bit unsigned integers we multiply by 255 and
					// then round to the nearest integer (means adding 0.5 and rounding down).
					//
					// Using the faster square root instead of 1/2.2 since this code
					// is a small bottleneck at the moment - also matches up with use
					// of gamma of 2.0 above for speed.
#if 1
					rgba_pixel.red = static_cast<boost::uint8_t>(
							255 * std::sqrt(red_row_ptr[i]) + 0.5);
					rgba_pixel.green = static_cast<boost::uint8_t>(
							255 * std::sqrt(green_row_ptr[i]) + 0.5);
					rgba_pixel.blue = static_cast<boost::uint8_t>(
							255 * std::sqrt(blue_row_ptr[i]) + 0.5);
					rgba_pixel.alpha = static_cast<boost::uint8_t>(
							255 * std::sqrt(alpha_row_ptr[i]) + 0.5);
#else
					// Used to convert [0,1] range to 8-bit unsigned integer.
					static const double inv_gamma = 1 / 2.2;

					rgba_pixel.red = static_cast<boost::uint8_t>(
							255 * std::pow(double(red_row_ptr[i]), inv_gamma) + 0.5);
					rgba_pixel.green = static_cast<boost::uint8_t>(
							255 * std::pow(double(green_row_ptr[i]), inv_gamma) + 0.5);
					rgba_pixel.blue = static_cast<boost::uint8_t>(
							255 * std::pow(double(blue_row_ptr[i]), inv_gamma) + 0.5);
					rgba_pixel.alpha = static_cast<boost::uint8_t>(
							255 * std::pow(double(alpha_row_ptr[i]), inv_gamma) + 0.5);
#endif
				}
			}

			return rgba_raster;
		}


		//
		// These float rasters store RGBA in linear space (converted from gamma-corrected pixels).
		// This is so the mipmapping can be done in linear colour intensity space where it makes
		// sense to add colour intensities.
		// Conversion from linear back to gamma-corrected space will happen before saving each mipmap.
		//
		boost::optional<GPlatesPropertyValues::FloatRawRaster::non_null_ptr_to_const_type> d_linear_red_raster;
		boost::optional<GPlatesPropertyValues::FloatRawRaster::non_null_ptr_to_const_type> d_linear_green_raster;
		boost::optional<GPlatesPropertyValues::FloatRawRaster::non_null_ptr_to_const_type> d_linear_blue_raster;
		boost::optional<GPlatesPropertyValues::FloatRawRaster::non_null_ptr_to_const_type> d_linear_alpha_raster;

		/**
		 * For each pixel in the current mipmap, this raster stores the fraction
		 * of that pixel that lies within the bounds of the source raster.
		 */
		GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_to_const_type d_fraction_in_source_raster;

		using base_type::d_current_mipmap;
	};


	/**
	 * This specialisation is for rasters that have a floating-point element_type
	 * and that have a no-data value.
	 *
	 * This version downsamples using averaging: a pixel in level n is the average
	 * of the four pixels in level (n - 1) that correspond to that pixel, weighted
	 * by the coverage and the "fraction-in-source-raster" value at those pixels.
	 */
	template<class RawRasterType>
	class Mipmapper<RawRasterType,
		typename boost::enable_if_c<RawRasterType::has_no_data_value &&
		boost::is_floating_point<typename RawRasterType::element_type>::value >::type
	> :
			public MipmapperInternals::BasicMipmapper<RawRasterType, Mipmapper<RawRasterType> >
	{
		typedef MipmapperInternals::BasicMipmapper<RawRasterType, Mipmapper<RawRasterType> > base_type;

	public:
		typedef typename base_type::output_raster_type output_raster_type;
		typedef typename base_type::coverage_element_type coverage_element_type;

		/**
		 * Returns the maximum amount of memory, as a multiplier of the source raster width times
		 * height, that will be allocated in order to perform mipmapping.
		 *
		 * This does *not* include the memory of the source raster being mipmapped (even if it's stored).
		 */
		static
		float
		get_max_memory_bytes_amplification_required_to_mipmap()
		{
			// Calculate memory usage for generating top-level mipmap since it will use
			// the most memory.
			return
					// No need to account for the floating-point raster as we store the source raster...
					0 * sizeof(typename RawRasterType::element_type) +
					// Coverage raster...
					1 * sizeof(GPlatesPropertyValues::CoverageRawRaster::element_type) +
					// Fraction-in-source raster...
					1 * sizeof(GPlatesPropertyValues::CoverageRawRaster::element_type) +
					// Extending a raster to even dimensions allocates another raster...
					1 * sizeof(typename RawRasterType::element_type);
			// No need to account for mipmapped raster since the extended raster
			// will be deallocated before it's created so it will reuse that memory.
		}


		/**
		 * @see MipmapperInternals::BasicMipmapper::BasicMipmapper.
		 *
		 * Can use @a does_raster_contain_a_no_data_value to determine
		 * the value of @a generate_coverage.
		 */
		Mipmapper(
				const typename RawRasterType::non_null_ptr_to_const_type &source_raster,
				bool generate_coverage) :
			d_source_raster(source_raster),
			d_fraction_in_source_raster(
				MipmapperInternals::get_initial_fraction_in_source_raster(
					source_raster->width(), source_raster->height()))
		{
			d_current_mipmap = source_raster;

			if (generate_coverage)
			{
				d_current_coverage = MipmapperInternals::CreateCoverageRawRaster<RawRasterType,
					   RawRasterType::has_no_data_value>::create_coverage_raster(
		   					   *source_raster);
			}
		}

	private:
		virtual
		void
		do_generate_next()
		{
			PROFILE_FUNC();

			// Make sure the dimensions are even. After this call, the dimensions of
			// the mipmaps may very well have changed.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					d_current_mipmap,
					GPLATES_ASSERTION_SOURCE);
			d_current_mipmap = MipmapperInternals::extend_raster(*d_current_mipmap.get());

			// Has coverage generation been requested?
			const bool generating_coverage = d_current_coverage;
			// Use a fully opaque coverage if we're not generating coverages.
			// This means more work for the CPU but less changes to the code.
			// TODO: This is all temporary until the quad-tree mipmap tiling is implemented.
			if (!d_current_coverage)
			{
				d_current_coverage = MipmapperInternals::get_opaque_coverage_raster(
						d_current_mipmap.get()->width(),
						d_current_mipmap.get()->height());
			}
			d_current_coverage = MipmapperInternals::extend_raster(*d_current_coverage.get());

			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					d_fraction_in_source_raster,
					GPLATES_ASSERTION_SOURCE);
			d_fraction_in_source_raster = MipmapperInternals::extend_raster(
					*d_fraction_in_source_raster,
					boost::optional<coverage_element_type>(coverage_element_type() /* 0.0 */));

			//
			// NOTE: We mipmap the main raster before the coverage raster since they
			// use the previous level's coverage and fraction-in-source rasters.
			//
			d_current_mipmap = MipmapperInternals::mipmap_main_raster(
					*d_current_mipmap.get(),
					*d_current_coverage.get(),
					*d_fraction_in_source_raster);

			// NOTE: We do this after mimapping the main raster.
			const std::pair<
				GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_to_const_type,
				GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_to_const_type>
					mipmapped_coverage_and_fraction_in_source_rasters =
						MipmapperInternals::mipmap_coverage_raster(
								*d_current_coverage.get(),
								*d_fraction_in_source_raster);

			if (generating_coverage)
			{
				d_current_coverage = mipmapped_coverage_and_fraction_in_source_rasters.first;
			}
			else
			{
				// Reset the coverage back to boost::none if we've not been requested
				// to generate coverages.
				// TODO: This is not ideal but is temporary until quad-tree mipmap tiling is implemented.
				d_current_coverage = boost::none;
			}
			d_fraction_in_source_raster = mipmapped_coverage_and_fraction_in_source_rasters.second;
		}


		typename RawRasterType::non_null_ptr_to_const_type d_source_raster;

		/**
		 * For each pixel in the current mipmap, this raster stores the fraction
		 * of that pixel that lies within the bounds of the source raster.
		 */
		GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_to_const_type d_fraction_in_source_raster;

		using base_type::d_current_mipmap;
		using base_type::d_current_coverage;
	};


#if defined(_MSC_VER) && _MSC_VER <= 1400
	namespace MipmapperInternals
	{
		// MSVC2005 has trouble finding the base class for Mipmapper<RawRaster>,
		// where RawRaster is an integer raster (this is the specialisation
		// immediately below this). This hack forces the Mipmapper for float
		// rasters to get instantiated here.
		class IgnoreMe
		{
			Mipmapper<GPlatesPropertyValues::FloatRawRaster> d_mipmapper;
		};
	}
#endif


	/**
	 * This specialisation is for rasters that have an integral element_type
	 * and that have a no-data value.
	 *
	 * This version converts the raster into a FloatRawRaster and then
	 * defers to the algorithm for mipmapping floating-point rasters.
	 */
	template<class RawRasterType>
	class Mipmapper<RawRasterType,
		typename boost::enable_if_c<RawRasterType::has_no_data_value &&
		boost::is_integral<typename RawRasterType::element_type>::value >::type
	> :
			public Mipmapper<GPlatesPropertyValues::FloatRawRaster>
	{
		typedef Mipmapper<GPlatesPropertyValues::FloatRawRaster> base_type;

	public:
		typedef typename base_type::output_raster_type output_raster_type;

		/**
		 * @see MipmapperInternals::BasicMipmapper::BasicMipmapper.
		 *
		 * Can use @a does_raster_contain_a_no_data_value to determine
		 * the value of @a generate_coverage.
		 */
		Mipmapper(
				const typename RawRasterType::non_null_ptr_to_const_type &source_raster,
				bool generate_coverage) :
			base_type(
					GPlatesPropertyValues::RawRasterUtils::convert_integer_raster_to_float_raster<
						RawRasterType, GPlatesPropertyValues::FloatRawRaster>(*source_raster),
					generate_coverage)
		{
		}
	};


	template<class RawRasterType>
	typename RawRasterType::non_null_ptr_to_const_type
	MipmapperInternals::extend_raster(
			const RawRasterType &source_raster,
			boost::optional<typename RawRasterType::element_type> fill_value,
			typename boost::enable_if_c<RawRasterType::has_data>::type *dummy)
	{
		unsigned int source_width = source_raster.width();
		unsigned int source_height = source_raster.height();
		bool extend_right = (source_width % 2);
		bool extend_down = (source_height % 2);
		
		// Early exit if no work to do...
		if (!extend_right && !extend_down)
		{
			return typename RawRasterType::non_null_ptr_to_const_type(&source_raster);
		}

		unsigned int dest_width = (extend_right ? source_width + 1 : source_width);
		unsigned int dest_height = (extend_down ? source_height + 1 : source_height);

		// Doesn't hurt to check for overflow...
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				dest_width >= source_width && dest_height >= source_height,
				GPLATES_ASSERTION_SOURCE);

		// Acquire a pointer to the source buffer.
		typedef typename RawRasterType::element_type element_type;
		const element_type * const source_buf = source_raster.data();

		// Allocate the destination buffer and get a pointer to it.
		typename RawRasterType::non_null_ptr_type dest_raster =
			RawRasterType::create(dest_width, dest_height);
		element_type * const dest_buf = dest_raster->data();

		// Copy the source buffer to the dest buffer.
		const element_type *source_ptr = source_buf;
		const element_type * const source_end_ptr = (source_buf + source_width * source_height);
		element_type *dest_ptr = dest_buf;
		element_type * const dest_end_ptr = (dest_buf + dest_width * dest_height);
		while (source_ptr != source_end_ptr)
		{
			std::copy(source_ptr, source_ptr + source_width, dest_ptr);
			source_ptr += source_width;
			dest_ptr += dest_width; // using dest_width ensures correct offset in new buffer
		}

		// Copy the last column over, or fill it with fill_value,
		// if extending to the right.
		if (extend_right)
		{
			dest_ptr = dest_buf;
			while (dest_ptr != dest_end_ptr)
			{
				// Note that if we get to here, dest_width >= 2.
				dest_ptr += dest_width;
				*(dest_ptr - 1) = fill_value ? *fill_value : *(dest_ptr - 2);
			}
		}

		// Copy the last source row down, or fill it with fill_value,
		// if extending down.
		// Note that this also fills the new corner, if extending both right and down.
		if (extend_down)
		{
			// Note that if we get to here, dest_height >= 2.
			element_type *second_last_row_ptr = (dest_buf + dest_width * (dest_height - 2));
			element_type *last_row_ptr = (dest_buf + dest_width * (dest_height - 1));

			if (fill_value)
			{
				std::fill(last_row_ptr, last_row_ptr + dest_width, *fill_value);
			}
			else
			{
				std::copy(second_last_row_ptr, second_last_row_ptr + dest_width, last_row_ptr);
			}
		}
		
		return dest_raster;
	}


	template<typename CoverageRasterType>
	std::pair<
			// Mipmapped coverage...
			typename CoverageRasterType::non_null_ptr_to_const_type,
			// Mipmapped fraction in source raster...
			GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_to_const_type>
	MipmapperInternals::mipmap_coverage_raster(
			const CoverageRasterType &coverage_raster,
			const GPlatesPropertyValues::CoverageRawRaster &fraction_in_source_raster)
	{
		typedef typename CoverageRasterType::element_type coverage_element_type;
		typedef GPlatesPropertyValues::CoverageRawRaster::element_type fraction_in_source_element_type;

		unsigned int current_width = fraction_in_source_raster.width();
		unsigned int current_height = fraction_in_source_raster.height();
		unsigned int new_width = current_width / 2;
		unsigned int new_height = current_height / 2;

		// Create the mipmapped coverage raster.
		typename CoverageRasterType::non_null_ptr_type new_coverage =
				CoverageRasterType::create(new_width, new_height);
		// Create the mipmapped fraction in source raster.
		GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type new_fraction_in_source_raster =
				GPlatesPropertyValues::CoverageRawRaster::create(new_width, new_height);

		// Pointers into the new rasters.
		coverage_element_type *new_coverage_ptr = new_coverage->data();
		coverage_element_type *const new_coverage_end = new_coverage->data() + new_width * new_height;
		fraction_in_source_element_type *new_fraction_in_source_raster_ptr = new_fraction_in_source_raster->data();

		// Pointers into the old rasters.
		const coverage_element_type *current_coverage_ptr = coverage_raster.data();
		const fraction_in_source_element_type *current_fraction_in_source_raster_ptr =
				fraction_in_source_raster.data();

		while (new_coverage_ptr != new_coverage_end)
		{
			for (unsigned int i = 0; i != new_width; ++i)
			{
				coverage_element_type sum_of_weights = coverage_element_type();
				fraction_in_source_element_type sum_of_fraction_in_source_raster =
						fraction_in_source_element_type();

				const coverage_element_type * const current_coverage_pixels[] = {
					current_coverage_ptr,
					current_coverage_ptr + 1,
					current_coverage_ptr + current_width,
					current_coverage_ptr + current_width + 1
				};
				const fraction_in_source_element_type * const current_fraction_in_source_raster_pixels[] = {
					current_fraction_in_source_raster_ptr,
					current_fraction_in_source_raster_ptr + 1,
					current_fraction_in_source_raster_ptr + current_width,
					current_fraction_in_source_raster_ptr + current_width + 1
				};

				// Go through the four pixels that will be downsampled to one.
				for (int j = 0; j != 4; ++j)
				{
					sum_of_fraction_in_source_raster += *current_fraction_in_source_raster_pixels[j];

					// Don't process the pixel if it is entirely sentinel value,
					// because mixing NaNs into the sum is going to screw things up.
					if (!GPlatesMaths::are_almost_exactly_equal(
							*current_coverage_pixels[j], coverage_element_type() /* 0.0f */))
					{
						coverage_element_type weight = (*current_coverage_pixels[j]) * 
							(*current_fraction_in_source_raster_pixels[j]);
						sum_of_weights += weight;
					}
				}

				*new_coverage_ptr = sum_of_weights / sum_of_fraction_in_source_raster;
#if 0
				if (!(GPlatesMaths::are_almost_exactly_equal(*new_coverage_ptr, coverage_element_type() /* 0.0f */) ||
						GPlatesMaths::are_almost_exactly_equal(*new_coverage_ptr, static_cast<coverage_element_type>(1.0))))
				{
					new_coverage_has_fractional_values = true;
				}
#endif
				*new_fraction_in_source_raster_ptr = sum_of_fraction_in_source_raster / 4;

				// Advance pointers.
				++new_coverage_ptr;
				++new_fraction_in_source_raster_ptr;
				current_coverage_ptr += 2;
				current_fraction_in_source_raster_ptr += 2;
			}

			// Because one line in the new mipmap corresponds to two lines
			// in the current mipmap, skip over a line.
			current_coverage_ptr += current_width;
			current_fraction_in_source_raster_ptr += current_width;
		}

		// Return the two mipmapped rasters.
		return std::make_pair(new_coverage, new_fraction_in_source_raster);
	}


	template<class RawRasterType, class CoverageRasterType>
	typename RawRasterType::non_null_ptr_to_const_type
	MipmapperInternals::mipmap_main_raster(
			const RawRasterType &raster,
			const CoverageRasterType &coverage_raster,
			const GPlatesPropertyValues::CoverageRawRaster &fraction_in_source_raster,
			typename boost::enable_if_c<RawRasterType::has_no_data_value>::type *dummy)
	{
		typedef typename RawRasterType::element_type element_type;
		typedef typename CoverageRasterType::element_type coverage_element_type;
		typedef GPlatesPropertyValues::CoverageRawRaster::element_type fraction_in_source_element_type;

		unsigned int current_width = raster.width();
		unsigned int current_height = raster.height();
		unsigned int new_width = current_width / 2;
		unsigned int new_height = current_height / 2;

		// Create the mipmapped raster.
		typename RawRasterType::non_null_ptr_type new_mipmap =
				RawRasterType::create(new_width, new_height);

		// Pointers into the new rasters.
		element_type *new_mipmap_ptr = new_mipmap->data();
		element_type * const new_mipmap_end = new_mipmap->data() + new_width * new_height;

		// Pointers into the old rasters.
		const element_type *current_mipmap_ptr = raster.data();
		const coverage_element_type *current_coverage_ptr = coverage_raster.data();
		const fraction_in_source_element_type *current_fraction_in_source_raster_ptr =
				fraction_in_source_raster.data();

		while (new_mipmap_ptr != new_mipmap_end)
		{
			for (unsigned int i = 0; i != new_width; ++i)
			{
				element_type weighted_sum_of_pixels = element_type();
				coverage_element_type sum_of_weights = coverage_element_type();

				const element_type * const current_mipmap_pixels[] = {
					current_mipmap_ptr,
					current_mipmap_ptr + 1,
					current_mipmap_ptr + current_width,
					current_mipmap_ptr + current_width + 1
				};
				const coverage_element_type * const current_coverage_pixels[] = {
					current_coverage_ptr,
					current_coverage_ptr + 1,
					current_coverage_ptr + current_width,
					current_coverage_ptr + current_width + 1
				};
				const fraction_in_source_element_type * const current_fraction_in_source_raster_pixels[] = {
					current_fraction_in_source_raster_ptr,
					current_fraction_in_source_raster_ptr + 1,
					current_fraction_in_source_raster_ptr + current_width,
					current_fraction_in_source_raster_ptr + current_width + 1
				};

				// Go through the four pixels that will be downsampled to one.
				for (int j = 0; j != 4; ++j)
				{
					// Don't process the pixel if it is entirely sentinel value,
					// because mixing NaNs into the sum is going to screw things up.
					if (!GPlatesMaths::are_almost_exactly_equal(
							*current_coverage_pixels[j], coverage_element_type() /* 0.0f */))
					{
						coverage_element_type weight = (*current_coverage_pixels[j]) * 
							(*current_fraction_in_source_raster_pixels[j]);
						sum_of_weights += weight;
						weighted_sum_of_pixels += weight * (*current_mipmap_pixels[j]);
					}
				}

				if (GPlatesMaths::are_almost_exactly_equal(
						sum_of_weights, coverage_element_type() /* 0.0f */))
				{
					// All of the source pixels are sentinel values.
					// It's a float raster which has a Nan no-data value so
					// we dereference the returned boost::optional.
					*new_mipmap_ptr = *raster.no_data_value();
				}
				else
				{
					*new_mipmap_ptr = weighted_sum_of_pixels / sum_of_weights;
				}

				// Advance pointers.
				++new_mipmap_ptr;
				current_mipmap_ptr += 2;
				current_coverage_ptr += 2;
				current_fraction_in_source_raster_ptr += 2;
			}

			// Because one line in the new mipmap corresponds to two lines
			// in the current mipmap, skip over a line.
			current_mipmap_ptr += current_width;
			current_coverage_ptr += current_width;
			current_fraction_in_source_raster_ptr += current_width;
		}

		// Return mipmapped raster.
		return new_mipmap;
	}
}

#endif	// GPLATES_GUI_MIPMAPPER_H
