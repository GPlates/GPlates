/* $Id$ */

/**
 * @file 
 * Contains the implementation of the FeatureTypeColourPalette class.
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

#include <string>
#include <boost/foreach.hpp>

#include "FeatureTypeColourPalette.h"
#include "HTMLColourNames.h"

#include "model/Gpgim.h"

#include "utils/UnicodeStringUtils.h"

namespace
{
	using namespace GPlatesGui;


	Colour
	map_to_colour(
			unsigned int number)
	{
		static const Colour COLOURS[] = {
			*(HTMLColourNames::instance().get_colour("saddlebrown")),
			Colour::get_yellow(),
			Colour::get_red(),
			Colour::get_blue(),
			Colour::get_green(),
			Colour::get_purple(),
			*(HTMLColourNames::instance().get_colour("orange")),
			*(HTMLColourNames::instance().get_colour("lightskyblue")),
			Colour::get_lime(),
			*(HTMLColourNames::instance().get_colour("lightsalmon")),
			*(HTMLColourNames::instance().get_colour("fuchsia")),
			*(HTMLColourNames::instance().get_colour("greenyellow")),
			*(HTMLColourNames::instance().get_colour("darkslategray")),
			*(HTMLColourNames::instance().get_colour("darkturquoise")),
			*(HTMLColourNames::instance().get_colour("cadetblue")),
			*(HTMLColourNames::instance().get_colour("beige")),
			*(HTMLColourNames::instance().get_colour("lightcoral")),
			*(HTMLColourNames::instance().get_colour("powderblue"))
		};
		static const size_t NUM_COLOURS = sizeof(COLOURS) / sizeof(Colour);

		return COLOURS[number % NUM_COLOURS];
	}


	/*
	* The previous name of this function(hash) conflicts 
	* with a name in boost on mac os 10.7.
	*/
	unsigned int
	generate_hash(
			const GPlatesModel::FeatureType &feature_type)
	{
		// First convert to std::string.
		std::string str = GPlatesUtils::make_std_string_from_icu_string(
				feature_type.build_aliased_name());

		// Then xor all the individual chars together.
		unsigned int result = 0;
		BOOST_FOREACH(char c, str)
		{
			result ^= c;
		}

		return result;
	}


	/**
	 * Assign a colour to a FeatureType.
	 */
	Colour
	create_colour(
			const GPlatesModel::FeatureType &feature_type)
	{
		// Using a hash ensures that the colour associated with a feature type
		// will not change when new feature types are added to GPGIM
		// (previously the integer index of feature type in GPGIM was used)...
		return map_to_colour(generate_hash(feature_type));
	}
}


GPlatesGui::FeatureTypeColourPalette::non_null_ptr_type
GPlatesGui::FeatureTypeColourPalette::create()
{
	return new FeatureTypeColourPalette();
}


GPlatesGui::FeatureTypeColourPalette::FeatureTypeColourPalette()
{
	// Populate the colours map with FeatureTypes that we know about.
	const GPlatesModel::Gpgim::feature_type_seq_type &feature_types =
			GPlatesModel::Gpgim::instance().get_concrete_feature_types();
	BOOST_FOREACH(const GPlatesModel::FeatureType &feature_type, feature_types)
	{
		d_colours.insert(
				std::make_pair(
					feature_type,
					create_colour(feature_type)));
	}

	//
	// Override some feature types with specific colours.
	//

	// These colours were changed from GPlates 1.2 to 1.3 so we'll just leave them as they are
	// (for GPlates 1.3 onwards) - except for "UnclassifiedFeature".
	d_colours[GPlatesModel::FeatureType::create_gpml("UnclassifiedFeature")] = *(HTMLColourNames::instance().get_colour("dimgray"));
	d_colours[GPlatesModel::FeatureType::create_gpml("Coastline")] = *(HTMLColourNames::instance().get_colour("saddlebrown"));
	d_colours[GPlatesModel::FeatureType::create_gpml("MeshNode")] = Colour::get_yellow();
	d_colours[GPlatesModel::FeatureType::create_gpml("Flowline")] = Colour::get_red();
	d_colours[GPlatesModel::FeatureType::create_gpml("Fault")] = Colour::get_blue();
	d_colours[GPlatesModel::FeatureType::create_gpml("MidOceanRidge")] = Colour::get_green();
	d_colours[GPlatesModel::FeatureType::create_gpml("FractureZone")] = Colour::get_purple();
	d_colours[GPlatesModel::FeatureType::create_gpml("HotSpot")] = *(HTMLColourNames::instance().get_colour("orange"));
	d_colours[GPlatesModel::FeatureType::create_gpml("Volcano")] = *(HTMLColourNames::instance().get_colour("lightskyblue"));
	d_colours[GPlatesModel::FeatureType::create_gpml("Basin")] = Colour::get_lime();
	d_colours[GPlatesModel::FeatureType::create_gpml("HeatFlow")] = Colour::get_navy();

	// Some new hard-wired colours below added in GPlates 2.1. In GPlates 2.0 they were all navy blue
	// (default colour 'FeatureTypePalette::d_default_colour' in "Palette.cc") due to above change in GPlates 1.3.
	// From now on, any colours not overridden here will get a random colour in 'map_to_colour()'
	// based on the hash of the feature type (instead of default navy colour). So from now on it's probably best
	// to only add new hard-wired colours here when two feature types should ideally be distinguishable
	// but end up with the same hash number.
	d_colours[GPlatesModel::FeatureType::create_gpml("TopologicalNetwork")] = *(HTMLColourNames::instance().get_colour("tan"));
	d_colours[GPlatesModel::FeatureType::create_gpml("TopologicalClosedPlateBoundary")] = *(HTMLColourNames::instance().get_colour("plum"));
}


boost::optional<GPlatesGui::Colour>
GPlatesGui::FeatureTypeColourPalette::get_colour(
		const GPlatesModel::FeatureType &feature_type) const
{
	std::map<GPlatesModel::FeatureType, Colour>::const_iterator colour =
		d_colours.find(feature_type);
	if (colour == d_colours.end())
	{
		Colour generated_colour = create_colour(feature_type);
		d_colours.insert(
				std::make_pair(
					feature_type,
					generated_colour));
		return generated_colour;
	}
	else
	{
		return colour->second;
	}
}

