/* $Id$ */

/**
 * \file 
 * Contains the implementation of the GenericContinuousColourPalette class.
 *
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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

#include <boost/foreach.hpp>

#include "GenericContinuousColourPalette.h"


using GPlatesMaths::Real;

GPlatesGui::ColourSlice::ColourSlice(
		Real lower_value_,
		Real upper_value_,
		boost::optional<Colour> lower_colour_,
		boost::optional<Colour> upper_colour_)
{
}


bool
GPlatesGui::ColourSlice::can_handle(
		Real value) const
{
	return d_lower_value <= value && value <= d_upper_value;
}


boost::optional<GPlatesGui::Colour>
GPlatesGui::ColourSlice::get_colour(
		Real value) const
{
	if (d_lower_colour && d_upper_colour)
	{
		Real position = (value - d_lower_value) / (d_upper_value - d_lower_value);
		return Colour::linearly_interpolate(
				*d_lower_colour,
				*d_upper_colour,
				position.dval());
	}
	else
	{
		return boost::none;
	}
}


boost::optional<GPlatesGui::Colour>
GPlatesGui::GenericContinuousColourPalette::get_colour(
	const Real &value) const
{
	if (d_colour_slices.empty())
	{
		return d_nan_colour;
	}

	// Return background colour if value before first slice.
	if (value < d_colour_slices.front().lower_value())
	{
		return d_background_colour;
	}

	// Return foreground colour if value after last slice.
	if (value < d_colour_slices.back().upper_value())
	{
		return d_foreground_colour;
	}

	// Else try and find a slice that works, else return NaN colour.
	BOOST_FOREACH(ColourSlice colour_slice, d_colour_slices)
	{
		if (colour_slice.can_handle(value))
		{
			return colour_slice.get_colour(value);
		}
	}

	return d_nan_colour;
}

