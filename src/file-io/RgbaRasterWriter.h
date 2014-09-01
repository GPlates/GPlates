/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2014 The University of Sydney, Australia
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

#ifndef GPLATES_FILE_IO_RGBARASTERWRITER_H
#define GPLATES_FILE_IO_RGBARASTERWRITER_H

#include <QImage>
#include <QString>

#include "RasterWriter.h"

#include "property-values/RasterType.h"


namespace GPlatesFileIO
{
	/**
	 * Writes RGBA rasters (with *no* support for georeferencing or spatial reference systems).
	 */
	class RgbaRasterWriter :
			public RasterWriterImpl
	{
	public:

		/**
		 * Adds information about the formats supported by this writer.
		 */
		static
		void
		get_supported_formats(
				RasterWriter::supported_formats_type &supported_formats);


		RgbaRasterWriter(
				const QString &filename,
				const RasterWriter::FormatInfo &format_info,
				unsigned int raster_width,
				unsigned int raster_height,
				unsigned int num_raster_bands,
				GPlatesPropertyValues::RasterType::Type raster_band_type);

		virtual
		bool
		can_write() const;

		virtual
		void
		set_georeferencing(
				const GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type &georeferencing);

		virtual
		void
		set_spatial_reference_system(
				const GPlatesPropertyValues::SpatialReferenceSystem::non_null_ptr_to_const_type& srs);

		virtual
		bool
		write_region_data(
				const GPlatesPropertyValues::RawRaster::non_null_ptr_type &region_data,
				unsigned int band_number,
				unsigned int x_offset,
				unsigned int y_offset);

		virtual
		bool
		write_file();

	private:

		QString d_filename;
		QImage d_image;
	};
}

#endif // GPLATES_FILE_IO_RGBARASTERWRITER_H
