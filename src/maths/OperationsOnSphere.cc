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

#include <sstream>
#include "OperationsOnSphere.h"
#include "InvalidLatLonException.h"
#include "InvalidPolyLineException.h"
#include "GreatCircleArc.h"


using namespace GPlatesMaths;


LatLonPoint
LatLonPoint::CreateLatLonPoint(const real_t &lat, const real_t &lon) {

	if ( ! LatLonPoint::isValidLat(lat)) {

		// not a valid latitude
		std::ostringstream oss("Attempted to create a lat/lon point "
		 "using the invalid latitude ");
		oss << lat;

		throw InvalidLatLonException(oss.str().c_str());
	}
	if ( ! LatLonPoint::isValidLon(lon)) {

		// not a valid latitude
		std::ostringstream oss("Attempted to create a lat/lon point "
		 "using the invalid latitude ");
		oss << lat;

		throw InvalidLatLonException(oss.str().c_str());
	}

	return LatLonPoint(lat, lon);
}


bool
LatLonPoint::isValidLat(const real_t &val) {

	return (-90.0 <= val && val <= 90.0);
}


bool
LatLonPoint::isValidLon(const real_t &val) {

	return (-180.0 < val && val <= 180.0);
}


UnitVector3D
OperationsOnSphere::convertLatLongToUnitVector(const real_t& latitude, 
 const real_t& longitude) {

	real_t lat_angle = degreesToRadians(latitude);
	real_t long_angle = degreesToRadians(longitude);

	real_t radius_of_small_circle_of_latitude = cos(lat_angle);

	real_t x_comp = radius_of_small_circle_of_latitude * cos(long_angle);
	real_t y_comp = radius_of_small_circle_of_latitude * sin(long_angle);
	real_t z_comp = sin(lat_angle);  // height above equator

	return UnitVector3D(x_comp, y_comp, z_comp);
}


PointOnSphere
OperationsOnSphere::convertLatLonPointToPointOnSphere(const LatLonPoint &llp) {

	real_t lat_angle = degreesToRadians(llp.latitude());
	real_t long_angle = degreesToRadians(llp.longitude());

	real_t radius_of_small_circle_of_latitude = cos(lat_angle);

	real_t x_comp = radius_of_small_circle_of_latitude * cos(long_angle);
	real_t y_comp = radius_of_small_circle_of_latitude * sin(long_angle);
	real_t z_comp = sin(lat_angle);  // height above equator

	UnitVector3D uv = UnitVector3D(x_comp, y_comp, z_comp);
	return PointOnSphere(uv);
}


PolyLineOnSphere
OperationsOnSphere::convertLatLonPointListToPolyLineOnSphere(const
	std::list< LatLonPoint > &llpl) {

	if (llpl.size() < 2) {

		// not enough points to create even a single great circle arc.
		std::ostringstream oss("Attempted to create a poly-line "
		 "from only ");
		oss << llpl.size() << " point.";

		throw InvalidPolyLineException(oss.str().c_str());
	}
	// else, we know that there will be *at least* two points

	PolyLineOnSphere plos;

	std::list< LatLonPoint >::const_iterator it = llpl.begin();

	LatLonPoint p1 = *it;
	UnitVector3D u1 = convertLatLongToUnitVector(p1.latitude(),
	 p1.longitude());

	for (it++; it != llpl.end(); it++) {

		LatLonPoint p2 = *it;
		UnitVector3D u2 = convertLatLongToUnitVector(p2.latitude(),
		 p2.longitude());

		GreatCircleArc g = GreatCircleArc::CreateGreatCircleArc(u1, u2);
		plos.push_back(g);

		u1 = u2;
	}
	return plos;
}
