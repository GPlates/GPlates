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


/**
 * Return whether a given value is a valid latitude.
 * GPlates uses the range [-90.0, 90.0].
 */
bool
LatLonPoint::isValidLat(const real_t &val) {

	return (-90.0 <= val && val <= 90.0);
}


/**
 * Return whether a given value is a valid longitude.
 * GPlates uses the half-open range (-180.0, 180.0].
 * Note that this is different to the range used by the PLATES format
 * (All Hail PLATES!), which is [-180.0, 180.0].
 */
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


UnitVector3D
OperationsOnSphere::convertLatLongPointToUnitVector(const LatLonPoint &p) {

	real_t lat_angle = degreesToRadians(p.latitude());
	real_t long_angle = degreesToRadians(p.longitude());

	real_t radius_of_small_circle_of_latitude = cos(lat_angle);

	real_t x_comp = radius_of_small_circle_of_latitude * cos(long_angle);
	real_t y_comp = radius_of_small_circle_of_latitude * sin(long_angle);
	real_t z_comp = sin(lat_angle);  // height above equator

	return UnitVector3D(x_comp, y_comp, z_comp);
}
