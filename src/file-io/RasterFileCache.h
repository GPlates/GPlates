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

#ifndef GPLATES_FILE_IO_RASTERFILECACHE_H
#define GPLATES_FILE_IO_RASTERFILECACHE_H

#include <exception>
#include <boost/shared_ptr.hpp>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QString>

#include "MipmappedRasterFormatReader.h"
#include "MipmappedRasterFormatWriter.h"
#include "RasterBandReaderHandle.h"
#include "RasterFileCacheFormat.h"
#include "SourceRasterFileCacheFormatReader.h"
#include "TemporaryFileRegistry.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "gui/Mipmapper.h"

#include "property-values/RasterType.h"
#include "property-values/RawRasterUtils.h"

#include "utils/Profile.h"


namespace GPlatesFileIO
{
	namespace RasterFileCache
	{
		/**
		 * Creates and returns a @a MipmappedRasterFormatReader object that can be used for reading
		 * regions of mipmaps.
		 *
		 * Note that @a colour_palette only needs to be specified for integer rasters (see below).
		 *
		 * This will generate the mipmap raster file cache if necessary because:
		 *  - it doesn't exist yet, or
		 *  - it is a only partial file (eg, an exception was thrown last time it was written and
		 *    it wasn't removed, or
		 *  - the file version is a newer version of GPlates (unrecognised) in which case it will
		 *    be removed and re-generated with the current version.
		 *
		 * Returns NULL if:
		 *  - mipmap file is unable to be created (if it doesn't exist or is a partial file) such as
		 *    no write permission on directory containing source raster of temp directory.
		 *
		 * For RGBA and floating-point rasters, there is only ever one mipmap
		 * file associated with the raster. The @a colour_palette parameter is
		 * ignored and has no effect.
		 *
		 * For integer rasters, there is a "main" mipmap file, used if the
		 * colour palette is floating-point. This function creates the "main" mipmap file if
		 * @a colour_palette is either boost::none or contains a floating-point colour palette.
		 *
		 * However, if an integer colour palette is to be used with an integer
		 * raster, there is a special mipmap file created for that integer
		 * colour palette + integer raster combination. This function creates a special mipmap file
		 * if @a colour_palette contains an integer colour palette.
		 *
		 * NOTE: Use "MipmappedRasterFormatWriter<ProxiedRawRasterType, true>" for an integer raster
		 * when using an integer colour palette - this ensures the mipmapping is done *after*
		 * the integer raster is coloured with the palette.
		 *
		 * Note that since the mipmap file might get removed during this function, the caller
		 * should not have any open file handles and hence not have any existing
		 * @a MipmappedRasterFormatReader referencing the source raster.
		 */
		template <
				class ProxiedRawRasterType,
				class MipmappedRasterType,
				class MipmapRasterFormatWriterType /*= MipmappedRasterFormatWriter<ProxiedRawRasterType>*/ >
		boost::shared_ptr<MipmappedRasterFormatReader<MipmappedRasterType> >
		create_mipmapped_raster_file_cache_format_reader(
				const typename ProxiedRawRasterType::non_null_ptr_type &proxied_raw_raster,
				GPlatesFileIO::RasterBandReaderHandle raster_band_reader_handle,
				const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &colour_palette =
						GPlatesGui::RasterColourPalette::create());


		namespace Internals
		{
			/**
			 * Creates a mipmap file for the specified proxied raster.
			 *
			 * Returns false if:
			 *  - cannot write to mipmap file, or
			 *  - the element type of @a proxied_raw_raster is not that of its associated raster band reader, or
			 *  - there is an error writing to the mipmap file.
			 */
			template <
					class ProxiedRawRasterType,
					class MipmapRasterFormatWriterType>
			bool
			create_mipmap_file(
					const typename ProxiedRawRasterType::non_null_ptr_type &proxied_raw_raster,
					GPlatesFileIO::RasterBandReaderHandle raster_band_reader_handle,
					const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &colour_palette)
			{
				PROFILE_FUNC();

				const QString &filename = raster_band_reader_handle.get_filename();
				unsigned int band_number = raster_band_reader_handle.get_band_number();

				const GPlatesGui::RasterColourPaletteType::Type colour_palette_type =
						GPlatesGui::RasterColourPaletteType::get_type(*colour_palette);
				const bool is_integer_colour_palette =
						colour_palette_type == GPlatesGui::RasterColourPaletteType::INT32 ||
						colour_palette_type == GPlatesGui::RasterColourPaletteType::UINT32;

				boost::optional<std::size_t> colour_palette_id;
				if (is_integer_colour_palette)
				{
					// If an integer colour palette is to be used with an integer raster then a special
					// mipmap file created for that integer colour palette + integer raster combination.
					colour_palette_id = RasterFileCacheFormat::get_colour_palette_id(colour_palette);
				}

				boost::optional<QString> mipmap_filename =
						RasterFileCacheFormat::get_writable_mipmap_cache_filename(
								filename, band_number, colour_palette_id);

				if (!mipmap_filename)
				{
					// Can't write mipmap file anywhere.
					return false;
				}

				// Check the type of the source raster band.
				typedef typename ProxiedRawRasterType::element_type element_type;
				if (raster_band_reader_handle.get_type() !=
						GPlatesPropertyValues::RasterType::get_type_as_enum<element_type>())
				{
					return false;
				}

				// Write the mipmap file.
				try
				{
					// Pass the colour palette so the mipmap format writer can colour the
					// source raster and mipmap the coloured sections.
					MipmapRasterFormatWriterType writer(
							proxied_raw_raster,
							raster_band_reader_handle,
							colour_palette);
					writer.write(mipmap_filename.get());

					if (is_integer_colour_palette)
					{
						// The coloured mipmap files used by integer rasters with integer colour
						// palettes are deleted when GPlates exits.
						// This is because they are created specifically for particular colour
						// palettes, indexed by their memory address, which of course does not
						// remain the same the next time GPlates gets run.
						TemporaryFileRegistry::instance().add_file(mipmap_filename.get());

						// Make sure the file is only readable and writable by the user.
						// Suppose the source raster file is on a shared directory that happens to
						// be global writable, and two users are running two instances of GPlates.
						// It makes no sense for the second user to use the coloured mipmap file
						// generated by the first user; the colour palette id, being generated
						// from the memory address, is unlikely to mean the same thing in the
						// second instance of GPlates.
						// 
						// Note: this should change if we start hashing colour palettes, though.
						QFile::setPermissions(mipmap_filename.get(), QFile::ReadUser | QFile::WriteUser);
					}
					else
					{
						// Copy the file permissions from the source raster file to the mipmap file.
						QFile::setPermissions(mipmap_filename.get(), QFile::permissions(filename));
					}
				}
				catch (std::exception &exc)
				{
					// Log the exception so we know what caused the failure.
					qWarning() << "Error writing mipmap file '" << mipmap_filename.get()
							<< "', removing it: " << exc.what();

					// Remove the mipmap file in case it was partially written.
					QFile(mipmap_filename.get()).remove();

					return false;
				}
				catch (...)
				{
					// Log the exception so we know what caused the failure.
					qWarning() << "Unknown error writing mipmap file '" << mipmap_filename.get()
							<< "', removing it";

					// Remove the mipmap file in case it was partially written.
					QFile(mipmap_filename.get()).remove();

					return false;
				}

				return true;
			}
		}


		template <
				class ProxiedRawRasterType,
				class MipmappedRasterType,
				class MipmapRasterFormatWriterType>
		boost::shared_ptr<MipmappedRasterFormatReader<MipmappedRasterType> >
		create_mipmapped_raster_file_cache_format_reader(
				const typename ProxiedRawRasterType::non_null_ptr_type &proxied_raw_raster,
				GPlatesFileIO::RasterBandReaderHandle raster_band_reader_handle,
				const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &colour_palette)
		{
			typedef MipmappedRasterFormatReader<MipmappedRasterType> mipmapped_raster_format_reader_type;

			const QString &source_filename = raster_band_reader_handle.get_filename();
			unsigned int band_number = raster_band_reader_handle.get_band_number();

			boost::optional<std::size_t> colour_palette_id;
			const GPlatesGui::RasterColourPaletteType::Type colour_palette_type =
					GPlatesGui::RasterColourPaletteType::get_type(*colour_palette);
			if (colour_palette_type == GPlatesGui::RasterColourPaletteType::INT32 ||
				colour_palette_type == GPlatesGui::RasterColourPaletteType::UINT32)
			{
				// If an integer colour palette is to be used with an integer raster then a special
				// mipmap file created for that integer colour palette + integer raster combination.
				colour_palette_id = RasterFileCacheFormat::get_colour_palette_id(colour_palette);
			}

			// Find the existing mipmap file (if exists).
			boost::optional<QString> mipmap_filename =
					RasterFileCacheFormat::get_existing_mipmap_cache_filename(
							source_filename, band_number, colour_palette_id);
			if (mipmap_filename)
			{
				// If the source raster was modified after the raster file cache then we need
				// to regenerate the raster file cache.
				QDateTime source_last_modified = QFileInfo(source_filename).lastModified();
				QDateTime mipmap_last_modified = QFileInfo(mipmap_filename.get()).lastModified();
				if (source_last_modified > mipmap_last_modified)
				{
					// Remove the file.
					QFile(mipmap_filename.get()).remove();
					// Create a new mipmap file.
					if (!Internals::create_mipmap_file<ProxiedRawRasterType, MipmapRasterFormatWriterType>(
						proxied_raw_raster, raster_band_reader_handle, colour_palette))
					{
						// Unable to create mipmap file.
						return boost::shared_ptr<mipmapped_raster_format_reader_type>();
					}
				}
			}
			// Generate the mipmap file if it doesn't exist...
			else
			{
				if (!Internals::create_mipmap_file<ProxiedRawRasterType, MipmapRasterFormatWriterType>(
					proxied_raw_raster, raster_band_reader_handle, colour_palette))
				{
					// Unable to create mipmap file.
					return boost::shared_ptr<mipmapped_raster_format_reader_type>();
				}

				mipmap_filename =
						RasterFileCacheFormat::get_existing_mipmap_cache_filename(
								source_filename, band_number, colour_palette_id);
				if (!mipmap_filename)
				{
					// Mipmap file was created but unable to read it for some reason.
					return boost::shared_ptr<mipmapped_raster_format_reader_type>();
				}
			}

			boost::shared_ptr<mipmapped_raster_format_reader_type> mipmapped_raster_format_reader;

			try
			{
				try
				{
					// Attempt to create the mipmap raster format reader.
					mipmapped_raster_format_reader.reset(
							new mipmapped_raster_format_reader_type(mipmap_filename.get()));
				}
				catch (RasterFileCacheFormat::UnsupportedVersion &exc)
				{
					// Log the exception so we know what caused the failure.
					qWarning() << exc;

					qWarning() << "Attempting rebuild of mipmap file '"
							<< mipmap_filename.get() << "' for current version of GPlates.";

					// We'll have to remove the file and build it for the current GPlates version.
					// This means if the future version of GPlates (the one that created the
					// unrecognised version file) runs again it will either know how to
					// load our version (or rebuild it for itself also if it determines its
					// new format is much better or much more efficient).
					QFile(mipmap_filename.get()).remove();

					// Build it with the current version format.
					if (Internals::create_mipmap_file<ProxiedRawRasterType, MipmapRasterFormatWriterType>(
						proxied_raw_raster, raster_band_reader_handle, colour_palette))
					{
						// Try reading it again.
						mipmapped_raster_format_reader.reset(
								new mipmapped_raster_format_reader_type(mipmap_filename.get()));
					}
				}
				catch (std::exception &exc)
				{
					// Log the exception so we know what caused the failure.
					qWarning() << "Error reading mipmap file '" << mipmap_filename.get()
							<< "', attempting rebuild: " << exc.what();

					// Remove the mipmap file in case it is corrupted somehow.
					// Eg, it was partially written to by a previous instance of GPlates and
					// not immediately removed for some reason.
					QFile(mipmap_filename.get()).remove();

					// Try building it again.
					if (Internals::create_mipmap_file<ProxiedRawRasterType, MipmapRasterFormatWriterType>(
						proxied_raw_raster, raster_band_reader_handle, colour_palette))
					{
						// Try reading it again.
						mipmapped_raster_format_reader.reset(
								new mipmapped_raster_format_reader_type(mipmap_filename.get()));
					}
				}
			}
			catch (std::exception &exc)
			{
				// Log the exception so we know what caused the failure.
				qWarning() << exc.what();

				// Log a warning message.
				qWarning() << "Unable to read, or generate, mipmap file for raster '"
						<< source_filename << "', giving up on it.";
			}
			catch (...)
			{
				qWarning() << "Unknown exception";

				// Log a warning message.
				qWarning() << "Unable to read, or generate, mipmap file for raster '"
						<< source_filename << "', giving up on it.";
			}

			return mipmapped_raster_format_reader;
		}
	}
}

#endif // GPLATES_FILE_IO_RASTERFILECACHE_H
