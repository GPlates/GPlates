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

#include "FileInfo.h"
#include "TemporaryFileRegistry.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


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


		QString
		make_source_filename_in_same_directory(
				const QString &source_filename,
				unsigned int band_number)
		{
			static const QString FORMAT = "%1.band%2.level0" + RASTER_FILE_CACHE_EXTENSION;
			return FORMAT.arg(source_filename).arg(band_number);
		}


		QString
		make_source_filename_in_tmp_directory(
				const QString &source_filename,
				unsigned int band_number)
		{
			return TemporaryFileRegistry::make_filename_in_tmp_directory(
					make_source_filename_in_same_directory(
						source_filename, band_number));
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

		template<>
		Type
		get_type_as_enum<quint8>()
		{
			return UINT8;
		}

		template<>
		Type
		get_type_as_enum<quint16>()
		{
			return UINT16;
		}

		template<>
		Type
		get_type_as_enum<qint16>()
		{
			return INT16;
		}

		template<>
		Type
		get_type_as_enum<quint32>()
		{
			return UINT32;
		}

		template<>
		Type
		get_type_as_enum<qint32>()
		{
			return INT32;
		}
	}
}


GPlatesFileIO::RasterFileCacheFormat::BlockInfos::BlockInfos(
		unsigned int image_width,
		unsigned int image_height) :
	d_num_blocks_in_x_direction(
			(image_width + BLOCK_SIZE - 1) / BLOCK_SIZE),
	d_num_blocks_in_y_direction(
			(image_height + BLOCK_SIZE - 1) / BLOCK_SIZE),
	d_block_infos(d_num_blocks_in_x_direction * d_num_blocks_in_y_direction)
{
}


unsigned int
GPlatesFileIO::RasterFileCacheFormat::BlockInfos::get_num_blocks() const
{
	return d_block_infos.size();
}


const GPlatesFileIO::RasterFileCacheFormat::BlockInfo &
GPlatesFileIO::RasterFileCacheFormat::BlockInfos::get_block_info(
		unsigned int block_x_offset,
		unsigned int block_y_offset) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			block_x_offset < d_num_blocks_in_x_direction &&
				block_y_offset < d_num_blocks_in_y_direction,
			GPLATES_ASSERTION_SOURCE);

	return d_block_infos[block_y_offset * d_num_blocks_in_x_direction + block_x_offset];
}


GPlatesFileIO::RasterFileCacheFormat::BlockInfo &
GPlatesFileIO::RasterFileCacheFormat::BlockInfos::get_block_info(
		unsigned int block_x_offset,
		unsigned int block_y_offset)
{
	// Reuse const method.
	return const_cast<BlockInfo &>(
			static_cast<const BlockInfos *>(this)
					->get_block_info(block_x_offset, block_y_offset));
}


const GPlatesFileIO::RasterFileCacheFormat::BlockInfo &
GPlatesFileIO::RasterFileCacheFormat::BlockInfos::get_block_info(
		unsigned int block_index) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			block_index < d_block_infos.size(),
			GPLATES_ASSERTION_SOURCE);

	return d_block_infos[block_index];
}


GPlatesFileIO::RasterFileCacheFormat::BlockInfo &
GPlatesFileIO::RasterFileCacheFormat::BlockInfos::get_block_info(
		unsigned int block_index)
{
	// Reuse const method.
	return const_cast<BlockInfo &>(
			static_cast<const BlockInfos *>(this)
					->get_block_info(block_index));
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


void
GPlatesFileIO::RasterFileCacheFormat::get_mipmap_dimensions(
		unsigned int &mipmap_width,
		unsigned int &mipmap_height,
		unsigned int mipmap_level,
		const unsigned int source_raster_width,
		const unsigned int source_raster_height)
{
	mipmap_width = source_raster_width;
	mipmap_height = source_raster_height;

	while (mipmap_width > BLOCK_SIZE || mipmap_height > BLOCK_SIZE)
	{
		mipmap_width = (mipmap_width >> 1) + (mipmap_width & 1);
		mipmap_height = (mipmap_height >> 1) + (mipmap_height & 1);

		if (mipmap_level == 0)
		{
			return;
		}

		--mipmap_level;
	}

	// If get here then specified 'mipmap_level' was too high.
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
}


boost::optional<QString>
GPlatesFileIO::RasterFileCacheFormat::get_writable_mipmap_cache_filename(
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
GPlatesFileIO::RasterFileCacheFormat::get_existing_mipmap_cache_filename(
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


boost::optional<QString>
GPlatesFileIO::RasterFileCacheFormat::get_writable_source_cache_filename(
		const QString &source_filename,
		unsigned int band_number)
{
	QString in_same_directory = make_source_filename_in_same_directory(
			source_filename, band_number);
	if (is_writable(in_same_directory))
	{
		// qDebug() << in_same_directory;
		return in_same_directory;
	}

	QString in_tmp_directory = make_source_filename_in_tmp_directory(
			source_filename, band_number);
	if (is_writable(in_tmp_directory))
	{
		// qDebug() << in_tmp_directory;
		return in_tmp_directory;
	}

	return boost::none;
}


boost::optional<QString>
GPlatesFileIO::RasterFileCacheFormat::get_existing_source_cache_filename(
		const QString &source_filename,
		unsigned int band_number)
{
	QString in_same_directory = make_source_filename_in_same_directory(
			source_filename, band_number);
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

	QString in_tmp_directory = make_source_filename_in_tmp_directory(
			source_filename, band_number);
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
