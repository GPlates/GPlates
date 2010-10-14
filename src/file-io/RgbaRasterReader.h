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
#if _MSC_VER == 1600 // For Boost 1.44 and Visual Studio 2010.
#	undef UINT8_C
#endif
#include <boost/function.hpp>
#include <boost/optional.hpp>
#if _MSC_VER == 1600 // For Boost 1.44 and Visual Studio 2010.
#	undef UINT8_C
#	include <cstdint>
#endif
#include <QSize>
#include <QFile>
#include <QDataStream>

#include "RasterReader.h"
#include "RasterBandReaderHandle.h"


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
				const boost::function<RasterBandReaderHandle (unsigned int)> &proxy_handle_function,
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

		virtual
		void *
		get_data(
				unsigned int band_number,
				const QRect &region,
				ReadErrorAccumulation *read_errors);

	private:

		/**
		 * Reads a region from a RGBA file.
		 *
		 * Returns NULL if @a region exceeds the boundaries of the source raster.
		 */
		GPlatesGui::rgba8_t *
		read_rgba_file(
				const QRect &region);

		/**
		 * This function ensures that there is a file, either in the same directory as
		 * the source raster or in the temp directory, that is an uncompressed RGBA
		 * representation of the source raster.
		 *
		 * This is to allow quicker lookups of regions of the source raster.
		 *
		 * Returns true if at the conclusion of the function such a file is available.
		 */
		bool
		ensure_rgba_file_available();

		void
		report_recoverable_error(
				ReadErrorAccumulation *read_errors,
				ReadErrors::Description description);

		void
		report_failure_to_begin(
				ReadErrorAccumulation *read_errors,
				ReadErrors::Description description);

		QString d_filename;
		boost::function<RasterBandReaderHandle (unsigned int)> d_proxy_handle_function;
		QFile d_rgba_file;
		QDataStream d_rgba_in;
		unsigned int d_source_width;
		unsigned int d_source_height;
		bool d_can_read;
	};
}

#endif  // GPLATES_FILEIO_RGBARASTERREADER_H
