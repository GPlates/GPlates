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
#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>
#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QString>

#include "ErrorOpeningFileForReadingException.h"
#include "FileFormatNotSupportedException.h"
#include "RasterFileCacheFormat.h"
#include "RasterFileCacheFormatReader.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/LogException.h"

#include "gui/Colour.h"

#include "property-values/RawRaster.h"

#include "utils/Profile.h"


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
		 *
		 * @throws @a RasterFileCacheFormat::UnsupportedVersion if the mipmap version is either
		 * not recognised (mipmap file created by a newer version of GPlates) or no longer supported
		 * (eg, if mipmap format is an old format that is inefficient and hence should be regenerated
		 * with a newer algorithm).
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

			d_in.setVersion(RasterFileCacheFormat::Q_DATA_STREAM_VERSION);

			// Check that there is enough data in the file for magic number, file size and version.
			QFileInfo file_info(d_file);
			static const qint64 MAGIC_NUMBER_AND_FILE_SIZE_AND_VERSION_SIZE =
					sizeof(RasterFileCacheFormat::MAGIC_NUMBER) + sizeof(qint64) + sizeof(quint32);
			if (file_info.size() < MAGIC_NUMBER_AND_FILE_SIZE_AND_VERSION_SIZE)
			{
				throw FileFormatNotSupportedException(
						GPLATES_EXCEPTION_SOURCE, "bad header");
			}

			// Check the magic number.
			for (unsigned int n = 0; n < sizeof(RasterFileCacheFormat::MAGIC_NUMBER); ++n)
			{
				quint8 magic_number;
				d_in >> magic_number;
				if (magic_number != RasterFileCacheFormat::MAGIC_NUMBER[n])
				{
					throw FileFormatNotSupportedException(
							GPLATES_EXCEPTION_SOURCE, "bad magic number");
				}
			}

			// The size of the file so we can check with the actual size.
			qint64 total_file_size;
			d_in >> total_file_size;
#if 0
			qDebug() << "total_file_size " << total_file_size;
			qDebug() << "file_info.size() " << file_info.size();
#endif

			// Check that the file length is correct.
			// This is in case mipmap generation from a previous instance of GPlates failed
			// part-way through writing the file and didn't remove the file for some reason.
			// We need to check this here because we don't actually read the mipmapped (encoded)
			// data until clients request region of the raster (and it's too late to detect errors then).
			if (total_file_size != static_cast<qint64>(file_info.size()))
			{
				throw FileFormatNotSupportedException(
						GPLATES_EXCEPTION_SOURCE, "detected a partially written mipmap file");
			}

			// Check the version number.
			quint32 version_number;
			d_in >> version_number;

			// Determine which reader to use depending on the version.
			if (version_number == 1)
			{
				d_impl.reset(new VersionOneReader(version_number, d_file, d_in));
			}
			// The following demonstrates a possible future scenario where VersionOneReader is used
			// for versions 1 and 2 and VersionsThreeReader is used for versions 3, 4, 5.
			// This could happen if only a small change is needed for version 2 but a larger more
			// structural change is required at version 3 necessitating a new reader class...
#if 0
			else if (version_number >=3 && version_number <= RasterFileCacheFormat::VERSION_NUMBER)
			{
				d_impl.reset(new VersionThreeReader(version_number, d_file, d_in));
			}
#endif
			else
			{
				throw RasterFileCacheFormat::UnsupportedVersion(
						GPLATES_EXCEPTION_SOURCE, version_number);
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
			PROFILE_FUNC();

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
			PROFILE_FUNC();

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
		 * A reader for version 1+ files.
		 *
		 * The most likely changes to the reader will be at the data-block encoding level in which
		 * case this class could be used for version 2, 3, etc, until/if a major change is implemented.
		 */
		class VersionOneReader :
				public ReaderImpl
		{
		public:

			VersionOneReader(
					quint32 version_number,
					QFile &file,
					QDataStream &in) :
				d_file(file),
				d_in(in)
			{
				// NOTE: The total file size has been verified before we get here so there's no
				// need to check that the file is large enough to read data as we read.

				// Check that the type of raster stored in the file is as requested.
				quint32 type;
				d_in >> type;
				if (type != static_cast<quint32>(RasterFileCacheFormat::get_type_as_enum<
							typename RawRasterType::element_type>()))
				{
					throw FileFormatNotSupportedException(
							GPLATES_EXCEPTION_SOURCE, "bad raster type");
				}

				// Flag to indicate whether coverage data is available in the file.
				quint32 has_coverage;
				d_in >> has_coverage;

				// Read the number of levels.
				quint32 num_levels;
				d_in >> num_levels;
				//qDebug() << "num_levels: " << num_levels;

				unsigned int level;

				// Read the level info.
				for (level = 0; level < num_levels; ++level)
				{
					RasterFileCacheFormat::LevelInfo current_level;
					d_in >> current_level.width
							>> current_level.height
							>> current_level.blocks_file_offset
							>> current_level.num_blocks;
#if 0
					qDebug() << "Level: " << level;
					qDebug() << "width: " << current_level.width;
					qDebug() << "height: " << current_level.height;
					qDebug() << "blocks_file_offset: " << current_level.file_offset;
					qDebug() << "num_blocks: " << current_level.num_blocks;
#endif

					d_level_infos.push_back(current_level);
				}

				// Create a raster file cache reader for each mipmap level.
				for (level = 0; level < num_levels; ++level)
				{
					const RasterFileCacheFormat::LevelInfo &level_info = d_level_infos[level];

					// Seek to the file position where the block information is.
					file.seek(d_level_infos[level].blocks_file_offset);

					boost::shared_ptr<RasterFileCacheFormatReader<RawRasterType> > reader(
							new RasterFileCacheFormatReader<RawRasterType>(
									version_number,
									d_file,
									d_in,
									level_info.width,
									level_info.height,
									level_info.num_blocks,
									has_coverage));

					d_raster_file_cache_readers.push_back(reader);
				}
			}

			~VersionOneReader()
			{
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
				if (level >= d_level_infos.size())
				{
					return boost::none;
				}

				return d_raster_file_cache_readers[level]->read_raster(x_offset, y_offset, width, height);
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
				if (level >= d_level_infos.size())
				{
					return boost::none;
				}

				return d_raster_file_cache_readers[level]->read_coverage(x_offset, y_offset, width, height);
			}

		private:

			QFile &d_file;
			QDataStream &d_in;
			std::vector<RasterFileCacheFormat::LevelInfo> d_level_infos;
			std::vector<boost::shared_ptr<RasterFileCacheFormatReader<RawRasterType> > > d_raster_file_cache_readers;
		};


		QFile d_file;
		QDataStream d_in;
		boost::scoped_ptr<ReaderImpl> d_impl;
		bool d_is_closed;
	};
}

#endif  // GPLATES_FILEIO_MIPMAPPEDRASTERFORMATREADER_H
