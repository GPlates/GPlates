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

#ifndef GPLATES_FILEIO_GDALRASTERREADER_H
#define GPLATES_FILEIO_GDALRASTERREADER_H

#include <vector>
#include <boost/function.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <QString>
#include <QtGlobal>

#include "Gdal.h"
#include "RasterBandReaderHandle.h"
#include "RasterReader.h"
#include "SourceRasterFileCacheFormatReader.h"


namespace GPlatesFileIO
{
	/**
	 * Reads rasters using GDAL.
	 */
	class GDALRasterReader :
			public RasterReaderImpl
	{
	public:

		GDALRasterReader(
				const QString &filename,
				RasterReader *raster_reader,
				ReadErrorAccumulation *read_errors);

		~GDALRasterReader();

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

		GDALRasterBand *
		get_raster_band(
				unsigned int band_number,
				ReadErrorAccumulation *read_errors);

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
		boost::shared_ptr<GPlatesFileIO::SourceRasterFileCacheFormatReader>
		create_source_raster_file_cache_format_reader(
				unsigned int band_number,
				ReadErrorAccumulation *read_errors);

		boost::shared_ptr<SourceRasterFileCacheFormatReader>
		create_source_raster_file_cache_format_reader(
				unsigned int band_number,
				const QString &cache_filename,
				ReadErrorAccumulation *read_errors);

		/**
		 * Creates a raster file cache for the source raster (returns false if unsuccessful).
		 */
		bool
		create_source_raster_file_cache(
				unsigned int band_number,
				ReadErrorAccumulation *read_errors);

		template <class RawRasterType>
		void
		write_source_raster_file_cache(
				unsigned int band_number,
				const QString &cache_filename,
				ReadErrorAccumulation *read_errors);

		template <class RawRasterType>
		void
		write_source_raster_file_cache_image_data(
				unsigned int band_number,
				QFile &cache_file,
				QDataStream &out,
				RasterFileCacheFormat::BlockInfos &block_infos,
				ReadErrorAccumulation *read_errors,
				double &raster_min,
				double &raster_max,
				double &raster_sum,
				double &raster_sum_squares,
				qint64 &num_valid_raster_samples);

		/**
		 * Traverse the Hilbert curve of blocks of the source raster using quad-tree recursion.
		 *
		 * The leaf nodes of the traversal correspond to the blocks in the source raster.
		 */
		template <class RawRasterType>
		void
		hilbert_curve_traversal(
				unsigned int band_number,
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
				boost::optional<typename RawRasterType::non_null_ptr_type> source_region_data,
				QRect source_region,
				ReadErrorAccumulation *read_errors,
				double &raster_min,
				double &raster_max,
				double &raster_sum,
				double &raster_sum_squares,
				qint64 &num_valid_raster_samples);


		// The minimum image allocation size to attempt - any image allocation lower than this size
		// that fails will result in a thrown exception. Note that if an allocation fails then
		// an allocation with half the dimensions will be attempted (and so on) unless the halved
		// dimension image is less than the minimum allocation size.
		static const int MIN_IMAGE_ALLOCATION_BYTES_TO_ATTEMPT = 50 * 1000 * 1000;

		// Currently removing this upper limit regardless of paging problems because GDAL reads
		// each row very slowly when requesting sections compared to just reading the entire file
		// in one go (most likely due to the file seeks required at the end of each row to seek to
		// the next row of the sub-section - or GDAL reads the entire row of the full-size image
		// even if a sub-section is requested).
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

		/**
		 * Handle to the raster file. NULL if file open failed.
		 *
		 * Memory released by @a GDALClose call in our destructor.
		 */
		GDALDataset *d_dataset;

		/*
		 * GMT style GRDs are stored, and imported, upside-down.
		 * See for example http://trac.osgeo.org/gdal/ticket/1926
		 *
		 * However, when we say that they are upside down, we mean upside down with
		 * respect to the convention that the first row of the raster is stored in the
		 * last scanline. In GPlates, we store rasters from top to bottom.
		 * Therefore, @a d_flip is false iff the GRD is GMT-style.
		 *
		 * Note that this is a setting that applies to the entire file, not to
		 * each band.
		 */
		bool d_flip;

		unsigned int d_source_width;
		unsigned int d_source_height;

		/**
		 * A source raster file cache reader for each raster band.
		 */
		std::vector<boost::shared_ptr<SourceRasterFileCacheFormatReader> > d_raster_band_file_cache_format_readers;
	};
}

#endif  // GPLATES_FILEIO_GDALRASTERREADER_H
