/* $Id$ */

/**
 * \file 
 * Contains the implementation of the Georeferencing class.
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

#include "Georeferencing.h"

#include "maths/Real.h"
#include "maths/MathsUtils.h"


namespace
{
	const double MIN_LATITUDE = -90.0;
	const double MAX_LATITUDE = 90.0;
	const double LATITUDE_EPISLON = 0.9; // Very generous.
}


boost::optional<GPlatesPropertyValues::Georeferencing::lat_lon_extents_type>
GPlatesPropertyValues::Georeferencing::lat_lon_extents(
		unsigned int raster_width,
		unsigned int raster_height) const
{
	if (GPlatesMaths::are_almost_exactly_equal(
				d_parameters.x_component_of_pixel_height,
				0.0) &&
		GPlatesMaths::are_almost_exactly_equal(
				d_parameters.y_component_of_pixel_width,
				0.0))
	{
		double extents_width = d_parameters.x_component_of_pixel_width * raster_width;
		double extents_height = d_parameters.y_component_of_pixel_height * raster_height;

		double top = d_parameters.top_left_y_coordinate;
		double bottom = d_parameters.top_left_y_coordinate + extents_height;
		double left = d_parameters.top_left_x_coordinate;
		double right = d_parameters.top_left_x_coordinate + extents_width;

		// Clamp top and bottom to [-90, +90] if within epsilon.
		if (top > MAX_LATITUDE)
		{
			if (top > MAX_LATITUDE + LATITUDE_EPISLON)
			{
				return boost::none;
			}
			top = MAX_LATITUDE;
		}
		if (bottom < MIN_LATITUDE)
		{
			if (bottom < MIN_LATITUDE - LATITUDE_EPISLON)
			{
				return boost::none;
			}
			bottom = MIN_LATITUDE;
		}

		lat_lon_extents_type result = {{{
			top,
			bottom,
			left,
			right
		}}};

		return result;
	}
	else
	{
		// The transform rotates or shears the raster.
		return boost::none;
	}
}


GPlatesPropertyValues::Georeferencing::SetLatLonExtentsError
GPlatesPropertyValues::Georeferencing::set_lat_lon_extents(
		lat_lon_extents_type extents,
		unsigned int raster_width,
		unsigned int raster_height)
{
	// Check that top != bottom and left != right.
	if (GPlatesMaths::are_almost_exactly_equal(extents.top, extents.bottom))
	{
		return ZERO_HEIGHT;
	}
	if (GPlatesMaths::are_almost_exactly_equal(extents.left, extents.right))
	{
		return ZERO_WIDTH;
	}

	// Clamp latitude extents if they are out of range but within epsilon.
	if (extents.bottom < MIN_LATITUDE)
	{
		if (extents.bottom < MIN_LATITUDE - LATITUDE_EPISLON)
		{
			return BOTTOM_OUT_OF_RANGE;
		}
		extents.bottom = MIN_LATITUDE;
	}
	if (extents.top > MAX_LATITUDE)
	{
		if (extents.top > MAX_LATITUDE + LATITUDE_EPISLON)
		{
			return TOP_OUT_OF_RANGE;
		}
		extents.top = MAX_LATITUDE;
	}

	// Convert to affine transform parameters.
	double extents_width = extents.right - extents.left;
	double extents_height = extents.bottom - extents.top;
	parameters_type converted = {{{
		extents.left,
		extents_width / raster_width,
		0.0,
		extents.top,
		0.0,
		extents_height / raster_height
	}}};

	d_parameters = converted;

	return NONE;
}


void
GPlatesPropertyValues::Georeferencing::reset_to_global_extents(
		unsigned int raster_width,
		unsigned int raster_height)
{
	d_parameters = create_global_extents(raster_width, raster_height);
}

