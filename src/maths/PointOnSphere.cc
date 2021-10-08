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
#ifdef HAVE_PYTHON
# include <boost/python.hpp>
#endif
#include "PointOnSphere.h"
#include "PointLiesOnGreatCircleArc.h"


const GPlatesMaths::PointOnSphere GPlatesMaths::PointOnSphere::north_pole =
		GPlatesMaths::make_point_on_sphere(GPlatesMaths::LatLonPoint(90.0, 0.0));


const GPlatesMaths::PointOnSphere GPlatesMaths::PointOnSphere::south_pole =
		GPlatesMaths::make_point_on_sphere(GPlatesMaths::LatLonPoint(-90.0, 0.0));


bool
GPlatesMaths::PointOnSphere::is_close_to(
		const PointOnSphere &test_point,
		const real_t &closeness_inclusion_threshold,
		real_t &closeness) const
{
	closeness = calculate_closeness(test_point, *this);
	return closeness.isPreciselyGreaterThan(closeness_inclusion_threshold.dval());
}


bool
GPlatesMaths::PointOnSphere::lies_on_gca(
		const GreatCircleArc &gca) const
{
	PointLiesOnGreatCircleArc test_whether_lies_on_gca(gca);
	return test_whether_lies_on_gca(*this);
}


std::ostream &
GPlatesMaths::operator<<(
	std::ostream &os,
	const PointOnSphere &p)
{
	os << p.position_vector();
	return os;
}


#ifdef HAVE_PYTHON
/**
 * Here begin the Python wrappers
 */


using namespace boost::python;


void
GPlatesMaths::export_PointOnSphere()
{
	class_<GPlatesMaths::PointOnSphere>("PointOnSphere", init<const PointOnSphere &>())
		// .def("create_on_heap", &GPlatesMaths::PointOnSphere::create_on_heap).staticmethod("create_on_heap")
		// .def("clone_on_heap", &GPlatesMaths::PointOnSphere::clone_on_heap).staticmethod("clone_on_heap")
		.def(init<const UnitVector3D &>())
		//.def("assign", &GPlatesMaths::PointOnSphere::operator=)
		.add_property("position_vector", make_function(
				&GPlatesMaths::PointOnSphere::position_vector,
				return_internal_reference<1>()))
		.def("is_close_to", &GPlatesMaths::PointOnSphere::is_close_to,
				args("test_point", "closeness_inclusion_threshold", "closeness"))
		.def("lies_on_gca", &GPlatesMaths::PointOnSphere::lies_on_gca,
				args("gca"))
		.def(self == other<GPlatesMaths::PointOnSphere>())
		.def(self != other<GPlatesMaths::PointOnSphere>())
		.def(self_ns::str(self))
	;
	// def("get_antipodal_point", &GPlatesMaths::get_antipodal_point, args("p"));
	def("calculate_closeness", &GPlatesMaths::calculate_closeness, args("p1", "p2"));
	def("points_are_coincident", &GPlatesMaths::points_are_coincident, args("p1", "p2"));
}
#endif

