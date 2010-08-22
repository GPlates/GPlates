/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2010 The University of Sydney, Australia
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

#include "GmlPoint.h"

#include "maths/MathsUtils.h"
#include "maths/PointOnSphere.h"


const GPlatesPropertyValues::GmlPoint::non_null_ptr_type
GPlatesPropertyValues::GmlPoint::create(
		const std::pair<double, double> &gml_pos,
		GmlProperty gml_property_)
{
	using namespace ::GPlatesMaths;

	const double &lon = gml_pos.first;
	const double &lat = gml_pos.second;

	// FIXME:  Check the validity of the lat/lon coords using functions in LatLonPoint.
	LatLonPoint llp(lat, lon);
	PointOnSphere p = make_point_on_sphere(llp);

	non_null_ptr_type point_ptr(
			new GmlPoint(PointOnSphere::create_on_heap(p.position_vector()), gml_property_));
	point_ptr->d_original_longitude = lon;

	return point_ptr;
}


const GPlatesPropertyValues::GmlPoint::non_null_ptr_type
GPlatesPropertyValues::GmlPoint::create(
		const GPlatesMaths::PointOnSphere &p,
		GmlProperty gml_property_)
{
	using namespace ::GPlatesMaths;

	GmlPoint::non_null_ptr_type point_ptr(
			new GmlPoint(PointOnSphere::create_on_heap(p.position_vector()), gml_property_));
	return point_ptr;
}


GPlatesMaths::LatLonPoint
GPlatesPropertyValues::GmlPoint::point_in_lat_lon() const
{
	// First convert it to lat-lon directly.
	GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*d_point);

	// Fix up the lon if the lat is near 90 or -90.
	if (d_original_longitude)
	{
		if (GPlatesMaths::are_almost_exactly_equal(llp.latitude(), 90.0) ||
				GPlatesMaths::are_almost_exactly_equal(llp.latitude(), -90.0))
		{
			llp = GPlatesMaths::LatLonPoint(llp.latitude(), *d_original_longitude);
		}
	}

	return llp;
}


std::ostream &
GPlatesPropertyValues::GmlPoint::print_to(
		std::ostream &os) const
{
	return os << *d_point;
}

