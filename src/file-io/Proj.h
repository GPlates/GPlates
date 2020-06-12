/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2019 The University of Sydney, Australia
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

#ifndef GPLATES_FILEIO_PROJ_H
#define GPLATES_FILEIO_PROJ_H

#include "global/config.h"
// Give preference to the Proj5+ header ("proj.h").
#ifdef HAVE_PROJ_H
	#include <proj.h>
#else
	// Fallback to using Proj4 header ("proj_api.h").
	//
	// Accept use of deprecated Proj4 library header ("proj_api.h").
	// Actually we don't really need this since we only get here if we haven't detected Proj (5+),
	// you only need to acknowledge if you use "proj_api.h" when there's also a "proj.h",
	// but we'll acknowledge use of deprecated API anyway.
	//
	#ifndef ACCEPT_USE_OF_DEPRECATED_PROJ_API_H
		#define ACCEPT_USE_OF_DEPRECATED_PROJ_API_H
	#endif
	// Note that "proj_api.h" will be removed when Proj7 is released, so any GPlates builds
	// using it should be switched over to Proj5+ soon. Any of those switched over builds
	// will be detected above and automatically start using "proj.h" (ie, Proj5+).
	#include <proj_api.h>
	#define GPLATES_USING_PROJ4
#endif

#endif  // GPLATES_FILEIO_PROJ_H
