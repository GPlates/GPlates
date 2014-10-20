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
#include "PythonHashDefVisitor.h"

#include "global/python.h"

#include "maths/LatLonPoint.h"
#include "maths/PointOnSphere.h"
#include "maths/UnitVector3D.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	// North and south poles.
	const GPlatesMaths::LatLonPoint lat_lon_point_north_pole(
			make_lat_lon_point(
					GPlatesMaths::PointOnSphere(GPlatesMaths::UnitVector3D::zBasis())));
	const GPlatesMaths::LatLonPoint lat_lon_point_south_pole(
			make_lat_lon_point(
					GPlatesMaths::PointOnSphere(-GPlatesMaths::UnitVector3D::zBasis())));


	bp::tuple
	lat_lon_point_to_xyz(
			const GPlatesMaths::LatLonPoint &lat_lon_point)
	{
		const GPlatesMaths::UnitVector3D position_vector =
				make_point_on_sphere(lat_lon_point).position_vector();

		return bp::make_tuple(position_vector.x(), position_vector.y(), position_vector.z());
	}

	bp::tuple
	lat_lon_point_to_lat_lon(
			const GPlatesMaths::LatLonPoint &lat_lon_point)
	{
		return bp::make_tuple(lat_lon_point.latitude(), lat_lon_point.longitude());
	}

	bp::object
	lat_lon_point_eq(
			const GPlatesMaths::LatLonPoint &lat_lon_point,
			bp::object other)
	{
		bp::extract<const GPlatesMaths::LatLonPoint &> extract_other_llp_instance(other);
		// Prevent equality comparisons between LatLonPoints.
		if (extract_other_llp_instance.check())
		{
			PyErr_SetString(PyExc_TypeError, "Cannot equality compare (==, !=) LatLonPoints");
			bp::throw_error_already_set();
		}

		// Return NotImplemented so python can continue looking for a match
		// (eg, in case 'other' is a class that implements relational operators with LatLonPoint).
		//
		// NOTE: This will most likely fall back to python's default handling which uses 'id()'
		// and hence will compare based on *python* object address rather than *C++* object address.
		return bp::object(bp::handle<>(bp::borrowed(Py_NotImplemented)));
	}

	bp::object
	lat_lon_point_ne(
			const GPlatesMaths::LatLonPoint &lat_lon_point,
			bp::object other)
	{
		bp::object ne_result = lat_lon_point_eq(lat_lon_point, other);
		if (ne_result.ptr() == Py_NotImplemented)
		{
			// Return NotImplemented.
			return ne_result;
		}

		// Invert the result.
		return bp::object(!bp::extract<bool>(ne_result));
	}
}

void
export_lat_lon_point()
{
	//
	// LatLonPoint - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<GPlatesMaths::LatLonPoint>(
					"LatLonPoint",
					"Represents a point in 2D geographic coordinates (latitude and longitude).\n"
					"\n"
					"LatLonPoints are *not* equality (``==``, ``!=``) comparable (will raise ``TypeError`` "
					"when compared) and are not hashable (cannot be used as a key in a ``dict``).\n"
					"\n"
					"Convenience class static data are available for the North and South poles:\n"
					"\n"
					"* ``pygplates.LatLonPoint.north_pole``\n"
					"* ``pygplates.LatLonPoint.south_pole``\n",
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
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Returns ``True`` if *latitude* is in the range [-90, 90].\n"
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
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Returns ``True`` if *longitude* is in the range [-360, 360].\n"
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
		// Static property 'pygplates.LatLonPoint.north_pole'...
		.def_readonly("north_pole", GPlatesApi::lat_lon_point_north_pole)
		// Static property 'pygplates.LatLonPoint.south_pole'...
		.def_readonly("south_pole", GPlatesApi::lat_lon_point_south_pole)
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
		.def("to_point_on_sphere",
				&GPlatesMaths::make_point_on_sphere,
				"to_point_on_sphere() -> PointOnSphere\n"
				"  Returns the cartesian coordinates as a :class:`PointOnSphere`.\n"
				"\n"
				"  :rtype: :class:`PointOnSphere`\n")
		.def("to_xyz",
				&GPlatesApi::lat_lon_point_to_xyz,
				"to_xyz() -> x, y, z\n"
				"  Returns the cartesian coordinates as the tuple (x,y,z).\n"
				"\n"
				"  :rtype: the tuple (float,float,float)\n"
				"\n"
				"  ::\n"
				"\n"
				"    x, y, z = lat_lon_point.to_xyz()\n"
				"\n"
				"  This is similar to :meth:`PointOnSphere.to_xyz`.\n")
		.def("to_lat_lon",
				&GPlatesApi::lat_lon_point_to_lat_lon,
				"to_lat_lon() -> latitude, longitude\n"
				"  Returns the tuple (latitude,longitude) in degrees.\n"
				"\n"
				"  :rtype: the tuple (float,float)\n"
				"\n"
				"  ::\n"
				"\n"
				"    latitude, longitude = lat_lon_point.to_lat_lon()\n")
		// Due to wrapping of longitude values representing unequal but equivalent positions
		// we prevent equality comparisons and also make unhashable since user will expect hashing
		// to be based on object value and not object identity (address).
		// Make unhashable, with no *equality* comparison operators (we explicitly define them)...
		.def(GPlatesApi::NoHashDefVisitor(false, true))
		.def("__eq__", &GPlatesApi::lat_lon_point_eq)
		.def("__ne__", &GPlatesApi::lat_lon_point_ne)
		// Generate '__str__' from 'operator<<'...
		// Note: Seems we need to qualify with 'self_ns::' to avoid MSVC compile error.
		.def(bp::self_ns::str(bp::self))
	;

	// Non-member conversion function...
	bp::def("convert_lat_lon_point_to_point_on_sphere",
			&GPlatesMaths::make_point_on_sphere,
			(bp::arg("lat_lon_point"))
			// We'll keep this function (for those still using it) but we won't document it so that
			// it doesn't show up in the API documentation.
			// There are better ways to convert between LatLonPoint and PointOnSphere such as
			// 'LatLonPoint.to_point_on_sphere()' and 'PointOnSphere.to_lat_lon_point()'...
#if 0
			,
			"convert_lat_lon_point_to_point_on_sphere(lat_lon_point) -> PointOnSphere\n"
			"  Converts a 2D latitude/longitude point to a 3D cartesian point.\n"
			"\n"
			"  :param lat_lon_point: the 2D latitude/longitude point\n"
			"  :type lat_lon_point: :class:`LatLonPoint`\n"
			"  :rtype: :class:`PointOnSphere`\n"
			"\n"
#endif
			);

	// Non-member conversion function...
	bp::def("convert_point_on_sphere_to_lat_lon_point",
			&GPlatesMaths::make_lat_lon_point,
			(bp::arg("point"))
			// We'll keep this function (for those still using it) but we won't document it so that
			// it doesn't show up in the API documentation.
			// There are better ways to convert between LatLonPoint and PointOnSphere such as
			// 'LatLonPoint.to_point_on_sphere()' and 'PointOnSphere.to_lat_lon_point()'...
#if 0
			,
			"convert_point_on_sphere_to_lat_lon_point(point) -> LatLonPoint\n"
			"  Converts a 3D cartesian point to a 2D latitude/longitude point.\n"
			"\n"
			"  :param point: the 3D cartesian point\n"
			"  :type point: :class:`PointOnSphere`\n"
			"  :rtype: :class:`LatLonPoint`\n"
			"\n"
#endif
			);

	// Enable boost::optional<LatLonPoint> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesMaths::LatLonPoint>();
}

#endif // GPLATES_NO_PYTHON
