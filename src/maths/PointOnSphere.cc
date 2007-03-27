/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006 The University of Sydney, Australia
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

#include <ostream>
#include <boost/python.hpp>
#include "PointOnSphere.h"
#include "PointLiesOnGreatCircleArc.h"


bool
GPlatesMaths::PointOnSphere::is_close_to(
 const PointOnSphere &test_point,
 const real_t &closeness_inclusion_threshold,
 real_t &closeness) const {

	closeness = calculate_closeness(test_point, *this);

	return
	 (closeness.isPreciselyGreaterThan(
	   closeness_inclusion_threshold.dval()));
}


bool
GPlatesMaths::PointOnSphere::lies_on_gca(
 const GreatCircleArc &gca) const {

	PointLiesOnGreatCircleArc test_whether_lies_on_gca(gca);
	return (test_whether_lies_on_gca(*this));
}


std::ostream &
GPlatesMaths::operator<<(
 std::ostream &os,
 const PointOnSphere &p) {

	os << p.position_vector();
	return os;
}


/**
 * Here begin the Python wrappers
 */


using namespace boost::python;


void
GPlatesMaths::export_PointOnSphere()
{
	class_<GPlatesMaths::PointOnSphere>("PointOnSphere", no_init)
		.add_property("position_vector", make_function(
					&GPlatesMaths::PointOnSphere::position_vector,
					return_internal_reference<1>()))
	;
}

