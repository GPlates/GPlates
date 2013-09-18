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

#include "global/CompilerWarnings.h"
#include "global/python.h"

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
// For PyFloat_Check below.
DISABLE_GCC_WARNING("-Wold-style-cast")

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

ENABLE_GCC_WARNING("-Wold-style-cast")
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
					"* :class:`PointOnsphere`\n"
					"* :class:`MultiPointOnSphere`\n"
					"* :class:`PolylineOnsphere`\n"
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
	//   pointer_holder.hpp:145:66: error: invalid conversion from ‘const void*’ to ‘void*’
	// ...if we return 'GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type' and rely on
	// 'python_ConstGeometryOnSphere' to convert for us - despite the fact that this conversion works
	// successfully for python bindings in other source files.
	// So we avoid it by using returning a pointer to 'non-const'.
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
	// Note that PointOnSphere (like all GeometryOnSphere types) is immutable so we can store it
	// in the python wrapper by-value instead, of by non_null_instrusive_ptr like the other
	// GeometryOnSphere types (which are more heavyweight), and not have to worry about modifying
	// the wrong instance (because cannot modify).
	//
	bp::class_<
			GPlatesMaths::PointOnSphere,
			// NOTE: See note in 'python_ConstGeometryOnSphere' for why this is 'non-const' instead of 'const'...
			GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PointOnSphere>,
			bp::bases<GPlatesMaths::GeometryOnSphere> >(
					"PointOnSphere",
					"Represents a point on the surface of a unit length sphere. "
					"Points are equality (``==``, ``!=``) comparable.\n"
					"\n"
					// Note that we put the __init__ docstring in the class docstring.
					// See the comment in 'BOOST_PYTHON_MODULE(pygplates)' for an explanation...
					"PointOnSphere(x, y, z)\n"
					"  Construct a *PointOnSphere* instance from a 3D cartesian coordinate consisting of "
					"floating-point coordinates *x*, *y* and *z*.\n"
					"\n"
					"  **NOTE:** The length of 3D vector (x,y,z) must be 1.0, otherwise a *RuntimeError* "
					"is generated. In other words the point must lie on the *unit* sphere.\n"
					"  ::\n"
					"\n"
					"    point = pygplates.PointOnSphere(x, y, z)\n"
					"\n"
					"PointOnSphere(unit_vector)\n"
					"  Construct a *PointOnSphere* instance from an instance of :class:`UnitVector3D`.\n"
					"  ::\n"
					"\n"
					"    point = pygplates.PointOnSphere(unit_vector)\n",
					bp::init<GPlatesMaths::UnitVector3D>())
		.def("__init__",
				bp::make_constructor(&GPlatesApi::point_on_sphere_create_xyz))
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


void
export_geometries_on_sphere()
{
	export_geometry_on_sphere();

	export_point_on_sphere();
}

#endif // GPLATES_NO_PYTHON
