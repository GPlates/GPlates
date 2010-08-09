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
#include <functional>
#include <boost/optional.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <Magick++.h>

#include "Colour.h"

#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"

#include "maths/MathsUtils.h"

#include "property-values/RawRaster.h"
#include "property-values/RawRasterUtils.h"


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
			 * Creates a BasicMipmapper.
			 *
			 * The purpose of this class is to progressively create mipmaps from
			 * @a source_raster until the largest dimension in the mipmap is less than or
			 * equal to the @a threshold_size.
			 *
			 * @a threshold_size must be greater than or equal to 1.
			 *
			 * Note that you must call @a generate_next before retrieving the first
			 * mipmap using @a get_current_mipmap.
			 */
			BasicMipmapper(
					const typename output_raster_type::non_null_ptr_to_const_type &source_raster,
					unsigned int threshold_size) :
				d_current_mipmap(source_raster),
				d_current_coverage_has_fractional_values(false),
				d_threshold_size(threshold_size)
			{
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						threshold_size >= 1,
						GPLATES_ASSERTION_SOURCE);

				d_current_coverage = CreateCoverageRawRaster<RawRasterType,
								   RawRasterType::has_no_data_value>::create_coverage_raster(
								   		   *source_raster);
			}

			/**
			 * Generates the next mipmap in the sequence of mipmaps.
			 *
			 * Returns true if a mipmap was successfully created. Returns false if no new
			 * mipmap was generated; this occurs when the current mipmap's largest
			 * dimension is less than or equal to the @a threshold_size specified in the
			 * constructor.
			 */
			bool
			generate_next()
			{
				// Check whether the current mipmap is at the threshold size.
				if ((*d_current_mipmap)->width() <= d_threshold_size &&
						(*d_current_mipmap)->height() <= d_threshold_size)
				{
					// Early exit - no more levels to generate.
					return false;
				}

				return static_cast<MipmapperType *>(this)->do_generate_next();
			}

			/**
			 * Returns the number of levels that have yet to be generated up
			 * until the threshold level.
			 */
			unsigned int
			get_number_of_remaining_levels() const
			{
				unsigned int result = 0;

				unsigned int width = (*d_current_mipmap)->width();
				unsigned int height = (*d_current_mipmap)->height();

				while (width > d_threshold_size || height > d_threshold_size)
				{
					width = width / 2 + (width % 2);
					height = height / 2 + (height % 2);
					++result;
				}

				return result;
			}

			/**
			 * Returns the current mipmap held by this Mipmapper.
			 */
			typename output_raster_type::non_null_ptr_to_const_type
			get_current_mipmap() const
			{
				return *d_current_mipmap;
			}

			/**
			 * Returns the current coverage raster, if any, that corresponds to
			 * the current mipmap.
			 *
			 * Returns boost::none if the current coverage raster does not have any
			 * fractional values, i.e. the current mipmap encodes all information
			 * necessary to interpret it (a non-sentinel value means that all
			 * corresponding pixels in the original raster are non-sentinel).
			 */
			boost::optional<GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_to_const_type>
			get_current_coverage() const
			{
				if (d_current_coverage_has_fractional_values)
				{
					return d_current_coverage;
				}
				else
				{
					return boost::none;
				}
			}

		protected:

			// Note: this is intentionally not virtual.
			~BasicMipmapper()
			{
			}

			/**
			 * The raster at the current mipmap level.
			 */
			boost::optional<typename output_raster_type::non_null_ptr_to_const_type> d_current_mipmap;

			/**
			 * The coverage raster, if any, corresponding to the current mipmap.
			 */
			boost::optional<GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_to_const_type> d_current_coverage;

			/**
			 * The current coverage raster is only returned to clients if it
			 * contains fractional values, for performance reasons.
			 */
			bool d_current_coverage_has_fractional_values;

		private:

			unsigned int d_threshold_size;
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
	}


	/**
	 * Mipmapper takes a raster of type RawRasterType and produces a sequence of
	 * mipmaps of successively smaller size.
	 *
	 * For the public interface of the various Mipmapper template specialisations,
	 * @see MipmapperInternals::BasicMipmapper. In addition, all specialisations
	 * have a public constructor that takes a RawRasterType::non_null_ptr_to_const_type
	 * as first parameter, and an optional threshold size second parameter.
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

		/**
		 * @see MipmapperInternals::BasicMipmapper::BasicMipmapper.
		 */
		Mipmapper(
				const typename RawRasterType::non_null_ptr_to_const_type &source_raster,
				unsigned int threshold_size = 1) :
			base_type(source_raster, threshold_size)
		{
		}

	private:

		bool
		do_generate_next()
		{
			// Make sure the dimensions are even. After this call, the dimensions of
			// d_current_mipmap may very well have changed.
			d_current_mipmap = MipmapperInternals::extend_raster(**d_current_mipmap);
			unsigned int current_width = (*d_current_mipmap)->width();
			unsigned int current_height = (*d_current_mipmap)->height();
			unsigned int new_width = current_width / 2;
			unsigned int new_height = current_height / 2;

			// Create an Image and use it to scale the image.
			Magick::Image image(current_width, current_height, "RGBA", Magick::CharPixel, (*d_current_mipmap)->data());
			image.scale(Magick::Geometry(new_width, new_height));

			// Clear the current mipmap first to save some memory.
			d_current_mipmap = boost::none;

			// Store the scaled image in our own data structure.
			typename output_raster_type::non_null_ptr_type new_mipmap = output_raster_type::create(new_width, new_height);
			image.write(0, 0, new_width, new_height, "RGBA", Magick::CharPixel, new_mipmap->data());
			d_current_mipmap = new_mipmap;

			return true;
		}

		friend class MipmapperInternals::BasicMipmapper<RawRasterType, Mipmapper<RawRasterType> >;
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
		typedef typename RawRasterType::element_type element_type;

	public:

		typedef typename base_type::output_raster_type output_raster_type;

		/**
		 * @see MipmapperInternals::BasicMipmapper::BasicMipmapper.
		 */
		Mipmapper(
				const typename RawRasterType::non_null_ptr_to_const_type &source_raster,
				unsigned int threshold_size = 1) :
			base_type(source_raster, threshold_size),
			d_source_raster(source_raster),
			d_fraction_in_source_raster(
				get_initial_fraction_in_source_raster(
					source_raster->width(), source_raster->height()))
		{
		}

	private:

		bool
		do_generate_next()
		{
			// Make sure the dimensions are even. After this call, the dimensions of
			// d_current_mipmap may very well have changed.
			typedef GPlatesPropertyValues::CoverageRawRaster::element_type coverage_element_type;
			d_current_mipmap = MipmapperInternals::extend_raster(**d_current_mipmap);
			d_current_coverage = MipmapperInternals::extend_raster(**d_current_coverage);
			d_fraction_in_source_raster = MipmapperInternals::extend_raster(
				*d_fraction_in_source_raster, boost::optional<coverage_element_type>(coverage_element_type() /* 0.0 */));
			unsigned int current_width = (*d_current_mipmap)->width();
			unsigned int current_height = (*d_current_mipmap)->height();
			unsigned int new_width = current_width / 2;
			unsigned int new_height = current_height / 2;

			// Create new rasters for the new mipmap level.
			typename output_raster_type::non_null_ptr_type new_mipmap =
					output_raster_type::create(new_width, new_height);
			GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type new_coverage =
					GPlatesPropertyValues::CoverageRawRaster::create(new_width, new_height);
			bool new_coverage_has_fractional_values = false;
			GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type new_fraction_in_source_raster =
					GPlatesPropertyValues::CoverageRawRaster::create(new_width, new_height);

			// Pointers into the new rasters.
			element_type *new_mipmap_ptr = new_mipmap->data();
			element_type * const new_mipmap_end = new_mipmap->data() + new_width * new_height;
			coverage_element_type *new_coverage_ptr = new_coverage->data();
			coverage_element_type *new_fraction_in_source_raster_ptr = new_fraction_in_source_raster->data();

			// Pointers into the old rasters.
			const element_type *current_mipmap_ptr = (*d_current_mipmap)->data();
			const coverage_element_type *current_coverage_ptr = (*d_current_coverage)->data();
			const coverage_element_type *current_fraction_in_source_raster_ptr = d_fraction_in_source_raster->data();

			while (new_mipmap_ptr != new_mipmap_end)
			{
				for (unsigned int i = 0; i != new_width; ++i)
				{
					element_type weighted_sum_of_pixels = element_type();
					coverage_element_type sum_of_weights = coverage_element_type();
					coverage_element_type sum_of_fraction_in_source_raster = coverage_element_type();

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
					const coverage_element_type * const current_fraction_in_source_raster_pixels[] = {
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
							weighted_sum_of_pixels += weight * (*current_mipmap_pixels[j]);
						}
					}

					if (GPlatesMaths::are_almost_exactly_equal(
							sum_of_weights, coverage_element_type() /* 0.0f */))
					{
						// All of the source pixels are sentinel values.
						*new_mipmap_ptr = *d_source_raster->no_data_value();
					}
					else
					{
						*new_mipmap_ptr = weighted_sum_of_pixels / sum_of_weights;
					}

					*new_coverage_ptr = sum_of_weights / sum_of_fraction_in_source_raster;
					if (!(GPlatesMaths::are_almost_exactly_equal(*new_coverage_ptr, coverage_element_type() /* 0.0f */) ||
							GPlatesMaths::are_almost_exactly_equal(*new_coverage_ptr, static_cast<coverage_element_type>(1.0))))
					{
						new_coverage_has_fractional_values = true;
					}
					*new_fraction_in_source_raster_ptr = sum_of_fraction_in_source_raster / 4;

					// Advance pointers.
					++new_mipmap_ptr;
					++new_coverage_ptr;
					++new_fraction_in_source_raster_ptr;
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

			// Save the new rasters
			d_current_mipmap = new_mipmap;
			d_current_coverage = new_coverage;
			d_current_coverage_has_fractional_values = new_coverage_has_fractional_values;
			d_fraction_in_source_raster = new_fraction_in_source_raster;

			return true;
		}

		static
		GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_to_const_type
		get_initial_fraction_in_source_raster(
				unsigned int width,
				unsigned int height)
		{
			GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type result =
				GPlatesPropertyValues::CoverageRawRaster::create(width, height);
			std::fill(
					result->data(),
					result->data() + width * height,
					1.0f /* all pixels in source raster are in source raster! */);
			return result;
		}

		typename RawRasterType::non_null_ptr_to_const_type d_source_raster;

		/**
		 * For each pixel in the current mipmap, this raster stores the fraction
		 * of that pixel that lies within the bounds of the source raster.
		 */
		GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_to_const_type d_fraction_in_source_raster;

		friend class MipmapperInternals::BasicMipmapper<RawRasterType, Mipmapper<RawRasterType> >;
		using base_type::d_current_mipmap;
		using base_type::d_current_coverage;
		using base_type::d_current_coverage_has_fractional_values;
	};


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
		 */
		Mipmapper(
				const typename RawRasterType::non_null_ptr_to_const_type &source_raster,
				unsigned int threshold_size = 1) :
			base_type(
					GPlatesPropertyValues::RawRasterUtils::convert_integer_raster_to_float_raster<
						RawRasterType, GPlatesPropertyValues::FloatRawRaster>(*source_raster),
					threshold_size)
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
}

#endif	// GPLATES_GUI_MIPMAPPER_H
