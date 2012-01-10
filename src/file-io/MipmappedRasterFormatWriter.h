/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
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

#ifndef GPLATES_FILEIO_MIPMAPPEDRASTERFORMATWRITER_H
#define GPLATES_FILEIO_MIPMAPPEDRASTERFORMATWRITER_H

#include <ostream>
#include <string>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/utility/enable_if.hpp>
#include <QString>
#include <QFile>
#include <QDataStream>
#include <QDebug>

#include "ErrorOpeningFileForWritingException.h"
#include "RasterFileCacheFormat.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesException.h"
#include "global/GPlatesAssert.h"
#include "global/LogException.h"

#include "gui/ColourRawRaster.h"
#include "gui/Mipmapper.h"
#include "gui/RasterColourPalette.h"

#include "property-values/RawRaster.h"
#include "property-values/RawRasterUtils.h"

#include "utils/Profile.h"


namespace GPlatesFileIO
{
	namespace MipmappedRasterFormatWriterInternals
	{
		template<typename T>
		void
		write(
				QDataStream &out,
				const T *data,
				unsigned int len)
		{
			const T *end = data + len;
			while (data != end)
			{
				out << *data;
				++data;
			}
		}

		template <class ProxiedRawRasterType, class MipmapperType>
		class BaseMipmappedRasterFormatWriter
		{
		public:
			typedef typename ProxiedRawRasterType::element_type proxied_raster_element_type;
			typedef typename GPlatesPropertyValues::RawRasterUtils
					::ConvertProxiedRasterToUnproxiedRaster<ProxiedRawRasterType>::unproxied_raster_type
							source_raster_type;
			typedef typename source_raster_type::element_type source_raster_element_type;
			typedef MipmapperType mipmapper_type;
			typedef typename mipmapper_type::output_raster_type mipmapped_raster_type;
			typedef typename mipmapped_raster_type::element_type mipmapped_element_type;
			typedef GPlatesPropertyValues::CoverageRawRaster coverage_raster_type;
			typedef coverage_raster_type::element_type coverage_element_type;


			BaseMipmappedRasterFormatWriter(
					typename ProxiedRawRasterType::non_null_ptr_type proxied_raw_raster,
					GPlatesFileIO::RasterBandReaderHandle &source_raster_band_reader_handle) :
				d_proxied_raw_raster(proxied_raw_raster),
				d_source_raster_band_reader_handle(source_raster_band_reader_handle),
				d_source_raster_width(proxied_raw_raster->width()),
				d_source_raster_height(proxied_raw_raster->height())
			{
				// Check that the raster band can offer us the correct data type.
				// This should always be true - if it's not then it's a program error so
				// throw an assertion in release builds and abort in debug builds.
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						d_source_raster_band_reader_handle.get_type() ==
								GPlatesPropertyValues::RasterType::get_type_as_enum<
									proxied_raster_element_type>(),
						GPLATES_ASSERTION_SOURCE);

				d_num_levels = GPlatesFileIO::RasterFileCacheFormat::get_number_of_mipmapped_levels(
						d_source_raster_width, d_source_raster_height);

				// Find out the maximum memory usage required by a mipmapper to mipmap.
				const float mipmapper_allocation_amplification =
						mipmapper_type::get_max_memory_bytes_amplification_required_to_mipmap();

				// If the memory used is bigger than this then mipmap it in sections
				// to avoid the potential for a memory allocation failure.
				const unsigned int MAX_MEMORY_TO_USE_IN_BYTES = 500 * 1000 * 1000;

				// The number of source rows that we mipmap in one pass must be at least
				// a multiple of this number (or the height of the entire source raster).
				// This is because each pixel in the lowest resolution mipmap needs to be
				// covered by the exact number of source raster pixels that result in the same
				// height as the mipmap pixel (except for the bottom-most section of the raster
				// which won't necessarily be a power-of-two).
				// NOTE: Here we are assuming a mipmap filter region of 2x2.
				// This means the Lanczos is no longer being used.
				// TODO: This is temporary and later the Lanczos filter should be added again
				// for both RGBA and GDAL rasters when the quad-tree mipmap arrangement is
				// implemented to minimise disk seeks.
				const unsigned int num_source_rows_multiple = (1 << d_num_levels);

				// Determine the number of rows in source raster to mipmap at a time.
				d_max_source_rows_per_mipmap_pass = static_cast<unsigned int>(
						MAX_MEMORY_TO_USE_IN_BYTES /
						(
							d_source_raster_width *
							(
								sizeof(source_raster_element_type) +
								mipmapper_allocation_amplification
							)
						)
					);
				// Make sure 'max_source_rows_per_mipmap_pass' is a multiple of a power-of-two.
				d_max_source_rows_per_mipmap_pass /= num_source_rows_multiple;
				d_max_source_rows_per_mipmap_pass *= num_source_rows_multiple;

				if (d_max_source_rows_per_mipmap_pass == 0)
				{
					d_max_source_rows_per_mipmap_pass = num_source_rows_multiple;
				}
			}


			/**
			 * Creates mipmaps and writes a Mipmapped Raster Format file at @a filename.
			 *
			 * Throws @a ErrorOpeningFileForWritingException if the file could not
			 * be opened for writing.
			 */
			void
			write(
					const QString &filename)
			{
				PROFILE_FUNC();

				// Open the file for writing.
				QFile file(filename);
				if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
				{
					throw ErrorOpeningFileForWritingException(
							GPLATES_EXCEPTION_SOURCE, filename);
				}
				QDataStream out(&file);

				// Write magic number/string.
				for (unsigned int n = 0; n < sizeof(RasterFileCacheFormat::MAGIC_NUMBER); ++n)
				{
					out << static_cast<quint8>(RasterFileCacheFormat::MAGIC_NUMBER[n]);
				}
				// Write version number.
				out << static_cast<quint32>(RasterFileCacheFormat::VERSION_NUMBER);
				out.setVersion(RasterFileCacheFormat::Q_DATA_STREAM_VERSION);

				// Write mipmap type.
				out << static_cast<quint32>(RasterFileCacheFormat::get_type_as_enum<mipmapped_element_type>());

				// Write number of levels.
				out << static_cast<quint32>(d_num_levels);

				// If there's no mipmap levels then nothing left to do.
				if (d_num_levels == 0)
				{
					file.close();
					return;
				}

				const qint64 level_info_pos = file.pos();

				const bool generate_coverage = should_generate_coverage();

				// Get information about each mipmap level that we'll write later.
				const std::vector<typename mipmapper_type::LevelInfo> mipmapper_level_infos =
						mipmapper_type::get_level_infos(
								RasterFileCacheFormat::BLOCK_SIZE,
								d_source_raster_width,
								d_source_raster_height,
								generate_coverage);
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						mipmapper_level_infos.size() == d_num_levels,
						GPLATES_ASSERTION_SOURCE);

				// The total number of bytes if mipmapped raster data that will get written
				// to the file - this does not include any header information - just the raw data.
				qint64 total_bytes_of_mipmapped_raster_data = 0;

				// Determine the level infos to write to the mipmap file.
				std::vector<RasterFileCacheFormat::LevelInfo> level_infos;
				{
					// Number of bytes written for each 'RasterFileCacheFormat::LevelInfo'.
					static const qint64 LEVEL_INFO_SIZE = sizeof(RasterFileCacheFormat::LevelInfo);

					// Start at the file offset which is the beginning of raw mipmapped data.
					// So need to skip past the level infos written to the file.
					const qint64 mipmapped_data_file_offset =
							level_info_pos + LEVEL_INFO_SIZE * d_num_levels;

					qint64 file_offset = mipmapped_data_file_offset;
					for (unsigned int level = 0; level < d_num_levels; ++level)
					{
						RasterFileCacheFormat::LevelInfo level_info;
						level_info.width = mipmapper_level_infos[level].width;
						level_info.height = mipmapper_level_infos[level].height;

						level_info.main_offset = file_offset;
						file_offset += mipmapper_level_infos[level].num_bytes_main_mipmap;

						if (generate_coverage)
						{
							level_info.coverage_offset = file_offset;
							file_offset += mipmapper_level_infos[level].num_bytes_coverage_mipmap;
						}
						else
						{
							level_info.coverage_offset = 0;
						}

						level_infos.push_back(level_info);
					}

					total_bytes_of_mipmapped_raster_data = file_offset - mipmapped_data_file_offset;
				}

				// Write out the level infos.
				typedef std::vector<RasterFileCacheFormat::LevelInfo>::const_iterator level_iterator_type;
				for (level_iterator_type iter = level_infos.begin(); iter != level_infos.end(); ++iter)
				{
					const RasterFileCacheFormat::LevelInfo &current_level_info = *iter;
					out << current_level_info.width
						<< current_level_info.height
						<< current_level_info.main_offset
						<< current_level_info.coverage_offset;
#if 0
					qDebug() << "RasterFileCacheFormat::LevelInfo: "
							<< current_level_info.width << " "
							<< current_level_info.height << " "
							<< current_level_info.main_offset << " "
							<< current_level_info.coverage_offset;
#endif
				}

				// Set up some working file offsets to keep track of where in the file
				// our partial mipmap writes should go - this is for those situations where
				// each mipmap is written in multiple passes to avoid memory allocation failure.
				std::vector<qint64> partial_main_mipmap_offsets;
				std::vector<qint64> partial_coverage_mipmap_offsets;
				for (unsigned int level = 0; level < d_num_levels; ++level)
				{
					partial_main_mipmap_offsets.push_back(level_infos[level].main_offset);
					partial_coverage_mipmap_offsets.push_back(level_infos[level].coverage_offset);
				}

				// Are we doing one pass to generate mipmaps or multiple?
				const bool single_mipmap_pass = (d_source_raster_height <= d_max_source_rows_per_mipmap_pass);
				if (!single_mipmap_pass)
				{
					// We need to be able to seek around the file as we write partial pieces
					// of the mipmap levels.
					// But we cannot seek to an area of the file that doesn't exist yet.
					// Easiest way to handle this is to write zeros to the entire file first.
					// And then go back and overwrite.
					const int NUM_BYTES_WRITE_BUFFER = 1024 * 1024;
					const QByteArray write_buffer(NUM_BYTES_WRITE_BUFFER, 0);
					qint64 num_zeros_left_to_write = total_bytes_of_mipmapped_raster_data;
					while (num_zeros_left_to_write > 0)
					{
						const int num_zeros_to_write =
								(num_zeros_left_to_write > NUM_BYTES_WRITE_BUFFER)
								? NUM_BYTES_WRITE_BUFFER
								: static_cast<int>(num_zeros_left_to_write);
						out.writeRawData(
								write_buffer.constData(),
								num_zeros_to_write);
						num_zeros_left_to_write -= num_zeros_to_write;
					}
				}

				// Iterate over the mipmap passes to generate the mipmaps.
				unsigned int y = 0;
				for (unsigned int mipmap_pass = 0; y < d_source_raster_height; ++mipmap_pass)
				{
					// Note that the last section of the source raster will have a number of rows
					// that are not necessarily a power-of-two.
					// That's OK because that's expected for the last section - we just don't
					// want to do that for prior sections as it will give the wrong result.
					unsigned int num_source_rows_to_mipmap = d_source_raster_height - y;
					if (num_source_rows_to_mipmap > d_max_source_rows_per_mipmap_pass)
					{
						num_source_rows_to_mipmap = d_max_source_rows_per_mipmap_pass;
					}

					// The source region we are going to mipmap.
					const QRect source_region_rect(0, y, d_source_raster_width, num_source_rows_to_mipmap);

					// Get the region data from the source raster.
					PROFILE_BEGIN(get_src_data, "get source region data");
					source_raster_element_type *source_region_data = reinterpret_cast<source_raster_element_type *>(
							d_source_raster_band_reader_handle.get_data(source_region_rect));
					PROFILE_END(get_src_data);
					if (source_region_data == NULL)
					{
						throw GPlatesGlobal::LogException(
								GPLATES_EXCEPTION_SOURCE,
								"Unable to read source raster region.");
					}

					// Store the region data in a source raster type and
					// retain the statistics/no-data-value of the proxied raster.
					typename source_raster_type::non_null_ptr_type source_region_raster =
							GPlatesPropertyValues::RawRasterUtils::convert_proxied_raster_to_unproxied_raster<
									ProxiedRawRasterType>(
											d_proxied_raw_raster,
											source_region_rect.width(),
											source_region_rect.height(),
											source_region_data);

					// Get derived class to create the mipmapper.
					boost::shared_ptr<mipmapper_type> mipmapper = create_mipmapper(
							source_region_raster,
							generate_coverage);
					if (!mipmapper)
					{
						throw GPlatesGlobal::LogException(
								GPLATES_EXCEPTION_SOURCE,
								"Unable to create mipmapper.");
					}

					for (unsigned int level = 0; level < d_num_levels; ++level)
					{
						mipmapper->generate_next();

						const RasterFileCacheFormat::LevelInfo &current_level_info = level_infos[level];

						// Get and write the mipmap for the current level.
						typename mipmapped_raster_type::non_null_ptr_to_const_type current_mipmap =
							mipmapper->get_current_mipmap();

						// This should always be true - if it's not then it's a program error so
						// throw an assertion in release builds and abort in debug builds.
						GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
								current_mipmap->width() == current_level_info.width,
								GPLATES_ASSERTION_SOURCE);

						// Seek to the subsection of the current mipmap that's appropriate for
						// the partial mipmap we are writing.
						if (!single_mipmap_pass)
						{
							file.seek(partial_main_mipmap_offsets[level]);
						}

						// Possible partial write of current main mipmap to the file.
						MipmappedRasterFormatWriterInternals::write<mipmapped_element_type>(
							out,
							current_mipmap->data(),
							current_mipmap->width() * current_mipmap->height());

						// Update the file offset for the current mipmap for the next partial write.
						partial_main_mipmap_offsets[level] +=
								current_mipmap->width() * current_mipmap->height() *
									sizeof(mipmapped_element_type);

						// Get and write the associated coverage raster.
						boost::optional<coverage_raster_type::non_null_ptr_to_const_type> current_coverage =
							mipmapper->get_current_coverage();

						// This should always be true - if it's not then it's a program error so
						// throw an assertion in release builds and abort in debug builds.
						GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
								generate_coverage == static_cast<bool>(current_coverage),
								GPLATES_ASSERTION_SOURCE);
						if (generate_coverage)
						{
							// This should always be true - if it's not then it's a program error so
							// throw an assertion in release builds and abort in debug builds.
							GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
									current_coverage.get()->width() == current_level_info.width,
									GPLATES_ASSERTION_SOURCE);

							// Seek to the subsection of the current coverage mipmap that's
							// appropriate for the partial coverage mipmap we are writing.
							if (!single_mipmap_pass)
							{
								file.seek(partial_coverage_mipmap_offsets[level]);
							}

							// Possible partial write of current coverage mipmap to the file.
							MipmappedRasterFormatWriterInternals::write<coverage_element_type>(
								out,
								current_coverage.get()->data(),
								current_coverage.get()->width() * current_coverage.get()->height());

							// Update the file offset for the current mipmap for the next partial write.
							partial_coverage_mipmap_offsets[level] +=
									current_coverage.get()->width() * current_coverage.get()->height() *
										sizeof(coverage_element_type);
						}
					}

					// Move to start of the next group of source rows to mipmap.
					y += num_source_rows_to_mipmap;
				}

				file.close();
			}

		protected:
			virtual
			~BaseMipmappedRasterFormatWriter()
			{  }

			/**
			 * Create a mipmapper from the specified source raster.
			 */
			virtual
			boost::shared_ptr<mipmapper_type>
			create_mipmapper(
					const typename source_raster_type::non_null_ptr_type &source_region_raster,
					bool generate_coverage) = 0;

			/**
			 * Returns true if a coverage should be generated for the raster type (handled by derived class).
			 *
			 * Only generated for non-RGBA rasters regardless of whether the raster actually contains
			 * a sentinel pixel (ie, a pixel with the no-data or sentinel value).
			 * If a raster type supports sentinel values (ie, non-RGBA) but has no sentinel values
			 * in the raster then the coverage raster will compress very well so the extra space
			 * used should be small (and it saves us having to do a full pass over the raster just
			 * to see if it contains even a single sentinel pixel).
			 */
			virtual
			bool
			should_generate_coverage() const = 0;


			typename ProxiedRawRasterType::non_null_ptr_type d_proxied_raw_raster;
			GPlatesFileIO::RasterBandReaderHandle &d_source_raster_band_reader_handle;
			unsigned int d_source_raster_width;
			unsigned int d_source_raster_height;
			unsigned int d_num_levels;
			unsigned int d_max_source_rows_per_mipmap_pass;
		};
	}


	/**
	 * Mipmapper takes a raster of type RawRasterType and produces a sequence of
	 * mipmaps of successively smaller size.
	 *
	 * For the public interface of the various MipmappedRasterFormatWriter template specialisations,
	 * @see MipmappedRasterFormatWriterInternals::BaseMipmappedRasterFormatWriter.
	 * In addition, all specialisations have a public constructor that takes a
	 * - ProxiedRawRasterType::non_null_ptr_to_const_type
	 * - GPlatesFileIO::RasterBandReaderHandle
	 *
	 * Note that the 'use_colour_palette' template parameter only applies to integer rasters.
	 */
	template<class ProxiedRawRasterType, bool use_colour_palette = false, class Enable = void>
	class MipmappedRasterFormatWriter;
		// This is intentionally not defined.


	/**
	 * This specialisation is for rasters that have an element_type of rgba8_t and are without a no-data value.
	 */
	template<class ProxiedRawRasterType>
	class MipmappedRasterFormatWriter<
		ProxiedRawRasterType,
		false/*use_colour_palette*/,  // Not applicable here
		typename boost::enable_if_c<!ProxiedRawRasterType::has_no_data_value &&
			boost::is_same<typename ProxiedRawRasterType::element_type, GPlatesGui::rgba8_t>::value>::type
	> :
			public MipmappedRasterFormatWriterInternals::BaseMipmappedRasterFormatWriter<
					ProxiedRawRasterType, GPlatesGui::Mipmapper<GPlatesPropertyValues::Rgba8RawRaster> >
	{
	private:
		typedef MipmappedRasterFormatWriterInternals::BaseMipmappedRasterFormatWriter<
					ProxiedRawRasterType, GPlatesGui::Mipmapper<GPlatesPropertyValues::Rgba8RawRaster> >
							base_type;
		typedef typename base_type::mipmapper_type mipmapper_type;
		typedef typename base_type::source_raster_type source_raster_type;

	public:
		/**
		 * @see MipmappedRasterFormatWriterInternals::BaseMipmappedRasterFormatWriter::BaseMipmappedRasterFormatWriter.
		 */
		MipmappedRasterFormatWriter(
				typename ProxiedRawRasterType::non_null_ptr_type proxied_raw_raster,
				GPlatesFileIO::RasterBandReaderHandle &source_raster_band_reader_handle) :
			base_type(
					proxied_raw_raster,
					source_raster_band_reader_handle)
		{
		}

	private:
		virtual
		boost::shared_ptr<mipmapper_type>
		create_mipmapper(
				const typename source_raster_type::non_null_ptr_type &source_region_raster,
				bool generate_coverage)
		{
			return boost::shared_ptr<mipmapper_type>(
					new mipmapper_type(source_region_raster));
		}

		virtual
		bool
		should_generate_coverage() const
		{
			// The source region will be RGBA and there's no need
			// to generate a coverage for a RGBA raster.
			return false;
		}
	};


	/**
	 * This specialisation is for rasters that have a floating-point element_type and that have a no-data value.
	 */
	template<class ProxiedRawRasterType>
	class MipmappedRasterFormatWriter<
		ProxiedRawRasterType,
		false/*use_colour_palette*/,  // Not applicable here
		typename boost::enable_if_c<ProxiedRawRasterType::has_no_data_value &&
			boost::is_floating_point<typename ProxiedRawRasterType::element_type>::value >::type
	> :
			public MipmappedRasterFormatWriterInternals::BaseMipmappedRasterFormatWriter<
					ProxiedRawRasterType, GPlatesGui::Mipmapper<
							typename GPlatesPropertyValues::RawRasterUtils
									::ConvertProxiedRasterToUnproxiedRaster<ProxiedRawRasterType>
											::unproxied_raster_type> >
	{
	private:
		typedef MipmappedRasterFormatWriterInternals::BaseMipmappedRasterFormatWriter<
					ProxiedRawRasterType, GPlatesGui::Mipmapper<
							typename GPlatesPropertyValues::RawRasterUtils
									::ConvertProxiedRasterToUnproxiedRaster<ProxiedRawRasterType>
											::unproxied_raster_type> >
							base_type;
		typedef typename base_type::mipmapper_type mipmapper_type;
		typedef typename base_type::source_raster_type source_raster_type;
		typedef typename base_type::source_raster_element_type source_raster_element_type;

	public:
		/**
		 * @see MipmappedRasterFormatWriterInternals::BaseMipmappedRasterFormatWriter::BaseMipmappedRasterFormatWriter.
		 */
		MipmappedRasterFormatWriter(
				typename ProxiedRawRasterType::non_null_ptr_type proxied_raw_raster,
				GPlatesFileIO::RasterBandReaderHandle &source_raster_band_reader_handle) :
			base_type(
					proxied_raw_raster,
					source_raster_band_reader_handle)
		{
		}

	private:
		virtual
		boost::shared_ptr<mipmapper_type>
		create_mipmapper(
				const typename source_raster_type::non_null_ptr_type &source_region_raster,
				bool generate_coverage)
		{
			return boost::shared_ptr<mipmapper_type>(
					new mipmapper_type(source_region_raster, generate_coverage));
		}

		virtual
		bool
		should_generate_coverage() const
		{
			// The source region will support no-data values.
			return true;
		}

		using base_type::d_proxied_raw_raster;
		using base_type::d_source_raster_band_reader_handle;
	};


	/**
	 * This specialisation is for rasters that have a integer element_type
	 * and that have a no-data value and that do *not* convert to RGBA (using a colour palette)
	 * before mipmapping - in other words it gets mipmapped as a float raster.
	 * The int-to-float conversion actually gets handled by the integer template specialisation
	 * of the Mipmapper class.
	 */
	template<class ProxiedRawRasterType>
	class MipmappedRasterFormatWriter<
		ProxiedRawRasterType,
		false/*use_colour_palette*/,
		typename boost::enable_if_c<ProxiedRawRasterType::has_no_data_value &&
			boost::is_integral<typename ProxiedRawRasterType::element_type>::value >::type
	> :
			public MipmappedRasterFormatWriterInternals::BaseMipmappedRasterFormatWriter<
					ProxiedRawRasterType, GPlatesGui::Mipmapper<
							typename GPlatesPropertyValues::RawRasterUtils
									::ConvertProxiedRasterToUnproxiedRaster<ProxiedRawRasterType>
											::unproxied_raster_type> >
	{
	private:
		typedef MipmappedRasterFormatWriterInternals::BaseMipmappedRasterFormatWriter<
					ProxiedRawRasterType, GPlatesGui::Mipmapper<
							typename GPlatesPropertyValues::RawRasterUtils
									::ConvertProxiedRasterToUnproxiedRaster<ProxiedRawRasterType>
											::unproxied_raster_type> >
							base_type;
		typedef typename base_type::mipmapper_type mipmapper_type;
		typedef typename base_type::source_raster_type source_raster_type;
		typedef typename base_type::source_raster_element_type source_raster_element_type;

	public:
		/**
		 * @see MipmappedRasterFormatWriterInternals::BaseMipmappedRasterFormatWriter::BaseMipmappedRasterFormatWriter.
		 */
		MipmappedRasterFormatWriter(
				typename ProxiedRawRasterType::non_null_ptr_type proxied_raw_raster,
				GPlatesFileIO::RasterBandReaderHandle &source_raster_band_reader_handle) :
			base_type(
					proxied_raw_raster,
					source_raster_band_reader_handle)
		{
		}

	private:
		virtual
		boost::shared_ptr<mipmapper_type>
		create_mipmapper(
				const typename source_raster_type::non_null_ptr_type &source_region_raster,
				bool generate_coverage)
		{
			return boost::shared_ptr<mipmapper_type>(
					new mipmapper_type(source_region_raster, generate_coverage));
		}

		virtual
		bool
		should_generate_coverage() const
		{
			// The source region will support no-data values.
			return true;
		}

		using base_type::d_proxied_raw_raster;
		using base_type::d_source_raster_band_reader_handle;
	};


	/**
	 * This specialisation is for rasters that have a integer element_type
	 * and that have a no-data value and that convert to RGBA (using a colour palette)
	 * before mipmapping.
	 */
	template<class ProxiedRawRasterType>
	class MipmappedRasterFormatWriter<
		ProxiedRawRasterType,
		true/*use_colour_palette*/,
		typename boost::enable_if_c<ProxiedRawRasterType::has_no_data_value &&
			boost::is_integral<typename ProxiedRawRasterType::element_type>::value >::type
	> :
			public MipmappedRasterFormatWriterInternals::BaseMipmappedRasterFormatWriter<
					ProxiedRawRasterType, GPlatesGui::Mipmapper<GPlatesPropertyValues::Rgba8RawRaster> >
	{
	private:
		// NOTE: The type passed to the mipmapper is Rgba8RawRaster and *not* an integer raster.
		// This is because the integer raster gets converted to RGBA using a colour palette
		// before it's mipmapped.
		typedef MipmappedRasterFormatWriterInternals::BaseMipmappedRasterFormatWriter<
					ProxiedRawRasterType, GPlatesGui::Mipmapper<GPlatesPropertyValues::Rgba8RawRaster> >
							base_type;
		typedef typename base_type::mipmapper_type mipmapper_type;
		typedef typename base_type::source_raster_type source_raster_type;

	public:
		/**
		 * @see MipmappedRasterFormatWriterInternals::BaseMipmappedRasterFormatWriter::BaseMipmappedRasterFormatWriter.
		 */
		MipmappedRasterFormatWriter(
				typename ProxiedRawRasterType::non_null_ptr_type proxied_raw_raster,
				GPlatesFileIO::RasterBandReaderHandle &source_raster_band_reader_handle,
				const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &colour_palette) :
			base_type(
					proxied_raw_raster,
					source_raster_band_reader_handle),
			d_colour_palette(colour_palette)
		{
		}

	private:
		virtual
		boost::shared_ptr<mipmapper_type>
		create_mipmapper(
				const typename source_raster_type::non_null_ptr_type &source_region_raster,
				bool /*generate_coverage*/)
		{
			// Convert the source raster band into RGBA8 using our colour palette.
			boost::optional<GPlatesPropertyValues::Rgba8RawRaster::non_null_ptr_type> coloured_raster =
					GPlatesGui::ColourRawRaster::colour_raw_raster_with_raster_colour_palette(
							*source_region_raster,
							d_colour_palette);
			if (!coloured_raster)
			{
				return boost::shared_ptr<mipmapper_type>();
			}

			// Mipmap the coloured raster not the integer source raster.
			return boost::shared_ptr<mipmapper_type>(
					new mipmapper_type(coloured_raster.get()));
		}

		virtual
		bool
		should_generate_coverage() const
		{
			// The source region will be coloured/converted to RGBA and there's no need
			// to generate a coverage for a RGBA raster.
			return false;
		}

	private:
		GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type d_colour_palette;
	};


	/**
	 * Creates a @a MipmappedRasterFormatWriter of the appropriate type.
	 *
	 * This can also be used for integer rasters than should be mipmapped like a float raster
	 * (ie, not coloured first and then mipmapped as RGBA).
	 */
	template <class ProxiedRawRasterType>
	MipmappedRasterFormatWriter<ProxiedRawRasterType>
	create_mipmapped_raster_format_writer(
			typename ProxiedRawRasterType::non_null_ptr_type proxied_raw_raster,
			GPlatesFileIO::RasterBandReaderHandle &source_raster_band_reader_handle)
	{
		return MipmappedRasterFormatWriter<ProxiedRawRasterType, false/*use_colour_palette*/>(
				proxied_raw_raster,
				source_raster_band_reader_handle);
	}


	/**
	 * Creates a @a MipmappedRasterFormatWriter for integer rasters coloured with a colour palette.
	 */
	template <class ProxiedRawRasterType>
	MipmappedRasterFormatWriter<ProxiedRawRasterType, true/*use_colour_palette*/>
	create_mipmapped_raster_format_writer(
			typename ProxiedRawRasterType::non_null_ptr_type proxied_raw_raster,
			GPlatesFileIO::RasterBandReaderHandle &source_raster_band_reader_handle,
			const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &colour_palette)
	{
		return MipmappedRasterFormatWriter<ProxiedRawRasterType, true/*use_colour_palette*/>(
				proxied_raw_raster,
				source_raster_band_reader_handle,
				colour_palette);
	}
}

#endif  // GPLATES_FILEIO_MIPMAPPEDRASTERFORMATWRITER_H
