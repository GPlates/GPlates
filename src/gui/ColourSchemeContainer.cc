/* $Id$ */

/**
 * @file 
 * Contains the defintion of the ColourSchemeContainer class.
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
#include "ColourSchemeContainer.h"
#include "FeatureTypeColourPalette.h"
#include "GenericColourScheme.h"
#include "HTMLColourNames.h"
#include "PlateIdColourPalettes.h"
#include "SingleColourScheme.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/PropertyExtractors.h"


GPlatesGui::ColourSchemeInfo::ColourSchemeInfo(
		ColourScheme::non_null_ptr_type colour_scheme_ptr_,
		QString short_description_,
		QString long_description_,
		bool is_built_in_) :
	colour_scheme_ptr(colour_scheme_ptr_),
	short_description(short_description_),
	long_description(long_description_),
	is_built_in(is_built_in_)
{
}


GPlatesGui::ColourSchemeCategory::Iterator
GPlatesGui::ColourSchemeCategory::begin()
{
	return Iterator(static_cast<Type>(0));
}


GPlatesGui::ColourSchemeCategory::Iterator
GPlatesGui::ColourSchemeCategory::end()
{
	return Iterator(NUM_CATEGORIES);
}


QString
GPlatesGui::ColourSchemeCategory::get_description(
		Type category)
{
	switch (category)
	{
		case PLATE_ID:
			return "Plate ID";

		case SINGLE_COLOUR:
			return "Single Colour";

		case FEATURE_AGE:
			return "Feature Age";

		case FEATURE_TYPE:
			return "Feature Type";

		default:
			return QString();
	}
}


GPlatesGui::ColourSchemeContainer::ColourSchemeContainer(
		GPlatesAppLogic::ApplicationState &application_state) :
	d_next_id(0)
{
	create_built_in_colour_schemes(application_state);
}


GPlatesGui::ColourSchemeContainer::iterator
GPlatesGui::ColourSchemeContainer::begin(
		ColourSchemeCategory::Type category) const
{
	return d_colour_schemes[category].begin();
}


GPlatesGui::ColourSchemeContainer::iterator
GPlatesGui::ColourSchemeContainer::end(
		ColourSchemeCategory::Type category) const
{
	return d_colour_schemes[category].end();
}


GPlatesGui::ColourSchemeContainer::id_type
GPlatesGui::ColourSchemeContainer::add(
		ColourSchemeCategory::Type category,
		const ColourSchemeInfo &colour_scheme)
{
	d_colour_schemes[category].insert(
			std::make_pair(d_next_id, colour_scheme));
	return d_next_id++;
}


void
GPlatesGui::ColourSchemeContainer::remove(
		ColourSchemeCategory::Type category,
		id_type id)
{
	container_type::iterator iter = d_colour_schemes[category].find(id);
	if (iter != d_colour_schemes[category].end())
	{
		d_colour_schemes[category].erase(iter);
	}
}


const GPlatesGui::ColourSchemeInfo &
GPlatesGui::ColourSchemeContainer::get(
		ColourSchemeCategory::Type category,
		id_type id) const
{
	iterator iter = d_colour_schemes[category].find(id);
	return iter->second;
}


void
GPlatesGui::ColourSchemeContainer::create_built_in_colour_schemes(
		GPlatesAppLogic::ApplicationState &application_state)
{
	// Plate ID colouring schemes:
	add(
			ColourSchemeCategory::PLATE_ID,
			ColourSchemeInfo(
				make_colour_scheme(
					DefaultPlateIdColourPalette::create(),
					GPlatesAppLogic::PlateIdPropertyExtractor()),
				"Default",
				"Colour geometries by plate ID in a manner that visually distinguishes nearby plates",
				true));
	add(
			ColourSchemeCategory::PLATE_ID,
			ColourSchemeInfo(
				make_colour_scheme(
					RegionalPlateIdColourPalette::create(),
					GPlatesAppLogic::PlateIdPropertyExtractor()),
				"Group by Region",
				"Colour geometries by plate ID such that plates with the same leading digit have similar colours",
				true));

	// Single Colour colouring schemes:
	add_single_colour_scheme(
			Colour::get_white(),
			"white");
	add_single_colour_scheme(
			Colour::get_black(),
			"black");
	add_single_colour_scheme(
			Colour::get_silver(),
			"silver");
	add_single_colour_scheme(
			*HTMLColourNames::instance().get_colour("gold"),
			"gold");
	add_single_colour_scheme(
			*HTMLColourNames::instance().get_colour("deepskyblue"),
			"blue");
	add_single_colour_scheme(
			*HTMLColourNames::instance().get_colour("deeppink"),
			"pink");
	add_single_colour_scheme(
			*HTMLColourNames::instance().get_colour("chartreuse"),
			"green");
	add_single_colour_scheme(
			*HTMLColourNames::instance().get_colour("darkorange"),
			"orange");

	// Feature Age colour schemes:
	add(
			ColourSchemeCategory::FEATURE_AGE,
			ColourSchemeInfo(
				make_colour_scheme(
					DefaultAgeColourPalette::create(),
					GPlatesAppLogic::AgePropertyExtractor(application_state)),
				"Default",
				"Colour geometries by age based on the current reconstruction time",
				true));
	add(
			ColourSchemeCategory::FEATURE_AGE,
			ColourSchemeInfo(
				make_colour_scheme(
					MonochromeAgeColourPalette::create(),
					GPlatesAppLogic::AgePropertyExtractor(application_state)),
				"Monochrome",
				"Colour geometries by age based on the current reconstruction time using shades of grey",
				true));

	// Feature Type colour schemes:
	add(
			ColourSchemeCategory::FEATURE_TYPE,
			ColourSchemeInfo(
				make_colour_scheme(
					FeatureTypeColourPalette::create(),
					GPlatesAppLogic::FeatureTypePropertyExtractor()),
				"Default",
				"Colour geometries by feature type",
				true));
}


GPlatesGui::ColourSchemeContainer::id_type
GPlatesGui::ColourSchemeContainer::add_single_colour_scheme(
		const Colour &colour,
		const QString &colour_name,
		bool is_built_in)
{
	return add(
			ColourSchemeCategory::SINGLE_COLOUR,
			create_single_colour_scheme(colour, colour_name, is_built_in));
}


void
GPlatesGui::ColourSchemeContainer::edit_single_colour_scheme(
		id_type id,
		const Colour &colour,
		const QString &colour_name)
{
	container_type::iterator iter = d_colour_schemes[ColourSchemeCategory::SINGLE_COLOUR].find(id);
	if (iter != d_colour_schemes[ColourSchemeCategory::SINGLE_COLOUR].end())
	{
		iter->second = create_single_colour_scheme(
				colour,
				colour_name,
				false /* should only edit non-built-in single colour schemes */);

		Q_EMIT colour_scheme_edited(ColourSchemeCategory::SINGLE_COLOUR, id);
	}
}


GPlatesGui::ColourSchemeInfo
GPlatesGui::ColourSchemeContainer::create_single_colour_scheme(
		const Colour &colour,
		const QString &colour_name,
		bool is_built_in)
{
	QString capitalised_colour_name = colour_name;
	capitalised_colour_name[0] = capitalised_colour_name[0].toUpper();

	return ColourSchemeInfo(
			make_single_colour_scheme(colour),
			capitalised_colour_name,
			"Colour all geometries " + colour_name,
			is_built_in);
}
