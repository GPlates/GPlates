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

#include "global/CompilerWarnings.h"
#include "global/python.h"
// This is not included by <boost/python.hpp>.
// Also we must include this after <boost/python.hpp> which means after "global/python.h".
#include <boost/python/slice.hpp>
#include <boost/python/stl_iterator.hpp>

#include "maths/GeometryOnSphere.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"

#include "utils/non_null_intrusive_ptr.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	/**
	 * Enables *const* @a GeometryOnSphere derived types to be passed to and from python.
	 *
	 * @a GeometryOnSphere and its derived geometry types are considered immutable and hence a lot
	 * of general functions return *const* geometry objects. This is fine except that boost-python
	 * currently does not compile when wrapping *const* objects
	 * (eg, 'GeometryOnSphere::non_null_ptr_to_const_type') - see:
	 *   https://svn.boost.org/trac/boost/ticket/857
	 *   https://mail.python.org/pipermail/cplusplus-sig/2006-November/011354.html
	 *
	 * ...so the current solution is to wrap *non-const* geometry objects (to keep boost-python happy)
	 * but provide a python to/from converter that converts:
	 *   to-python: GeometryOnSphereType::non_null_ptr_to_const_type -> GeometryOnSphereType::non_null_ptr_type
	 *   from-python: GeometryOnSphereType::non_null_ptr_type -> GeometryOnSphereType::non_null_ptr_to_const_type
	 * ...which enables us to keep using 'non_null_ptr_to_const_type' everywhere (ie, we don't have
	 * to change our C++ source code) but have it converted to/from 'non_null_ptr_type' when
	 * interacting with python. And since our python-wrapped geometries are 'non_null_ptr_type' then
	 * boost-python takes care of the rest of the conversion/wrapping.
	 * Normally this is a little dangerous since it involves casting away const, but the
	 * geometry types are immutable anyway and hence cannot be modified.
	 *
	 * @a PointOnSphere is a bit of an exception to the immutable rule since it can be allocated on
	 * the stack or heap (unlike the other geometry types) and it has an assignment operator.
	 * However we treat it the same as the other geometry types.
	 *
	 * For more information on boost python to/from conversions, see:
	 *   http://misspent.wordpress.com/2009/09/27/how-to-write-boost-python-converters/
	 */
	template <class GeometryOnSphereType>
	struct python_ConstGeometryOnSphere :
			private boost::noncopyable
	{
		explicit
		python_ConstGeometryOnSphere()
		{
			namespace bp = boost::python;

			// To python conversion.
			bp::to_python_converter<
					GPlatesUtils::non_null_intrusive_ptr<const GeometryOnSphereType>,
					Conversion>();

			// From python conversion.
			bp::converter::registry::push_back(
					&convertible,
					&construct,
					bp::type_id< GPlatesUtils::non_null_intrusive_ptr<const GeometryOnSphereType> >());
		}

		struct Conversion
		{
			static
			PyObject *
			convert(
					const GPlatesUtils::non_null_intrusive_ptr<const GeometryOnSphereType> &geometry_on_sphere)
			{
				namespace bp = boost::python;

				// 'GPlatesUtils::non_null_intrusive_ptr<GeometryOnSphereType>' is the HeldType of the
				// 'bp::class_' wrapper of the GeometryOnSphere type so it will be used to complete
				// the conversion to a python-wrapped object. See note above about casting away const.
				return bp::incref(
						bp::object(
								GPlatesUtils::const_pointer_cast<GeometryOnSphereType>(
										geometry_on_sphere)).ptr());
			};
		};

		static
		void *
		convertible(
				PyObject *obj)
		{
			namespace bp = boost::python;

			// 'GPlatesUtils::non_null_intrusive_ptr<GeometryOnSphereType>' is the HeldType of the
			// 'bp::class_' wrapper of the GeometryOnSphere type so we know we can extract that
			// from python and we know we can take care of the rest of the conversion (to 'const').
			return bp::extract<
					GPlatesUtils::non_null_intrusive_ptr<GeometryOnSphereType> >(obj)
							.check() ? obj : NULL;
		}

		static
		void
		construct(
				PyObject *obj,
				boost::python::converter::rvalue_from_python_stage1_data *data)
		{
			namespace bp = boost::python;

			void *const storage = reinterpret_cast<
					bp::converter::rvalue_from_python_storage<GPlatesMaths::Real> *>(
							data)->storage.bytes;

			// 'GPlatesUtils::non_null_intrusive_ptr<GeometryOnSphereType>' is the HeldType of the
			// 'bp::class_' wrapper of the GeometryOnSphere type so first extract that from python.
			GPlatesUtils::non_null_intrusive_ptr<GeometryOnSphereType> geometry_on_sphere_ptr =
					bp::extract< GPlatesUtils::non_null_intrusive_ptr<GeometryOnSphereType> >(obj);

			// Take care of the rest of the conversion (to 'const').
			new (storage) GPlatesUtils::non_null_intrusive_ptr<const GeometryOnSphereType>(
					geometry_on_sphere_ptr);

			data->convertible = storage;
		}
	};
}


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
	 * to a derived geometry type (eg, 'GeometryOnSphere::non_null_ptr_to_const_type' -> 'PointOnSphere &')
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

	// Register to/from conversion for GeometryOnSphere::non_null_ptr_to_const_type.
	GPlatesApi::python_ConstGeometryOnSphere<GPlatesMaths::GeometryOnSphere>();

	// Enable boost::optional<GeometryOnSphere::non_null_ptr_to_const_type> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::python_optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>();
}


namespace GPlatesApi
{
	// Convenience constructor to create from (x,y,z) without having to first create a UnitVector3D.
	//
	// We can't use bp::make_constructor with a function that returns a non-pointer, so instead we
	// return 'non_null_intrusive_ptr'.
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
					"Represents a point on the surface of the unit length sphere.\n"
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
					bp::init<GPlatesMaths::UnitVector3D>(
							(bp::arg("unit_vector")),
							"__init__(unit_vector)\n"
							"  Create a *PointOnSphere* instance from a 3D unit vector.\n"
							"\n"
							"  :param unit_vector: the 3D unit vector\n"
							"  :type unit_vector: :class:`UnitVector3D`\n"
							"\n"
							"  ::\n"
							"\n"
							"    point = pygplates.PointOnSphere(unit_vector)\n"))
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
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
		// Generate '__str__' from 'operator<<'...
		// Note: Seems we need to qualify with 'self_ns::' to avoid MSVC compile error.
		.def(bp::self_ns::str(bp::self))
	;

	// Register to/from conversion for PointOnSphere::non_null_ptr_to_const_type.
	GPlatesApi::python_ConstGeometryOnSphere<GPlatesMaths::PointOnSphere>();

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Note that we're only dealing with 'const' conversions here.
	// The 'non-const' workaround to get boost-python to compile is handled by 'python_ConstGeometryOnSphere'.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			const GPlatesMaths::PointOnSphere,
			const GPlatesMaths::GeometryOnSphere>();
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
					"``len(mp)``                 length of *mp*\n"
					"``for p in mp``             iterates over the points *p* of multi-point *mp*\n"
					"``p in mp``                 ``True`` if *p* is equal to a point in *mp*\n"
					"``p not in mp``             ``False`` if *p* is equal to a point in *mp*\n"
					"``mp[i]``                   the point of *mp* at index *i*\n"
					"``mp[i:j]``                 slice of *mp* from *i* to *j*\n"
					"``mp[i:j:k]``               slice of *mp* from *i* to *j* with step *k*\n"
					"=========================== ==========================================================\n"
					"\n"
					"Note that since a *MultiPointOnSphere* is immutable it contains no operations or "
					"methods that modify its state (such as adding or removing points).\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::multi_point_on_sphere_create,
						bp::default_call_policies(),
						(bp::arg("points"))),
				"__init__(points)\n"
				"  Create a multi-point from a sequence of :class:`point<PointOnSphere>` instances.\n"
				"\n"
				"  :param points: A sequence of :class:`PointOnSphere` elements.\n"
				"  :type points: Any sequence such as a ``list`` or a ``tuple``\n"
				"  :raises: InsufficientPointsForMultiPointConstructionError if point sequence is empty\n"
				"\n"
				"  **NOTE** that the sequence must contain at least one point, otherwise "
				"*InsufficientPointsForMultiPointConstructionError* will be raised.\n"
				"  ::\n"
				"\n"
				"    multi_point = pygplates.MultiPointOnSphere(points)\n")
		.def("__iter__", bp::iterator<const GPlatesMaths::MultiPointOnSphere>())
		.def("__len__", &GPlatesMaths::MultiPointOnSphere::number_of_points)
		.def("__contains__", &GPlatesApi::multi_point_on_sphere_contains_point)
		.def("__getitem__", &GPlatesApi::multi_point_on_sphere_get_item)
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
	;

	// Register to/from conversion for MultiPointOnSphere::non_null_ptr_to_const_type.
	GPlatesApi::python_ConstGeometryOnSphere<GPlatesMaths::MultiPointOnSphere>();

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
			bp::init<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>())
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
			bp::init<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>())
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
					"Note that since a *PolylineOnSphere* is immutable it contains no operations or "
					"methods that modify its state (such as adding or removing points). This is similar "
					"to other immutable types in python such as ``str``. So instead of modifying an "
					"existing polyline you will need to create a new :class:`PolylineOnSphere` "
					"instance as the following example demonstrates:\n"
					"::\n"
					"\n"
					"  # Get a list of points from 'polyline'.\n"
					"  points = list(polyline.get_points_view())\n"
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
				"  Create a polyline from a sequence of :class:`point<PointOnSphere>` instances.\n"
				"\n"
				"  :param points: A sequence of :class:`PointOnSphere` elements.\n"
				"  :type points: Any sequence such as a ``list`` or a ``tuple``\n"
				"  :raises: InvalidPointsForPolylineConstructionError if sequence has less than two points"
				"\n"
				"  **NOTE** that the sequence must contain at least two points in order to be a valid "
				"polyline, otherwise *InvalidPointsForPolylineConstructionError* will be raised.\n"
				"\n"
				"  During creation, a :class:`GreatCircleArc` is created between each adjacent pair "
				"points in *points* - see :meth:`get_great_circle_arcs_view`.\n"
				"\n"
				"  It is *not* an error for adjacent points in the sequence to be coincident. In this "
				"case each :class:`GreatCircleArc` between two such adjacent points will have zero length "
				"(:meth:`GreatCircleArc.is_zero_length` will return ``True``) and will have no "
				"rotation axis (:meth:`GreatCircleArc.get_rotation_axis` will raise an error).\n"
				"  ::\n"
				"\n"
				"    polyline = pygplates.PolylineOnSphere(points)\n")
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
				"  ``len(view)``               number of points on the polyline\n"
				"  ``for p in view``           iterates over the points *p* on the polyline\n"
				"  ``p in view``               ``True`` if *p* is a point on the polyline\n"
				"  ``p not in view``           ``False`` if *p* is a point on the polyline\n"
				"  ``view[i]``                 the point on the polyline at index *i*\n"
				"  ``view[i:j]``               slice of points on the polyline from *i* to *j*\n"
				"  ``view[i:j:k]``             slice of points on the polyline from *i* to *j* with step *k*\n"
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
				"    last_point = points_view[-1]\n")
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
				"  ``len(view)``               number of arcs on the polyline\n"
				"  ``for a in view``           iterates over the arcs *a* on the polyline\n"
				"  ``a in view``               ``True`` if *a* is an arc on the polyline\n"
				"  ``a not in view``           ``False`` if *a* is an arc on the polyline\n"
				"  ``view[i]``                 the arc on the polyline at index *i*\n"
				"  ``view[i:j]``               slice of arcs on the polyline from *i* to *j*\n"
				"  ``view[i:j:k]``             slice of arcs on the polyline from *i* to *j* with step *k*\n"
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
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
	;

	// Register to/from conversion for PolylineOnSphere::non_null_ptr_to_const_type.
	GPlatesApi::python_ConstGeometryOnSphere<GPlatesMaths::PolylineOnSphere>();

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Note that we're only dealing with 'const' conversions here.
	// The 'non-const' workaround to get boost-python to compile is handled by 'python_ConstGeometryOnSphere'.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			const GPlatesMaths::PolylineOnSphere,
			const GPlatesMaths::GeometryOnSphere>();
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
	bp::class_<
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
					"Note that since a *PolygonOnSphere* is immutable it contains no operations or "
					"methods that modify its state (such as adding or removing points). This is similar "
					"to other immutable types in python such as ``str``. So instead of modifying an "
					"existing polygon you will need to create a new :class:`PolygonOnSphere` "
					"instance as the following example demonstrates:\n"
					"::\n"
					"\n"
					"  # Get a list of points from 'polygon'.\n"
					"  points = list(polygon.get_points_view())\n"
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
					"  polygon = pygplates.PolygonOnSphere(polyline.get_points_view())\n"
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
				"  Create a polygon from a sequence of :class:`point<PointOnSphere>` instances.\n"
				"\n"
				"  :param points: A sequence of :class:`PointOnSphere` elements.\n"
				"  :type points: Any sequence such as a ``list`` or a ``tuple``\n"
				"  :raises: InvalidPointsForPolygonConstructionError if sequence has less than three points\n"
				"\n"
				"  **NOTE** that the sequence must contain at least three points in order to be a valid "
				"polygon, otherwise *InvalidPointsForPolygonConstructionError* will be raised.\n"
				"\n"
				"  During creation, a :class:`GreatCircleArc` is created between each adjacent pair "
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
				"  ::\n"
				"\n"
				"    polygon = pygplates.PolygonOnSphere(points)\n")
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
				"  ``len(view)``               number of points on the polygon\n"
				"  ``for p in view``           iterates over the points *p* on the polygon\n"
				"  ``p in view``               ``True`` if *p* is a point on the polygon\n"
				"  ``p not in view``           ``False`` if *p* is a point on the polygon\n"
				"  ``view[i]``                 the point on the polygon at index *i*\n"
				"  ``view[i:j]``               slice of points on the polygon from *i* to *j*\n"
				"  ``view[i:j:k]``             slice of points on the polygon from *i* to *j* with step *k*\n"
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
				"    last_point = points_view[-1]\n")
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
				"  ``len(view)``               number of arcs on the polygon\n"
				"  ``for a in view``           iterates over the arcs *a* on the polygon\n"
				"  ``a in view``               ``True`` if *a* is an arc on the polygon\n"
				"  ``a not in view``           ``False`` if *a* is an arc on the polygon\n"
				"  ``view[i]``                 the arc on the polygon at index *i*\n"
				"  ``view[i:j]``               slice of arcs on the polygon from *i* to *j*\n"
				"  ``view[i:j:k]``             slice of arcs on the polygon from *i* to *j* with step *k*\n"
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
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
	;

	// Register to/from conversion for PolygonOnSphere::non_null_ptr_to_const_type.
	GPlatesApi::python_ConstGeometryOnSphere<GPlatesMaths::PolygonOnSphere>();

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
