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

#ifndef GPLATES_FILEIO_RASTERBANDREADERHANDLE_H
#define GPLATES_FILEIO_RASTERBANDREADERHANDLE_H

#include "RasterBandReader.h"


namespace GPlatesFileIO
{
	/**
	 * This class acts as a bridge between RasterBandReader and proxied RawRasters.
	 */
	class RasterBandReaderHandle
	{
	public:

		explicit
		RasterBandReaderHandle(
				const RasterBandReader &raster_band_reader);

		const QString &
		get_filename() const;

		unsigned int
		get_band_number() const;

		bool
		can_read();

		GPlatesPropertyValues::RasterType::Type
		get_type(
				ReadErrorAccumulation *read_errors = NULL);

		boost::optional<GPlatesGlobal::PointerTraits<GPlatesPropertyValues::RawRaster>::non_null_ptr_type>
		get_raw_raster(
				const QRect &region = QRect(),
				ReadErrorAccumulation *read_errors = NULL);

	private:

		RasterBandReader d_raster_band_reader;
	};
}

#endif  // GPLATES_FILEIO_RASTERBANDREADERHANDLE_H
