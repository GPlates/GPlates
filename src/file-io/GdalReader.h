/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, Geological Survey of Norway
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

#include "property-values/InMemoryRaster.h"
#include "ReadErrorAccumulation.h"

class GDALDataset; 

namespace GPlatesFileIO{
	
	class GdalReader
	{
	public:

		GdalReader();

		~GdalReader();

		void
		read_file(
			const QString &filename,
			GPlatesPropertyValues::InMemoryRaster &raster,
			ReadErrorAccumulation &read_errors);

	private:

		GdalReader(
			const GdalReader &other);

		GdalReader &
		operator=(
			const GdalReader &other);

		GDALDataset *d_dataset_ptr;
	};
}

#endif // GPLATES_FILEIO_GDALREADER_H
