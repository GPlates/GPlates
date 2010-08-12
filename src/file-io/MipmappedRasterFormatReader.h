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

#ifndef GPLATES_FILEIO_MIPMAPPEDRASTERFORMATREADER_H
#define GPLATES_FILEIO_MIPMAPPEDRASTERFORMATREADER_H

#include <vector>
#include <boost/scoped_ptr.hpp>
#include <boost/optional.hpp>
#include <QString>
#include <QFile>
#include <QFileInfo>
#include <QDataStream>
#include <QDebug>

#include "ErrorOpeningFileForReadingException.h"
#include "FileFormatNotSupportedException.h"
#include "MipmappedRasterFormat.h"

#include "gui/Colour.h"

#include "property-values/RawRaster.h"


namespace GPlatesFileIO
{
	/**
	 * MipmappedRasterFormatReader reads mipmapped images from a mipmapped raster
	 * file. It is able to read a given region of a given mipmap level.
	 *
	 * The template parameter @a RawRasterType is the type of the *mipmapped*
	 * rasters stored in the file, not the type of the source raster.
	 */
	template<class RawRasterType>
	class MipmappedRasterFormatReader
	{
	public:

		/**
		 * Opens @a filename for reading as a mipmapped raster file.
		 *
		 * @throws @a ErrorOpeningFileForReadingException if @a filename could not be
		 * opened for reading.
		 *
		 * @throws @a FileFormatNotException if the header information is wrong.
		 */
		MipmappedRasterFormatReader(
				const QString &filename) :
			d_file(filename),
			d_in(&d_file),
			d_is_closed(false)
		{
			// Attempt to open the file for reading.
			if (!d_file.open(QIODevice::ReadOnly))
			{
				throw ErrorOpeningFileForReadingException(
						GPLATES_EXCEPTION_SOURCE, filename);
			}

			// Check that there is enough data in the file for magic number and version.
			QFileInfo file_info(d_file);
			static const qint64 MAGIC_NUMBER_AND_VERSION_SIZE = 8;
			if (file_info.size() < MAGIC_NUMBER_AND_VERSION_SIZE)
			{
				throw FileFormatNotSupportedException(
						GPLATES_EXCEPTION_SOURCE, "bad header");
			}

			// Check the magic number.
			quint32 magic_number;
			d_in >> magic_number;
			if (magic_number != MipmappedRasterFormat::MAGIC_NUMBER)
			{
				throw FileFormatNotSupportedException(
						GPLATES_EXCEPTION_SOURCE, "bad magic number");
			}

			// Check the version number.
			quint32 version_number;
			d_in >> version_number;
			if (version_number == 1)
			{
				d_impl.reset(new VersionOneReader(d_file, d_in));
			}
			else
			{
				throw FileFormatNotSupportedException(
						GPLATES_EXCEPTION_SOURCE, "bad version number");
			}
		}

		~MipmappedRasterFormatReader()
		{
			if (!d_is_closed)
			{
				d_file.close();
			}
		}

		/**
		 * Closes the file, and no further reading is possible.
		 */
		void
		close()
		{
			d_file.close();
			d_is_closed = true;
		}

		/**
		 * Returns the number of levels in the current mipmapped raster file.
		 */
		unsigned int
		get_number_of_levels() const
		{
			return d_impl->get_number_of_levels();
		}

		/**
		 * Reads the given region from the mipmap at the given @a level.
		 *
		 * Returns boost::none if the @a level is non-existent, or if the region given
		 * lies partly or wholly outside the mipmap at the given @a level. Also
		 * returns boost::none if the file has already been closed.
		 *
		 * The @a level is the level in the mipmapped raster file. For the first
		 * mipmap level (i.e. one size smaller than the source raster), specify 0 as
		 * the @a level, because the mipmapped raster file does not store the source
		 * raster.
		 */
		boost::optional<typename RawRasterType::non_null_ptr_type>
		read_level(
				unsigned int level,
				unsigned int x_offset,
				unsigned int y_offset,
				unsigned int width,
				unsigned int height) const
		{
			if (d_is_closed)
			{
				return boost::none;
			}
			else
			{
				return d_impl->read_level(level, x_offset, y_offset, width, height);
			}
		}

		/**
		 * Reads the given region from the coverage raster at the given @a level.
		 *
		 * Returns boost::none if the @a level is non-existent, or if the region given
		 * lies partly or wholly outside the mipmap at the given @a level. Also
		 * returns boost::none if the file has already been closed.
		 *
		 * The @a level is the level in the mipmapped raster file. For the first
		 * mipmap level (i.e. one size smaller than the source raster), specify 0 as
		 * the @a level, because the mipmapped raster file does not store the source
		 * raster.
		 */
		boost::optional<GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type>
		read_coverage(
				unsigned int level,
				unsigned int x_offset,
				unsigned int y_offset,
				unsigned int width,
				unsigned int height) const
		{
			if (d_is_closed)
			{
				return boost::none;
			}
			else
			{
				return d_impl->read_coverage(level, x_offset, y_offset, width, height);
			}
		}

		/**
		 * Retrieves information about the file that we are reading.
		 */
		QFileInfo
		get_file_info() const
		{
			return QFileInfo(d_file);
		}

		/**
		 * Returns the filename of the file that we are reading.
		 */
		QString
		get_filename() const
		{
			return d_file.fileName();
		}

	private:

		class ReaderImpl
		{
		public:

			virtual
			~ReaderImpl()
			{
			}

			virtual
			unsigned int
			get_number_of_levels() = 0;

			virtual
			boost::optional<typename RawRasterType::non_null_ptr_type>
			read_level(
					unsigned int level,
					unsigned int x_offset,
					unsigned int y_offset,
					unsigned int width,
					unsigned int height) = 0;

			virtual
			boost::optional<GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type>
			read_coverage(
					unsigned int level,
					unsigned int x_offset,
					unsigned int y_offset,
					unsigned int width,
					unsigned int height) = 0;
		};

		/**
		 * Reads in mipmapped raster files that have a version number of 1.
		 */
		class VersionOneReader :
				public ReaderImpl
		{
		public:

			VersionOneReader(
					QFile &file,
					QDataStream &in) :
				d_file(file),
				d_in(in)
			{
				d_in.setVersion(MipmappedRasterFormat::Q_DATA_STREAM_VERSION);

				// Check that the file is big enough to hold at least a v1 header.
				QFileInfo file_info(d_file);
				static const qint64 MINIMUM_HEADER_SIZE = 16;
				if (file_info.size() < MINIMUM_HEADER_SIZE)
				{
					throw FileFormatNotSupportedException(
							GPLATES_EXCEPTION_SOURCE, "bad v1 header");
				}

				// Check that the type of raster stored in the file is as requested.
				quint32 type;
				static const qint64 TYPE_OFFSET = 8;
				d_file.seek(TYPE_OFFSET);
				d_in >> type;
				if (type != static_cast<quint32>(MipmappedRasterFormat::get_type_as_enum<
							typename RawRasterType::element_type>()))
				{
					throw FileFormatNotSupportedException(
							GPLATES_EXCEPTION_SOURCE, "bad raster type");
				}

				// Read the number of levels.
				quint32 num_levels;
				static const qint64 NUM_LEVELS_OFFSET = 12;
				static const qint64 LEVEL_INFO_OFFSET = 16;
				static const qint64 LEVEL_INFO_SIZE = 16;
				d_file.seek(NUM_LEVELS_OFFSET);
				d_in >> num_levels;
				if (file_info.size() < LEVEL_INFO_OFFSET + LEVEL_INFO_SIZE * num_levels)
				{
					throw FileFormatNotSupportedException(
							GPLATES_EXCEPTION_SOURCE, "insufficient levels");
				}

				// Read the level info.
				d_file.seek(LEVEL_INFO_OFFSET);
				for (quint32 i = 0; i != num_levels; ++i)
				{
					MipmappedRasterFormat::LevelInfo current_level;
					d_in >> current_level.width
							>> current_level.height
							>> current_level.main_offset
							>> current_level.coverage_offset;

					d_level_infos.push_back(current_level);
				}
			}

			virtual
			unsigned int
			get_number_of_levels()
			{
				return d_level_infos.size();
			}

			virtual
			boost::optional<typename RawRasterType::non_null_ptr_type>
			read_level(
					unsigned int level,
					unsigned int x_offset,
					unsigned int y_offset,
					unsigned int width,
					unsigned int height)
			{
				if (!is_valid_region(level, x_offset, y_offset, width, height))
				{
					return boost::none;
				}

				const MipmappedRasterFormat::LevelInfo &level_info = d_level_infos[level];
				typename RawRasterType::non_null_ptr_type result = RawRasterType::create(
						width, height);
				copy_region(
						level_info.main_offset,
						level_info.width,
						result->data(),
						x_offset,
						y_offset,
						width,
						height);

				return result;
			}

			virtual
			boost::optional<GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type>
			read_coverage(
					unsigned int level,
					unsigned int x_offset,
					unsigned int y_offset,
					unsigned int width,
					unsigned int height)
			{
				if (!is_valid_region(level, x_offset, y_offset, width, height))
				{
					return boost::none;
				}

				const MipmappedRasterFormat::LevelInfo &level_info = d_level_infos[level];
				if (level_info.coverage_offset == 0)
				{
					// No coverage for this level.
					return boost::none;
				}
				GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type result =
					GPlatesPropertyValues::CoverageRawRaster::create(width, height);
				copy_region(
						level_info.coverage_offset,
						level_info.width,
						result->data(),
						x_offset,
						y_offset,
						width,
						height);

				return result;
			}

		private:

			bool
			is_valid_region(
					unsigned int level,
					unsigned int x_offset,
					unsigned int y_offset,
					unsigned int width,
					unsigned int height)
			{
				if (level >= d_level_infos.size())
				{
					return false;
				}

				const MipmappedRasterFormat::LevelInfo &level_info = d_level_infos[level];
				return width > 0 && height > 0 &&
					x_offset + width <= level_info.width &&
					y_offset + height <= level_info.height;
			}

			template<typename T>
			void
			copy_region(
					unsigned int offset_in_file,
					unsigned int level_width,
					T *dest_data,
					unsigned int dest_x_offset_in_source,
					unsigned int dest_y_offset_in_source,
					unsigned int dest_width,
					unsigned int dest_height)
			{
				for (unsigned int i = 0; i != dest_height; ++i)
				{
					unsigned int source_row = dest_y_offset_in_source + i;
					d_file.seek(offset_in_file +
							(source_row * level_width + dest_x_offset_in_source) * sizeof(T));
					T *dest = dest_data + i * dest_width;
					T *dest_end = dest + dest_width;

					while (dest != dest_end)
					{
						d_in >> *dest;
						++dest;
					}
				}
			}

			QFile &d_file;
			QDataStream &d_in;
			std::vector<MipmappedRasterFormat::LevelInfo> d_level_infos;
		};

		QFile d_file;
		QDataStream d_in;
		boost::scoped_ptr<ReaderImpl> d_impl;
		bool d_is_closed;
	};
}

#endif  // GPLATES_FILEIO_MIPMAPPEDRASTERFORMATREADER_H
