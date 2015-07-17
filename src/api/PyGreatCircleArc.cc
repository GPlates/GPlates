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

#include <boost/foreach.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include "PyGreatCircleArc.h"

#include "PythonConverterUtils.h"
#include "PythonHashDefVisitor.h"

#include "global/GPlatesAssert.h"
#include "global/python.h"

#include "maths/GreatCircleArc.h"
#include "maths/LatLonPoint.h"
#include "maths/PointOnSphere.h"
#include "maths/Real.h"
#include "maths/Rotation.h"
#include "maths/Vector3D.h"


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

	GPlatesMaths::Vector3D
	great_circle_arc_get_great_circle_normal(
			const GPlatesMaths::GreatCircleArc &great_circle_arc)
	{
		GPlatesGlobal::Assert<IndeterminateGreatCircleArcNormalException>(
				!great_circle_arc.is_zero_length(),
				GPLATES_ASSERTION_SOURCE);

		return GPlatesMaths::Vector3D(great_circle_arc.rotation_axis());
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

	GPlatesMaths::PointOnSphere
	great_circle_arc_get_arc_point(
			const GPlatesMaths::GreatCircleArc &great_circle_arc,
			const GPlatesMaths::Real &normalised_distance_from_start_point)
	{
		// If arc is zero length then all arc points are the same.
		if (great_circle_arc.is_zero_length())
		{
			// Start and end points are the same.
			return great_circle_arc.start_point();
		}

		if (normalised_distance_from_start_point < 0 ||
			normalised_distance_from_start_point > 1)
		{
			// Raise the 'ValueError' python exception if outside range.
			PyErr_SetString(PyExc_ValueError, "Normalised distance should be in the range [0,1]");
			bp::throw_error_already_set();
		}

		// Return exactly the start or end point if requested.
		// This avoids numerical precision differences due to rotating at 0 or 1.
		if (normalised_distance_from_start_point == 0)
		{
			return great_circle_arc.start_point();
		}
		if (normalised_distance_from_start_point == 1)
		{
			return great_circle_arc.end_point();
		}

		// Rotation from start point to requested arc point.
		const GPlatesMaths::Real angle_from_start_to_end = acos(great_circle_arc.dot_of_endpoints());
		const GPlatesMaths::Rotation rotation = GPlatesMaths::Rotation::create(
				great_circle_arc.rotation_axis(),
				normalised_distance_from_start_point * angle_from_start_to_end);

		return rotation * great_circle_arc.start_point();
	}

	GPlatesMaths::Vector3D
	great_circle_arc_get_arc_direction(
			const GPlatesMaths::GreatCircleArc &great_circle_arc,
			const GPlatesMaths::Real &normalised_distance_from_start_point)
	{
		GPlatesGlobal::Assert<IndeterminateGreatCircleArcDirectionException>(
				!great_circle_arc.is_zero_length(),
				GPLATES_ASSERTION_SOURCE);

		const GPlatesMaths::PointOnSphere arc_point =
				great_circle_arc_get_arc_point(great_circle_arc, normalised_distance_from_start_point);

		// Get unit-magnitude direction at the arc point towards the end point (from start point).
		return GPlatesMaths::Vector3D(
				cross(arc_point.position_vector(), great_circle_arc.rotation_axis())
						.get_normalisation());
	}

	bp::list
	great_circle_arc_to_tessellated(
			const GPlatesMaths::GreatCircleArc &great_circle_arc,
			const double &tessellate_radians)
	{
		std::vector<GPlatesMaths::PointOnSphere> tessellation_points;
		tessellate(tessellation_points, great_circle_arc, tessellate_radians);

		bp::list tessellation_points_list;

		BOOST_FOREACH(const GPlatesMaths::PointOnSphere &tessellation_point, tessellation_points)
		{
			tessellation_points_list.append(tessellation_point);
		}

		return tessellation_points_list;
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
					"A great-circle arc on the surface of the unit globe.\n"
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
				"  | An arc is specified by a start-point and an end-point:\n"
				"  | If these two points are not antipodal, a unique great-circle arc (with angle-span "
				"less than PI radians) will be determined between them. If they are antipodal then "
				"*IndeterminateResultException* will be raised. Note that an error is *not* raised if "
				"the two points are coincident.\n"
				"\n"
				"  ::\n"
				"\n"
				"    great_circle_arc = pygplates.GreatCircleArc(start_point, end_point)\n")
		.def("get_start_point",
				&GPlatesMaths::GreatCircleArc::start_point,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_start_point()\n"
				"  Return the arc's start point geometry.\n"
				"\n"
				"  :rtype: :class:`PointOnSphere`\n")
		.def("get_end_point",
				&GPlatesMaths::GreatCircleArc::end_point,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_end_point()\n"
				"  Return the arc's end point geometry.\n"
				"\n"
				"  :rtype: :class:`PointOnSphere`\n")
		.def("is_zero_length",
				&GPlatesMaths::GreatCircleArc::is_zero_length,
				"is_zero_length()\n"
				"  Return whether this great circle arc is of zero length.\n"
				"\n"
				"  :rtype: bool\n"
				"\n"
				"  If this arc is of zero length, it will not have a determinate rotation axis "
				"and a call to :meth:`get_rotation_axis` will raise an error.\n")
		.def("get_arc_length",
				&GPlatesApi::great_circle_arc_get_arc_length,
				"get_arc_length()\n"
				"  Returns the arc length of this great circle arc (in radians).\n"
				"\n"
				"  :rtype: float\n"
				"\n"
				"  To convert to distance, multiply the result by the Earth radius (see :class:`Earth`).\n")
		.def("get_great_circle_normal",
				&GPlatesApi::great_circle_arc_get_great_circle_normal,
				"get_great_circle_normal()\n"
				"  Return the unit vector normal direction of the great circle this arc lies on.\n"
				"\n"
				"  :returns: the unit-length 3D vector\n"
				"  :rtype: :class:`Vector3D`\n"
				"  :raises: IndeterminateGreatCircleArcNormalError if arc is zero length\n"
				"\n"
				"  ::\n"
				"\n"
				"    if not arc.is_zero_length():\n"
				"        normal = arc.get_great_circle_normal()\n"
				"\n"
				"  .. note:: This returns the same (x, y, z) result as :meth:`get_rotation_axis`, "
				"but in the form of a :class:`Vector3D` instead of an (x, y, z) tuple.\n"
				"\n"
				"  .. note:: The normal to the great circle can be considered to be the tangential "
				"direction (to the Earth's surface) at any point along the great circle arc that is most "
				"pointing away from (perpendicular to) the direction of the arc (from start point "
				"to end point - see :meth:`get_arc_direction`).\n"
				"\n"
				"  The normal vector is the same direction as the :meth:`cross product<Vector3D.cross>` "
				"of the start point and the end point. In fact it is equivalent to "
				"``pygplates.Vector3D.cross(arc.start_point().to_xyz(), arc.end_point().to_xyz()).to_normalised()``.\n"
				"\n"
				"  If the arc start and end points are the same (if :meth:`is_zero_length` is ``True``) "
				"then *IndeterminateGreatCircleArcNormalError* is raised.\n"
				"\n"
				"  .. seealso:: :meth:`get_rotation_axis`\n")
		.def("get_rotation_axis",
				&GPlatesApi::great_circle_arc_get_rotation_axis,
				"get_rotation_axis()\n"
				"  Return the rotation axis of the arc as a 3D vector.\n"
				"\n"
				"  :returns: the unit-length 3D vector (x,y,z)\n"
				"  :rtype: the tuple (float, float, float)\n"
				"  :raises: IndeterminateArcRotationAxisError if arc is zero length\n"
				"\n"
				"  ::\n"
				"\n"
				"    if not arc.is_zero_length():\n"
				"        axis_x, axis_y, axis_z = arc.get_rotation_axis()\n"
				"\n"
				"  .. note:: This returns the same (x, y, z) result as :meth:`get_great_circle_normal`, "
				"but in the form of an (x, y, z) tuple instead of a :class:`Vector3D`.\n"
				"\n"
				"  The rotation axis is the unit-length 3D vector (x,y,z) returned in the tuple.\n"
				"\n"
				"  The rotation axis direction is such that it rotates the start point towards the "
				"end point along the arc (assuming a right-handed coordinate system).\n"
				"\n"
				"  If the arc start and end points are the same (if :meth:`is_zero_length` is ``True``) "
				"then *IndeterminateArcRotationAxisError* is raised.\n"
				"\n"
				"  .. seealso:: :meth:`get_great_circle_normal`\n")
		.def("get_rotation_axis_lat_lon",
				&GPlatesApi::great_circle_arc_get_rotation_axis_lat_lon,
				"get_rotation_axis_lat_lon()\n"
				"  Return the (latitude, longitude) equivalent of :meth:`get_rotation_axis`.\n"
				"\n"
				"  :returns: the axis as (latitude, longitude)\n"
				"  :rtype: the tuple (float, float)\n"
				"  :raises: IndeterminateArcRotationAxisError if arc is zero length\n"
				"\n"
				"  ::\n"
				"\n"
				"    if not arc.is_zero_length():\n"
				"        axis_lat, axis_lon = arc.get_rotation_axis_lat_lon()\n"
				"\n"
				"  The rotation axis is the (latitude, longitude) returned in the tuple.\n"
				"\n"
				"  The rotation axis direction is such that it rotates the start point towards the "
				"end point along the arc (assuming a right-handed coordinate system).\n"
				"\n"
				"  If the arc start and end points are the same (if :meth:`is_zero_length` is ``True``) "
				"then *IndeterminateArcRotationAxisError* is raised.\n")
		.def("get_arc_point",
				&GPlatesApi::great_circle_arc_get_arc_point,
				"get_arc_point(normalised_distance_from_start_point)\n"
				"  Return a point on this arc.\n"
				"\n"
				"  :param normalised_distance_from_start_point: distance from start point where "
				"zero is the start point, one is the end point and between zero and one are points "
				"along the arc\n"
				"  :type normalised_distance_from_start_point: float\n"
				"  :rtype: :class:`PointOnSphere`\n"
				"  :raises: ValueError if arc *normalised_distance_from_start_point* is not in the "
				"range [0,1]\n"
				"\n"
				"  The midpoint of an arc:\n"
				"  ::\n"
				"\n"
				"    arc_midpoint = arc.get_arc_point(0.5)\n"
				"\n"
				"  If *normalised_distance_from_start_point* is zero then the start point is returned. "
				"If *normalised_distance_from_start_point* is one then the end point is returned. "
				"Values of *normalised_distance_from_start_point* between zero and one return points on the arc. "
				"If *normalised_distance_from_start_point* is outside the range from zero to one then "
				"then *ValueError* is raised.\n")
		.def("get_arc_direction",
				&GPlatesApi::great_circle_arc_get_arc_direction,
				"get_arc_direction(normalised_distance_from_start_point)\n"
				"  Return the direction along the arc at a point on the arc.\n"
				"\n"
				"  :param normalised_distance_from_start_point: distance from start point where "
				"zero is the start point, one is the end point and between zero and one are points "
				"along the arc\n"
				"  :type normalised_distance_from_start_point: float\n"
				"  :rtype: :class:`Vector3D`\n"
				"  :raises: ValueError if arc *normalised_distance_from_start_point* is not in the "
				"range [0,1]\n"
				"  :raises: IndeterminateGreatCircleArcDirectionError if arc is zero length\n"
				"\n"
				"  The returned direction is tangential to the Earth's surface and is aligned with "
				"the direction of the great circle arc (in the direction going from the start point "
				"towards the end point). This direction is perpendicular to the great circle normal "
				"direction (see :meth:`get_great_circle_normal`).\n"
				"\n"
				"  The direction at the midpoint of an arc:\n"
				"  ::\n"
				"\n"
				"    if not arc.is_zero_length():\n"
				"        arc_midpoint_direction = arc.get_arc_direction(0.5)\n"
				"\n"
				"  If *normalised_distance_from_start_point* is zero then the direction at start point is returned. "
				"If *normalised_distance_from_start_point* is one then the direction at end point is returned. "
				"Values of *normalised_distance_from_start_point* between zero and one return directions at points on the arc. "
				"If *normalised_distance_from_start_point* is outside the range from zero to one then "
				"then *ValueError* is raised.\n")
		.def("to_tessellated",
				&GPlatesApi::great_circle_arc_to_tessellated,
				(bp::arg("tessellate_radians")),
				"to_tessellated(tessellate_radians)\n"
				"  Returns a list of :class:`points<PointOnSphere>` new polyline that is tessellated version of this polyline.\n"
				"\n"
				"  :param tessellate_radians: maximum tessellation angle (in radians)\n"
				"  :type tessellate_radians: float\n"
				"  :rtype: list :class:`points<PointOnSphere>`\n"
				"\n"
				"  Adjacent points (in the returned list of points) are separated by no more than "
				"*tessellate_radians* on the globe.\n"
				"\n"
				"  Tessellate a great circle arc to 2 degrees:\n"
				"  ::\n"
				"\n"
				"    tessellation_points = great_circle_arc.to_tessellated(math.radians(2))\n"
				"\n"
				"  .. note:: Since a *GreatCircleArc* is immutable it cannot be modified. Which is why a "
				"tessellated list of *PointOnSphere* is returned.\n"
				"\n"
				"  .. seealso:: :meth:`PolylineOnSphere.to_tessellated` and :meth:`PolygonOnSphere.to_tessellated`\n")
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
