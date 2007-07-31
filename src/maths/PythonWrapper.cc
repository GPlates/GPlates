/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006 The University of Sydney, Australia
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

#include <boost/python.hpp>
#include "PointOnSphere.h"
#include "PolylineOnSphere.h"
#include "Real.h"
#include "UnitVector3D.h"
#include "Vector3D.h"

using namespace boost::python;

BOOST_PYTHON_MODULE(_maths)
{
	GPlatesMaths::export_PointOnSphere();
	GPlatesMaths::export_PolylineOnSphere();
	GPlatesMaths::export_Real();
	GPlatesMaths::export_UnitVector3D();
	GPlatesMaths::export_Vector3D();
}

