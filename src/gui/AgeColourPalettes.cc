/* $Id$ */

/**
 * @file 
 * Contains the implementation of the DefaultAgeColourPalette class.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#include "AgeColourPalettes.h"
#include "Colour.h"
#include "ColourSpectrum.h" // FIXME: remove
#include "app-logic/Reconstruct.h"
#include "maths/Real.h"


namespace
{
	GPlatesGui::Colour
	get_colour_from_age(
			double age)
	{
		static const int COLOUR_SCALE_FACTOR = 10; // value from old AgeColourTable

		std::vector<GPlatesGui::Colour> &colours =
			GPlatesGui::ColourSpectrum::instance().get_colour_spectrum();

		// A valid time of appearance with a usable 'age' relative to the
		// current reconstruction time.
		unsigned long index = static_cast<unsigned long>(age) * COLOUR_SCALE_FACTOR;
		return colours[index % colours.size()];
	}
}


GPlatesGui::Colour
GPlatesGui::DefaultAgeColourPalette::DISTANT_PAST_COLOUR = GPlatesGui::Colour::get_olive();


GPlatesGui::Colour
GPlatesGui::DefaultAgeColourPalette::DISTANT_FUTURE_COLOUR = GPlatesGui::Colour::get_red();


boost::optional<GPlatesGui::Colour>
GPlatesGui::DefaultAgeColourPalette::get_colour(
		const GPlatesMaths::Real &age) const
{
	if (age.is_negative_infinity())
	{
		// Distant past
		return boost::optional<GPlatesGui::Colour>(DISTANT_PAST_COLOUR);
	}
	else if (age.is_positive_infinity())
	{
		// Distant future
		return boost::optional<GPlatesGui::Colour>(DISTANT_FUTURE_COLOUR);
	}
	else if (age < 0)
	{
		// The feature shouldn't exist yet.
		// If (for some reason) we are drawing things without regard to their
		// valid time, we will display this with the same colour as the
		// 'distant past' case.
		return boost::optional<GPlatesGui::Colour>(DISTANT_PAST_COLOUR);
	}
	else // age >= 0
	{
		return boost::optional<GPlatesGui::Colour>(get_colour_from_age(age.dval()));
	}
}


boost::optional<GPlatesGui::Colour>
GPlatesGui::MonochromeAgeColourPalette::get_colour(
		const GPlatesMaths::Real &age) const
{
	if (age > UPPER_BOUND)
	{
		return Colour::get_white();
	}
	else if (age < LOWER_BOUND)
	{
		return Colour::get_black();
	}
	else
	{
		double position = (age.dval() - LOWER_BOUND) / (UPPER_BOUND - LOWER_BOUND);
		return Colour::linearly_interpolate(
				Colour::get_black(),
				Colour::get_white(),
				position);
	}
}

