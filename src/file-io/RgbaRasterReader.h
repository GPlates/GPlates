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

#ifndef GPLATES_FILEIO_RGBARASTERREADER_H
#define GPLATES_FILEIO_RGBARASTERREADER_H

#include <string>
#include <utility>
#include <boost/function.hpp>
#include <boost/optional.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_array.hpp>
#include <QSize>
#include <QFile>
#include <QDataStream>
#include <QImage>
#include <QImageReader>
#include <QtGlobal>

#include "SourceRasterFileCacheFormatReader.h"
#include "RasterReader.h"
#include "RasterBandReaderHandle.h"

#include "gui/Colour.h"


namespace GPlatesFileIO
{
	/**
	 * Reads RGBA rasters.
	 */
	class RgbaRasterReader :
			public RasterReaderImpl
	{
	public:

		RgbaRasterReader(
				const QString &filename,
				RasterReader *raster_reader,
				ReadErrorAccumulation *read_errors);

		virtual
		bool
		can_read();

		virtual
		unsigned int
		get_number_of_bands(
				ReadErrorAccumulation *read_errors);

		virtual
		std::pair<unsigned int, unsigned int>
		get_size(
				ReadErrorAccumulation *read_errors);

		virtual
		boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type>
		get_proxied_raw_raster(
				unsigned int band_number,
				ReadErrorAccumulation *read_errors);

		virtual
		boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type>
		get_raw_raster(
				unsigned int band_number,
				const QRect &region,
				ReadErrorAccumulation *read_errors);

		virtual
		GPlatesPropertyValues::RasterType::Type
		get_type(
				unsigned int band_number,
				ReadErrorAccumulation *read_errors);

	private:

		void
		report_recoverable_error(
				ReadErrorAccumulation *read_errors,
				ReadErrors::Description description);

		void
		report_failure_to_begin(
				ReadErrorAccumulation *read_errors,
				ReadErrors::Description description);

		/**
		 * Creates a reader for the cached source raster.
		 *
		 * If no cache exists, or it's out-of-date, etc then the cache is regenerated.
		 */
		void
		create_source_raster_file_cache_format_reader(
				ReadErrorAccumulation *read_errors);

		/**
		 * Creates a raster file cache for the source raster (returns false if unsuccessful).
		 */
		bool
		create_source_raster_file_cache(
				ReadErrorAccumulation *read_errors);

		void
		write_source_raster_file_cache(
				const QString &cache_filename,
				ReadErrorAccumulation *read_errors);

		void
		write_source_raster_file_cache_image_data(
				QFile &cache_file,
				QDataStream &out,
				RasterFileCacheFormat::BlockInfos &block_infos,
				ReadErrorAccumulation *read_errors);

		/**
		 * Traverse the Hilbert curve of blocks of the source raster using quad-tree recursion.
		 *
		 * The leaf nodes of the traversal correspond to the blocks in the source raster.
		 */
		void
		hilbert_curve_traversal(
				unsigned int depth,
				unsigned int read_source_raster_depth,
				unsigned int write_source_raster_depth,
				unsigned int x_offset,
				unsigned int y_offset,
				unsigned int dimension,
				unsigned int hilbert_start_point,
				unsigned int hilbert_end_point,
				QDataStream &out,
				RasterFileCacheFormat::BlockInfos &block_infos,
				boost::shared_array<GPlatesGui::rgba8_t> source_region_data,
				QRect source_region,
				ReadErrorAccumulation *read_errors);

		/**
		 * Reads source raster from the specified region.
		 *
		 * Returns NULL on memory allocation failure.
		 */
		boost::shared_array<GPlatesGui::rgba8_t> 
		read_source_raster_region(
				QImageReader &source_reader,
				const QRect &source_region,
				ReadErrorAccumulation *read_errors);


		// The minimum image allocation size to attempt - any image allocation lower than this size
		// that fails will result in a thrown exception. Note that if an allocation fails then
		// an allocation with half the dimensions will be attempted (and so on) unless the halved
		// dimension image is less than the minimum allocation size.
		static const int MIN_IMAGE_ALLOCATION_BYTES_TO_ATTEMPT = 100 * 1000 * 1000;

		// Currently removing this upper limit regardless of paging because currently
		// (as of Qt 4.6.1 - see http://bugreports.qt.nokia.com/browse/QTBUG-3249) only JPEG
		// supports this and looking at the source code it actually reads the entire image for
		// each cliprect request - it does it scanline-by-scanline so still uses low memory -
		// and just copies the relevant parts (cliprect) of the entire image into the
		// returned QImage. So we don't really want to do lots of cliprect requests
		// (each decoding the entire JPEG image) so we set the image size at which
		// we start using cliprects to be as high as we can.
		//
		// If the allocation fails we will repeatedly reduce the allocation size until
		// it reaches @a MIN_IMAGE_ALLOCATION_BYTES_TO_ATTEMPT.
#if 0
		// The maximum memory allocation to attempt for an image.
		// Going higher than this is likely to cause memory to start paging to disk
		// which will just slow things down.
		//
		// The 32-bit limit is to avoid integer overflow in 32-bit programs.
		static const quint64 MAX_IMAGE_ALLOCATION_BYTES_TO_ATTEMPT = Q_UINT64_C(0xffffffff);
#endif


		QString d_source_raster_filename;
		unsigned int d_source_width;
		unsigned int d_source_height;
		boost::scoped_ptr<SourceRasterFileCacheFormatReader> d_source_raster_file_cache_format_reader;
	};
}

#endif  // GPLATES_FILEIO_RGBARASTERREADER_H
