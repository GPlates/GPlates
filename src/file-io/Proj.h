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

#ifdef HAVE_CONFIG_H
// We're building on a UNIX-y system, and can thus expect "global/config.h".

#include "global/config.h"
// Give preference to the Proj6 header ("proj.h").
#ifdef HAVE_PROJ_H
#include <proj.h>
#else
// Fallback to using Proj4 header ("proj_api.h").
//
// Accept use of deprecated Proj4 library header("proj_api.h").
// It'll be removed after a few minor versions of Proj6 have been released,
// so any GPlates builds using it should be switched over to Proj6 soon.
#ifndef ACCEPT_USE_OF_DEPRECATED_PROJ_API_H
#define ACCEPT_USE_OF_DEPRECATED_PROJ_API_H
#endif
#include <proj_api.h>
#endif

#else  // We're not building on a UNIX-y system.  We'll assume it's <proj.h> (for Proj6, not Proj4).
#include <proj.h>
#endif  // HAVE_CONFIG_H

#endif  // GPLATES_FILEIO_PROJ_H
