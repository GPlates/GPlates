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
#include <boost/shared_ptr.hpp>

#include "PythonConverterUtils.h"

#include "global/python.h"

#include "maths/FiniteRotation.h"
#include "maths/GeometryOnSphere.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/UnitQuaternion3D.h"
#include "maths/UnitVector3D.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	boost::shared_ptr<GPlatesMaths::FiniteRotation>
	finite_rotation_create(
			const GPlatesMaths::PointOnSphere &pole,
			const GPlatesMaths::Real &angle)
	{
		return boost::shared_ptr<GPlatesMaths::FiniteRotation>(
				new GPlatesMaths::FiniteRotation(
						GPlatesMaths::FiniteRotation::create(pole, angle)));
	}

	bool
	finite_rotation_represents_identity_rotation(
			const GPlatesMaths::FiniteRotation &finite_rotation)
	{
		return represents_identity_rotation(finite_rotation.unit_quat());
	}
}

void
export_finite_rotation()
{
	//
	// FiniteRotation - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	// FiniteRotation is immutable (contains no mutable methods) hence we can copy it into python
	// wrapper objects without worrying that modifications from the C++ will not be visible to the
	// python side and vice versa.
	//
	bp::class_<
			GPlatesMaths::FiniteRotation,
			// A pointer holder is required by 'bp::make_constructor'...
			boost::shared_ptr<GPlatesMaths::FiniteRotation>
			// Since it's immutable it can be copied without worrying that a modification from the
			// C++ side will not be visible on the python side, or vice versa. It needs to be
			// copyable anyway so that boost-python can copy it into a shared holder pointer...
#if 0
			boost::noncopyable
#endif
			>(
					"FiniteRotation",
					"Represents the motion of plates on the surface of the globe.\n"
					"\n"
					"A finite rotation is a rotation about an *Euler pole* (a point on the surface of the \n"
					"globe, which is the intersection point of a rotation vector (the semi-axis of rotation) \n"
					"which extends from the centre of the globe), by an angular distance. \n"
					"\n"
					"An Euler pole is specified by a point on the surface of the globe. \n"
					"\n"
					"A rotation angle is specified in radians, with the usual sense of rotation:\n"
					"\n"
					"* a positive angle represents an anti-clockwise rotation around the rotation vector,\n"
					"* a negative angle corresponds to a clockwise rotation.\n"
					"\n"
					"The multiplication operations can be used to rotate various geometry types including:\n"
					"\n"
					"* :class:`PointOnSphere`\n"
					"* :class:`MultiPointOnSphere`\n"
					"* :class:`PolylineOnSphere`\n"
					"* :class:`PolygonOnSphere`\n"
					"* :class:`GreatCircleArc`\n"
					"* :class:`UnitVector3D`\n"
					"\n"
					"For example, the rotation of a :class:`PointOnSphere`:\n"
					"  ::\n"
					"\n"
					"    point = pygplates.PointOnSphere(...)\n"
					"    finite_rotation = pygplates.FiniteRotation(pole, angle)\n"
					"    rotated_point = finite_rotation * point\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::finite_rotation_create,
						bp::default_call_policies(),
						(bp::arg("pole"), bp::arg("angle"))),
				"__init__(pole, angle)\n"
				"  Create a finite rotation from an Euler pole and a rotation angle (in *radians*). "
				"Finite rotations are equality (``==``, ``!=``) comparable.\n"
				"  ::\n"
				"\n"
				"    finite_rotation = pygplates.FiniteRotation(pole, angle)\n"
				"\n"
				"  :param pole: the Euler pole.\n"
				"  :type pole: :class:`PointOnSphere`\n"
				"  :param angle: the rotation angle (in *radians*).\n"
				"  :type angle: float\n")
		.def("get_unit_quaternion",
				&GPlatesMaths::FiniteRotation::unit_quat,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_unit_quaternion() -> UnitQuaternion3D\n"
				"  Return the internal unit quaternion which effects the rotation of this finite rotation.\n"
				"\n"
				"  :rtype: :class:`UnitQuaternion3D`\n")
		// Rotations...
		.def(bp::self * bp::other<GPlatesMaths::UnitVector3D>())
		.def(bp::self * bp::other<GPlatesMaths::GreatCircleArc>())
		.def(bp::self * bp::other<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>())
		.def(bp::self * bp::other<GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type>())
		.def(bp::self * bp::other<GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type>())
		.def(bp::self * bp::other<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>())
		.def(bp::self * bp::other<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type>())
		// Comparison operators...
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
		// Generate '__str__' from 'operator<<'...
		// Note: Seems we need to qualify with 'self_ns::' to avoid MSVC compile error.
		.def(bp::self_ns::str(bp::self))
	;

	// Enable boost::optional<FiniteRotation> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::python_optional<GPlatesMaths::FiniteRotation>();
}

#endif // GPLATES_NO_PYTHON
