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

#ifndef GPLATES_FILEIO_OGR_H
#define GPLATES_FILEIO_OGR_H


#ifdef HAVE_CONFIG_H
// We're building on a UNIX-y system, and can thus expect "global/config.h".

// On some systems, it's <ogrsf_frmts.h>, on others, <gdal/ogrsf_frmts.h>.
// The "CMake" script should have determined which one to use.
#include "global/config.h"
#ifdef HAVE_GDAL_OGRSF_FRMTS_H
#include <gdal/ogrsf_frmts.h>
#else
#include <ogrsf_frmts.h>
#endif

#else  // We're not building on a UNIX-y system.  We'll have to assume it's <ogrsf_frmts.h>.
#include <ogrsf_frmts.h>
#endif  // HAVE_CONFIG_H

#endif // GPLATES_FILEIO_OGR_H
