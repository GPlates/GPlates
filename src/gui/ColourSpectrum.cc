/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2010 The University of Sydney, Australia
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

#include "ColourSpectrum.h"
#include "Colour.h"


namespace
{
	using namespace GPlatesGui;

	static const Colour COLOURS[] = {
		Colour(1, 0, 0),
		Colour(1, 1, 0),
		Colour(0, 1, 0),
		Colour(0, 1, 1),
		Colour(0, 0, 1),
		Colour(1, 0, 1)
	};

	static const int NUM_RANGES = sizeof(COLOURS) / sizeof(Colour) - 1;
}


GPlatesGui::Colour
GPlatesGui::ColourSpectrum::get_colour_at(
		double position)
{
	int range = static_cast<int>(position * NUM_RANGES);

	// Handle cases where position is outside of [0.0, 1.0].
	if (range < 0)
	{
		return COLOURS[0];
	}
	else if (range >= NUM_RANGES)
	{
		return COLOURS[NUM_RANGES];
	}

	double position_in_range = position * NUM_RANGES - range;

	return Colour::linearly_interpolate(
			COLOURS[range],
			COLOURS[range + 1],
			position_in_range);
}

