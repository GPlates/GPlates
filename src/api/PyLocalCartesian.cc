/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#include <algorithm>
#include <iterator>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include "PythonConverterUtils.h"
#include "PythonHashDefVisitor.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/python.h"
// This is not included by <boost/python.hpp>.
// Also we must include this after <boost/python.hpp> which means after "global/python.h".
#include <boost/python/stl_iterator.hpp>

#include "maths/CartesianConvMatrix3D.h"
#include "maths/PointOnSphere.h"
#include "maths/Vector3D.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	GPlatesMaths::Vector3D
	local_cartesian_xyz_from_geocentric_to_north_east_down(
			const GPlatesMaths::CartesianConvMatrix3D &ccm,
			const GPlatesMaths::Real &x,
			const GPlatesMaths::Real &y,
			const GPlatesMaths::Real &z)
	{
		return GPlatesMaths::convert_from_geocentric_to_north_east_down(
				ccm,
				GPlatesMaths::Vector3D(x, y, z));
	}

	GPlatesMaths::Vector3D
	local_cartesian_xyz_from_north_east_down_to_geocentric(
			const GPlatesMaths::CartesianConvMatrix3D &ccm,
			const GPlatesMaths::Real &x,
			const GPlatesMaths::Real &y,
			const GPlatesMaths::Real &z)
	{
		return GPlatesMaths::convert_from_north_east_down_to_geocentric(
				ccm,
				GPlatesMaths::Vector3D(x, y, z));
	}


	GPlatesMaths::Vector3D
	local_cartesian_convert_xyz_from_geocentric_to_north_east_down(
			const GPlatesMaths::PointOnSphere &local_origin,
			const GPlatesMaths::Real &x,
			const GPlatesMaths::Real &y,
			const GPlatesMaths::Real &z)
	{
		return GPlatesMaths::convert_from_geocentric_to_north_east_down(
				GPlatesMaths::CartesianConvMatrix3D(local_origin),
				GPlatesMaths::Vector3D(x, y, z));
	}

	GPlatesMaths::Vector3D
	local_cartesian_convert_xyz_from_north_east_down_to_geocentric(
			const GPlatesMaths::PointOnSphere &local_origin,
			const GPlatesMaths::Real &x,
			const GPlatesMaths::Real &y,
			const GPlatesMaths::Real &z)
	{
		return GPlatesMaths::convert_from_north_east_down_to_geocentric(
				GPlatesMaths::CartesianConvMatrix3D(local_origin),
				GPlatesMaths::Vector3D(x, y, z));
	}


	GPlatesMaths::Vector3D
	local_cartesian_convert_from_geocentric_to_north_east_down(
			const GPlatesMaths::PointOnSphere &local_origin,
			const GPlatesMaths::Vector3D &vector)
	{
		return GPlatesMaths::convert_from_geocentric_to_north_east_down(
				GPlatesMaths::CartesianConvMatrix3D(local_origin),
				vector);
	}

	GPlatesMaths::Vector3D
	local_cartesian_convert_from_north_east_down_to_geocentric(
			const GPlatesMaths::PointOnSphere &local_origin,
			const GPlatesMaths::Vector3D &vector)
	{
		return GPlatesMaths::convert_from_north_east_down_to_geocentric(
				GPlatesMaths::CartesianConvMatrix3D(local_origin),
				vector);
	}


	bp::list
	local_cartesian_convert_sequence_from_geocentric_to_north_east_down(
			bp::object local_origins_object, // Any python sequence (eg, list, tuple).
			bp::object vectors_object) // Any python sequence (eg, list, tuple).
	{
		// Begin/end iterators over the python points sequence.
		bp::stl_input_iterator<GPlatesMaths::PointOnSphere>
				local_origins_begin(local_origins_object),
				local_origins_end;
		// Copy into a vector.
 		std::vector<GPlatesMaths::PointOnSphere> local_origins;
		std::copy(local_origins_begin, local_origins_end, std::back_inserter(local_origins));

		// Begin/end iterators over the python vectors sequence.
		bp::stl_input_iterator<GPlatesMaths::Vector3D>
				vectors_begin(vectors_object),
				vectors_end;
		// Copy into a vector.
 		std::vector<GPlatesMaths::Vector3D> vectors;
		std::copy(vectors_begin, vectors_end, std::back_inserter(vectors));

		if (local_origins.size() != vectors.size())
		{
			PyErr_SetString(PyExc_ValueError, "'local_origins' and 'vectors' should be the same size");
			bp::throw_error_already_set();
		}

		bp::list ned_vectors;

		const unsigned int num_vectors = vectors.size();
		for (unsigned int n = 0; n < num_vectors; ++n)
		{
			const GPlatesMaths::Vector3D ned_vector =
					GPlatesMaths::convert_from_geocentric_to_north_east_down(
							GPlatesMaths::CartesianConvMatrix3D(local_origins[n]),
							vectors[n]);

			ned_vectors.append(ned_vector);
		}

		return ned_vectors;
	}

	bp::list
	local_cartesian_convert_sequence_from_north_east_down_to_geocentric(
			bp::object local_origins_object, // Any python sequence (eg, list, tuple).
			bp::object vectors_object) // Any python sequence (eg, list, tuple).
	{
		// Begin/end iterators over the python points sequence.
		bp::stl_input_iterator<GPlatesMaths::PointOnSphere>
				local_origins_begin(local_origins_object),
				local_origins_end;
		// Copy into a vector.
 		std::vector<GPlatesMaths::PointOnSphere> local_origins;
		std::copy(local_origins_begin, local_origins_end, std::back_inserter(local_origins));

		// Begin/end iterators over the python vectors sequence.
		bp::stl_input_iterator<GPlatesMaths::Vector3D>
				vectors_begin(vectors_object),
				vectors_end;
		// Copy into a vector.
 		std::vector<GPlatesMaths::Vector3D> vectors;
		std::copy(vectors_begin, vectors_end, std::back_inserter(vectors));

		if (local_origins.size() != vectors.size())
		{
			PyErr_SetString(PyExc_ValueError, "'local_origins' and 'vectors' should be the same size");
			bp::throw_error_already_set();
		}

		bp::list geocentric_vectors;

		const unsigned int num_vectors = vectors.size();
		for (unsigned int n = 0; n < num_vectors; ++n)
		{
			const GPlatesMaths::Vector3D geocentric_vector =
					GPlatesMaths::convert_from_north_east_down_to_geocentric(
							GPlatesMaths::CartesianConvMatrix3D(local_origins[n]),
							vectors[n]);

			geocentric_vectors.append(geocentric_vector);
		}

		return geocentric_vectors;
	}
}


void
export_local_cartesian()
{
	//
	// LocalCartesian - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesMaths::CartesianConvMatrix3D,
			// A pointer holder is required by 'bp::make_constructor'...
			boost::shared_ptr<GPlatesMaths::CartesianConvMatrix3D>
			// Since it's immutable it can be copied without worrying that a modification from the
			// C++ side will not be visible on the python side, or vice versa. It needs to be
			// copyable anyway so that boost-python can copy it into a shared holder pointer...
#if 0
			boost::noncopyable
#endif
			>(
					"LocalCartesian",
					"A local cartesian coordinate system at a point on the sphere.\n"
					"\n"
					"Local cartesians are equality (``==``, ``!=``) comparable (but not hashable - "
					"cannot be used as a key in a ``dict``).\n",
					bp::init<
						// There are from-python converters from LatLonPoint and sequence(latitude,longitude) and
						// sequence(x,y,z) to PointOnSphere so they will also get matched by this...
						const GPlatesMaths::PointOnSphere &>(
							(bp::arg("local_origin")),
							"__init__(local_origin)\n"
							"  Create a local cartesian coordinate system at a point on the sphere.\n"
							"\n"
							"  :param local_origin: the origin of the local coordinate system\n"
							"  :type local_origin: :class:`PointOnSphere` or :class:`LatLonPoint` or tuple "
							"(latitude,longitude), in degrees, or tuple (x,y,z)\n"
							"\n"
							"  ::\n"
							"\n"
							"    local_cartesian = pygplates.LocalCartesian(local_origin)\n"))

		// NOTE: This should be defined *before* the following (more restrictive overload) since this
		// overload matches generic bp::object. This is because boost-python attempts to resolve
		// overloaded functions in the reverse order in which they're declared...
		.def("convert_from_geocentric_to_north_east_down",
				&GPlatesApi::local_cartesian_convert_sequence_from_geocentric_to_north_east_down,
				(bp::arg("local_origins"), bp::arg("vectors")),
				// General overloaded signature (must be in first overloaded 'def' - used by Sphinx)...
				"convert_from_geocentric_to_north_east_down(...) -> Vector3D\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"[*staticmethod*] This function can be called in more than one way...\n"
				"\n"
				// Specific overload signature...
				"convert_from_geocentric_to_north_east_down(local_origins, vectors) -> list\n"
				"  Converts geocentric *vectors* to cartesian vectors in local "
				"North/East/Down coordinate systems located at *local_origins*.\n"
				"\n"
				"  :param local_origins: sequence of origins of local cartesian systems.\n"
				"  :type local_origins: Any sequence of :class:`PointOnSphere` or :class:`LatLonPoint` or "
				"tuple (latitude,longitude), in degrees, or tuple (x,y,z)\n"
				"  :param vectors: the geocentric vectors\n"
				"  :type vector: Any sequence of :class:`Vector3D` or tuple (x,y,z)\n"
				"  :rtype: list of :class:`Vector3D`\n"
				"\n"
				"  Convert geocentric vectors to local cartesian vectors in corresponding local "
				"North/East/Down coordinate systems (specified as a local origins):\n"
				"  ::\n"
				"\n"
				"    local_origins = [...]\n"
				"    geocentric_vectors = [...]\n"
				"    local_vectors = pygplates.LocalCartesian.convert_from_geocentric_to_north_east_down(\n"
				"        local_origins, geocentric_vectors)\n")
		.def("convert_from_geocentric_to_north_east_down",
				&GPlatesApi::local_cartesian_convert_from_geocentric_to_north_east_down,
				(bp::arg("local_origin"), bp::arg("vector")),
				// Specific overload signature...
				"convert_from_geocentric_to_north_east_down(local_origin, vector) -> Vector3D\n"
				"  Converts a geocentric *vector* to a cartesian vector in the local "
				"North/East/Down coordinate system located at *local_origin*.\n"
				"\n"
				"  :param local_origin: origin of local cartesian system.\n"
				"  :type local_origin: :class:`PointOnSphere` or :class:`LatLonPoint` or tuple (latitude,longitude)"
				", in degrees, or tuple (x,y,z)\n"
				"  :param vector: the geocentric vector\n"
				"  :type vector: :class:`Vector3D`, or sequence (such as list or tuple) of (float,float,float)\n"
				"  :rtype: :class:`Vector3D`\n"
				"\n"
				"  Convert a geocentric vector to the local North/East/Down coordinate system "
				"located at latitude/longitude (0, 0) on the globe:\n"
				"  ::\n"
				"\n"
				"    local_vector = pygplates.LocalCartesian.convert_from_geocentric_to_north_east_down(\n"
				"        (0, 0), geocentric_vector)\n")
		.def("convert_from_geocentric_to_north_east_down",
				&GPlatesApi::local_cartesian_convert_xyz_from_geocentric_to_north_east_down,
				(bp::arg("local_origin"), bp::arg("x"), bp::arg("y"), bp::arg("z")),
				// Specific overload signature...
				"convert_from_geocentric_to_north_east_down(local_origin, x, y, z) -> Vector3D\n"
				"  Converts the geocentric vector (x, y, z) to a cartesian vector in the local "
				"North/East/Down coordinate system located at *local_origin*.\n"
				"\n"
				"  :param local_origin: origin of local cartesian system.\n"
				"  :type local_origin: :class:`PointOnSphere` or :class:`LatLonPoint` or tuple (latitude,longitude)"
				", in degrees, or tuple (x,y,z)\n"
				"  :param x: the *x* component of the geocentric vector\n"
				"  :type x: float\n"
				"  :param y: the *y* component of the geocentric vector\n"
				"  :type y: float\n"
				"  :param z: the *z* component of the geocentric vector\n"
				"  :type z: float\n"
				"  :rtype: :class:`Vector3D`\n"
				"\n"
				"  Convert the geocentric vector (2, 1, 0) to the local North/East/Down coordinate system "
				"located at latitude/longitude (0, 0) on the globe:\n"
				"  ::\n"
				"\n"
				"    local_vector = pygplates.LocalCartesian.convert_from_geocentric_to_north_east_down(\n"
				"        (0,0), 2, 1, 0)\n")
		.staticmethod("convert_from_geocentric_to_north_east_down")
		.def("from_geocentric_to_north_east_down",
				&GPlatesMaths::convert_from_geocentric_to_north_east_down,
				(bp::arg("vector")),
				// General overloaded signature (must be in first overloaded 'def' - used by Sphinx)...
				"from_geocentric_to_north_east_down(...) -> Vector3D\n"
				"This method can be called in more than one way...\n"
				"\n"
				// Specific overload signature...
				"from_geocentric_to_north_east_down(vector) -> Vector3D\n"
				"  Converts the geocentric *vector* to a local North/East/Down cartesian vector.\n"
				"\n"
				"  :param vector: the geocentric vector\n"
				"  :type vector: :class:`Vector3D`, or sequence (such as list or tuple) of (float,float,float)\n"
				"  :rtype: :class:`Vector3D`\n"
				"\n"
				"  Convert a geocentric vector to the local North/East/Down coordinate system "
				"located at latitude/longitude (0, 0) on the globe:\n"
				"  ::\n"
				"\n"
				"    local_cartesian = pygplates.LocalCartesian((0,0))\n"
				"    local_vector = local_cartesian.from_geocentric_to_north_east_down(geocentric_vector)\n")
		.def("from_geocentric_to_north_east_down",
				&GPlatesApi::local_cartesian_xyz_from_geocentric_to_north_east_down,
				(bp::arg("x"), bp::arg("y"), bp::arg("z")),
				// Specific overload signature...
				"from_geocentric_to_north_east_down(x, y, z) -> Vector3D\n"
				"  Converts the geocentric vector (x, y, z) to a local North/East/Down cartesian vector.\n"
				"\n"
				"  :param x: the *x* component of the geocentric vector\n"
				"  :type x: float\n"
				"  :param y: the *y* component of the geocentric vector\n"
				"  :type y: float\n"
				"  :param z: the *z* component of the geocentric vector\n"
				"  :type z: float\n"
				"  :rtype: :class:`Vector3D`\n"
				"\n"
				"  Convert the geocentric vector (2, 1, 0) to the local North/East/Down coordinate system "
				"located at latitude/longitude (0, 0) on the globe:\n"
				"  ::\n"
				"\n"
				"    local_cartesian = pygplates.LocalCartesian((0,0))\n"
				"    local_vector = local_cartesian.from_geocentric_to_north_east_down(2, 1, 0)\n")

		// NOTE: This should be defined *before* the following (more restrictive overload) since this
		// overload matches generic bp::object. This is because boost-python attempts to resolve
		// overloaded functions in the reverse order in which they're declared...
		.def("convert_from_north_east_down_to_geocentric",
				&GPlatesApi::local_cartesian_convert_sequence_from_north_east_down_to_geocentric,
				(bp::arg("local_origins"), bp::arg("vectors")),
				// General overloaded signature (must be in first overloaded 'def' - used by Sphinx)...
				"convert_from_north_east_down_to_geocentric(...) -> Vector3D\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"[*staticmethod*] This function can be called in more than one way...\n"
				"\n"
				// Specific overload signature...
				"convert_from_north_east_down_to_geocentric(local_origins, vectors) -> list\n"
				"  Converts a cartesian *vector* in the local North/East/Down coordinate system "
				"located at *local_origin* to a geocentric vector.\n"
				"\n"
				"  :param local_origins: sequence of origins of local cartesian systems.\n"
				"  :type local_origins: Any sequence of :class:`PointOnSphere` or :class:`LatLonPoint` or "
				"tuple (latitude,longitude), in degrees, or tuple (x,y,z)\n"
				"  :param vectors: the local cartesian vectors\n"
				"  :type vector: Any sequence of :class:`Vector3D` or tuple (x,y,z)\n"
				"  :rtype: list of :class:`Vector3D`\n"
				"\n"
				"  Convert local cartesian vectors in corresponding local North/East/Down coordinate systems "
				"(specified as a local origins) to geocentric vectors:\n"
				"  ::\n"
				"\n"
				"    local_origins = [...]\n"
				"    local_vectors = [...]\n"
				"    geocentric_vectors = pygplates.LocalCartesian.convert_from_north_east_down_to_geocentric(\n"
				"        local_origins, local_vectors)\n")
		.def("convert_from_north_east_down_to_geocentric",
				&GPlatesApi::local_cartesian_convert_from_north_east_down_to_geocentric,
				(bp::arg("local_origin"), bp::arg("vector")),
				// Specific overload signature...
				"convert_from_north_east_down_to_geocentric(local_origin, vector) -> Vector3D\n"
				"  Converts a cartesian vector in the local North/East/Down coordinate system located "
				"at *local_origin* to a geocentric vector.\n"
				"\n"
				"  :param local_origin: origin of local cartesian system.\n"
				"  :type local_origin: :class:`PointOnSphere` or :class:`LatLonPoint` or tuple (latitude,longitude)"
				", in degrees, or tuple (x,y,z)\n"
				"  :param vector: the local cartesian vector\n"
				"  :type vector: :class:`Vector3D`, or sequence (such as list or tuple) of (float,float,float)\n"
				"  :rtype: :class:`Vector3D`\n"
				"\n"
				"  Convert a local cartesian vector in a local North/East/Down coordinate system "
				"located at latitude/longitude (0, 0) on the globe to a geocentric vector:\n"
				"  ::\n"
				"\n"
				"    north_east_down_vector = pygplates.LocalCartesian.convert_from_north_east_down_to_geocentric(\n"
				"        (0, 0), geocentric_vector)\n")
		.def("convert_from_north_east_down_to_geocentric",
				&GPlatesApi::local_cartesian_convert_xyz_from_north_east_down_to_geocentric,
				(bp::arg("local_origin"), bp::arg("x"), bp::arg("y"), bp::arg("z")),
				// Specific overload signature...
				"convert_from_north_east_down_to_geocentric(local_origin, x, y, z) -> Vector3D\n"
				"  Converts a cartesian vector (x, y, z) in the local North/East/Down coordinate system "
				"located at *local_origin* to a geocentric vector.\n"
				"\n"
				"  :param local_origin: origin of local cartesian system.\n"
				"  :type local_origin: :class:`PointOnSphere` or :class:`LatLonPoint` or tuple (latitude,longitude)"
				", in degrees, or tuple (x,y,z)\n"
				"  :param x: the *x* component of the local cartesian vector\n"
				"  :type x: float\n"
				"  :param y: the *y* component of the local cartesian vector\n"
				"  :type y: float\n"
				"  :param z: the *z* component of the local cartesian vector\n"
				"  :type z: float\n"
				"  :rtype: :class:`Vector3D`\n"
				"\n"
				"  Convert the local cartesian vector (2, 1, 0) in the local North/East/Down coordinate system "
				"located at latitude/longitude (0, 0) on the globe to a geocentric vector:\n"
				"  ::\n"
				"\n"
				"    geocentric_vector = pygplates.LocalCartesian.convert_from_north_east_down_to_geocentric(\n"
				"        (0,0), 2, 1, 0)\n")
		.staticmethod("convert_from_north_east_down_to_geocentric")
		.def("from_north_east_down_to_geocentric",
				&GPlatesMaths::convert_from_north_east_down_to_geocentric,
				(bp::arg("vector")),
				// General overloaded signature (must be in first overloaded 'def' - used by Sphinx)...
				"from_north_east_down_to_geocentric(...) -> Vector3D\n"
				"This method can be called in more than one way...\n"
				"\n"
				// Specific overload signature...
				"from_north_east_down_to_geocentric(vector) -> Vector3D\n"
				"  Converts the local North/East/Down cartesian *vector* to a geocentric vector.\n"
				"\n"
				"  :param vector: the local cartesian vector\n"
				"  :type vector: :class:`Vector3D`, or sequence (such as list or tuple) of (float,float,float)\n"
				"  :rtype: :class:`Vector3D`\n"
				"\n"
				"  Convert a cartesian vector in the local North/East/Down coordinate system located "
				"at latitude/longitude (0, 0) on the globe to a geocentric vector:\n"
				"  ::\n"
				"\n"
				"    local_cartesian = pygplates.LocalCartesian((0,0))\n"
				"    geocentric_vector = local_cartesian.from_north_east_down_to_geocentric(local_cartesian_vector)\n")
		.def("from_north_east_down_to_geocentric",
				&GPlatesApi::local_cartesian_xyz_from_north_east_down_to_geocentric,
				(bp::arg("x"), bp::arg("y"), bp::arg("z")),
				// Specific overload signature...
				"from_north_east_down_to_geocentric(x, y, z) -> Vector3D\n"
				"  Converts the local North/East/Down cartesian vector (x, y, z) to a geocentric vector.\n"
				"\n"
				"  :param x: the *x* component of the local cartesian vector\n"
				"  :type x: float\n"
				"  :param y: the *y* component of the local cartesian vector\n"
				"  :type y: float\n"
				"  :param z: the *z* component of the local cartesian vector\n"
				"  :type z: float\n"
				"  :rtype: :class:`Vector3D`\n"
				"\n"
				"  Convert the local cartesian vector (2, 1, 0) in the local North/East/Down coordinate system "
				"located at latitude/longitude (0, 0) on the globe to a geocentric vector:\n"
				"  ::\n"
				"\n"
				"    local_cartesian = pygplates.LocalCartesian((0,0))\n"
				"    geocentric_vector = local_cartesian.from_north_east_down_to_geocentric(2, 1, 0)\n")

		.def("get_north",
				&GPlatesMaths::CartesianConvMatrix3D::north,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_north() -> Vector3D\n"
				"  Returns the North coordinate axis.\n"
				"\n"
				"  :rtype: :class:`Vector3D`\n"
				"\n"
				"  The North axis is the tangential vector (to the unit globe) that is most "
				"Northward pointing. It has unit magnitude.\n"
				"\n"
				"  Get the North axis of the local cartesian system located at latitude/longitude (0, 0) on the globe:\n"
				"  ::\n"
				"\n"
				"    local_cartesian = pygplates.LocalCartesian((0,0))\n"
				"    north = local_cartesian.get_north()\n")
		.def("get_east",
				&GPlatesMaths::CartesianConvMatrix3D::east,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_east() -> Vector3D\n"
				"  Returns the East coordinate axis.\n"
				"\n"
				"  :rtype: :class:`Vector3D`\n"
				"\n"
				"  The East axis is the tangential vector (to the unit globe) that is most "
				"Eastward pointing. It has unit magnitude.\n"
				"\n"
				"  Get the East axis of the local cartesian system located at latitude/longitude (0, 0) on the globe:\n"
				"  ::\n"
				"\n"
				"    local_cartesian = pygplates.LocalCartesian((0,0))\n"
				"    east = local_cartesian.get_east()\n")
		.def("get_down",
				&GPlatesMaths::CartesianConvMatrix3D::down,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_down() -> Vector3D\n"
				"  Returns the Down coordinate axis.\n"
				"\n"
				"  :rtype: :class:`Vector3D`\n"
				"\n"
				"  The Down axis points at the centre of the globe. It has unit magnitude.\n"
				"\n"
				"  Get the Down axis of the local cartesian system located at latitude/longitude (0, 0) on the globe:\n"
				"  ::\n"
				"\n"
				"    local_cartesian = pygplates.LocalCartesian((0,0))\n"
				"    down = local_cartesian.get_down()\n")
		// Comparisons...
		// Due to the numerical tolerance in comparisons we cannot make hashable.
		// Make unhashable, with no *equality* comparison operators (we explicitly define them)...
		.def(GPlatesApi::NoHashDefVisitor(false, true))
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
	;

	// Enable boost::optional<CartesianConvMatrix3D> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesMaths::CartesianConvMatrix3D>();
}

#endif // GPLATES_NO_PYTHON
