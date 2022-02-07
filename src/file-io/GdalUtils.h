/* $Id: GdalReader.h 2952 2008-05-19 13:17:33Z jboyden $ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2008-05-19 06:17:33 -0700 (Mon, 19 May 2008) $
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

#ifndef GPLATES_FILEIO_GDALUTILS_H
#define GPLATES_FILEIO_GDALUTILS_H

#include <QString>

class GDALDataset; 

namespace GPlatesFileIO
{
	struct ReadErrorAccumulation;

	namespace GdalUtils
	{
		/**
		 * A convenience function that wraps GDALAllRegister.
		 *
		 * The first time this function is called it will call GDALAllRegister.
		 */
		void
		gdal_register_drivers();


		/**
		 * A convenience function that wraps around GDALOpen. The file with
		 * @a filename is opened for read-only access.
		 *
		 * If an error is encountered, this function returns NULL and
		 * @a read_errors is populated accordingly.
		 *
		 * For systems that segfault on GDALOpen, this function traps the segfault
		 * signal, and fails gracefully.
		 *
		 * This function calls @a gdal_register_drivers internally.
		 */
		GDALDataset *
		gdal_open(
				const QString &filename,
				ReadErrorAccumulation *read_errors = NULL);
	}
}

#endif // GPLATES_FILEIO_GDALUTILS_H
