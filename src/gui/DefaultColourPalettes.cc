/* $Id$ */

/**
 * @file 
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2010, 2011 The University of Sydney, Australia
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

#include "DefaultColourPalettes.h"

#include "ColourPaletteAdapter.h"
#include "CptColourPalette.h"


namespace
{
	using GPlatesGui::Colour;

	// These colours are arbitrary.
	const Colour DEFAULT_SCALAR_COLOURS[] = {
			Colour(0, 0, 1) /* blue - low */,
			Colour(0, 1, 1) /* cyan */,
			Colour(0, 1, 0) /* green - middle */,
			Colour(1, 1, 0) /* yellow */,
			Colour(1, 0, 0) /* red - high */
	};

	const unsigned int NUM_DEFAULT_RASTER_COLOURS =
			sizeof(DEFAULT_SCALAR_COLOURS) / sizeof(Colour);

	const double DEFAULT_SCALAR_LOWER_BOUND = 0;
	const double DEFAULT_SCALAR_UPPER_BOUND = 1;
}


GPlatesGui::ColourPalette<double>::non_null_ptr_type
GPlatesGui::DefaultColourPalettes::create_scalar_colour_palette()
{
	RegularCptColourPalette::non_null_ptr_type colour_palette = RegularCptColourPalette::create();

	// [min, max] is the range [0, 1].
	const double min = DEFAULT_SCALAR_LOWER_BOUND;
	const double max = DEFAULT_SCALAR_UPPER_BOUND;
	const double range = max - min;

	// Background colour, for values before min value.
	colour_palette->set_background_colour(DEFAULT_SCALAR_COLOURS[0]);

	// Foreground colour, for values after max value.
	colour_palette->set_foreground_colour(DEFAULT_SCALAR_COLOURS[NUM_DEFAULT_RASTER_COLOURS - 1]);

	// Add the colour slices for everything in between.
	for (unsigned int i = 0; i != NUM_DEFAULT_RASTER_COLOURS - 1; ++i)
	{
		ColourSlice colour_slice(
				min + i * range / (NUM_DEFAULT_RASTER_COLOURS - 1),
				DEFAULT_SCALAR_COLOURS[i],
				min + (i + 1) * range / (NUM_DEFAULT_RASTER_COLOURS - 1),
				DEFAULT_SCALAR_COLOURS[i + 1]);
		colour_palette->add_entry(colour_slice);
	}

	// Convert/adapt Real to double.
	return convert_colour_palette<
			RegularCptColourPalette::key_type,
			double,
			RealToBuiltInConverter<double> >(
					colour_palette,
					RealToBuiltInConverter<double>());
}


GPlatesGui::ColourPalette<double>::non_null_ptr_type
GPlatesGui::DefaultColourPalettes::create_gradient_colour_palette()
{
	RegularCptColourPalette::non_null_ptr_type colour_palette = RegularCptColourPalette::create();

	// Background colour, for values before -1.
	colour_palette->set_background_colour(Colour(0, 0, 1) /* blue - high */);

	// Foreground colour, for values after +1.
	colour_palette->set_foreground_colour(Colour(1, 0, 1) /* magenta - high */);

	// Add the colour slices for the range [-1,1].
	colour_palette->add_entry(
			ColourSlice(
					-1, Colour(0, 0, 1) /* blue - high gradient magnitude */,
					-0.5, Colour(0, 1, 1) /* cyan - mid gradient magnitude */));
	colour_palette->add_entry(
			ColourSlice(
					-0.5, Colour(0, 1, 1) /* cyan - mid gradient magnitude */,
					 0, Colour(0, 1, 0) /* green - low gradient magnitude */));
	colour_palette->add_entry(
			ColourSlice(
					0, Colour(1, 1, 0) /* yellow - low gradient magnitude */,
					0.5, Colour(1, 0, 0) /* red - mid gradient magnitude */));
	colour_palette->add_entry(
			ColourSlice(
					0.5, Colour(1, 0, 0) /* red - mid gradient magnitude */,
					1, Colour(1, 0, 1) /* magenta - high gradient magnitude */));

	// Convert/adapt Real to double.
	return convert_colour_palette<
			RegularCptColourPalette::key_type,
			double,
			RealToBuiltInConverter<double> >(
					colour_palette,
					RealToBuiltInConverter<double>());
}


GPlatesGui::ColourPalette<double>::non_null_ptr_type
GPlatesGui::DefaultColourPalettes::create_deformation_strain_colour_palette(
		const double &range1_max,
		const double &range1_min,
		const double &range2_max,
		const double &range2_min,
		const Colour &fg_colour,
		const Colour &max_colour,
		const Colour &mid_colour,
		const Colour &min_colour,
		const Colour &bg_colour)
{
	RegularCptColourPalette::non_null_ptr_type colour_palette = RegularCptColourPalette::create();

	// Note Add the lowest values first, that is, from Range2:

	// Background colour, for values before min value.
	colour_palette->set_background_colour(bg_colour);

    // NOTE: 
	// the 'inversion' in the slices below is on purpose 
	// so that the most intese colours are the smallest values

	// Add the slice from range2_min to range2_max
	const ColourSlice colour_slice_range2(
			range2_min,
			min_colour,
			range2_max,
			mid_colour);
	colour_palette->add_entry(colour_slice_range2);

	// Add the middle to the spectrum
	const ColourSlice colour_slice_mid(
			range2_max,
			mid_colour,
			range1_min,
			mid_colour);
	colour_palette->add_entry(colour_slice_mid);

	// Add the slice from range1_min to range1_max
	const ColourSlice colour_slice_range1(
			range1_min,
			mid_colour,
			range1_max,
			max_colour);
	colour_palette->add_entry(colour_slice_range1);

	// Foreground colour, for values after max value.
	colour_palette->set_foreground_colour(fg_colour);

	// Set nan colour
	colour_palette->set_nan_colour( Colour(0.5, 0.5, 0.5) );

	// Convert/adapt Real to double.
	return convert_colour_palette<
			RegularCptColourPalette::key_type,
			double,
			RealToBuiltInConverter<double> >(
					colour_palette,
					RealToBuiltInConverter<double>());
}
