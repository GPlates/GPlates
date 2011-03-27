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

#ifndef GPLATES_PROPERTYVALUES_PROXIEDRASTERCACHE_H
#define GPLATES_PROPERTYVALUES_PROXIEDRASTERCACHE_H

#include <vector>
#include <boost/scoped_ptr.hpp>

#include "RawRaster.h"
#include "TextContent.h"

#include "file-io/ReadErrorAccumulation.h"

#include "utils/ReferenceCount.h"
#include "utils/non_null_intrusive_ptr.h"


namespace GPlatesPropertyValues
{
	namespace ProxiedRasterCacheInternals
	{
		// Forward declaration.
		class ProxiedRasterCacheImpl;
	}

	/**
	 * This class maintains updated proxied RawRasters for each band in a given
	 * raster file. The proxied RawRasters are updated when the file name changes
	 * and then the actual file on disk gets modified.
	 */
	class ProxiedRasterCache :
			public GPlatesUtils::ReferenceCount<ProxiedRasterCache>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<ProxiedRasterCache> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const ProxiedRasterCache> non_null_ptr_to_const_type;

		static
		non_null_ptr_type
		create(
				const TextContent &file_name,
				GPlatesFileIO::ReadErrorAccumulation *read_errors = NULL);

		const std::vector<RawRaster::non_null_ptr_type> &
		proxied_raw_rasters() const;

		void
		set_file_name(
				const TextContent &file_name,
				GPlatesFileIO::ReadErrorAccumulation *read_errors = NULL);

	private:

		ProxiedRasterCache(
				const TextContent &file_name,
				GPlatesFileIO::ReadErrorAccumulation *read_errors);

		boost::scoped_ptr<ProxiedRasterCacheInternals::ProxiedRasterCacheImpl> d_impl;
	};

	namespace ProxiedRasterCacheInternals
	{
		class ProxiedRasterCacheImpl
		{
		public:

			virtual
			~ProxiedRasterCacheImpl()
			{  }

			virtual
			const std::vector<RawRaster::non_null_ptr_type> &
			proxied_raw_rasters() = 0;

			virtual
			void
			set_file_name(
					const TextContent &file_name,
					GPlatesFileIO::ReadErrorAccumulation *read_errors) = 0;
		};
	}
}

#endif  // GPLATES_PROPERTYVALUES_PROXIEDRASTERCACHE_H
