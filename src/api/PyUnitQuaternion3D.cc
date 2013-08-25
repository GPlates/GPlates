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

#include "maths/UnitQuaternion3D.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	boost::optional<GPlatesMaths::UnitQuaternion3D::RotationParams>
	unit_quaternion_get_rotation_params(
			const GPlatesMaths::UnitQuaternion3D &unit_quaternion)
	{
		if (represents_identity_rotation(unit_quaternion))
		{
			// 'boost::none' causes an indeterminate rotation to end up as Py_None in python.
			return boost::none;
		}

		return unit_quaternion.get_rotation_params(boost::none/*axis_hint*/);
	}
}


void
export_unit_quaternion_3d()
{
	//
	// UnitQuaternion3D
	//
	// Non-member functions.
	bp::def("represents_identity_rotation", &GPlatesMaths::represents_identity_rotation);
	// Class.
	bp::scope unit_quaternion_scope = bp::class_<GPlatesMaths::UnitQuaternion3D>("UnitQuaternion3D", bp::no_init)
		.def("get_w", &GPlatesMaths::UnitQuaternion3D::w, bp::return_value_policy<bp::copy_const_reference>())
		.def("get_x", &GPlatesMaths::UnitQuaternion3D::x, bp::return_value_policy<bp::copy_const_reference>())
		.def("get_y", &GPlatesMaths::UnitQuaternion3D::y, bp::return_value_policy<bp::copy_const_reference>())
		.def("get_z", &GPlatesMaths::UnitQuaternion3D::z, bp::return_value_policy<bp::copy_const_reference>())
		.def("get_rotation_parameters", &GPlatesApi::unit_quaternion_get_rotation_params)
		// Generate '__str__' from 'operator<<'...
		// Note: Seems we need to qualify with 'self_ns::' to avoid MSVC compile error.
		.def(bp::self_ns::str(bp::self))
	;

	// Create nested class RotationParameters inside UnitQuaternion.
	bp::class_<GPlatesMaths::UnitQuaternion3D::RotationParams>("RotationParameters", bp::no_init)
		.def_readonly("axis", &GPlatesMaths::UnitQuaternion3D::RotationParams::axis)
		// RotationParams::angle uses custom-converted type 'GPlatesMaths::Real' which does not
		// work with the 'return_internal_reference' return value policy used in 'def_readonly'...
		// See http://www.boost.org/doc/libs/1_52_0/libs/python/doc/v2/faq.html#topythonconversionfailed
		.add_property("angle",
				bp::make_getter(
						&GPlatesMaths::UnitQuaternion3D::RotationParams::angle,
						bp::return_value_policy<bp::return_by_value>()))
	;

	// Enable boost::optional<UnitQuaternion3D> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::python_optional<GPlatesMaths::UnitQuaternion3D>();

	// Enable boost::optional<UnitQuaternion3D::RotationParams> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::python_optional<GPlatesMaths::UnitQuaternion3D::RotationParams>();
}

#endif // GPLATES_NO_PYTHON
