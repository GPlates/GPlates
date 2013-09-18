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

#include "PythonConverterUtils.h"

#include "global/python.h"

#include "maths/UnitVector3D.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


void
export_unit_vector_3d()
{
	//
	// UnitVector3D - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesMaths::UnitVector3D>(
					"UnitVector3D",
					"Represents a unit length 3D vector. Unit vectors are equality (``==``, ``!=``) comparable.\n"
					"\n"
					// Note that we put the __init__ docstring in the class docstring.
					// See the comment in 'BOOST_PYTHON_MODULE(pygplates)' for an explanation...
					"UnitVector3D(x, y, z)\n"
					"  Construct a *UnitVector3D* instance from a 3D cartesian coordinate consisting of "
					"floating-point coordinates *x*, *y* and *z*.\n"
					"\n"
					"  **NOTE:** The length of 3D vector (x,y,z) must be 1.0, otherwise a *RuntimeError* "
					"is generated.\n"
					"  ::\n"
					"\n"
					"    unit_vector = pygplates.UnitVector3D(x, y, z)\n",
					bp::init<GPlatesMaths::Real, GPlatesMaths::Real, GPlatesMaths::Real>())
		.def("get_x",
				&GPlatesMaths::UnitVector3D::x,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_x() -> float\n"
				"  Returns the *x* coordinate.\n"
				"\n"
				"  :rtype: float\n")
		.def("get_y",
				&GPlatesMaths::UnitVector3D::y,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_y() -> float\n"
				"  Returns the *y* coordinate.\n"
				"\n"
				"  :rtype: float\n")
		.def("get_z",
				&GPlatesMaths::UnitVector3D::z,
				bp::return_value_policy<bp::copy_const_reference>(),
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

	// Enable boost::optional<UnitVector3D> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::python_optional<GPlatesMaths::UnitVector3D>();
}

#endif // GPLATES_NO_PYTHON
