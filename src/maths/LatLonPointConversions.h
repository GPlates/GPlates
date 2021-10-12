/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004 The University of Sydney, Australia
 *  (under the name "OperationsOnSphere.h")
 * Copyright (C) 2004, 2005, 2006, 2007 The University of Sydney, Australia
 *  (under the name "LatLonPointConversions.h")
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

#ifndef GPLATES_MATHS_LATLONPOINTCONVERSIONS_H
#define GPLATES_MATHS_LATLONPOINTCONVERSIONS_H

#include <iosfwd>
#include <list>


namespace GPlatesMaths
{
	class PointOnSphere;

	class LatLonPoint
	{
	public:

		/**
		 * Make a point in the standard spherical coordinate
		 * system.
		 * @throws InvalidLatLonException when
		 *   @p isValidLat( @a lat ) would return false or
		 *   @p isValidLon( @a lon ) would return false.
		 */
		LatLonPoint(
				const double &lat,
				const double &lon);

		/**
		 * Return whether a given value is a valid latitude.
		 *
		 * GPlates uses the range [-90.0, 90.0].
		 */
		static
		bool
		is_valid_latitude(
				const double &val);

		/**
		 * Return whether a given value is a valid longitude.
		 *
		 * GPlates uses the half-open range (-180.0, 180.0] for
		 * output, but accepts [-360.0, 360.0] as input.
		 */
		static
		bool
		is_valid_longitude(
				const double &val);

		const double &
		latitude() const
		{
			return d_latitude;
		}

		const double &
		longitude() const
		{
			return d_longitude;
		}

	private:

		/**
		 * The latitude of the point, in degrees.
		 */
		double d_latitude;

		/**
		 * The longitude of the point, in degrees.
		 */
		double d_longitude;

		// Declare this operator private (but don't define it) so it can never be invoked.
		bool
		operator==(
				const LatLonPoint &);

		// Declare this operator private (but don't define it) so it can never be invoked.
		bool
		operator!=(
				const LatLonPoint &);
	};


	std::ostream &
	operator<<(
			std::ostream &os,
			const LatLonPoint &p);


	const PointOnSphere
	make_point_on_sphere(
			const LatLonPoint &);


	const LatLonPoint 
	make_lat_lon_point(
			const PointOnSphere &);

}

#endif  // GPLATES_MATHS_LATLONPOINTCONVERSIONS_H
