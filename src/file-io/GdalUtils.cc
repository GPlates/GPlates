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
#include "ReadErrorAccumulation.h"
#include "ReadErrorOccurrence.h"

using namespace GPlatesFileIO;

#ifdef Q_WS_X11

// On some older Linux systems, GDALOpen segfaults when you try and open a GMT
// grd file. What we do here is set up a handler for the segfault and fail
// gracefully by returning NULL instead of crashing the whole application.

#include <csignal>
#include <cstring> // for memset
#include <csetjmp>

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
	do_gdal_open(
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

#else

// For everyone else, let's just call GDALOpen directly.

namespace
{
	GDALDataset *
	do_gdal_open(
			const QString &filename,
			GPlatesFileIO::ReadErrorAccumulation *read_errors)
	{
		const std::string filename_std_string = filename.toStdString();
		GDALDataset *result = static_cast<GDALDataset *>(GDALOpen(filename_std_string.c_str(), GA_ReadOnly));

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

#endif  // Q_WS_X11


void
GPlatesFileIO::GdalUtils::gdal_register_drivers()
{
	static bool s_registered = false;

	// Only register once.
	if (!s_registered)
	{
		GDALAllRegister();

		s_registered = true;
	}
}


GDALDataset *
GPlatesFileIO::GdalUtils::gdal_open(
		const QString &filename,
		ReadErrorAccumulation *read_errors)
{
	gdal_register_drivers();

	return do_gdal_open(filename, read_errors);
}

