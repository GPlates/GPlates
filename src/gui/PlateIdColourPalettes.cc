/* $Id$ */

/**
 * @file 
 * Contains the implementation of the DefaultPlateIdColourPalette and
 * RegionalPlateIdColourPalette classes.
 *
 * Most recent change:
 *   $Date$
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

#include "PlateIdColourPalettes.h"
#include "HTMLColourNames.h"
#include <cmath>

namespace
{
	using namespace GPlatesGui;

	int leading_digit(
			GPlatesModel::integer_plate_id_type plate_id)
	{
		while (plate_id >= 10)
		{
			plate_id /= 10;
		}
		return static_cast<int>(plate_id);
	}

	int
	get_region_from_plate_id(
			GPlatesModel::integer_plate_id_type plate_id)
	{
		if (plate_id < 100) // plate 0xx is treated as being in region zero
		{
			return 0;
		}
		else
		{
			return leading_digit(plate_id);
		}
	}
}

// There are intentionally 13 colours - see ColourByPlateId on GPlates wiki
GPlatesGui::Colour
GPlatesGui::DefaultPlateIdColourPalette::DEFAULT_COLOUR_ARRAY[] = {
	/*  0 */ Colour::get_maroon(),
	/*  1 */ *(HTMLColourNames::instance().get_colour("lightsalmon")),
	/*  2 */ Colour::get_lime(),
	/*  3 */ Colour::get_fuchsia(),
	/*  4 */ Colour::get_navy(),
	/*  5 */ Colour::get_yellow(),
	/*  6 */ Colour::get_blue(),
	/*  7 */ Colour::get_purple(),
	/*  8 */ Colour::get_red(),
	/*  9 */ Colour::get_green(),
	/* 10 */ Colour::get_aqua(),
	/* 11 */ *(HTMLColourNames::instance().get_colour("slategray")),
	/* 12 */ *(HTMLColourNames::instance().get_colour("orange")) 
};

size_t
GPlatesGui::DefaultPlateIdColourPalette::DEFAULT_COLOUR_ARRAY_SIZE =
	sizeof(GPlatesGui::DefaultPlateIdColourPalette::DEFAULT_COLOUR_ARRAY) /
	sizeof(GPlatesGui::Colour);

boost::optional<GPlatesGui::Colour>
GPlatesGui::DefaultPlateIdColourPalette::get_colour(
		GPlatesModel::integer_plate_id_type plate_id) const
{
	// Note: integer_plate_id_type is unsigned, so always >= 0
	return boost::optional<Colour>(
			DEFAULT_COLOUR_ARRAY[plate_id % DEFAULT_COLOUR_ARRAY_SIZE]);
}

// There are intentionally 10 colours (only 10 possible leading digits)
GPlatesGui::Colour
GPlatesGui::RegionalPlateIdColourPalette::REGIONAL_COLOUR_ARRAY[] = {
	/*  0 */ Colour::get_olive(),
	/*  1 */ Colour::get_red(),
	/*  2 */ Colour::get_blue(),
	/*  3 */ Colour::get_lime(),
	/*  4 */ *(HTMLColourNames::instance().get_colour("mistyrose")),
	/*  5 */ Colour::get_aqua(),
	/*  6 */ Colour::get_yellow(),
	/*  7 */ *(HTMLColourNames::instance().get_colour("orange")),
	/*  8 */ Colour::get_purple(),
	/*  9 */ *(HTMLColourNames::instance().get_colour("slategray"))
};

size_t
GPlatesGui::RegionalPlateIdColourPalette::REGIONAL_COLOUR_ARRAY_SIZE =
	sizeof(GPlatesGui::RegionalPlateIdColourPalette::REGIONAL_COLOUR_ARRAY) /
	sizeof(GPlatesGui::Colour);

boost::optional<GPlatesGui::Colour>
GPlatesGui::RegionalPlateIdColourPalette::get_colour(
		GPlatesModel::integer_plate_id_type plate_id) const
{
	int region = get_region_from_plate_id(plate_id);
	HSVColour hsv = Colour::to_hsv(REGIONAL_COLOUR_ARRAY[region]);

	// spread the v values from 0.6-1.0
	const double V_MIN = 0.6; // why 0.6? enough variation while not being too dark
	const double V_MAX = 1.0;
	const int V_STEPS = 13; // why 13? same rationale as for DEFAULT_COLOUR_ARRAY above
	hsv.v = (plate_id % V_STEPS) / static_cast<double>(V_STEPS) * (V_MAX - V_MIN) + V_MIN;
	return boost::optional<Colour>(Colour::from_hsv(hsv));
}

