/* $Id$ */

/**
 * @file 
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

#include "RasterColourPalette.h"


namespace
{
	using GPlatesGui::Colour;

	// These colours are arbitrary - maybe replace them with colours appropriate
	// for the type of raster that we have.
	const Colour DEFAULT_RASTER_COLOURS[] = {
		Colour(0, 0, 1) /* blue - low */,
		Colour(0, 1, 1) /* cyan */,
		Colour(0, 1, 0) /* green - middle */,
		Colour(1, 1, 0) /* yellow */,
		Colour(1, 0, 0) /* red - high */
	};

	const unsigned int NUM_DEFAULT_RASTER_COLOURS =
		sizeof(DEFAULT_RASTER_COLOURS) / sizeof(Colour);
}


GPlatesGui::DefaultRasterColourPalette::DefaultRasterColourPalette(
		double mean,
		double std_dev) :
	d_inner_palette(
			RegularCptColourPalette::create())
{
	double min = mean - NUM_STD_DEV_AWAY_FROM_MEAN * std_dev;
	double max = mean + NUM_STD_DEV_AWAY_FROM_MEAN * std_dev;
	double range = max - min;

	// Background colour, for values before min value.
	d_inner_palette->set_background_colour(
			DEFAULT_RASTER_COLOURS[0]);

	// Foreground colour, for values after max value.
	d_inner_palette->set_foreground_colour(
			DEFAULT_RASTER_COLOURS[NUM_DEFAULT_RASTER_COLOURS - 1]);

	// Add the colour slices for everything in between.
	for (unsigned int i = 0; i != NUM_DEFAULT_RASTER_COLOURS - 1; ++i)
	{
		ColourSlice colour_slice(
				min + i * range / NUM_DEFAULT_RASTER_COLOURS,
				DEFAULT_RASTER_COLOURS[i],
				min + (i + 1) * range / NUM_DEFAULT_RASTER_COLOURS,
				DEFAULT_RASTER_COLOURS[i + 1]);
		d_inner_palette->add_entry(colour_slice);
	}
}


GPlatesGui::DefaultRasterColourPalette::non_null_ptr_type
GPlatesGui::DefaultRasterColourPalette::create(
		double mean,
		double std_dev)
{
	return new DefaultRasterColourPalette(mean, std_dev);
}


boost::optional<GPlatesGui::Colour>
GPlatesGui::DefaultRasterColourPalette::get_colour(
		double value) const
{
	return d_inner_palette->get_colour(value);
}
