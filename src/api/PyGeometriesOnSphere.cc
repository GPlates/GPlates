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

#include <boost/shared_ptr.hpp>

#include "PythonConverterUtils.h"

#include "global/python.h"

#include "maths/GeometryOnSphere.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	// Return cloned geometry to python as its derived geometry type.
	bp::object/*derived geometry-on-sphere non_null_intrusive_ptr*/
	geometry_on_sphere_clone(
			GPlatesMaths::GeometryOnSphere &geometry_on_sphere)
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
	 * NOTE: We never return a 'GeometryOnSphere::non_null_ptr_to_const_type' to python because then
	 * python is unable to access the attributes of the derived property value type.
	 */
	bp::class_<
			GPlatesMaths::GeometryOnSphere,
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
}


namespace GPlatesApi
{
	// Convenience constructor to create from (x,y,z) without having to first create a UnitVector3D.
	//
	// We can't use bp::make_constructor with a function that returns a non-pointer, so instead we
	// use the magical boost::shared_ptr which works even though we didn't wrap PointOnSphere
	// using a boost::shared_ptr. It's slightly more expensive with the heap allocation though.
	boost::shared_ptr<GPlatesMaths::PointOnSphere>
	point_on_sphere_create_xyz(
			const GPlatesMaths::real_t &x,
			const GPlatesMaths::real_t &y,
			const GPlatesMaths::real_t &z)
	{
		return boost::shared_ptr<GPlatesMaths::PointOnSphere>(
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

	// Enable boost::optional<PointOnSphere> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::python_optional<GPlatesMaths::PointOnSphere>();
}


void
export_geometries_on_sphere()
{
	export_geometry_on_sphere();

	export_point_on_sphere();
}

#endif // GPLATES_NO_PYTHON
