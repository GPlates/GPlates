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

#include "Gdal.h"

#include "global/GdalVersion.h" // For GDAL_VERSION_MAJOR


class GDALDataset;
class GDALDriver;
class GDALDriverManager;

// GDAL 1 uses OGRSFDriverRegistrar, OGRSFDriver and OGRDataSource.
// But GDAL 2 may deprecate them (should use GDAL equivalents instead).
#if !defined(GDAL_VERSION_MAJOR) || GDAL_VERSION_MAJOR < 2
class OGRDataSource;
class OGRSFDriver;
class OGRSFDriverRegistrar;
#endif


namespace GPlatesFileIO
{
	struct ReadErrorAccumulation;

	namespace GdalUtils
	{
		/**
		 * Data types.
		 *
		 * For GDAL 1 we use:
		 * - GDALDriverManager
		 * - GDALDriver
		 * - GDALDataset
		 * - GIntBig
		 *
		 * For GDAL 1 we use:
		 * - OGRSFDriverRegistrar
		 * - OGRSFDriver
		 * - OGRDataSource
		 * - int
		 *
		 * Note: GDAL version 2 transferred all methods of OGRDataSource to GDALDataset.
		 * OGRDataSource still exists in GDAL 2 but might get deprecated.
		 *
		 * See https://svn.osgeo.org/gdal/branches/2.0/gdal/MIGRATION_GUIDE.TXT
		 */
#if defined(GDAL_VERSION_MAJOR) && GDAL_VERSION_MAJOR >= 2
		typedef GDALDriverManager vector_data_driver_manager_type;
		typedef GDALDriver vector_data_driver_type;
		typedef GDALDataset vector_data_source_type;
		typedef GIntBig big_int_type;
#else
		typedef OGRSFDriverRegistrar vector_data_driver_manager_type;
		typedef OGRSFDriver vector_data_driver_type;
		typedef OGRDataSource vector_data_source_type;
		typedef int big_int_type;
#endif

		/**
		 * A convenience function that wraps GDALAllRegister.
		 *
		 * The first time this function is called it will call GDALAllRegister.
		 */
		void
		register_all_drivers();


		/**
		 * A convenience function that wraps around GetGDALDriverManager for raster formats.
		 */
		GDALDriverManager *
		get_raster_driver_manager();

		/**
		 * A convenience function that wraps around GDALOpenEx for raster formats.
		 * The file with @a filename is opened for read-only access (default) unless @a update is true.
		 *
		 * If an error is encountered, this function returns NULL and @a read_errors is populated accordingly.
		 *
		 * For systems that segfault on GDALOpen, this function traps the segfault signal, and fails gracefully.
		 *
		 * This function calls @a register_all_drivers internally.
		 */
		GDALDataset *
		open_raster(
				const QString &filename,
				bool update = false,
				ReadErrorAccumulation *read_errors = NULL);

		/**
		 * Convenience function that wraps around GDALClose.
		 * 
		 */
		void
		close_raster(
				GDALDataset *gdal_data_set);


		//
		// The following OGR functions mirror the equivalent GDAL functions above.
		//
		// This is mainly while we transition from GDAL 1 to GDAL 2.
		// Some OGR functions have been deprecated (or in process of being deprecated) in GDAL 2.
		// Ideally we should just use the GDAL structures and functions in these cases, but
		// for a short while we'll need to support both GDAL 1 and GDAL 2.
		// Once we no longer support Ubuntu Xenial (16.04 LTS), which uses GDAL 1, then we
		// can solely use GDAL 2 where required for OGR.
		//



		/**
		 * A convenience function that wraps around GetGDALDriverManager for vector formats.
		 */
		vector_data_driver_manager_type *
		get_vector_driver_manager();


		/**
		 * A convenience function that wraps around GDALDriver::Create() for vector formats.
		 */
		vector_data_source_type *
		create_data_source(
				vector_data_driver_type *vector_data_driver,
				const char *pszName,
                char **papszOptions = NULL);


		/**
		 * A convenience function that wraps around GDALOpenEx for vector formats.
		 * The file with @a filename is opened for read-only access (default) unless @a update is true.
		 *
		 * If an error is encountered, this function returns NULL and @a read_errors is populated accordingly.
		 *
		 * This function calls @a register_all_drivers internally.
		 */
		vector_data_source_type *
		open_vector(
				const QString &filename,
				bool update = false,
				ReadErrorAccumulation *read_errors = NULL);

		/**
		 * Convenience function that wraps around GDALClose.
		 * 
		 */
		void
		close_vector(
				vector_data_source_type *ogr_data_source);
	}
}

#endif // GPLATES_FILEIO_GDALUTILS_H
