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
#include <boost/scoped_ptr.hpp>
#include <boost/cstdint.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QDebug>

#include "RawRaster.h"

#include "file-io/MipmappedRasterFormatReader.h"
#include "file-io/MipmappedRasterFormatWriter.h"
#include "file-io/RasterReader.h"
#include "file-io/TemporaryFileRegistry.h"

#include "gui/ColourRawRaster.h"
#include "gui/Mipmapper.h"
#include "gui/RasterColourPalette.h"

#include "utils/ReferenceCount.h"
#include "utils/non_null_intrusive_ptr.h"


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
		 * availablity of this "main" mipmpa file if @a colour_palette is either
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
		template<class RawRasterType>
		GPlatesFileIO::RasterBandReaderHandle &
		get_raster_band_reader_handle(
				RawRasterType &raster)
		{
			return raster.get_raster_band_reader_handle();
		}

		template<class RawRasterType>
		const GPlatesFileIO::RasterBandReaderHandle &
		get_raster_band_reader_handle(
				const RawRasterType &raster)
		{
			return raster.get_raster_band_reader_handle();
		}
	};


	namespace ProxiedRasterResolverInternals
	{
		QString
		make_mipmap_filename_in_same_directory(
				const QString &source_filename,
				unsigned int band_number,
				std::size_t colour_palette_id = 0);

		QString
		make_mipmap_filename_in_tmp_directory(
				const QString &source_filename,
				unsigned int band_number,
				std::size_t colour_palette_id = 0);
		
		/**
		 * Returns the filename of a file that can be used for writing out a
		 * mipmaps file for the given @a source_filename.
		 *
		 * It first checks whether a mipmap file in the same directory as the
		 * source raster is writable. If not, it will check whether a mipmap
		 * file in the temp directory is writable. In the rare case in which the
		 * user has no permissions to write in the temp directory, the empty
		 * string is returned.
		 */
		QString
		get_writable_mipmap_filename(
				const QString &source_filename,
				unsigned int band_number,
				std::size_t colour_palette_id = 0);

		/**
		 * Returns the filename of an existing mipmap file for the given
		 * @a source_filename, if any.
		 *
		 * It first checks in the same directory as the source raster. If it is
		 * not found there, it then checks in the temp directory. If the mipmaps
		 * file is not found in either of those two places, the empty string is
		 * returned.
		 */
		QString
		get_existing_mipmap_filename(
				const QString &source_filename,
				unsigned int band_number,
				std::size_t colour_palette_id = 0);

		/**
		 * Gets the colour palette id for the given @a colour_palette.
		 *
		 * It simply casts the memory address of the colour palette and casts it
		 * to an unsigned int.
		 */
		std::size_t
		get_colour_palette_id(
				const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &colour_palette =
					GPlatesGui::RasterColourPalette::create());

		/**
		 * Allows us to decompose the policies out of RawRasterType.
		 */
		template<class ProxiedRawRasterType>
		struct RawRasterTraits;
			// This is intentionally not defined anywhere.
		
		template
		<
			typename T,
			class StatisticsPolicy,
			template <class> class NoDataValuePolicy
		>
		struct RawRasterTraits<RawRasterImpl<
			T, RawRasterDataPolicies::WithProxiedData, StatisticsPolicy, NoDataValuePolicy> >
		{
			/**
			 * Basically, it takes uses the same element_type, statistics and
			 * no data value policies as ExistingRawRasterType, but swaps out
			 * the data policy to be WithData (i.e. not proxied).
			 */
			typedef RawRasterImpl
			<
				T,
				RawRasterDataPolicies::WithData,
				StatisticsPolicy,
				NoDataValuePolicy
			> with_data_type;

			typedef RawRasterImpl
			<
				T,
				RawRasterDataPolicies::WithProxiedData,
				StatisticsPolicy,
				NoDataValuePolicy
			> with_proxied_data_type;

			/**
			 * Converts a proxied raw raster to the unproxied raw raster with
			 * the same data type.
			 */
			static
			typename with_data_type::non_null_ptr_type
			convert_proxied_raster_to_unproxied_raster(
					typename with_proxied_data_type::non_null_ptr_type proxied_raster,
					unsigned int region_width,
					unsigned int region_height,
					T *data)
			{
				return with_data_type::create(
						region_width,
						region_height,
						data,
						*proxied_raster /* copy statistics policy */,
						*proxied_raster /* copy no-data policy */);
			}
		};

		/**
		 * Colours a RawRaster using a RasterColourPalette.
		 */
		boost::optional<Rgba8RawRaster::non_null_ptr_type>
		colour_raw_raster(
				RawRaster &raster,
				const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &colour_palette);

		/**
		 * Used by @a get_main_mipmap_reader and @a get_coloured_mipmap_reader,
		 * to reduce code duplication.
		 */
		template<class MipmappedRasterType>
		GPlatesFileIO::MipmappedRasterFormatReader<MipmappedRasterType> *
		get_mipmap_reader(
				boost::scoped_ptr<GPlatesFileIO::MipmappedRasterFormatReader<MipmappedRasterType> > &cached_mipmap_reader,
				std::size_t colour_palette_id,
				const GPlatesFileIO::RasterBandReaderHandle &raster_band_reader_handle,
				const boost::function<bool ()> &ensure_mipmaps_available_function,
				std::size_t *cached_colour_palette_id = NULL)
		{
			try
			{
				const QString &source_filename = raster_band_reader_handle.get_filename();

				// If it is already open, check the last modified time of
				// the source raster - in case the user modified the source
				// raster since the mipmaps file was open.
				QDateTime source_last_modified = QFileInfo(source_filename).lastModified();
				if (cached_mipmap_reader &&
						cached_mipmap_reader->get_file_info().lastModified() < source_last_modified)
				{
					// Close the mipmap reader and delete the file.
					QString mipmap_filename = cached_mipmap_reader->get_filename();
					cached_mipmap_reader.reset(NULL);
					QFile(mipmap_filename).remove();
				}

				// Open the mipmaps file if it is not already open or if it is open,
				// it is open for another colour palette.
				if (!cached_mipmap_reader ||
						(cached_colour_palette_id && *cached_colour_palette_id != colour_palette_id))
				{
					// Create the mipmaps file if not yet available.
					if (!ensure_mipmaps_available_function())
					{
						return NULL;
					}

					unsigned int source_band_number = raster_band_reader_handle.get_band_number();
					QString mipmap_filename = get_existing_mipmap_filename(
							source_filename,
							source_band_number,
							colour_palette_id);
					cached_mipmap_reader.reset(
							new GPlatesFileIO::MipmappedRasterFormatReader<MipmappedRasterType>(
								mipmap_filename));

					// Check the last modified time of the source raster.
					// If the user has modified the source raster since the
					// mipmaps file was created, close the reader, delete
					// the file and attempt to open it again.
					if (cached_mipmap_reader->get_file_info().lastModified() < source_last_modified)
					{
						cached_mipmap_reader.reset(NULL);
						QFile(mipmap_filename).remove();

						if (!ensure_mipmaps_available_function())
						{
							return NULL;
						}

						cached_mipmap_reader.reset(
								new GPlatesFileIO::MipmappedRasterFormatReader<MipmappedRasterType>(
									mipmap_filename));

						// Check again, but give up if it's still wrong.
						if (cached_mipmap_reader->get_file_info().lastModified() < source_last_modified)
						{
							cached_mipmap_reader.reset(NULL);
						}
					}

					if (cached_colour_palette_id)
					{
						*cached_colour_palette_id = colour_palette_id;
					}
				}

				return cached_mipmap_reader.get();
			}
			catch (...)
			{
				return NULL;
			}
		}

		/**
		 * BaseProxiedRasterResolver resolves proxied rasters, but only using
		 * the "main", not-colour-palette-specific, mipmap file.
		 *
		 * As such, it does everything correctly, except for the case of integer
		 * rasters with integer colour palettes.
		 */
		template<class RawRasterType>
		class BaseProxiedRasterResolver :
				public ProxiedRasterResolver
		{
		public:

			/**
			 * This is the type of raw raster that can be read from the source raster file.
			 */
			typedef typename RawRasterTraits<RawRasterType>::with_data_type source_raster_type;

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
					// Colour the region_raster using the colour_palette.
					boost::optional<Rgba8RawRaster::non_null_ptr_type> coloured_region =
						colour_raw_raster(*region_raster, colour_palette);
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
				return ensure_main_mipmaps_available();
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
				GPlatesFileIO::RasterBandReaderHandle &raster_band_reader_handle =
					get_raster_band_reader_handle(*d_proxied_raw_raster);

				// Check that the raster band can offer us the correct data type.
				typedef typename RawRasterType::element_type element_type;
				if (raster_band_reader_handle.get_type() != RasterType::get_type_as_enum<element_type>())
				{
					return boost::none;
				}

				element_type *raster_data = reinterpret_cast<element_type *>(raster_band_reader_handle.get_data(
						QRect(region_x_offset, region_y_offset, region_width, region_height)));
				if (raster_data)
				{
					return RawRasterTraits<RawRasterType>::convert_proxied_raster_to_unproxied_raster(
							d_proxied_raw_raster,
							region_width,
							region_height,
							raster_data);
				}
				else
				{
					return boost::none;
				}
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
					const typename RawRasterType::non_null_ptr_type &raster) :
				d_proxied_raw_raster(raster),
				d_main_mipmap_reader(NULL)
			{  }

			~BaseProxiedRasterResolver()
			{  }

			typename RawRasterType::non_null_ptr_type d_proxied_raw_raster;

		private:

			bool
			ensure_main_mipmaps_available()
			{
				GPlatesFileIO::RasterBandReaderHandle raster_band_reader_handle =
					get_raster_band_reader_handle(*d_proxied_raw_raster);

				const QString &filename = raster_band_reader_handle.get_filename();
				unsigned int band_number = raster_band_reader_handle.get_band_number();

				if (!get_existing_mipmap_filename(filename, band_number).isEmpty())
				{
					// Mipmap file already exists.
					return true;
				}

				QString mipmap_filename = get_writable_mipmap_filename(filename, band_number);

				if (mipmap_filename.isEmpty())
				{
					// Can't write mipmap file anywhere.
					return false;
				}

				// Check the type of the source raster band.
				typedef typename RawRasterType::element_type element_type;
				if (raster_band_reader_handle.get_type() != RasterType::get_type_as_enum<element_type>())
				{
					return false;
				}

				// Read the entire source raster band into memory.
				boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type> source_raster =
					raster_band_reader_handle.get_raw_raster();
				if (!source_raster)
				{
					// Can't read source raster band.
					return false;
				}

				// Write the mipmap file.
				try
				{
					GPlatesFileIO::MipmappedRasterFormatWriter<GPlatesPropertyValues::RawRaster> writer;
					writer.write(*source_raster, mipmap_filename);

					// Copy the file permissions from the source raster file to the mipmap file.
					QFile::setPermissions(mipmap_filename, QFile::permissions(filename));

					return true;
				}
				catch (...)
				{
					return false;
				}
			}

			/**
			 * Returns a pointer to the mipmap reader for the main mipmap file, after
			 * checking that the file exists.
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
				return ProxiedRasterResolverInternals::get_mipmap_reader(
						d_main_mipmap_reader,
						0 /* no colour palette id */,
						get_raster_band_reader_handle(*d_proxied_raw_raster),
						boost::bind(
							&BaseProxiedRasterResolver::ensure_main_mipmaps_available,
							boost::ref(*this)),
						NULL /* no colour palette id */);
			}

			// Cached so that we don't have to open and close it all the time.
			boost::scoped_ptr<GPlatesFileIO::MipmappedRasterFormatReader<mipmapped_raster_type> >
				d_main_mipmap_reader;
		};
	}


	template<class RawRasterType, class Enable = void>
	class ProxiedRasterResolverImpl;


	// Specialisation where RawRasterType has proxied data that is not integral.
	template<class RawRasterType>
	class ProxiedRasterResolverImpl<RawRasterType,
		typename boost::enable_if_c<RawRasterType::has_proxied_data &&
		!boost::is_integral<typename RawRasterType::element_type>::value>::type> :
			public ProxiedRasterResolverInternals::BaseProxiedRasterResolver<RawRasterType>
	{
	public:

		typedef ProxiedRasterResolverImpl<RawRasterType> this_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<this_type> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const this_type> non_null_ptr_to_const_type;

		static
		non_null_ptr_type
		create(
				const typename RawRasterType::non_null_ptr_type &raster)
		{
			return new ProxiedRasterResolverImpl(raster);
		}

	private:

		typedef ProxiedRasterResolverInternals::BaseProxiedRasterResolver<RawRasterType> base_type;

		ProxiedRasterResolverImpl(
				const typename RawRasterType::non_null_ptr_type &raster) :
			base_type(raster)
		{  }
	};


	// Specialisation where RawRasterType has proxied data that is integral.
	template<class RawRasterType>
	class ProxiedRasterResolverImpl<RawRasterType,
		typename boost::enable_if_c<RawRasterType::has_proxied_data &&
		boost::is_integral<typename RawRasterType::element_type>::value>::type> :
			public ProxiedRasterResolverInternals::BaseProxiedRasterResolver<RawRasterType>
	{
	public:

		typedef ProxiedRasterResolverImpl<RawRasterType> this_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<this_type> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const this_type> non_null_ptr_to_const_type;

		static
		non_null_ptr_type
		create(
				const typename RawRasterType::non_null_ptr_type &raster)
		{
			return new ProxiedRasterResolverImpl(raster);
		}

	private:

		typedef ProxiedRasterResolverInternals::BaseProxiedRasterResolver<RawRasterType> base_type;
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

				return ProxiedRasterResolverInternals::colour_raw_raster(
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
				return base_type::ensure_mipmaps_available();
			}
			else
			{
				return ensure_coloured_mipmaps_available(colour_palette);
			}
		}

	private:

		ProxiedRasterResolverImpl(
				const typename RawRasterType::non_null_ptr_type &raster) :
			base_type(raster),
			d_coloured_mipmap_reader(NULL),
			d_colour_palette_id_of_coloured_mipmap_reader(0)
		{  }

		bool
		ensure_coloured_mipmaps_available(
				const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &colour_palette)
		{
			GPlatesFileIO::RasterBandReaderHandle raster_band_reader_handle =
					get_raster_band_reader_handle(*d_proxied_raw_raster);

			GPlatesGui::RasterColourPaletteType::Type colour_palette_type =
				GPlatesGui::RasterColourPaletteType::get_type(*colour_palette);
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					colour_palette_type == GPlatesGui::RasterColourPaletteType::INT32 ||
					colour_palette_type == GPlatesGui::RasterColourPaletteType::UINT32,
					GPLATES_ASSERTION_SOURCE);

			const QString &filename = raster_band_reader_handle.get_filename();
			unsigned int band_number = raster_band_reader_handle.get_band_number();
			std::size_t colour_palette_id = ProxiedRasterResolverInternals::get_colour_palette_id(colour_palette);

			if (!ProxiedRasterResolverInternals::get_existing_mipmap_filename(
				filename, band_number, colour_palette_id).isEmpty())
			{
				// Mipmap file already exists.
				return true;
			}

			QString mipmap_filename = ProxiedRasterResolverInternals::get_writable_mipmap_filename(
				filename, band_number, colour_palette_id);

			if (mipmap_filename.isEmpty())
			{
				// Can't write mipmap file anywhere.
				return false;
			}

			// Check the type of the source raster band.
			typedef typename source_raster_type::element_type element_type;
			if (raster_band_reader_handle.get_type() != RasterType::get_type_as_enum<element_type>())
			{
				return false;
			}

			// Read the entire source raster band into memory.
			boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type> source_raster =
				raster_band_reader_handle.get_raw_raster();
			if (!source_raster)
			{
				// Can't read source raster band.
				return false;
			}

			// Convert the source raster band into RGBA8 using the colour palette.
			boost::optional<Rgba8RawRaster::non_null_ptr_type> coloured_raster =
				ProxiedRasterResolverInternals::colour_raw_raster(**source_raster, colour_palette);
			if (!coloured_raster)
			{
				return false;
			}

			// Write the mipmap file.
			try
			{
				GPlatesFileIO::MipmappedRasterFormatWriter<Rgba8RawRaster> writer;
				writer.write(*coloured_raster, mipmap_filename);

				// The coloured mipmap files used by integer rasters with integer colour
				// palettes are deleted when GPlates exits.
				// This is because they are created specifically for particular colour
				// palettes, indexed by their memory address, which of course does not
				// remain the same the next time GPlates gets run.
				GPlatesFileIO::TemporaryFileRegistry::instance().add_file(mipmap_filename);
				
				// Make sure the file is only readable and writable by the user.
				// Suppose the source raster file is on a shared directory that happens to
				// be global writable, and two users are running two instances of GPlates.
				// It makes no sense for the second user to use the coloured mipmap file
				// generated by the first user; the colour palette id, being generated
				// from the memory address, is unlikely to mean the same thing in the
				// second instance of GPlates.
				// 
				// Note: this should change if we start hashing colour palettes, though.
				QFile::setPermissions(mipmap_filename, QFile::ReadUser | QFile::WriteUser);

				return true;
			}
			catch (...)
			{
				return false;
			}
		}

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
			return ProxiedRasterResolverInternals::get_mipmap_reader<Rgba8RawRaster>(
					d_coloured_mipmap_reader,
					ProxiedRasterResolverInternals::get_colour_palette_id(colour_palette),
					get_raster_band_reader_handle(*d_proxied_raw_raster),
					boost::bind(
						&ProxiedRasterResolverImpl::ensure_coloured_mipmaps_available,
						boost::ref(*this),
						colour_palette),
					&d_colour_palette_id_of_coloured_mipmap_reader);
		}

		boost::scoped_ptr<GPlatesFileIO::MipmappedRasterFormatReader<Rgba8RawRaster> >
			d_coloured_mipmap_reader;
		std::size_t d_colour_palette_id_of_coloured_mipmap_reader;

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
