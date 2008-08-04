/* $Id$ */

/**
 * \file 
 * This file contains forward declarations for geometries such as PointOnSphere.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007 The University of Sydney, Australia
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

#ifndef GPLATES_MATHS_GEOMETRYFORWARDDECLARATIONS_H
#define GPLATES_MATHS_GEOMETRYFORWARDDECLARATIONS_H

#include "utils/non_null_intrusive_ptr.h"


namespace GPlatesMaths
{
	class MultiPointOnSphere;
	class PointOnSphere;
	class PolylineOnSphere;
	class PolygonOnSphere;

	void
	intrusive_ptr_add_ref(
			const MultiPointOnSphere *p);

	void
	intrusive_ptr_release(
			const MultiPointOnSphere *p);

	void
	intrusive_ptr_add_ref(
			const PointOnSphere *p);

	void
	intrusive_ptr_release(
			const PointOnSphere *p);

	void
	intrusive_ptr_add_ref(
			const PolylineOnSphere *p);

	void
	intrusive_ptr_release(
			const PolylineOnSphere *p);

	void
	intrusive_ptr_add_ref(
			const PolygonOnSphere *p);

	void
	intrusive_ptr_release(
			const PolygonOnSphere *p);

}

#endif  // GPLATES_MATHS_GEOMETRYFORWARDDECLARATIONS_H
