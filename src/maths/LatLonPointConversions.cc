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

#include <iostream>
#include <sstream>
#include <algorithm>  /* std::transform, std::back_inserter */

#include "LatLonPointConversions.h"
#include "InvalidLatLonException.h"
#include "InvalidPolylineException.h"
#include "IndeterminateResultException.h"
#include "PointOnSphere.h"


GPlatesMaths::LatLonPoint::LatLonPoint(
 const real_t &lat,
 const real_t &lon) :
 m_lat(lat),
 m_lon(lon) {

	if ( ! LatLonPoint::isValidLat(lat)) {

		// not a valid latitude
		std::ostringstream oss;
		oss << "Attempted to create a lat/lon point "
			"using the invalid latitude " << lat;

		throw InvalidLatLonException(oss.str().c_str());
	}
	if ( ! LatLonPoint::isValidLon(lon)) {

		// not a valid longitude
		std::ostringstream oss;
		oss << "Attempted to create a lat/lon point "
			"using the invalid longitude " << lon;

		throw InvalidLatLonException(oss.str().c_str());
	}
}


bool
GPlatesMaths::LatLonPoint::isValidLat(
 const real_t &val) {

	return (-90.0 <= val && val <= 90.0);
}


bool
GPlatesMaths::LatLonPoint::isValidLon(
 const real_t &val) {

	return (-360.0 <= val && val <= 360.0);
}


std::ostream &
GPlatesMaths::operator<<(
 std::ostream &os,
 const LatLonPoint &p) {

	// TODO: use N/S/E/W notation?
	os
	 << "(lat: "
	 << p.latitude()
	 << ", lon: "
	 << p.longitude()
	 << ")";

	return os;
}


const GPlatesMaths::PointOnSphere
GPlatesMaths::LatLonPointConversions::convertLatLonPointToPointOnSphere(
 const LatLonPoint &llp) {

	real_t lat_angle = degreesToRadians(llp.latitude());
	real_t long_angle = degreesToRadians(llp.longitude());

	real_t radius_of_small_circle_of_latitude = cos(lat_angle);

	real_t x_comp = radius_of_small_circle_of_latitude * cos(long_angle);
	real_t y_comp = radius_of_small_circle_of_latitude * sin(long_angle);
	real_t z_comp = sin(lat_angle);  // height above equator

	UnitVector3D uv = UnitVector3D(x_comp, y_comp, z_comp);
	return PointOnSphere(uv);
}


const GPlatesMaths::LatLonPoint 
GPlatesMaths::LatLonPointConversions::convertPointOnSphereToLatLonPoint(
 const PointOnSphere& point) {

	const real_t
	 &x = point.position_vector().x(),
	 &y = point.position_vector().y(),
	 &z = point.position_vector().z();

	// arcsin(theta) is defined for all theta in [-PI/2, PI/2].
	real_t lat = asin(z);

	// Radius of the small circle of latitude.
//	real_t rad_sc = cos(lat);
//	real_t lon = (rad_sc == 0.0 ? 0.0 : asin(y/rad_sc));
	
	real_t lon = atan2(y.dval(), x.dval());

	if (lon <= real_t(-GPlatesMaths::PI)) lon = real_t(GPlatesMaths::PI);

	return
	 LatLonPoint::LatLonPoint(
	  radiansToDegrees(lat),
	  radiansToDegrees(lon));
}


const GPlatesMaths::PolylineOnSphere
GPlatesMaths::LatLonPointConversions::convertLatLonPointListToPolylineOnSphere(
 const std::list< LatLonPoint > &llp_list) {

	std::list< PointOnSphere > pos_list;
	std::transform(llp_list.begin(), llp_list.end(),
	 std::back_inserter(pos_list),
	 convertLatLonPointToPointOnSphere);
	PolylineOnSphere polyline = PolylineOnSphere::create(pos_list);

	// Yeah, this function is pretty short, but don't try to inline it; the
	// invocation of 'PolylineOnSphere::create' is going to lead to a whole
	// bunch of inlined template code.

	return polyline;
}
