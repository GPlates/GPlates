/* $Id$ */

/**
 * \file 
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2010, 2011 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTYVALUES_PROXIEDRASTERRESOLVER_H
#define GPLATES_PROPERTYVALUES_PROXIEDRASTERRESOLVER_H

#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QDebug>

#include "RawRaster.h"
#include "RawRasterUtils.h"

#include "file-io/MipmappedRasterFormatReader.h"
#include "file-io/MipmappedRasterFormatWriter.h"
#include "file-io/RasterFileCache.h"
#include "file-io/RasterFileCacheFormat.h"
#include "file-io/RasterReader.h"
#include "file-io/TemporaryFileRegistry.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "gui/ColourRawRaster.h"
#include "gui/Mipmapper.h"
#include "gui/RasterColourPalette.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/Profile.h"
#include "utils/ReferenceCount.h"


namespace GPlatesPropertyValues
{
	/**
	 * ProxiedRasterResolver takes a proxied raw raster and allows you to retrieve
	 * actual raster data from risk.
	 *
	 * All ProxiedRasterResolver derivations can retrieve an RGBA region from an
	 * arbitrary level, given a colour palette. This is exposed as pure virtual
	 * functions in the base ProxiedRasterResolver class.
	 *
	 * For retrieving regions in the raster's native data type, you will need a
	 * reference to the specific resolver derivation for that data type. If all you
	 * have is a RawRaster (and you don't know the specific type of RawRaster), you
	 * will need to get hold of the specific type using the utility functions in
	 * RawRasterUtils: determine the data type of the raster and whether it has
	 * proxied data, and then cast the RawRaster to the expected type.
	 */
	class ProxiedRasterResolver :
			public GPlatesUtils::ReferenceCount<ProxiedRasterResolver>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<ProxiedRasterResolver> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const ProxiedRasterResolver> non_null_ptr_to_const_type;

		virtual
		~ProxiedRasterResolver();

		/**
		 * Creates a ProxiedRasterResolver; the dynamic type is dependent upon the
		 * dynamic type of @a raster.
		 *
		 * Returns boost::none if the @a raster is not a proxied raw raster.
		 */
		static
		boost::optional<non_null_ptr_type>
		create(
				const RawRaster::non_null_ptr_type &raster);

		/**
		 * Returns a region from a mipmap level, coloured using the given colour palette.
		 *
		 * Returns boost::none if the level is not valid, or if the region is not
		 * valid, or if the colour palette is not appropriate for the underlying
		 * raster type.
		 *
		 * Returns boost::none if an error was encountered while reading from the
		 * source raster or the mipmaps file.
		 *
		 * Note that an invalid @a colour_palette is only appropriate if the
		 * underlying raster type is RGBA.
		 */
		virtual
		boost::optional<Rgba8RawRaster::non_null_ptr_type>
		get_coloured_region_from_level(
				unsigned int level,
				unsigned int region_x_offset,
				unsigned int region_y_offset,
				unsigned int region_width,
				unsigned int region_height,
				const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &colour_palette =
					GPlatesGui::RasterColourPalette::create()) = 0;

		/**
		 * Returns the number of levels in the mipmap file.
		 *
		 * The number of levels available is independent of the colour palette
		 * to be used to colour a region of a level.
		 *
		 * Returns 1 if there was an error in reading the mipmap file; 1 is
		 * returned because level 0 is read from the source raster file, not
		 * from the mipmap file.
		 */
		virtual
		unsigned int
		get_number_of_levels() = 0;

		/**
		 * Checks whether a mipmap file exists, and if not, generates a mipmap file.
		 * This function exists to allow client code to ensure mipmap generation
		 * occurs at a convenient time.
		 *
		 * For RGBA and floating-point rasters, there is only ever one mipmap
		 * file associated with the raster. The @a colour_palette parameter is
		 * ignored and has no effect.
		 *
		 * For integer rasters, there is a "main" mipmap file, used if the
		 * colour palette is floating-point. This function ensures the
		 * availability of this "main" mipmap file if @a colour_palette is either
		 * boost::none or contains a floating-point colour palette.
		 *
		 * However, if an integer colour palette is to be used with an integer
		 * raster, there is a special mipmap file created for that integer
		 * colour palette + integer raster combination. This function ensures
		 * the availability of a special mipmap file if @a colour_palette
		 * contains an integer colour palette.
		 *
		 * Returns true if a mipmap file appropriate for @a colour_palette is
		 * available after this function exits.
		 */
		virtual
		bool
		ensure_mipmaps_available(
				const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &colour_palette =
					GPlatesGui::RasterColourPalette::create()) = 0;

		/**
		 * Retrieves a region from a level in the mipmapped raster file, in the data
		 * type of the mipmapped raster file (i.e. not coloured into RGBA).
		 *
		 * If source raster is RGBA8:
		 *  - Returns RGBA8 raster.
		 *
		 * If source raster is float/double:
		 *  - Returns float/double raster.
		 *
		 * If source raster is integral:
		 *  - Returns float raster.
		 *
		 * Returns boost::none if an error is encountered when reading from disk.
		 */
		virtual
		boost::optional<RawRaster::non_null_ptr_type>
		get_region_from_level(
				unsigned int level,
				unsigned int region_x_offset,
				unsigned int region_y_offset,
				unsigned int region_width,
				unsigned int region_height) = 0;

		/**
		 * Retrieves the coverage raster (the raster that specifies, at each pixel,
		 * how much of that pixel is not the sentinel value in the source raster) for
		 * the given level and the given region.
		 *
		 * Returns boost::none on error. Also returns boost::none if all pixels in the
		 * given level are composed of fully sentinel or fully non-sentinel values.
		 *
		 * @see get_coverage_from_level.
		 */
		virtual
		boost::optional<CoverageRawRaster::non_null_ptr_type>
		get_coverage_from_level_if_necessary(
				unsigned int level,
				unsigned int region_x_offset,
				unsigned int region_y_offset,
				unsigned int region_width,
				unsigned int region_height) = 0;

		/**
		 * Retrieves the coverage raster (the raster that specifies, at each pixel,
		 * how much of that pixel is not the sentinel value in the source raster) for
		 * the given level and the given region.
		 *
		 * Returns boost::none on error only. Unlike @a get_coverage_from_level_if_necessary,
		 * if all pixels in the given level are composed of fully sentinel or fully
		 * non-sentinel values, this function will return a valid coverage raster,
		 * generated on the fly.
		 */
		virtual
		boost::optional<CoverageRawRaster::non_null_ptr_type>
		get_coverage_from_level(
				unsigned int level,
				unsigned int region_x_offset,
				unsigned int region_y_offset,
				unsigned int region_width,
				unsigned int region_height) = 0;

		/**
		 * Retrieves a region from the source raster, in the data type of the source
		 * raster (i.e. not coloured into RGBA).
		 *
		 * If source raster is RGBA8:
		 *  - Returns RGBA8 raster.
		 *
		 * If source raster is float/double:
		 *  - Returns float/double raster.
		 *
		 * If source raster is integral:
		 *  - Returns integral raster.
		 *
		 * Returns boost::none if an error was encountered reading from disk.
		 */
		virtual
		boost::optional<RawRaster::non_null_ptr_type>
		get_region_from_source(
				unsigned int region_x_offset,
				unsigned int region_y_offset,
				unsigned int region_width,
				unsigned int region_height) = 0;

	protected:

		ProxiedRasterResolver();

		/**
		 * Returns the RasterBandReaderHandle from a proxied @a raster.
		 *
		 * This lives here because ProxiedRasterResolver is a friend of the
		 * WithProxiedData policy class.
		 */
		template<class ProxiedRawRasterType>
		GPlatesFileIO::RasterBandReaderHandle &
		get_raster_band_reader_handle(
				ProxiedRawRasterType &raster)
		{
			return raster.get_raster_band_reader_handle();
		}

		template<class ProxiedRawRasterType>
		const GPlatesFileIO::RasterBandReaderHandle &
		get_raster_band_reader_handle(
				const ProxiedRawRasterType &raster)
		{
			return raster.get_raster_band_reader_handle();
		}
	};


	namespace ProxiedRasterResolverInternals
	{

		/**
		 * BaseProxiedRasterResolver resolves proxied rasters, but only using
		 * the "main", not-colour-palette-specific, mipmap file.
		 *
		 * As such, it does everything correctly, except for the case of integer
		 * rasters with integer colour palettes.
		 */
		template<class ProxiedRawRasterType>
		class BaseProxiedRasterResolver :
				public ProxiedRasterResolver
		{
		public:

			/**
			 * This is the type of raw raster that can be read from the source raster file.
			 */
			typedef typename RawRasterUtils::ConvertProxiedRasterToUnproxiedRaster<ProxiedRawRasterType>
					::unproxied_raster_type source_raster_type;

			/**
			 * This is the type of raw raster that can be read from the mipmapped file.
			 *
			 * For RGBA8 and floating point rasters, this is the same as @a source_raster_type.
			 * For integer rasters, @a mipmapped_raster_type uses float.
			 */
			typedef typename GPlatesGui::Mipmapper<source_raster_type>::output_raster_type mipmapped_raster_type;

		private:

			// Helper struct for get_coloured_region_from_level() function.
			template<class MipmappedRasterType, bool is_rgba8 /* = false */>
			struct ColourRegionIfNecessary
			{
				static
				boost::optional<Rgba8RawRaster::non_null_ptr_type>
				colour_region_if_necessary(
						const typename MipmappedRasterType::non_null_ptr_type &region_raster,
						const boost::optional<CoverageRawRaster::non_null_ptr_type> &region_coverage,
						const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &colour_palette)
				{
					//PROFILE_FUNC();

					// Colour the region_raster using the colour_palette.
					boost::optional<Rgba8RawRaster::non_null_ptr_type> coloured_region =
							GPlatesGui::ColourRawRaster::colour_raw_raster_with_raster_colour_palette(
									*region_raster, colour_palette);
					if (!coloured_region)
					{
						return boost::none;
					}

					// Apply the coverage raster if available.
					if (region_coverage)
					{
						RawRasterUtils::apply_coverage_raster(*coloured_region, *region_coverage);
					}

					return coloured_region;
				}
			};

			template<class MipmappedRasterType>
			struct ColourRegionIfNecessary<MipmappedRasterType, /* bool is_rgba8 = */ true>
			{
				static
				boost::optional<Rgba8RawRaster::non_null_ptr_type>
				colour_region_if_necessary(
						const typename MipmappedRasterType::non_null_ptr_type &region_raster,
						const boost::optional<CoverageRawRaster::non_null_ptr_type> &region_coverage,
						const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &colour_palette)
				{
					// Do nothing: already in RGBA.
					return region_raster;
				}
			};

		public:

			/**
			 * Implementation of pure virtual function defined in base.
			 *
			 * If source raster is RGBA8:
			 *  - @a colour_palette is ignored.
			 *
			 * If source raster is float/double:
			 *  - Expects @a colour_palette to be double.
			 *  - If @a colour_palette is integral, returns boost::none.
			 *
			 * If source raster is integral:
			 *  - If @a colour_palette is double, uses mipmapped raster file.
			 *  - If @a colour_palette is integral, return boost::none.
			 *    Note that this is not the expected result; this case is
			 *    handled in a subclass.
			 */
			virtual
			boost::optional<Rgba8RawRaster::non_null_ptr_type>
			get_coloured_region_from_level(
					unsigned int level,
					unsigned int region_x_offset,
					unsigned int region_y_offset,
					unsigned int region_width,
					unsigned int region_height,
					const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &colour_palette =
						GPlatesGui::RasterColourPalette::create())
			{
				//PROFILE_FUNC();

				// Get the raster data and coverage.
				boost::optional<typename mipmapped_raster_type::non_null_ptr_type> region_raster = get_region_from_level_as_mipmapped_type(
						level, region_x_offset, region_y_offset, region_width, region_height);
				if (!region_raster)
				{
					return boost::none;
				}
				boost::optional<CoverageRawRaster::non_null_ptr_type> region_coverage = get_coverage_from_level_if_necessary(
						level, region_x_offset, region_y_offset, region_width, region_height);

				return ColourRegionIfNecessary<mipmapped_raster_type, boost::is_same<mipmapped_raster_type, Rgba8RawRaster>::value>::
					colour_region_if_necessary(*region_raster, region_coverage, colour_palette);
			}

			/**
			 * Implementation of pure virtual function defined in base.
			 */
			virtual
			unsigned int
			get_number_of_levels()
			{
				GPlatesFileIO::MipmappedRasterFormatReader<mipmapped_raster_type> *mipmap_reader =
					get_main_mipmap_reader();

				if (mipmap_reader)
				{
					return mipmap_reader->get_number_of_levels() + 1;
				}
				else
				{
					// Level 0 is read from the source raster, not the mipmap file.
					return 1;
				}
			}

			/**
			 * Implementation of pure virtual function defined in base.
			 *
			 * Note that this implementation only ensures that the "main" mipmap
			 * file is available; the @a colour_palette is ignored. Note that
			 * this is not the expected behaviour for integer rasters; this case
			 * is handled in a subclass.
			 */
			virtual
			bool
			ensure_mipmaps_available(
					const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &colour_palette =
							GPlatesGui::RasterColourPalette::create())
			{
				// Create the main mipmap reader - this ensures the mipmap file exists and is
				// ready for reading.
				// Note: ignoring the colour palette.
				return get_main_mipmap_reader();
			}

		private:

			/**
			 * Converts level 0 from the raster type stored in the source raster into the
			 * raster type stored in the mipmapped raster file.
			 *
			 * This conversion only happens if the source raster type differs from the
			 * mipmapped raster type, and that is only the case for integer rasters. For
			 * integer rasters, a conversion is made to floating-point, because the mipmap
			 * files for integer rasters stores floating-point values.
			 */
			template<class SourceRasterType, class MipmappedRasterType>
			struct ConvertLevel0IfNecessary
			{
				static
				typename MipmappedRasterType::non_null_ptr_type
				convert_level_0_if_necessary(
						const typename SourceRasterType::non_null_ptr_type &source_level_0)
				{
					return RawRasterUtils::convert_integer_raster_to_float_raster<
						SourceRasterType, MipmappedRasterType>(*source_level_0);
				}
			};

			// Template specialisation where SourceRasterType == MipmappedRasterType.
			template<class SourceRasterType>
			struct ConvertLevel0IfNecessary<SourceRasterType, SourceRasterType>
			{
				static
				typename SourceRasterType::non_null_ptr_type
				convert_level_0_if_necessary(
						const typename SourceRasterType::non_null_ptr_type &source_level_0)
				{
					// Nothing to do.
					return source_level_0;
				}
			};

		public:

			/**
			 * Retrieves a region from a level in the mipmapped raster file, in the data
			 * type of the mipmapped raster file (i.e. not coloured into RGBA).
			 *
			 * If source raster is RGBA8:
			 *  - Returns RGBA8 raster.
			 *
			 * If source raster is float/double:
			 *  - Returns float/double raster.
			 *
			 * If source raster is integral:
			 *  - Returns float raster.
			 *
			 * Returns boost::none if an error is encountered when reading from disk.
			 */
			boost::optional<typename mipmapped_raster_type::non_null_ptr_type>
			get_region_from_level_as_mipmapped_type(
					unsigned int level,
					unsigned int region_x_offset,
					unsigned int region_y_offset,
					unsigned int region_width,
					unsigned int region_height)
			{
				//PROFILE_FUNC();

				if (level == 0)
				{
					// Level 0 is not stored in the mipmap file.
					boost::optional<typename source_raster_type::non_null_ptr_type> result =
						get_region_from_source_as_source_type(
								region_x_offset,
								region_y_offset,
								region_width,
								region_height);
					if (!result)
					{
						return boost::none;
					}

					return ConvertLevel0IfNecessary<
						source_raster_type, mipmapped_raster_type>::convert_level_0_if_necessary(*result);
				}

				GPlatesFileIO::MipmappedRasterFormatReader<mipmapped_raster_type> *mipmap_reader =
					get_main_mipmap_reader();

				if (mipmap_reader)
				{
					// Level n is level n - 1 in the mipmap file, which store levels >= 1.
					return mipmap_reader->read_level(
							level - 1,
							region_x_offset,
							region_y_offset,
							region_width,
							region_height);
				}
				else
				{
					return boost::none;
				}
			}

			/**
			 * Implementation of pure virtual function defined in base.
			 *
			 * Returns @a get_region_from_level_as_mipmapped_type but as pointer to RawRaster.
			 */
			virtual
			boost::optional<RawRaster::non_null_ptr_type>
			get_region_from_level(
					unsigned int level,
					unsigned int region_x_offset,
					unsigned int region_y_offset,
					unsigned int region_width,
					unsigned int region_height)
			{
				boost::optional<typename mipmapped_raster_type::non_null_ptr_type> region_raster =
					get_region_from_level_as_mipmapped_type(
							level, region_x_offset, region_y_offset, region_width, region_height);
				if (region_raster)
				{
					return RawRaster::non_null_ptr_type(region_raster->get());
				}
				else
				{
					return boost::none;
				}
			}

			/**
			 * Implementation of pure virtual function defined in base.
			 */
			virtual
			boost::optional<CoverageRawRaster::non_null_ptr_type>
			get_coverage_from_level_if_necessary(
					unsigned int level,
					unsigned int region_x_offset,
					unsigned int region_y_offset,
					unsigned int region_width,
					unsigned int region_height)
			{
				//PROFILE_FUNC();

				if (level == 0)
				{
					// There is never a coverage raster for level 0.
					return boost::none;
				}

				GPlatesFileIO::MipmappedRasterFormatReader<mipmapped_raster_type> *mipmap_reader =
					get_main_mipmap_reader();

				if (mipmap_reader)
				{
					// Level n is level n - 1 in the mipmap file, which store levels >= 1.
					return mipmap_reader->read_coverage(
							level - 1,
							region_x_offset,
							region_y_offset,
							region_width,
							region_height);
				}
				else
				{
					return boost::none;
				}
			}


			/**
			 * Implementation of pure virtual function defined in base.
			 */
			virtual
			boost::optional<CoverageRawRaster::non_null_ptr_type>
			get_coverage_from_level(
					unsigned int level,
					unsigned int region_x_offset,
					unsigned int region_y_offset,
					unsigned int region_width,
					unsigned int region_height)
			{
				boost::optional<CoverageRawRaster::non_null_ptr_type> coverage_from_mipmap_file =
					get_coverage_from_level_if_necessary(
							level, region_x_offset, region_y_offset, region_width, region_height);
				if (coverage_from_mipmap_file)
				{
					// Great, it's there in the file already.
					return coverage_from_mipmap_file;
				}

				// Otherwise, retrieve the data from region.
				boost::optional<typename mipmapped_raster_type::non_null_ptr_type> region_raster_opt =
					get_region_from_level_as_mipmapped_type(
							level, region_x_offset, region_y_offset, region_width, region_height);
				if (!region_raster_opt)
				{
					return boost::none;
				}
				const typename mipmapped_raster_type::non_null_ptr_type &region_raster = *region_raster_opt;

				// Create the coverage raster.
				CoverageRawRaster::non_null_ptr_type coverage = CoverageRawRaster::create(
						region_width, region_height);
				CoverageRawRaster::element_type *coverage_data = coverage->data();
				CoverageRawRaster::element_type *coverage_data_end = coverage_data +
					coverage->width() * coverage->height();
				typename mipmapped_raster_type::element_type *region_data = region_raster->data();

				boost::function<bool (typename mipmapped_raster_type::element_type)> is_no_data_value_function =
					RawRasterUtils::get_is_no_data_value_function(*region_raster);

				while (coverage_data != coverage_data_end)
				{
					// If it is the no-data value, put 0.0, else put 1.0.
					*coverage_data = is_no_data_value_function(*region_data) ? 0.0 : 1.0;
					
					++coverage_data;
					++region_data;
				}

				return coverage;
			}


			/**
			 * Retrieves a region from the source raster, in the data type of the source
			 * raster (i.e. not coloured into RGBA).
			 *
			 * If source raster is RGBA8:
			 *  - Returns RGBA8 raster.
			 *
			 * If source raster is float/double:
			 *  - Returns float/double raster.
			 *
			 * If source raster is integral:
			 *  - Returns integral raster.
			 *
			 * Returns boost::none if an error was encountered reading from disk.
			 */
			boost::optional<typename source_raster_type::non_null_ptr_type>
			get_region_from_source_as_source_type(
					unsigned int region_x_offset,
					unsigned int region_y_offset,
					unsigned int region_width,
					unsigned int region_height)
			{
				//PROFILE_FUNC();

				GPlatesFileIO::RasterBandReaderHandle &raster_band_reader_handle =
					get_raster_band_reader_handle(*d_proxied_raw_raster);

				// Check that the raster band can offer us the correct data type.
				typedef typename ProxiedRawRasterType::element_type element_type;
				if (raster_band_reader_handle.get_type() != RasterType::get_type_as_enum<element_type>())
				{
					return boost::none;
				}

				// Get the region data from the source raster.
				const QRect source_region_rect(region_x_offset, region_y_offset, region_width, region_height);
				boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type> source_region_raw_raster =
						raster_band_reader_handle.get_raw_raster(source_region_rect);
				if (!source_region_raw_raster)
				{
					return boost::none;
				}

				// Downcast the source region raster to the source raster type.
				boost::optional<typename source_raster_type::non_null_ptr_type> source_region_raster =
						GPlatesPropertyValues::RawRasterUtils::try_raster_cast<
								source_raster_type>(*source_region_raw_raster.get());
				if (!source_region_raster)
				{
					// Shouldn't happen.
					return boost::none;
				}

				return source_region_raster.get();
			}

			/**
			 * Implementation of pure virtual function defined in base.
			 *
			 * Returns @a get_region_from_source_as_source_type but as pointer to RawRaster.
			 */
			virtual
			boost::optional<RawRaster::non_null_ptr_type>
			get_region_from_source(
					unsigned int region_x_offset,
					unsigned int region_y_offset,
					unsigned int region_width,
					unsigned int region_height)
			{
				boost::optional<typename source_raster_type::non_null_ptr_type> region_raster =
					get_region_from_source_as_source_type(
							region_x_offset, region_y_offset, region_width, region_height);
				if (region_raster)
				{
					return RawRaster::non_null_ptr_type(region_raster->get());
				}
				else
				{
					return boost::none;
				}
			}

		protected:

			BaseProxiedRasterResolver(
					const typename ProxiedRawRasterType::non_null_ptr_type &raster) :
				d_proxied_raw_raster(raster),
				d_error_getting_mipmap_reader(false)
			{  }

			~BaseProxiedRasterResolver()
			{  }

			typename ProxiedRawRasterType::non_null_ptr_type d_proxied_raw_raster;

		private:

			/**
			 * Returns a pointer to the mipmap reader for the main mipmap file, also ensuring
			 * that the mipmap file exists.
			 *
			 * The main mipmap is the file that holds the mipmaps for use by RGBA and
			 * floating-point rasters, and integer rasters with floating-point colour
			 * palettes.
			 *
			 * Returns NULL if the reader could not be opened for some reason.
			 */
			GPlatesFileIO::MipmappedRasterFormatReader<mipmapped_raster_type> *
			get_main_mipmap_reader()
			{
				// There's only one main mipmap file for all but integer rasters with integer colour
				// palettes and they are handled in a derived class.
				if (!d_main_mipmap_reader)
				{
					// If we fail once to get a mipmap reader then we don't need to try again.
					// This is because frequent partial mipmap builds will slow down GPlates.
					// The client code should notify the user of failure.
					if (!d_error_getting_mipmap_reader)
					{
						d_main_mipmap_reader =
								GPlatesFileIO::RasterFileCache::create_mipmapped_raster_file_cache_format_reader<
										ProxiedRawRasterType,
										mipmapped_raster_type,
										GPlatesFileIO::MipmappedRasterFormatWriter<ProxiedRawRasterType> >(
												d_proxied_raw_raster,
												get_raster_band_reader_handle(*d_proxied_raw_raster));

						// If there was an error then don't try again next time.
						if (!d_main_mipmap_reader)
						{
							d_error_getting_mipmap_reader = true;
						}
					}
				}

				return d_main_mipmap_reader.get();
			}

			// Cached so that we don't have to open and close it all the time.
			boost::shared_ptr<GPlatesFileIO::MipmappedRasterFormatReader<mipmapped_raster_type> > d_main_mipmap_reader;

			/**
			 * Prevents repeated attempts to read (or generate) the mipmap file when there's an error.
			 *
			 * If it fails once then client will need to propagate error to user.
			 */
			bool d_error_getting_mipmap_reader;
		};
	}


	template<class ProxiedRawRasterType, class Enable = void>
	class ProxiedRasterResolverImpl;


	// Specialisation where ProxiedRawRasterType has proxied data that is not integral.
	template<class ProxiedRawRasterType>
	class ProxiedRasterResolverImpl<ProxiedRawRasterType,
		typename boost::enable_if_c<ProxiedRawRasterType::has_proxied_data &&
		!boost::is_integral<typename ProxiedRawRasterType::element_type>::value>::type> :
			public ProxiedRasterResolverInternals::BaseProxiedRasterResolver<ProxiedRawRasterType>
	{
	public:

		typedef ProxiedRasterResolverImpl<ProxiedRawRasterType> this_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<this_type> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const this_type> non_null_ptr_to_const_type;

		static
		non_null_ptr_type
		create(
				const typename ProxiedRawRasterType::non_null_ptr_type &raster)
		{
			return new ProxiedRasterResolverImpl(raster);
		}

	private:

		typedef ProxiedRasterResolverInternals::BaseProxiedRasterResolver<ProxiedRawRasterType> base_type;

		ProxiedRasterResolverImpl(
				const typename ProxiedRawRasterType::non_null_ptr_type &raster) :
			base_type(raster)
		{  }
	};


	// Specialisation where ProxiedRawRasterType has proxied data that is integral.
	template<class ProxiedRawRasterType>
	class ProxiedRasterResolverImpl<ProxiedRawRasterType,
		typename boost::enable_if_c<ProxiedRawRasterType::has_proxied_data &&
		boost::is_integral<typename ProxiedRawRasterType::element_type>::value>::type> :
			public ProxiedRasterResolverInternals::BaseProxiedRasterResolver<ProxiedRawRasterType>
	{
	public:

		typedef ProxiedRasterResolverImpl<ProxiedRawRasterType> this_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<this_type> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const this_type> non_null_ptr_to_const_type;

		static
		non_null_ptr_type
		create(
				const typename ProxiedRawRasterType::non_null_ptr_type &raster)
		{
			return new ProxiedRasterResolverImpl(raster);
		}

	private:

		typedef ProxiedRasterResolverInternals::BaseProxiedRasterResolver<ProxiedRawRasterType> base_type;
		typedef typename base_type::source_raster_type source_raster_type;

	public:

		virtual
		boost::optional<Rgba8RawRaster::non_null_ptr_type>
		get_coloured_region_from_level(
				unsigned int level,
				unsigned int region_x_offset,
				unsigned int region_y_offset,
				unsigned int region_width,
				unsigned int region_height,
				const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &colour_palette =
					GPlatesGui::RasterColourPalette::create())
		{
			GPlatesGui::RasterColourPaletteType::Type colour_palette_type =
				GPlatesGui::RasterColourPaletteType::get_type(*colour_palette);
			if (colour_palette_type == GPlatesGui::RasterColourPaletteType::DOUBLE ||
					colour_palette_type == GPlatesGui::RasterColourPaletteType::INVALID)
			{
				return base_type::get_coloured_region_from_level(
						level,
						region_x_offset,
						region_y_offset,
						region_width,
						region_height,
						colour_palette);
			}

			if (level == 0)
			{
				// Get the integer data from the source raster and colour it.
				boost::optional<typename source_raster_type::non_null_ptr_type> region_raster =
					base_type::get_region_from_source_as_source_type(region_x_offset, region_y_offset, region_width, region_height);
				if (!region_raster)
				{
					return boost::none;
				}

				return GPlatesGui::ColourRawRaster::colour_raw_raster_with_raster_colour_palette(
						**region_raster, colour_palette);
			}

			// Get the coloured regions from the mipmap file associated with this colour palette.
			GPlatesFileIO::MipmappedRasterFormatReader<Rgba8RawRaster> *mipmap_reader =
				get_coloured_mipmap_reader(colour_palette);
			if (!mipmap_reader)
			{
				return boost::none;
			}

			return mipmap_reader->read_level(
					level - 1,
					region_x_offset,
					region_y_offset,
					region_width,
					region_height);
		}

		virtual
		bool
		ensure_mipmaps_available(
				const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &colour_palette =
					GPlatesGui::RasterColourPalette::create())
		{
			GPlatesGui::RasterColourPaletteType::Type colour_palette_type =
				GPlatesGui::RasterColourPaletteType::get_type(*colour_palette);
			if (colour_palette_type == GPlatesGui::RasterColourPaletteType::DOUBLE ||
					colour_palette_type == GPlatesGui::RasterColourPaletteType::INVALID)
			{
				// Note: Ignoring colour palette.
				return base_type::ensure_mipmaps_available();
			}
			else
			{
				// Create the coloured mipmap reader for the specified colour palette - this ensures
				// the mipmap file exists and is ready for reading.
				return get_coloured_mipmap_reader(colour_palette);
			}
		}

	private:

		ProxiedRasterResolverImpl(
				const typename ProxiedRawRasterType::non_null_ptr_type &raster) :
			base_type(raster),
			d_error_getting_mipmap_reader_for_current_colour_palette(false)
		{  }

		/**
		 * Returns a pointer to the mipmap reader for the coloured mipmap file for the
		 * given colour palette, after checking that the file exists.
		 *
		 * Coloured mipmap files are used for integer rasters with integer colour
		 * palettes.
		 *
		 * Returns NULL if the reader could not be opened for some reason.
		 */
		GPlatesFileIO::MipmappedRasterFormatReader<Rgba8RawRaster> *
		get_coloured_mipmap_reader(
				const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &colour_palette)
		{
			const boost::optional<std::size_t> colour_palette_id =
					GPlatesFileIO::RasterFileCacheFormat::get_colour_palette_id(colour_palette);

			// If the colour palette has changed then clear the error flag and the mipmap reader.
			if (d_colour_palette_id_of_coloured_mipmap_reader != colour_palette_id)
			{
				d_colour_palette_id_of_coloured_mipmap_reader = colour_palette_id;

				d_coloured_mipmap_reader.reset();
				d_error_getting_mipmap_reader_for_current_colour_palette = false;
			}

			// If we don't have a mipmap reader then we need to create a new one.
			if (!d_coloured_mipmap_reader)
			{
				// If we fail once to get a mipmap reader then we don't need to try
				// again until something changes - in this case the colour palette.
				// This is because frequent partial mipmap builds will slow down GPlates.
				// The client code should notify the user of failure.
				if (!d_error_getting_mipmap_reader_for_current_colour_palette)
				{
					d_coloured_mipmap_reader =
							GPlatesFileIO::RasterFileCache::create_mipmapped_raster_file_cache_format_reader<
									ProxiedRawRasterType,
									Rgba8RawRaster,
									// The 'true' means use the colour palette before mipmapping...
									GPlatesFileIO::MipmappedRasterFormatWriter<ProxiedRawRasterType, true> >(
											d_proxied_raw_raster,
											this->get_raster_band_reader_handle(*d_proxied_raw_raster),
											colour_palette);

					// If there was an error then don't try again next time
					// (unless there's a different colour palette)...
					if (!d_coloured_mipmap_reader)
					{
						d_error_getting_mipmap_reader_for_current_colour_palette = true;
					}
				}
			}

			return d_coloured_mipmap_reader.get();
		}

		boost::shared_ptr<GPlatesFileIO::MipmappedRasterFormatReader<Rgba8RawRaster> > d_coloured_mipmap_reader;
		boost::optional<std::size_t> d_colour_palette_id_of_coloured_mipmap_reader;

		/**
		 * Prevents repeated attempts to read (or generate) the mipmap file when there's an error.
		 *
		 * If it fails once then client will need to propagate error to user.
		 */
		bool d_error_getting_mipmap_reader_for_current_colour_palette;

		using base_type::d_proxied_raw_raster;
	};


	typedef ProxiedRasterResolverImpl<ProxiedInt8RawRaster> Int8ProxiedRasterResolver;
	typedef ProxiedRasterResolverImpl<ProxiedUInt8RawRaster> UInt8ProxiedRasterResolver;
	typedef ProxiedRasterResolverImpl<ProxiedInt16RawRaster> Int16ProxiedRasterResolver;
	typedef ProxiedRasterResolverImpl<ProxiedUInt16RawRaster> UInt16ProxiedRasterResolver;
	typedef ProxiedRasterResolverImpl<ProxiedInt32RawRaster> Int32ProxiedRasterResolver;
	typedef ProxiedRasterResolverImpl<ProxiedUInt32RawRaster> UInt32ProxiedRasterResolver;
	typedef ProxiedRasterResolverImpl<ProxiedFloatRawRaster> FloatProxiedRasterResolver;
	typedef ProxiedRasterResolverImpl<ProxiedDoubleRawRaster> DoubleProxiedRasterResolver;
	typedef ProxiedRasterResolverImpl<ProxiedRgba8RawRaster> Rgba8ProxiedRasterResolver;
}

#endif  // GPLATES_PROPERTYVALUES_PROXIEDRASTERRESOLVER_H
