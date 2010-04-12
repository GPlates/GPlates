/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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
#include "maths/LatLonPoint.h"
#include "maths/PointOnSphere.h"


const GPlatesPropertyValues::GmlPoint::non_null_ptr_type
GPlatesPropertyValues::GmlPoint::create(
		const std::pair<double, double> &gml_pos)
{
	using namespace ::GPlatesMaths;

	const double &lon = gml_pos.first;
	const double &lat = gml_pos.second;

	// FIXME:  Check the validity of the lat/lon coords using functions in LatLonPoint.
	LatLonPoint llp(lat, lon);
	PointOnSphere p = make_point_on_sphere(llp);

	non_null_ptr_type point_ptr(
			new GmlPoint(PointOnSphere::create_on_heap(p.position_vector())));
	return point_ptr;
}


const GPlatesPropertyValues::GmlPoint::non_null_ptr_type
GPlatesPropertyValues::GmlPoint::create(
		const GPlatesMaths::PointOnSphere &p)
{
	using namespace ::GPlatesMaths;

	GmlPoint::non_null_ptr_type point_ptr(
			new GmlPoint(PointOnSphere::create_on_heap(p.position_vector())));
	return point_ptr;
}


std::ostream &
GPlatesPropertyValues::GmlPoint::print_to(
		std::ostream &os) const
{
	return os << *d_point;
}

