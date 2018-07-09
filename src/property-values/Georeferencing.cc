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

#include "global/PreconditionViolationError.h"
#include "global/GPlatesAssert.h"

#include "maths/Real.h"
#include "maths/MathsUtils.h"


namespace
{
	// Use 'Real' instead of 'double' to allow a numerical tolerance/epsilon during comparisons.
	const GPlatesMaths::Real MIN_LATITUDE = -90.0;
	const GPlatesMaths::Real MAX_LATITUDE = 90.0;
}


const GPlatesPropertyValues::Georeferencing::lat_lon_extents_type
GPlatesPropertyValues::Georeferencing::GLOBAL_EXTENTS =
{{{
	90.0   /*top*/,
	-90.0  /*bottom*/,
	-180.0 /*left*/,
	180.0  /*right*/
}}};


GPlatesPropertyValues::Georeferencing::parameters_type
GPlatesPropertyValues::Georeferencing::convert_to_parameters(
		parameters_type parameters,
		bool convert_from_grid_line_registration)
{
	// Grid registration uses an extra row and column of pixels (data points) since data points are
	// *on* the grid lines instead of at the centre of grid cells (area between grid lines).
	// For example...
	//
	//   -----    +-+-+
	//   |+|+|    | | |
	//   -----    +-+-+
	//   |+|+|    | | |
	//   -----    +-+-+
	//
	// ...the '+' symbols are data points.
	// The left is pixel registration with 2x2 data points.
	// The right is grid line registration with 3x3 data points.
	//
	if (convert_from_grid_line_registration)
	{
		// The top-left coordinate is always that of the pixel *box* (not centre).
		// So we have to adjust since the coordinates are currently referring to the pixel *centre*.
		// We do this by substituting pixel coordinates (-0.5, -0.5) into the georeferencing equation:
		//
		//   x_geo = x * A + y * B + C
		//   y_geo = x * D + y * E + F
		//
		parameters.top_left_x_coordinate -=
				0.5 * parameters.x_component_of_pixel_width/*A*/ +
				0.5 * parameters.x_component_of_pixel_height/*B*/;
		parameters.top_left_y_coordinate -=
				0.5 * parameters.y_component_of_pixel_width/*D*/ +
				0.5 * parameters.y_component_of_pixel_height/*E*/;
	}

	return parameters;
}


GPlatesPropertyValues::Georeferencing::parameters_type
GPlatesPropertyValues::Georeferencing::convert_lat_lon_extents_to_parameters(
		const lat_lon_extents_type &lat_lon_extents,
		unsigned int raster_width,
		unsigned int raster_height,
		bool convert_from_grid_line_registration)
{
	double top_left_x_coordinate;
	double top_left_y_coordinate;
	double x_component_of_pixel_width;
	double y_component_of_pixel_height;

	// Grid registration uses an extra row and column of pixels (data points) since data points are
	// *on* the grid lines instead of at the centre of grid cells (area between grid lines).
	// For example...
	//
	//   -----    +-+-+
	//   |+|+|    | | |
	//   -----    +-+-+
	//   |+|+|    | | |
	//   -----    +-+-+
	//
	// ...the '+' symbols are data points.
	// The left is pixel registration with 2x2 data points.
	// The right is grid line registration with 3x3 data points.
	//
	if (convert_from_grid_line_registration)
	{
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				raster_width > 1 && raster_height > 1,
				GPLATES_ASSERTION_SOURCE);

		x_component_of_pixel_width = (lat_lon_extents.right - lat_lon_extents.left) / (raster_width - 1);
		y_component_of_pixel_height = (lat_lon_extents.bottom - lat_lon_extents.top) / (raster_height - 1);
		// The top-left coordinate is always that of the pixel *box* (not centre).
		// So we have to adjust since the coordinates are currently referring to the pixel *centre*.
		// We do this by substituting pixel coordinates (-0.5, -0.5) into the georeferencing equation:
		//
		//   x_geo = x * A + y * B + C
		//   y_geo = x * D + y * E + F
		//
		// Note that this can put the latitude outside the normal [-90,90] range (eg, for global lat-lon extents).
		top_left_x_coordinate = lat_lon_extents.left - 0.5 * x_component_of_pixel_width/*A*/;
		top_left_y_coordinate = lat_lon_extents.top - 0.5 * y_component_of_pixel_height/*E*/;
	}
	else
	{
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				raster_width > 0 && raster_height > 0,
				GPLATES_ASSERTION_SOURCE);

		x_component_of_pixel_width = (lat_lon_extents.right - lat_lon_extents.left) / raster_width;
		y_component_of_pixel_height = (lat_lon_extents.bottom - lat_lon_extents.top) / raster_height;
		top_left_x_coordinate = lat_lon_extents.left;
		top_left_y_coordinate = lat_lon_extents.top;
	}

	const parameters_type result = {{{
		top_left_x_coordinate,
		x_component_of_pixel_width,
		0.0,
		top_left_y_coordinate,
		0.0,
		y_component_of_pixel_height
	}}};

	return result;
}


GPlatesPropertyValues::Georeferencing::parameters_type
GPlatesPropertyValues::Georeferencing::get_parameters(
		bool convert_to_grid_line_registration) const
{
	parameters_type parameters = d_parameters;

	if (convert_to_grid_line_registration)
	{
		// The boundary of the pixel *boxes* (not pixel *centres*).
		// We store georeferencing (as does GDAL) with the boundary around the pixel *boxes* (not pixel *centres*).
		// So the boundary of the pixel *centres* need to be adjusted inward by half a pixel.
		// We do this by substituting pixel coordinates (0.5, 0.5) into the georeferencing equation:
		//
		//   x_geo = x * A + y * B + C
		//   y_geo = x * D + y * E + F
		//
		parameters.top_left_x_coordinate +=
				0.5 * parameters.x_component_of_pixel_width/*A*/ +
				0.5 * parameters.x_component_of_pixel_height/*B*/;
		parameters.top_left_y_coordinate +=
				0.5 * parameters.y_component_of_pixel_width/*D*/ +
				0.5 * parameters.y_component_of_pixel_height/*E*/;
	}

	return parameters;
}


boost::optional<GPlatesPropertyValues::Georeferencing::lat_lon_extents_type>
GPlatesPropertyValues::Georeferencing::get_lat_lon_extents(
		unsigned int raster_width,
		unsigned int raster_height,
		bool convert_to_grid_line_registration) const
{
	if (!GPlatesMaths::are_almost_exactly_equal(
				d_parameters.x_component_of_pixel_height,
				0.0) ||
		!GPlatesMaths::are_almost_exactly_equal(
				d_parameters.y_component_of_pixel_width,
				0.0))
	{
		// The transform rotates or shears the raster.
		return boost::none;
	}

	// The boundary of the pixel *boxes* (not pixel *centres*).
	// We store georeferencing this way (as does GDAL).
	// Note that this can put the top/bottom latitude outside the normal [-90,90] range
	// (eg, for global lat-lon extents) but pixel *centres* should always be within [-90,90].
	const double top = d_parameters.top_left_y_coordinate;
	const double bottom = d_parameters.top_left_y_coordinate + raster_height * d_parameters.y_component_of_pixel_height/*E*/;
	const double left = d_parameters.top_left_x_coordinate;
	const double right = d_parameters.top_left_x_coordinate + raster_width * d_parameters.x_component_of_pixel_width/*A*/;

	// The boundary of the pixel *centres* are adjusted inward by half a pixel.
	const double top_pixel_centre = top + 0.5 * d_parameters.y_component_of_pixel_height/*E*/;
	const double bottom_pixel_centre = bottom - 0.5 * d_parameters.y_component_of_pixel_height/*E*/;
	const double left_pixel_centre = left + 0.5 * d_parameters.x_component_of_pixel_width/*A*/;
	const double right_pixel_centre = right - 0.5 * d_parameters.x_component_of_pixel_width/*A*/;

	// Check that the boundary pixel *centres* are within acceptable latitude range [-90, 90].
	// Note that the pixel *boxes* of the boundary pixels can be outside though.
	//
	// NOTE: These are epsilon comparisons using 'Real'.
	if (top_pixel_centre > MAX_LATITUDE ||
		top_pixel_centre < MIN_LATITUDE ||
		bottom_pixel_centre > MAX_LATITUDE ||
		bottom_pixel_centre < MIN_LATITUDE)
	{
		return boost::none;
	}

	if (convert_to_grid_line_registration)
	{
		// The top-left coordinate for grid line registration is that of the pixel *centre* (not box).
		const lat_lon_extents_type result = {{{
			top_pixel_centre,
			bottom_pixel_centre,
			left_pixel_centre,
			right_pixel_centre
		}}};

		return result;
	}
	else
	{
		// The top-left coordinate for pixel registration is that of the pixel *box* (not centre).
		const lat_lon_extents_type result = {{{
			top,
			bottom,
			left,
			right
		}}};

		return result;
	}
}
