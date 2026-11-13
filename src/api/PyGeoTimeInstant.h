/* $Id$ */

/**
 * \file
 * $Revision$
 * $Date$
 *
 * Copyright (C) 2026 The University of Sydney, Australia
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

#ifndef GPLATES_API_PYGEOTIMEINSTANT_H
#define GPLATES_API_PYGEOTIMEINSTANT_H

#include "property-values/GeoTimeInstant.h"


namespace GPlatesApi
{
	/**
	 * Converts a time from Python (a 'float') to a GeoTimeInstant.
	 *
	 * Positive infinity converts to distant past, and negative infinity to distant future.
	 *
	 * Raises Python 'ValueError' for NaN (the GeoTimeInstant constructor would abort instead).
	 * Use this, rather than constructing a GeoTimeInstant directly, for any time from Python.
	 */
	GPlatesPropertyValues::GeoTimeInstant
	convert_float_to_geo_time_instant(
			const double &time_value);
}

#endif // GPLATES_API_PYGEOTIMEINSTANT_H
