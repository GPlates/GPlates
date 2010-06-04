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

#include <vector>

#include "PlateIdColourPalettes.h"
#include "HTMLColourNames.h"

#include "utils/Singleton.h"


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


	GPlatesGui::Colour
	html_colour(
			const char *name)
	{
		static HTMLColourNames &html_colours = HTMLColourNames::instance();
		return *html_colours.get_colour(name);
	}


	class DefaultColours
	{
	public:

		DefaultColours()
		{
			// There are intentionally 11 colours because it does the best job of
			// assigning adjacent plates different colours in the sample data coastlines
			// file.
			d_colours.reserve(11);
			/*  0 */ d_colours.push_back(Colour::get_yellow());
			/*  1 */ d_colours.push_back(Colour::get_aqua());
			/*  2 */ d_colours.push_back(html_colour("seagreen"));
			/*  3 */ d_colours.push_back(Colour::get_fuchsia());
			/*  4 */ d_colours.push_back(html_colour("slategray"));
			/*  5 */ d_colours.push_back(Colour::get_lime());
			/*  6 */ d_colours.push_back(html_colour("indigo"));
			/*  7 */ d_colours.push_back(Colour::get_red());
			/*  8 */ d_colours.push_back(html_colour("orange"));
			/*  9 */ d_colours.push_back(html_colour("lightsalmon"));
			/* 10 */ d_colours.push_back(Colour::get_navy());
		}

		const std::vector<GPlatesGui::Colour> &
		get_colours() const
		{
			return d_colours;
		}

	private:

		std::vector<Colour> d_colours;
	};


	class RegionalColours
	{
	public:

		RegionalColours()
		{
			// There are intentionally 10 colours (only 10 possible leading digits)
			d_colours.reserve(10);
			/*  0 */ d_colours.push_back(Colour::get_olive());
			/*  1 */ d_colours.push_back(Colour::get_red());
			/*  2 */ d_colours.push_back(Colour::get_blue());
			/*  3 */ d_colours.push_back(Colour::get_lime());
			/*  4 */ d_colours.push_back(html_colour("mistyrose"));
			/*  5 */ d_colours.push_back(Colour::get_aqua());
			/*  6 */ d_colours.push_back(Colour::get_yellow());
			/*  7 */ d_colours.push_back(html_colour("orange"));
			/*  8 */ d_colours.push_back(Colour::get_purple());
			/*  9 */ d_colours.push_back(html_colour("slategray"));
		}

		const std::vector<GPlatesGui::Colour> &
		get_colours() const
		{
			return d_colours;
		}

	private:

		std::vector<Colour> d_colours;
	};
}


GPlatesGui::DefaultPlateIdColourPalette::non_null_ptr_type
GPlatesGui::DefaultPlateIdColourPalette::create()
{
	return new DefaultPlateIdColourPalette();
}


boost::optional<GPlatesGui::Colour>
GPlatesGui::DefaultPlateIdColourPalette::get_colour(
		value_type plate_id) const
{
	const std::vector<GPlatesGui::Colour> &colours =
		GPlatesUtils::Singleton<DefaultColours>::instance().get_colours();

	// Note: integer_plate_id_type is unsigned, so always >= 0
	return boost::optional<Colour>(
			colours[plate_id % colours.size()]);
}


GPlatesGui::RegionalPlateIdColourPalette::non_null_ptr_type
GPlatesGui::RegionalPlateIdColourPalette::create()
{
	return new RegionalPlateIdColourPalette();
}


boost::optional<GPlatesGui::Colour>
GPlatesGui::RegionalPlateIdColourPalette::get_colour(
		value_type plate_id) const
{
	int region = get_region_from_plate_id(plate_id);
	HSVColour hsv = Colour::to_hsv(
			GPlatesUtils::Singleton<RegionalColours>::instance().get_colours()[region]);

	// spread the v values from 0.6-1.0
	const double V_MIN = 0.6; // why 0.6? enough variation while not being too dark
	const double V_MAX = 1.0;
	const int V_STEPS = 13; // why 13? same rationale as for DEFAULT_COLOUR_ARRAY above
	hsv.v = (plate_id % V_STEPS) / static_cast<double>(V_STEPS) * (V_MAX - V_MIN) + V_MIN;
	return Colour::from_hsv(hsv);
}

