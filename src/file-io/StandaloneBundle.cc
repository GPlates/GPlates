/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2021 The University of Sydney, Australia
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

#include <QCoreApplication>
#include <QDir>
#include <QtGlobal>

#include "StandaloneBundle.h"

#include "Gdal.h"
#include "Proj.h"

#include "global/config.h"  // For GPLATES_INSTALL_STANDALONE, GPLATES_STANDALONE_PROJ_DATA_DIR, etc


namespace GPlatesFileIO
{
	namespace StandaloneBundle
	{
		namespace
		{
#if !defined(GPLATES_PYTHON_EMBEDDING)  // pygplates
			// This should be initialised with 'set_pygplates_bundle_directory()' just after the
			// non-embedded pygplates module was imported by an external (non-embedded Python interpreter).
			boost::optional<QString> pygplates_bundle_directory;
#endif
		}


		boost::optional<QString>
		get_bundle_root_directory()
		{
#if defined(GPLATES_INSTALL_STANDALONE)

#	if defined(GPLATES_PYTHON_EMBEDDING)  // gplates

#		if defined(Q_OS_MACOS)
			// On macOS the gplates executable is in 'gplates.app/Contents/MacOS'.
			// But the root directory contains 'gplates.app'.
			return QCoreApplication::applicationDirPath() + "/../../../";
#		else
			// On Windows and Linux the resources directory is the base installation directory (containing gplates exe).
			return QCoreApplication::applicationDirPath();
#		endif

#	else  // pygplates
			// Note that this should have been initialised with 'set_pygplates_bundle_directory()' just after
			// the non-embedded pygplates module was imported by an external (non-embedded Python interpreter).
			return pygplates_bundle_directory;
#	endif

#else  // not a standalone bundle
			return boost::none;
#endif
		}


		boost::optional<QString>
		get_bundle_resources_directory()
		{
			// Get the bundle root directory.
			boost::optional<QString> bundle_root_dir = get_bundle_root_directory();
			if (!bundle_root_dir)
			{
				return boost::none;
			}
			QString bundle_resources_dir = bundle_root_dir.get();

			// With GPlates on macOS the resources directory is 'gplates.app/Contents/Resources'.
#if defined(GPLATES_PYTHON_EMBEDDING)/*gplates*/ && defined(Q_OS_MACOS)
			bundle_resources_dir += "/gplates.app/Contents/Resources";
#endif

			return bundle_resources_dir;
		}


		boost::optional<QString>
		get_bundle_data_directory(
				QString data_dir_relative_to_resources_dir)
		{
			// Get the bundle resources directory.
			boost::optional<QString> bundle_resources_dir = get_bundle_resources_directory();
			if (!bundle_resources_dir)
			{
				return boost::none;
			}

			// See if the bundle data directory (in the resources directory) exists.
			// If it does then it means the requested data was included in the standalone bundle.
			QDir bundle_data_dir(bundle_resources_dir.get() + "/" + data_dir_relative_to_resources_dir);
			if (!bundle_data_dir.exists())
			{
				return boost::none;
			}

			return bundle_data_dir.absolutePath();
		}
	}
}


void
GPlatesFileIO::StandaloneBundle::initialise(
#if !defined(GPLATES_PYTHON_EMBEDDING)  // pygplates
		QString bundle_directory
#endif
)
{
#if !defined(GPLATES_PYTHON_EMBEDDING)  // pygplates
	pygplates_bundle_directory = bundle_directory;
#endif

#if defined(GPLATES_INSTALL_STANDALONE)

	//
	// Let the PROJ and GDAL dependency libraries know where to find Proj resources files (eg, 'proj.db').
	//
	boost::optional<QString> bundle_proj_data_directory = get_proj_data_directory();
	if (bundle_proj_data_directory &&
		QDir(bundle_proj_data_directory.get()).exists())
	{
		const std::string bundle_proj_search_path = bundle_proj_data_directory->toStdString();
		const char *bundle_proj_search_paths[] = { bundle_proj_search_path.c_str(), nullptr };

#	if defined(PROJ_VERSION_MAJOR) && (PROJ_VERSION_MAJOR > 6 || (PROJ_VERSION_MAJOR == 6 && PROJ_VERSION_MINOR >= 1))
		// We use a PROJ context - tell it where to find 'proj.db' in standalone bundle.
		//
		// With Proj >=6.1, paths set here have priority over the PROJ_LIB to allow for multiple versions
		// of PROJ resource files on your system without conflicting...
		proj_context_set_search_paths(nullptr, 1, bundle_proj_search_paths);
#	endif

#	if defined(GDAL_VERSION_MAJOR) && GDAL_VERSION_MAJOR >= 3
		// GDAL also has its own PROJ context - tell it where to find 'proj.db' in standalone bundle.
		OSRSetPROJSearchPaths(bundle_proj_search_paths);
#	endif
	}

	//
	// Let the GDAL dependency library know where to find its resources files (eg, 'gcs.csv' for GDAL < 2.5,
	// which was moved into 'proj.db' for GDAL >= 2.5, but there are other GDAL data files to bundle).
	//
	boost::optional<QString> bundle_gdal_data_directory = get_gdal_data_directory();
	if (bundle_gdal_data_directory &&
		QDir(bundle_gdal_data_directory.get()).exists())
	{
		const std::string bundle_gdal_data_dir = bundle_gdal_data_directory->toStdString();

		CPLSetConfigOption("GDAL_DATA", bundle_gdal_data_dir.c_str());
	}

#endif
}


boost::optional<QString>
GPlatesFileIO::StandaloneBundle::get_proj_data_directory()
{
	return get_bundle_data_directory(GPLATES_STANDALONE_PROJ_DATA_DIR);
}


boost::optional<QString>
GPlatesFileIO::StandaloneBundle::get_gdal_data_directory()
{
	return get_bundle_data_directory(GPLATES_STANDALONE_GDAL_DATA_DIR);
}


boost::optional<QString>
GPlatesFileIO::StandaloneBundle::get_python_standard_library_directory()
{
#if defined(GPLATES_PYTHON_EMBEDDING)  // gplates

	// Get the bundle root directory.
	boost::optional<QString> bundle_root_dir = get_bundle_root_directory();
	if (!bundle_root_dir)
	{
		return boost::none;
	}
	QString bundle_python_stdlib_dir = bundle_root_dir.get();

	// On macOS the bundle Python standard library is relative to the frameworks directory.
#	if defined(Q_OS_MACOS)
	bundle_python_stdlib_dir += "/gplates.app/Contents/Frameworks";
#	endif

	bundle_python_stdlib_dir = bundle_python_stdlib_dir + "/" + GPLATES_STANDALONE_PYTHON_STDLIB_DIR;

	// See if the bundle Python standard library directory exists.
	// If it does then it means it was included in the standalone bundle.
	if (!QDir(bundle_python_stdlib_dir).exists())
	{
		return boost::none;
	}

	return bundle_python_stdlib_dir;

#else // pygplates

	// Pygplates is imported by an external non-embedded Python interpreter
	// that has its own Python standard library.
	return boost::none;

#endif
}
