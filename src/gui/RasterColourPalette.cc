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

#include "RasterColourPalette.h"


namespace
{
	class RasterColourPaletteTypeVisitor :
			public boost::static_visitor<GPlatesGui::RasterColourPaletteType::Type>
	{
	public:

		GPlatesGui::RasterColourPaletteType::Type
		operator()(
				const GPlatesGui::RasterColourPalette::empty &) const
		{
			return GPlatesGui::RasterColourPaletteType::INVALID;
		}

		GPlatesGui::RasterColourPaletteType::Type
		operator()(
				const GPlatesGui::ColourPalette<boost::int32_t>::non_null_ptr_type &) const
		{
			return GPlatesGui::RasterColourPaletteType::INT32;
		}

		GPlatesGui::RasterColourPaletteType::Type
		operator()(
				const GPlatesGui::ColourPalette<boost::uint32_t>::non_null_ptr_type &) const
		{
			return GPlatesGui::RasterColourPaletteType::UINT32;
		}

		GPlatesGui::RasterColourPaletteType::Type
		operator()(
				const GPlatesGui::ColourPalette<double>::non_null_ptr_type &) const
		{
			return GPlatesGui::RasterColourPaletteType::DOUBLE;
		}
	};


	// A visitor to actually get a colour out of the palette
	class RasterColourPaletteGetColourVisitor :
			public boost::static_visitor<boost::optional<GPlatesGui::Colour> >
	{
	public:
		RasterColourPaletteGetColourVisitor(
				const double &value) :
			d_value( value )
		{	
		}

		boost::optional<GPlatesGui::Colour>
		operator()(
				const GPlatesGui::RasterColourPalette::empty &palette) const
		{
			return boost::none;
		}

		boost::optional<GPlatesGui::Colour>
		operator()(
				const GPlatesGui::ColourPalette<boost::int32_t>::non_null_ptr_type &palette) const
		{
			return boost::none;
		}

		boost::optional<GPlatesGui::Colour>
		operator()(
				const GPlatesGui::ColourPalette<boost::uint32_t>::non_null_ptr_type &palette) const
		{
			return boost::none;
		}

		boost::optional<GPlatesGui::Colour>
		operator()(
				const GPlatesGui::ColourPalette<double>::non_null_ptr_type &palette) const
		{
			return palette->get_colour( d_value );
			//return boost::none;
		}
		// The value to look up a colour for 
		const double d_value;
	};
}


// Get the Type of the palette
GPlatesGui::RasterColourPaletteType::Type
GPlatesGui::RasterColourPaletteType::get_type(
		const RasterColourPalette &colour_palette)
{
	return boost::apply_visitor(RasterColourPaletteTypeVisitor(), colour_palette);
}

// Get a colour out of the palette for the value 
boost::optional<GPlatesGui::Colour>
GPlatesGui::RasterColourPaletteColour::get_colour(
		const RasterColourPalette &colour_palette,
		const double &value)
{
	return boost::apply_visitor(RasterColourPaletteGetColourVisitor(value), colour_palette);
}

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
			RegularCptColourPalette::create()),
	d_mean(mean),
	d_std_dev(std_dev)
{
	double min = get_lower_bound();
	double max = get_upper_bound();
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
				min + i * range / (NUM_DEFAULT_RASTER_COLOURS - 1),
				DEFAULT_RASTER_COLOURS[i],
				min + (i + 1) * range / (NUM_DEFAULT_RASTER_COLOURS - 1),
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


double
GPlatesGui::DefaultRasterColourPalette::get_lower_bound() const
{
	return d_mean - NUM_STD_DEV_AWAY_FROM_MEAN * d_std_dev;
}


double
GPlatesGui::DefaultRasterColourPalette::get_upper_bound() const
{
	return d_mean + NUM_STD_DEV_AWAY_FROM_MEAN * d_std_dev;
}


//
// Deformation palette
//
GPlatesGui::DeformationColourPalette::DeformationColourPalette(
		double range1_max,
		double range1_min,
		double range2_max,
		double range2_min,
		GPlatesGui::Colour fg_c,
		GPlatesGui::Colour max_c,
		GPlatesGui::Colour mid_c,
		GPlatesGui::Colour min_c,
		GPlatesGui::Colour bg_c) :
	d_inner_palette(
			RegularCptColourPalette::create()),
	d_range1_max(range1_max),
	d_range1_min(range1_min),
	d_range2_max(range2_max),
	d_range2_min(range2_min),
	d_fg_colour(fg_c),
	d_max_colour(max_c),
	d_mid_colour(mid_c),
	d_min_colour(min_c),
	d_bg_colour(bg_c)
{
	// Note Add the lowest values first, that is, from Range2:

	// Background colour, for values before min value.
	d_inner_palette->set_background_colour(d_bg_colour);

    // NOTE: 
	// the 'inversion' in the slices below is on purpose 
	// so that the most intese colours are the smallest values

	// Add the slice from range2_min to range2_max
	ColourSlice colour_slice_range2(
			d_range2_min,
			d_mid_colour,
			d_range2_max,
			d_min_colour);
	d_inner_palette->add_entry(colour_slice_range2);

	// Add the middle to the spectrum
	ColourSlice colour_slice_mid(
			d_range2_max,
			d_mid_colour,
			d_range1_min,
			d_mid_colour);
	d_inner_palette->add_entry(colour_slice_mid);

	// Add the slice from range1_min to range1_max
	ColourSlice colour_slice_range1(
			d_range1_min,
			d_max_colour,
			d_range1_max,
			d_mid_colour);
	d_inner_palette->add_entry(colour_slice_range1);

	// Foreground colour, for values after max value.
	d_inner_palette->set_foreground_colour(d_fg_colour);

	// Set nan colour
	d_inner_palette->set_nan_colour( Colour(0.5, 0.5, 0.5) );
}

GPlatesGui::DeformationColourPalette::non_null_ptr_type
GPlatesGui::DeformationColourPalette::create()
{
	return new DeformationColourPalette(
		1.0,
		0.0,
		0.0,
		-1.0,
		GPlatesGui::Colour(1, 1, 1), /* white - fg */
		GPlatesGui::Colour(1, 0, 0), /* red - high */
		GPlatesGui::Colour(1, 1, 1), /* white - middle */
		GPlatesGui::Colour(0, 0, 1), /* blue - low */
		GPlatesGui::Colour(1, 1, 1)  /* white - bg */
	);
}


GPlatesGui::DeformationColourPalette::non_null_ptr_type
GPlatesGui::DeformationColourPalette::create(
		double range_1max,
		double range_1min,
		double range_2max,
		double range_2min,
		GPlatesGui::Colour fg_c,
		GPlatesGui::Colour max_c,
		GPlatesGui::Colour mid_c,
		GPlatesGui::Colour min_c,
		GPlatesGui::Colour bg_c)
{
	return new DeformationColourPalette(
		range_1max, 
		range_1min, 
		range_2max, 
		range_2min, 
		fg_c,
		max_c, 
		mid_c, 
		min_c,
		bg_c);
}


boost::optional<GPlatesGui::Colour>
GPlatesGui::DeformationColourPalette::get_colour(
		double value) const
{
	return d_inner_palette->get_colour(value);
}


double
GPlatesGui::DeformationColourPalette::get_lower_bound() const
{
	return d_range2_min;
}


double
GPlatesGui::DeformationColourPalette::get_upper_bound() const
{
	return d_range1_max;
}

//
// User controled palette
//
GPlatesGui::UserColourPalette::UserColourPalette(
		double range1_max,
		double range1_min,
		double range2_max,
		double range2_min,
		GPlatesGui::Colour max_c,
		GPlatesGui::Colour mid_c,
		GPlatesGui::Colour min_c) :
	d_inner_palette(
			RegularCptColourPalette::create()),
	d_range1_max(range1_max),
	d_range1_min(range1_min),
	d_range2_max(range2_max),
	d_range2_min(range2_min),
	d_max_colour(max_c),
	d_mid_colour(mid_c),
	d_min_colour(min_c) 
{
	// Background colour, for values before min value.
	d_inner_palette->set_background_colour(d_min_colour);

	// Note Add the lowest values first, that is, from Range2:

	// Add the slice from range2_min to range2_max
	ColourSlice colour_slice_range2(
			d_range2_min,
			d_min_colour,
			d_range2_max,
			d_mid_colour);
	d_inner_palette->add_entry(colour_slice_range2);

	// Add the middle to the spectrum
	ColourSlice colour_slice_mid(
			d_range2_max,
			d_mid_colour,
			d_range1_min,
			d_mid_colour);
	d_inner_palette->add_entry(colour_slice_mid);

	// Add the slice from range1_min to range1_max
	ColourSlice colour_slice_range1(
			d_range1_min,
			d_mid_colour,
			d_range1_max,
			d_max_colour);
	d_inner_palette->add_entry(colour_slice_range1);

	// Foreground colour, for values after max value.
	d_inner_palette->set_foreground_colour(d_max_colour);

	// Set nan colour
	d_inner_palette->set_nan_colour( Colour(0.5, 0.5, 0.5) );
}

GPlatesGui::UserColourPalette::non_null_ptr_type
GPlatesGui::UserColourPalette::create()
{
	return new UserColourPalette(
		1.0,
		0.0,
		0.0,
		-1.0,
		GPlatesGui::Colour(1, 0, 0) /* red - high */,
		GPlatesGui::Colour(1, 1, 1) /* white - middle */,
		GPlatesGui::Colour(0, 0, 1) /* blue - low */
	);
}


GPlatesGui::UserColourPalette::non_null_ptr_type
GPlatesGui::UserColourPalette::create(
		double range_1max,
		double range_1min,
		double range_2max,
		double range_2min,
		GPlatesGui::Colour max_c,
		GPlatesGui::Colour mid_c,
		GPlatesGui::Colour min_c)
{
	return new UserColourPalette(
		range_1max, 
		range_1min, 
		range_2max, 
		range_2min, 
		max_c, 
		mid_c, 
		min_c);
}


boost::optional<GPlatesGui::Colour>
GPlatesGui::UserColourPalette::get_colour(
		double value) const
{
	return d_inner_palette->get_colour(value);
}


double
GPlatesGui::UserColourPalette::get_lower_bound() const
{
	return d_range2_min;
}


double
GPlatesGui::UserColourPalette::get_upper_bound() const
{
	return d_range1_max;
}


GPlatesGui::DefaultScalarFieldScalarColourPalette::DefaultScalarFieldScalarColourPalette() :
	d_inner_palette(RegularCptColourPalette::create())
{
	// [min, max] is the range [0, 1].
	double min = get_lower_bound();
	double max = get_upper_bound();
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
				min + i * range / (NUM_DEFAULT_RASTER_COLOURS - 1),
				DEFAULT_RASTER_COLOURS[i],
				min + (i + 1) * range / (NUM_DEFAULT_RASTER_COLOURS - 1),
				DEFAULT_RASTER_COLOURS[i + 1]);
		d_inner_palette->add_entry(colour_slice);
	}
}


GPlatesGui::DefaultScalarFieldScalarColourPalette::non_null_ptr_type
GPlatesGui::DefaultScalarFieldScalarColourPalette::create()
{
	return new DefaultScalarFieldScalarColourPalette();
}


boost::optional<GPlatesGui::Colour>
GPlatesGui::DefaultScalarFieldScalarColourPalette::get_colour(
		double value) const
{
	return d_inner_palette->get_colour(value);
}


GPlatesGui::DefaultScalarFieldGradientColourPalette::DefaultScalarFieldGradientColourPalette() :
	d_inner_palette(RegularCptColourPalette::create())
{
	// Background colour, for values before -1.
	d_inner_palette->set_background_colour(Colour(0, 0, 1) /* blue - high */);

	// Foreground colour, for values after +1.
	d_inner_palette->set_foreground_colour(Colour(1, 0, 1) /* magenta - high */);

	// Add the colour slices for the range [-1,1].
	d_inner_palette->add_entry(
			ColourSlice(
					-1, Colour(0, 0, 1) /* blue - high gradient magnitude */,
					-0.5, Colour(0, 1, 1) /* cyan - mid gradient magnitude */));
	d_inner_palette->add_entry(
			ColourSlice(
					-0.5, Colour(0, 1, 1) /* cyan - mid gradient magnitude */,
					 0, Colour(0, 1, 0) /* green - low gradient magnitude */));
	d_inner_palette->add_entry(
			ColourSlice(
					0, Colour(1, 1, 0) /* yellow - low gradient magnitude */,
					0.5, Colour(1, 0, 0) /* red - mid gradient magnitude */));
	d_inner_palette->add_entry(
			ColourSlice(
					0.5, Colour(1, 0, 0) /* red - mid gradient magnitude */,
					1, Colour(1, 0, 1) /* magenta - high gradient magnitude */));
}


GPlatesGui::DefaultScalarFieldGradientColourPalette::non_null_ptr_type
GPlatesGui::DefaultScalarFieldGradientColourPalette::create()
{
	return new DefaultScalarFieldGradientColourPalette();
}


boost::optional<GPlatesGui::Colour>
GPlatesGui::DefaultScalarFieldGradientColourPalette::get_colour(
		double value) const
{
	return d_inner_palette->get_colour(value);
}
