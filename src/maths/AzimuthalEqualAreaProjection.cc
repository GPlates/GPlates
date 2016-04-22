/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#include "AzimuthalEqualAreaProjection.h"

#include "MathsUtils.h"
#include "types.h"


GPlatesMaths::AzimuthalEqualAreaProjection::AzimuthalEqualAreaProjection(
		const LatLonPoint &center_of_projection,
		const double &projection_scale) :
	d_center_of_projection(center_of_projection),
	d_sin_center_of_projection_latitude(
			std::sin(convert_deg_to_rad(d_center_of_projection.latitude()))),
	d_cos_center_of_projection_latitude(
			std::cos(convert_deg_to_rad(d_center_of_projection.latitude()))),
	d_projection_scale(projection_scale)
{
}


GPlatesMaths::AzimuthalEqualAreaProjection::AzimuthalEqualAreaProjection(
		const PointOnSphere &center_of_projection,
		const double &projection_scale) :
	d_center_of_projection(make_lat_lon_point(center_of_projection)),
	d_sin_center_of_projection_latitude(
			std::sin(convert_deg_to_rad(d_center_of_projection.latitude()))),
	d_cos_center_of_projection_latitude(
			std::cos(convert_deg_to_rad(d_center_of_projection.latitude()))),
	d_projection_scale(projection_scale)
{
}


QPointF
GPlatesMaths::AzimuthalEqualAreaProjection::project_from_lat_lon(
		const LatLonPoint &point) const
{
	const double lam_0 = convert_deg_to_rad(d_center_of_projection.longitude()); // center of projection lon

	const double sin_phi_0 = d_sin_center_of_projection_latitude;
	const double cos_phi_0 = d_cos_center_of_projection_latitude;

	const double phi = convert_deg_to_rad(point.latitude()); // Node latitude
	const double lam = convert_deg_to_rad(point.longitude()); // Node longitude

	const double sin_phi = std::sin(phi);
	const double cos_phi = std::cos(phi);

	const double cos_lam_minus_lam_0 = std::cos(lam - lam_0);
	const double sin_lam_minus_lam_0 = std::sin(lam - lam_0);

	const double k = std::sqrt(
			2 / (1 + sin_phi_0 * sin_phi + cos_phi_0 * cos_phi * cos_lam_minus_lam_0));

	const double x = d_projection_scale * k * cos_phi * sin_lam_minus_lam_0;
	const double y = d_projection_scale * k * (
			cos_phi_0 * sin_phi - sin_phi_0 * cos_phi * cos_lam_minus_lam_0);

	return QPointF(x,y);
}


QPointF
GPlatesMaths::AzimuthalEqualAreaProjection::project_from_point_on_sphere(
		const PointOnSphere &point) const
{
	return project_from_lat_lon(make_lat_lon_point(point));
}


GPlatesMaths::LatLonPoint
GPlatesMaths::AzimuthalEqualAreaProjection::unproject_to_lat_lon(
		const QPointF &point) const
{
	const double x = point.x();
	const double y = point.y();

	const double rho = std::sqrt(x * x + y * y);

	// If point is at centre of projection then return it now.
	// This avoids dividing by zero when calculating 'phi' below.
	if (GPlatesMaths::are_almost_exactly_equal(rho, 0))
	{
		return LatLonPoint(
				d_center_of_projection.latitude(),
				d_center_of_projection.longitude());
	}

	const double a = rho / (2.0 * d_projection_scale);
	const double c = 2 * std::asin(a);

	const double sin_c = std::sin(c);
	const double cos_c = std::cos(c);

	const double phi_0 = convert_deg_to_rad(d_center_of_projection.latitude()); // center of projection lat
	const double lam_0 = convert_deg_to_rad(d_center_of_projection.longitude()); // center of projection lon

	const double sin_phi_0 = d_sin_center_of_projection_latitude;
	const double cos_phi_0 = d_cos_center_of_projection_latitude;

	// Latitude in Radians
	const double phi = std::asin(cos_c * sin_phi_0 + y * sin_c * cos_phi_0 / rho);

	// Longitude in radians
	double lam;

	if (real_t(phi_0) == PI / 2)
	{
		lam = lam_0 + std::atan2(x, -y);
	}
	else if (real_t(phi_0) == -PI / 2)
	{
		lam = lam_0 + std::atan2(x, y);
	}
	else
	{
		lam = lam_0 + std::atan2(
				x * sin_c,
				rho * cos_phi_0 * cos_c - y * sin_phi_0 * sin_c);
	}

	const double lat = convert_rad_to_deg(phi);
	const double lon = convert_rad_to_deg(lam);

	return LatLonPoint(lat, lon);
}


GPlatesMaths::PointOnSphere
GPlatesMaths::AzimuthalEqualAreaProjection::unproject_to_point_on_sphere(
		const QPointF &point) const
{
	return make_point_on_sphere(unproject_to_lat_lon(point));
}
