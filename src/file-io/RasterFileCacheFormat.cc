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

#include <ostream>
#include <QDebug>
#include <QFile>
#include <QFileInfo>

#include "RasterFileCacheFormat.h"

#include "TemporaryFileRegistry.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


namespace GPlatesFileIO
{
	namespace
	{
		// All raster file caches have filenames that end with this.
		const QString RASTER_FILE_CACHE_EXTENSION = ".gplates.cache";


		class GetColourPaletteIdVisitor :
				public boost::static_visitor<boost::optional<std::size_t> >
		{
		public:

			boost::optional<std::size_t>
			operator()(
					const GPlatesGui::RasterColourPalette::empty &) const
			{
				return boost::none;
			}

			template<class ColourPaletteType>
			boost::optional<std::size_t>
			operator()(
					const GPlatesUtils::non_null_intrusive_ptr<ColourPaletteType> &colour_palette) const
			{
				return reinterpret_cast<std::size_t>(colour_palette.get());
			}
		};

		/**
		 * Returns whether a file is writable by actually attempting to open the file for writing.
		 *
		 * We do this because QFileInfo::isWritable() sometimes gives the wrong answer, especially on Windows.
		 */
		bool
		is_writable(
				const QString &filename)
		{
			QFile file(filename);
			bool exists = file.exists();
			bool can_open = file.open(QIODevice::WriteOnly | QIODevice::Append);
			if (can_open)
			{
				file.close();

				// Clean up: file.open() creates the file if it doesn't exist.
				if (!exists)
				{
					file.remove();
				}
			}

			return can_open;
		}


		QString
		make_mipmap_filename_in_same_directory(
				const QString &source_filename,
				unsigned int band_number,
				boost::optional<std::size_t> colour_palette_id = boost::none)
		{
			if (colour_palette_id)
			{
				static const QString FORMAT = "%1.band%2.palette%3.mipmaps" + RASTER_FILE_CACHE_EXTENSION;
				return FORMAT.arg(source_filename).arg(band_number).arg(colour_palette_id.get());
			}

			static const QString FORMAT = "%1.band%2.mipmaps" + RASTER_FILE_CACHE_EXTENSION;
			return FORMAT.arg(source_filename).arg(band_number);
		}


		QString
		make_mipmap_filename_in_tmp_directory(
				const QString &source_filename,
				unsigned int band_number,
				boost::optional<std::size_t> colour_palette_id = boost::none)
		{
			return TemporaryFileRegistry::make_filename_in_tmp_directory(
					make_mipmap_filename_in_same_directory(
						source_filename, band_number, colour_palette_id));
		}
	}
}


namespace GPlatesFileIO
{
	namespace RasterFileCacheFormat
	{
		template<>
		Type
		get_type_as_enum<GPlatesGui::rgba8_t>()
		{
			return RGBA;
		}

		template<>
		Type
		get_type_as_enum<float>()
		{
			return FLOAT;
		}

		template<>
		Type
		get_type_as_enum<double>()
		{
			return DOUBLE;
		}
	}
}


unsigned int
GPlatesFileIO::RasterFileCacheFormat::get_number_of_mipmapped_levels(
		const unsigned int source_raster_width,
		const unsigned int source_raster_height)
{
	unsigned int num_mipmapped_levels = 0;

	unsigned int width = source_raster_width;
	unsigned int height = source_raster_height;

	while (width > BLOCK_SIZE || height > BLOCK_SIZE)
	{
		width = (width >> 1) + (width & 1);
		height = (height >> 1) + (height & 1);

		++num_mipmapped_levels;
	}

	return num_mipmapped_levels;
}


boost::optional<QString>
GPlatesFileIO::RasterFileCacheFormat::get_writable_mipmap_filename(
		const QString &source_filename,
		unsigned int band_number,
		boost::optional<std::size_t> colour_palette_id)
{
	QString in_same_directory = make_mipmap_filename_in_same_directory(
			source_filename, band_number, colour_palette_id);
	if (is_writable(in_same_directory))
	{
		// qDebug() << in_same_directory;
		return in_same_directory;
	}

	QString in_tmp_directory = make_mipmap_filename_in_tmp_directory(
			source_filename, band_number, colour_palette_id);
	if (is_writable(in_tmp_directory))
	{
		// qDebug() << in_tmp_directory;
		return in_tmp_directory;
	}

	return boost::none;
}


boost::optional<QString>
GPlatesFileIO::RasterFileCacheFormat::get_existing_mipmap_filename(
		const QString &source_filename,
		unsigned int band_number,
		boost::optional<std::size_t> colour_palette_id)
{
	QString in_same_directory = make_mipmap_filename_in_same_directory(
			source_filename, band_number, colour_palette_id);
	if (QFileInfo(in_same_directory).exists())
	{
		// Check whether we can open it for reading.
		QFile file(in_same_directory);
		if (file.open(QIODevice::ReadOnly))
		{
			file.close();
			return in_same_directory;
		}
	}

	QString in_tmp_directory = make_mipmap_filename_in_tmp_directory(
			source_filename, band_number, colour_palette_id);
	if (QFileInfo(in_tmp_directory).exists())
	{
		// Check whether we can open it for reading.
		QFile file(in_tmp_directory);
		if (file.open(QIODevice::ReadOnly))
		{
			file.close();
			return in_tmp_directory;
		}
	}

	return boost::none;
}


boost::optional<std::size_t>
GPlatesFileIO::RasterFileCacheFormat::get_colour_palette_id(
		const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &colour_palette)
{
	return boost::apply_visitor(GetColourPaletteIdVisitor(), *colour_palette);
}


GPlatesFileIO::RasterFileCacheFormat::UnsupportedVersion::UnsupportedVersion(
		const GPlatesUtils::CallStack::Trace &exception_source,
		quint32 unrecognised_version_) :
	Exception(exception_source),
	d_unrecognised_version(unrecognised_version_)
{
}


const char *
GPlatesFileIO::RasterFileCacheFormat::UnsupportedVersion::exception_name() const
{

	return "RasterFileCacheFormat::UnsupportedVersion";
}


void
GPlatesFileIO::RasterFileCacheFormat::UnsupportedVersion::write_message(
		std::ostream &os) const
{
	os << "unsupported version: " << d_unrecognised_version;
}
