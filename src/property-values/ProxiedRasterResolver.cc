/* $Id$ */

/**
 * \file 
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

#include "ProxiedRasterResolver.h"


namespace
{
	using namespace GPlatesPropertyValues;

	class CreateProxiedRasterResolverVisitorImpl
	{
	public:

		typedef boost::optional<ProxiedRasterResolver::non_null_ptr_type> result_type;

		result_type
		get_result() const
		{
			return d_result;
		}

	private:

		template<class RawRasterType, bool has_proxied_data>
		class Create
		{
		public:

			static
			result_type
			create(
					RawRasterType &raster)
			{
				// No work to do; has_proxied_data = false case handled here.
				return boost::none;
			}
		};

		template<class RawRasterType>
		class Create<RawRasterType, true>
		{
		public:

			static
			result_type
			create(
					RawRasterType &raster)
			{
				boost::optional<typename ProxiedRasterResolverImpl<RawRasterType>::non_null_ptr_type> result =
					ProxiedRasterResolverImpl<RawRasterType>::create(&raster);
				if (result)
				{
					return result_type(*result);
				}
				else
				{
					return boost::none;
				}
			}
		};

		template<class RawRasterType>
		void
		do_visit(
				RawRasterType &raster)
		{
			d_result = Create<RawRasterType, RawRasterType::has_proxied_data>::create(raster);
		}

		friend class TemplatedRawRasterVisitor<CreateProxiedRasterResolverVisitorImpl>;

		result_type d_result;
	};

	typedef TemplatedRawRasterVisitor<CreateProxiedRasterResolverVisitorImpl>
		CreateProxiedRasterResolverVisitor;

	/**
	 * Returns whether a file is writable by actually attempting to open the
	 * file for writing.
	 *
	 * We do this because QFileInfo::isWritable() sometimes gives the wrong
	 * answer, especially on Windows.
	 *
	 * Note: this will create the file if it doesn't already exist.
	 */
	bool
	is_writable(
			const QString &filename)
	{
		QFile file(filename);
		bool can_open = file.open(QIODevice::WriteOnly | QIODevice::Append);
		if (can_open)
		{
			file.close();
		}
		return can_open;
	}
}


GPlatesPropertyValues::ProxiedRasterResolver::ProxiedRasterResolver()
{  }


GPlatesPropertyValues::ProxiedRasterResolver::~ProxiedRasterResolver()
{  }


boost::optional<GPlatesPropertyValues::ProxiedRasterResolver::non_null_ptr_type>
GPlatesPropertyValues::ProxiedRasterResolver::create(
		const RawRaster::non_null_ptr_type &raster)
{
	CreateProxiedRasterResolverVisitor visitor;
	raster->accept_visitor(visitor);

	return visitor.get_result();
}


QString
GPlatesPropertyValues::ProxiedRasterResolverInternals::make_mipmap_filename_in_same_directory(
		const QString &source_filename,
		unsigned int band_number,
		size_t colour_palette_id)
{
	static const QString FORMAT = "%1.%2.mipmaps";
	static const QString FORMAT_WITH_COLOUR_PALETTE = "%1.%2.%3.mipmaps";

	if (colour_palette_id)
	{
		return FORMAT_WITH_COLOUR_PALETTE.
			arg(source_filename).
			arg(band_number).
			arg(colour_palette_id);
	}
	else
	{
		return FORMAT.
			arg(source_filename).
			arg(band_number);
	}
}


QString
GPlatesPropertyValues::ProxiedRasterResolverInternals::make_mipmap_filename_in_tmp_directory(
		const QString &source_filename,
		unsigned int band_number,
		size_t colour_palette_id)
{
	return GPlatesFileIO::TemporaryFileRegistry::make_filename_in_tmp_directory(
			make_mipmap_filename_in_same_directory(
				source_filename, band_number, colour_palette_id));
}


QString
GPlatesPropertyValues::ProxiedRasterResolverInternals::get_writable_mipmap_filename(
		const QString &source_filename,
		unsigned int band_number,
		size_t colour_palette_id)
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

	return QString();
}


QString
GPlatesPropertyValues::ProxiedRasterResolverInternals::get_existing_mipmap_filename(
		const QString &source_filename,
		unsigned int band_number,
		size_t colour_palette_id)
{
	QString in_same_directory = make_mipmap_filename_in_same_directory(
			source_filename, band_number, colour_palette_id);
	if (QFileInfo(in_same_directory).exists())
	{
		return in_same_directory;
	}

	QString in_tmp_directory = make_mipmap_filename_in_tmp_directory(
			source_filename, band_number, colour_palette_id);
	if (QFileInfo(in_tmp_directory).exists())
	{
		return in_tmp_directory;
	}

	return QString();
}


size_t
GPlatesPropertyValues::ProxiedRasterResolverInternals::get_colour_palette_id(
		boost::optional<GPlatesGui::RasterColourScheme::non_null_ptr_type> colour_palette)
{
	if (!colour_palette)
	{
		return 0;
	}

	GPlatesGui::RasterColourScheme::ColourPaletteType type = (*colour_palette)->get_type();

	// Note that we intentionally only do INT32 and UINT32.
	// This is because it is only when we are using integer colour palettes that we
	// need to create a special mipmap file for that colour palette.
	switch (type)
	{
		case GPlatesGui::RasterColourScheme::INT32:
			return reinterpret_cast<size_t>(
					(*colour_palette)->get_colour_palette<boost::int32_t>().get());

		case GPlatesGui::RasterColourScheme::UINT32:
			return reinterpret_cast<size_t>(
					(*colour_palette)->get_colour_palette<boost::uint32_t>().get());

		default:
			return 0;
	}
}


boost::optional<GPlatesPropertyValues::Rgba8RawRaster::non_null_ptr_type>
GPlatesPropertyValues::ProxiedRasterResolverInternals::colour_raw_raster(
		RawRaster &raster,
		const GPlatesGui::RasterColourScheme::non_null_ptr_type &colour_palette)
{
	switch (colour_palette->get_type())
	{
		case GPlatesGui::RasterColourScheme::INT32:
			return GPlatesGui::ColourRawRaster::colour_raw_raster<boost::int32_t>(
					raster, colour_palette->get_colour_palette<boost::int32_t>());

		case GPlatesGui::RasterColourScheme::UINT32:
			return GPlatesGui::ColourRawRaster::colour_raw_raster<boost::uint32_t>(
					raster, colour_palette->get_colour_palette<boost::uint32_t>());

		case GPlatesGui::RasterColourScheme::DOUBLE:
			return GPlatesGui::ColourRawRaster::colour_raw_raster<double>(
					raster, colour_palette->get_colour_palette<double>());

		default:
			return boost::none;
	}
}

