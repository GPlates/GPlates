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

#include <QString>
#include <QFileInfo>
#include <QDateTime>

#include "ProxiedRasterCache.h"

#include "file-io/RasterReader.h"

#include "utils/UnicodeStringUtils.h"


namespace
{
	using namespace GPlatesPropertyValues;

	class ConcreteProxiedRasterCacheImpl :
			public ProxiedRasterCacheInternals::ProxiedRasterCacheImpl
	{
	public:

		ConcreteProxiedRasterCacheImpl(
				const TextContent &file_name) :
			d_file_name(file_name),
			d_file_name_as_qstring(GPlatesUtils::make_qstring_from_icu_string(file_name.get()))
		{
			update_proxied_raw_rasters(true /* force */);
		}

		virtual
		const std::vector<RawRaster::non_null_ptr_type> &
		proxied_raw_rasters()
		{
			update_proxied_raw_rasters(false);
			return d_proxied_raw_rasters;
		}

		virtual
		void
		set_file_name(
				const TextContent &file_name)
		{
			if (d_file_name != file_name)
			{
				d_file_name = file_name;
				d_file_name_as_qstring = GPlatesUtils::make_qstring_from_icu_string(file_name.get());
				update_proxied_raw_rasters(true /* force */);
			}
		}

	private:

		/**
		 * If @a force is true, will update proxied RawRasters if file exists.
		 *
		 * If @a force is false, will update proxied RawRasters if file exists and
		 * last modified timestamp is newer than what it was last time we saw it.
		 */
		void
		update_proxied_raw_rasters(
				bool force)
		{
			QFileInfo file_info(d_file_name_as_qstring);

			if (!file_info.exists())
			{
				return;
			}

			QDateTime last_modified = file_info.lastModified();
			if (force || d_last_modified < last_modified)
			{
				d_last_modified = last_modified;

				d_proxied_raw_rasters.clear();

				// Create a proxied RawRaster for each band in file.
				GPlatesFileIO::RasterReader::non_null_ptr_type reader =
					GPlatesFileIO::RasterReader::create(d_file_name_as_qstring);
				if (!reader->can_read())
				{
					return;
				}

				unsigned int number_of_bands = reader->get_number_of_bands();

				// Band numbers start at 1.
				for (unsigned int i = 1; i <= number_of_bands; ++i)
				{
					boost::optional<RawRaster::non_null_ptr_type> proxied_raw_raster =
						reader->get_proxied_raw_raster(i);
					if (proxied_raw_raster)
					{
						d_proxied_raw_rasters.push_back(*proxied_raw_raster);
					}
					else
					{
						// This shouldn't happen but if for some reason we get back boost::none,
						// we stick an UninitialisedRawRaster in place of the proxied RawRaster.
						// This is because otherwise the band numbering would be out of whack.
						d_proxied_raw_rasters.push_back(UninitialisedRawRaster::create());
					}
				}
			}
		}

		TextContent d_file_name;
		QString d_file_name_as_qstring;
		QDateTime d_last_modified;
		std::vector<RawRaster::non_null_ptr_type> d_proxied_raw_rasters;
	};
}


GPlatesPropertyValues::ProxiedRasterCache::non_null_ptr_type
GPlatesPropertyValues::ProxiedRasterCache::create(
		const TextContent &file_name)
{
	return new ProxiedRasterCache(file_name);
}


GPlatesPropertyValues::ProxiedRasterCache::ProxiedRasterCache(
		const TextContent &file_name) :
	d_impl(new ConcreteProxiedRasterCacheImpl(file_name))
{  }


const std::vector<GPlatesPropertyValues::RawRaster::non_null_ptr_type> &
GPlatesPropertyValues::ProxiedRasterCache::proxied_raw_rasters() const
{
	return d_impl->proxied_raw_rasters();
}


void
GPlatesPropertyValues::ProxiedRasterCache::set_file_name(
		const TextContent &file_name)
{
	d_impl->set_file_name(file_name);
}

