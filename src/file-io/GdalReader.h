/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 Geological Survey of Norway
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

#ifndef GPLATES_FILEIO_GDALREADER_H
#define GPLATES_FILEIO_GDALREADER_H

#include <boost/noncopyable.hpp>

#include "ReadErrorAccumulation.h"

#include "property-values/RawRaster.h"

class GDALDataset; 

namespace GPlatesFileIO
{
	struct ErrorReadingGDALBand {  };

	class GdalReader :
			public boost::noncopyable
	{
	public:

		GdalReader();

		~GdalReader();

		boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type>
		read_file(
				const QString &filename,
				ReadErrorAccumulation &read_errors);

	private:

		GDALDataset *d_dataset_ptr;
	};
}

#endif  // GPLATES_FILEIO_GDALREADER_H
