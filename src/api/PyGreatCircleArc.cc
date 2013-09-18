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

#include "PythonConverterUtils.h"

#include "global/python.h"

#include "maths/GreatCircleArc.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	boost::optional<GPlatesMaths::UnitVector3D>
	great_circle_arc_get_rotation_axis(
			const GPlatesMaths::GreatCircleArc &great_circle_arc)
	{
		if (great_circle_arc.is_zero_length())
		{
			return boost::none;
		}

		return great_circle_arc.rotation_axis();
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
			GPlatesMaths::GreatCircleArc>(
					"GreatCircleArc",
					"A great-circle arc on the surface of the unit sphere. "
					"Great circle arcs are equality (``==``, ``!=``) comparable.\n",
					bp::no_init)
		.def("create",
				&GPlatesMaths::GreatCircleArc::create,
				(bp::arg("start_point"), bp::arg("end_point")),
				"create(start_point, end_point) -> GreatCircleArc\n"
				"  Create a great circle arc from two points.\n"
				"\n"
				"  An arc is specified by a start-point and an end-point:  If these two points are not "
				"antipodal, a unique great-circle arc (with angle-span less than PI radians) will be "
				"determined between them. If they are antipodal then *RuntimeError* will be raised. "
				"Note that an error is *not* raised if the two points are coincident.\n"
				"  ::\n"
				"\n"
				"    great_circle_arc = pygplates.GreatCircleArc.create(start_point, end_point)\n"
				"\n"
				"  :param start_point: the start point of the arc.\n"
				"  :type start_point: :class:`PointOnSphere`\n"
				"  :param end_point: the end point of the arc.\n"
				"  :type end_point: :class:`PointOnSphere`\n")
		.staticmethod("create")
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
				"  If this arc is of zero length, it will not have a determinate rotation axis "
				"and a call to :meth:`get_rotation_axis` will return ``None``.\n"
				"\n"
				"  :rtype: bool\n")
		.def("get_rotation_axis",
				&GPlatesApi::great_circle_arc_get_rotation_axis,
				"get_rotation_axis() -> UnitVector3D\n"
				"  Return the rotation axis of the arc, or ``None`` if the arc start and end points "
				"are the same (if :meth:`is_zero_length` is ``True``).\n"
				"\n"
				"  :rtype: :class:`UnitVector3D` or None\n")
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
	;

	// Enable boost::optional<GreatCircleArc> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::python_optional<GPlatesMaths::GreatCircleArc>();
}

#endif // GPLATES_NO_PYTHON
