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


#include <QDebug>
boost::optional<GPlatesPropertyValues::Georeferencing::lat_lon_extents_type>
GPlatesPropertyValues::Georeferencing::lat_lon_extents(
		int raster_width,
		int raster_height) const
{
	if (GPlatesMaths::are_almost_exactly_equal(
				d_parameters.x_component_of_pixel_height,
				0.0) &&
		GPlatesMaths::are_almost_exactly_equal(
				d_parameters.y_component_of_pixel_width,
				0.0))
	{
		if (d_parameters.x_component_of_pixel_width > 0.0 &&
			d_parameters.y_component_of_pixel_height < 0.0)
		{
			double extents_width = d_parameters.x_component_of_pixel_width * raster_width;
			double extents_height = d_parameters.y_component_of_pixel_height * raster_height;

			double bottom = d_parameters.top_left_y_coordinate + extents_height;

			if (GPlatesMaths::Real(extents_width) <= GPlatesMaths::Real(360.0) &&
					GPlatesMaths::Real(bottom) >= GPlatesMaths::Real(-90.0))
			{
				double right = d_parameters.top_left_x_coordinate + extents_width;
				if (GPlatesMaths::Real(right) > GPlatesMaths::Real(180.0))
				{
					right -= 360.0;
				}
				lat_lon_extents_type result = {{{
					d_parameters.top_left_y_coordinate,
					bottom,
					d_parameters.top_left_x_coordinate,
					right
				}}};
				return result;
			}
			else
			{
				// Bounds out of range.
				return boost::none;
			}
		}
		else
		{
			// The raster is flipped horizontally or vertically.
			return boost::none;
		}
	}
	else
	{
		// The transform rotates or shears the raster.
		return boost::none;
	}
}


bool
GPlatesPropertyValues::Georeferencing::set_lat_lon_extents(
		lat_lon_extents_type extents,
		int raster_width,
		int raster_height)
{
	// Check whether the bottom is greater than the top.
	if (extents.bottom > extents.top)
	{
		return false;
	}

	// Check that the longitudes are in range.
	static const double MIN_LONGITUDE = -180.0;
	static const double MAX_LONGITUDE = 180.0;
	static const double LONGITUDE_RANGE = MAX_LONGITUDE - MIN_LONGITUDE;
	if (extents.left < MIN_LONGITUDE)
	{
		int steps = (MAX_LONGITUDE - extents.left) / LONGITUDE_RANGE;
		extents.left += steps * LONGITUDE_RANGE;
	}
	if (extents.right > MAX_LONGITUDE)
	{
		int steps = (extents.right - MIN_LONGITUDE) / LONGITUDE_RANGE;
		extents.right -= steps * LONGITUDE_RANGE;
	}

	// Check that the latitudes are in range.
	static const double MIN_LATITUDE = -90.0;
	static const double MAX_LATITUDE = 90.0;
	if (extents.bottom < MIN_LATITUDE)
	{
		extents.bottom = MIN_LATITUDE;
	}
	if (extents.top > MAX_LATITUDE)
	{
		extents.top = MAX_LATITUDE;
	}

	// Convert to affine transform parameters.
	double extents_width = extents.right - extents.left;
	if (extents_width < 0.0)
	{
		extents_width += LONGITUDE_RANGE;
	}

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
	return true;
}


void
GPlatesPropertyValues::Georeferencing::reset_to_global_extents(
		int raster_width,
		int raster_height)
{
	d_parameters = create_global_extents(raster_width, raster_height);
}

