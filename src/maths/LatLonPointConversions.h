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

#ifndef _GPLATES_MATHS_LATLONPOINTCONVERSIONS_H_
#define _GPLATES_MATHS_LATLONPOINTCONVERSIONS_H_

#include <list>
#include "UnitVector3D.h"
#include "PointOnSphere.h"
#include "PolyLineOnSphere.h"


namespace GPlatesMaths
{
	class LatLonPoint {

		public:

			/**
			 * Make a point in the standard spherical coordinate system.
			 * @throws InvalidLatLonException when
			 *   @p isValidLat( @a lat ) returns false or
			 *   @p isValidLon( @a lon ) returns false.
			 */
			LatLonPoint(const real_t &lat, const real_t &lon);

			/**
			 * Return whether a given value is a valid latitude.
			 * GPlates uses the range [-90.0, 90.0].
			 */
			static bool isValidLat(const real_t &val);

			/**
			 * Return whether a given value is a valid longitude.
			 * GPlates uses the half-open range (-180.0, 180.0].
			 * Note that this seems to be different to the range
			 * accepted by the PLATES formats (All Hail PLATES!),
			 * which seems to be [-360.0, 360.0].
			 */
			static bool isValidLon(const real_t &val);

			real_t
			latitude() const { return _lat; }

			real_t
			longitude() const { return _lon; }

		private:

			real_t _lat, _lon;  // in degrees
	};


	inline bool
	operator==(LatLonPoint p1, LatLonPoint p2) {

		return ((p1.latitude() == p2.latitude()) &&
		        (p1.longitude() == p2.longitude()));
	}

	inline std::ostream &operator<< (std::ostream &os, const LatLonPoint &p)
	{
		// TODO: use N/S/E/W notation?
		return os << "(" << p.latitude () << " lat, " << p.longitude ()
			<< " long)";
	}

	namespace LatLonPointConversions
	{
		PointOnSphere convertLatLonPointToPointOnSphere(const
		 LatLonPoint &llp);

		LatLonPoint 
		convertPointOnSphereToLatLonPoint(const PointOnSphere&);
						
		/**
		 * The list must contain at least TWO LatLonPoints.
		 * No two successive LatLonPoints may be equivalent.
		 *
		 * @throws InvalidPolyLineException if @a llpl contains less 
		 *   than two LatLonPoints.
		 */
		PolyLineOnSphere convertLatLonPointListToPolyLineOnSphere(const
		 std::list< LatLonPoint > &llpl);
	}

	/// The north pole (latitude \f$ 90^\circ \f$)
	static const PointOnSphere NorthPole = LatLonPointConversions::convertLatLonPointToPointOnSphere (LatLonPoint (90.0, 0.0));
	/// The south pole (latitude \f$ -90^\circ \f$)
	static const PointOnSphere SouthPole = LatLonPointConversions::convertLatLonPointToPointOnSphere (LatLonPoint (-90.0, 0.0));

}

#endif  // _GPLATES_MATHS_LATLONPOINTCONVERSIONS_H_
