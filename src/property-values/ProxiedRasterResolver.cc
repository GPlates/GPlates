/* $Id$ */

/**
 * \file 
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2010, 2011 The University of Sydney, Australia
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
		std::size_t colour_palette_id)
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
		std::size_t colour_palette_id)
{
	return GPlatesFileIO::TemporaryFileRegistry::make_filename_in_tmp_directory(
			make_mipmap_filename_in_same_directory(
				source_filename, band_number, colour_palette_id));
}


QString
GPlatesPropertyValues::ProxiedRasterResolverInternals::get_writable_mipmap_filename(
		const QString &source_filename,
		unsigned int band_number,
		std::size_t colour_palette_id)
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
		std::size_t colour_palette_id)
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

	return QString();
}


namespace
{
	class GetColourPaletteIdVisitor :
			public boost::static_visitor<std::size_t>
	{
	public:

		std::size_t
		operator()(
				const GPlatesGui::RasterColourPalette::empty &) const
		{
			return 0;
		}

		template<class ColourPaletteType>
		std::size_t
		operator()(
				const GPlatesUtils::non_null_intrusive_ptr<ColourPaletteType> &colour_palette) const
		{
			return reinterpret_cast<std::size_t>(colour_palette.get());
		}
	};
}


std::size_t
GPlatesPropertyValues::ProxiedRasterResolverInternals::get_colour_palette_id(
		const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &colour_palette)
{
	return boost::apply_visitor(GetColourPaletteIdVisitor(), *colour_palette);
}
