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

#include "PyInterpolationException.h"
#include "PythonConverterUtils.h"
#include "PythonHashDefVisitor.h"

#include "global/python.h"

#include "maths/FiniteRotation.h"
#include "maths/GeometryOnSphere.h"
#include "maths/LatLonPoint.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/UnitQuaternion3D.h"
#include "maths/UnitVector3D.h"

#include "property-values/GeoTimeInstant.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	boost::shared_ptr<GPlatesMaths::FiniteRotation>
	finite_rotation_create(
			// There are from-python converters from LatLonPoint and sequence(latitude,longitude) and
			// sequence(x,y,z) to PointOnSphere so they will also get matched by this...
			const GPlatesMaths::PointOnSphere &pole,
			const GPlatesMaths::Real &angle_radians)
	{
		return boost::shared_ptr<GPlatesMaths::FiniteRotation>(
				new GPlatesMaths::FiniteRotation(
						GPlatesMaths::FiniteRotation::create(pole, angle_radians)));
	}

	boost::shared_ptr<GPlatesMaths::FiniteRotation>
	finite_rotation_create_identity_rotation()
	{
		return boost::shared_ptr<GPlatesMaths::FiniteRotation>(
				new GPlatesMaths::FiniteRotation(
						GPlatesMaths::FiniteRotation::create(
								GPlatesMaths::UnitQuaternion3D::create_identity_rotation(),
								boost::none)));
	}

	GPlatesMaths::FiniteRotation
	finite_rotation_interpolate(
			const GPlatesMaths::FiniteRotation &finite_rotation1,
			const GPlatesMaths::FiniteRotation &finite_rotation2,
			// NOTE: Using the 'GPlatesPropertyValues::GeoTimeInstant' enables conversions
			// both from python 'float' and python 'GeoTimeInstant' (see PyGeoTimeInstant.cc)...
			const GPlatesPropertyValues::GeoTimeInstant &time1,
			const GPlatesPropertyValues::GeoTimeInstant &time2,
			const GPlatesPropertyValues::GeoTimeInstant &target_time)
	{
		// We can't interpolate if any time is distant past/future.
		GPlatesGlobal::Assert<InterpolationException>(
				time1.is_real() && time2.is_real() && target_time.is_real(),
				GPLATES_ASSERTION_SOURCE,
				"Interpolation time values cannot be distant-past (float('inf')) or distant-future (float('-inf')).");

		// We can't interpolate if both times are equal.
		if (time1 == time2)
		{
			return finite_rotation1;
		}

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
				time1.value(), time2.value(),
				target_time.value(),
				axis_hint);
	}

	bool
	finite_rotation_represents_identity_rotation(
			const GPlatesMaths::FiniteRotation &finite_rotation)
	{
		return GPlatesMaths::represents_identity_rotation(finite_rotation.unit_quat());
	}

	bool
	finite_rotation_represent_equivalent_rotations(
			const GPlatesMaths::FiniteRotation &finite_rotation1,
			const GPlatesMaths::FiniteRotation &finite_rotation2)
	{
		return GPlatesMaths::represent_equiv_rotations(
				finite_rotation1.unit_quat(),
				finite_rotation2.unit_quat());
	}

	// Return the Euler (pole, angle_radians) tuple.
	bp::tuple
	finite_rotation_get_euler_pole_and_angle(
			const GPlatesMaths::FiniteRotation &finite_rotation,
			bool use_north_pole_for_identity)
	{
		if (use_north_pole_for_identity)
		{
			if (GPlatesMaths::represents_identity_rotation(finite_rotation.unit_quat()))
			{
				return bp::make_tuple(
						GPlatesMaths::PointOnSphere::north_pole,
						GPlatesMaths::real_t(0));
			}
		}

		// Throws IndeterminateResultException if represents identity rotation.
		const GPlatesMaths::UnitQuaternion3D::RotationParams rotation_params =
				finite_rotation.unit_quat().get_rotation_params(
						finite_rotation.axis_hint());

		return bp::make_tuple(
				GPlatesMaths::PointOnSphere(rotation_params.axis),
				rotation_params.angle);
	}

	// Return the Euler (latitude_pole, longitude_pole, angle_degrees) tuple.
	bp::tuple
	finite_rotation_get_lat_lon_euler_pole_and_angle_degrees(
			const GPlatesMaths::FiniteRotation &finite_rotation,
			bool use_north_pole_for_identity)
	{
		if (use_north_pole_for_identity)
		{
			if (GPlatesMaths::represents_identity_rotation(finite_rotation.unit_quat()))
			{
				return bp::make_tuple(
						GPlatesMaths::real_t(0),
						GPlatesMaths::real_t(0),
						GPlatesMaths::real_t(0));
			}
		}

		// Throws IndeterminateResultException if represents identity rotation.
		const GPlatesMaths::UnitQuaternion3D::RotationParams rotation_params =
				finite_rotation.unit_quat().get_rotation_params(
						finite_rotation.axis_hint());

		const GPlatesMaths::LatLonPoint lat_lon_pole =
				make_lat_lon_point(GPlatesMaths::PointOnSphere(rotation_params.axis));

		return bp::make_tuple(
				lat_lon_pole.latitude(),
				lat_lon_pole.longitude(),
				GPlatesMaths::convert_rad_to_deg(rotation_params.angle));
	}

	bp::object
	finite_rotation_eq(
			const GPlatesMaths::FiniteRotation &finite_rotation,
			bp::object other)
	{
		bp::extract<const GPlatesMaths::FiniteRotation &> extract_other_finite_rotation(other);
		if (extract_other_finite_rotation.check())
		{
			return bp::object(finite_rotation.unit_quat() == extract_other_finite_rotation().unit_quat());
		}

		// Return NotImplemented so python can continue looking for a match
		// (eg, in case 'other' is a class that implements relational operators with FiniteRotation).
		//
		// NOTE: This will most likely fall back to python's default handling which uses 'id()'
		// and hence will compare based on *python* object address rather than *C++* object address.
		return bp::object(bp::handle<>(bp::borrowed(Py_NotImplemented)));
	}

	bp::object
	finite_rotation_ne(
			GPlatesMaths::FiniteRotation &finite_rotation,
			bp::object other)
	{
		bp::object ne_result = finite_rotation_eq(finite_rotation, other);
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
					"Finite rotations are equality (``==``, ``!=``) comparable (but not hashable "
					"- cannot be used as a key in a dict).\n"
					"\n"
					"Finite rotations can also be compared using :meth:`are_equivalent` to detect "
					"equivalent rotations (that rotate a geometry to the same final position but rotate "
					"in opposite directions around the globe).\n"
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
					"=========================== =======================================================================\n"
					"\n"
					"For example, the rotation of a :class:`PolylineOnSphere`:\n"
					"::\n"
					"\n"
					"  polyline = pygplates.PolylineOnSphere(...)\n"
					"  finite_rotation = pygplates.FiniteRotation(pole, angle)\n"
					"  rotated_polyline = finite_rotation * polyline\n"
					"\n"
					"Two finite rotations can be interpolated using :meth:`interpolate`:\n"
					"::\n"
					"\n"
					"  interpolated_rotation = pygplates.FiniteRotation.interpolate("
					"finite_rotation1, finite_rotation2, time1, time2, target_time)\n"
					"\n"
					"Two finite rotations can be composed in either of the following equivalent ways:\n"
					"\n"
					"* ``composed_finite_rotation = finite_rotation1 * finite_rotation2``\n"
					"* ``composed_finite_rotation = pygplates.FiniteRotation.compose("
					"finite_rotation1, finite_rotation2)``\n"
					"\n"
					"The latter technique uses :meth:`compose`. "
					"Note that rotation composition is *not* commutative (``A*B != B*A``)\n"
					"\n"
					"**The following is general information on composing finite rotations in various "
					"plate tectonic scenarios**...\n"
					"\n"
					"In the following examples a composed rotation ``R2 * R1`` means the rotation ``R1`` "
					"is the first rotation to be applied followed by the rotation ``R2`` "
					"(note that rotations are *not* commutative ``R2*R1 != R1*R2``) such that "
					"a geometry is rotated in the following way:\n"
					"::\n"
					"\n"
					"  Geometry_final        = R2 * R1 * Geometry_initial\n"
					"                        = R2 * (R1 * Geometry_initial)\n"
					"\n"
					"...which is the equivalent of...\n"
					"::\n"
					"\n"
					"  Geometry_intermediate = R1 * Geometry_initial\n"
					"  Geometry_final        = R2 * Geometry_intermediate\n"
					"\n"
					"Rotation from present day (0Ma) to time ``t2`` (via time ``t1``):\n"
					"::\n"
					"\n"
					"  R(0->t2)  = R(t1->t2) * R(0->t1)\n"
					"\n"
					"...or by post-multiplying both sides by ``R(t1->0)``, and then swapping sides, this becomes...\n"
					"::\n"
					"\n"
					"  R(t1->t2) = R(0->t2) * R(t1->0)\n"
					"\n"
					"Rotation from anchor plate ``A`` to moving plate ``M`` (via fixed plate ``F``):\n"
					"::\n"
					"\n"
					"  R(A->M) = R(A->F) * R(F->M)\n"
					"\n"
					"...or by pre-multiplying both sides by ``R(F->A)`` this becomes...\n"
					"::\n"
					"\n"
					"  R(F->M) = R(F->A) * R(A->M)\n"
					"\n"
					"NOTE: The rotations for relative times and for relative plates have the opposite order of each other !\n"
					"\n"
					"In other words:\n"
					"\n"
					"* For times ``0->t1->t2`` you apply the ``0->t1`` rotation first followed by the ``t1->t2`` rotation:\n"
					"  ::\n"
					"\n"
					"    R(0->t2)  = R(t1->t2) * R(0->t1)\n"
					"\n"
					"* For plate circuit ``A->F->M`` you apply the ``F->M`` rotation first followed by the ``A->F`` rotation:\n"
					"  ::\n"
					"\n"
					"    R(A->M) = R(A->F) * R(F->M)\n"
					"\n"
					"  Note that this is not ``A->F`` followed by ``F->M`` as you might expect (looking at the time example).\n"
					"\n"
					"This is probably best explained by the difference between thinking in terms of the grand fixed "
					"coordinate system and local coordinate system (see http://glprogramming.com/red/chapter03.html#name2). "
					"Essentially, in the plate circuit ``A->F->M``, the ``F->M`` rotation can be thought of as a rotation "
					"within the local coordinate system of ``A->F``. In other words ``F->M`` is not a rotation that "
					"occurs relative to the global spin axis but a rotation relative to the local coordinate system "
					"of plate ``F`` *after* it has been rotated relative to the anchor plate ``A``.\n"
					"\n"
					"For the times ``0->t1->t2`` this local/relative coordinate system concept does not apply.\n"
					"\n"
					"NOTE: A rotation must be relative to present day (0Ma) before it can be separated into "
					"a (plate circuit) chain of moving/fixed plate pairs.\n"
					"For example, the following is correct...\n"
					"::\n"
					"\n"
					"  R(t1->t2,A->C)\n"
					"     = R(0->t2,A->C) * R(t1->0,A->C)\n"
					"     = R(0->t2,A->C) * inverse[R(0->t1,A->C)]\n"
					"     // Now that all times are relative to 0Ma we can split A->C into A->B->C...\n"
					"     = R(0->t2,A->B) * R(0->t2,B->C) * inverse[R(0->t1,A->B) * R(0->t1,B->C)]\n"
					"     = R(0->t2,A->B) * R(0->t2,B->C) * inverse[R(0->t1,B->C)] * inverse[R(0->t1,A->B)]\n"
					"\n"
					"...but the following is *incorrect*...\n"
					"::\n"
					"\n"
					"  R(t1->t2,A->C)\n"
					"     = R(t1->t2,A->B) * R(t1->t2,B->C)   // <-- This line is *incorrect*\n"
					"     = R(0->t2,A->B) * R(t1->0,A->B) * R(0->t2,B->C) * R(t1->0,B->C)\n"
					"     = R(0->t2,A->B) * inverse[R(0->t1,A->B)] * R(0->t2,B->C) * inverse[R(0->t1,B->C)]\n"
					"\n"
					"...as can be seen above this gives two different results - the same four rotations are "
					"present in each result but in a different order.\n"
					"\n"
					"``A->B->C`` means ``B->C`` is the rotation of ``C`` relative to ``B`` and ``A->B`` is "
					"the rotation of ``B`` relative to ``A``. The need for rotation ``A->C`` to be relative "
					"to present day (0Ma) before it can be split into ``A->B`` and ``B->C`` is because "
					"``A->B`` and ``B->C`` are defined (in the rotation file) as total reconstruction "
					"poles which are always relative to present day.\n"
					"\n"
					"\n"
					"An example that combines all this is the stage rotation of a moving plate relative "
					"to a fixed plate and from time ``t1`` to time ``t2``:\n"
					"::\n"
					"\n"
					"  R(t1->t2,F->M)\n"
					"     = R(0->t2,F->M) * R(t1->0,F->M)\n"
					"     = R(0->t2,F->M) * inverse[R(0->t1,F->M)]\n"
					"     = R(0->t2,F->A) * R(0->t2,A->M) * inverse[R(0->t1,F->A) * R(0->t1,A->M)]\n"
					"     = inverse[R(0->t2,A->F)] * R(0->t2,A->M) * inverse[inverse[R(0->t1,A->F)] * R(0->t1,A->M)]\n"
					"     = inverse[R(0->t2,A->F)] * R(0->t2,A->M) * inverse[R(0->t1,A->M)] * R(0->t1,A->F)\n"
					"\n"
					"...where ``A`` is the anchor plate, ``F`` is the fixed plate and ``M`` is the moving plate "
					"(see :meth:`ReconstructionTree.get_relative_stage_rotation`).\n"
					"\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::finite_rotation_create,
						bp::default_call_policies(),
						(bp::arg("pole"), bp::arg("angle_radians"))),
				"__init__(pole, angle_radians)\n"
				"  Create a finite rotation from an Euler pole and a rotation angle (in *radians*).\n"
				"\n"
				"  :param pole: the Euler pole.\n"
				"  :type pole: :class:`PointOnSphere` or :class:`LatLonPoint` or tuple (latitude,longitude)"
				", in degrees, or tuple (x,y,z)\n"
				"  :param angle_radians: the rotation angle (in *radians*).\n"
				"  :type angle_radians: float\n"
				"  :raises: InvalidLatLonError if *latitude* or *longitude* is invalid\n"
				"  :raises: ViolatedUnitVectorInvariantError if (x,y,z) is not unit magnitude\n"
				"\n"
				"  The following example shows a few different ways to use this method:\n"
				"  ::\n"
				"\n"
				"    finite_rotation = pygplates.FiniteRotation(pygplates.PointOnSphere(x,y,z), angle_radians)\n"
				"    finite_rotation = pygplates.FiniteRotation((x,y,z), math.radians(angle_degrees))\n"
				"    finite_rotation = pygplates.FiniteRotation([x,y,z], angle_radians)\n"
				"    finite_rotation = pygplates.FiniteRotation(numpy.array([x,y,z]), angle_radians)\n"
				"    finite_rotation = pygplates.FiniteRotation(pygplates.LatLonPoint(latitude,longitude), angle_radians)\n"
				"    finite_rotation = pygplates.FiniteRotation((latitude,longitude), angle_radians)\n"
				"    finite_rotation = pygplates.FiniteRotation([latitude,longitude], math.radians(angle_degrees))\n"
				"    finite_rotation = pygplates.FiniteRotation(numpy.array([latitude,longitude]), angle_radians)\n")
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::finite_rotation_create_identity_rotation,
						bp::default_call_policies()),
				"__init__()\n"
				"  Creates a finite rotation that does not rotate (it maps a vector onto the same vector).\n"
				"\n"
				"  Equivalent to :meth:`create_identity_rotation`.\n"
				"\n"
				"  ::\n"
				"\n"
				"    identity_finite_rotation = pygplates.FiniteRotation()\n")
		.def("create_identity_rotation",
				&GPlatesApi::finite_rotation_create_identity_rotation,
				"create_identity_rotation() -> FiniteRotation\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Creates a finite rotation that does not rotate (it maps a vector onto the same vector).\n"
				"\n"
				"  :rtype: :class:`FiniteRotation`\n"
				"\n"
				"  To determine if a finite rotation is an identity rotation use :meth:`represents_identity_rotation`.\n"
				"\n"
				"  ::\n"
				"\n"
				"    identity_finite_rotation = pygplates.FiniteRotation.create_identity_rotation()\n"
				"    assert(identity_finite_rotation.represents_identity_rotation())\n"
				"    # The rotated point and original point are at the same position.\n"
				"    rotated_point = identity_finite_rotation * point\n"
				"\n"
				"  An alternative way to create an identity rotation is with *any* Euler pole and a *zero* angle:\n"
				"  ::\n"
				"\n"
				"    identity_finite_rotation = pygplates.FiniteRotation(any_euler_pole, 0)\n")
		.staticmethod("create_identity_rotation")
		.def("are_equivalent",
				&GPlatesApi::finite_rotation_represent_equivalent_rotations,
				(bp::arg("finite_rotation1"), bp::arg("finite_rotation2")),
				"are_equivalent(finite_rotation1, finite_rotation2) -> bool\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Return whether two finite rotations represent equivalent rotations.\n"
				"\n"
				"  :param finite_rotation1: the first finite rotation\n"
				"  :type finite_rotation1: :class:`FiniteRotation`\n"
				"  :param finite_rotation2: the second finite rotation\n"
				"  :type finite_rotation2: :class:`FiniteRotation`\n"
				"  :rtype: bool\n"
				"\n"
				"  Two rotations are equivalent if they rotate a geometry to the same final location. "
				"This includes rotating in opposite directions around the globe.\n"
				"\n"
				"  Some examples of equivalent rotations:\n"
				"\n"
				"  1. Negating a finite rotation's Euler pole (making it antipodal) and negating its angle.\n"
				"  2. Negating a finite rotation's Euler pole (making it antipodal) and setting its angle "
				"to '360 - angle' degrees (making the rotation go the other way around the globe).\n"
				"  3. Setting a finite rotation's angle to 'angle - 360' degrees (making the rotation "
				"go the other way around the globe).\n"
				"\n"
				"  Note that in (1) the finite rotations also compare equal (``==``), even though they "
				"were created with a different pole/angle, whereas in (2) and (3) the finite rotations "
				"compare unequal (``!=``). This is because (1) generates the exact same rotation whereas "
				"(2) and (3) generate rotations that go the opposite direction around the globe. "
				"Note however that all three rotations are still *equivalent*.\n"
				"\n"
				"  ::\n"
				"\n"
				"    if pygplates.FiniteRotation.are_equivalent(finite_rotation1, finite_rotation2):\n"
				"        ....\n")
		.staticmethod("are_equivalent")
		.def("compose",
				compose,
				(bp::arg("finite_rotation1"), bp::arg("finite_rotation2")),
				"compose(finite_rotation1, finite_rotation2) -> FiniteRotation\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Composes two finite rotations and returns the composed finite rotation.\n"
				"\n"
				"  :param finite_rotation1: the left-hand-side finite rotation\n"
				"  :type finite_rotation1: :class:`FiniteRotation`\n"
				"  :param finite_rotation2: the right-hand-side finite rotation\n"
				"  :type finite_rotation2: :class:`FiniteRotation`\n"
				"  :rtype: :class:`FiniteRotation`\n"
				"\n"
				"  This method does the same as ``finite_rotation1 * finite_rotation2``.\n"
				"\n"
				"  See :class:`FiniteRotation` for more details on composing finite rotations.\n"
				"\n"
				"  ::\n"
				"\n"
				"    composed_rotation = pygplates.FiniteRotation.compose(finite_rotation1, finite_rotation2)\n"
				"    #...or...\n"
				"    composed_rotation = finite_rotation1 * finite_rotation2\n")
		.staticmethod("compose")
		.def("interpolate",
				&GPlatesApi::finite_rotation_interpolate,
				(bp::arg("finite_rotation1"),
					bp::arg("finite_rotation2"),
					bp::arg("time1"),
					bp::arg("time2"),
					bp::arg("target_time")),
				"interpolate(finite_rotation1, finite_rotation2, time1, time2, target_time) -> FiniteRotation\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Calculate the finite rotation which is the interpolation of two finite rotations.\n"
				"\n"
				"  :param finite_rotation1: the left-hand-side finite rotation\n"
				"  :type finite_rotation1: :class:`FiniteRotation`\n"
				"  :param finite_rotation2: the right-hand-side finite rotation\n"
				"  :type finite_rotation2: :class:`FiniteRotation`\n"
				"  :param time1: the time associated with the left-hand-side finite rotation\n"
				"  :type time1: float or :class:`GeoTimeInstant`\n"
				"  :param time2: the time associated with the right-hand-side finite rotation\n"
				"  :type time2: float or :class:`GeoTimeInstant`\n"
				"  :param target_time: the time associated with the result of the interpolation\n"
				"  :type target_time: float or :class:`GeoTimeInstant`\n"
				"  :rtype: :class:`FiniteRotation`\n"
				"  :raises: InterpolationError if any time value is "
				":meth:`distant past<GeoTimeInstant.is_distant_past>` or "
				":meth:`distant future<GeoTimeInstant.is_distant_future>`\n"
				"\n"
				"\n"
				"  The finite rotations *finite_rotation1* and *finite_rotation2* are associated with "
				"times *time1* and *time2*, respectively. The result of the interpolation is associated "
				"with *target_time*. The interpolated finite rotation is generated using Spherical Linear "
				"intERPolation (SLERP) with the interpolation factor ``(target_time - time1) / (time2 - time1)``.\n"
				"\n"
				"  *target_time* can be any time - it does not have to be between *time1* and *time2*.\n"
				"\n"
				"  If *time1* and *time2* are equal then *finite_rotation1* is returned.\n"
				"  ::\n"
				"\n"
				"    interpolated_rotation = pygplates.FiniteRotation.interpolate("
				"finite_rotation1, finite_rotation2, time1, time2, target_time)\n")
		.staticmethod("interpolate")
		.def("represents_identity_rotation",
				&GPlatesApi::finite_rotation_represents_identity_rotation,
				"represents_identity_rotation() -> bool\n"
				"  Return whether this finite rotation represents an identity rotation (a rotation "
				"which maps a vector onto the same vector).\n"
				"\n"
				"  :rtype: bool\n"
				"\n"
				"  ::\n"
				"\n"
				"    # Create an identity rotation using zero angle and any pole location.\n"
				"    identity_finite_rotation = pygplates.FiniteRotation(any_pole, 0)\n"
				"    assert(identity_finite_rotation.represents_identity_rotation())\n")
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
		.def("get_euler_pole_and_angle",
				&GPlatesApi::finite_rotation_get_euler_pole_and_angle,
				(bp::arg("use_north_pole_for_identity")=true),
				"get_euler_pole_and_angle([use_north_pole_for_identity=True]) -> pole, angle_radians\n"
				"  Return the (pole, angle) representing finite rotation.\n"
				"\n"
				"  *NOTE:* the returned angle is in *radians*.\n"
				"\n"
				"  :param use_north_pole_for_identity: whether to return the north pole axis (and zero angle) "
				"for an :meth:`identity rotation<represents_identity_rotation>` or raise "
				"IndeterminateResultError (default is to return north pole axis)\n"
				"  :type use_north_pole_for_identity: bool\n"
				"  :rtype: tuple (:class:`PointOnSphere`, float)\n"
				"  :returns: the tuple of (pole, angle_radians)\n"
				"  :raises: IndeterminateResultError if *use_north_pole_for_identity* is ``False`` "
				"and this finite rotation represents the identity rotation\n"
				"\n"
				"  If :meth:`represents_identity_rotation` returns ``True`` then this method will "
				"return the north pole axis (and zero angle) if *use_north_pole_for_identity* is ``True``, "
				"otherwise *IndeterminateResultError* is raised.\n"
				"\n"
				"  Alternatively :meth:`get_lat_lon_euler_pole_and_angle_degrees` can be used to "
				"return the euler pole as latitude/longitude and angle (all in degrees).\n"
				"\n"
				"  Note that (pole, angle) and (-pole, -angle) represent equivalent rotations "
				"(see :meth:`are_equivalent`) and either could be returned. "
				"However, if this finite rotation was created with *__init__(pole, angle)* then the "
				"same pole and angle will be returned here.\n"
				"\n"
				"  ::\n"
				"\n"
				"    finite_rotation = pygplates.FiniteRotation(pole, angle_radians)\n"
				"    pole, angle_radians = finite_rotation.get_euler_pole_and_angle()\n")
		.def("get_lat_lon_euler_pole_and_angle_degrees",
				&GPlatesApi::finite_rotation_get_lat_lon_euler_pole_and_angle_degrees,
				(bp::arg("use_north_pole_for_identity")=true),
				"get_lat_lon_euler_pole_and_angle_degrees([use_north_pole_for_identity=True]) -> "
				"pole_latitude, pole_longitude, angle_degrees\n"
				"  Return the finite rotation as a tuple of pole latitude, pole longitude and "
				" angle (all in degrees).\n"
				"\n"
				"  *NOTE:* the returned angle is in *degrees* (as are the latitude and longitude).\n"
				"\n"
				"  :param use_north_pole_for_identity: whether to return the north pole axis (and zero angle) "
				"for an :meth:`identity rotation<represents_identity_rotation>` or raise "
				"IndeterminateResultError (default is to return north pole axis)\n"
				"  :type use_north_pole_for_identity: bool\n"
				"  :rtype: tuple (float, float, float)\n"
				"  :returns: the tuple of (pole_latitude, pole_longitude, angle_degrees) all in *degrees*\n"
				"  :raises: IndeterminateResultError if *use_north_pole_for_identity* is ``False`` "
				"and this finite rotation represents the identity rotation\n"
				"\n"
				"  If :meth:`represents_identity_rotation` returns ``True`` then this method will "
				"return the north pole axis (and zero angle) if *use_north_pole_for_identity* is ``True``, "
				"otherwise *IndeterminateResultError* is raised.\n"
				"\n"
				"  Note that (latitude, longitude, angle) and (-latitude, longitude-180, -angle) "
				"represent equivalent rotations (see :meth:`are_equivalent`) and "
				"either could be returned. However, if this finite rotation was created with "
				"*__init__(pole, angle)* then the same pole and angle will be returned here.\n"
				"\n"
				"  ::\n"
				"\n"
				"    finite_rotation = pygplates.FiniteRotation(pole, angle_radians)\n"
				"    pole_latitude, pole_longitude, angle_degrees = "
				"finite_rotation.get_lat_lon_euler_pole_and_angle_degrees()\n")
		// Multiply two finite rotations...
		.def("__mul__", compose)
		// Rotations...
		.def(bp::self * bp::other<GPlatesMaths::GreatCircleArc>())
		.def(bp::self * bp::other<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>())
		.def(bp::self * bp::other<GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type>())
		.def(bp::self * bp::other<GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type>())
		.def(bp::self * bp::other<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>())
		.def(bp::self * bp::other<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type>())
		// Comparison operators...
		// Due to the numerical tolerance in comparisons we cannot make hashable.
		// Make unhashable, with no *equality* comparison operators (we explicitly define them)...
		.def(GPlatesApi::NoHashDefVisitor(false, true))
		.def("__eq__", &GPlatesApi::finite_rotation_eq)
		.def("__ne__", &GPlatesApi::finite_rotation_ne)
		// Generate '__str__' from 'operator<<'...
		// Note: Seems we need to qualify with 'self_ns::' to avoid MSVC compile error.
		.def(bp::self_ns::str(bp::self))
	;

	// Non-member equivalent rotations function...
	//
	// NOTE: This will get deprecated at some stage in favour of the equivalent static class method.
	// There's no docstring because we are only documenting the static class method version.
	bp::def("represent_equivalent_rotations",
			&GPlatesApi::finite_rotation_represent_equivalent_rotations,
			(bp::arg("finite_rotation1"), bp::arg("finite_rotation2")));

	// Non-member conversion function...
	//
	// NOTE: This will get deprecated at some stage in favour of the equivalent static class method.
	// There's no docstring because we are only documenting the static class method version.
	bp::def("compose_finite_rotations",
			compose,
			(bp::arg("finite_rotation1"), bp::arg("finite_rotation2")));

	// Non-member interpolation function...
	//
	// NOTE: This will get deprecated at some stage in favour of the equivalent static class method.
	// There's no docstring because we are only documenting the static class method version.
	bp::def("interpolate_finite_rotations",
			&GPlatesApi::finite_rotation_interpolate,
			(bp::arg("finite_rotation1"),
				bp::arg("finite_rotation2"),
				bp::arg("time1"),
				bp::arg("time2"),
				bp::arg("target_time")));

	// Enable boost::optional<FiniteRotation> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesMaths::FiniteRotation>();
}

#endif // GPLATES_NO_PYTHON
