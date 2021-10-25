/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#include "scribe/ScribeExportExternal.h"
#include "scribe/ScribeExportRegistration.h"


/**
 * Group all classes/types to be scribe export registered for the 'pygplates' dynamic/shared library.
 *
 * See "ScribeExportRegistration.h" for more details.
 */
#define SCRIBE_EXPORT_PYGPLATES \
		SCRIBE_EXPORT_EXTERNAL


/**
 * Scribe export register all the above classes/types.
 *
 * See "ScribeExportRegistration.h" for more details.
 */
SCRIBE_EXPORT_REGISTRATION(SCRIBE_EXPORT_PYGPLATES)
