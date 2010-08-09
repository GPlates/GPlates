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

#include <boost/function.hpp>
#include <QString>

#include "Gdal.h"
#include "RasterBandReaderHandle.h"
#include "RasterReader.h"


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
				const boost::function<RasterBandReaderHandle (unsigned int)> &proxy_handle_function,
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

		virtual
		void *
		get_data(
				unsigned int band_number,
				const QRect &region,
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
	
		QString d_filename;

		/**
		 * Handle to the raster file. NULL if file open failed.
		 *
		 * Memory released by @a GDALClose call in our destructor.
		 */
		GDALDataset *d_dataset;

		boost::function<RasterBandReaderHandle (unsigned int)> d_proxy_handle_function;

		/*
		 * GMT style GRDs are stored, and imported, upside-down.
		 * See for example http://trac.osgeo.org/gdal/ticket/1926
		 *
		 * Note that this is a setting that applies to the entire file, not to
		 * each band.
		 */
		bool d_flip;
	};
}

#endif  // GPLATES_FILEIO_GDALRASTERREADER_H
