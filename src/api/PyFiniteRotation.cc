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

	GPlatesMaths::FiniteRotation
	finite_rotation_interpolate(
			const GPlatesMaths::FiniteRotation &finite_rotation1,
			const GPlatesMaths::FiniteRotation &finite_rotation2,
			const GPlatesMaths::Real &time1,
			const GPlatesMaths::Real &time2,
			const GPlatesMaths::Real &target_time)
	{
		// If either of the finite rotations has an axis hint, use it.
		boost::optional<GPlatesMaths::UnitVector3D> axis_hint;
		if (finite_rotation1.axis_hint())
		{
			axis_hint = finite_rotation1.axis_hint();
		}
		else if (finite_rotation2.axis_hint())
		{
			axis_hint = finite_rotation2.axis_hint();
		}

		return GPlatesMaths::interpolate(
				finite_rotation1, finite_rotation2,
				time1, time2,
				target_time,
				axis_hint);
	}
}

void
export_finite_rotation()
{
	// Select correct 'compose' overload of FiniteRotation...
	const GPlatesMaths::FiniteRotation
			(*compose)(
					const GPlatesMaths::FiniteRotation &,
					const GPlatesMaths::FiniteRotation &) =
							&GPlatesMaths::compose;

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
					"Finite rotations are equality (``==``, ``!=``) comparable.\n"
					"\n"
					"Multiplication operations can be used to rotate various geometry types:\n"
					"\n"
					"=========================== =======================================================================\n"
					"Operation                    Result\n"
					"=========================== =======================================================================\n"
					"``fr * point``               Rotates :class:`PointOnSphere` *point* using finite rotation *fr*\n"
					"``fr * multi_point``         Rotates :class:`MultiPointOnSphere` *multi_point* using finite rotation *fr*\n"
					"``fr * polyline``            Rotates :class:`PolylineOnSphere` *polyline* using finite rotation *fr*\n"
					"``fr * polygon``             Rotates :class:`PolygonOnSphere` *polygon* using finite rotation *fr*\n"
					"``fr * great_circle_arc``    Rotates :class:`GreatCircleArc` *great_circle_arc* using finite rotation *fr*\n"
					"``fr * unit_vector_3d``      Rotates :class:`UnitVector3D` *unit_vector_3d* using finite rotation *fr*\n"
					"=========================== =======================================================================\n"
					"\n"
					"For example, the rotation of a :class:`PolylineOnSphere`:\n"
					"::\n"
					"\n"
					"  polyline = pygplates.PolylineOnSphere(...)\n"
					"  finite_rotation = pygplates.FiniteRotation(pole, angle)\n"
					"  rotated_polyline = finite_rotation * polyline\n"
					"\n"
					"Two finite rotations can be interpolated using the :func:`interpolate_finite_rotations` function:\n"
					"::\n"
					"\n"
					"  interpolated_rotation = pygplates.interpolate_finite_rotations("
					"finite_rotation1, finite_rotation2, time1, time2, target_time)\n"
					"\n"
					"Two finite rotations can be composed in either of the following equivalent ways:\n"
					"\n"
					"* ``composed_finite_rotation = finite_rotation1 * finite_rotation2``\n"
					"* ``composed_finite_rotation = pygplates.compose_finite_rotations("
					"finite_rotation1, finite_rotation2)``\n"
					"\n"
					"The latter technique uses the :func:`compose_finite_rotations` function.\n"
					"\n"
					"**The following is general information on composing finite rotations in various "
					"plate tectonic scenarios**...\n"
					"\n"
					"Rotation from present day (0Ma) to time 't2' (via time 't1'):\n"
					"\n"
					"|  R(0->t2)  = R(t1->t2) * R(0->t1)\n"
					"\n"
					"...or by post-multiplying both sides by R(t1->0), and then swapping sides, this becomes...\n"
					"\n"
					"|  R(t1->t2) = R(0->t2) * R(t1->0)\n"
					"\n"
					"Rotation from anchor plate 'A' to moving plate 'M' (via fixed plate 'F'):\n"
					"\n"
					"|  R(A->M) = R(A->F) * R(F->M)\n"
					"\n"
					"...or by pre-multiplying both sides by R(F->A) this becomes...\n"
					"\n"
					"|  R(F->M) = R(F->A) * R(A->M)\n"
					"\n"
					"NOTE: The rotations for relative times and for relative plates have the opposite order of each other !\n"
					"\n"
					"In other words:\n"
					"\n"
					"* For times 0->t1->t2 you apply the '0->t1' rotation first followed by the 't1->t2' rotation:\n"
					"\n"
					"  | R(0->t2)  = R(t1->t2) * R(0->t1)\n"
					"\n"
					"* For plate circuit A->F->M you apply the 'F->M' rotation first followed by the 'A->F' rotation:\n"
					"\n"
					"  | R(A->M) = R(A->F) * R(F->M)\n"
					"\n"
					"  Note that this is not 'A->F' followed by 'F->M' as you might expect (looking at the time example).\n"
					"\n"
					"This is probably best explained by the difference between thinking in terms of the grand fixed "
					"coordinate system and local coordinate system (see http://glprogramming.com/red/chapter03.html#name2). "
					"Essentially, in the plate circuit A->F->M, the 'F->M' rotation can be thought of as a rotation "
					"within the local coordinate system of 'A->F'. In other words 'F->M' is not a rotation that "
					"occurs relative to the global spin axis but a rotation relative to the local coordinate system "
					"of plate 'F' *after* it has been rotated relative to the anchor plate 'A'.\n"
					"\n"
					"For the times 0->t1->t2 this local/relative coordinate system concept does not apply.\n"
					"\n"
					"NOTE: A rotation must be relative to present day (0Ma) before it can be separated into "
					"a (plate circuit) chain of moving/fixed plate pairs.\n"
					"For example, the following is correct...\n"
					"\n"
					"|  R(t1->t2,A->C)\n"
					"|     = R(0->t2,A->C) * R(t1->0,A->C)\n"
					"|     = R(0->t2,A->C) * inverse[R(0->t1,A->C)]\n"
					"|     // Now that all times are relative to 0Ma we can split A->C into A->B->C...\n"
					"|     = R(0->t2,A->B) * R(0->t2,B->C) * inverse[R(0->t1,A->B) * R(0->t1,B->C)]\n"
					"|     = R(0->t2,A->B) * R(0->t2,B->C) * inverse[R(0->t1,B->C)] * inverse[R(0->t1,A->B)]\n"
					"\n"
					"...but the following is *incorrect*...\n"
					"\n"
					"|  R(t1->t2,A->C)\n"
					"|     = R(t1->t2,A->B) * R(t1->t2,B->C)   // <-- This line is *incorrect*\n"
					"|     = R(0->t2,A->B) * R(t1->0,A->B) * R(0->t2,B->C) * R(t1->0,B->C)\n"
					"|     = R(0->t2,A->B) * inverse[R(0->t1,A->B)] * R(0->t2,B->C) * inverse[R(0->t1,B->C)]\n"
					"\n"
					"...as can be seen above this gives two different results - the same four rotations are "
					"present in each result but in a different order.\n"
					"\n"
					"A->B->C means B->C is the rotation of C relative to B and A->B is the rotation of B relative to A. "
					"The need for rotation A->C to be relative to present day (0Ma) before it can be split into "
					"A->B and B->C is because A->B and B->C are defined (in the rotation file) as total reconstruction "
					"poles which are always relative to present day.\n"
					"\n"
					"\n"
					"An example that combines all this is the stage rotation of a moving plate relative "
					"to a fixed plate and from time 't1' to time 't2':\n"
					"\n"
					"| R(t1->t2,F->M)\n"
					"|   = R(0->t2,F->M) * R(t1->0,F->M)\n"
					"|   = R(0->t2,F->M) * inverse[R(0->t1,F->M)]\n"
					"|   = R(0->t2,F->A) * R(0->t2,A->M) * inverse[R(0->t1,F->A) * R(0->t1,A->M)]\n"
					"|   = inverse[R(0->t2,A->F)] * R(0->t2,A->M) * inverse[inverse[R(0->t1,A->F)] * R(0->t1,A->M)]\n"
					"|   = inverse[R(0->t2,A->F)] * R(0->t2,A->M) * inverse[R(0->t1,A->M)] * R(0->t1,A->F)\n"
					"\n"
					"...where 'A' is the anchor plate, 'F' is the fixed plate and 'M' is the moving plate.\n"
					"\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::finite_rotation_create,
						bp::default_call_policies(),
						(bp::arg("pole"), bp::arg("angle"))),
				"__init__(pole, angle)\n"
				"  Create a finite rotation from an Euler pole and a rotation angle (in *radians*).\n"
				"  ::\n"
				"\n"
				"    finite_rotation = pygplates.FiniteRotation(pole, angle)\n"
				"\n"
				"  :param pole: the Euler pole.\n"
				"  :type pole: :class:`PointOnSphere`\n"
				"  :param angle: the rotation angle (in *radians*).\n"
				"  :type angle: float\n")
		.def("get_inverse",
				&GPlatesMaths::get_reverse,
				"get_inverse() -> FiniteRotation\n"
				"  Return the inverse of this finite rotation.\n"
				"\n"
				"  :rtype: :class:`FiniteRotation`\n"
				"\n"
				"  The inverse represents the reverse rotation as the following code demonstrates:\n"
				"  ::\n"
				"\n"
				"    rotated_point = finite_rotation * point\n"
				"    original_point = finite_rotation.get_inverse() * rotated_point\n")
		.def("get_unit_quaternion",
				&GPlatesMaths::FiniteRotation::unit_quat,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_unit_quaternion() -> UnitQuaternion3D\n"
				"  Return the internal unit quaternion which effects the rotation of this finite rotation.\n"
				"\n"
				"  :rtype: :class:`UnitQuaternion3D`\n")
		// Multiply two finite rotations...
		.def("__mul__", compose)
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

	// Non-member conversion function...
	bp::def("compose_finite_rotations",
			compose,
			(bp::arg("finite_rotation1"), bp::arg("finite_rotation2")),
			"compose_finite_rotations(finite_rotation1, finite_rotation2) -> FiniteRotation\n"
			"  Composes two finite rotations and returns the composed finite rotation.\n"
			"\n"
			"  :param finite_rotation1: the left-hand-side finite rotation\n"
			"  :type finite_rotation1: :class:`FiniteRotation`\n"
			"  :param finite_rotation2: the right-hand-side finite rotation\n"
			"  :type finite_rotation2: :class:`FiniteRotation`\n"
			"  :rtype: :class:`FiniteRotation`\n"
			"\n"
			"  This method does the same as ``finite_rotation1 * finite_rotation2``.\n"
			"\n"
			"  See :class:`FiniteRotation` for more details on composing finite rotations.\n");

	// Non-member interpolation function...
	bp::def("interpolate_finite_rotations",
			&GPlatesApi::finite_rotation_interpolate,
			(bp::arg("finite_rotation1"),
				bp::arg("finite_rotation2"),
				bp::arg("time1"),
				bp::arg("time2"),
				bp::arg("target_time")),
			"interpolate_finite_rotations(finite_rotation1, finite_rotation2, time1, time2, target_time) -> FiniteRotation\n"
			"  Calculate the finite rotation which is the interpolation of two finite rotations. "
			"The finite rotations *finite_rotation1* and *finite_rotation2* are associated with "
			"times *time1* and *time2*, respectively. The result of the interpolation is associated "
			"with *target_time*. The interpolated finite rotation is generated using Spherical Linear "
			"intERPolation (SLERP) with the interpolation factor ``(target_time - time1) / (time2 - time1)``.\n"
			"\n"
			"  :param finite_rotation1: the left-hand-side finite rotation\n"
			"  :type finite_rotation1: :class:`FiniteRotation`\n"
			"  :param finite_rotation2: the right-hand-side finite rotation\n"
			"  :type finite_rotation2: :class:`FiniteRotation`\n"
			"  :param time1: the time associated with the left-hand-side finite rotation\n"
			"  :type time1: float\n"
			"  :param time2: the time associated with the right-hand-side finite rotation\n"
			"  :type time2: float\n"
			"  :param target_time: the time associated with the result of the interpolation\n"
			"  :type target_time: float\n"
			"  :rtype: :class:`FiniteRotation`\n"
			"  :raises: IndeterminateResultException if *time1* and *time2* are equal\n"
			"\n"
			"  *target_time* can be any time - it does not have to be between *time1* and *time2*.\n"
			"  ::\n"
			"\n"
			"    interpolated_rotation = pygplates.interpolate_finite_rotations("
			"finite_rotation1, finite_rotation2, time1, time2, target_time)\n");

	// Enable boost::optional<FiniteRotation> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::python_optional<GPlatesMaths::FiniteRotation>();
}

#endif // GPLATES_NO_PYTHON
