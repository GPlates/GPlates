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

#include <iostream>
#include <sstream>
#include <algorithm>  /* std::transform, std::back_inserter */

#include "LatLonPointConversions.h"
#include "InvalidLatLonException.h"
#include "InvalidPolylineException.h"
#include "IndeterminateResultException.h"
#include "PointOnSphere.h"
#include "utils/MathUtils.h"


GPlatesMaths::LatLonPoint::LatLonPoint(
		const double &lat,
		const double &lon):
	d_latitude(lat),
	d_longitude(lon)
{
	if ( ! LatLonPoint::is_valid_latitude(lat)) {

		// It's not a valid latitude.
		throw InvalidLatLonException(lat, InvalidLatLonException::Latitude);
	}
	if ( ! LatLonPoint::is_valid_longitude(lon)) {

		// It's not a valid longitude.
		throw InvalidLatLonException(lon, InvalidLatLonException::Longitude);
	}
}


bool
GPlatesMaths::LatLonPoint::is_valid_latitude(
		const double &val)
{
	return GPlatesUtils::is_in_range(val, -90.0, 90.0);
}


bool
GPlatesMaths::LatLonPoint::is_valid_longitude(
		const double &val)
{
	return GPlatesUtils::is_in_range(val, -360.0, 360.0);
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
	double lat_angle = GPlatesUtils::convert_deg_to_rad(llp.latitude());
	double lon_angle = GPlatesUtils::convert_deg_to_rad(llp.longitude());

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
	const double
			&x = point.position_vector().x().dval(),
			&y = point.position_vector().y().dval(),
			&z = point.position_vector().z().dval();

	// arcsin(theta) is defined for all theta in [-PI/2, PI/2].
	double lat = std::asin(z);
	double lon = std::atan2(y, x);
	if (lon < -GPlatesUtils::Pi) {
		lon = GPlatesUtils::Pi;
	}

	return LatLonPoint::LatLonPoint(
			GPlatesUtils::convert_rad_to_deg(lat),
			GPlatesUtils::convert_rad_to_deg(lon));
}
