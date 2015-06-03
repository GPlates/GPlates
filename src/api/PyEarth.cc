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

#include "PythonHashDefVisitor.h"

#include "global/python.h"

#include "utils/Earth.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


void
export_earth()
{
	//
	// Earth - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<GPlatesUtils::Earth>(
			"Earth",
			"Various Earth-related parameters (such as radius).\n"
			"\n"
			"The following *radius* parameters are available as class attributes:\n"
			"\n"
			"* ``pygplates.Earth.equatorial_radius_in_kms``: radius at equator (6378.137 kms)\n"
			"* ``pygplates.Earth.polar_radius_in_kms``: radius at the poles (6356.7523142 kms)\n"
			"* ``pygplates.Earth.mean_radius_in_kms``: mean radius (6371.009 kms)\n"
			"\n"
			"For example, to access the mean radius:\n"
			"::\n"
			"\n"
			"  earth_mean_radius_in_kms = pygplates.Earth.mean_radius_in_kms\n"
			"\n"
			".. note:: The *radius* parameters are based on the WGS-84 coordinate system.\n",
			bp::init<>("__init__()\n")) // Sphinx autosummary complains if signature not present in docstring.

		// Radius parameters...
		.def_readonly("equatorial_radius_in_kms", GPlatesUtils::Earth::EQUATORIAL_RADIUS_KMS)
		.def_readonly("polar_radius_in_kms", GPlatesUtils::Earth::POLAR_RADIUS_KMS)
		.def_readonly("mean_radius_in_kms", GPlatesUtils::Earth::MEAN_RADIUS_KMS)

		// Make hash and comparisons based on C++ object identity (not python object identity)...
		// We don't really need this since all the data is class data (not instance data).
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	;
}

#endif // GPLATES_NO_PYTHON
