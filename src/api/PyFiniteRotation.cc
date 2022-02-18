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

#include <cmath>
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
#include "maths/Vector3D.h"

#include "property-values/GeoTimeInstant.h"


namespace bp = boost::python;


namespace GPlatesApi
{
	boost::shared_ptr<GPlatesMaths::FiniteRotation>
	finite_rotation_create_from_euler_pole_and_angle(
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
						GPlatesMaths::FiniteRotation::create_identity_rotation()));
	}

	boost::shared_ptr<GPlatesMaths::FiniteRotation>
	finite_rotation_create_great_circle_point_rotation(
			// There are from-python converters from LatLonPoint and sequence(latitude,longitude) and
			// sequence(x,y,z) to PointOnSphere so they will also get matched by this...
			const GPlatesMaths::PointOnSphere &from_point,
			const GPlatesMaths::PointOnSphere &to_point)
	{
		return boost::shared_ptr<GPlatesMaths::FiniteRotation>(
				new GPlatesMaths::FiniteRotation(
						GPlatesMaths::FiniteRotation::create_great_circle_point_rotation(from_point, to_point)));
	}

	boost::shared_ptr<GPlatesMaths::FiniteRotation>
	finite_rotation_create_small_circle_point_rotation(
			// There are from-python converters from LatLonPoint and sequence(latitude,longitude) and
			// sequence(x,y,z) to PointOnSphere so they will also get matched by this...
			const GPlatesMaths::PointOnSphere &rotation_pole,
			const GPlatesMaths::PointOnSphere &from_point,
			const GPlatesMaths::PointOnSphere &to_point)
	{
		return boost::shared_ptr<GPlatesMaths::FiniteRotation>(
				new GPlatesMaths::FiniteRotation(
						GPlatesMaths::FiniteRotation::create_small_circle_point_rotation(rotation_pole, from_point, to_point)));
	}

	boost::shared_ptr<GPlatesMaths::FiniteRotation>
	finite_rotation_create_segment_rotation(
			// There are from-python converters from LatLonPoint and sequence(latitude,longitude) and
			// sequence(x,y,z) to PointOnSphere so they will also get matched by this...
				const GPlatesMaths::PointOnSphere &from_segment_start,
				const GPlatesMaths::PointOnSphere &from_segment_end,
				const GPlatesMaths::PointOnSphere &to_segment_start,
				const GPlatesMaths::PointOnSphere &to_segment_end)
	{
		return boost::shared_ptr<GPlatesMaths::FiniteRotation>(
				new GPlatesMaths::FiniteRotation(
						GPlatesMaths::FiniteRotation::create_segment_rotation(
								from_segment_start, from_segment_end,
								to_segment_start, to_segment_end)));
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
						GPlatesMaths::real_t(90),
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

	GPlatesMaths::real_t
	finite_rotation_get_rotation_distance(
			const GPlatesMaths::FiniteRotation &finite_rotation,
			// There are from-python converters from LatLonPoint and sequence(latitude,longitude) and
			// sequence(x,y,z) to PointOnSphere so they will also get matched by this...
			const GPlatesMaths::PointOnSphere &from_point)
	{
		if (GPlatesMaths::represents_identity_rotation(finite_rotation.unit_quat()))
		{
			return 0;
		}

		const GPlatesMaths::UnitQuaternion3D::RotationParams rotation_params =
				finite_rotation.unit_quat().get_rotation_params(finite_rotation.axis_hint());

		const GPlatesMaths::real_t small_circle_radius =
				cross(rotation_params.axis, from_point.position_vector()).magnitude();

		return small_circle_radius * rotation_params.angle;
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

	bool
	finite_rotation_are_equal(
			const GPlatesMaths::FiniteRotation &finite_rotation1,
			const GPlatesMaths::FiniteRotation &finite_rotation2,
			boost::optional<double> threshold_degrees)
	{
		if (threshold_degrees)
		{
			// Lat/lon/angle for 'finite_rotation1'.
			double rotation1_lat, rotation1_lon, rotation1_angle;
			if (GPlatesMaths::represents_identity_rotation(finite_rotation1.unit_quat()))
			{
				rotation1_lat = 90;
				rotation1_lon = 0;
				rotation1_angle = 0;
			}
			else
			{
				// Throws IndeterminateResultException if represents identity rotation.
				const GPlatesMaths::UnitQuaternion3D::RotationParams rotation1_params =
						finite_rotation1.unit_quat().get_rotation_params(boost::none);

				const GPlatesMaths::LatLonPoint finite_rotation1_lat_lon_pole =
						make_lat_lon_point(GPlatesMaths::PointOnSphere(rotation1_params.axis));

				rotation1_lat = finite_rotation1_lat_lon_pole.latitude();
				rotation1_lon = finite_rotation1_lat_lon_pole.longitude();
				rotation1_angle = GPlatesMaths::convert_rad_to_deg(rotation1_params.angle.dval());
			}

			// Lat/lon/angle for 'finite_rotation2'.
			double rotation2_lat, rotation2_lon, rotation2_angle;
			if (GPlatesMaths::represents_identity_rotation(finite_rotation2.unit_quat()))
			{
				rotation2_lat = 90;
				rotation2_lon = 0;
				rotation2_angle = 0;
			}
			else
			{
				// Throws IndeterminateResultException if represents identity rotation.
				const GPlatesMaths::UnitQuaternion3D::RotationParams rotation2_params =
						finite_rotation2.unit_quat().get_rotation_params(boost::none);

				const GPlatesMaths::LatLonPoint finite_rotation2_lat_lon_pole =
						make_lat_lon_point(GPlatesMaths::PointOnSphere(rotation2_params.axis));

				rotation2_lat = finite_rotation2_lat_lon_pole.latitude();
				rotation2_lon = finite_rotation2_lat_lon_pole.longitude();
				rotation2_angle = GPlatesMaths::convert_rad_to_deg(rotation2_params.angle.dval());
			}

			// See if both rotations are close enough.
			//
			// Note that we don't need to consider negating the axis and angle of any of these
			// rotations because doing that results in the exact same rotation (quaternion).
			// Also note that we return the original UnitQuaternion3D::get_rotation_params() above
			// by not specifying an axis hint (and hence avoids returning flipped angle/axis version.
			return std::fabs(rotation2_lat - rotation1_lat) <= threshold_degrees.get() &&
					std::fabs(rotation2_lon - rotation1_lon) <= threshold_degrees.get() &&
					std::fabs(rotation2_angle - rotation1_angle) <= threshold_degrees.get();
		}

		return finite_rotation1.unit_quat() == finite_rotation2.unit_quat();
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
					".. note:: For general information on composing finite rotations in various plate tectonic scenarios "
					"see :ref:`pygplates_foundations_working_with_finite_rotations`.\n"
					"\n"
					"A finite rotation is a rotation about an *Euler pole* by an angular distance. "
					"An Euler pole is represented by a point on the surface of the globe where a "
					"rotation vector (radially extending from the centre of the globe) intersects "
					"the surface of the (unit radius) globe.\n"
					"\n"
					"An Euler pole is specified by a point on the surface of the globe. \n"
					"\n"
					"A rotation angle is specified in radians, with the usual sense of rotation:\n"
					"\n"
					"* a positive angle represents an anti-clockwise rotation around the rotation vector,\n"
					"* a negative angle corresponds to a clockwise rotation.\n"
					"\n"
					"A finite rotation can be :meth:`created<__init__>`:\n"
					"\n"
					"* explicitly from an Euler pole and an angle, or\n"
					"* from two points (rotates one point to the other along great circle arc), or\n"
					"* as an :meth:`identity<create_identity_rotation>` rotation (no rotation).\n"
					"\n"
					"The Euler pole and angle can be retrieved using:\n"
					"\n"
					"* :meth:`get_euler_pole_and_angle` as a tuple of Euler pole and angle (radians), or\n"
					"* :meth:`get_lat_lon_euler_pole_and_angle_degrees` as a tuple of Euler pole "
					"latitude and longitude and angle (all in degrees).\n"
					"\n"
					"Multiplication operations can be used to rotate various geometry types:\n"
					"\n"
					"=========================== =======================================================================\n"
					"Operation                    Result\n"
					"=========================== =======================================================================\n"
					"``fr * vector``              Rotates :class:`Vector3D` *vector* using finite rotation *fr*\n"
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
					"The distance that a point is rotated along its small circle rotation arc can be found "
					"using :meth:`get_rotation_distance`.\n"
					"\n"
					"Two finite rotations can be composed in either of the following equivalent ways:\n"
					"\n"
					"* ``composed_finite_rotation = finite_rotation1 * finite_rotation2``\n"
					"* ``composed_finite_rotation = pygplates.FiniteRotation.compose("
					"finite_rotation1, finite_rotation2)``\n"
					"\n"
					"The latter technique uses :meth:`compose`. "
					"Note that rotation composition is *not* commutative (:math:`A \\times B \\neq B \\times A`).\n"
					"\n"
					"The reverse, or inverse, of a finite rotation can be found using :meth:`get_inverse`.\n"
					"\n"
					"Two finite rotations can be interpolated using :meth:`interpolate`:\n"
					"::\n"
					"\n"
					"  interpolated_rotation = pygplates.FiniteRotation.interpolate("
					"finite_rotation1, finite_rotation2, time1, time2, target_time)\n"
					"\n"
					"Finite rotations are equality (``==``, ``!=``) comparable (but not hashable "
					"- cannot be used as a key in a ``dict``).\n"
					"\n"
					"Finite rotations can also be compared using :meth:`are_equivalent` to detect "
					"equivalent rotations (that rotate a geometry to the same final position but might rotate "
					"in opposite directions around the globe). A finite rotation can be tested to see if "
					"it is an :meth:`identity<represents_identity_rotation>` rotation (no rotation).\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::finite_rotation_create_from_euler_pole_and_angle,
						bp::default_call_policies(),
						(bp::arg("pole"), bp::arg("angle_radians"))),
				// General overloaded signature (must be in first overloaded 'def' - used by Sphinx)...
				"__init__(...)\n"
				"A *FiniteRotation* object can be constructed in more than one way...\n"
				"\n"
				// Specific overload signature...
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
						&GPlatesApi::finite_rotation_create_great_circle_point_rotation,
						bp::default_call_policies(),
						(bp::arg("from_point"), bp::arg("to_point"))),
				// Specific overload signature...
				"__init__(from_point, to_point)\n"
				"  Create a finite rotation that rotates one point to another along the "
				"great circle arc connecting them.\n"
				"\n"
				"  :param from_point: the point to rotate *from*\n"
				"  :type from_point: :class:`PointOnSphere` or :class:`LatLonPoint` or tuple (latitude,longitude)"
				", in degrees, or tuple (x,y,z)\n"
				"  :param to_point: the point to rotate *to*\n"
				"  :type to_point: :class:`PointOnSphere` or :class:`LatLonPoint` or tuple (latitude,longitude)"
				", in degrees, or tuple (x,y,z)\n"
				"  :raises: InvalidLatLonError if *latitude* or *longitude* is invalid\n"
				"  :raises: ViolatedUnitVectorInvariantError if (x,y,z) is not unit magnitude\n"
				"\n"
				"  If *from_point* and *to_point* are the same or antipodal (opposite sides of globe) "
				"then an arbitrary rotation axis (among the infinite possible choices) is selected.\n"
				"\n"
				"  ::\n"
				"\n"
				"    finite_rotation = pygplates.FiniteRotation(from_point, to_point)\n"
				"    # assert(to_point == finite_rotation * from_point)\n"
				"\n"
				"  .. seealso:: :meth:`create_great_circle_point_rotation`\n")
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::finite_rotation_create_identity_rotation,
						bp::default_call_policies()),
				// Specific overload signature...
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
				"create_identity_rotation()\n"
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
				"    # assert(identity_finite_rotation.represents_identity_rotation())\n"
				"\n"
				"    # The rotated point and original point are at the same position.\n"
				"    rotated_point = identity_finite_rotation * point\n"
				"\n"
				"  An alternative way to create an identity rotation is with *any* Euler pole and a *zero* angle:\n"
				"  ::\n"
				"\n"
				"    identity_finite_rotation = pygplates.FiniteRotation(any_euler_pole, 0)\n")
		.staticmethod("create_identity_rotation")
		.def("create_great_circle_point_rotation",
				&GPlatesApi::finite_rotation_create_great_circle_point_rotation,
				"create_great_circle_point_rotation(from_point, to_point)\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Create a finite rotation that rotates one point to another along the "
				"great circle arc connecting them.\n"
				"\n"
				"  :param from_point: the point to rotate *from*\n"
				"  :type from_point: :class:`PointOnSphere` or :class:`LatLonPoint` or tuple (latitude,longitude)"
				", in degrees, or tuple (x,y,z)\n"
				"  :param to_point: the point to rotate *to*\n"
				"  :type to_point: :class:`PointOnSphere` or :class:`LatLonPoint` or tuple (latitude,longitude)"
				", in degrees, or tuple (x,y,z)\n"
				"  :raises: InvalidLatLonError if *latitude* or *longitude* is invalid\n"
				"  :raises: ViolatedUnitVectorInvariantError if (x,y,z) is not unit magnitude\n"
				"\n"
				"  If *from_point* and *to_point* are the same or antipodal (opposite sides of globe) "
				"then an arbitrary rotation axis (among the infinite possible choices) is selected.\n"
				"\n"
				"  ::\n"
				"\n"
				"    finite_rotation = pygplates.FiniteRotation.create_great_circle_point_rotation(from_point, to_point)\n"
				"    # assert(to_point == finite_rotation * from_point)\n"
				"\n"
				"  .. versionadded:: 29\n")
		.staticmethod("create_great_circle_point_rotation")
		.def("create_small_circle_point_rotation",
				&GPlatesApi::finite_rotation_create_small_circle_point_rotation,
				"create_small_circle_point_rotation(rotation_pole, from_point, to_point)\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Create a finite rotation, using the specified rotation pole, that rotates "
				"*from_point* to *to_point*.\n"
				"\n"
				"  :param rotation_pole: the rotation pole to rotate around\n"
				"  :type rotation_pole: :class:`PointOnSphere` or :class:`LatLonPoint` or tuple (latitude,longitude)"
				", in degrees, or tuple (x,y,z)\n"
				"  :param from_point: the point to rotate *from*\n"
				"  :type from_point: :class:`PointOnSphere` or :class:`LatLonPoint` or tuple (latitude,longitude)"
				", in degrees, or tuple (x,y,z)\n"
				"  :param to_point: the point to rotate *to*\n"
				"  :type to_point: :class:`PointOnSphere` or :class:`LatLonPoint` or tuple (latitude,longitude)"
				", in degrees, or tuple (x,y,z)\n"
				"  :raises: InvalidLatLonError if *latitude* or *longitude* is invalid\n"
				"  :raises: ViolatedUnitVectorInvariantError if (x,y,z) is not unit magnitude\n"
				"\n"
				"  .. note:: *from_point* doesn't actually have to rotate onto *to_point*. "
				"Imagine *rotation_pole* is the North Pole, then the returned rotation will rotate such that "
				"the longitude matches but not necessarily the latitude.\n"
				"\n"
				"  .. note:: If either *from_point* or *to_point* coincides with *rotation_pole* then the identity rotation is returned.\n"
				"\n"
				"  ::\n"
				"\n"
				"    finite_rotation = pygplates.FiniteRotation.create_small_circle_point_rotation(rotation_pole, from_point, to_point)\n"
				"\n"
				"  .. versionadded:: 29\n")
		.staticmethod("create_small_circle_point_rotation")
		.def("create_segment_rotation",
				&GPlatesApi::finite_rotation_create_segment_rotation,
				"create_segment_rotation(from_segment_start, from_segment_end, to_segment_start, to_segment_end)\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Create a finite rotation that rotates the *from* line segment to the *to* line segment.\n"
				"\n"
				"  :param from_segment_start: the start point of the segment to rotate *from*\n"
				"  :type from_segment_start: :class:`PointOnSphere` or :class:`LatLonPoint` or tuple (latitude,longitude)"
				", in degrees, or tuple (x,y,z)\n"
				"  :param from_segment_end: the end point of the segment to rotate *from*\n"
				"  :type from_segment_end: :class:`PointOnSphere` or :class:`LatLonPoint` or tuple (latitude,longitude)"
				", in degrees, or tuple (x,y,z)\n"
				"  :param to_segment_start: the start point of the segment to rotate *to*\n"
				"  :type to_segment_start: :class:`PointOnSphere` or :class:`LatLonPoint` or tuple (latitude,longitude)"
				", in degrees, or tuple (x,y,z)\n"
				"  :param to_segment_end: the end point of the segment to rotate *to*\n"
				"  :type to_segment_end: :class:`PointOnSphere` or :class:`LatLonPoint` or tuple (latitude,longitude)"
				", in degrees, or tuple (x,y,z)\n"
				"  :raises: InvalidLatLonError if *latitude* or *longitude* is invalid\n"
				"  :raises: ViolatedUnitVectorInvariantError if (x,y,z) is not unit magnitude\n"
				"\n"
				"  This is useful if you have the same geometry but at two different positions/orientations on the globe and you "
				"want to determine the rotation that maps one onto the other. In this case you can choose two non-coincident points "
				"of the geometry (at two different positions/orientations) and pass those four points to this function. "
				"For example, you might have a geometry that's been reconstructed to two different times but you don't "
				"have those two reconstruction rotations (you only have the two reconstructed geometries) - you can then "
				"use this function to find the rotation from one reconstruction time to the other.\n"
				"\n"
				"  ::\n"
				"\n"
				"    finite_rotation = pygplates.FiniteRotation.create_segment_rotation(\n"
				"        from_segment_start, from_segment_end,\n"
				"        to_segment_start, to_segment_end)\n"
				"\n"
				"  .. note:: The *from* and *to* segments do not actually have to be the same (arc) length. "
				"In this case, while *from_segment_start* is always rotated onto *to_segment_start*, "
				"*from_segment_end* is not rotated onto *to_segment_end*. Instead *from_segment_end* is "
				"rotated such that it is on the great circle containing the *to* segment. "
				"In this way the *from* segment is rotated such that its orientation matches the *to* segment "
				"(as well as having matching start points).\n"
				"\n"
				"  .. note:: If either segment is zero length then the returned rotation reduces to one that rotates "
				"*from_segment_start* to *to_segment_start* along the great circle arc between those two points. "
				"This is because one (or both) segments has no orientation (so all we can match are the start points).\n"
				"\n"
				"  .. note:: It's fine for the start points of both *from* and *to* segments to coincide "
				"(and it's also fine for the end points of both segments to coincide for that matter).\n"
				"\n"
				"  .. versionadded:: 29\n")
		.staticmethod("create_segment_rotation")
		.def("are_equivalent",
				&GPlatesApi::finite_rotation_represent_equivalent_rotations,
				(bp::arg("finite_rotation1"), bp::arg("finite_rotation2")),
				"are_equivalent(finite_rotation1, finite_rotation2)\n"
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
		.def("are_equal",
				&GPlatesApi::finite_rotation_are_equal,
				(bp::arg("finite_rotation1"),
						bp::arg("finite_rotation2"),
						bp::arg("threshold_degrees") = boost::optional<double>()),
				"are_equal(finite_rotation1, finite_rotation2, [threshold_degrees])\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Return whether two finite rotations have equal pole "
				"latitude, longitude and angle to within a threshold in degrees.\n"
				"\n"
				"  :param finite_rotation1: the first finite rotation\n"
				"  :type finite_rotation1: :class:`FiniteRotation`\n"
				"  :param finite_rotation2: the second finite rotation\n"
				"  :type finite_rotation2: :class:`FiniteRotation`\n"
				"  :param threshold_degrees: optional closeness threshold in degrees\n"
				"  :type threshold_degrees: float\n"
				"  :rtype: bool\n"
				"\n"
				"  If *threshold_degrees* is *not* specified then this function is the same as equality "
				"comparison (``==``).\n"
				"\n"
				"  If *threshold_degrees* is specified then *finite_rotation1* and *finite_rotation2* "
				"compare equal if both pole latitudes and both pole longitudes and both angles are within "
				"*threshold_degrees* degrees of each other.\n"
				"\n"
				"  Using a threshold in latitude/longitude coordinates is subject to longitude compression "
				"at the North and South poles. However these coordinates are useful when comparing finite "
				"rotations loaded from a text file that stores rotations using these coordinates (such as "
				"PLATES rotation format) and that typically stores values with limited precision.\n"
				"  ::\n"
				"\n"
				"    # Are two finite rotations equal to within 0.01 degrees.\n"
				"    # This is useful when the rotations were loaded from a PLATES rotation file\n"
				"    # that stored rotation lat/lon/angle to 2 decimal places accuracy.\n"
				"    if pygplates.FiniteRotation.are_equal(finite_rotation1, finite_rotation2, 0.01):\n"
				"        ....\n")
		.staticmethod("are_equal")
		.def("compose",
				compose,
				(bp::arg("finite_rotation1"), bp::arg("finite_rotation2")),
				"compose(finite_rotation1, finite_rotation2)\n"
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
				"  See :ref:`pygplates_foundations_working_with_finite_rotations` for more details on composing finite rotations.\n"
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
				"interpolate(finite_rotation1, finite_rotation2, time1, time2, target_time)\n"
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
				"represents_identity_rotation()\n"
				"  Return whether this finite rotation represents an identity rotation (a rotation "
				"which maps a vector onto the same vector).\n"
				"\n"
				"  :rtype: bool\n"
				"\n"
				"  ::\n"
				"\n"
				"    # Create an identity rotation using zero angle and any pole location.\n"
				"    identity_finite_rotation = pygplates.FiniteRotation(any_pole, 0)\n"
				"    # assert(identity_finite_rotation.represents_identity_rotation())\n")
		.def("get_inverse",
				&GPlatesMaths::get_reverse,
				"get_inverse()\n"
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
				"get_euler_pole_and_angle([use_north_pole_for_identity=True])\n"
				"  Return the (pole, angle) representing finite rotation.\n"
				"\n"
				"  .. note:: The returned angle is in *radians*.\n"
				"\n"
				"  :param use_north_pole_for_identity: whether to return the north pole axis (and zero angle) "
				"for an :meth:`identity rotation<represents_identity_rotation>` or raise "
				"IndeterminateResultError (default is to return north pole axis)\n"
				"  :type use_north_pole_for_identity: bool\n"
				"  :returns: the tuple of (pole, angle_radians)\n"
				"  :rtype: tuple (:class:`PointOnSphere`, float)\n"
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
				"get_lat_lon_euler_pole_and_angle_degrees([use_north_pole_for_identity=True])\n"
				"  Return the finite rotation as a tuple of pole latitude, pole longitude and "
				" angle (all in degrees).\n"
				"\n"
				"  .. note:: The returned angle is in *degrees* (as are the latitude and longitude).\n"
				"\n"
				"  :param use_north_pole_for_identity: whether to return the north pole axis (and zero angle) "
				"for an :meth:`identity rotation<represents_identity_rotation>` or raise "
				"IndeterminateResultError (default is to return north pole axis)\n"
				"  :type use_north_pole_for_identity: bool\n"
				"  :returns: the tuple of (pole_latitude, pole_longitude, angle_degrees) all in *degrees*\n"
				"  :rtype: tuple (float, float, float)\n"
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
		.def("get_rotation_distance",
				&GPlatesApi::finite_rotation_get_rotation_distance,
				(bp::arg("point")),
				"get_rotation_distance(point)\n"
				"  Return the distance that a point rotates along its small circle rotation arc (in radians).\n"
				"\n"
				"  :param point: the point being rotated (the start point of the rotation arc)\n"
				"  :type point: :class:`PointOnSphere` or :class:`LatLonPoint` or tuple (latitude,longitude)"
				", in degrees, or tuple (x,y,z)\n"
				"  :rtype: float\n"
				"\n"
				"  Returns the distance along the (small circle) rotation arc from the start point "
				"*point* to the end point ``finite_rotation * point``. Note that the returned distance "
				"is not the angle of rotation - it is the actual distance on the unit radius sphere "
				"(hence radians). To convert to distance on the Earth's surface multiply by the "
				"Earth radius (see :class:`Earth`).\n"
				"\n"
				"  ::\n"
				"\n"
				"    rotated_distance_radians = finite_rotation.get_rotation_distance(point)\n")
		// Multiply two finite rotations...
		.def("__mul__", compose)
		// Rotations...
		.def(bp::self * bp::other<GPlatesMaths::Vector3D>())
		.def(bp::self * bp::other<GPlatesMaths::GreatCircleArc>())
		.def(bp::self * bp::other<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>())
		.def(bp::self * bp::other<GPlatesMaths::PointGeometryOnSphere::non_null_ptr_to_const_type>())
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

	// Enable boost::optional<FiniteRotation> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesMaths::FiniteRotation>();
}
