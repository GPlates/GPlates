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

#ifndef GPLATES_FILEIO_RASTERBANDREADER_H
#define GPLATES_FILEIO_RASTERBANDREADER_H

#include <boost/optional.hpp>
#include <QRect>

#include "global/PointerTraits.h"

#include "property-values/RasterType.h"


namespace GPlatesPropertyValues
{
	// Forward declaration.
	class RawRaster;
}

namespace GPlatesFileIO
{
	// Forward declarations.
	class RasterBandReaderHandle;
	class RasterReader;
	struct ReadErrorAccumulation;

	/**
	 * RasterBandReader is a wrapper around RasterReader that always reads the
	 * raster data from one particular band number.
	 *
	 * Its interface is similar to RasterReader, with the exception that the band
	 * number is not a parameter to any of the methods.
	 */
	class RasterBandReader
	{
	public:

		/**
		 * Constructs a RasterBandReader using an existing @a raster_reader, binding
		 * all reads to the given @a band_number.
		 *
		 * @a band_number must be greater than or equal to 1, and less than or equal
		 * to the number of bands in the source raster.
		 */
		RasterBandReader(
				const GPlatesGlobal::PointerTraits<RasterReader>::non_null_ptr_type &raster_reader,
				unsigned int band_number);

		RasterBandReader(
				const RasterBandReader &other);

		~RasterBandReader();

		const QString &
		get_filename() const;

		unsigned int
		get_band_number() const;

		bool
		can_read();

		boost::optional<GPlatesGlobal::PointerTraits<GPlatesPropertyValues::RawRaster>::non_null_ptr_type>
		get_proxied_raw_raster(
				ReadErrorAccumulation *read_errors = NULL);

		boost::optional<GPlatesGlobal::PointerTraits<GPlatesPropertyValues::RawRaster>::non_null_ptr_type>
		get_raw_raster(
				const QRect &region = QRect(),
				ReadErrorAccumulation *read_errors = NULL);

	private:

		GPlatesPropertyValues::RasterType::Type
		get_type(
				ReadErrorAccumulation *read_errors = NULL);

		void *
		get_data(
				const QRect &region = QRect(),
				ReadErrorAccumulation *read_errors = NULL);

		friend class RasterBandReaderHandle;

		GPlatesGlobal::PointerTraits<RasterReader>::non_null_ptr_type d_raster_reader;
		unsigned int d_band_number;
	};
}

#endif  // GPLATES_FILEIO_RASTERBANDREADER_H
