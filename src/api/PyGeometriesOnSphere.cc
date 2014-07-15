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

#include <algorithm>
#include <iterator>
#include <vector>
#include <boost/cast.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include "PythonConverterUtils.h"

#include "global/AssertionFailureException.h"
#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/python.h"
// This is not included by <boost/python.hpp>.
// Also we must include this after <boost/python.hpp> which means after "global/python.h".
#include <boost/python/slice.hpp>
#include <boost/python/stl_iterator.hpp>

#include "maths/GeometryOnSphere.h"
#include "maths/LatLonPoint.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"

#include "utils/non_null_intrusive_ptr.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	// Return cloned geometry to python as its derived geometry type.
	bp::object/*derived geometry-on-sphere non_null_intrusive_ptr*/
	geometry_on_sphere_clone(
			const GPlatesMaths::GeometryOnSphere &geometry_on_sphere)
	{
		// The derived geometry-on-sphere type is needed otherwise python is unable to access the derived attributes.
		return PythonConverterUtils::get_geometry_on_sphere_as_derived_type(
				geometry_on_sphere.clone_as_geometry());
	}
}

void
export_geometry_on_sphere()
{
	/*
	 * GeometryOnSphere - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	 *
	 * Base geometry-on-sphere wrapper class.
	 *
	 * Enables 'isinstance(obj, GeometryOnSphere)' in python - not that it's that useful.
	 *
	 * NOTE: We don't normally return a 'GeometryOnSphere::non_null_ptr_to_const_type' to python
	 * because then python is unable to access the attributes of the derived geometry type.
	 * For this reason usually the derived geometry type is returned using:
	 *   'PythonConverterUtils::get_geometry_on_sphere_as_derived_type()'
	 * which returns a 'non_null_ptr_type' to the *derived* geometry type.
	 * UPDATE: Actually boost-python will convert a 'GeometryOnSphere::non_null_ptr_to_const_type' to a reference
	 * to a derived geometry type (eg, 'GeometryOnSphere::non_null_ptr_to_const_type' -> 'const PointOnSphere &')
	 * but it can't convert to a 'non_null_ptr_to_const_type' of derived geometry type
	 * (eg, 'GeometryOnSphere::non_null_ptr_to_const_type' -> 'PointOnSphere::non_null_ptr_to_const_type') so
	 * 'PythonConverterUtils::get_geometry_on_sphere_as_derived_type()' is still needed for that.
	 */
	bp::class_<
			GPlatesMaths::GeometryOnSphere,
			// NOTE: See note in 'python_ConstGeometryOnSphere' for why this is 'non-const' instead of 'const'...
			GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::GeometryOnSphere>,
			boost::noncopyable>(
					"GeometryOnSphere",
					"The base class inherited by all derived classes representing geometries on the sphere.\n"
					"\n"
					"The list of derived geometry on sphere classes is:\n"
					"\n"
					"* :class:`PointOnSphere`\n"
					"* :class:`MultiPointOnSphere`\n"
					"* :class:`PolylineOnSphere`\n"
					"* :class:`PolygonOnSphere`\n",
					bp::no_init)
		.def("clone",
				&GPlatesApi::geometry_on_sphere_clone,
				"clone() -> GeometryOnSphere\n"
				"  Create a duplicate of this geometry (derived) instance.\n"
				"\n"
				"  :rtype: :class:`GeometryOnSphere`\n")
	;

	// Register to-python conversion for GeometryOnSphere::non_null_ptr_to_const_type.
	GPlatesApi::PythonConverterUtils::register_to_python_const_to_non_const_non_null_intrusive_ptr_conversion<
			GPlatesMaths::GeometryOnSphere>();
	// Register from-python 'non-const' to 'const' conversion.
	boost::python::implicitly_convertible<
			GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::GeometryOnSphere>,
			GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::GeometryOnSphere> >();

	// Enable boost::optional<GeometryOnSphere::non_null_ptr_to_const_type> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>();
}


namespace GPlatesApi
{
	/**
	 * Enables LatLonPoint to be passed from python (to a PointOnSphere).
	 *
	 * For more information on boost python to/from conversions, see:
	 *   http://misspent.wordpress.com/2009/09/27/how-to-write-boost-python-converters/
	 */
	struct python_PointOnSphereFromLatLonPoint :
			private boost::noncopyable
	{
		explicit
		python_PointOnSphereFromLatLonPoint()
		{
			namespace bp = boost::python;

			// From python conversion.
			bp::converter::registry::push_back(
					&convertible,
					&construct,
					bp::type_id<GPlatesMaths::PointOnSphere>());
		}

		static
		void *
		convertible(
				PyObject *obj)
		{
			namespace bp = boost::python;

			// PointOnSphere is created from a LatLonPoint.
			return bp::extract<GPlatesMaths::LatLonPoint>(obj).check() ? obj : NULL;
		}

		static
		void
		construct(
				PyObject *obj,
				boost::python::converter::rvalue_from_python_stage1_data *data)
		{
			namespace bp = boost::python;

			void *const storage = reinterpret_cast<
					bp::converter::rvalue_from_python_storage<GPlatesMaths::PointOnSphere> *>(
							data)->storage.bytes;

			new (storage) GPlatesMaths::PointOnSphere(
					make_point_on_sphere(bp::extract<GPlatesMaths::LatLonPoint>(obj)));

			data->convertible = storage;
		}
	};


	/**
	 * Enables a sequence, such as tuple or list, of (x,y,z) or (latitude,longitude)to be passed
	 * from python (to a PointOnSphere).
	 *
	 * For more information on boost python to/from conversions, see:
	 *   http://misspent.wordpress.com/2009/09/27/how-to-write-boost-python-converters/
	 */
	struct python_PointOnSphereFromXYZOrLatLonSequence :
			private boost::noncopyable
	{
		explicit
		python_PointOnSphereFromXYZOrLatLonSequence()
		{
			namespace bp = boost::python;

			// From python conversion.
			bp::converter::registry::push_back(
					&convertible,
					&construct,
					bp::type_id<GPlatesMaths::PointOnSphere>());
		}

		static
		void *
		convertible(
				PyObject *obj)
		{
			namespace bp = boost::python;

			bp::object py_obj = bp::object(bp::handle<>(bp::borrowed(obj)));

			// If the check fails then we'll catch a python exception and return NULL.
			try
			{
				// A sequence containing floats.
				bp::stl_input_iterator<double>
						float_seq_begin(py_obj),
						float_seq_end;

				// Copy into a vector.
				std::vector<double> float_vector;
				std::copy(float_seq_begin, float_seq_end, std::back_inserter(float_vector));
				if (float_vector.size() != 2 && // (lat,lon)
					float_vector.size() != 3)   // (x,y,z)
				{
					return NULL;
				}
			}
			catch (const boost::python::error_already_set &)
			{
				PyErr_Clear();
				return NULL;
			}

			return obj;
		}

		static
		void
		construct(
				PyObject *obj,
				boost::python::converter::rvalue_from_python_stage1_data *data)
		{
			namespace bp = boost::python;

			bp::object py_obj = bp::object(bp::handle<>(bp::borrowed(obj)));

			// A sequence containing floats.
			bp::stl_input_iterator<double>
					float_seq_begin(py_obj),
					float_seq_end;

			// Copy into a vector.
			std::vector<double> float_vector;
			std::copy(float_seq_begin, float_seq_end, std::back_inserter(float_vector));

			void *const storage = reinterpret_cast<
					bp::converter::rvalue_from_python_storage<GPlatesMaths::PointOnSphere> *>(
							data)->storage.bytes;

			if (float_vector.size() == 2)
			{
				// (lat,lon) sequence.
				new (storage) GPlatesMaths::PointOnSphere(
						make_point_on_sphere(
								GPlatesMaths::LatLonPoint(float_vector[0], float_vector[1])));
			}
			else
			{
				// Note that 'convertible' has checked for the correct size of the vector (size either 2 or 3).
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						float_vector.size() == 3,
						GPLATES_ASSERTION_SOURCE);

				// (x,y,z) sequence.
				new (storage) GPlatesMaths::PointOnSphere(
						GPlatesMaths::UnitVector3D(float_vector[0], float_vector[1], float_vector[2]));
			}

			data->convertible = storage;
		}
	};


	// Convenience constructor to create from (x,y,z) without having to first create a UnitVector3D.
	//
	// We can't use bp::make_constructor with a function that returns a non-pointer, so instead we
	// return 'non_null_intrusive_ptr'.
	//
	// With boost 1.42 we get the following compile error...
	//   pointer_holder.hpp:145:66: error: invalid conversion from 'const void*' to 'void*'
	// ...if we return 'GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type' and rely on
	// 'python_ConstGeometryOnSphere' to convert for us - despite the fact that this conversion works
	// successfully for python bindings in other source files. It's possibly due to
	// 'bp::make_constructor' which isn't used for MultiPointOnSphere which has no problem with 'const'.
	//
	// So we avoid it by using returning a pointer to 'non-const' PointOnSphere.
	GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PointOnSphere>
	point_on_sphere_create_xyz(
			const GPlatesMaths::real_t &x,
			const GPlatesMaths::real_t &y,
			const GPlatesMaths::real_t &z)
	{
		return GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PointOnSphere>(
				new GPlatesMaths::PointOnSphere(GPlatesMaths::UnitVector3D(x, y, z)));
	}

	GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PointOnSphere>
	point_on_sphere_create(
			bp::object point_object)
	{
		// There are from-python converters from LatLonPoint and sequence(latitude,longitude) and
		// sequence(x,y,z) so they will also get matched by this...
		bp::extract<GPlatesMaths::PointOnSphere> extract_point_on_sphere(point_object);
		if (extract_point_on_sphere.check())
		{
			return GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PointOnSphere>(
					new GPlatesMaths::PointOnSphere(extract_point_on_sphere()));
		}

		bp::extract<GPlatesMaths::UnitVector3D> extract_unit_vector_3d(point_object);
		if (extract_unit_vector_3d.check())
		{
			return GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PointOnSphere>(
					new GPlatesMaths::PointOnSphere(extract_unit_vector_3d()));
		}

		PyErr_SetString(PyExc_TypeError, "Expected PointOnSphere or UnitVector3D or LatLonPoint or "
				"(latitude,longitude) or (x,y,z)");
		bp::throw_error_already_set();

		// Shouldn't be able to get here.
		return GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PointOnSphere>(NULL);
	}

	GPlatesMaths::Real
	point_on_sphere_get_x(
			const GPlatesMaths::PointOnSphere &point_on_sphere)
	{
		return point_on_sphere.position_vector().x();
	}

	GPlatesMaths::Real
	point_on_sphere_get_y(
			const GPlatesMaths::PointOnSphere &point_on_sphere)
	{
		return point_on_sphere.position_vector().y();
	}

	GPlatesMaths::Real
	point_on_sphere_get_z(
			const GPlatesMaths::PointOnSphere &point_on_sphere)
	{
		return point_on_sphere.position_vector().z();
	}

	bp::tuple
	point_on_sphere_to_xyz(
			const GPlatesMaths::PointOnSphere &point_on_sphere)
	{
		const GPlatesMaths::UnitVector3D &position_vector = point_on_sphere.position_vector();

		return bp::make_tuple(position_vector.x(), position_vector.y(), position_vector.z());
	}

	bp::tuple
	point_on_sphere_to_lat_lon(
			const GPlatesMaths::PointOnSphere &point_on_sphere)
	{
		const GPlatesMaths::LatLonPoint lat_lon_point = make_lat_lon_point(point_on_sphere);

		return bp::make_tuple(lat_lon_point.latitude(), lat_lon_point.longitude());
	}
}

void
export_point_on_sphere()
{
	//
	// PointOnSphere - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesMaths::PointOnSphere,
			// NOTE: See note in 'python_ConstGeometryOnSphere' for why this is 'non-const' instead of 'const'...
			GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PointOnSphere>,
			bp::bases<GPlatesMaths::GeometryOnSphere>
			// NOTE: PointOnSphere is the only GeometryOnSphere type that is copyable.
			// And it needs to be copyable (in our wrapper) so that, eg, MultiPointOnSphere's iterator
			// can copy raw PointOnSphere elements (in its vector) into 'non_null_intrusive_ptr' versions
			// of PointOnSphere as it iterates over its collection (this conversion is one of those
			// cool things that boost-python takes care of automatically).
			//
			// Also note that PointOnSphere (like all GeometryOnSphere types) is immutable so the fact
			// that it can be copied into a python object (unlike the other GeometryOnSphere types) does
			// not mean we have to worry about a PointOnSphere modification from the C++ side not being
			// visible on the python side, or vice versa (because cannot modify since immutable - or at least
			// the python interface is immutable - the C++ has an assignment operator we should be careful of).
#if 0
			boost::noncopyable
#endif
			>(
					"PointOnSphere",
					"Represents a point on the surface of the unit length sphere in 3D cartesian coordinates.\n"
					"\n"
					"Points are equality (``==``, ``!=``) comparable where two points are considered "
					"equal if their coordinates match within a *very* small numerical epsilon that accounts "
					"for the limits of floating-point precision. Note that usually two points will only compare "
					"equal if they are the same point or created from the exact same input data. If two "
					"points are generated in two different ways (eg, two different processing paths) they "
					"will most likely *not* compare equal even if mathematically they should be identical.\n"
					"\n"
					"Note that since a *PointOnSphere* is immutable it contains no operations or "
					"methods that modify its state.\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::point_on_sphere_create_xyz,
						bp::default_call_policies(),
						(bp::arg("x"), bp::arg("y"), bp::arg("z"))),
				"__init__(x, y, z)\n"
				"  Create a *PointOnSphere* instance from a 3D cartesian coordinate consisting of "
				"floating-point coordinates *x*, *y* and *z*.\n"
				"\n"
				"  :param x: the *x* component of the 3D unit vector\n"
				"  :type x: float\n"
				"  :param y: the *y* component of the 3D unit vector\n"
				"  :type y: float\n"
				"  :param z: the *z* component of the 3D unit vector\n"
				"  :type z: float\n"
				"  :raises: ViolatedUnitVectorInvariantError if resulting vector does not have unit magnitude\n"
				"\n"
				"  **NOTE:** The length of 3D vector (x,y,z) must be 1.0, otherwise "
				"*ViolatedUnitVectorInvariantError* is raised.\n"
				"  ::\n"
				"\n"
				"    point = pygplates.PointOnSphere(x, y, z)\n")
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::point_on_sphere_create,
						bp::default_call_policies(),
						(bp::arg("point"))),
				"__init__(point)\n"
				"  Create a *PointOnSphere* instance from a (x,y,z) or (latitude,longitude) point.\n"
				"\n"
				"  :param point: (x,y,z) point, or (latitude,longitude) point (in degrees)\n"
				"  :type point: :class:`PointOnSphere` or :class:`UnitVector3D` or :class:`LatLonPoint` or "
				"(float,float,float) or (float,float)\n"
				"  :raises: InvalidLatLonError if *latitude* or *longitude* is invalid\n"
				"  :raises: ViolatedUnitVectorInvariantError if (x,y,z) is not unit magnitude\n"
				"\n"
				"  The following example shows a few different ways to use this method:\n"
				"  ::\n"
				"\n"
				"    point = pygplates.PointOnSphere(pygplates.UnitVector3D(x,y,z))\n"
				"    point = pygplates.PointOnSphere(pygplates.PointOnSphere(x,y,z))\n"
				"    point = pygplates.PointOnSphere((x,y,z))\n"
				"    point = pygplates.PointOnSphere([x,y,z])\n"
				"    point = pygplates.PointOnSphere(numpy.array([x,y,z]))\n"
				"    point = pygplates.PointOnSphere(pygplates.LatLonPoint(latitude,longitude))\n"
				"    point = pygplates.PointOnSphere((latitude,longitude))\n"
				"    point = pygplates.PointOnSphere([latitude,longitude])\n"
				"    point = pygplates.PointOnSphere(numpy.array([latitude,longitude]))\n")
		.def("get_position_vector",
				&GPlatesMaths::PointOnSphere::position_vector,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_position_vector() -> UnitVector3D\n"
				"  Returns the position on the unit sphere as a unit length vector.\n"
				"\n"
				"  :rtype: :class:`UnitVector3D`\n")
		.def("get_x",
				&GPlatesApi::point_on_sphere_get_x,
				"get_x() -> float\n"
				"  Returns the *x* coordinate.\n"
				"\n"
				"  :rtype: float\n")
		.def("get_y",
				&GPlatesApi::point_on_sphere_get_y,
				"get_y() -> float\n"
				"  Returns the *y* coordinate.\n"
				"\n"
				"  :rtype: float\n")
		.def("get_z",
				&GPlatesApi::point_on_sphere_get_z,
				"get_z() -> float\n"
				"  Returns the *z* coordinate.\n"
				"\n"
				"  :rtype: float\n")
		.def("to_xyz",
				&GPlatesApi::point_on_sphere_to_xyz,
				"to_xyz() -> x, y, z\n"
				"  Returns the cartesian coordinates as the tuple (x,y,z).\n"
				"\n"
				"  :rtype: the tuple (float,float,float)\n"
				"\n"
				"  ::\n"
				"\n"
				"    x, y, z = point.to_xyz()\n")
		.def("to_lat_lon_point",
				&GPlatesMaths::make_lat_lon_point,
				"to_lat_lon_point() -> LatLonPoint\n"
				"  Returns the (latitude,longitude) equivalent of this :class:`PointOnSphere`.\n"
				"\n"
				"  :rtype: :class:`LatLonPoint`\n")
		.def("to_lat_lon",
				&GPlatesApi::point_on_sphere_to_lat_lon,
				"to_lat_lon() -> latitude, longitude\n"
				"  Returns the tuple (latitude,longitude) in degrees.\n"
				"\n"
				"  :rtype: the tuple (float, float)\n"
				"\n"
				"  ::\n"
				"\n"
				"    latitude, longitude = point.to_lat_lon()\n"
				"\n"
				"  This is similar to :meth:`LatLonPoint.to_lat_lon`.\n")
		// Since we're defining '__eq__' we need to define a compatible '__hash__' or make it unhashable.
		// This is because the default '__hash__'is based on 'id()' which is not compatible and
		// would cause errors when used as key in a dictionary.
		// In python 3 fixes this by automatically making unhashable if define '__eq__' only.
		.setattr("__hash__", bp::object()/*None*/) // make unhashable
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
		// Generate '__str__' from 'operator<<'...
		// Note: Seems we need to qualify with 'self_ns::' to avoid MSVC compile error.
		.def(bp::self_ns::str(bp::self))
	;

	// Register to-python conversion for PointOnSphere::non_null_ptr_to_const_type.
	//
	// PointOnSphere is a bit of an exception to the immutable rule since it can be allocated on
	// the stack or heap (unlike the other geometry types) and it has an assignment operator.
	// However we treat it the same as the other geometry types.
	GPlatesApi::PythonConverterUtils::register_to_python_const_to_non_const_non_null_intrusive_ptr_conversion<
			GPlatesMaths::PointOnSphere>();
	// Register from-python 'non-const' to 'const' conversion.
	boost::python::implicitly_convertible<
			GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PointOnSphere>,
			GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::PointOnSphere> >();

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Note that we're only dealing with 'const' conversions here.
	// The 'non-const' workaround to get boost-python to compile is handled by 'python_ConstGeometryOnSphere'.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			const GPlatesMaths::PointOnSphere,
			const GPlatesMaths::GeometryOnSphere>();

	// Registers the from-python converter from GPlatesMaths::LatLonPoint.
	GPlatesApi::python_PointOnSphereFromLatLonPoint();

	// Registers the from-python converter from a (x,y,z) or (lat,lon) sequence.
	GPlatesApi::python_PointOnSphereFromXYZOrLatLonSequence();
}


namespace GPlatesApi
{
	// Create a multi-point from a sequence of points.
	GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::MultiPointOnSphere>
	multi_point_on_sphere_create(
			bp::object points) // Any python sequence (eg, list, tuple).
	{
		// Begin/end iterators over the python points sequence.
		bp::stl_input_iterator<GPlatesMaths::PointOnSphere>
				points_begin(points),
				points_end;

		// Copy into a vector.
		// Note: We can't pass the iterators directly to 'MultiPointOnSphere::create_on_heap'
		// because they are *input* iterators (ie, one pass only) and 'MultiPointOnSphere' expects
		// forward iterators (it actually makes two passes over the points).
 		std::vector<GPlatesMaths::PointOnSphere> points_vector;
		std::copy(points_begin, points_end, std::back_inserter(points_vector));

		// With boost 1.42 we get the following compile error...
		//   pointer_holder.hpp:145:66: error: invalid conversion from 'const void*' to 'void*'
		// ...if we return 'GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type' and rely on
		// 'python_ConstGeometryOnSphere' to convert for us - despite the fact that this conversion works
		// successfully for python bindings in other source files. It's likely due to 'bp::make_constructor'.
		//
		// So we avoid it by using returning a pointer to 'non-const' MultiPointOnSphere.
		return GPlatesUtils::const_pointer_cast<GPlatesMaths::MultiPointOnSphere>(
				GPlatesMaths::MultiPointOnSphere::create_on_heap(
						points_vector.begin(),
						points_vector.end()));
	}

	bool
	multi_point_on_sphere_contains_point(
			const GPlatesMaths::MultiPointOnSphere &multi_point_on_sphere,
			// There are from-python converters from LatLonPoint and sequence(latitude,longitude) and
			// sequence(x,y,z) to PointOnSphere so they will also get matched by this...
			const GPlatesMaths::PointOnSphere &point_on_sphere)
	{
		return std::find(
				multi_point_on_sphere.begin(),
				multi_point_on_sphere.end(),
				point_on_sphere)
						!= multi_point_on_sphere.end();
	}

	//
	// Support for "__get_item__".
	//
	boost::python::object
	multi_point_on_sphere_get_item(
			const GPlatesMaths::MultiPointOnSphere &multi_point_on_sphere,
			boost::python::object i)
	{
		namespace bp = boost::python;

		// Set if the index is a slice object.
		bp::extract<bp::slice> extract_slice(i);
		if (extract_slice.check())
		{
			bp::list slice_list;

			try
			{
				// Use boost::python::slice to manage index variations such as negative indices or
				// indices that are None.
				bp::slice slice = extract_slice();
				bp::slice::range<GPlatesMaths::MultiPointOnSphere::const_iterator> slice_range =
						slice.get_indicies(multi_point_on_sphere.begin(), multi_point_on_sphere.end());

				GPlatesMaths::MultiPointOnSphere::const_iterator iter = slice_range.start;
				for ( ; iter != slice_range.stop; std::advance(iter, slice_range.step))
				{
					slice_list.append(*iter);
				}
				slice_list.append(*iter);
			}
			catch (std::invalid_argument)
			{
				// Invalid slice - return empty list.
				return bp::list();
			}

			return slice_list;
		}

		// See if the index is an integer.
		bp::extract<long> extract_index(i);
		if (extract_index.check())
		{
			long index = extract_index();
			if (index < 0)
			{
				index += multi_point_on_sphere.number_of_points();
			}

			if (index >= boost::numeric_cast<long>(multi_point_on_sphere.number_of_points()) ||
				index < 0)
			{
				PyErr_SetString(PyExc_IndexError, "Index out of range");
				bp::throw_error_already_set();
			}

			GPlatesMaths::MultiPointOnSphere::const_iterator iter = multi_point_on_sphere.begin();
			std::advance(iter, index); // Should be fast since 'iter' is random access.
			const GPlatesMaths::PointOnSphere &point = *iter;

			return bp::object(point);
		}

		PyErr_SetString(PyExc_TypeError, "Invalid index type");
		bp::throw_error_already_set();

		return bp::object();
	}

	GPlatesMaths::PointOnSphere
	multi_point_on_sphere_get_centroid(
			const GPlatesMaths::MultiPointOnSphere &multi_point_on_sphere)
	{
		return GPlatesMaths::PointOnSphere(multi_point_on_sphere.get_centroid());
	}
}

void
export_multi_point_on_sphere()
{
	//
	// MultiPointOnSphere - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesMaths::MultiPointOnSphere,
			// NOTE: See note in 'python_ConstGeometryOnSphere' for why this is 'non-const' instead of 'const'...
			GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::MultiPointOnSphere>,
			bp::bases<GPlatesMaths::GeometryOnSphere>,
			boost::noncopyable>(
					"MultiPointOnSphere",
					"Represents a multi-point (collection of points) on the surface of the unit length sphere. "
					"Multi-points are equality (``==``, ``!=``) comparable - see :class:`PointOnSphere` "
					"for an overview of equality in the presence of limited floating-point precision.\n"
					"\n"
					"A multi-point instance is iterable over its points:\n"
					"::\n"
					"\n"
					"  multi_point = pygplates.MultiPointOnSphere(points)\n"
					"  for i, point in enumerate(multi_point):\n"
					"      assert(point == multi_point[i])\n"
					"\n"
					"The following operations for accessing the points are supported:\n"
					"\n"
					"=========================== ==========================================================\n"
					"Operation                   Result\n"
					"=========================== ==========================================================\n"
					"``len(mp)``                 number of points in *mp*\n"
					"``for p in mp``             iterates over the points *p* of multi-point *mp*\n"
					"``p in mp``                 ``True`` if *p* is equal to a point in *mp*\n"
					"``p not in mp``             ``False`` if *p* is equal to a point in *mp*\n"
					"``mp[i]``                   the point of *mp* at index *i*\n"
					"``mp[i:j]``                 slice of *mp* from *i* to *j*\n"
					"``mp[i:j:k]``               slice of *mp* from *i* to *j* with step *k*\n"
					"=========================== ==========================================================\n"
					"\n"
					"Note that since a *MultiPointOnSphere* is immutable it contains no operations or "
					"methods that modify its state (such as adding or removing points).\n"
					"\n"
					"There are also methods that return the sequence of points as (latitude,longitude) "
					"values and (x,y,z) values contained in lists and numpy arrays "
					"(:meth:`GeometryOnSphere.to_lat_lon_list`, :meth:`GeometryOnSphere.to_lat_lon_array`, "
					":meth:`GeometryOnSphere.to_xyz_list` and :meth:`GeometryOnSphere.to_xyz_array`).\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::multi_point_on_sphere_create,
						bp::default_call_policies(),
						(bp::arg("points"))),
				"__init__(points)\n"
				"  Create a multi-point from a sequence of (x,y,z) or (latitude,longitude) points.\n"
				"\n"
				"  :param points: A sequence of (x,y,z) points, or (latitude,longitude) points (in degrees).\n"
				"  :type points: Any sequence of :class:`PointOnSphere` or :class:`UnitVector3D` or "
				":class:`LatLonPoint` or (float,float,float) or (float,float)\n"
				"  :raises: InvalidLatLonError if any *latitude* or *longitude* is invalid\n"
				"  :raises: ViolatedUnitVectorInvariantError if any (x,y,z) is not unit magnitude\n"
				"  :raises: InsufficientPointsForMultiPointConstructionError if point sequence is empty\n"
				"\n"
				"  **NOTE** that the sequence must contain at least one point, otherwise "
				"*InsufficientPointsForMultiPointConstructionError* will be raised.\n"
				"\n"
				"  The following example shows a few different ways to create a :class:`multi-point<MultiPointOnSphere>`:\n"
				"  ::\n"
				"\n"
				"    points = []\n"
				"    points.append(pygplates.PointOnSphere(...))\n"
				"    points.append(pygplates.PointOnSphere(...))\n"
				"    points.append(pygplates.PointOnSphere(...))\n"
				"    multi_point = pygplates.MultiPointOnSphere(points)\n"
				"    \n"
				"    points = []\n"
				"    points.append((lat1,lon1))\n"
				"    points.append((lat2,lon2))\n"
				"    points.append((lat3,lon3))\n"
				"    multi_point = pygplates.MultiPointOnSphere(points)\n"
				"    \n"
				"    points = []\n"
				"    points.append([x1,y1,z1])\n"
				"    points.append([x2,y2,z2])\n"
				"    points.append([x3,y3,z3])\n"
				"    multi_point = pygplates.MultiPointOnSphere(points)\n"
				"\n"
				"  If you have latitude/longitude values but they are not a sequence of tuples or "
				"if the latitude/longitude order is swapped then the following examples demonstrate "
				"how you could restructure them:\n"
				"  ::\n"
				"\n"
				"    # Flat lat/lon array.\n"
				"    points = numpy.array([lat1, lon1, lat2, lon2, lat3, lon3])\n"
				"    multi_point = pygplates.MultiPointOnSphere(zip(points[::2],points[1::2]))\n"
				"    \n"
				"    # Flat lon/lat list (ie, different latitude/longitude order).\n"
				"    points = [lon1, lat1, lon2, lat2, lon3, lat3]\n"
				"    multi_point = pygplates.MultiPointOnSphere(zip(points[1::2],points[::2]))\n"
				"    \n"
				"    # Separate lat/lon arrays.\n"
				"    lats = numpy.array([lat1, lat2, lat3])\n"
				"    lons = numpy.array([lon1, lon2, lon3])\n"
				"    multi_point = pygplates.MultiPointOnSphere(zip(lats,lons))\n"
				"    \n"
				"    # Lon/lat list of tuples (ie, different latitude/longitude order).\n"
				"    points = [(lon1, lat1), (lon2, lat2), (lon3, lat3)]\n"
				"    multi_point = pygplates.MultiPointOnSphere([(lat,lon) for lon, lat in points])\n")
		.def("get_centroid",
				&GPlatesApi::multi_point_on_sphere_get_centroid,
				"get_centroid() -> PointOnSphere\n"
				"  Returns the centroid of this multi-point.\n"
				"\n"
				"  :rtype: :class:`PointOnSphere`\n"
				"\n"
				"  The centroid is calculated as the sum of the points of the multi-point with the "
				"result normalized to a vector of unit length.\n")
		.def("__iter__", bp::iterator<const GPlatesMaths::MultiPointOnSphere>())
		.def("__len__", &GPlatesMaths::MultiPointOnSphere::number_of_points)
		.def("__contains__", &GPlatesApi::multi_point_on_sphere_contains_point)
		.def("__getitem__", &GPlatesApi::multi_point_on_sphere_get_item)
		// Since we're defining '__eq__' we need to define a compatible '__hash__' or make it unhashable.
		// This is because the default '__hash__'is based on 'id()' which is not compatible and
		// would cause errors when used as key in a dictionary.
		// In python 3 fixes this by automatically making unhashable if define '__eq__' only.
		.setattr("__hash__", bp::object()/*None*/) // make unhashable
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
	;

	// Register to-python conversion for MultiPointOnSphere::non_null_ptr_to_const_type.
	GPlatesApi::PythonConverterUtils::register_to_python_const_to_non_const_non_null_intrusive_ptr_conversion<
			GPlatesMaths::MultiPointOnSphere>();
	// Register from-python 'non-const' to 'const' conversion.
	boost::python::implicitly_convertible<
			GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::MultiPointOnSphere>,
			GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::MultiPointOnSphere> >();

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Note that we're only dealing with 'const' conversions here.
	// The 'non-const' workaround to get boost-python to compile is handled by 'python_ConstGeometryOnSphere'.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			const GPlatesMaths::MultiPointOnSphere,
			const GPlatesMaths::GeometryOnSphere>();
}


namespace GPlatesApi
{
	// Create a polyline/polygon from a sequence of points.
	template <class PolyGeometryOnSphereType>
	GPlatesUtils::non_null_intrusive_ptr<PolyGeometryOnSphereType>
	poly_geometry_on_sphere_create(
			bp::object points) // Any python sequence (eg, list, tuple).
	{
		// Begin/end iterators over the python points sequence.
		bp::stl_input_iterator<GPlatesMaths::PointOnSphere>
				points_begin(points),
				points_end;

		// Copy into a vector.
		// Note: We can't pass the iterators directly to 'PolylineOnSphere::create_on_heap' or
		// 'PolygonOnSphere::create_on_heap' because they are *input* iterators (ie, one pass only)
		// and 'PolylineOnSphere/PolygonOnSphere' expects forward iterators (they actually makes
		// two passes over the points).
 		std::vector<GPlatesMaths::PointOnSphere> points_vector;
		std::copy(points_begin, points_end, std::back_inserter(points_vector));

		// With boost 1.42 we get the following compile error...
		//   pointer_holder.hpp:145:66: error: invalid conversion from 'const void*' to 'void*'
		// ...if we return 'GPlatesMaths::PolyGeometryOnSphereType::non_null_ptr_to_const_type' and rely on
		// 'python_ConstGeometryOnSphere' to convert for us - despite the fact that this conversion works
		// successfully for python bindings in other source files. It's likely due to 'bp::make_constructor'.
		//
		// So we avoid it by using returning a pointer to 'non-const' PolyGeometryOnSphereType.
		return GPlatesUtils::const_pointer_cast<PolyGeometryOnSphereType>(
				PolyGeometryOnSphereType::create_on_heap(
						points_vector.begin(),
						points_vector.end()));
	}

	/**
	 * Wrapper class for functions accessing the *points* of a PolylineOnSphere/PolygonOnSphere.
	 *
	 * This is a view into the internal points of a PolylineOnSphere/PolygonOnSphere in that an
	 * iterator can be obtained from the view and the view supports indexing.
	 */
	template <class PolyGeometryOnSphereType>
	class PolyGeometryOnSpherePointsView
	{
	public:
		explicit
		PolyGeometryOnSpherePointsView(
				typename PolyGeometryOnSphereType::non_null_ptr_to_const_type poly_geometry_on_sphere) :
			d_poly_geometry_on_sphere(poly_geometry_on_sphere)
		{  }

		typedef typename PolyGeometryOnSphereType::vertex_const_iterator const_iterator;

		const_iterator
		begin() const
		{
			return d_poly_geometry_on_sphere->vertex_begin();
		}

		const_iterator
		end() const
		{
			return d_poly_geometry_on_sphere->vertex_end();
		}

		typename PolyGeometryOnSphereType::size_type
		get_number_of_points() const
		{
			return d_poly_geometry_on_sphere->number_of_vertices();
		}

		bool
		contains_point(
				const GPlatesMaths::PointOnSphere &point_on_sphere) const
		{
			return std::find(
					d_poly_geometry_on_sphere->vertex_begin(),
					d_poly_geometry_on_sphere->vertex_end(),
					point_on_sphere)
							!= d_poly_geometry_on_sphere->vertex_end();
		}

		//
		// Support for "__get_item__".
		//
		boost::python::object
		get_item(
				boost::python::object i) const
		{
			namespace bp = boost::python;

			// Set if the index is a slice object.
			bp::extract<bp::slice> extract_slice(i);
			if (extract_slice.check())
			{
				bp::list slice_list;

				try
				{
					// Use boost::python::slice to manage index variations such as negative indices or
					// indices that are None.
					bp::slice slice = extract_slice();
					bp::slice::range<typename PolyGeometryOnSphereType::vertex_const_iterator> slice_range =
							slice.get_indicies(
									d_poly_geometry_on_sphere->vertex_begin(),
									d_poly_geometry_on_sphere->vertex_end());

					typename PolyGeometryOnSphereType::vertex_const_iterator iter = slice_range.start;
					for ( ; iter != slice_range.stop; std::advance(iter, slice_range.step))
					{
						slice_list.append(*iter);
					}
					slice_list.append(*iter);
				}
				catch (std::invalid_argument)
				{
					// Invalid slice - return empty list.
					return bp::list();
				}

				return slice_list;
			}

			// See if the index is an integer.
			bp::extract<long> extract_index(i);
			if (extract_index.check())
			{
				long index = extract_index();
				if (index < 0)
				{
					index += d_poly_geometry_on_sphere->number_of_vertices();
				}

				if (index >= boost::numeric_cast<long>(d_poly_geometry_on_sphere->number_of_vertices()) ||
					index < 0)
				{
					PyErr_SetString(PyExc_IndexError, "Index out of range");
					bp::throw_error_already_set();
				}

				typename PolyGeometryOnSphereType::vertex_const_iterator iter =
						d_poly_geometry_on_sphere->vertex_begin();
				std::advance(iter, index); // Should be fast since 'iter' is random access.
				const GPlatesMaths::PointOnSphere &point = *iter;

				return bp::object(point);
			}

			PyErr_SetString(PyExc_TypeError, "Invalid index type");
			bp::throw_error_already_set();

			return bp::object();
		}

	private:
		typename PolyGeometryOnSphereType::non_null_ptr_to_const_type d_poly_geometry_on_sphere;
	};

	template <class PolyGeometryOnSphereType>
	PolyGeometryOnSpherePointsView<PolyGeometryOnSphereType>
	poly_geometry_on_sphere_get_points_view(
			typename PolyGeometryOnSphereType::non_null_ptr_to_const_type poly_geometry_on_sphere)
	{
		return PolyGeometryOnSpherePointsView<PolyGeometryOnSphereType>(poly_geometry_on_sphere);
	}


	/**
	 * Wrapper class for functions accessing the *great circle arcs* of a PolylineOnSphere/PolygonOnSphere.
	 *
	 * This is a view into the internal arcs of a PolylineOnSphere/PolygonOnSphere in that an
	 * iterator can be obtained from the view and the view supports indexing.
	 */
	template <class PolyGeometryOnSphereType>
	class PolyGeometryOnSphereArcsView
	{
	public:
		explicit
		PolyGeometryOnSphereArcsView(
				typename PolyGeometryOnSphereType::non_null_ptr_to_const_type poly_geometry_on_sphere) :
			d_poly_geometry_on_sphere(poly_geometry_on_sphere)
		{  }

		typedef typename PolyGeometryOnSphereType::const_iterator const_iterator;

		const_iterator
		begin() const
		{
			return d_poly_geometry_on_sphere->begin();
		}

		const_iterator
		end() const
		{
			return d_poly_geometry_on_sphere->end();
		}

		typename PolyGeometryOnSphereType::size_type
		get_number_of_arcs() const
		{
			return d_poly_geometry_on_sphere->number_of_segments();
		}

		bool
		contains_arc(
				const GPlatesMaths::GreatCircleArc &gca) const
		{
			return std::find(
					d_poly_geometry_on_sphere->begin(),
					d_poly_geometry_on_sphere->end(),
					gca)
							!= d_poly_geometry_on_sphere->end();
		}

		//
		// Support for "__get_item__".
		//
		boost::python::object
		get_item(
				boost::python::object i) const
		{
			namespace bp = boost::python;

			// Set if the index is a slice object.
			bp::extract<bp::slice> extract_slice(i);
			if (extract_slice.check())
			{
				bp::list slice_list;

				try
				{
					// Use boost::python::slice to manage index variations such as negative indices or
					// indices that are None.
					bp::slice slice = extract_slice();
					bp::slice::range<typename PolyGeometryOnSphereType::const_iterator> slice_range =
							slice.get_indicies(
									d_poly_geometry_on_sphere->begin(),
									d_poly_geometry_on_sphere->end());

					typename PolyGeometryOnSphereType::const_iterator iter = slice_range.start;
					for ( ; iter != slice_range.stop; std::advance(iter, slice_range.step))
					{
						slice_list.append(*iter);
					}
					slice_list.append(*iter);
				}
				catch (std::invalid_argument)
				{
					// Invalid slice - return empty list.
					return bp::list();
				}

				return slice_list;
			}

			// See if the index is an integer.
			bp::extract<long> extract_index(i);
			if (extract_index.check())
			{
				long index = extract_index();
				if (index < 0)
				{
					index += d_poly_geometry_on_sphere->number_of_segments();
				}

				if (index >= boost::numeric_cast<long>(d_poly_geometry_on_sphere->number_of_segments()) ||
					index < 0)
				{
					PyErr_SetString(PyExc_IndexError, "Index out of range");
					bp::throw_error_already_set();
				}

				typename PolyGeometryOnSphereType::const_iterator iter = d_poly_geometry_on_sphere->begin();
				std::advance(iter, index); // Should be fast since 'iter' is random access.
				const GPlatesMaths::GreatCircleArc &gca = *iter;

				return bp::object(gca);
			}

			PyErr_SetString(PyExc_TypeError, "Invalid index type");
			bp::throw_error_already_set();

			return bp::object();
		}

	private:
		typename PolyGeometryOnSphereType::non_null_ptr_to_const_type d_poly_geometry_on_sphere;
	};

	template <class PolyGeometryOnSphereType>
	PolyGeometryOnSphereArcsView<PolyGeometryOnSphereType>
	poly_geometry_on_sphere_get_arcs_view(
			typename PolyGeometryOnSphereType::non_null_ptr_to_const_type poly_geometry_on_sphere)
	{
		return PolyGeometryOnSphereArcsView<PolyGeometryOnSphereType>(poly_geometry_on_sphere);
	}

	GPlatesMaths::PointOnSphere
	polyline_on_sphere_get_centroid(
			const GPlatesMaths::PolylineOnSphere &polyline_on_sphere)
	{
		return GPlatesMaths::PointOnSphere(polyline_on_sphere.get_centroid());
	}

	bool
	polyline_on_sphere_contains_point(
			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere,
			// There are from-python converters from LatLonPoint and sequence(latitude,longitude) and
			// sequence(x,y,z) to PointOnSphere so they will also get matched by this...
			const GPlatesMaths::PointOnSphere &point_on_sphere)
	{
		PolyGeometryOnSpherePointsView<GPlatesMaths::PolylineOnSphere> points_view(polyline_on_sphere);

		return points_view.contains_point(point_on_sphere);
	}

	//
	// Support for "__get_item__".
	//
	boost::python::object
	polyline_on_sphere_get_item(
			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere,
			boost::python::object i)
	{
		PolyGeometryOnSpherePointsView<GPlatesMaths::PolylineOnSphere> points_view(polyline_on_sphere);

		return points_view.get_item(i);
	}
}


void
export_polyline_on_sphere()
{
	//
	// A wrapper around view access to the *points* of a PolylineOnSphere.
	//
	// We don't document this wrapper (using docstrings) since it's documented in "PolylineOnSphere".
	bp::class_< GPlatesApi::PolyGeometryOnSpherePointsView<GPlatesMaths::PolylineOnSphere> >(
			"PolylineOnSpherePointsView",
			bp::no_init)
		.def("__iter__",
				bp::iterator< const GPlatesApi::PolyGeometryOnSpherePointsView<GPlatesMaths::PolylineOnSphere> >())
		.def("__len__",
				&GPlatesApi::PolyGeometryOnSpherePointsView<GPlatesMaths::PolylineOnSphere>::get_number_of_points)
		.def("__contains__",
				&GPlatesApi::PolyGeometryOnSpherePointsView<GPlatesMaths::PolylineOnSphere>::contains_point)
		.def("__getitem__",
				&GPlatesApi::PolyGeometryOnSpherePointsView<GPlatesMaths::PolylineOnSphere>::get_item)
	;

	//
	// A wrapper around view access to the *great circle arcs* of a PolylineOnSphere.
	//
	// We don't document this wrapper (using docstrings) since it's documented in "PolylineOnSphere".
	bp::class_< GPlatesApi::PolyGeometryOnSphereArcsView<GPlatesMaths::PolylineOnSphere> >(
			"PolylineOnSphereArcsView",
			bp::no_init)
		.def("__iter__",
				bp::iterator< const GPlatesApi::PolyGeometryOnSphereArcsView<GPlatesMaths::PolylineOnSphere> >())
		.def("__len__",
				&GPlatesApi::PolyGeometryOnSphereArcsView<GPlatesMaths::PolylineOnSphere>::get_number_of_arcs)
		.def("__contains__",
				&GPlatesApi::PolyGeometryOnSphereArcsView<GPlatesMaths::PolylineOnSphere>::contains_arc)
		.def("__getitem__",
				&GPlatesApi::PolyGeometryOnSphereArcsView<GPlatesMaths::PolylineOnSphere>::get_item)
	;

	//
	// PolylineOnSphere - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesMaths::PolylineOnSphere,
			// NOTE: See note in 'python_ConstGeometryOnSphere' for why this is 'non-const' instead of 'const'...
			GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PolylineOnSphere>,
			bp::bases<GPlatesMaths::GeometryOnSphere>,
			boost::noncopyable>(
					"PolylineOnSphere",
					"Represents a polyline on the surface of the unit length sphere. "
					"Polylines are equality (``==``, ``!=``) comparable - see :class:`PointOnSphere` "
					"for an overview of equality in the presence of limited floating-point precision.\n"
					"\n"
					"A polyline instance provides two types of *views*:\n"
					"\n"
					"* a view of its points - see :meth:`get_points_view`, and\n"
					"* a view of its *great circle arc* subsegments (between adjacent points) - see "
					":meth:`get_great_circle_arcs_view`.\n"
					"\n"
					"In addition a polyline instance is directly iterable over its points:\n"
					"::\n"
					"\n"
					"  polyline = pygplates.PolylineOnSphere(points)\n"
					"  for i, point in enumerate(polyline):\n"
					"      assert(point == polyline[i])\n"
					"\n"
					"...and so the following operations for accessing the points are supported:\n"
					"\n"
					"=========================== ==========================================================\n"
					"Operation                   Result\n"
					"=========================== ==========================================================\n"
					"``len(polyline)``           number of points in *polyline*\n"
					"``for p in polyline``       iterates over the points *p* of *polyline*\n"
					"``p in polyline``           ``True`` if *p* is equal to a point in *polyline*\n"
					"``p not in polyline``       ``False`` if *p* is equal to a point in *polyline*\n"
					"``polyline[i]``             the point of *polyline* at index *i*\n"
					"``polyline[i:j]``           slice of *polyline* from *i* to *j*\n"
					"``polyline[i:j:k]``         slice of *polyline* from *i* to *j* with step *k*\n"
					"=========================== ==========================================================\n"
					"\n"
					"Note that since a *PolylineOnSphere* is immutable it contains no operations or "
					"methods that modify its state (such as adding or removing points). This is similar "
					"to other immutable types in python such as ``str``. So instead of modifying an "
					"existing polyline you will need to create a new :class:`PolylineOnSphere` "
					"instance as the following example demonstrates:\n"
					"::\n"
					"\n"
					"  # Get a list of points from 'polyline'.\n"
					"  points = list(polyline)\n"
					"\n"
					"  # Modify the points list somehow.\n"
					"  points[0] = pygplates.PointOnSphere(...)\n"
					"  points.append(pygplates.PointOnSphere(...))\n"
					"\n"
					"  # 'polyline' now references a new PolylineOnSphere instance.\n"
					"  polyline = pygplates.PolylineOnSphere(points)\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::poly_geometry_on_sphere_create<GPlatesMaths::PolylineOnSphere>,
						bp::default_call_policies(),
						(bp::arg("points"))),
				"__init__(points)\n"
				"  Create a polyline from a sequence of (x,y,z) or (latitude,longitude) points.\n"
				"\n"
				"  :param points: A sequence of (x,y,z) points, or (latitude,longitude) points (in degrees).\n"
				"  :type points: Any sequence of :class:`PointOnSphere` or :class:`UnitVector3D` or "
				":class:`LatLonPoint` or (float,float,float) or (float,float)\n"
				"  :raises: InvalidLatLonError if any *latitude* or *longitude* is invalid\n"
				"  :raises: ViolatedUnitVectorInvariantError if any (x,y,z) is not unit magnitude\n"
				"  :raises: InvalidPointsForPolylineConstructionError if sequence has less than two points\n"
				"\n"
				"  **NOTE** that the sequence must contain at least two points in order to be a valid "
				"polyline, otherwise *InvalidPointsForPolylineConstructionError* will be raised.\n"
				"\n"
				"  During creation, a :class:`GreatCircleArc` is created between each adjacent pair of "
				"points in *points* - see :meth:`get_great_circle_arcs_view`.\n"
				"\n"
				"  It is *not* an error for adjacent points in the sequence to be coincident. In this "
				"case each :class:`GreatCircleArc` between two such adjacent points will have zero length "
				"(:meth:`GreatCircleArc.is_zero_length` will return ``True``) and will have no "
				"rotation axis (:meth:`GreatCircleArc.get_rotation_axis` will raise an error).\n"
				"\n"
				"  The following example shows a few different ways to create a :class:`polyline<PolylineOnSphere>`:\n"
				"  ::\n"
				"\n"
				"    points = []\n"
				"    points.append(pygplates.PointOnSphere(...))\n"
				"    points.append(pygplates.PointOnSphere(...))\n"
				"    points.append(pygplates.PointOnSphere(...))\n"
				"    polyline = pygplates.PolylineOnSphere(points)\n"
				"    \n"
				"    points = []\n"
				"    points.append((lat1,lon1))\n"
				"    points.append((lat2,lon2))\n"
				"    points.append((lat3,lon3))\n"
				"    polyline = pygplates.PolylineOnSphere(points)\n"
				"    \n"
				"    points = []\n"
				"    points.append([x1,y1,z1])\n"
				"    points.append([x2,y2,z2])\n"
				"    points.append([x3,y3,z3])\n"
				"    polyline = pygplates.PolylineOnSphere(points)\n"
				"\n"
				"  If you have latitude/longitude values but they are not a sequence of tuples or "
				"if the latitude/longitude order is swapped then the following examples demonstrate "
				"how you could restructure them:\n"
				"  ::\n"
				"\n"
				"    # Flat lat/lon array.\n"
				"    points = numpy.array([lat1, lon1, lat2, lon2, lat3, lon3])\n"
				"    polyline = pygplates.PolylineOnSphere(zip(points[::2],points[1::2]))\n"
				"    \n"
				"    # Flat lon/lat list (ie, different latitude/longitude order).\n"
				"    points = [lon1, lat1, lon2, lat2, lon3, lat3]\n"
				"    polyline = pygplates.PolylineOnSphere(zip(points[1::2],points[::2]))\n"
				"    \n"
				"    # Separate lat/lon arrays.\n"
				"    lats = numpy.array([lat1, lat2, lat3])\n"
				"    lons = numpy.array([lon1, lon2, lon3])\n"
				"    polyline = pygplates.PolylineOnSphere(zip(lats,lons))\n"
				"    \n"
				"    # Lon/lat list of tuples (ie, different latitude/longitude order).\n"
				"    points = [(lon1, lat1), (lon2, lat2), (lon3, lat3)]\n"
				"    polyline = pygplates.PolylineOnSphere([(lat,lon) for lon, lat in points])\n")
		.def("get_points_view",
				&GPlatesApi::poly_geometry_on_sphere_get_points_view<GPlatesMaths::PolylineOnSphere>,
				"get_points_view() -> PolylineOnSpherePointsView\n"
				"  Returns a *view*, of the :class:`points<PointOnSphere>` of this polyline, that supports iteration "
				"over the points as well as indexing to retrieve individual points or slices of points.\n"
				"\n"
				"  Note that the *view* object returned by this method is not a ``list`` or an *iterator* but, "
				"like dictionary views in Python 3, a ``list`` or *iterator* can be obtained from the *view* "
				"as in ``list(polyline.get_points_view())`` or ``iter(polyline.get_points_view())``.\n"
				"\n"
				"  The returned view provides the following operations for accessing the points:\n"
				"\n"
				"  =========================== ==========================================================\n"
				"  Operation                   Result\n"
				"  =========================== ==========================================================\n"
				"  ``len(view)``               number of points of the polyline\n"
				"  ``for p in view``           iterates over the points *p* of the polyline\n"
				"  ``p in view``               ``True`` if *p* is a point of the polyline\n"
				"  ``p not in view``           ``False`` if *p* is a point of the polyline\n"
				"  ``view[i]``                 the point of the polyline at index *i*\n"
				"  ``view[i:j]``               slice of points of the polyline from *i* to *j*\n"
				"  ``view[i:j:k]``             slice of points of the polyline from *i* to *j* with step *k*\n"
				"  =========================== ==========================================================\n"
				"\n"
				"  The following example demonstrates some uses of the above operations:\n"
				"  ::\n"
				"\n"
				"    polyline = pygplates.PolylineOnSphere(points)\n"
				"    ...\n"
				"    points_view = polyline.get_points_view()\n"
				"    for point in points_view:\n"
				"        print point\n"
				"    first_point = points_view[0]\n"
				"    last_point = points_view[-1]\n"
				"\n"
				"  There are also methods that return the sequence of points as (latitude,longitude) "
				"values and (x,y,z) values contained in lists and numpy arrays "
				"(:meth:`GeometryOnSphere.to_lat_lon_list`, :meth:`GeometryOnSphere.to_lat_lon_array`, "
				":meth:`GeometryOnSphere.to_xyz_list` and :meth:`GeometryOnSphere.to_xyz_array`).\n")
		.def("get_great_circle_arcs_view",
				&GPlatesApi::poly_geometry_on_sphere_get_arcs_view<GPlatesMaths::PolylineOnSphere>,
				"get_great_circle_arcs_view() -> PolylineOnSphereArcsView\n"
				"  Returns a *view*, of the :class:`great circle arcs<GreatCircleArc>` of this polyline, "
				"that supports iteration over the arcs as well as indexing to retrieve individual arcs "
				"or slices of arcs. Between each adjacent pair of :class:`points<PointOnSphere>` there "
				"is an :class:`arc<GreatCircleArc>` such that the number of points exceeds the number of "
				"arcs by one.\n"
				"\n"
				"  Note that the *view* object returned by this method is not a ``list`` or an *iterator* but, "
				"like dictionary views in Python 3, a ``list`` or *iterator* can be obtained from the *view* "
				"as in ``list(polyline.get_great_circle_arcs_view())`` or ``iter(polyline.get_great_circle_arcs_view())``.\n"
				"\n"
				"  The returned view provides the following operations for accessing the great circle arcs:\n"
				"\n"
				"  =========================== ==========================================================\n"
				"  Operation                   Result\n"
				"  =========================== ==========================================================\n"
				"  ``len(view)``               number of arcs of the polyline\n"
				"  ``for a in view``           iterates over the arcs *a* of the polyline\n"
				"  ``a in view``               ``True`` if *a* is an arc of the polyline\n"
				"  ``a not in view``           ``False`` if *a* is an arc of the polyline\n"
				"  ``view[i]``                 the arc of the polyline at index *i*\n"
				"  ``view[i:j]``               slice of arcs of the polyline from *i* to *j*\n"
				"  ``view[i:j:k]``             slice of arcs of the polyline from *i* to *j* with step *k*\n"
				"  =========================== ==========================================================\n"
				"\n"
				"  The following example demonstrates some uses of the above operations:\n"
				"  ::\n"
				"\n"
				"    polyline = pygplates.PolylineOnSphere(points)\n"
				"    ...\n"
				"    arcs_view = polyline.get_great_circle_arcs_view()\n"
				"    for arc in arcs_view:\n"
				"        if not arc.is_zero_length():\n"
				"            print arc.get_rotation_axis()\n"
				"    first_arc = arcs_view[0]\n"
				"    last_arc = arcs_view[-1]\n")
		.def("get_arc_length",
				&GPlatesMaths::PolylineOnSphere::get_arc_length,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_arc_length() -> float\n"
				"  Returns the total arc length of this polyline (in radians).\n"
				"\n"
				"  :rtype: float\n"
				"\n"
				"  This is the sum of the arc lengths of each individual "
				":class:`great circle arc<GreatCircleArc>` of this polyline. "
				"To convert to distance, multiply the result by the Earth radius.\n")
		.def("get_centroid",
				&GPlatesApi::polyline_on_sphere_get_centroid,
				"get_centroid() -> PointOnSphere\n"
				"  Returns the centroid of this polyline.\n"
				"\n"
				"  :rtype: :class:`PointOnSphere`\n"
				"\n"
				"  The centroid is calculated as a weighted average of the mid-points of the "
				":class:`great circle arcs<GreatCircleArc>` of this polyline with weighting "
				"proportional to the individual arc lengths.\n")
		.def("__iter__",
				bp::range(
						&GPlatesMaths::PolylineOnSphere::vertex_begin,
						&GPlatesMaths::PolylineOnSphere::vertex_end))
		.def("__len__", &GPlatesMaths::PolylineOnSphere::number_of_vertices)
		.def("__contains__", &GPlatesApi::polyline_on_sphere_contains_point)
		.def("__getitem__", &GPlatesApi::polyline_on_sphere_get_item)
		// Since we're defining '__eq__' we need to define a compatible '__hash__' or make it unhashable.
		// This is because the default '__hash__'is based on 'id()' which is not compatible and
		// would cause errors when used as key in a dictionary.
		// In python 3 fixes this by automatically making unhashable if define '__eq__' only.
		.setattr("__hash__", bp::object()/*None*/) // make unhashable
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
	;

	// Register to-python conversion for PolylineOnSphere::non_null_ptr_to_const_type.
	GPlatesApi::PythonConverterUtils::register_to_python_const_to_non_const_non_null_intrusive_ptr_conversion<
			GPlatesMaths::PolylineOnSphere>();
	// Register from-python 'non-const' to 'const' conversion.
	boost::python::implicitly_convertible<
			GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PolylineOnSphere>,
			GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::PolylineOnSphere> >();

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Note that we're only dealing with 'const' conversions here.
	// The 'non-const' workaround to get boost-python to compile is handled by 'python_ConstGeometryOnSphere'.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			const GPlatesMaths::PolylineOnSphere,
			const GPlatesMaths::GeometryOnSphere>();
}


namespace GPlatesApi
{
	bool
	polygon_on_sphere_is_point_in_polygon(
			const GPlatesMaths::PolygonOnSphere &polygon_on_sphere,
			// There are from-python converters from LatLonPoint and sequence(latitude,longitude) and
			// sequence(x,y,z) to PointOnSphere so they will also get matched by this...
			const GPlatesMaths::PointOnSphere &point)
	{
		return polygon_on_sphere.is_point_in_polygon(point);
	}

	GPlatesMaths::PointOnSphere
	polygon_on_sphere_get_boundary_centroid(
			const GPlatesMaths::PolygonOnSphere &polygon_on_sphere)
	{
		return GPlatesMaths::PointOnSphere(polygon_on_sphere.get_boundary_centroid());
	}

	GPlatesMaths::PointOnSphere
	polygon_on_sphere_get_interior_centroid(
			const GPlatesMaths::PolygonOnSphere &polygon_on_sphere)
	{
		return GPlatesMaths::PointOnSphere(polygon_on_sphere.get_interior_centroid());
	}

	bool
	polygon_on_sphere_contains_point(
			GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere,
			// There are from-python converters from LatLonPoint and sequence(latitude,longitude) and
			// sequence(x,y,z) to PointOnSphere so they will also get matched by this...
			const GPlatesMaths::PointOnSphere &point_on_sphere)
	{
		PolyGeometryOnSpherePointsView<GPlatesMaths::PolygonOnSphere> points_view(polygon_on_sphere);

		return points_view.contains_point(point_on_sphere);
	}

	//
	// Support for "__get_item__".
	//
	boost::python::object
	polygon_on_sphere_get_item(
			GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere,
			boost::python::object i)
	{
		PolyGeometryOnSpherePointsView<GPlatesMaths::PolygonOnSphere> points_view(polygon_on_sphere);

		return points_view.get_item(i);
	}
}


void
export_polygon_on_sphere()
{
	//
	// A wrapper around view access to the *points* of a PolygonOnSphere.
	//
	// We don't document this wrapper (using docstrings) since it's documented in "PolygonOnSphere".
	bp::class_< GPlatesApi::PolyGeometryOnSpherePointsView<GPlatesMaths::PolygonOnSphere> >(
			"PolygonOnSpherePointsView",
			bp::init<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type>())
		.def("__iter__",
				bp::iterator< const GPlatesApi::PolyGeometryOnSpherePointsView<GPlatesMaths::PolygonOnSphere> >())
		.def("__len__",
				&GPlatesApi::PolyGeometryOnSpherePointsView<GPlatesMaths::PolygonOnSphere>::get_number_of_points)
		.def("__contains__",
				&GPlatesApi::PolyGeometryOnSpherePointsView<GPlatesMaths::PolygonOnSphere>::contains_point)
		.def("__getitem__",
				&GPlatesApi::PolyGeometryOnSpherePointsView<GPlatesMaths::PolygonOnSphere>::get_item)
	;

	//
	// A wrapper around view access to the *great circle arcs* of a PolygonOnSphere.
	//
	// We don't document this wrapper (using docstrings) since it's documented in "PolygonOnSphere".
	bp::class_< GPlatesApi::PolyGeometryOnSphereArcsView<GPlatesMaths::PolygonOnSphere> >(
			"PolygonOnSphereArcsView",
			bp::init<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type>())
		.def("__iter__",
				bp::iterator< const GPlatesApi::PolyGeometryOnSphereArcsView<GPlatesMaths::PolygonOnSphere> >())
		.def("__len__",
				&GPlatesApi::PolyGeometryOnSphereArcsView<GPlatesMaths::PolygonOnSphere>::get_number_of_arcs)
		.def("__contains__",
				&GPlatesApi::PolyGeometryOnSphereArcsView<GPlatesMaths::PolygonOnSphere>::contains_arc)
		.def("__getitem__",
				&GPlatesApi::PolyGeometryOnSphereArcsView<GPlatesMaths::PolygonOnSphere>::get_item)
	;

	//
	// PolygonOnSphere - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::scope polygon_on_sphere_class = bp::class_<
			GPlatesMaths::PolygonOnSphere,
			// NOTE: See note in 'python_ConstGeometryOnSphere' for why this is 'non-const' instead of 'const'...
			GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PolygonOnSphere>,
			bp::bases<GPlatesMaths::GeometryOnSphere>,
			boost::noncopyable>(
					"PolygonOnSphere",
					"Represents a polygon on the surface of the unit length sphere. "
					"Polygons are equality (``==``, ``!=``) comparable - see :class:`PointOnSphere` "
					"for an overview of equality in the presence of limited floating-point precision.\n"
					"\n"
					"A polygon instance provides two types of *views*:\n"
					"\n"
					"* a view of its points - see :meth:`get_points_view`, and\n"
					"* a view of its *great circle arc* subsegments (between adjacent points) - see "
					":meth:`get_great_circle_arcs_view`.\n"
					"\n"
					"In addition a polygon instance is directly iterable over its points:\n"
					"::\n"
					"\n"
					"  polygon = pygplates.PolygonOnSphere(points)\n"
					"  for i, point in enumerate(polygon):\n"
					"      assert(point == polygon[i])\n"
					"\n"
					"...and so the following operations for accessing the points are supported:\n"
					"\n"
					"=========================== ==========================================================\n"
					"Operation                   Result\n"
					"=========================== ==========================================================\n"
					"``len(polygon)``            number of points in *polygon*\n"
					"``for p in polygon``        iterates over the points *p* of *polygon*\n"
					"``p in polygon``            ``True`` if *p* is equal to a point in *polygon*\n"
					"``p not in polygon``        ``False`` if *p* is equal to a point in *polygon*\n"
					"``polygon[i]``              the point of *polygon* at index *i*\n"
					"``polygon[i:j]``            slice of *polygon* from *i* to *j*\n"
					"``polygon[i:j:k]``          slice of *polygon* from *i* to *j* with step *k*\n"
					"=========================== ==========================================================\n"
					"\n"
					"Note that since a *PolygonOnSphere* is immutable it contains no operations or "
					"methods that modify its state (such as adding or removing points). This is similar "
					"to other immutable types in python such as ``str``. So instead of modifying an "
					"existing polygon you will need to create a new :class:`PolygonOnSphere` "
					"instance as the following example demonstrates:\n"
					"::\n"
					"\n"
					"  # Get a list of points from 'polygon'.\n"
					"  points = list(polygon)\n"
					"\n"
					"  # Modify the points list somehow.\n"
					"  points[0] = pygplates.PointOnSphere(...)\n"
					"  points.append(pygplates.PointOnSphere(...))\n"
					"\n"
					"  # 'polygon' now references a new PolygonOnSphere instance.\n"
					"  polygon = pygplates.PolygonOnSphere(points)\n"
					"\n"
					"The following example demonstrates creating a :class:`PolygonOnSphere` from a "
					":class:`PolylineOnSphere`:\n"
					"::\n"
					"\n"
					"  polygon = pygplates.PolygonOnSphere(polyline)\n"
					"\n"
					"...note that the polygon closes the loop between the last and first points "
					"so there's no need to make the first and last points equal.\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::poly_geometry_on_sphere_create<GPlatesMaths::PolygonOnSphere>,
						bp::default_call_policies(),
						(bp::arg("points"))),
				"__init__(points)\n"
				"  Create a polygon from a sequence of (x,y,z) or (latitude,longitude) points.\n"
				"\n"
				"  :param points: A sequence of (x,y,z) points, or (latitude,longitude) points (in degrees).\n"
				"  :type points: Any sequence of :class:`PointOnSphere` or :class:`UnitVector3D` or "
				":class:`LatLonPoint` or (float,float,float) or (float,float)\n"
				"  :raises: InvalidLatLonError if any *latitude* or *longitude* is invalid\n"
				"  :raises: ViolatedUnitVectorInvariantError if any (x,y,z) is not unit magnitude\n"
				"  :raises: InvalidPointsForPolygonConstructionError if sequence has less than three points\n"
				"\n"
				"  **NOTE** that the sequence must contain at least three points in order to be a valid "
				"polygon, otherwise *InvalidPointsForPolygonConstructionError* will be raised.\n"
				"\n"
				"  During creation, a :class:`GreatCircleArc` is created between each adjacent pair of "
				"of points in *points* - see :meth:`get_great_circle_arcs_view`. The last arc is "
				"created between the last and first points to close the loop of the polygon. "
				"For this reason you do *not* need to ensure that the first and last points have the "
				"same position (although it's not an error if this is the case because the final arc "
				"will then just have a zero length).\n"
				"\n"
				"  It is *not* an error for adjacent points in the sequence to be coincident. In this "
				"case each :class:`GreatCircleArc` between two such adjacent points will have zero length "
				"(:meth:`GreatCircleArc.is_zero_length` will return ``True``) and will have no "
				"rotation axis (:meth:`GreatCircleArc.get_rotation_axis` will raise an error).\n"
				"\n"
				"  The following example shows a few different ways to create a :class:`polygon<PolygonOnSphere>`:\n"
				"  ::\n"
				"\n"
				"    points = []\n"
				"    points.append(pygplates.PointOnSphere(...))\n"
				"    points.append(pygplates.PointOnSphere(...))\n"
				"    points.append(pygplates.PointOnSphere(...))\n"
				"    polygon = pygplates.PolygonOnSphere(points)\n"
				"    \n"
				"    points = []\n"
				"    points.append((lat1,lon1))\n"
				"    points.append((lat2,lon2))\n"
				"    points.append((lat3,lon3))\n"
				"    polygon = pygplates.PolygonOnSphere(points)\n"
				"    \n"
				"    points = []\n"
				"    points.append([x1,y1,z1])\n"
				"    points.append([x2,y2,z2])\n"
				"    points.append([x3,y3,z3])\n"
				"    polygon = pygplates.PolygonOnSphere(points)\n"
				"\n"
				"  If you have latitude/longitude values but they are not a sequence of tuples or "
				"if the latitude/longitude order is swapped then the following examples demonstrate "
				"how you could restructure them:\n"
				"  ::\n"
				"\n"
				"    # Flat lat/lon array.\n"
				"    points = numpy.array([lat1, lon1, lat2, lon2, lat3, lon3])\n"
				"    polygon = pygplates.PolygonOnSphere(zip(points[::2],points[1::2]))\n"
				"    \n"
				"    # Flat lon/lat list (ie, different latitude/longitude order).\n"
				"    points = [lon1, lat1, lon2, lat2, lon3, lat3]\n"
				"    polygon = pygplates.PolygonOnSphere(zip(points[1::2],points[::2]))\n"
				"    \n"
				"    # Separate lat/lon arrays.\n"
				"    lats = numpy.array([lat1, lat2, lat3])\n"
				"    lons = numpy.array([lon1, lon2, lon3])\n"
				"    polygon = pygplates.PolygonOnSphere(zip(lats,lons))\n"
				"    \n"
				"    # Lon/lat list of tuples (ie, different latitude/longitude order).\n"
				"    points = [(lon1, lat1), (lon2, lat2), (lon3, lat3)]\n"
				"    polygon = pygplates.PolygonOnSphere([(lat,lon) for lon, lat in points])\n")
		.def("get_points_view",
				&GPlatesApi::poly_geometry_on_sphere_get_points_view<GPlatesMaths::PolygonOnSphere>,
				"get_points_view() -> PolygonOnSpherePointsView\n"
				"  Returns a *view*, of the :class:`points<PointOnSphere>` of this polygon, that supports iteration "
				"over the points as well as indexing to retrieve individual points or slices of points.\n"
				"\n"
				"  Note that the *view* object returned by this method is not a ``list`` or an *iterator* but, "
				"like dictionary views in Python 3, a ``list`` or *iterator* can be obtained from the *view* "
				"as in ``list(polygon.get_points_view())`` or ``iter(polygon.get_points_view())``.\n"
				"\n"
				"  The returned view provides the following operations for accessing the points:\n"
				"\n"
				"  =========================== ==========================================================\n"
				"  Operation                   Result\n"
				"  =========================== ==========================================================\n"
				"  ``len(view)``               number of points of the polygon\n"
				"  ``for p in view``           iterates over the points *p* of the polygon\n"
				"  ``p in view``               ``True`` if *p* is a point of the polygon\n"
				"  ``p not in view``           ``False`` if *p* is a point of the polygon\n"
				"  ``view[i]``                 the point of the polygon at index *i*\n"
				"  ``view[i:j]``               slice of points of the polygon from *i* to *j*\n"
				"  ``view[i:j:k]``             slice of points of the polygon from *i* to *j* with step *k*\n"
				"  =========================== ==========================================================\n"
				"\n"
				"  The following example demonstrates some uses of the above operations:\n"
				"  ::\n"
				"\n"
				"    polygon = pygplates.PolygonOnSphere(points)\n"
				"    ...\n"
				"    points_view = polygon.get_points_view()\n"
				"    for point in points_view:\n"
				"        print point\n"
				"    first_point = points_view[0]\n"
				"    last_point = points_view[-1]\n"
				"\n"
				"  There are also methods that return the sequence of points as (latitude,longitude) "
				"values and (x,y,z) values contained in lists and numpy arrays "
				"(:meth:`GeometryOnSphere.to_lat_lon_list`, :meth:`GeometryOnSphere.to_lat_lon_array`, "
				":meth:`GeometryOnSphere.to_xyz_list` and :meth:`GeometryOnSphere.to_xyz_array`).\n")
		.def("get_great_circle_arcs_view",
				&GPlatesApi::poly_geometry_on_sphere_get_arcs_view<GPlatesMaths::PolygonOnSphere>,
				"get_great_circle_arcs_view() -> PolylineOnSphereArcsView\n"
				"  Returns a *view*, of the :class:`great circle arcs<GreatCircleArc>` of this polygon, "
				"that supports iteration over the arcs as well as indexing to retrieve individual arcs "
				"or slices of arcs. Between each adjacent pair of :class:`points<PointOnSphere>` there "
				"is an :class:`arc<GreatCircleArc>` such that the number of points equals the number of arcs. "
				"Note that the *end* point of the last arc is equal to the *start* point of the first arc.\n"
				"\n"
				"  Note that the *view* object returned by this method is not a ``list`` or an *iterator* but, "
				"like dictionary views in Python 3, a ``list`` or *iterator* can be obtained from the *view* "
				"as in ``list(polygon.get_great_circle_arcs_view())`` or ``iter(polygon.get_great_circle_arcs_view())``.\n"
				"\n"
				"  The returned view provides the following operations for accessing the great circle arcs:\n"
				"\n"
				"  =========================== ==========================================================\n"
				"  Operation                   Result\n"
				"  =========================== ==========================================================\n"
				"  ``len(view)``               number of arcs of the polygon\n"
				"  ``for a in view``           iterates over the arcs *a* of the polygon\n"
				"  ``a in view``               ``True`` if *a* is an arc of the polygon\n"
				"  ``a not in view``           ``False`` if *a* is an arc of the polygon\n"
				"  ``view[i]``                 the arc of the polygon at index *i*\n"
				"  ``view[i:j]``               slice of arcs of the polygon from *i* to *j*\n"
				"  ``view[i:j:k]``             slice of arcs of the polygon from *i* to *j* with step *k*\n"
				"  =========================== ==========================================================\n"
				"\n"
				"  The following example demonstrates some uses of the above operations:\n"
				"  ::\n"
				"\n"
				"    polygon = pygplates.PolygonOnSphere(points)\n"
				"    ...\n"
				"    arcs_view = polygon.get_great_circle_arcs_view()\n"
				"    for arc in arcs_view:\n"
				"        if not arc.is_zero_length():\n"
				"            print arc.get_rotation_axis()\n"
				"    first_arc = arcs_view[0]\n"
				"    last_arc = arcs_view[-1]\n"
				"    assert(last_arc.get_end_point() == first_arc.get_start_point())")
		.def("get_arc_length",
				&GPlatesMaths::PolygonOnSphere::get_arc_length,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_arc_length() -> float\n"
				"  Returns the total arc length of this polygon (in radians).\n"
				"\n"
				"  :rtype: float\n"
				"\n"
				"  This is the sum of the arc lengths of each individual "
				":class:`great circle arc<GreatCircleArc>` of this polygon. "
				"To convert to distance, multiply the result by the Earth radius.\n")
		.def("get_area",
				&GPlatesMaths::PolygonOnSphere::get_area,
				"get_area() -> float\n"
				"  Returns the area of this polygon (on a sphere of unit radius).\n"
				"\n"
				"  :rtype: float\n"
				"\n"
				"  The area is essentially the absolute value of the :meth:`signed area<get_signed_area>`.\n"
				"\n"
				"  To convert to area on the Earth's surface, multiply the result by the Earth radius squared.\n")
		.def("get_signed_area",
				&GPlatesMaths::PolygonOnSphere::get_signed_area,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_signed_area() -> float\n"
				"  Returns the *signed* area of this polygon (on a sphere of unit radius).\n"
				"\n"
				"  :rtype: float\n"
				"\n"
				"  If this polygon is clockwise (when viewed from above the surface of the sphere) "
				"then the returned area will be negative, otherwise it will be positive. "
				"However if you only want to determine the orientation of this polygon then "
				":meth:`get_orientation` is more efficient than comparing the sign of the area.\n"
				"\n"
				"  To convert to *signed* area on the Earth's surface, multiply the result by the "
				"Earth radius squared.\n")
		.def("get_orientation",
				&GPlatesMaths::PolygonOnSphere::get_orientation,
				"get_orientation() -> PolygonOnSphere.Orientation\n"
				"  Returns whether this polygon is clockwise or counter-clockwise.\n"
				"\n"
				"  :rtype: PolygonOnSphere.Orientation\n"
				"\n"
				"  If this polygon is clockwise (when viewed from above the surface of the sphere) "
				"then *PolygonOnSphere.Orientation.clockwise* is returned, otherwise "
				"*PolygonOnSphere.Orientation.counter_clockwise* is returned.\n"
				"\n"
				"  ::\n"
				"\n"
				"    if polygon.get_orientation() == pygplates.PolygonOnSphere.Orientation.clockwise:\n"
				"      print 'Orientation is clockwise'\n"
				"    else:\n"
				"      print 'Orientation is counter-clockwise'\n")
		.def("is_point_in_polygon",
				&GPlatesApi::polygon_on_sphere_is_point_in_polygon,
				"is_point_in_polygon(point) -> bool\n"
				"  Determines whether the specified point lies within the interior of this polygon.\n"
				"\n"
				"  :param point: the point to be tested\n"
				"  :type point: :class:`PointOnSphere` or :class:`LatLonPoint` or (latitude,longitude)"
				", in degrees, or (x,y,z)\n"
				"  :rtype: bool\n")
		.def("get_boundary_centroid",
				&GPlatesApi::polygon_on_sphere_get_boundary_centroid,
				"get_boundary_centroid() -> PointOnSphere\n"
				"  Returns the *boundary* centroid of this polygon.\n"
				"\n"
				"  :rtype: :class:`PointOnSphere`\n"
				"\n"
				"  The *boundary* centroid is calculated as a weighted average of the mid-points of the "
				":class:`great circle arcs<GreatCircleArc>` of this polygon with weighting "
				"proportional to the individual arc lengths.\n"
				"\n"
				"  Note that if you want a centroid closer to the centre-of-mass of the polygon interior "
				"then use :meth:`get_interior_centroid` instead.\n")
		.def("get_interior_centroid",
				&GPlatesApi::polygon_on_sphere_get_interior_centroid,
				"get_interior_centroid() -> PointOnSphere\n"
				"  Returns the *interior* centroid of this polygon.\n"
				"\n"
				"  :rtype: :class:`PointOnSphere`\n"
				"\n"
				"  The *interior* centroid is calculated as a weighted average of the centroids of "
				"spherical triangles formed by the :class:`great circle arcs<GreatCircleArc>` of this polygon "
				"and its (boundary) centroid with weighting proportional to the signed area of each individual "
				"spherical triangle. The three vertices of each spherical triangle consist of the "
				"polygon (boundary) centroid and the two end points of a great circle arc.\n"
				"\n"
				"  This centroid is useful when the centre-of-mass of the polygon interior is desired. "
				"For example, the *interior* centroid of a bottom-heavy, pear-shaped polygon will be "
				"closer to the bottom of the polygon. This centroid is not exactly at the centre-of-mass, "
				"but it will be a lot closer to the real centre-of-mass than :meth:`get_boundary_centroid`.\n")
		.def("__iter__",
				bp::range(
						&GPlatesMaths::PolygonOnSphere::vertex_begin,
						&GPlatesMaths::PolygonOnSphere::vertex_end))
		.def("__len__", &GPlatesMaths::PolygonOnSphere::number_of_vertices)
		.def("__contains__", &GPlatesApi::polygon_on_sphere_contains_point)
		.def("__getitem__", &GPlatesApi::polygon_on_sphere_get_item)
		// Since we're defining '__eq__' we need to define a compatible '__hash__' or make it unhashable.
		// This is because the default '__hash__'is based on 'id()' which is not compatible and
		// would cause errors when used as key in a dictionary.
		// In python 3 fixes this by automatically making unhashable if define '__eq__' only.
		.setattr("__hash__", bp::object()/*None*/) // make unhashable
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
	;

	// An enumeration nested within python class PolygonOnSphere (due to above 'bp::scope').
	bp::enum_<GPlatesMaths::PolygonOrientation::Orientation>("Orientation")
			.value("clockwise", GPlatesMaths::PolygonOrientation::CLOCKWISE)
			.value("counter_clockwise", GPlatesMaths::PolygonOrientation::COUNTERCLOCKWISE);

	// Register to-python conversion for PolygonOnSphere::non_null_ptr_to_const_type.
	GPlatesApi::PythonConverterUtils::register_to_python_const_to_non_const_non_null_intrusive_ptr_conversion<
			GPlatesMaths::PolygonOnSphere>();
	// Register from-python 'non-const' to 'const' conversion.
	boost::python::implicitly_convertible<
			GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PolygonOnSphere>,
			GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::PolygonOnSphere> >();

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Note that we're only dealing with 'const' conversions here.
	// The 'non-const' workaround to get boost-python to compile is handled by 'python_ConstGeometryOnSphere'.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			const GPlatesMaths::PolygonOnSphere,
			const GPlatesMaths::GeometryOnSphere>();
}


void
export_geometries_on_sphere()
{
	export_geometry_on_sphere();

	export_point_on_sphere();
	export_multi_point_on_sphere();
	export_polyline_on_sphere();
	export_polygon_on_sphere();
}

#endif // GPLATES_NO_PYTHON
