/* $Id$ */

/**
 * \file 
 * Contains the implementation of the LatLonPoint class.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2010 The University of Sydney, Australia
 *  (also under the names "OperationsOnSphere.cc" and "LatLonPointConversions.cc")
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

#include <iostream>
#include <sstream>
#include <algorithm>  /* std::transform, std::back_inserter */

#include "LatLonPoint.h"
#include "InvalidLatLonException.h"
#include "IndeterminateResultException.h"
#include "MathsUtils.h"
#include "PointOnSphere.h"


GPlatesMaths::LatLonPoint::LatLonPoint(
		const double &lat,
		const double &lon):
	d_latitude(lat),
	d_longitude(lon)
{
	if ( ! LatLonPoint::is_valid_latitude(lat)) {

		// It's not a valid latitude.
		throw InvalidLatLonException(GPLATES_EXCEPTION_SOURCE,
				lat, InvalidLatLonException::Latitude);
	}
	if ( ! LatLonPoint::is_valid_longitude(lon)) {

		// It's not a valid longitude.
		throw InvalidLatLonException(GPLATES_EXCEPTION_SOURCE,
				lon, InvalidLatLonException::Longitude);
	}
}


bool
GPlatesMaths::LatLonPoint::is_valid_latitude(
		const double &val)
{
	return is_in_range(val, -90.0, 90.0);
}


bool
GPlatesMaths::LatLonPoint::is_valid_longitude(
		const double &val)
{
	return is_in_range(val, -360.0, 360.0);
}


std::ostream &
GPlatesMaths::operator<<(
		std::ostream &os,
		const LatLonPoint &p)
{
	// TODO: use N/S/E/W notation?
	os << "(lat: " << p.latitude() << ", lon: " << p.longitude() << ")";
	return os;
}


const GPlatesMaths::PointOnSphere
GPlatesMaths::make_point_on_sphere(
		const LatLonPoint &llp)
{
	double lat_angle = convert_deg_to_rad(llp.latitude());
	double lon_angle = convert_deg_to_rad(llp.longitude());

	double radius_of_small_circle_of_latitude = std::cos(lat_angle);

	double x_comp = radius_of_small_circle_of_latitude * std::cos(lon_angle);
	double y_comp = radius_of_small_circle_of_latitude * std::sin(lon_angle);
	double z_comp = std::sin(lat_angle);  // height above equator

	UnitVector3D uv = UnitVector3D(x_comp, y_comp, z_comp);
	return PointOnSphere(uv);
}


const GPlatesMaths::LatLonPoint 
GPlatesMaths::make_lat_lon_point(
		const PointOnSphere& point)
{
	// Note:  We use class Real's asin/atan2, since these functions perform domain-validity
	// checking (and correct almost-valid values, whose invalidity is presumably the result of
	// accumulated floating-point error).
	const Real
			&x = point.position_vector().x(),
			&y = point.position_vector().y(),
			&z = point.position_vector().z();

	//std::cerr << "--\nx: " << x << ", y: " << y << ", z: " << z << std::endl;
	double lat = asin(z).dval();
	double lon = atan2(y, x).dval();
	if (lon < -Pi) {
		lon = Pi;
	}
	//std::cerr << "lat: " << lat << ", lon: " << lon << std::endl;

	return LatLonPoint::LatLonPoint(
			convert_rad_to_deg(lat),
			convert_rad_to_deg(lon));
}
