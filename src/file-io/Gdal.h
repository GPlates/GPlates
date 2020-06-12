/* $Id: GdalReader.h 2952 2008-05-19 13:17:33Z jboyden $ */

/**
 * \file 
 * Includes the GDAL header file(s) appropriate for each system.
 *
 * Most recent change:
 *   $Date: 2008-05-19 06:17:33 -0700 (Mon, 19 May 2008) $
 *
 * Copyright (C) 2008 Geological Survey of Norway
 * (derived from "GdalReader.cc")
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

#ifndef GPLATES_FILEIO_GDAL_H
#define GPLATES_FILEIO_GDAL_H

// The <gdal_version.h> header is probably included indirectly below but we'll include here also
// just in case. That way including "Gdal.h" ensures access to the version macros.
#include "global/GdalVersion.h"


// On some systems, it's <gdal_priv.h>, on others, <gdal/gdal_priv.h>.
// The "CMake" script should have determined which one to use.
// We hijack the HAVE_GDAL_OGRSF_FRMTS_H variable because the ogrsf_frmts.h
// file is in the same directory.
#include "global/config.h"
#ifdef HAVE_GDAL_OGRSF_FRMTS_H
	#include <gdal/gdal_priv.h>
#else
	#include <gdal_priv.h>
#endif

#endif  // GPLATES_FILEIO_GDAL_H
