/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Author$
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
 * Authors:
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_MATHS_OPERATIONSONSPHERE_H_
#define _GPLATES_MATHS_OPERATIONSONSPHERE_H_

#include <list>
#include "UnitVector3D.h"
#include "PointOnSphere.h"
#include "PolyLineOnSphere.h"


namespace GPlatesMaths
{
	class LatLonPoint {

		public:

			static LatLonPoint
			CreateLatLonPoint(const real_t &lat, const real_t &lon);

			static bool isValidLat(const real_t &val);
			static bool isValidLon(const real_t &val);

			real_t
			latitude() const { return _lat; }

			real_t
			longitude() const { return _lon; }

			// no default constructor

		protected:

			LatLonPoint(const real_t &lat, const real_t &lon)
			 : _lat(lat), _lon(lon) {  }

		private:

			real_t _lat, _lon;  // in degrees
	};


	inline bool
	operator==(LatLonPoint p1, LatLonPoint p2) {

		return ((p1.latitude() == p2.latitude()) &&
		        (p1.longitude() == p2.longitude()));
	}


	namespace OperationsOnSphere
	{
		UnitVector3D convertLatLongToUnitVector(const real_t& latitude,
		 const real_t& longitude);

		PointOnSphere convertLatLonPointToPointOnSphere(const
		 LatLonPoint &llp);

		/**
		 * The list must contain at least TWO LatLonPoints.
		 * No two successive LatLonPoints may be equivalent.
		 */
		PolyLineOnSphere convertLatLonPointListToPolyLineOnSphere(const
		 std::list< LatLonPoint > &llpl);
	}
}

#endif  // _GPLATES_MATHS_OPERATIONSONSPHERE_H_
