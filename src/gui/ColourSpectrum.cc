/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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
#include <vector>

GPlatesGui::ColourSpectrum::ColourSpectrum()
{
	//worked out the ranges from the QtColourDialogue
	//the colour wheel is as follows 
	//R 	G 	B
	//255	0 	0
	//255	0	255
	//0	0	255
	//0	255	255
	//0	255	0
	//255	255	0
	//255	0	0

	static int total_steps = 1784; //number of distinct steps to cycle through the colour wheel
	d_colours.reserve(total_steps);

	int red = 255;
	int green = 0;
	int blue = 0;
	static float channel_max = 255;

	//(255,0,0) -> (255,0,255)
	for( int i = 0; i < 255; i++) {
		++blue;
		Colour colour(red/channel_max, green/channel_max, blue/channel_max);
		d_colours.push_back(colour);	
	}

	//(255,0,255) -> (0,0,255)
	for( int i = 0; i < 255; i++) {
		--red;
		d_colours.push_back(Colour(red/channel_max, green/channel_max, blue/channel_max));		
	}

	//(0,0,255) -> (0,255,255)
	for( int i = 0; i < 255; i++) {
		++green;
		d_colours.push_back(Colour(red/channel_max, green/channel_max, blue/channel_max));		
	}

	//(0,255,255) -> (0,255,0)
	for( int i = 0; i < 255; i++) {
		--blue;
		d_colours.push_back(Colour(red/channel_max, green/channel_max, blue/channel_max));		
	}

	//(0,255,0) -> (255,255,0)
	for( int i = 0; i < 255; i++) {
		++red;
		d_colours.push_back(Colour(red/channel_max, green/channel_max, blue/channel_max));		
	}

	//(255,255,0) -> (255,0,0)
	for( int i = 0; i < 255; i++) {
		--green;
		d_colours.push_back(Colour(red/channel_max, green/channel_max, blue/channel_max));		
	}
}

std::vector<GPlatesGui::Colour> &
GPlatesGui::ColourSpectrum::get_colour_spectrum()
{
	return d_colours;
}

