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
#include <boost/variant.hpp>
#include <QString>
#include <QtGlobal>

#include "Gdal.h"
#include "RasterBandReaderHandle.h"
#include "RasterReader.h"
#include "SourceRasterFileCacheFormatReader.h"

#include "gui/Colour.h"


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
		boost::optional<GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type>
		get_georeferencing();

		virtual
		boost::optional<GPlatesPropertyValues::SpatialReferenceSystem::non_null_ptr_to_const_type>
		get_spatial_reference_system();

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

		/**
		 * Raster band information.
		 *
		 * Colour raster bands (an R band, G band and B band) are merged into a single RGBA pseudo-raster-band.
		 * So the mapping to GDALDataset bands is not necessarily one-to-one to the GDAL bands.
		 */
		struct RasterBand
		{
			//! If this band comes from R,G,B (and optionally A) colour GDAL bands.
			struct GDALRgbaBands
			{
				GDALDataType band_data_type; //!< All bands have the same data type.
				GDALRasterBand *red_band;
				GDALRasterBand *green_band;
				GDALRasterBand *blue_band;
				boost::optional<GDALRasterBand *> alpha_band;
			};

			//! This band is either a single GDAL band or RGB[A] GDAL bands.
			typedef boost::variant<
					GDALRasterBand *,
					GDALRgbaBands> gdal_raster_band_type;


			explicit
			RasterBand(
					GPlatesPropertyValues::RasterType::Type raster_type_,
					const gdal_raster_band_type &gdal_raster_band_) :
				raster_type(raster_type_),
				gdal_raster_band(gdal_raster_band_)
			{  }

			GPlatesPropertyValues::RasterType::Type raster_type;
			gdal_raster_band_type gdal_raster_band;
			//! Source raster file cache reader.
			boost::shared_ptr<SourceRasterFileCacheFormatReader> file_cache_format_reader;
		};


		bool
		initialise_source_raster_dimensions();

		boost::optional<RasterBand::GDALRgbaBands>
		is_colour_raster();

		void
		report_recoverable_error(
				ReadErrorAccumulation *read_errors,
				ReadErrors::Description description);

		void
		report_failure_to_begin(
				ReadErrorAccumulation *read_errors,
				ReadErrors::Description description);

		template<class RawRasterType>
		GPlatesPropertyValues::RawRaster::non_null_ptr_type
		create_proxied_raw_raster(
				const RasterBand &raster_band,
				const GPlatesFileIO::RasterBandReaderHandle &raster_band_reader_handle,
				ReadErrorAccumulation *read_errors);

		/**
		 * Creates a reader for the cached source raster.
		 *
		 * If no cache exists, or it's out-of-date, etc then the cache is regenerated.
		 */
		boost::shared_ptr<GPlatesFileIO::SourceRasterFileCacheFormatReader>
		create_source_raster_file_cache_format_reader(
				RasterBand &raster_band,
				unsigned int band_number,
				ReadErrorAccumulation *read_errors);

		boost::shared_ptr<SourceRasterFileCacheFormatReader>
		create_source_raster_file_cache_format_reader(
				RasterBand &raster_band,
				const QString &cache_filename,
				ReadErrorAccumulation *read_errors);

		/**
		 * Creates a raster file cache for the source raster (returns false if unsuccessful).
		 */
		bool
		create_source_raster_file_cache(
				RasterBand &raster_band,
				unsigned int band_number,
				ReadErrorAccumulation *read_errors);

		template <class RawRasterType>
		void
		write_source_raster_file_cache(
				RasterBand &raster_band,
				const QString &cache_filename,
				ReadErrorAccumulation *read_errors);

		template <class RawRasterType>
		void
		write_source_raster_file_cache_image_data(
				RasterBand &raster_band,
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
		 * Returns the no-data value of the specified raster band.
		 *
		 * Returns false if raster band does not have a no-data value (this includes colour rasters).
		 */
		template <typename RasterElementType>
		bool
		get_no_data_value(
				const RasterBand &raster_band,
				RasterElementType &no_data_value);

		template <class RawRasterType>
		void
		add_no_data_value(
				RawRasterType &raster,
				const RasterBand &raster_band);

		template <class RawRasterType>
		boost::optional<GPlatesPropertyValues::RasterStatistics>
		get_statistics(
				RawRasterType &raster,
				const RasterBand &raster_band,
				ReadErrorAccumulation *read_errors);

		template <class RawRasterType>
		void
		add_statistics(
				RawRasterType &raster,
				const RasterBand &raster_band,
				ReadErrorAccumulation *read_errors);

		template<typename RasterElementType>
		void
		add_data(
				RasterElementType *result_buf,
				const RasterBand &raster_band,
				bool flip,
				unsigned int region_x_offset,
				unsigned int region_y_offset,
				unsigned int region_width,
				unsigned int region_height);

		template <typename RasterBandElementType>
		void
		add_rgba_data(
				GPlatesGui::rgba8_t *result_buf,
				const RasterBand::GDALRgbaBands &gdal_rgba_raster_bands,
				bool flip,
				unsigned int region_x_offset,
				unsigned int region_y_offset,
				unsigned int region_width,
				unsigned int region_height);

		template<class RawRasterType>
		boost::optional<typename RawRasterType::non_null_ptr_type>
		read_data(
				const RasterBand &raster_band,
				bool flip,
				const QRect &region);

		template <class RawRasterType>
		void
		update_statistics(
				RawRasterType &source_region_data,
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
				RasterBand &raster_band,
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

		std::vector<RasterBand> d_raster_bands;

		/**
		 * A source raster file cache reader for each raster band.
		 */
		std::vector<boost::shared_ptr<SourceRasterFileCacheFormatReader> > d_raster_band_file_cache_format_readers;
	};


	// Template specialisations are in .cc file.
	template <>
	bool
	GDALRasterReader::get_no_data_value<GPlatesGui::rgba8_t>(
			const GDALRasterReader::RasterBand &raster_band,
			GPlatesGui::rgba8_t &no_data_value);

	// Template specialisations are in .cc file.
	template <>
	boost::optional<GPlatesPropertyValues::RasterStatistics>
	GDALRasterReader::get_statistics<GPlatesPropertyValues::Rgba8RawRaster>(
			GPlatesPropertyValues::Rgba8RawRaster &raster,
			const GDALRasterReader::RasterBand &raster_band,
			ReadErrorAccumulation *read_errors);
	template <>
	boost::optional<GPlatesPropertyValues::RasterStatistics>
	GDALRasterReader::get_statistics<GPlatesPropertyValues::ProxiedRgba8RawRaster>(
			GPlatesPropertyValues::ProxiedRgba8RawRaster &raster,
			const GDALRasterReader::RasterBand &raster_band,
			ReadErrorAccumulation *read_errors);

	// Template specialisations are in .cc file.
	template <>
	void
	GDALRasterReader::add_data<GPlatesGui::rgba8_t>(
			GPlatesGui::rgba8_t *result_buf,
			const GDALRasterReader::RasterBand &raster_band,
			bool flip,
			unsigned int region_x_offset,
			unsigned int region_y_offset,
			unsigned int region_width,
			unsigned int region_height);

	// Template specialisations are in .cc file.
	template <>
	void
	GDALRasterReader::update_statistics<GPlatesPropertyValues::Rgba8RawRaster>(
			GPlatesPropertyValues::Rgba8RawRaster &source_region_data,
			double &raster_min,
			double &raster_max,
			double &raster_sum,
			double &raster_sum_squares,
			qint64 &num_valid_raster_samples);
	template <>
	void
	GDALRasterReader::update_statistics<GPlatesPropertyValues::ProxiedRgba8RawRaster>(
			GPlatesPropertyValues::ProxiedRgba8RawRaster &source_region_data,
			double &raster_min,
			double &raster_max,
			double &raster_sum,
			double &raster_sum_squares,
			qint64 &num_valid_raster_samples);
}

#endif  // GPLATES_FILEIO_GDALRASTERREADER_H
