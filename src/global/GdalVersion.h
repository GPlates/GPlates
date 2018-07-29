/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#ifndef GPLATES_GLOBAL_GDALVERSION_H
#define GPLATES_GLOBAL_GDALVERSION_H

//
// Include the GDAL version header so we can access its macros such as GDAL_VERSION_MAJOR and GDAL_RELEASE_DATE.
//
#ifdef HAVE_CONFIG_H
	// Include config header so we know whether (and how) to include "gdal_version.h"
	// which contains the version of GDAL we're compiling against.
	#include "global/config.h"

	#ifdef HAVE_GDAL_VERSION_H
		#if defined(HAVE_GDAL_VERSION_H_UPPERCASE_GDAL_PREFIX)
			#include <GDAL/gdal_version.h>
		#elif defined(HAVE_GDAL_VERSION_H_LOWERCASE_GDAL_PREFIX)
			#include <gdal/gdal_version.h>
		#else
			#include <gdal_version.h>
		#endif
	#endif
#else  // We'll have to assume it's <gdal_version.h>.
	#include <gdal_version.h>
#endif


//
// GDAL introduced the GDAL_COMPUTE_VERSION macro in GDAL 1.10.
// However statements like:
//
//   #if defined(GDAL_COMPUTE_VERSION) && GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(2,3,0)
//
// ...will fail if GDAL_COMPUTE_VERSION is not defined because the "GDAL_COMPUTE_VERSION(2,3,0)" part
// will not make sense to the preprocessor.
//
// So we'll just go ahead and copy the relevant macros from GDAL so that they're always defined.
//

#ifndef GPLATES_GDAL_COMPUTE_VERSION
// Same as defined in GDAL >= 1.10...
#define GPLATES_GDAL_COMPUTE_VERSION(maj,min,rev) ((maj)*1000000+(min)*10000+(rev)*100)
#endif

#ifndef GPLATES_GDAL_VERSION_NUM
// Same as defined in GDAL >= 1.10...
#define GPLATES_GDAL_VERSION_NUM (GPLATES_GDAL_COMPUTE_VERSION(GDAL_VERSION_MAJOR,GDAL_VERSION_MINOR,GDAL_VERSION_REV)+GDAL_VERSION_BUILD)
#endif

#endif // GPLATES_GLOBAL_GDALVERSION_H
