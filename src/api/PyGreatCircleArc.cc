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
#include <boost/shared_ptr.hpp>

#include "PythonConverterUtils.h"
#include "PythonHashDefVisitor.h"

#include "global/python.h"

#include "maths/GreatCircleArc.h"
#include "maths/LatLonPoint.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	boost::shared_ptr<GPlatesMaths::GreatCircleArc>
	great_circle_arc_create(
			// There are from-python converters from LatLonPoint and sequence(latitude,longitude) and
			// sequence(x,y,z) to PointOnSphere so they will also get matched by this...
			const GPlatesMaths::PointOnSphere &start_point,
			const GPlatesMaths::PointOnSphere &end_point)
	{
		return boost::shared_ptr<GPlatesMaths::GreatCircleArc>(
				new GPlatesMaths::GreatCircleArc(
						GPlatesMaths::GreatCircleArc::create(start_point, end_point)));
	}

	GPlatesMaths::real_t
	great_circle_arc_get_arc_length(
			const GPlatesMaths::GreatCircleArc &great_circle_arc)
	{
		return acos(great_circle_arc.dot_of_endpoints());
	}

	bp::tuple
	great_circle_arc_get_rotation_axis(
			const GPlatesMaths::GreatCircleArc &great_circle_arc)
	{
		const GPlatesMaths::UnitVector3D &axis = great_circle_arc.rotation_axis();

		return bp::make_tuple(axis.x(), axis.y(), axis.z());
	}

	bp::tuple
	great_circle_arc_get_rotation_axis_lat_lon(
			const GPlatesMaths::GreatCircleArc &great_circle_arc)
	{
		const GPlatesMaths::UnitVector3D &axis = great_circle_arc.rotation_axis();

		const GPlatesMaths::LatLonPoint axis_lat_lon =
				GPlatesMaths::make_lat_lon_point(GPlatesMaths::PointOnSphere(axis));

		return bp::make_tuple(axis_lat_lon.latitude(), axis_lat_lon.longitude());
	}
}

void
export_great_circle_arc()
{
	//
	// GreatCircleArc - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	// GreatCircleArc is immutable (contains no mutable methods) hence we can copy it into python
	// wrapper objects without worrying that modifications from the C++ will not be visible to the
	// python side and vice versa.
	//
	bp::class_<
			GPlatesMaths::GreatCircleArc,
			// A pointer holder is required by 'bp::make_constructor'...
			boost::shared_ptr<GPlatesMaths::GreatCircleArc>
			// Since it's immutable it can be copied without worrying that a modification from the
			// C++ side will not be visible on the python side, or vice versa. It needs to be
			// copyable anyway so that boost-python can copy it into a shared holder pointer...
#if 0
			boost::noncopyable
#endif
			>(
					"GreatCircleArc",
					"A great-circle arc on the surface of the unit sphere.\n"
					"\n"
					"Great circle arcs are equality (``==``, ``!=``) comparable (but not hashable "
					"- cannot be used as a key in a ``dict``).\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::great_circle_arc_create,
						bp::default_call_policies(),
						(bp::arg("start_point"), bp::arg("end_point"))),
				"__init__(start_point, end_point)\n"
				"  Create a great circle arc from two points.\n"
				"\n"
				"  :param start_point: the start point of the arc.\n"
				"  :type start_point: :class:`PointOnSphere` or :class:`LatLonPoint` or tuple (latitude,longitude)"
				", in degrees, or tuple (x,y,z)\n"
				"  :param end_point: the end point of the arc.\n"
				"  :type end_point: :class:`PointOnSphere` or :class:`LatLonPoint` or tuple (latitude,longitude)"
				", in degrees, or tuple (x,y,z)\n"
				"  :raises: IndeterminateResultError if points are antipodal (opposite each other)\n"
				"\n"
				"  An arc is specified by a start-point and an end-point:  If these two points are not "
				"antipodal, a unique great-circle arc (with angle-span less than PI radians) will be "
				"determined between them. If they are antipodal then *IndeterminateResultException* will be raised. "
				"Note that an error is *not* raised if the two points are coincident.\n"
				"  ::\n"
				"\n"
				"    great_circle_arc = pygplates.GreatCircleArc(start_point, end_point)\n")
		.def("get_start_point",
				&GPlatesMaths::GreatCircleArc::start_point,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_start_point() -> PointOnSphere\n"
				"  Return the arc's start point geometry.\n"
				"\n"
				"  :rtype: :class:`PointOnSphere`\n")
		.def("get_end_point",
				&GPlatesMaths::GreatCircleArc::end_point,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_end_point() -> PointOnSphere\n"
				"  Return the arc's end point geometry.\n"
				"\n"
				"  :rtype: :class:`PointOnSphere`\n")
		.def("is_zero_length",
				&GPlatesMaths::GreatCircleArc::is_zero_length,
				"is_zero_length() -> bool\n"
				"  Return whether this great circle arc is of zero length.\n"
				"\n"
				"  :rtype: bool\n"
				"\n"
				"  If this arc is of zero length, it will not have a determinate rotation axis "
				"and a call to :meth:`get_rotation_axis` will raise an error.\n")
		.def("get_arc_length",
				&GPlatesApi::great_circle_arc_get_arc_length,
				"get_arc_length() -> float\n"
				"  Returns the arc length of this great circle arc (in radians).\n"
				"\n"
				"  :rtype: float\n"
				"\n"
				"  To convert to distance, multiply the result by the Earth radius.\n")
		.def("get_rotation_axis",
				&GPlatesApi::great_circle_arc_get_rotation_axis,
				"get_rotation_axis() -> x, y, z\n"
				"  Return the rotation axis of the arc as a 3D vector.\n"
				"\n"
				"  :returns: the unit-length 3D vector (x,y,z)\n"
				"  :rtype: the tuple (float, float, float)\n"
				"  :raises: IndeterminateArcRotationAxisError if arc is zero length\n"
				"\n"
				"  The rotation axis is the unit-length 3D vector (x,y,z) returned in the tuple.\n"
				"\n"
				"  If the arc start and end points are the same (if :meth:`is_zero_length` is ``True``) "
				"then *IndeterminateArcRotationAxisError* is raised.\n")
		.def("get_rotation_axis_lat_lon",
				&GPlatesApi::great_circle_arc_get_rotation_axis_lat_lon,
				"get_rotation_axis_lat_lon() -> latitude, longitude\n"
				"  Return the (latitude, longitude) equivalent of :meth:`get_rotation_axis`.\n"
				"\n"
				"  :returns: the axis as (latitude, longitude)\n"
				"  :rtype: the tuple (float, float)\n"
				"  :raises: IndeterminateArcRotationAxisError if arc is zero length\n"
				"\n"
				"  The rotation axis is the (latitude, longitude) returned in the tuple.\n"
				"\n"
				"  If the arc start and end points are the same (if :meth:`is_zero_length` is ``True``) "
				"then *IndeterminateArcRotationAxisError* is raised.\n")
		// Due to the numerical tolerance in comparisons we cannot make hashable.
		// Make unhashable, with no *equality* comparison operators (we explicitly define them)...
		.def(GPlatesApi::NoHashDefVisitor(false, true))
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
	;

	// Enable boost::optional<GreatCircleArc> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesMaths::GreatCircleArc>();
}

#endif // GPLATES_NO_PYTHON
