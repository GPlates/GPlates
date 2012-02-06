/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_FILE_IO_SOURCERASTERFILECACHEFORMATREADER_H
#define GPLATES_FILE_IO_SOURCERASTERFILECACHEFORMATREADER_H

#include <utility>
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
	 * Reads a copy of a source image originating from a @a RasterReader and stored in a cached
	 * file for efficient retrieval/streaming during raster rendering.
	 */
	class SourceRasterFileCacheFormatReader
	{
	public:
		virtual
		~SourceRasterFileCacheFormatReader()
		{  }


		/**
		 * Returns the dimensions of the source raster (width, height).
		 */
		virtual
		std::pair<unsigned int, unsigned int>
		get_raster_dimensions() const = 0;


		/**
		 * Reads the given region from the source raster.
		 *
		 * Returns boost::none if the region given lies partly or wholly outside the source raster.
		 * Also returns boost::none if the file has already been closed.
		 */
		virtual
		boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type>
		read_raster(
				unsigned int x_offset,
				unsigned int y_offset,
				unsigned int width,
				unsigned int height) const = 0;


		/**
		 * Reads the given region from the source raster as a coverage.
		 *
		 * The coverage values are 1.0 for all pixels except sentinel pixels (pixels containing the
		 * non-data value) which they are set to 0.0 coverage value.
		 *
		 * Returns boost::none if the region given lies partly or wholly outside the source raster.
		 * Also returns boost::none if the file has already been closed.
		 */
		virtual
		boost::optional<GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type>
		read_coverage(
				unsigned int x_offset,
				unsigned int y_offset,
				unsigned int width,
				unsigned int height) const = 0;
	};


	/**
	 * Implementation of @a SourceRasterFileCacheFormatReader for a specific template parameter
	 * @a RawRasterType representing the type of the source raster.
	 */
	template<class RawRasterType>
	class SourceRasterFileCacheFormatReaderImpl :
			public SourceRasterFileCacheFormatReader
	{
	public:

		/**
		 * Opens @a filename for reading as a source raster file cache.
		 *
		 * @throws @a ErrorOpeningFileForReadingException if @a filename could not be opened for reading.
		 *
		 * @throws @a FileFormatNotException if the header information is wrong.
		 *
		 * @throws @a RasterFileCacheFormat::UnsupportedVersion if the version is either not recognised
		 * (file cache created by a newer version of GPlates) or no longer supported (eg, if format
		 * is an old format that is inefficient and hence should be regenerated with a newer algorithm).
		 */
		SourceRasterFileCacheFormatReaderImpl(
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
			// This is in case raster file cache generation from a previous instance of GPlates failed
			// part-way through writing the file and didn't remove the file for some reason.
			// We need to check this here because we don't actually read the cached (encoded)
			// data until clients request region of the raster (and it's too late to detect errors then).
			if (total_file_size != static_cast<qint64>(file_info.size()))
			{
				throw FileFormatNotSupportedException(
						GPLATES_EXCEPTION_SOURCE, "detected a partially written source raster file cache");
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

		~SourceRasterFileCacheFormatReaderImpl()
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
		 * Returns the dimensions of the source raster (width, height).
		 */
		virtual
		std::pair<unsigned int, unsigned int>
		get_raster_dimensions() const
		{
			return d_impl->get_raster_dimensions();
		}


		/**
		 * Reads the given region from the source raster.
		 */
		virtual
		boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type>
		read_raster(
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
				return d_impl->read_raster(x_offset, y_offset, width, height);
			}
		}


		/**
		 * Reads the given region from the source raster as a coverage.
		 */
		virtual
		boost::optional<GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type>
		read_coverage(
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
				return d_impl->read_coverage(x_offset, y_offset, width, height);
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
			std::pair<unsigned int, unsigned int>
			get_raster_dimensions() const = 0;

			virtual
			boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type>
			read_raster(
					unsigned int x_offset,
					unsigned int y_offset,
					unsigned int width,
					unsigned int height) = 0;

			virtual
			boost::optional<GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type>
			read_coverage(
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
				d_in(in),
				d_raster_width(0),
				d_raster_height(0)
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

				// Read the source raster dimensions.
				quint32 source_raster_width;
				quint32 source_raster_height;
				d_in >> source_raster_width;
				d_in >> source_raster_height;
				d_raster_width = source_raster_width;
				d_raster_height = source_raster_height;

				// Read the number of blocks in the source raster.
				quint32 num_blocks_in_source_raster;
				d_in >> num_blocks_in_source_raster;

				// Create a raster file cache reader for the source raster.
				d_raster_file_cache_reader.reset(
						new RasterFileCacheFormatReader<RawRasterType>(
								version_number,
								d_file,
								d_in,
								source_raster_width,
								source_raster_height,
								num_blocks_in_source_raster,
								has_coverage));
			}

			~VersionOneReader()
			{
			}

			virtual
			std::pair<unsigned int, unsigned int>
			get_raster_dimensions() const
			{
				return std::make_pair(d_raster_width, d_raster_height);
			}

			virtual
			boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type>
			read_raster(
					unsigned int x_offset,
					unsigned int y_offset,
					unsigned int width,
					unsigned int height)
			{
				boost::optional<typename RawRasterType::non_null_ptr_type> raster =
						d_raster_file_cache_reader->read_raster(x_offset, y_offset, width, height);
				if (!raster)
				{
					return boost::none;
				}

				GPlatesPropertyValues::RawRaster::non_null_ptr_type raw_raster = raster.get();

				return raw_raster;
			}

			virtual
			boost::optional<GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type>
			read_coverage(
					unsigned int x_offset,
					unsigned int y_offset,
					unsigned int width,
					unsigned int height)
			{
				return d_raster_file_cache_reader->read_coverage(x_offset, y_offset, width, height);
			}

		private:

			QFile &d_file;
			QDataStream &d_in;
			unsigned int d_raster_width;
			unsigned int d_raster_height;
			boost::shared_ptr<RasterFileCacheFormatReader<RawRasterType> > d_raster_file_cache_reader;
		};


		QFile d_file;
		QDataStream d_in;
		boost::scoped_ptr<ReaderImpl> d_impl;
		bool d_is_closed;
	};
}

#endif // GPLATES_FILE_IO_SOURCERASTERFILECACHEFORMATREADER_H
