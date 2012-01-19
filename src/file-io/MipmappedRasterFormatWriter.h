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

#include <limits>
#include <ostream>
#include <string>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/type_traits/is_floating_point.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/utility/enable_if.hpp>
#include <QByteArray>
#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QTemporaryFile>

#include "ErrorOpeningFileForWritingException.h"
#include "FileInfo.h"
#include "RasterFileCacheFormat.h"
#include "TemporaryFileRegistry.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesException.h"
#include "global/GPlatesAssert.h"
#include "global/LogException.h"

#include "gui/ColourRawRaster.h"
#include "gui/Mipmapper.h"
#include "gui/RasterColourPalette.h"

#include "maths/Real.h"

#include "property-values/RawRaster.h"
#include "property-values/RawRasterUtils.h"

#include "utils/Base2Utils.h"
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
			PROFILE_FUNC();

			const T *end = data + len;
			PROFILE_BEGIN(profile_first_write, "first write");
			out << *data;
			PROFILE_END(profile_first_write);
			++data;
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
					const GPlatesFileIO::RasterBandReaderHandle &source_raster_band_reader_handle) :
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

				out.setVersion(RasterFileCacheFormat::Q_DATA_STREAM_VERSION);

				// Write magic number/string.
				for (unsigned int n = 0; n < sizeof(RasterFileCacheFormat::MAGIC_NUMBER); ++n)
				{
					out << static_cast<quint8>(RasterFileCacheFormat::MAGIC_NUMBER[n]);
				}

				// Write the file size - write zero for now and come back later to fill it in.
				const qint64 file_size_offset = file.pos();
				qint64 total_output_file_size = 0;
				out << total_output_file_size;

				// Write version number.
				out << static_cast<quint32>(RasterFileCacheFormat::VERSION_NUMBER);

				// Write mipmap type.
				out << static_cast<quint32>(RasterFileCacheFormat::get_type_as_enum<mipmapped_element_type>());

				// Write whether coverage data is available in the file.
				// Availability of coverage data is determined by the mipmapped raster type.
				out << static_cast<quint32>(has_coverage());

				// Write number of levels.
				out << static_cast<quint32>(d_num_levels);

				// If there's no mipmap levels then nothing left to do.
				if (d_num_levels == 0)
				{
					file.close();
					return;
				}

				unsigned int level;

				// Create a temporary file for each mipmap level to contain its encoded data.
				// These files are temporary and will be removed on scope exit after their data
				// is concatenated to the final mipmap pyramid file.
				std::vector<boost::shared_ptr<QTemporaryFile> > temporary_mipmap_files;
				std::vector<boost::shared_ptr<QDataStream> > temporary_mipmap_file_streams;
				std::vector<boost::shared_ptr<QByteArray> > temporary_mipmap_byte_arrays;
				std::vector<boost::shared_ptr<QDataStream> > temporary_mipmap_byte_streams;
				for (level = 0; level < d_num_levels; ++level)
				{
					boost::shared_ptr<QTemporaryFile> temporary_mipmap_file(new QTemporaryFile());
					boost::shared_ptr<QDataStream> temporary_mipmap_file_stream(
							new QDataStream(temporary_mipmap_file.get()));
					// Use the same Qt data stream version as the final output file/stream.
					temporary_mipmap_file_stream->setVersion(RasterFileCacheFormat::Q_DATA_STREAM_VERSION);

					boost::shared_ptr<QByteArray> temporary_mipmap_byte_array(new QByteArray());
					boost::shared_ptr<QDataStream> temporary_mipmap_byte_stream(
							new QDataStream(temporary_mipmap_byte_array.get(), QIODevice::ReadWrite));
					// Use the same Qt data stream version as the final output file/stream.
					temporary_mipmap_byte_stream->setVersion(RasterFileCacheFormat::Q_DATA_STREAM_VERSION);

					// Attempt to open mipmap file (for reading/writing) in temporary directory.
					if (!temporary_mipmap_file->open())
					{
						// Attempt to open mipmap file in same directory as source raster.
						// The auto-generated part of the filename should get appended.
						temporary_mipmap_file->setFileTemplate(filename);
						if (!temporary_mipmap_file->open())
						{
							throw ErrorOpeningFileForWritingException(
									GPLATES_EXCEPTION_SOURCE,
									filename + ".tmp"/*give it an extension to indicate a temporary file*/);
						}
					}

					temporary_mipmap_files.push_back(temporary_mipmap_file);
					temporary_mipmap_file_streams.push_back(temporary_mipmap_file_stream);
					temporary_mipmap_byte_arrays.push_back(temporary_mipmap_byte_array);
					temporary_mipmap_byte_streams.push_back(temporary_mipmap_byte_stream);
				}

				// Create the block information for each mipmap level.
				std::vector<RasterFileCacheFormat::BlockInfos> mipmap_block_infos;
				for (level = 0; level < d_num_levels; ++level)
				{
					unsigned int mipmap_width;
					unsigned int mipmap_height;
					RasterFileCacheFormat::get_mipmap_dimensions(
							mipmap_width,
							mipmap_height,
							level,
							d_source_raster_width,
							d_source_raster_height);

					RasterFileCacheFormat::BlockInfos block_infos(mipmap_width, mipmap_height);
					mipmap_block_infos.push_back(block_infos);
				}

				// Find the smallest power-of-two that is greater than (or equal to) both the source
				// raster width and height - this will be used during the Hilbert curve traversal.
				const unsigned int source_raster_width_next_power_of_two =
						GPlatesUtils::Base2::next_power_of_two(d_source_raster_width);
				const unsigned int source_raster_height_next_power_of_two =
						GPlatesUtils::Base2::next_power_of_two(d_source_raster_height);
				const unsigned int source_raster_dimension_next_power_of_two =
						(std::max)(
								source_raster_width_next_power_of_two,
								source_raster_height_next_power_of_two);

				// Traverse the Hilbert curve of blocks of the source (base level) raster
				// using quad-tree recursion.
				// The leaf nodes of the traversal correspond to the blocks in the base level.
				// As we traverse back towards the root of the quad tree we perform mipmapping.
				// Each mipmap will have its own Hilbert curve (appropriate for its mipmap level)
				// and will temporarily write to its own output file.
				// Once traversal has finished the individual temporary mipmap files will be
				// concatenated - separate (temporary) files are used instead of writing all mipmaps
				// to a single mipmap pyramid file (the final output file) as they are generated
				// because due to block-compression it is not known in advance the size of encoded
				// data for each mipmap level.
				hilbert_curve_traversal(
						d_num_levels - 1/*level*/,
						0/*x_offset*/,
						0/*y_offset*/,
						source_raster_dimension_next_power_of_two/*dimension*/,
						0/*hilbert_start_point*/,
						0/*hilbert_end_point*/,
						temporary_mipmap_file_streams,
						temporary_mipmap_byte_arrays,
						temporary_mipmap_byte_streams,
						mipmap_block_infos);

				// Flush the mipmap byte streams to their file streams if any data remaining in them.
				for (level = 0; level < d_num_levels; ++level)
				{
					QDataStream &mipmap_file_stream = *temporary_mipmap_file_streams[level];
					QByteArray &mipmap_byte_array = *temporary_mipmap_byte_arrays[level];
					QDataStream &mipmap_byte_stream = *temporary_mipmap_byte_streams[level];

					if (!mipmap_byte_array.isEmpty())
					{
						mipmap_file_stream.writeRawData(
								mipmap_byte_array.constData(), mipmap_byte_array.size());
						mipmap_byte_array.clear();
						mipmap_byte_stream.device()->seek(0);
					}
				}

				const qint64 level_info_pos = file.pos();

				// The start of mipmap data for all levels.
				const qint64 data_file_start_pos = level_info_pos +
						// Skip the level infos...
						d_num_levels * RasterFileCacheFormat::LevelInfo::STREAM_SIZE;

				// Determine, and write, the mipmap level infos to the mipmap file.
				qint64 data_file_pos = data_file_start_pos;
				for (level = 0; level < d_num_levels; ++level)
				{
					RasterFileCacheFormat::LevelInfo level_info;

					RasterFileCacheFormat::get_mipmap_dimensions(
							level_info.width,
							level_info.height,
							level,
							d_source_raster_width,
							d_source_raster_height);

					level_info.blocks_file_offset = data_file_pos;
					level_info.num_blocks = mipmap_block_infos[level].get_num_blocks();

					out << level_info.width
						<< level_info.height
						<< level_info.blocks_file_offset
						<< level_info.num_blocks;

					// Account for the storage of the mipmapped raster's (optional) no-data value and
					// raster statistics.
					data_file_pos +=
							// no-data value...
							sizeof(quint32) + sizeof(mipmapped_element_type) +
							// raster statistics...
							5 * sizeof(quint32) + 4 * sizeof(double);

					// We'll be writing the mipmap's block information to the output file along
					// with the mipmap encoded data.
					data_file_pos +=
							level_info.num_blocks * RasterFileCacheFormat::BlockInfo::STREAM_SIZE;

					// The temporary mipmap file contains the encoded mipmap data for the current level and
					// that will also be written to the output file.
					data_file_pos += temporary_mipmap_files[level]->size();
				}

				// Predict the total size of the output file.
				total_output_file_size = data_file_pos;

				// We're about to write the mipmap data (block info + encoded block data).
				// So make sure we got the start file offset correct.
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						data_file_start_pos == file.pos(),
						GPLATES_ASSERTION_SOURCE);

				// Write the block information and encoded mipmap data for each mipmap level.
				for (level = 0; level < d_num_levels; ++level)
				{
					const RasterFileCacheFormat::BlockInfos &mipmap_blocks = mipmap_block_infos[level];

					// Make sure the width and height of each block makes sense.
					verify_mipmap_block_dimensions(mipmap_blocks, level);

					// Write the (optional) raster no-data value.
					//
					// NOTE: The stored mipmapped formats are floating-point and RGBA.
					// The former uses Nan (and hence has no stored no-data value).
					// The latter does not have a no-data value.
					//
					// FIXME: Get this value from the mipmapped raster just to be sure.
					if (boost::is_floating_point<mipmapped_element_type>::value)
					{
						out << static_cast<quint32>(true);
						typedef typename boost::remove_const<mipmapped_element_type>::type non_const_mipmapped_element_type;
						if (boost::is_same<double, non_const_mipmapped_element_type>::value)
						{
							out << GPlatesMaths::quiet_nan<double>();
						}
						else
						{
							out << GPlatesMaths::quiet_nan<float>();
						}
					}
					else
					{
						out << static_cast<quint32>(false);
						out << mipmapped_element_type(); // Doesn't matter what gets stored.
					}

					// Write the (optional) raster statistics.
					// We can get this from the proxied raster.
					// The statistics are for the original source raster (not mipmapped rasters)
					// but the statistics still apply to the mipmapped versions.
					quint32 has_raster_statistics = false;
					quint32 has_raster_minimum = false;
					quint32 has_raster_maximum = false;
					quint32 has_raster_mean = false;
					quint32 has_raster_standard_deviation = false;
					double raster_minimum = 0;
					double raster_maximum = 0;
					double raster_mean = 0;
					double raster_standard_deviation = 0;
					GPlatesPropertyValues::RasterStatistics *raster_statistics =
							GPlatesPropertyValues::RawRasterUtils::get_raster_statistics(*d_proxied_raw_raster);
					if (raster_statistics)
					{
						has_raster_statistics = true;

						if (raster_statistics->minimum)
						{
							has_raster_minimum = true;
							raster_minimum = raster_statistics->minimum.get();
						}
						if (raster_statistics->maximum)
						{
							has_raster_maximum = true;
							raster_maximum = raster_statistics->maximum.get();
						}
						if (raster_statistics->mean)
						{
							has_raster_mean = true;
							raster_mean = raster_statistics->mean.get();
						}
						if (raster_statistics->standard_deviation)
						{
							has_raster_standard_deviation = true;
							raster_standard_deviation = raster_statistics->standard_deviation.get();
						}
					}
					out << has_raster_statistics;
					out << has_raster_minimum;
					out << has_raster_maximum;
					out << has_raster_mean;
					out << has_raster_standard_deviation;
					out << raster_minimum;
					out << raster_maximum;
					out << raster_mean;
					out << raster_standard_deviation;

					// The file offset at which the current mipmap's encoded data will be written to.
					const qint64 encoded_data_file_pos =
							file.pos() +
							mipmap_blocks.get_num_blocks() * RasterFileCacheFormat::BlockInfo::STREAM_SIZE;

					// Write the current mipmap's block information to the output file.
					const unsigned int num_blocks = mipmap_blocks.get_num_blocks();
					for (unsigned int block_index = 0; block_index < num_blocks; ++block_index)
					{
						RasterFileCacheFormat::BlockInfo block_info =
								mipmap_blocks.get_block_info(block_index);

						// The offsets from the start of the encoded data are converted to file offsets.
						block_info.main_offset += encoded_data_file_pos;
						if (block_info.coverage_offset != 0)
						{
							block_info.coverage_offset += encoded_data_file_pos;
						}

						out << block_info.x_offset
							<< block_info.y_offset
							<< block_info.width
							<< block_info.height
							<< block_info.main_offset
							<< block_info.coverage_offset;
					}

					// Now write the mipmap's encoded data to the output file.
					// We do this by copying the encoded data from the temporary mipmap file.
					// The temporary file will get removed on scope exit.
					write_temporary_mipmap_file_to_output(*temporary_mipmap_files[level], out);
				}

				// Make sure our predicted file size matches the actual file size.
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						total_output_file_size == file.size(),
						GPLATES_ASSERTION_SOURCE);

				// Write the total size of the output file so the reader can verify that the
				// file was not partially written.
				file.seek(file_size_offset);
				out << total_output_file_size;
			}

		protected:
			virtual
			~BaseMipmappedRasterFormatWriter()
			{  }

			/**
			 * Create a mipmapper from the specified source raster region.
			 *
			 * This is required since some derived classes need to transform the source raster data
			 * using a colour palette before mipmapping. This is required for integer rasters that
			 * are not mipmapped directly (ie, the integer data is not mipmapped like a float raster)
			 * but instead coloured first and then mipmapped.
			 */
			virtual
			boost::shared_ptr<mipmapper_type>
			create_source_region_mipmapper(
					const typename source_raster_type::non_null_ptr_type &source_region_raster) = 0;

			/**
			 * Returns true if the mipmapped raster type has coverage data (determined by derived class).
			 *
			 * Coverage data is only generated for non-RGBA rasters (regardless of whether a non-RGBA
			 * raster actually contains a sentinel pixel, ie, a pixel with the no-data or sentinel value).
			 * If a raster type supports sentinel values (ie, non-RGBA) but has no sentinel values
			 * in the raster then the coverage raster will compress very well so the extra space
			 * used should be small (and it saves us having to do a full pass over the source raster
			 * just to see if it contains even a single sentinel pixel).
			 */
			virtual
			bool
			has_coverage() const = 0;


			typename ProxiedRawRasterType::non_null_ptr_type d_proxied_raw_raster;
			GPlatesFileIO::RasterBandReaderHandle d_source_raster_band_reader_handle;
			unsigned int d_source_raster_width;
			unsigned int d_source_raster_height;
			unsigned int d_num_levels;

		private:

			/**
			 * When the number of bytes written to a mipmap byte stream (attached to QByteArray)
			 * exceeds this threshold then we'll stream it to the mipmap file.
			 *
			 * Doing this avoids an excessive number of disk file seeks (slowing things dramatically).
			 */
			static const unsigned int MIPMAP_BYTE_STREAM_SIZE_THRESHOLD = 8 * 1024 * 1024;


			/**
			 * Traverse the Hilbert curve of blocks of the source (base level) raster
			 * using quad-tree recursion.
			 *
			 * The leaf nodes of the traversal correspond to the blocks in the base level.
			 * As we traverse back towards the root of the quad tree we perform mipmapping.
			 * Each mipmap will have its own Hilbert curve (appropriate for its mipmap level)
			 * and will temporarily write to its own mipmap file (stream) and record its own
			 * block informations.
			 */
			boost::optional<boost::shared_ptr<mipmapper_type> >
			hilbert_curve_traversal(
					unsigned int level,
					unsigned int x_offset,
					unsigned int y_offset,
					unsigned int dimension,
					unsigned int hilbert_start_point,
					unsigned int hilbert_end_point,
					const std::vector<boost::shared_ptr<QDataStream> > &temporary_mipmap_file_streams,
					const std::vector<boost::shared_ptr<QByteArray> > &temporary_mipmap_byte_arrays,
					const std::vector<boost::shared_ptr<QDataStream> > &temporary_mipmap_byte_streams,
					std::vector<RasterFileCacheFormat::BlockInfos> &mipmap_block_infos)
			{
				// See if the current quad-tree region is outside the source raster.
				// This can happen because the Hilbert traversal operates on power-of-two dimensions
				// which encompass the source raster (leaving regions that contain no source raster data).
				if (x_offset >= d_source_raster_width || y_offset >= d_source_raster_height)
				{
					return boost::none;
				}

				// For the highest-resolution mipmap level (not the full-resolution base level)
				// we need to get data from the source raster.
				if (level == 0)
				{
					// The source raster region should be twice the size of the mipmapped region.
					// The later is the size of a single block.
					GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
							dimension == 2 * RasterFileCacheFormat::BLOCK_SIZE,
							GPLATES_ASSERTION_SOURCE);

					// Get the source raster data from the region we need for mipmapping the current
					// quad tree region.
					boost::shared_ptr<mipmapper_type> mipmapper = get_source_raster_data(x_offset, y_offset);

					// Get the current block in the current mipmap based on the block x/y offsets.
					RasterFileCacheFormat::BlockInfo &block_info =
							mipmap_block_infos[level].get_block_info(
									x_offset / dimension,
									y_offset / dimension);

					// Mipmap the source raster region.
					mipmap(
							*mipmapper,
							*temporary_mipmap_file_streams[level],
							*temporary_mipmap_byte_arrays[level],
							*temporary_mipmap_byte_streams[level],
							block_info,
							// Level 0 is half the resolution of the full-resolution source raster...
							x_offset >> 1,
							y_offset >> 1);

					// Return the mipmapper so the second mipmap level (parent of this quad-tree
					// recursion) can use the mipmapped data for further mipmapping.
					return mipmapper;
				}

				const unsigned int child_level = level - 1;
				const unsigned int child_dimension = (dimension >> 1);

				// References to the (up to) four child mipmapped regions.
				// The Hilbert curve traverses the child nodes in an order that changes
				// (unlike the fixed z-order traversal) and so we need to map back to a z-order
				// traversal before we can join the child mipmaps and perform further mipmapping.
				boost::optional<const mipmapper_type &> child_mipmappers_zorder[2][2];

				const unsigned int child_x_offset_hilbert0 = hilbert_start_point;
				const unsigned int child_y_offset_hilbert0 = hilbert_start_point;
				const boost::optional<boost::shared_ptr<mipmapper_type> > child_mipmapper_hilbert0 =
						hilbert_curve_traversal(
								child_level,
								x_offset + child_x_offset_hilbert0 * child_dimension,
								y_offset + child_y_offset_hilbert0 * child_dimension,
								child_dimension,
								hilbert_start_point,
								1 - hilbert_end_point,
								temporary_mipmap_file_streams,
								temporary_mipmap_byte_arrays,
								temporary_mipmap_byte_streams,
								mipmap_block_infos);
				if (child_mipmapper_hilbert0)
				{
					// Map Hilbert traversal to z-order traversal.
					child_mipmappers_zorder[child_y_offset_hilbert0][child_x_offset_hilbert0] =
							*child_mipmapper_hilbert0.get();
				}

				const unsigned int child_x_offset_hilbert1 = hilbert_end_point;
				const unsigned int child_y_offset_hilbert1 = 1 - hilbert_end_point;
				const boost::optional<boost::shared_ptr<mipmapper_type> > child_mipmapper_hilbert1 =
						hilbert_curve_traversal(
								child_level,
								x_offset + child_x_offset_hilbert1 * child_dimension,
								y_offset + child_y_offset_hilbert1 * child_dimension,
								child_dimension,
								hilbert_start_point,
								hilbert_end_point,
								temporary_mipmap_file_streams,
								temporary_mipmap_byte_arrays,
								temporary_mipmap_byte_streams,
								mipmap_block_infos);
				if (child_mipmapper_hilbert1)
				{
					// Map Hilbert traversal to z-order traversal.
					child_mipmappers_zorder[child_y_offset_hilbert1][child_x_offset_hilbert1] =
							*child_mipmapper_hilbert1.get();
				}

				const unsigned int child_x_offset_hilbert2 = 1 - hilbert_start_point;
				const unsigned int child_y_offset_hilbert2 = 1 - hilbert_start_point;
				const boost::optional<boost::shared_ptr<mipmapper_type> > child_mipmapper_hilbert2 =
						hilbert_curve_traversal(
								child_level,
								x_offset + child_x_offset_hilbert2 * child_dimension,
								y_offset + child_y_offset_hilbert2 * child_dimension,
								child_dimension,
								hilbert_start_point,
								hilbert_end_point,
								temporary_mipmap_file_streams,
								temporary_mipmap_byte_arrays,
								temporary_mipmap_byte_streams,
								mipmap_block_infos);
				if (child_mipmapper_hilbert2)
				{
					// Map Hilbert traversal to z-order traversal.
					child_mipmappers_zorder[child_y_offset_hilbert2][child_x_offset_hilbert2] =
							*child_mipmapper_hilbert2.get();
				}

				const unsigned int child_x_offset_hilbert3 = 1 - hilbert_end_point;
				const unsigned int child_y_offset_hilbert3 = hilbert_end_point;
				const boost::optional<boost::shared_ptr<mipmapper_type> > child_mipmapper_hilbert3 =
						hilbert_curve_traversal(
								child_level,
								x_offset + child_x_offset_hilbert3 * child_dimension,
								y_offset + child_y_offset_hilbert3 * child_dimension,
								child_dimension,
								1 - hilbert_start_point,
								hilbert_end_point,
								temporary_mipmap_file_streams,
								temporary_mipmap_byte_arrays,
								temporary_mipmap_byte_streams,
								mipmap_block_infos);
				if (child_mipmapper_hilbert3)
				{
					// Map Hilbert traversal to z-order traversal.
					child_mipmappers_zorder[child_y_offset_hilbert3][child_x_offset_hilbert3] =
							*child_mipmapper_hilbert3.get();
				}

				// We shouldn't be able to get here unless the child mipmap (at z-order child x/y
				// indices 0/0) contains data (ie, is not outside the entire source raster).
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						child_mipmappers_zorder[0][0],
						GPLATES_ASSERTION_SOURCE);

				// Join the four child mipmappers into one mipmapper.
				boost::shared_ptr<mipmapper_type> mipmapper(
						new mipmapper_type(
								child_mipmappers_zorder[0][0].get(),
								child_mipmappers_zorder[0][1],
								child_mipmappers_zorder[1][0],
								child_mipmappers_zorder[1][1]));

				// Get the current block in the current mipmap based on the block x/y offsets.
				RasterFileCacheFormat::BlockInfo &block_info =
						mipmap_block_infos[level].get_block_info(
								x_offset / dimension,
								y_offset / dimension);

				// Mipmap the joined child regions.
				mipmap(
						*mipmapper,
						*temporary_mipmap_file_streams[level],
						*temporary_mipmap_byte_arrays[level],
						*temporary_mipmap_byte_streams[level],
						block_info,
						// Level 0 is half the resolution of the full-resolution source raster.
						// The other levels scale resolution as 1 / 2^(level+1) ...
						x_offset >> (level + 1),
						y_offset >> (level + 1));

				// Return the mipmapper so the next mipmap level (parent of this quad-tree
				// recursion) can use the mipmapped data for further mipmapping.
				return mipmapper;
			}


			/**
			 * Get source raster data (full-resolution data) of size 2*BLOCK_SIZE x 2*BLOCK_SIZE
			 * (or less near right or bottom edge of source raster) - to be used for generating
			 * mipmap data for a region of size BLOCK_SIZE x BLOCK_SIZE (or less).
			 *
			 * The returned mipmapper contains the source region data and is ready for mipmapping.
			 */
			boost::shared_ptr<mipmapper_type>
			get_source_raster_data(
					unsigned int x_offset,
					unsigned int y_offset)
			{
				const unsigned int dimension = 2 * RasterFileCacheFormat::BLOCK_SIZE;

				// If we are near the right or bottom edge of the source raster then we can
				// get partially covered blocks so ensure the source region is valid.
				const unsigned int source_region_width =
						(x_offset + dimension > d_source_raster_width)
						? d_source_raster_width - x_offset
						: dimension;
				const unsigned int source_region_height =
						(y_offset + dimension > d_source_raster_height)
						? d_source_raster_height - y_offset
						: dimension;

				// The region of the source raster that we are going to mipmap.
				const QRect source_region_rect(x_offset, y_offset, source_region_width, source_region_height);

				// Get the region data from the source raster.
				PROFILE_BEGIN(profile_get_src_data, "get source region data");
				boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type> source_region_raw_raster =
						d_source_raster_band_reader_handle.get_raw_raster(source_region_rect);
				PROFILE_END(profile_get_src_data);
				if (!source_region_raw_raster)
				{
					throw GPlatesGlobal::LogException(
							GPLATES_EXCEPTION_SOURCE,
							"Unable to read source raster region.");
				}

				// Downcast the source region raster to the source raster type.
				boost::optional<typename source_raster_type::non_null_ptr_type> source_region_raster =
						GPlatesPropertyValues::RawRasterUtils::try_raster_cast<
								source_raster_type>(*source_region_raw_raster.get());
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						source_region_raster,
						GPLATES_ASSERTION_SOURCE);

				// Get derived class to create the mipmapper.
				boost::shared_ptr<mipmapper_type> mipmapper =
						create_source_region_mipmapper(source_region_raster.get());
				if (!mipmapper)
				{
					throw GPlatesGlobal::LogException(
							GPLATES_EXCEPTION_SOURCE,
							"Unable to create mipmapper.");
				}

				return mipmapper;
			}


			/**
			 * Mipmap source data (either from source raster or parent mipmap level) and
			 * write data to the specified mipmap stream and record stream offsets in block info.
			 */
			void
			mipmap(
					mipmapper_type &mipmapper,
					QDataStream &mipmap_file_stream,
					QByteArray &mipmap_byte_array,
					QDataStream &mipmap_byte_stream,
					RasterFileCacheFormat::BlockInfo &mipmap_block_info,
					unsigned int mipmap_x_offset,
					unsigned int mipmap_y_offset)
			{
				PROFILE_FUNC();

				// Perform the mipmapping.
				mipmapper.generate_next();

				// Get the mipmap for the current level.
				typename mipmapped_raster_type::non_null_ptr_to_const_type current_mipmap =
						mipmapper.get_current_mipmap();

				// The pixel offsets of mipmapped block within the (mipmapped) raster.
				mipmap_block_info.x_offset = mipmap_x_offset;
				mipmap_block_info.y_offset = mipmap_y_offset;

				// For most blocks the dimensions will be RasterFileCacheFormat::BLOCK_SIZE but
				// for blocks near the right or bottom edge of source raster they can be less.
				mipmap_block_info.width = current_mipmap->width();
				mipmap_block_info.height = current_mipmap->height();
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						mipmap_block_info.width <= RasterFileCacheFormat::BLOCK_SIZE &&
							mipmap_block_info.height <= RasterFileCacheFormat::BLOCK_SIZE,
						GPLATES_ASSERTION_SOURCE);

				// Record the file offset of the current block of data.
				// The offset is the current file offset plus any unwritten data.
				mipmap_block_info.main_offset =
						mipmap_file_stream.device()->pos() + mipmap_byte_array.size();

				// Write current main mipmap to the byte stream.
				// We do this instead of writing to the file in order to avoid constantly
				// doing file seeks which slow things down dramatically.
				MipmappedRasterFormatWriterInternals::write<mipmapped_element_type>(
						mipmap_byte_stream,
						current_mipmap->data(),
						current_mipmap->width() * current_mipmap->height());

				// Get and write the associated coverage raster if required.
				boost::optional<coverage_raster_type::non_null_ptr_to_const_type> current_coverage =
						mipmapper.get_current_coverage();
				if (current_coverage)
				{
					GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
							current_coverage.get()->width() == current_mipmap->width() &&
								current_coverage.get()->height() == current_mipmap->height(),
							GPLATES_ASSERTION_SOURCE);

					// Record the file offset of the current block of coverage data.
					// The offset is the current file offset plus any unwritten data.
					mipmap_block_info.coverage_offset =
							mipmap_file_stream.device()->pos() + mipmap_byte_array.size();

					// Write the current coverage mipmap to the byte stream.
					// We do this instead of writing to the file in order to avoid constantly
					// doing file seeks which slow things down dramatically.
					MipmappedRasterFormatWriterInternals::write<coverage_element_type>(
							mipmap_byte_stream,
							current_coverage.get()->data(),
							current_coverage.get()->width() * current_coverage.get()->height());
				}
				else
				{
					mipmap_block_info.coverage_offset = 0;
				}

				// Flush the mipmap byte stream to the file stream if enough data has accumulated.
				if (mipmap_byte_array.size() >= MIPMAP_BYTE_STREAM_SIZE_THRESHOLD)
				{
					mipmap_file_stream.writeRawData(mipmap_byte_array.constData(), mipmap_byte_array.size());
					mipmap_byte_array.clear();
					mipmap_byte_stream.device()->seek(0);
				}
			}


			/**
			 * Make sure the block dimensions are correct for the mipmap level.
			 *
			 * Internal blocks should have dimensions 'RasterFileCacheFormat::BLOCK_SIZE'.
			 * Blocks near right or bottom edge of mipmap can be smaller.
			 */
			void
			verify_mipmap_block_dimensions(
					const RasterFileCacheFormat::BlockInfos &mipmap_blocks,
					unsigned int level)
			{
				unsigned int mipmap_width;
				unsigned int mipmap_height;
				RasterFileCacheFormat::get_mipmap_dimensions(
						mipmap_width,
						mipmap_height,
						level,
						d_source_raster_width,
						d_source_raster_height);

				// Verify the dimensions of each block.
				const unsigned int num_blocks = mipmap_blocks.get_num_blocks();
				for (unsigned int block_index = 0; block_index < num_blocks; ++block_index)
				{
					const RasterFileCacheFormat::BlockInfo &block_info =
							mipmap_blocks.get_block_info(block_index);

					// Verify the block width.
					if (block_info.x_offset + block_info.width < mipmap_width)
					{
						// Block not at right edge of mipmap so should be the full block size.
						GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
								block_info.width == RasterFileCacheFormat::BLOCK_SIZE,
								GPLATES_ASSERTION_SOURCE);
					}
					else
					{
						// Block is at right edge of mipmap.
						GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
								block_info.x_offset + block_info.width == mipmap_width,
								GPLATES_ASSERTION_SOURCE);
					}

					// Verify the block height.
					if (block_info.y_offset + block_info.height < mipmap_height)
					{
						// Block not at bottom edge of mipmap so should be the full block size.
						GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
								block_info.height == RasterFileCacheFormat::BLOCK_SIZE,
								GPLATES_ASSERTION_SOURCE);
					}
					else
					{
						// Block is at bottom edge of mipmap.
						GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
								block_info.y_offset + block_info.height == mipmap_height,
								GPLATES_ASSERTION_SOURCE);
					}
				}
			}


			/**
			 * Appends the specified temporary mipmap file (contained encoded mipmap data) to the
			 * specified output stream.
			 *
			 * Note that the data in the temporary mipmap file is expected to have been written
			 * using a 'QDataStream' and with the same Qt data stream version.
			 */
			void
			write_temporary_mipmap_file_to_output(
					QFile &temporary_mipmap_file,
					QDataStream &out)
			{
				PROFILE_FUNC();

				// Make sure any data written to the temporary file is not still buffered.
				temporary_mipmap_file.flush();
				// Start reading at the beginning of the file.
				temporary_mipmap_file.seek(0);

				// Used to ensure we write the entire temporary file to the output file.
				const qint64 temporary_mipmap_file_size = temporary_mipmap_file.size();
				qint64 total_bytes_written = 0;

				// Allocate a buffer for reading.
				const unsigned int read_buffer_size = 1024 * 1024;
				boost::scoped_ptr<char> read_buffer(new char[read_buffer_size]);
				// Append the temporary mipmap file to the output file.
				while (true)
				{
					const qint64 bytes_read = temporary_mipmap_file.read(read_buffer.get(), read_buffer_size);
					if (bytes_read == 0)
					{
						// If we've read the entire file then we're finished.
						break;
					}
					if (bytes_read < 0)
					{
						throw GPlatesGlobal::LogException(
								GPLATES_EXCEPTION_SOURCE,
								"Unable to read temporary mipmap file during raster file cache mipmap generation.");
					}

					const int bytes_written = out.writeRawData(read_buffer.get(), static_cast<int>(bytes_read));
					if (bytes_written != bytes_read)
					{
						throw GPlatesGlobal::LogException(
								GPLATES_EXCEPTION_SOURCE,
								"Error writing to raster file cache mipmap.");
					}
					total_bytes_written += bytes_written;
				}

				// Ensure we write the entire temporary file to the output file.
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						total_bytes_written == temporary_mipmap_file_size,
						GPLATES_ASSERTION_SOURCE);
			}
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
		 *
		 * NOTE: @a colour_palette is ignored and not used - it is only there so that all
		 * constructors have the same signature - easier for clients to write generic code.
		 */
		MipmappedRasterFormatWriter(
				typename ProxiedRawRasterType::non_null_ptr_type proxied_raw_raster,
				const GPlatesFileIO::RasterBandReaderHandle &source_raster_band_reader_handle,
				const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &colour_palette =
						GPlatesGui::RasterColourPalette::create()) :
			base_type(
					proxied_raw_raster,
					source_raster_band_reader_handle)
		{
		}

	private:
		virtual
		boost::shared_ptr<mipmapper_type>
		create_source_region_mipmapper(
				const typename source_raster_type::non_null_ptr_type &source_region_raster)
		{
			return boost::shared_ptr<mipmapper_type>(
					new mipmapper_type(source_region_raster));
		}

		virtual
		bool
		has_coverage() const
		{
			// The source region will be RGBA and there's no 'separate' coverage for a RGBA raster.
			// The coverage is already in the alpha channel.
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
		 *
		 * NOTE: @a colour_palette is ignored and not used - it is only there so that all
		 * constructors have the same signature - easier for clients to write generic code.
		 */
		MipmappedRasterFormatWriter(
				typename ProxiedRawRasterType::non_null_ptr_type proxied_raw_raster,
				const GPlatesFileIO::RasterBandReaderHandle &source_raster_band_reader_handle,
				const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &colour_palette =
						GPlatesGui::RasterColourPalette::create()) :
			base_type(
					proxied_raw_raster,
					source_raster_band_reader_handle)
		{
		}

	private:
		virtual
		boost::shared_ptr<mipmapper_type>
		create_source_region_mipmapper(
				const typename source_raster_type::non_null_ptr_type &source_region_raster)
		{
			return boost::shared_ptr<mipmapper_type>(
					new mipmapper_type(source_region_raster));
		}

		virtual
		bool
		has_coverage() const
		{
			// The source region will support no-data values and hence coverage.
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
		 *
		 * NOTE: @a colour_palette is ignored and not used - it is only there so that all
		 * constructors have the same signature - easier for clients to write generic code.
		 */
		MipmappedRasterFormatWriter(
				typename ProxiedRawRasterType::non_null_ptr_type proxied_raw_raster,
				const GPlatesFileIO::RasterBandReaderHandle &source_raster_band_reader_handle,
				const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &colour_palette =
						GPlatesGui::RasterColourPalette::create()) :
			base_type(
					proxied_raw_raster,
					source_raster_band_reader_handle)
		{
		}

	private:
		virtual
		boost::shared_ptr<mipmapper_type>
		create_source_region_mipmapper(
				const typename source_raster_type::non_null_ptr_type &source_region_raster)
		{
			return boost::shared_ptr<mipmapper_type>(
					new mipmapper_type(source_region_raster));
		}

		virtual
		bool
		has_coverage() const
		{
			// The source region will support no-data values and hence coverage.
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
				const GPlatesFileIO::RasterBandReaderHandle &source_raster_band_reader_handle,
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
		create_source_region_mipmapper(
				const typename source_raster_type::non_null_ptr_type &source_region_raster)
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
		has_coverage() const
		{
			// The source region will be coloured/converted to RGBA and there's no 'separate'
			// coverage for a RGBA raster - the coverage is already in the alpha channel.
			return false;
		}

	private:
		GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type d_colour_palette;
	};
}

#endif  // GPLATES_FILEIO_MIPMAPPEDRASTERFORMATWRITER_H
