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

#ifndef GPLATES_FILE_IO_RASTERFILECACHEFORMATREADER_H
#define GPLATES_FILE_IO_RASTERFILECACHEFORMATREADER_H

#include <cstring> // for memcpy
#include <functional>
#include <queue>
#include <vector>
#include <boost/optional.hpp>
#include <boost/scoped_array.hpp>
#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QSysInfo>
#include <QtGlobal>

#include "ErrorOpeningFileForReadingException.h"
#include "FileFormatNotSupportedException.h"
#include "RasterFileCacheFormat.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/LogException.h"

#include "gui/Colour.h"

#include "property-values/RasterStatistics.h"
#include "property-values/RawRaster.h"
#include "property-values/RawRasterUtils.h"

#include "utils/Profile.h"


namespace GPlatesFileIO
{
	/**
	 * Reads an image stored in a raster file cache by traversing a Hilbert curve of encoded blocks
	 * of raster data stored in the file.
	 *
	 * The image data is encoded in blocks of dimension 'RasterFileCacheFormat::BLOCK_SIZE' where
	 * the blocks follow a Hilbert curved path through the image (for optimal locality of data
	 * in the file - minimise disk seeks).
	 *
	 * This can be used to retrieve a cached copy of the original source raster as well as mipmapped
	 * versions of the source raster.
	 */
	template <class RawRasterType>
	class RasterFileCacheFormatReader
	{
	public:

		RasterFileCacheFormatReader(
				quint32 version_number,
				QFile &file,
				QDataStream &in,
				unsigned int image_width,
				unsigned int image_height,
				unsigned int num_blocks,
				bool has_coverage) :
			d_file(file),
			d_in(in),
			d_image_width(image_width),
			d_image_height(image_height),
			d_has_coverage(has_coverage),
			d_block_infos(image_width, image_height)
		{
			// NOTE: The total file size should have been verified before we get here so there's no
			// need to check that the file is large enough to read data as we read.

			// Read the (optional) raster no-data value.
			quint32 has_no_data_value;
			raster_element_type no_data_value;
			d_in >> has_no_data_value;
			d_in >> no_data_value;
			if (has_no_data_value)
			{
				d_no_data_value = no_data_value;
			}

			// Read the (optional) raster statistics.
			quint32 has_raster_statistics;
			quint32 has_raster_minimum;
			quint32 has_raster_maximum;
			quint32 has_raster_mean;
			quint32 has_raster_standard_deviation;
			double raster_minimum;
			double raster_maximum;
			double raster_mean;
			double raster_standard_deviation;
			d_in >> has_raster_statistics;
			d_in >> has_raster_minimum;
			d_in >> has_raster_maximum;
			d_in >> has_raster_mean;
			d_in >> has_raster_standard_deviation;
			d_in >> raster_minimum;
			d_in >> raster_maximum;
			d_in >> raster_mean;
			d_in >> raster_standard_deviation;
			if (has_raster_statistics)
			{
				d_raster_statistics = GPlatesPropertyValues::RasterStatistics();
				if (has_raster_minimum)
				{
					d_raster_statistics->minimum = raster_minimum;
				}
				if (has_raster_maximum)
				{
					d_raster_statistics->maximum = raster_maximum;
				}
				if (has_raster_mean)
				{
					d_raster_statistics->mean = raster_mean;
				}
				if (has_raster_standard_deviation)
				{
					d_raster_statistics->standard_deviation = raster_standard_deviation;
				}
			}

			// Verify the number of blocks makes sense.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					num_blocks == d_block_infos.get_num_blocks(),
					GPLATES_ASSERTION_SOURCE);

			// Read the block information.
			for (unsigned int block_index = 0; block_index < num_blocks; ++block_index)
			{
				RasterFileCacheFormat::BlockInfo &block_info = d_block_infos.get_block_info(block_index);

				// Note that the offsets are from the start of the file and hence are file offsets
				// and not offsets from the beginning of the block-encoded data.
				d_in >> block_info.x_offset
					>> block_info.y_offset
					>> block_info.width
					>> block_info.height
					>> block_info.main_offset
					>> block_info.coverage_offset;

				// Make sure the coverage offsets match whether we have coverage data or not.
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						(block_info.coverage_offset != 0) == has_coverage,
						GPLATES_ASSERTION_SOURCE);
			}
		}


		~RasterFileCacheFormatReader()
		{
		}


		/**
		 * Reads the given region from the raster file cache.
		 *
		 * Returns boost::none if the region given lies partly or wholly outside the raster image.
		 * Also returns boost::none if the file has already been closed.
		 */
		boost::optional<typename RawRasterType::non_null_ptr_type>
		read_raster(
				unsigned int x_offset,
				unsigned int y_offset,
				unsigned int width,
				unsigned int height) const
		{
			if (!is_valid_region(x_offset, y_offset, width, height))
			{
				return boost::none;
			}

			typename RawRasterType::non_null_ptr_type result = RawRasterType::create(width, height);
			
			copy_region(
					result->data(),
					x_offset,
					y_offset,
					width,
					height,
					d_block_infos,
					&RasterFileCacheFormat::BlockInfo::main_offset);

			// Add the no-data value to the raster if the raster type needs one (ie, if not RGBA).
			if (d_no_data_value)
			{
				GPlatesPropertyValues::RawRasterUtils::add_no_data_value(*result, d_no_data_value.get());
			}

			// Add the raster statistics to the raster if its type accepts them (ie, if not RGBA).
			if (d_raster_statistics)
			{
				GPlatesPropertyValues::RawRasterUtils::add_raster_statistics(*result, d_raster_statistics.get());
			}

			return result;
		}


		/**
		 * Reads the given region from the raster file cache as a coverage.
		 *
		 * The coverage values are 1.0 for all pixels except sentinel pixels (pixels containing the
		 * non-data value) which they are set to 0.0 coverage value.
		 *
		 * Returns boost::none if the region given lies partly or wholly outside the raster image.
		 * Also returns boost::none if the file has already been closed.
		 */
		boost::optional<GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type>
		read_coverage(
				unsigned int x_offset,
				unsigned int y_offset,
				unsigned int width,
				unsigned int height) const
		{
			if (!is_valid_region(x_offset, y_offset, width, height))
			{
				return boost::none;
			}

			// If raster type does not have separate coverage data.
			// This happens for RGBA format because coverage is already embedded in the alpha channel.
			if (!d_has_coverage)
			{
				return boost::none;
			}

			GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type result =
					GPlatesPropertyValues::CoverageRawRaster::create(width, height);

			copy_region(
					result->data(),
					x_offset,
					y_offset,
					width,
					height,
					d_block_infos,
					&RasterFileCacheFormat::BlockInfo::coverage_offset);

			return result;
		}

	private:

		typedef typename RawRasterType::element_type raster_element_type;


		/**
		 * Used to sort blocks by file offset.
		 *
		 * Sorts highest-to-lowest so that priority_queue gives high priority to lower file offsets.
		 *
		 * We want to read lower file offsets before higher file offsets because if two blocks
		 * are adjacent in the file then after reading the lower file offset block the file's
		 * offset will already be at the correct file offset for the next block (and this is not
		 * the case for the opposite ordering).
		 */
		class SortByFileOffset :
				public std::binary_function<RasterFileCacheFormat::BlockInfo, RasterFileCacheFormat::BlockInfo, bool>
		{
		public:
			//! Pointer-to-data-member determines which file offset (main or coverage) to use in comparison.
			explicit
			SortByFileOffset(
					quint64 RasterFileCacheFormat::BlockInfo::*file_offset) :
				d_file_offset(file_offset)
			{  }

			bool
			operator()(
					const RasterFileCacheFormat::BlockInfo &lhs,
					const RasterFileCacheFormat::BlockInfo &rhs) const
			{
				// Sorts highest-to-lowest so that priority_queue gives high priority to lower file offsets.
				return lhs.*d_file_offset > rhs.*d_file_offset;
			}

		private:
			quint64 RasterFileCacheFormat::BlockInfo::*d_file_offset;
		};


		bool
		is_valid_region(
				unsigned int x_offset,
				unsigned int y_offset,
				unsigned int width,
				unsigned int height) const
		{
			return x_offset + width <= d_image_width && y_offset + height <= d_image_height;
		}


		template <typename T>
		void
		copy_region(
				T *region_data,
				unsigned int region_x_offset,
				unsigned int region_y_offset,
				unsigned int region_width,
				unsigned int region_height,
				const RasterFileCacheFormat::BlockInfos &block_infos,
				// Determines whether to use image file offset or coverage file offset...
				quint64 RasterFileCacheFormat::BlockInfo::*encoded_block_data_offset) const
		{
			// Determine the range blocks in the 'x' direction covered by the requested region.
			const unsigned int start_block_x_offset =
					region_x_offset / RasterFileCacheFormat::BLOCK_SIZE;
			const unsigned int last_block_x_offset =
					(region_x_offset + region_width - 1) / RasterFileCacheFormat::BLOCK_SIZE;
			// Determine the range blocks in the 'y' direction covered by the requested region.
			const unsigned int start_block_y_offset =
					region_y_offset / RasterFileCacheFormat::BLOCK_SIZE;
			const unsigned int last_block_y_offset =
					(region_y_offset + region_height - 1) / RasterFileCacheFormat::BLOCK_SIZE;

			// Sort the blocks according to file offset to minimise the distance between file seeks
			// as we proceed to sequentially read the blocks.
			// The blocks are written to the file in a Hilbert curve path (through the raster)
			// in order to achieve optimal locality within the file for nearby blocks.
			const SortByFileOffset sort_by_file_offset(encoded_block_data_offset);
			std::priority_queue<
					RasterFileCacheFormat::BlockInfo,
					std::vector<RasterFileCacheFormat::BlockInfo>,
					SortByFileOffset> blocks_in_region(sort_by_file_offset);
			for (unsigned int block_y_offset = start_block_y_offset;
				block_y_offset <= last_block_y_offset;
				++block_y_offset)
			{
				for (unsigned int block_x_offset = start_block_x_offset;
					block_x_offset <= last_block_x_offset;
					++block_x_offset)
				{
					blocks_in_region.push(
							block_infos.get_block_info(block_x_offset, block_y_offset));
				}
			}

			// Allocate working space to read block data into.
			boost::scoped_array<T> block_data(
					new T[RasterFileCacheFormat::BLOCK_SIZE * RasterFileCacheFormat::BLOCK_SIZE]);

			// Read each block in the sorted sequence and write into the appropriate sub-section
			// of the destination region.
			while (!blocks_in_region.empty())
			{
				const RasterFileCacheFormat::BlockInfo &block_info = blocks_in_region.top();

				PROFILE_BEGIN(profile_seek, "RasterFileCacheFormatReader seek");
				// Seek to the beginning of the block's encoded data.
				d_file.seek(block_info.*encoded_block_data_offset);
				PROFILE_END(profile_seek);

				// Read the encoded block data into our block data buffer.
				read_block_data(block_data.get(), block_info.width * block_info.height);

				// Copy the block data into the appropriate sub-section of the destination region.
				copy_block_data_into_region(
						region_data,
						region_x_offset,
						region_y_offset,
						region_width,
						region_height,
						block_data.get(),
						block_info.x_offset,
						block_info.y_offset,
						block_info.width,
						block_info.height);

				blocks_in_region.pop();
			}
		}


		template<typename T>
		void
		copy_block_data_into_region(
				T *region_data,
				const unsigned int region_x_offset,
				const unsigned int region_y_offset,
				const unsigned int region_width,
				const unsigned int region_height,
				const T *block_data,
				const unsigned int block_x_offset,
				const unsigned int block_y_offset,
				const unsigned int block_width,
				const unsigned int block_height) const
		{
			PROFILE_FUNC();

			unsigned int copy_width;
			unsigned int copy_height;

			if (region_y_offset > block_y_offset)
			{
				block_data += (region_y_offset - block_y_offset) * block_width;

				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						region_y_offset < block_y_offset + block_height,
						GPLATES_ASSERTION_SOURCE);
				copy_height = block_y_offset + block_height - region_y_offset;
			}
			else // region_y_offset <= block_y_offset ...
			{
				region_data += (block_y_offset - region_y_offset) * region_width;

				copy_height = region_y_offset + region_height - block_y_offset;
				if (copy_height > block_height)
				{
					copy_height = block_height;
				}
			}

			if (region_x_offset > block_x_offset)
			{
				block_data += region_x_offset - block_x_offset;

				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						region_x_offset < block_x_offset + block_width,
						GPLATES_ASSERTION_SOURCE);
				copy_width = block_x_offset + block_width - region_x_offset;
			}
			else // region_x_offset <= block_x_offset ...
			{
				region_data += block_x_offset - region_x_offset;

				copy_width = region_x_offset + region_width - block_x_offset;
				if (copy_width > block_width)
				{
					copy_width = block_width;
				}
			}

			// Copy the block data to the region row-by-row.
			for (unsigned int y = 0; y < copy_height; ++y)
			{
				// Copy a row from the block.
				std::memcpy(region_data, block_data, copy_width * sizeof(T));
				region_data += region_width;
				block_data += block_width;
			}
		}


		template<typename T>
		void
		read_block_data(
				T *data,
				unsigned int num_elements) const
		{
			PROFILE_FUNC();

			// NOTE: We bypass the expensive output operator '>>' and hence is *much*
			// faster than doing a loop with '>>' (as determined by profiling).
			// We have to do our own endian conversion though.
			void *const raw_data = data;
			const int bytes_read = d_in.readRawData(static_cast<char *>(raw_data), num_elements * sizeof(T));
			if (bytes_read != num_elements * sizeof(T))
			{
				throw GPlatesGlobal::LogException(
						GPLATES_EXCEPTION_SOURCE,
						"Error reading block data from raster file cache mipmap.");
			}

			convert_from_big_endian(data, num_elements, sizeof(T));
		}


		void
		convert_from_big_endian(
				GPlatesGui::rgba8_t *data,
				unsigned int num_elements,
				unsigned int element_size) const
		{
			// Note that GPlatesGui::rgba8_t stores 4 bytes in memory as (R,G,B,A) and the
			// data is read from the stream as bytes (not 32-bit integers) so there's
			// no need to re-order the bytes according to the endianess of the current system.
		}


		void
		convert_from_big_endian(
				void *data,
				unsigned int num_elements,
				unsigned int element_size) const
		{
			PROFILE_FUNC();

			// If runtime system is big-endian then no need to convert.
			if (QSysInfo::ByteOrder == QSysInfo::BigEndian)
			{
				return;
			}

			if (element_size == 1)
			{
				return;
			}

			if (element_size == 2)
			{
				quint16 *data_ptr = static_cast<quint16 *>(data);
				for (unsigned int n = 0; n < num_elements; ++n)
				{
					const quint16 element = data_ptr[n];
					data_ptr[n] = ((element & 0xff00) >> 8) | ((element & 0x00ff) << 8);
				}
			}
			else if (element_size == 4)
			{
				quint32 *data_ptr = static_cast<quint32 *>(data);
				for (unsigned int n = 0; n < num_elements; ++n)
				{
					const quint32 element = data_ptr[n];
					data_ptr[n] =
							((element & 0xff000000) >> 24) |
							((element & 0x00ff0000) >> 8) |
							((element & 0x0000ff00) << 8) |
							((element & 0x000000ff) << 24);
				}
			}
			else // element_size == 8 ...
			{
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						element_size == 8,
						GPLATES_ASSERTION_SOURCE);

				// Simulate 64-bit with 32-bit just in case 64-bit arithmetic is not available.
				quint32 *data_ptr = static_cast<quint32 *>(data);
				for (unsigned int n = 0; n < num_elements; ++n)
				{
					// The high and low 32-bit words of the 64-bit word.
					const quint32 element_hi = data_ptr[(n << 1)];
					const quint32 element_lo = data_ptr[(n << 1) + 1];

					data_ptr[(n << 1)] =
							((element_lo & 0xff000000) >> 24) |
							((element_lo & 0x00ff0000) >> 8) |
							((element_lo & 0x0000ff00) << 8) |
							((element_lo & 0x000000ff) << 24);
					data_ptr[(n << 1) + 1] =
							((element_hi & 0xff000000) >> 24) |
							((element_hi & 0x00ff0000) >> 8) |
							((element_hi & 0x0000ff00) << 8) |
							((element_hi & 0x000000ff) << 24);
				}
			}
		}


		QFile &d_file;
		QDataStream &d_in;
		unsigned int d_image_width;
		unsigned int d_image_height;
		bool d_has_coverage;

		RasterFileCacheFormat::BlockInfos d_block_infos;
		boost::optional<raster_element_type> d_no_data_value;
		boost::optional<GPlatesPropertyValues::RasterStatistics> d_raster_statistics;
	};
}

#endif // GPLATES_FILE_IO_RASTERFILECACHEFORMATREADER_H
