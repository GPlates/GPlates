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

#include "AgeColourTable.h"
#include "feature-visitors/GmlTimePeriodFinder.h"
#include "model/ReconstructedFeatureGeometry.h"
#include "ColourSpectrum.h"

GPlatesGui::AgeColourTable *
GPlatesGui::AgeColourTable::Instance()
{
	if (d_instance == NULL) {

		// create a new instance
		d_instance = new AgeColourTable();
	}
	return d_instance;
}

GPlatesGui::AgeColourTable::AgeColourTable()
{
	d_colours = &(GPlatesGui::ColourSpectrum::Instance()->get_colour_spectrum());
	d_colour_scale_factor = 10;
}

GPlatesGui::ColourTable::const_iterator
GPlatesGui::AgeColourTable::lookup(
		const GPlatesModel::ReconstructedFeatureGeometry &feature_geometry) const
{
	GPlatesGui::ColourTable::const_iterator colour = NULL;

	if( ! feature_geometry.reconstruction_feature_time()) {
		// The feature does not have a gml:validTime property.
		return &GPlatesGui::Colour::MAROON;
	}

	GPlatesPropertyValues::GeoTimeInstant geo_time = *(feature_geometry.reconstruction_feature_time());
	if(geo_time.is_distant_past()) {
		// The feature's time of appearance is the distant past.
		// We cannot calculate the 'age' from the point of view of the current recon time.
		colour = &GPlatesGui::Colour::OLIVE;
		
	} else if(geo_time.is_distant_future()) {
		// The feature's time of appearance is the distant future.
		// What the hell.
		colour = &GPlatesGui::Colour::RED;
		
	} else if (geo_time.is_real()) {
		double age = geo_time.value() - d_viewport_window->reconstruction_time();
		if( age < 0 ) {
			// The feature shouldn't exist yet.
			// If (for some reason) we are drawing things without regard to their
			// valid time, we will display this with the same colour as the
			// 'distant past' case.
			colour = &GPlatesGui::Colour::OLIVE; 
			
		} else {
			// A valid time of appearance with a usable 'age' relative to the
			// current reconstruction time.
			unsigned long index = static_cast<unsigned long>(age)*d_colour_scale_factor;
			index = index%d_colours->size();
			colour = &((*d_colours)[index]);
			
		}
	}

	return colour;
}

void 
GPlatesGui::AgeColourTable::set_colour_scale_factor(
		int factor)
{
	d_colour_scale_factor = factor;
}


void 
GPlatesGui::AgeColourTable::set_viewport_window(
		const GPlatesQtWidgets::ViewportWindow &viewport)
{
	d_viewport_window = &viewport;
}

GPlatesGui::AgeColourTable *
GPlatesGui::AgeColourTable::d_instance;
