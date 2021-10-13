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

#include <vector>
#include <boost/cstdint.hpp>
#include <boost/optional.hpp>
#include <QtGlobal>
#include <QDataStream>
#include <QString>

#include "global/AssertionFailureException.h"
#include "global/GPlatesException.h"

#include "gui/Colour.h"
#include "gui/RasterColourPalette.h"


namespace GPlatesFileIO
{
	/**
	 * This namespace contains parameters that define the GPlates cached/mipmapped raster
	 * format. This format is used to store block-encoded versions of rasters to enable
	 * faster retrieval of the original raster and its lower-resolution mipmaps.
	 *
	 * One raster file cache stores a copy of the original raster in block-encoded format,
	 * or if the original raster file contains a number of bands, stores a copy of one band
	 * in that original raster file. This is the base (full-resolution) level of the mipmap pyramid.
	 *
	 * Another raster file cache stores the mipmaps for the original full-resolution raster,
	 * or if the full-resolution raster file contains a number of bands, stores the
	 * mipmaps for one band in that full-resolution raster file.
	 *
	 * A raster file cache is a binary file that consists of a header followed by block-encoded
	 * image data (either the full-resolution raster or a succession of downsampled
	 * images, each with half the width and half the height of the previous).
	 * In the latter case (mipmaps) the first image has half the width and height of the
	 * original raster; the original raster is not stored in the mipmapped raster
	 * file. The sequence of images ends when the greatest dimension of the last
	 * image is less than the block dimension. If the greatest dimension of the
	 * original raster is less than that block dimension, no mipmapped raster file cache is
	 * created for it (since it's not necessary).
	 *
	 * If the original raster is an RGBA raster, the mipmaps are in RGBA.
	 * If the original raster is an integer or float (assumed to be 32-bit) raster,
	 * the mipmaps are stored as floats.
	 * If the original raster is a double (assumed to be 64-bit) raster, the
	 * mipmaps are stored as doubles.
	 *
	 * For each cached image (mipmaps and base level) stored as floats or doubles there is a coverage
	 * raster even if there are no pixels that correspond to, in the original raster, the sentinel value.
	 * The coverage raster is a 16-bit integer raster that stores the fraction of the corresponding
	 * pixel in the cached image that is non-sentinel in the original raster.
	 *
	 * For the base level raster file cache...
	 *
	 * The header consists of the following fields, in order:
	 *  - ( 0) A magic number that identifies a file as GPlates.
	 *  - ( 8) The version number of the GPlates raster file cache format used.
	 *  - (12) The type of the source raster: RGBA, float or double.
	 *  - (16) For the base level:
	 *     - The width of the image in this level.
	 *     - The height of the image in this level.
	 *     - The starting position, in bytes, of the encoded source raster data in the file.
	 *
	 * For the mipmaps raster file cache...
	 *
	 * The header consists of the following fields, in order:
	 *  - ( 0) A magic number that identifies a file as GPlates.
	 *  - ( 8) The version number of the GPlates raster file cache format used.
	 *  - (12) The type of the mipmaps: RGBA, float or double.
	 *  - (16) The number of levels.
	 *  - (20) For each level:
	 *     - The width of the mipmap in this level.
	 *     - The height of the mipmap in this level.
	 *     - The starting position, in bytes, of the encoded mipmap data in the file.
	 *
	 * Most of the fields in the header are unsigned 32-bit integers.
	 * Each RGBA component is stored as an unsigned 8-bit integer.
	 * The byte order of the entire raster file cache is big endian (the QDataStream default).
	 * The file format is independent of the operating system and CPU, with one
	 * qualification: float is assumed to be 32-bit and double is assumed to be
	 * 64-bit.
	 */
	namespace RasterFileCacheFormat
	{
		/**
		 * The magic number that identifies a file as GPlates.
		 */
		const boost::uint8_t MAGIC_NUMBER[] = { 'G', 'P', 'l', 'a', 't', 'e', 's', 0 };

		/**
		 * The current version number of the GPlates raster file cache format.
		 *
		 * NOTE: This must be updated if there are any breaking changes to the file
		 * format between public GPlates releases.
		 * For example adding a new parameter to the file or updating a block-decoding algorithm.
		 *
		 * The same version number is used for both mipmap and source raster file caches (separate files).
		 * This means a change to the mipmap raster file format, for example, requires incrementing
		 * the version number which also affects the source raster file format (even if it hasn't changed).
		 * But this is OK since each file format can test sub-ranges of version numbers and perform
		 * backwards compatible reading of raster file caches as needed.
		 */
		const boost::uint32_t VERSION_NUMBER = 1;

		/**
		 * The type of raster used to store.
		 */
		enum Type
		{
			RGBA,
			FLOAT,
			DOUBLE,
			UINT8,
			UINT16,
			INT16,
			UINT32,
			INT32,

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

		template<>
		Type
		get_type_as_enum<quint8>();

		template<>
		Type
		get_type_as_enum<quint16>();

		template<>
		Type
		get_type_as_enum<qint16>();

		template<>
		Type
		get_type_as_enum<quint32>();

		template<>
		Type
		get_type_as_enum<qint32>();

		/**
		 * The block size is the value is dimension of square blocks of image data, in the raster
		 * file cache, containing BLOCK_SIZE x BLOCK_SIZE pixels of data.
		 *
		 * It is also such that the greatest dimension in the lowest level is less than or equal to this.
		 *
		 * This is set to a size of OpenGL texture that all hardware platforms support.
		 * They all support 256 x 256 textures.
		 */
		const unsigned int BLOCK_SIZE = 256;

		/**
		 * The QDataStream serialisation version.
		 */
		const int Q_DATA_STREAM_VERSION = QDataStream::Qt_4_4;
	
		/**
		 * Information for the size and file location of a level (base or mipmap) of the mipmap pyramid.
		 *
		 * Note the base level and the mipmap levels are in separate files.
		 */
		struct LevelInfo
		{
			quint32 width, height;
			quint64 blocks_file_offset;
			quint32 num_blocks;

			// Size of sum of individual data members.
			// This is not necessarily equal to the size of the structure due to alignment reasons.
			static const unsigned int STREAM_SIZE = 3 * sizeof(quint32) + sizeof(quint64);
		};

		/**
		 * Information for a block of encoded data.
		 */
		struct BlockInfo
		{
			// Pixel offsets locating block within the image of the source (or mipmapped) raster.
			quint32 x_offset, y_offset;
			// Most blocks have @a BLOCK_SIZE dimensions except those at right and bottom edges of source raster.
			quint32 width, height;
			// Offset within level of encoded data for the source (or mipmapped) raster.
			quint64 main_offset;
			// Offset within level of encoded data for the coverage (or mipmapped) raster.
			// This is zero for source raster formats that don't require a separate coverage (ie, RGBA).
			quint64 coverage_offset;

			// Size of sum of individual data members.
			// This is not necessarily equal to the size of the structure due to alignment reasons.
			static const unsigned int STREAM_SIZE = 4 * sizeof(quint32) + 2 * sizeof(quint64);
		};


		/**
		 * Keeps track of encoded blocks within an image.
		 */
		class BlockInfos
		{
		public:
			//! Constructor allocates *un-initialised* @a BlockInfo structures.
			BlockInfos(
					unsigned int image_width,
					unsigned int image_height);

			//! Returns the number of blocks.
			unsigned int
			get_num_blocks() const;

			//! Returns specified block.
			const BlockInfo &
			get_block_info(
					unsigned int block_x_offset,
					unsigned int block_y_offset) const;

			//! Returns specified non-const block.
			BlockInfo &
			get_block_info(
					unsigned int block_x_offset,
					unsigned int block_y_offset);

			//! Returns specified block.
			const BlockInfo &
			get_block_info(
					unsigned int block_index) const;

			//! Returns specified non-const block.
			BlockInfo &
			get_block_info(
					unsigned int block_index);

		private:
			unsigned int d_num_blocks_in_x_direction;
			unsigned int d_num_blocks_in_y_direction;
			std::vector<BlockInfo> d_block_infos;
		};


		/**
		 * Returns the number of mipmapped levels in total needed for a source raster of
		 * the specified dimensions.
		 *
		 * NOTE: This does *not* include the base level (full resolution).
		 */
		unsigned int
		get_number_of_mipmapped_levels(
				const unsigned int source_raster_width,
				const unsigned int source_raster_height);


		/**
		 * Returns the mipmap image dimensions for the specified source raster dimensions and mipmap level.
		 *
		 * NOTE: Level 0 is *not* the base level (full resolution).
		 * Instead it is the first filtered/reduced mipmap level (half dimensions of full resolution).
		 */
		void
		get_mipmap_dimensions(
				unsigned int &mipmap_width,
				unsigned int &mipmap_height,
				unsigned int mipmap_level,
				const unsigned int source_raster_width,
				const unsigned int source_raster_height);


		/**
		 * Returns the filename of a file that can be used for writing out a
		 * mipmaps file for the given @a source_filename.
		 *
		 * It first checks whether a mipmap file in the same directory as the
		 * source raster is writable. If not, it will check whether a mipmap
		 * file in the temp directory is writable. In the rare case in which the
		 * user has no permissions to write in the temp directory, boost::none is returned.
		 */
		boost::optional<QString>
		get_writable_mipmap_cache_filename(
				const QString &source_filename,
				unsigned int band_number,
				boost::optional<std::size_t> colour_palette_id = boost::none);


		/**
		 * Returns the filename of an existing mipmap file for the given
		 * @a source_filename, if any.
		 *
		 * It first checks in the same directory as the source raster. If it is
		 * not found there, it then checks in the temp directory. If the mipmaps
		 * file is not found in either of those two places, boost::none is returned.
		 */
		boost::optional<QString>
		get_existing_mipmap_cache_filename(
				const QString &source_filename,
				unsigned int band_number,
				boost::optional<std::size_t> colour_palette_id = boost::none);


		/**
		 * Returns the filename of a file that can be used for writing out a
		 * source raster file cache for the given @a source_filename.
		 *
		 * It first checks whether a source raster file cache in the same directory as the
		 * source raster is writable. If not, it will check whether a source raster file cache
		 * file in the temp directory is writable. In the rare case in which the
		 * user has no permissions to write in the temp directory, boost::none is returned.
		 */
		boost::optional<QString>
		get_writable_source_cache_filename(
				const QString &source_filename,
				unsigned int band_number);


		/**
		 * Returns the filename of an existing source raster file cache for the given
		 * @a source_filename, if any.
		 *
		 * It first checks in the same directory as the source raster. If it is
		 * not found there, it then checks in the temp directory. If the source raster file cache
		 * is not found in either of those two places, boost::none is returned.
		 */
		boost::optional<QString>
		get_existing_source_cache_filename(
				const QString &source_filename,
				unsigned int band_number);


		/**
		 * Gets the colour palette id for the given @a colour_palette.
		 *
		 * It simply casts the memory address of the colour palette and casts it
		 * to an unsigned int.
		 */
		boost::optional<std::size_t>
		get_colour_palette_id(
				const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &colour_palette =
					GPlatesGui::RasterColourPalette::create());


		/**
		 * Thrown when reading a file containing an unrecognised version number.
		 *
		 * This happens after reading the magic number so we're fairly sure it's a file that GPlates wrote.
		 *
		 * Most likely we're this is an old version of GPlates and we're reading a file generated
		 * by a newer version of GPlates.
		 */
		class UnsupportedVersion :
				public GPlatesGlobal::Exception
		{
		public:
			UnsupportedVersion(
				const GPlatesUtils::CallStack::Trace &exception_source,
				quint32 unrecognised_version);

			quint32
			unrecognised_version() const
			{
				return d_unrecognised_version;
			}

		protected:
			virtual
				const char *
				exception_name() const;

			virtual
				void
				write_message(
				std::ostream &os) const;

		private:
			quint32 d_unrecognised_version;
		};
	}
}

#endif  // GPLATES_FILEIO_MIPMAPPEDRASTERFORMAT_H
