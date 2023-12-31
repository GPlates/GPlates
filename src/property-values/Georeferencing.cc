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

#include "maths/MathsUtils.h"


namespace
{
	const double MIN_LATITUDE = -90.0;
	const double MAX_LATITUDE = 90.0;
	// Enough to account for transformations back and forth between grid line registration.
	const double LATITUDE_EPISLON = 1e-4; // This is quite generous.
}


const GPlatesPropertyValues::Georeferencing::lat_lon_extents_type
GPlatesPropertyValues::Georeferencing::GLOBAL_LAT_LON_EXTENTS =
{{{
	90.0   /*top*/,
	-90.0  /*bottom*/,
	-180.0 /*left*/,
	180.0  /*right*/
}}};


GPlatesPropertyValues::Georeferencing::parameters_type
GPlatesPropertyValues::Georeferencing::convert_to_pixel_registration(
		parameters_type parameters,
		bool convert_from_grid_line_registration)
{
	if (convert_from_grid_line_registration)
	{
		//
		// Grid registration places data points *on* the grid lines instead of at the centre of
		// grid cells (area between grid lines). For example...
		//
		//                -------------
		//   +---+---+    | + | + | + |
		//   |   |   |    -------------
		//   +---+---+    | + | + | + |
		//   |   |   |    -------------
		//   +---+---+    | + | + | + |
		//                -------------
		//
		// ...the '+' symbols are data points.
		// On the left is the grid line registration we are converting from.
		// On the right is the pixel registration we are converting to.
		// Both registrations have 3x3 data points.
		//

		// The top-left coordinate stored in this class is always that of the pixel *box* (not centre).
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

		//
		// Note that we don't need to adjust the other parameters (components of pixel width and height)
		// since the spacing between pixels does not change during the conversion.
		//
		// This can also be shown mathematically by equating the geographic coordinates at the
		// centre of the top-left pixel (in pixel and grid line registrations), and likewise for
		// the bottom-right pixels. Top-left and bottom-right pixel coordinates for grid line registration
		// are (0,0) and (Nx-1,Ny-1) respectively, where Nx and Ny are raster width and height in number of pixels.
		// And top-left and bottom-right pixel coordinates for pixel registration are (0.5,0.5) and (Nx-0.5,Ny-0.5)...
		//
		//   x_top_left_centre = 0.5 * A_p + 0.5 * B_p + C_p
		//   y_top_left_centre = 0.5 * D_p + 0.5 * E_p + F_p
		//
		//   x_top_left_centre = 0.0 * A_g + 0.0 * B_g + C_g = C_g
		//   y_top_left_centre = 0.0 * D_g + 0.0 * E_g + F_g = F_g
		//
		//   x_bottom_right_centre = (Nx - 0.5) * A_p + (Ny - 0.5) * B_p + C_p
		//   y_bottom_right_centre = (Nx - 0.5) * D_p + (Ny - 0.5) * E_p + F_p
		//
		//   x_bottom_right_centre = (Nx - 1.0) * A_g + (Ny - 1.0) * B_g + C_g
		//   y_bottom_right_centre = (Nx - 1.0) * D_g + (Ny - 1.0) * E_g + F_g
		//
		// ...where '_p' refers to pixel registration and '_g' refers to grid line registration.
		// Equating the top-left pixel centre coordinates results in:
		// 
		//   C_p = C_g - 0.5 * A_p - 0.5 * B_p
		//   F_p = F_g - 0.5 * D_p - 0.5 * E_p
		//
		// ...which substituted into the equations for bottom-right pixel centre become:
		// 
		//   (Nx - 0.5) * A_p + (Ny - 0.5) * B_p + C_g - 0.5 * A_p - 0.5 * B_p = (Nx - 1.0) * A_g + (Ny - 1.0) * B_g + C_g
		//   (Nx - 0.5) * D_p + (Ny - 0.5) * E_p + F_g - 0.5 * D_p - 0.5 * E_p = (Nx - 1.0) * D_g + (Ny - 1.0) * E_g + F_g
		// 
		// ...which simplifies to:
		// 
		//   (Nx - 1.0) * A_p + (Ny - 1.0) * B_p = (Nx - 1.0) * A_g + (Ny - 1.0) * B_g
		//   (Nx - 1.0) * D_p + (Ny - 1.0) * E_p = (Nx - 1.0) * D_g + (Ny - 1.0) * E_g
		// 
		// ...which results in:
		// 
		//   A_p = A_g
		//   B_p = B_g
		//   D_p = D_g
		//   E_p = E_g
		//
	}
	//
	// Else the input data is already in pixel registration...
	//
	//   -------------
	//   | + | + | + |
	//   -------------
	//   | + | + | + |
	//   -------------
	//   | + | + | + |
	//   -------------
	//
	// ...the '+' symbols are data points.
	//

	return parameters;
}


GPlatesPropertyValues::Georeferencing::parameters_type
GPlatesPropertyValues::Georeferencing::convert_to_pixel_registration(
		const lat_lon_extents_type &lat_lon_extents,
		unsigned int raster_width,
		unsigned int raster_height,
		bool convert_from_grid_line_registration)
{
	double top_left_x_coordinate;
	double top_left_y_coordinate;
	double x_component_of_pixel_width;
	double y_component_of_pixel_height;

	if (convert_from_grid_line_registration)
	{
		//
		// Grid registration places data points *on* the grid lines instead of at the centre of
		// grid cells (area between grid lines). For example...
		//
		//                -------------
		//   +---+---+    | + | + | + |
		//   |   |   |    -------------
		//   +---+---+    | + | + | + |
		//   |   |   |    -------------
		//   +---+---+    | + | + | + |
		//                -------------
		//
		// ...the '+' symbols are data points.
		// On the left is the grid line registration we are converting from.
		// On the right is the pixel registration we are converting to.
		// Both registrations have 3x3 data points.
		//

		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				raster_width > 1 && raster_height > 1,
				GPLATES_ASSERTION_SOURCE);

		// We divide by raster width minus one (and raster height minus one) since this is the spacing
		// in pixels between the pixel *centres* of the left and right extents (and top and bottom).
		x_component_of_pixel_width = (lat_lon_extents.right - lat_lon_extents.left) / (raster_width - 1);
		y_component_of_pixel_height = (lat_lon_extents.bottom - lat_lon_extents.top) / (raster_height - 1);

		// The top-left coordinate stored in this class is always that of the pixel *box* (not centre).
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
		//
		// Input data is already in pixel registration...
		//
		//   -------------
		//   | + | + | + |
		//   -------------
		//   | + | + | + |
		//   -------------
		//   | + | + | + |
		//   -------------
		//
		// ...the '+' symbols are data points.
		//

		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				raster_width > 0 && raster_height > 0,
				GPLATES_ASSERTION_SOURCE);

		// We divide by raster width (and raster height) since this is the spacing in pixels across
		// the pixel *boxes* from left to right (and top to bottom).
		x_component_of_pixel_width = (lat_lon_extents.right - lat_lon_extents.left) / raster_width;
		y_component_of_pixel_height = (lat_lon_extents.bottom - lat_lon_extents.top) / raster_height;

		// The top-left coordinate stored in this class is always that of the pixel *box* (not centre).
		// The coordinates are already referring to the pixel *box* so we don't need to make any adjustments.
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
		//
		// Grid registration places data points *on* the grid lines instead of at the centre of
		// grid cells (area between grid lines). For example...
		//
		//   -------------             
		//   | + | + | + |    +---+---+
		//   -------------    |   |   |
		//   | + | + | + |    +---+---+
		//   -------------    |   |   |
		//   | + | + | + |    +---+---+
		//   -------------             
		//
		// ...the '+' symbols are data points.
		// On the left is the pixel registration we are converting from.
		// On the right is the grid line registration we are converting to.
		// Both registrations have 3x3 data points.
		//

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

		//
		// Note that we don't need to adjust the other parameters (components of pixel width and height)
		// since the spacing between pixels does not change during the conversion.
		//
		// See 'convert_to_pixel_registration()' for a mathematical proof.
		//
	}
	//
	// Else the input data is already in pixel registration...
	//
	//   -------------
	//   | + | + | + |
	//   -------------
	//   | + | + | + |
	//   -------------
	//   | + | + | + |
	//   -------------
	//
	// ...the '+' symbols are data points.
	//

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
	// NOTE: We use epsilon comparisons to account for transformations back and forth between grid line registration.
	if (top_pixel_centre > MAX_LATITUDE + LATITUDE_EPISLON ||
		top_pixel_centre < MIN_LATITUDE - LATITUDE_EPISLON ||
		bottom_pixel_centre > MAX_LATITUDE + LATITUDE_EPISLON ||
		bottom_pixel_centre < MIN_LATITUDE - LATITUDE_EPISLON)
	{
		return boost::none;
	}

	if (convert_to_grid_line_registration)
	{
		//
		// Grid registration places data points *on* the grid lines instead of at the centre of
		// grid cells (area between grid lines). For example...
		//
		//   -------------             
		//   | + | + | + |    +---+---+
		//   -------------    |   |   |
		//   | + | + | + |    +---+---+
		//   -------------    |   |   |
		//   | + | + | + |    +---+---+
		//   -------------             
		//
		// ...the '+' symbols are data points.
		// On the left is the pixel registration we are converting from.
		// On the right is the grid line registration we are converting to.
		// Both registrations have 3x3 data points.
		//

		// The top-left and bottom-right coordinates for grid line registration are that of pixel *centres* (not boxes).
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
		//
		// Input data is already in pixel registration...
		//
		//   -------------
		//   | + | + | + |
		//   -------------
		//   | + | + | + |
		//   -------------
		//   | + | + | + |
		//   -------------
		//
		// ...the '+' symbols are data points.
		//

		// The top-left and bottom-right coordinates for pixel registration are that of pixel *boxes* (not centres).
		const lat_lon_extents_type result = {{{
			top,
			bottom,
			left,
			right
		}}};

		return result;
	}
}


void
GPlatesPropertyValues::Georeferencing::contract_grid_line_to_pixel_registration(
		unsigned int raster_width,
		unsigned int raster_height)
{
	//
	// Grid registration places data points *on* the grid lines instead of at the centre of
	// grid cells (area between grid lines). For example...
	//
	//   +--+--+  -------
	//   |  |  |  |+|+|+|
	//   |  |  |  -------
	//   +--+--+  |+|+|+|
	//   |  |  |  -------
	//   |  |  |  |+|+|+|
	//   +--+--+  -------
	//
	// ...the '+' symbols are data points.
	// On the left is grid line registration we are converting from.
	// On the right is pixel registration we are converting to.
	// Both registrations have 3x3 data points.
	//
	// NOTE: This conversion differs from the usual conversions to/from the native pixel registration
	// used internally inside this class in that this conversion contracts the pixels (data node locations)
	// which are the '+' symbols in the above diagrams.
	//

	//
	// The conversion equation can be derived mathematically by equating the geographic coordinates
	// at the centre of the top-left pixel (in grid line registration) with the top-left corner of the
	// top-left pixel (in pixel registration), and likewise for the bottom-right pixels.
	// Top-left and bottom-right coordinates for grid line registration are (0.5,0.5) and
	// (Nx-0.5,Ny-0.5) respectively, where Nx and Ny are raster width and height in number of pixels.
	// And top-left and bottom-right coordinates for pixel registration are (0.5,0.5) and (Nx,Ny)...
	//
	//   x_top_left = 0.0 * A_p + 0.0 * B_p + C_p = C_p
	//   y_top_left = 0.0 * D_p + 0.0 * E_p + F_p = F_p
	//
	//   x_top_left = 0.5 * A_g + 0.5 * B_g + C_g
	//   y_top_left = 0.5 * D_g + 0.5 * E_g + F_g
	//
	//   x_bottom_right = Nx * A_p + Ny * B_p + C_p
	//   y_bottom_right = Nx * D_p + Ny * E_p + F_p
	//
	//   x_bottom_right = (Nx - 0.5) * A_g + (Ny - 0.5) * B_g + C_g
	//   y_bottom_right = (Nx - 0.5) * D_g + (Ny - 0.5) * E_g + F_g
	//
	// ...where '_p' refers to pixel registration and '_g' refers to grid line registration.
	// Equating the top-left pixel coordinates results in:
	// 
	//   C_p = C_g + 0.5 * A_g + 0.5 * B_g
	//   F_p = F_g + 0.5 * D_g + 0.5 * E_g
	//
	// ...which substituted into the equations for bottom-right pixel coordinates become:
	// 
	//   Nx * A_p + Ny * B_p + C_g + 0.5 * A_g + 0.5 * B_g = (Nx - 0.5) * A_g + (Ny - 0.5) * B_g + C_g
	//   Nx * D_p + Ny * E_p + F_g + 0.5 * D_g + 0.5 * E_g = (Nx - 0.5) * D_g + (Ny - 0.5) * E_g + F_g
	// 
	// ...which simplifies to:
	// 
	//   Nx * A_p + Ny * B_p = (Nx - 1.0) * A_g + (Ny - 1.0) * B_g
	//   Nx * D_p + Ny * E_p = (Nx - 1.0) * D_g + (Ny - 1.0) * E_g
	// 
	// ...which results in:
	// 
	//   A_p = A_g * ((Nx - 1) / Nx)
	//   B_p = B_g * ((Ny - 1) / Ny)
	//   D_p = D_g * ((Nx - 1) / Nx)
	//   E_p = E_g * ((Ny - 1) / Ny)
	//

	//
	// The final conversion equations are:
	//
	//   C_p = C_g + 0.5 * A_g + 0.5 * B_g
	//   F_p = F_g + 0.5 * D_g + 0.5 * E_g
	//   A_p = A_g * ((Nx - 1) / Nx)
	//   B_p = B_g * ((Ny - 1) / Ny)
	//   D_p = D_g * ((Nx - 1) / Nx)
	//   E_p = E_g * ((Ny - 1) / Ny)
	//

	// Copy current parameters.
	parameters_type new_parameters = d_parameters;

	new_parameters.top_left_x_coordinate = d_parameters.top_left_x_coordinate +
			0.5 * d_parameters.x_component_of_pixel_width + 0.5 * d_parameters.x_component_of_pixel_height;
	new_parameters.top_left_y_coordinate = d_parameters.top_left_y_coordinate +
			0.5 * d_parameters.y_component_of_pixel_width + 0.5 * d_parameters.y_component_of_pixel_height;

	new_parameters.x_component_of_pixel_width = d_parameters.x_component_of_pixel_width *
			(raster_width - 1.0) / raster_width;
	new_parameters.x_component_of_pixel_height = d_parameters.x_component_of_pixel_height *
			(raster_height - 1.0) / raster_height;

	new_parameters.y_component_of_pixel_width = d_parameters.y_component_of_pixel_width *
			(raster_width - 1.0) / raster_width;
	new_parameters.y_component_of_pixel_height = d_parameters.y_component_of_pixel_height *
			(raster_height - 1.0) / raster_height;

	// Overwrite current parameters with new parameters.
	d_parameters = new_parameters;
}


void
GPlatesPropertyValues::Georeferencing::expand_pixel_to_grid_line_registration(
		unsigned int raster_width,
		unsigned int raster_height)
{
	//
	// Grid registration places data points *on* the grid lines instead of at the centre of
	// grid cells (area between grid lines). For example...
	//
	//   -------  +--+--+
	//   |+|+|+|  |  |  |
	//   -------  |  |  |
	//   |+|+|+|  +--+--+
	//   -------  |  |  |
	//   |+|+|+|  |  |  |
	//   -------  +--+--+
	//
	// ...the '+' symbols are data points.
	// On the left is pixel registration we are converting from.
	// On the right is grid line registration we are converting to.
	// Both registrations have 3x3 data points.
	//
	// NOTE: This conversion differs from the usual conversions to/from the native pixel registration
	// used internally inside this class in that this conversion expands the pixels (data node locations)
	// which are the '+' symbols in the above diagrams.
	//

	//
	// The conversion equation is the inverse of the equation using in 'contract_grid_line_to_pixel_registration()'...
	// 
	//   C_p = C_g + 0.5 * A_g + 0.5 * B_g
	//   F_p = F_g + 0.5 * D_g + 0.5 * E_g
	//   A_p = A_g * ((Nx - 1) / Nx)
	//   B_p = B_g * ((Ny - 1) / Ny)
	//   D_p = D_g * ((Nx - 1) / Nx)
	//   E_p = E_g * ((Ny - 1) / Ny)
	//
	// ...where the inverse is...
	// 
	//   A_g = A_p * (Nx / (Nx - 1))
	//   B_g = B_p * (Ny / (Ny - 1))
	//   D_g = D_p * (Nx / (Nx - 1))
	//   E_g = E_p * (Ny / (Ny - 1))
	//   C_g = C_p - 0.5 * A_g - 0.5 * B_g
	//   F_g = F_p - 0.5 * D_g - 0.5 * E_g
	//
	// ...noting that C_g and F_g use the '_g' values of A, B, D and E which need to be
	// calculated first.
	//

	// Copy current parameters.
	parameters_type new_parameters = d_parameters;

	new_parameters.x_component_of_pixel_width = d_parameters.x_component_of_pixel_width *
			raster_width / (raster_width - 1.0);
	new_parameters.x_component_of_pixel_height = d_parameters.x_component_of_pixel_height *
			raster_height / (raster_height - 1.0);

	new_parameters.y_component_of_pixel_width = d_parameters.y_component_of_pixel_width *
			raster_width / (raster_width - 1.0);
	new_parameters.y_component_of_pixel_height = d_parameters.y_component_of_pixel_height *
			raster_height / (raster_height - 1.0);

	//
	// NOTE: C_g and F_g use the '_g' values of A, B, D and E (that we calculated above) and
	//       hence use 'new_parameters' below instead of 'd_parameters' below for A, B, D and E.
	//
	new_parameters.top_left_x_coordinate = d_parameters.top_left_x_coordinate -
			0.5 * new_parameters.x_component_of_pixel_width - 0.5 * new_parameters.x_component_of_pixel_height;
	new_parameters.top_left_y_coordinate = d_parameters.top_left_y_coordinate -
			0.5 * new_parameters.y_component_of_pixel_width - 0.5 * new_parameters.y_component_of_pixel_height;

	// Overwrite current parameters with new parameters.
	d_parameters = new_parameters;
}
