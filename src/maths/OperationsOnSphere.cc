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
 *   Hamish Ivey-Law <hlaw@geosci.usyd.edu.au>
 *   Dave Symonds <ds@geosci.usyd.edu.au>
 */

#include <sstream>
#include "OperationsOnSphere.h"
#include "InvalidLatLonException.h"
#include "InvalidPolyLineException.h"
#include "GreatCircleArc.h"
#include "PointOnSphere.h"

using namespace GPlatesMaths;

GPlatesMaths::LatLonPoint::LatLonPoint (const real_t &lat, const real_t &lon)
	: _lat(lat), _lon(lon)
{

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
}


bool
GPlatesMaths::LatLonPoint::isValidLat(const real_t &val) {

	return (-90.0 <= val && val <= 90.0);
}


bool
GPlatesMaths::LatLonPoint::isValidLon(const real_t &val) {

	return (-180.0 < val && val <= 180.0);
}


UnitVector3D
GPlatesMaths::OperationsOnSphere::convertLatLongToUnitVector
		(const real_t& latitude, const real_t& longitude)
{

	real_t lat_angle = degreesToRadians(latitude);
	real_t long_angle = degreesToRadians(longitude);

	real_t radius_of_small_circle_of_latitude = cos(lat_angle);

	real_t x_comp = radius_of_small_circle_of_latitude * cos(long_angle);
	real_t y_comp = radius_of_small_circle_of_latitude * sin(long_angle);
	real_t z_comp = sin(lat_angle);  // height above equator

	return UnitVector3D(x_comp, y_comp, z_comp);
}


PointOnSphere
GPlatesMaths::OperationsOnSphere::convertLatLonPointToPointOnSphere
						(const LatLonPoint &llp)
{
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
GPlatesMaths::OperationsOnSphere::convertLatLonPointListToPolyLineOnSphere
				(const std::list< LatLonPoint > &llpl)
{
	if (llpl.size() == 0) {

		// not enough points to create even a single great circle arc.
		std::ostringstream oss("Attempted to create a poly-line "
		 "from 0 points.");

		throw InvalidPolyLineException(oss.str().c_str());
	}
	if (llpl.size() == 1) {

		// not enough points to create even a single great circle arc.
		std::ostringstream oss("Attempted to create a poly-line "
		 "from only 1 point.");

		throw InvalidPolyLineException(oss.str().c_str());
	}
	// else, we know that there will be *at least* two points

	PolyLineOnSphere plos;

	std::list< LatLonPoint >::const_iterator it = llpl.begin();
	PointOnSphere p1 = convertLatLonPointToPointOnSphere(*it);

	for (it++; it != llpl.end(); it++) {

		PointOnSphere p2 = convertLatLonPointToPointOnSphere(*it);
		GreatCircleArc g = GreatCircleArc::CreateGreatCircleArc(p1, p2);
		plos.push_back(g);

		p1 = p2;
	}
	return plos;
}


LatLonPoint 
GPlatesMaths::OperationsOnSphere::convertPointOnSphereToLatLonPoint
					(const PointOnSphere& point)
{
	const real_t &x = point.unitvector().x(),
				 &y = point.unitvector().y(),
				 &z = point.unitvector().z();

	// arcsin(theta) is defined for all theta in [-pi/2,pi/2].
	real_t lat = asin(z);
	// Radius of the small circle of latitude.
//	real_t rad_sc = cos(lat);
//	real_t lon = (rad_sc == 0.0 ? 0.0 : asin(y/rad_sc));
	
	real_t lon = atan2(y.dval(), x.dval());
	
	if (lon <= real_t(-MATHVALUE_PI))
		lon = real_t(MATHVALUE_PI);

	lat = radiansToDegrees(lat);
	lon = radiansToDegrees(lon);
	
	return LatLonPoint::LatLonPoint(lat, lon);
}
