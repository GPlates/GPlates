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

#include <boost/static_assert.hpp>

#include "VelocityDeltaTime.h"

#include "global/GPlatesAssert.h"


std::pair<double, double>
GPlatesAppLogic::VelocityDeltaTime::get_time_range(
		Type delta_time_type,
		const double &time,
		const double &delta_time,
		bool allow_negative_range)
{
	if (delta_time_type == T_PLUS_DELTA_T_TO_T)
	{
		return std::make_pair(time + delta_time, time);
	}
	else if (delta_time_type == T_TO_T_MINUS_DELTA_T)
	{
		const double young_time = time - delta_time;
		if (!allow_negative_range &&
			young_time < 0)
		{
			// The time interval is always 'delta_time'.
			return std::make_pair(delta_time, 0.0);
		}

		return std::make_pair(time, young_time);
	}
	else if (delta_time_type == T_PLUS_MINUS_HALF_DELTA_T)
	{
		const double half_delta_time = 0.5 * delta_time;
		const double young_time = time - half_delta_time;
		if (!allow_negative_range &&
			young_time < 0)
		{
			// The time interval is always 'delta_time'.
			return std::make_pair(delta_time, 0.0);
		}

		const double old_time = time + half_delta_time;
		return std::make_pair(old_time, young_time);
	}
	else
	{
		// Update this source code if more enumeration values have been added (or removed).
		BOOST_STATIC_ASSERT(NUM_TYPES == 3);

		// Shouldn't get here.
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
	}

	// Return a dummy value to keep compiler happy - shouldn't get here though.
	return std::make_pair(0.0, 0.0);
}
