/* $Id: Colour.h 11559 2011-05-16 07:41:01Z mchin $ */

/**
 * @file 
 * Contains the definition of the Colour class.
 *
 * Most recent change:
 *   $Date: 2011-05-16 17:41:01 +1000 (Mon, 16 May 2011) $
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009, 2010 The University of Sydney, Australia
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

#include "app-logic/PropertyExtractors.h"
#include "global/CompilerWarnings.h"
#include "AgeColourPalettes.h"
#include "DrawStyleManager.h"
#include "FeatureTypeColourPalette.h"
#include "GenericColourScheme.h"
#include "HTMLColourNames.h"
#include "PlateIdColourPalettes.h"
#include "SingleColourScheme.h"

DISABLE_GCC_WARNING("-Wold-style-cast")


GPlatesGui::DrawStyleManager::DrawStyleManager() 
{ 
	init_built_in_styles();
}

void
GPlatesGui::DrawStyleManager::init_built_in_styles()
{
	using namespace GPlatesAppLogic;
	const StyleCatagory* plate_id = register_style_catagory(COLOUR_PLATE_ID,"colour by plate id", true);
	const StyleCatagory* single = register_style_catagory(COLOUR_SINGLE,"single color",true);
	const StyleCatagory* feature_age = register_style_catagory(COLOUR_FEATURE_AGE, "colour by feature age", true);
	const StyleCatagory* feature_type = register_style_catagory(COLOUR_FEATURE_TYPE, "colour by feature type", true);
	
	
	register_style( 
		new ColourStyleAdapter(
				boost::shared_ptr<ColourScheme>(new GenericColourScheme<PlateIdPropertyExtractor>(
						DefaultPlateIdColourPalette::create(),
						PlateIdPropertyExtractor())),
				plate_id,
				"Default"));

	register_style(
		new ColourStyleAdapter(
				boost::shared_ptr<ColourScheme>(new GenericColourScheme<PlateIdPropertyExtractor>(
						RegionalPlateIdColourPalette::create(),
						PlateIdPropertyExtractor())),
				plate_id,
				"Group by Region"));
	

	register_style(
		new ColourStyleAdapter(
				boost::shared_ptr<ColourScheme>( 
						new SingleColourScheme(Colour::get_white())),
				single,
				"white"));
	register_style( 
		new ColourStyleAdapter(
				boost::shared_ptr<ColourScheme>( 
						new SingleColourScheme(Colour::get_black())),
				single,
				"black"));
	register_style( 
		new ColourStyleAdapter(
				boost::shared_ptr<ColourScheme>( 
						new SingleColourScheme(Colour::get_silver())),
				single,
				"silver"));
	register_style( 
		new ColourStyleAdapter(
				boost::shared_ptr<ColourScheme>( 
						new SingleColourScheme(*HTMLColourNames::instance().get_colour("gold"))),
				single,
				"gold"));
	register_style( 
		new ColourStyleAdapter(
				boost::shared_ptr<ColourScheme>( 
						new SingleColourScheme(*HTMLColourNames::instance().get_colour("deepskyblue"))),
				single,
				"blue"));
	register_style(
		new ColourStyleAdapter(
				boost::shared_ptr<ColourScheme>( 
						new SingleColourScheme(*HTMLColourNames::instance().get_colour("deeppink"))),
				single,
				"pink"));
	register_style( 
		new ColourStyleAdapter(
				boost::shared_ptr<ColourScheme>( 
						new SingleColourScheme(*HTMLColourNames::instance().get_colour("chartreuse"))),
				single,
				"green"));
	register_style( 
		new ColourStyleAdapter(
				boost::shared_ptr<ColourScheme>( 
						new SingleColourScheme(*HTMLColourNames::instance().get_colour("darkorange"))),
				single,
				"orange"));
	
	register_style( 
		new ColourStyleAdapter(
				boost::shared_ptr<ColourScheme>(new GenericColourScheme<AgePropertyExtractor>(
						DefaultAgeColourPalette::create(),
						AgePropertyExtractor())),
				feature_age,
				"Default"));

	register_style( 
		new ColourStyleAdapter(
				boost::shared_ptr<ColourScheme>(new GenericColourScheme<AgePropertyExtractor>(
						MonochromeAgeColourPalette::create(),
						AgePropertyExtractor())),
				feature_age,
				"Monochrome"));
	
	register_style(  
		new ColourStyleAdapter(
				boost::shared_ptr<ColourScheme>(new GenericColourScheme<FeatureTypePropertyExtractor>(
						FeatureTypeColourPalette::create(),
						FeatureTypePropertyExtractor())),
				feature_type,
				"Default"));
}


















