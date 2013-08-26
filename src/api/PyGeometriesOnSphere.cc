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

#include "PythonConverterUtils.h"

#include "global/python.h"

#include "maths/GeometryOnSphere.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


void
export_geometry_on_sphere()
{
	/*
	 * Base geometry-on-sphere wrapper class.
	 *
	 * Enables 'isinstance(obj, GeometryOnSphere)' in python - not that it's that useful.
	 *
	 * NOTE: We never return a 'GeometryOnSphere::non_null_ptr_to_const_type' to python because then
	 * python is unable to access the attributes of the derived property value type.
	 */
	bp::class_<GPlatesMaths::GeometryOnSphere, boost::noncopyable>("GeometryOnSphere", bp::no_init)
	;
}


void
export_point_on_sphere()
{
	//
	// PointOnSphere
	//
	bp::class_<GPlatesMaths::PointOnSphere, bp::bases<GPlatesMaths::GeometryOnSphere> >(
			"PointOnSphere", bp::init<GPlatesMaths::UnitVector3D>())
		.def("get_position_vector",
				&GPlatesMaths::PointOnSphere::position_vector,
				bp::return_value_policy<bp::copy_const_reference>())
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
