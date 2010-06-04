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
	 *
	 * FIXME: Turn this into a PropertyValue derivation!
	 */
	class Georeferencing :
			public GPlatesUtils::ReferenceCount<Georeferencing>
	{
		// Note: The class, for now, only encapsulates an affine transform.

	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<Georeferencing> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const Georeferencing> non_null_ptr_to_const_type;

		typedef boost::intrusive_ptr<Georeferencing> maybe_null_ptr_type;
		typedef boost::intrusive_ptr<const Georeferencing> maybe_null_ptr_to_const_type;

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
		 *	C = C' - A/2 + B/2
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
		 * We constrain the latitude extents to be in the range [-90, +90], and the
		 * top latitude must be greater than or equal to the bottom latitude.
		 *
		 * We constrain the longitude extents to be in the range [-180, +180], even
		 * if the raster crosses the International Date Line (IDL). If the right
		 * longitude is strictly greater than the left longitude, then the raster does
		 * not cross the IDL. If the right longitude is strictly less than the left
		 * longitude, the raster crosses the IDL.
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
					double right; // Mat lon
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
		 * The @a raster_pixel_width and @a raster_pixel_height need to be supplied.
		 */
		static
		non_null_ptr_type
		create(
				int raster_width,
				int raster_height)
		{
			return new Georeferencing(
					create_global_extents(
						raster_width,
						raster_height));
		}

		/**
		 * Creates an affine transform with the specified @a parameters.
		 */
		static
		non_null_ptr_type
		create(
				const parameters_type &parameters)
		{
			return new Georeferencing(parameters);
		}

		/**
		 * Retrieves the affine transform parameters.
		 */
		const parameters_type &
		parameters() const
		{
			return d_parameters;
		}

		/**
		 * Sets the affine transform parameters.
		 */
		void
		set_parameters(
				const parameters_type &parameters_)
		{
			d_parameters = parameters_;
		}

		/**
		 * Retrieves the affine transform parameters as lat-lon extents.
		 *
		 * It is not possible to convert the parameters to lat-lon extents where:
		 *  - The transform rotates or shears the raster, or
		 *  - The raster is flipped horizontally (in this case, we would need to say
		 *    that the right extent has a smaller longitude than the left extent, but
		 *    this would be interpreted as a specifying a raster that crosses the
		 *    International Date Line instead), or
		 *  - The raster is flipped vertically (the bottom is further north than the top).
		 *
		 * Where it is not possible to produce lat-lon extents, boost::none is returned.
		 */
		boost::optional<lat_lon_extents_type>
		lat_lon_extents(
				int raster_width,
				int raster_height) const;

		/**
		 * Sets the affine transform parameters using lat-lon extents.
		 *
		 * If the longitude extents are outside of [-180, +180], the extents are
		 * brought within that range by adding or subtracting multiples of 360.
		 *
		 * If the latitude extents are outside of [-90, +90], the extents are clamped
		 * to the nearest value in that range.
		 *
		 * Returns true if the extents were successfully set, false otherwise.
		 * Extents cannot be set if the bottom is greater than the top.
		 */
		bool
		set_lat_lon_extents(
				lat_lon_extents_type extents,
				int raster_width,
				int raster_height);

		/**
		 * Resets the affine transform so that the raster covers the entire globe.
		 */
		void
		reset_to_global_extents(
				int raster_width,
				int raster_height);

	private:

		static
		parameters_type
		create_global_extents(
				int raster_width,
				int raster_height)
		{
			parameters_type result = {{{
				-180 /* Top left x coordinate */,
				360.0 / raster_width /* 360 degs of longitude from left to right */,
				0.0,
				90.0 /* Top left y coordinate */,
				0.0,
				-180.0 / raster_height /* -180 degs of latitude from top to bottom */
			}}};
			return result;
		}

		Georeferencing(
				const parameters_type &parameters_) :
			d_parameters(parameters_)
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
