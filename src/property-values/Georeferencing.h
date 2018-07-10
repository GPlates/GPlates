/* $Id$ */

/**
 * \file 
 * Contains the definition of the Georeferencing class.
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

#ifndef GPLATES_PROPERTYVALUES_GEOREFERENCING_H
#define GPLATES_PROPERTYVALUES_GEOREFERENCING_H

#include <boost/intrusive_ptr.hpp>
#include <boost/optional.hpp>

#include "maths/MathsUtils.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesPropertyValues
{
	/**
	 * The Georeferencing class stores information relating to the georeferencing
	 * of raster images.
	 *
	 * Georeferencing refers to the process of mapping pixels in a raster to their
	 * corresponding positions in some coordinate system.
	 *
	 * There are, in general, two ways in which we can specify the mapping of pixel
	 * coordinates to geographic coordinates.
	 *
	 * The first method is to apply an affine transformation to the raster. This
	 * will translate, rotate and shear the raster to the right position in the
	 * coordinate system. Simple lat-lon bounding box georeferencing is a special
	 * case of this first method.
	 *
	 * The second method is to use control points, which provide the geographic
	 * coordinates for specific pixels in the raster. Pixels around the control
	 * points are mapped to geographic coordinates using some arbitrary
	 * interpolation function.
	 */
	class Georeferencing :
			public GPlatesUtils::ReferenceCount<Georeferencing>
	{
		// Note: The class, for now, only encapsulates an affine transform.

	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<Georeferencing> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const Georeferencing> non_null_ptr_to_const_type;

		/**
		 * The parameters that specify the affine transform.
		 *
		 * The parameters are, in order:
		 *  (0) Top left x coordinate [C]
		 *  (1) x component of pixel width [A]
		 *  (2) x component of pixel height [B]
		 *  (3) Top left y coordinate [F]
		 *  (4) y component of pixel width [D]
		 *  (5) y component of pixel height [E]
		 *
		 * (The terminology is borrowed from the Wikipedia article on ESRI world files.)
		 *
		 * The order of the components is the order in which GDAL gives us the affine
		 * transform parameters.
		 *
		 * For a given (x, y) pixel coordinate, the corresponding geographic coordinate is:
		 *
		 *	x_geo = x * A + y * B + C
		 *	y_geo = x * D + y * E + F
		 *
		 * ESRI world files provide the parameters in the following order:
		 *  - Line 1: A
		 *  - Line 2: D
		 *  - Line 3: B
		 *  - Line 4: E
		 *  - Line 5: C'
		 *  - Line 6: F'
		 *
		 * Note that lines 5 and 6 provide the coordinates of the *centre* of the
		 * top-left pixel of the raster, whereas GDAL uses the top-left corner of the
		 * top-left pixel of the raster. The following conversions can be used:
		 *
		 *	C = C' - A/2 + B/2  // *update*: or should it be "C' - (A + B)/2" ?
		 *	F = F' - (D + E)/2
		 */
		struct parameters_type
		{
			static const unsigned int NUM_COMPONENTS = 6;
			union
			{
				double components[NUM_COMPONENTS];
				struct
				{
					double top_left_x_coordinate;
					double x_component_of_pixel_width;
					double x_component_of_pixel_height;
					double top_left_y_coordinate;
					double y_component_of_pixel_width;
					double y_component_of_pixel_height;
				};
			};
		};

		/**
		 * A convenience structure for conversions to and from the affine transform
		 * and lat-lon extents.
		 *
		 * We constrain the latitude (top/bottom) extents such that the top/bottom pixel *centres*
		 * are in the range [-90, +90]:
		 *  - If 'top' is strictly greater than 'bottom' (the usual case), the
		 *    first line of the raster file is drawn to the north of the last line.
		 *  - If 'top' is equal to 'bottom', the raster is drawn with height zero.
		 *  - If 'top' is strictly less than 'bottom', the first line of the
		 *    raster file is drawn to the south of the last line; that is, the
		 *    raster is drawn flipped vertically.
		 *
		 * There are no constraints on the longitude (left/right) extents:
		 *  - If 'left' is strictly less than 'right' (the usual case), the
		 *    columns of the raster are drawn from west to east.
		 *  - If 'left' is equal to 'right', the raster is drawn with width zero.
		 *  - If 'left' is strictly greater than 'right', the columns of the
		 *    raster are drawn from east to west; that is, the raster is drawn
		 *    flipped horizontally.
		 */
		struct lat_lon_extents_type
		{
			static const unsigned int NUM_COMPONENTS = 4;
			union
			{
				double components[NUM_COMPONENTS];
				struct
				{
					double top; // Max lat
					double bottom; // Min lat
					double left; // Min lon
					double right; // Max lon
				};
			};
		};

		/**
		 * Default constructor.
		 *
		 * Sets all fields in the affine transform to zero.
		 */
		static
		non_null_ptr_type
		create()
		{
			static const parameters_type ZEROES = {{{ 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 }}};
			return new Georeferencing(ZEROES);
		}

		/**
		 * Creates an affine transform that maps a raster to the entire globe.
		 *
		 * Grid registration places data points *on* the grid lines instead of at the centre of
		 * grid cells (area between grid lines). For example...
		 *
		 *   +--+--+  -------
		 *   |  |  |  |+|+|+|
		 *   |  |  |  -------
		 *   +--+--+  |+|+|+|
		 *   |  |  |  -------
		 *   |  |  |  |+|+|+|
		 *   +--+--+  -------
		 *
		 * ...the '+' symbols are data points.
		 * On the left is grid line registration.
		 * On the right is pixel registration.
		 * Both registrations have 3x3 data points.
		 *
		 * If @a convert_from_grid_line_registration is true then the global extents ([-90,90] and [-180,180])
		 * are assumed to bound the pixel *centres* (not *boxes*) - in other words the border pixels have their
		 * *centres* lying at the min/max extents ([-90,90] and [-180,180]).
		 * Otherwise the global extents ([-90,90] and [-180,180]) are assumed to bound the border pixel *boxes*.
		 *
		 * @throws PreconditionViolationError if either:
		 *         1) @a raster_width or @a raster_height are zero (when @a convert_from_grid_line_registration is false), or
		 *         2) @a raster_width or @a raster_height are less than 2 (when @a convert_from_grid_line_registration is true).
		 */
		static
		non_null_ptr_type
		create(
				unsigned int raster_width,
				unsigned int raster_height,
				bool convert_from_grid_line_registration = false)
		{
			return new Georeferencing(
					convert_to_pixel_registration(
							GLOBAL_LAT_LON_EXTENTS,
							raster_width,
							raster_height,
							convert_from_grid_line_registration));
		}

		/**
		 * Creates an affine transform that maps a raster to the specified lat-lon extents.
		 *
		 * Grid registration places data points *on* the grid lines instead of at the centre of
		 * grid cells (area between grid lines). For example...
		 *
		 *   +--+--+  -------
		 *   |  |  |  |+|+|+|
		 *   |  |  |  -------
		 *   +--+--+  |+|+|+|
		 *   |  |  |  -------
		 *   |  |  |  |+|+|+|
		 *   +--+--+  -------
		 *
		 * ...the '+' symbols are data points.
		 * On the left is grid line registration.
		 * On the right is pixel registration.
		 * Both registrations have 3x3 data points.
		 *
		 * If @a convert_from_grid_line_registration is true then @a lat_lon_extents are assumed to bound the
		 * pixel *centres* (not *boxes*) - in other words the border pixels have their *centres* lying
		 * at the min/max lat_lon_extents. Otherwise the @a lat_lon_extents are assumed to bound the border pixel *boxes*.
		 *
		 * NOTE: It is the responsibility of the caller to ensure that @a lat_lon_extents are specified so that
		 * the end result is such that the pixel *centres* have latitudes in the range [-90, 90]. This means
		 * when @a convert_from_grid_line_registration is false it is allowed for latitude extents to be
		 * outside [-90, 90] since the extents refer to pixel *boxes* (as long as *centres* lie in [-90, 90]).
		 * Note that if this is violated then a subsequent call to @a get_lat_lon_extents will return none.
		 *
		 * @throws PreconditionViolationError if either:
		 *         1) @a raster_width or @a raster_height are zero (when @a convert_from_grid_line_registration is false), or
		 *         2) @a raster_width or @a raster_height are less than 2 (when @a convert_from_grid_line_registration is true).
		 */
		static
		non_null_ptr_type
		create(
				const lat_lon_extents_type &lat_lon_extents,
				unsigned int raster_width,
				unsigned int raster_height,
				bool convert_from_grid_line_registration = false)
		{
			return new Georeferencing(
					convert_to_pixel_registration(
							lat_lon_extents,
							raster_width,
							raster_height,
							convert_from_grid_line_registration));
		}

		/**
		 * Creates an affine transform with the specified @a parameters.
		 *
		 * Grid registration places data points *on* the grid lines instead of at the centre of
		 * grid cells (area between grid lines). For example...
		 *
		 *   +--+--+  -------
		 *   |  |  |  |+|+|+|
		 *   |  |  |  -------
		 *   +--+--+  |+|+|+|
		 *   |  |  |  -------
		 *   |  |  |  |+|+|+|
		 *   +--+--+  -------
		 *
		 * ...the '+' symbols are data points.
		 * On the left is grid line registration.
		 * On the right is pixel registration.
		 * Both registrations have 3x3 data points.
		 *
		 * If @a convert_from_grid_line_registration is true then @a parameters are assumed to bound the
		 * pixel *centres* (not *boxes*) - in other words the border pixels have their *centres* lying
		 * at the min/max extents. Otherwise the @a parameters are assumed to bound the border pixel *boxes*.
		 */
		static
		non_null_ptr_type
		create(
				const parameters_type &parameters,
				bool convert_from_grid_line_registration = false)
		{
			return new Georeferencing(
					convert_to_pixel_registration(
							parameters,
							convert_from_grid_line_registration));
		}

		/**
		 * Retrieves the affine transform parameters.
		 *
		 * If @a convert_to_grid_line_registration is true then the returned parameters will
		 * bound the pixel *centres*, otherwise will bound the pixel *boxes*.
		 * Note that this class always stores georeferencing that bounds pixel *boxes*.
		 */
		parameters_type
		get_parameters(
				bool convert_to_grid_line_registration = false) const;

		/**
		 * Sets the affine transform parameters.
		 *
		 * Grid registration places data points *on* the grid lines instead of at the centre of
		 * grid cells (area between grid lines). For example...
		 *
		 *   +--+--+  -------
		 *   |  |  |  |+|+|+|
		 *   |  |  |  -------
		 *   +--+--+  |+|+|+|
		 *   |  |  |  -------
		 *   |  |  |  |+|+|+|
		 *   +--+--+  -------
		 *
		 * ...the '+' symbols are data points.
		 * On the left is grid line registration.
		 * On the right is pixel registration.
		 * Both registrations have 3x3 data points.
		 *
		 * If @a convert_from_grid_line_registration is true then @a parameters are assumed to bound the
		 * pixel *centres* (not *boxes*) - in other words the border pixels have their *centres* lying
		 * at the min/max extents. Otherwise the @a parameters are assumed to bound the border pixel *boxes*.
		 */
		void
		set_parameters(
				const parameters_type &parameters,
				bool convert_from_grid_line_registration = false)
		{
			d_parameters = convert_to_pixel_registration(
					parameters,
					convert_from_grid_line_registration);
		}

		/**
		 * Retrieves the affine transform parameters as lat-lon extents.
		 *
		 * It is not possible to convert the parameters to lat-lon extents where:
		 *  - The transform rotates or shears the raster, or
		 *  - The pixel *centres* of the 'top' or 'bottom' row of pixels lie outside
		 *    the range [-90, +90] within a very small numerical tolerance.
		 *    Pixel *centres* of used (instead of pixel *boxes*) because, for example, grid line
		 *    registered rasters with global extents have pixel centres at -90 and 90.
		 *
		 * If @a convert_to_grid_line_registration is true then the returned lat-lon extents will
		 * bound the pixel *centres*, otherwise will bound the pixel *boxes*.
		 * Note that this class always stores georeferencing that bounds pixel *boxes*.
		 *
		 * Where it is not possible to produce lat-lon extents, boost::none is returned.
		 */
		boost::optional<lat_lon_extents_type>
		get_lat_lon_extents(
				unsigned int raster_width,
				unsigned int raster_height,
				bool convert_to_grid_line_registration = false) const;

		/**
		 * Sets the affine transform parameters using lat-lon extents.
		 *
		 * Grid registration places data points *on* the grid lines instead of at the centre of
		 * grid cells (area between grid lines). For example...
		 *
		 *   +--+--+  -------
		 *   |  |  |  |+|+|+|
		 *   |  |  |  -------
		 *   +--+--+  |+|+|+|
		 *   |  |  |  -------
		 *   |  |  |  |+|+|+|
		 *   +--+--+  -------
		 *
		 * ...the '+' symbols are data points.
		 * On the left is grid line registration.
		 * On the right is pixel registration.
		 * Both registrations have 3x3 data points.
		 *
		 * If @a convert_from_grid_line_registration is true then @a extents are assumed to bound the
		 * pixel *centres* (not *boxes*) - in other words the border pixels have their *centres* lying
		 * at the min/max extents. Otherwise the @a extents are assumed to bound the border pixel *boxes*.
		 *
		 * NOTE: It is the responsibility of the caller to ensure that @a extents are specified so that
		 * the end result is such that the pixel *centres* have latitudes in the range [-90, 90]. This means
		 * when @a convert_from_grid_line_registration is false it is allowed for latitude extents to be
		 * outside [-90, 90] since the extents refer to pixel *boxes* (as long as *centres* lie in [-90, 90]).
		 * Note that if this is violated then a subsequent call to @a get_lat_lon_extents will return none.
		 *
		 * @throws PreconditionViolationError if either:
		 *         1) @a raster_width or @a raster_height are zero (when @a convert_from_grid_line_registration is false), or
		 *         2) @a raster_width or @a raster_height are less than 2 (when @a convert_from_grid_line_registration is true).
		 */
		void
		set_lat_lon_extents(
				const lat_lon_extents_type &lat_lon_extents,
				unsigned int raster_width,
				unsigned int raster_height,
				bool convert_from_grid_line_registration = false)
		{
			d_parameters = convert_to_pixel_registration(
					lat_lon_extents,
					raster_width,
					raster_height,
					convert_from_grid_line_registration);
		}

		/**
		 * Resets the affine transform so that the raster covers the entire globe.
		 *
		 * Grid registration places data points *on* the grid lines instead of at the centre of
		 * grid cells (area between grid lines). For example...
		 *
		 *   +--+--+  -------
		 *   |  |  |  |+|+|+|
		 *   |  |  |  -------
		 *   +--+--+  |+|+|+|
		 *   |  |  |  -------
		 *   |  |  |  |+|+|+|
		 *   +--+--+  -------
		 *
		 * ...the '+' symbols are data points.
		 * On the left is grid line registration.
		 * On the right is pixel registration.
		 * Both registrations have 3x3 data points.
		 *
		 * If @a convert_from_grid_line_registration is true then the global extents ([-90,90] and [-180,180])
		 * are assumed to bound the pixel *centres* (not *boxes*) - in other words the border pixels have their
		 * *centres* lying at the min/max extents ([-90,90] and [-180,180]).
		 * Otherwise the global extents ([-90,90] and [-180,180]) are assumed to bound the border pixel *boxes*.
		 *
		 * @throws PreconditionViolationError if either:
		 *         1) @a raster_width or @a raster_height are zero (when @a convert_from_grid_line_registration is false), or
		 *         2) @a raster_width or @a raster_height are less than 2 (when @a convert_from_grid_line_registration is true).
		 */
		void
		reset_to_global_extents(
				unsigned int raster_width,
				unsigned int raster_height,
				bool convert_from_grid_line_registration = false)
		{
			d_parameters = convert_to_pixel_registration(
					GLOBAL_LAT_LON_EXTENTS,
					raster_width,
					raster_height,
					convert_from_grid_line_registration);
		}

	private:

		//! Global lat-lon extents (latitude range [-90, 90] and longitude range [-180, 180]).
		static const lat_lon_extents_type GLOBAL_LAT_LON_EXTENTS;

		//! Convert parameters to pixel registration (if @a convert_from_grid_line_registration is true),
		//! otherwise simply returns parameters.
		static
		parameters_type
		convert_to_pixel_registration(
				parameters_type parameters,
				bool convert_from_grid_line_registration);

		/**
		 * Convert lat-lon extents (as pixel or grid line registration) to pixel registration parameters.
		 */
		static
		parameters_type
		convert_to_pixel_registration(
				const lat_lon_extents_type &lat_lon_extents,
				unsigned int raster_width,
				unsigned int raster_height,
				bool convert_from_grid_line_registration);


		explicit
		Georeferencing(
				const parameters_type &parameters) :
			d_parameters(parameters)
		{  }

		Georeferencing(
				const Georeferencing &other) :
			GPlatesUtils::ReferenceCount<Georeferencing>(),
			d_parameters(other.d_parameters)
		{  }

		parameters_type d_parameters;
	};
}

#endif  // GPLATES_PROPERTYVALUES_GEOREFERENCING_H
