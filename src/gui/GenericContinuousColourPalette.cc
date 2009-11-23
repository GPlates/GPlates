/* $Id$ */

/**
 * \file 
 * Contains the implementation of the GenericContinuousColourPalette class.
 *
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include <algorithm>

#include "GenericContinuousColourPalette.h"
#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

GPlatesGui::GenericContinuousColourPalette::GenericContinuousColourPalette(
		std::map<GPlatesMaths::Real, Colour> control_points) :
	d_control_points(control_points)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			!control_points.empty(),
			GPLATES_ASSERTION_SOURCE);
}

boost::optional<GPlatesGui::Colour>
GPlatesGui::GenericContinuousColourPalette::get_colour(
		const GPlatesMaths::Real &value) const
{
	typedef std::map<GPlatesMaths::Real, Colour>::const_iterator iterator_type;
	iterator_type upper_value = d_control_points.upper_bound(value);
		// first control value strictly greater than given value

	if (upper_value == d_control_points.end())
	{
		// given value is greater than or equal to largest control value; return last colour
		iterator_type last = d_control_points.end();
		--last;
		return last->second;
	}
	else
	{
		if (upper_value == d_control_points.begin())
		{
			// given value is before first control point; return first colour
			return upper_value->second;
		}
		else
		{
			// in the middle; get the previous control point and linearly interpolate
			iterator_type lower_value = upper_value;
			--lower_value;
			double position = ((value - lower_value->first) / (upper_value->first - lower_value->first)).dval();
			return Colour::linearly_interpolate(
					lower_value->second,
					upper_value->second,
					position);
		}
	}
}

