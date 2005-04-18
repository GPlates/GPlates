/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef GPLATES_MATHS_LATLONPOINTCONVERSIONS_H
#define GPLATES_MATHS_LATLONPOINTCONVERSIONS_H

#include <iosfwd>
#include <list>

#include "PointOnSphere.h"
#include "PolyLineOnSphere.h"


namespace GPlatesMaths {

	class LatLonPoint {

		public:

			/**
			 * Make a point in the standard spherical coordinate
			 * system.
			 * @throws InvalidLatLonException when
			 *   @p isValidLat( @a lat ) returns false or
			 *   @p isValidLon( @a lon ) returns false.
			 */
			LatLonPoint(
			 const real_t &lat,
			 const real_t &lon);

			/**
			 * Return whether a given value is a valid latitude.
			 *
			 * GPlates uses the range [-90.0, 90.0].  Thankfully,
			 * the PLATES formats appear to use the same thing.
			 */
			static
			bool
			isValidLat(
			 const real_t &val);

			/**
			 * Return whether a given value is a valid longitude.
			 *
			 * GPlates uses the half-open range (-180.0, 180.0].
			 * Note that this is different to the range accepted by
			 * the PLATES formats (All Hail PLATES!), which seems
			 * to be [-360.0, 360.0].
			 */
			static
			bool
			isValidLon(
			 const real_t &val);

			real_t
			latitude() const {
				
				return m_lat;
			}

			real_t
			longitude() const {
				
				return m_lon;
			}

		private:

			real_t m_lat, m_lon;  // in degrees

	};


	inline
	bool
	operator==(
	 const LatLonPoint &p1,
	 const LatLonPoint &p2) {

		return ((p1.latitude() == p2.latitude()) &&
		        (p1.longitude() == p2.longitude()));
	}


	std::ostream &
	operator<<(
	 std::ostream &os,
	 const LatLonPoint &p);


	namespace LatLonPointConversions {

		const PointOnSphere
		convertLatLonPointToPointOnSphere(
		 const LatLonPoint &);

		const LatLonPoint 
		convertPointOnSphereToLatLonPoint(
		 const PointOnSphere &);
						
		/**
		 * The list must contain at least TWO LatLonPoints.
		 * No two successive LatLonPoints may be equivalent.
		 *
		 * @throws InvalidPolyLineException if @a llpl contains less 
		 *   than two LatLonPoints.
		 */
		const PolyLineOnSphere
		convertLatLonPointListToPolyLineOnSphere(
		 const std::list< LatLonPoint > &);

		const std::list< LatLonPoint >
		convertPolyLineOnSphereToLatLonPointList(
		 const PolyLineOnSphere &);

	}

	/// The north pole (latitude \f$ 90^\circ \f$)
	static const PointOnSphere NorthPole =
	 LatLonPointConversions::convertLatLonPointToPointOnSphere(
	  LatLonPoint(90.0, 0.0));

	/// The south pole (latitude \f$ -90^\circ \f$)
	static const PointOnSphere SouthPole =
	 LatLonPointConversions::convertLatLonPointToPointOnSphere(
	  LatLonPoint(-90.0, 0.0));

}

#endif  // GPLATES_MATHS_LATLONPOINTCONVERSIONS_H
