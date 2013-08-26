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

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "PythonConverterUtils.h"

#include "global/python.h"

#include "maths/LatLonPoint.h"
#include "maths/PointOnSphere.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


void
export_lat_lon_point()
{
	//
	// LatLonPoint
	//
	bp::class_<GPlatesMaths::LatLonPoint>("LatLonPoint", bp::init<double,double>())
		.def("is_valid_latitude", &GPlatesMaths::LatLonPoint::is_valid_latitude)
		.staticmethod("is_valid_latitude")
		.def("is_valid_longitude", &GPlatesMaths::LatLonPoint::is_valid_longitude)
		.staticmethod("is_valid_longitude")
		.def("get_latitude",
				&GPlatesMaths::LatLonPoint::latitude,
				bp::return_value_policy<bp::copy_const_reference>())
		.def("get_longitude",
				&GPlatesMaths::LatLonPoint::longitude,
				bp::return_value_policy<bp::copy_const_reference>())
		// Member to-PointOnSphere conversion function...
		.def("to_point_on_sphere", &GPlatesMaths::make_point_on_sphere)
		// Static-member from-PointOnSphere conversion function...
		.def("from_point_on_sphere", &GPlatesMaths::make_lat_lon_point)
 		.staticmethod("from_point_on_sphere")
		// Generate '__str__' from 'operator<<'...
		// Note: Seems we need to qualify with 'self_ns::' to avoid MSVC compile error.
		.def(bp::self_ns::str(bp::self))
	;

	// Enable boost::optional<LatLonPoint> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::python_optional<GPlatesMaths::LatLonPoint>();
}

#endif // GPLATES_NO_PYTHON
