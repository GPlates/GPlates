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

#include "PyGpmlPlateId.h"

#if !defined(GPLATES_NO_PYTHON)

using namespace boost::python;

void
export_gpml_plate_id()
{
	class_<GPlatesApi::GpmlPlateId, bases<GPlatesApi::PropertyValue> >("GpmlPlateId", no_init)
		.def("create", &GPlatesApi::GpmlPlateId::create)
		.staticmethod("create")
		.def("get_value", &GPlatesApi::GpmlPlateId::get_value)
		.def("set_value", &GPlatesApi::GpmlPlateId::set_value);
}

#endif // GPLATES_NO_PYTHON
