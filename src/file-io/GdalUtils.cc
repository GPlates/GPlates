/* $Id: GdalReader.cc 8004 2010-04-13 08:24:16Z elau $ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2010-04-13 01:24:16 -0700 (Tue, 13 Apr 2010) $
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

#include <QtGlobal>

#include "Gdal.h"
#include "GdalUtils.h"
#include "Ogr.h"
#include "ReadErrorAccumulation.h"
#include "ReadErrorOccurrence.h"


#if defined(GDAL_VERSION_MAJOR) && GDAL_VERSION_MAJOR >= 2

	//
	// For GDAL version 2, use GDALOpenEx() to open both vector and raster formats.
	//

	namespace GPlatesFileIO
	{
		namespace
		{
			GDALDataset *
			open_gdal_version2(
					const QString &filename,
					unsigned int open_flags, // GDAL_OF_RASTER or GDAL_OF_VECTOR (and GDAL_OF_READONLY or GDAL_OF_UPDATE).
					ReadErrorAccumulation *read_errors)
			{
				const std::string filename_std_string = filename.toStdString();
				GDALDataset *result = static_cast<GDALDataset *>(
						GDALOpenEx(
								filename_std_string.c_str(),
								open_flags,
								NULL,
								NULL,
								NULL));

				// Add errors as necessary.
				if (!result)
				{
					if (read_errors)
					{
						DataFormats::DataFormat data_format;
						ReadErrors::Description description;
						if ((open_flags & GDAL_OF_RASTER) != 0)
						{
							data_format = DataFormats::RasterImage;
							description = ReadErrors::ErrorReadingRasterFile;
						}
						else // vector format ...
						{
							data_format = DataFormats::Shapefile;
							description = ReadErrors::ErrorReadingVectorFile;
						}

						read_errors->d_failures_to_begin.push_back(
								make_read_error_occurrence(
									filename,
									data_format,
									0,
									description,
									ReadErrors::FileNotLoaded));
					}
				}

				return result;
			}
		}
	}

#else // GDAL version 1 ...

	//
	// For GDAL version 1 use:
	// - GDALOpen() to open raster formats, and
	// - OGRSFDriverRegistrar::Open() to open vector formats.
	//

	namespace GPlatesFileIO
	{
		namespace
		{
			GdalUtils::vector_data_source_type *
			open_vector_gdal_version1(
					const QString &filename,
					int bUpdate,
					ReadErrorAccumulation *read_errors)
			{
				const std::string file_name_string = filename.toStdString();
				
				GdalUtils::vector_data_source_type *result =
						OGRSFDriverRegistrar::Open(file_name_string.c_str(), bUpdate);

				// Add errors as necessary.
				if (!result)
				{
					if (read_errors)
					{
						read_errors->d_failures_to_begin.push_back(
								make_read_error_occurrence(
									filename,
									DataFormats::Shapefile,
									0,
									ReadErrors::ErrorReadingVectorFile,
									ReadErrors::FileNotLoaded));
					}
				}

				return result;
			}
		}
	}

	#ifndef Q_WS_X11

		// For everyone but Q_WS_X11, let's just call GDALOpen directly.

		namespace GPlatesFileIO
		{
			namespace
			{
				GDALDataset *
				open_raster_gdal_version1(
						const QString &filename,
						GPlatesFileIO::ReadErrorAccumulation *read_errors)
				{
					const std::string filename_std_string = filename.toStdString();
					GDALDataset *result = static_cast<GDALDataset *>(
							GDALOpen(filename_std_string.c_str(), GA_ReadOnly));

					// Add errors as necessary.
					if (!result)
					{
						if (read_errors)
						{
							read_errors->d_failures_to_begin.push_back(
									make_read_error_occurrence(
										filename,
										DataFormats::RasterImage,
										0,
										ReadErrors::ErrorReadingRasterFile,
										ReadErrors::FileNotLoaded));
						}
					}

					return result;
				}
			}
		}

	#else

		// On some older Linux systems, GDALOpen segfaults when you try and open a GMT
		// grd file. What we do here is set up a handler for the segfault and fail
		// gracefully by returning NULL instead of crashing the whole application.

		#include <csignal>
		#include <cstring> // for memset
		#include <csetjmp>

		namespace GPlatesFileIO
		{
			namespace
			{
				// The old sigaction for SIGSEGV before we called sigaction().
				struct sigaction old_action;

				// Holds the information necessary for setjmp/longjmp to work.
				jmp_buf buf;

				/**
				 * Handles the SIGSEGV signal.
				 */
				void
				handler(int signum)
				{
					// Jump past the problem call to GDALOpen.
					longjmp(buf, 1 /* non-zero value so that the GDALOpen call doesn't get called again */);
				}

				GDALDataset *
				open_raster_gdal_version1(
						const QString &filename,
						GPlatesFileIO::ReadErrorAccumulation *read_errors)
				{
					// Set a handler for SIGSEGV in case the call to GDALOpen does a segfault.
					struct sigaction action;
					std::memset(&action, 0, sizeof(action)); // sizeof(struct sigaction) is platform dependent.
					action.sa_handler = &handler;
					sigaction(SIGSEGV, &action, &old_action);

					// Try and call GDALOpen.
					GDALDataset *result = NULL;
					bool segfaulted = true;
					if (!setjmp(buf))
					{
						// The first time setjmp() is called, it returns 0. If GDALOpen() segfaults,
						// we longjmp back to the if statement, but with a non-zero value.
						const std::string filename_std_string = filename.toStdString();
						result = static_cast<GDALDataset *>(GDALOpen(filename_std_string.c_str(), GA_ReadOnly));
						segfaulted = false;
					}

					// Restore the old sigaction whether or not we segfaulted.
					sigaction(SIGSEGV, &old_action, NULL);

					// Add errors as necessary.
					if (!result)
					{
						if (read_errors)
						{
							read_errors->d_failures_to_begin.push_back(
									make_read_error_occurrence(
										filename,
										DataFormats::RasterImage,
										0,
										segfaulted ?
											ReadErrors::ErrorInSystemLibraries :
											ReadErrors::ErrorReadingRasterFile,
										ReadErrors::FileNotLoaded));
						}
					}

					return result;
				}
			}
		}

	#endif  // Q_WS_X11

#endif // defined(GDAL_VERSION_MAJOR) && GDAL_VERSION_MAJOR >= 2


void
GPlatesFileIO::GdalUtils::register_all_drivers()
{
	static bool s_registered = false;

	// Only register once.
	if (!s_registered)
	{
		//
		// OGRRegisterAll() in GDAL 1 replaced by GDALAllRegister() in GDAL 2.
		//
		// See https://svn.osgeo.org/gdal/branches/2.0/gdal/MIGRATION_GUIDE.TXT
		//
		GDALAllRegister();

		// Not sure if, for GDAL 1, also need to call OGRRegisterAll().
		// Will do it anyway - it's what we were doing previously anyway.
#if !defined(GDAL_VERSION_MAJOR) || GDAL_VERSION_MAJOR < 2
		OGRRegisterAll();
#endif

		s_registered = true;
	}
}


GDALDriverManager *
GPlatesFileIO::GdalUtils::get_raster_driver_manager()
{
	return GetGDALDriverManager();
}


GDALDataset *
GPlatesFileIO::GdalUtils::open_raster(
		const QString &filename,
		bool update,
		ReadErrorAccumulation *read_errors)
{
	register_all_drivers();

	//
	// Use GDALOpen() in GDAL 1 and GDALOpenEx() in GDAL 2.
	//
	// See https://svn.osgeo.org/gdal/branches/2.0/gdal/MIGRATION_GUIDE.TXT
	//
#if defined(GDAL_VERSION_MAJOR) && GDAL_VERSION_MAJOR >= 2
	unsigned int open_flags = GDAL_OF_RASTER;
	if (update)
	{
		open_flags |= GDAL_OF_UPDATE;
	}
	else
	{
		open_flags |= GDAL_OF_READONLY;
	}
	return open_gdal_version2(filename, open_flags, read_errors);
#else
	return open_raster_gdal_version1(filename, read_errors);
#endif
}


void
GPlatesFileIO::GdalUtils::close_raster(
		GDALDataset *gdal_data_set)
{
	GDALClose(gdal_data_set);
}


GPlatesFileIO::GdalUtils::vector_data_driver_manager_type *
GPlatesFileIO::GdalUtils::get_vector_driver_manager()
{
	//
	// Use GetGDALDriverManager in GDAL 2 and OGRSFDriverRegistrar::GetRegistrar() in GDAL 1.
	//
	// See https://svn.osgeo.org/gdal/branches/2.0/gdal/MIGRATION_GUIDE.TXT
	//
#if defined(GDAL_VERSION_MAJOR) && GDAL_VERSION_MAJOR >= 2
	return GetGDALDriverManager();
#else
	return OGRSFDriverRegistrar::GetRegistrar();
#endif
}


GPlatesFileIO::GdalUtils::vector_data_source_type *
GPlatesFileIO::GdalUtils::create_data_source(
		vector_data_driver_type *vector_data_driver,
		const char *pszName,
        char **papszOptions)
{
	//
	// Use GDALDriver::Create() in GDAL 2 and OGRSFDriver::CreateDataSource() in GDAL 1.
	//
	// See http://www.gdal.org/ogr_apitut.html
	//
#if defined(GDAL_VERSION_MAJOR) && GDAL_VERSION_MAJOR >= 2
	return vector_data_driver->Create(pszName, 0/*nXSize*/, 0/*nYSize*/, 0/*nBands*/, GDT_Unknown/*eType*/, papszOptions);
#else
	return vector_data_driver->CreateDataSource(pszName, papszOptions);
#endif
}


GPlatesFileIO::GdalUtils::vector_data_source_type *
GPlatesFileIO::GdalUtils::open_vector(
		const QString &filename,
		bool update,
		ReadErrorAccumulation *read_errors)
{
	//
	// Use GDALOpen() in GDAL 1 and GDALOpenEx() in GDAL 2.
	//
	// See https://svn.osgeo.org/gdal/branches/2.0/gdal/MIGRATION_GUIDE.TXT
	//
#if defined(GDAL_VERSION_MAJOR) && GDAL_VERSION_MAJOR >= 2
	unsigned int open_flags = GDAL_OF_VECTOR;
	if (update)
	{
		open_flags |= GDAL_OF_UPDATE;
	}
	else
	{
		open_flags |= GDAL_OF_READONLY;
	}
	return open_gdal_version2(filename, open_flags, read_errors);
#else
	return open_vector_gdal_version1(filename, update, read_errors);
#endif
}


void
GPlatesFileIO::GdalUtils::close_vector(
		vector_data_source_type *ogr_data_source)
{
	//
	// Use GDALDataset in GDAL 2 and OGRDataSource in GDAL 1.
	//
	// See https://svn.osgeo.org/gdal/branches/2.0/gdal/MIGRATION_GUIDE.TXT
	//
#if defined(GDAL_VERSION_MAJOR) && GDAL_VERSION_MAJOR >= 2
	GDALClose(ogr_data_source);
#else
	OGRDataSource::DestroyDataSource(ogr_data_source);
#endif
}
