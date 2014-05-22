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
	// LatLonPoint - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesMaths::LatLonPoint>(
					"LatLonPoint",
					"Represents a point in 2D geographic coordinates (latitude and longitude).\n"
					"\n"
					"The following functions convert between :class:`LatLonPoint` and :class:`PointOnSphere`:\n"
					"\n"
					"* :func:`convert_point_on_sphere_to_lat_lon_point` - "
					"convert *from* a :class:`PointOnSphere` *to* a :class:`LatLonPoint`.\n"
					"* :func:`convert_lat_lon_point_to_point_on_sphere` - "
					"convert *from* a :class:`LatLonPoint` *to* a :class:`PointOnSphere`.\n",
					bp::init<double,double>(
							(bp::arg("latitude"), bp::arg("longitude")),
							"__init__(latitude, longitude)\n"
							"  Create a *LatLonPoint* instance from a *latitude* and *longitude*.\n"
							"\n"
							"  :param latitude: the latitude (in degrees)\n"
							"  :type latitude: float\n"
							"  :param longitude: the longitude (in degrees)\n"
							"  :type longitude: float\n"
							"  :raises: InvalidLatLonError if *latitude* or *longitude* is invalid\n"
							"\n"
							"  **NOTE** that *latitude* must satisfy :meth:`is_valid_latitude` and "
							"*longitude* must satisfy :meth:`is_valid_longitude`, otherwise "
							"*InvalidLatLonError* will be raised.\n"
							"  ::\n"
							"\n"
							"    point = pygplates.LatLonPoint(latitude, longitude)\n"))
		.def("is_valid_latitude",
				&GPlatesMaths::LatLonPoint::is_valid_latitude,
				(bp::arg("latitude")),
				"is_valid_latitude(latitude) -> bool\n"
				"  Returns ``True`` if *latitude* is in the range [-90, 90].\n"
				"\n"
				"  :param latitude: the latitude (in degrees)\n"
				"  :type latitude: float\n"
				"  :rtype: bool\n"
				"\n"
				"  ::\n"
				"\n"
				"    if pygplates.LatLonPoint.is_valid_latitude(latitude):\n"
				"      ...\n")
		.staticmethod("is_valid_latitude")
		.def("is_valid_longitude",
				&GPlatesMaths::LatLonPoint::is_valid_longitude,
				(bp::arg("longitude")),
				"is_valid_longitude(longitude) -> bool\n"
				"  Returns ``True`` if *longitude* is in the range [-360, 360].\n"
				"\n"
				"  :param longitude: the longitude (in degrees)\n"
				"  :type longitude: float\n"
				"  :rtype: bool\n"
				"\n"
				"  GPlates uses the half-open range (-180.0, 180.0], but accepts [-360.0, 360.0] as input\n"
				"  ::\n"
				"\n"
				"    if pygplates.LatLonPoint.is_valid_longitude(longitude):\n"
				"      ...\n")
		.staticmethod("is_valid_longitude")
		.def("get_latitude",
				&GPlatesMaths::LatLonPoint::latitude,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_latitude() -> float\n"
				"  Returns the latitude (in degrees).\n"
				"\n"
				"  :rtype: float\n")
		.def("get_longitude",
				&GPlatesMaths::LatLonPoint::longitude,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_longitude() -> float\n"
				"  Returns the longitude (in degrees).\n"
				"\n"
				"  :rtype: float\n")
		// Generate '__str__' from 'operator<<'...
		// Note: Seems we need to qualify with 'self_ns::' to avoid MSVC compile error.
		.def(bp::self_ns::str(bp::self))
	;

	// Non-member conversion function...
	bp::def("convert_lat_lon_point_to_point_on_sphere",
			&GPlatesMaths::make_point_on_sphere,
			(bp::arg("lat_lon_point")),
			"convert_lat_lon_point_to_point_on_sphere(lat_lon_point) -> PointOnSphere\n"
			"  Converts a 2D latitude/longitude point to a 3D cartesian point.\n"
			"\n"
			"  :param lat_lon_point: the 2D latitude/longitude point\n"
			"  :type lat_lon_point: :class:`LatLonPoint`\n"
			"  :rtype: :class:`PointOnSphere`\n"
			"\n");

	// Non-member conversion function...
	bp::def("convert_point_on_sphere_to_lat_lon_point",
			&GPlatesMaths::make_lat_lon_point,
			(bp::arg("point")),
			"convert_point_on_sphere_to_lat_lon_point(point) -> LatLonPoint\n"
			"  Converts a 3D cartesian point to a 2D latitude/longitude point.\n"
			"\n"
			"  :param point: the 3D cartesian point\n"
			"  :type point: :class:`PointOnSphere`\n"
			"  :rtype: :class:`LatLonPoint`\n"
			"\n");

	// Enable boost::optional<LatLonPoint> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesMaths::LatLonPoint>();
}

#endif // GPLATES_NO_PYTHON
