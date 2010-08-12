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

#ifndef GPLATES_FILEIO_MIPMAPPEDRASTERFORMAT_H
#define GPLATES_FILEIO_MIPMAPPEDRASTERFORMAT_H

#include <boost/cstdint.hpp>
#include <QDataStream>

#include "gui/Colour.h"


namespace GPlatesFileIO
{
	/**
	 * This namespace contains parameters that define the GPlates mipmapped raster
	 * format. This format is used to store mipmapped versions of rasters to enable
	 * faster retrieval of lower-resolution versions of rasters.
	 *
	 * One mipmapped raster file stores the mipmaps for one full-resolution raster,
	 * or if the full-resolution raster file contains a number of bands, stores the
	 * mipmaps for one band in that full-resolution raster file.
	 *
	 * A mipmapped raster file is a binary file that consists of a header followed
	 * by a succession of downsampled images, each with half the width and half the
	 * height of the previous. The first image has half the width and height of the
	 * original raster; the original raster is not stored in the mipmapped raster
	 * file. The sequence of images ends when the greatest dimension of the last
	 * image is less than a certain threshold. If the greatest dimension of the
	 * original raster is less than that threshold, no mipmapped raster file is
	 * created for it.
	 *
	 * If the original raster is an RGBA raster, the mipmaps are in RGBA.
	 * If the original raster is an integer or float (assumed to be 32-bit) raster,
	 * the mipmaps are stored as floats.
	 * If the original raster is a double (assumed to be 64-bit) raster, the
	 * mipmaps are stored as doubles.
	 *
	 * For each mipmap stored as floats or doubles, where there is at least one
	 * pixel that corresponds to, in the original raster, a mixture of sentinel and
	 * non-sentinel values, there is a coverage raster. The coverage raster is a
	 * 16-bit integer raster that stores the fraction of the corresponding pixel in
	 * the mipmap that is non-sentinel in the original raster.
	 *
	 * The header consists of the following fields, in order:
	 *  - ( 0) A magic number that identifies a file as a GPlates mipmapped raster.
	 *  - ( 4) The version number of the GPlates mipmapped raster format used.
	 *  - ( 8) The type of the mipmaps: RGBA, float or double.
	 *  - (12) The number of levels.
	 *  - (16) For each level:
	 *     - The width of the mipmap in this level.
	 *     - The height of the mipmap in this level.
	 *     - The starting position, in bytes, of the mipmap in the file.
	 *     - The starting position, in bytes, of the coverage raster in the file.
	 *       This value is 0 if the mipmap is RGBA, or where the float/double
	 *       mipmap does not have any pixels that are part sentinel and part
	 *       non-sentinel.
	 *
	 * Each of the fields in the header is an unsigned 32-bit integer.
	 * Each RGBA component is stored as an unsigned 8-bit integer.
	 * The byte order of the entire mipmapped raster file is big endian
	 * (the QDataStream default).
	 * The file format is independent of the operating system and CPU, with one
	 * qualification: float is assumed to be 32-bit and double is assumed to be
	 * 64-bit.
	 */
	namespace MipmappedRasterFormat
	{
		/**
		 * The magic number that identifies a file as a GPlates mimapped raster.
		 */
		const boost::uint32_t MAGIC_NUMBER = 0x00F00BAA;

		/**
		 * The current version number of the GPlates mimapped raster format.
		 *
		 * NOTE: This must be updated if there are any breaking changes to the file
		 * format between public GPlates releases.
		 */
		const boost::uint32_t VERSION_NUMBER = 1;

		/**
		 * The type of raster used to store the mipmaps.
		 */
		enum Type
		{
			RGBA,
			FLOAT,
			DOUBLE,

			NUM_TYPES // must be the last entry
		};

		template<typename T>
		Type
		get_type_as_enum();

		template<>
		Type
		get_type_as_enum<GPlatesGui::rgba8_t>();

		template<>
		Type
		get_type_as_enum<float>();

		template<>
		Type
		get_type_as_enum<double>();

		/**
		 * The threshold size is the value such that the greatest dimension in the
		 * lowest level is less than or equal to this.
		 */
		const unsigned int THRESHOLD_SIZE = 64;

		/**
		 * The QDataStream serialisation version.
		 */
		const int Q_DATA_STREAM_VERSION = QDataStream::Qt_4_4;
	
		struct LevelInfo
		{
			quint32 width, height, main_offset, coverage_offset;
			static const quint32 NUM_COMPONENTS = 4;
		};
	}
}

#endif  // GPLATES_FILEIO_MIPMAPPEDRASTERFORMAT_H
