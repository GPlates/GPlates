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
#include <boost/python/slice.hpp>
#include <boost/python/stl_iterator.hpp>

#include "maths/CartesianConvMatrix3D.h"
#include "maths/Vector3D.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	bp::object
	local_cartesian_eq(
			const GPlatesMaths::CartesianConvMatrix3D &local_cartesian,
			bp::object other)
	{
		bp::extract<const GPlatesMaths::CartesianConvMatrix3D &> extract_other_lc_instance(other);
		// Prevent equality comparisons between CartesianConvMatrix3D.
		if (extract_other_lc_instance.check())
		{
			PyErr_SetString(PyExc_TypeError,
					"Cannot equality compare (==, !=) LocalCartesian");
			bp::throw_error_already_set();
		}

		// Return NotImplemented so python can continue looking for a match
		// (eg, in case 'other' is a class that implements relational operators with CartesianConvMatrix3D).
		//
		// NOTE: This will most likely fall back to python's default handling which uses 'id()'
		// and hence will compare based on *python* object address rather than *C++* object address.
		return bp::object(bp::handle<>(bp::borrowed(Py_NotImplemented)));
	}

	bp::object
	local_cartesian_ne(
			const GPlatesMaths::CartesianConvMatrix3D &local_cartesian,
			bp::object other)
	{
		bp::object ne_result = local_cartesian_eq(local_cartesian, other);
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
					"LocalCartesian is *not* equality (``==``, ``!=``) comparable (will raise ``TypeError`` "
					"when compared) and is not hashable (cannot be used as a key in a ``dict``).\n",
					bp::init<
						// There are from-python converters from LatLonPoint and sequence(latitude,longitude) and
						// sequence(x,y,z) to PointOnSphere so they will also get matched by this...
						const GPlatesMaths::PointOnSphere &>(
							(bp::arg("point")),
							"__init__(point)\n"
							"  Create a local cartesian coordinate system at a point on the sphere.\n"
							"\n"
							"  :param point: the origin of the local coordinate system\n"
							"  :type point: :class:`PointOnSphere` or :class:`LatLonPoint` or tuple "
							"(latitude,longitude), in degrees, or tuple (x,y,z)\n"
							"\n"
							"  ::\n"
							"\n"
							"    local_cartesian = pygplates.LocalCartesian(point)\n"))
		.def("get_north",
				&GPlatesMaths::CartesianConvMatrix3D::north,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_north() -> Vector3D\n"
				"  Returns the North coordinate axis.\n"
				"\n"
				"  :rtype: :class:`Vector3D`\n"
				"\n"
				"  The North axis is the tangential vector (to the unit globe) that is most "
				"Northward pointing. It has unit magnitude.\n")
		.def("get_east",
				&GPlatesMaths::CartesianConvMatrix3D::east,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_east() -> Vector3D\n"
				"  Returns the East coordinate axis.\n"
				"\n"
				"  :rtype: :class:`Vector3D`\n"
				"\n"
				"  The East axis is the tangential vector (to the unit globe) that is most "
				"Eastward pointing. It has unit magnitude.\n")
		.def("get_down",
				&GPlatesMaths::CartesianConvMatrix3D::down,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_down() -> Vector3D\n"
				"  Returns the Down coordinate axis.\n"
				"\n"
				"  :rtype: :class:`Vector3D`\n"
				"\n"
				"  The Down axis points at the centre of the globe. It has unit magnitude.\n")
		// We prevent equality comparisons since we don't have C++ equality comparison and users
		// might expect a LocalCartesian equality operator to compare by value instead of object identity.
		// Also make unhashable since user will also expect hashing to be based on object value (not identity).
		.def(GPlatesApi::NoHashDefVisitor(false, true))
		.def("__eq__", &GPlatesApi::local_cartesian_eq)
		.def("__ne__", &GPlatesApi::local_cartesian_ne)
	;

	// Enable boost::optional<CartesianConvMatrix3D> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesMaths::CartesianConvMatrix3D>();
}

#endif // GPLATES_NO_PYTHON
