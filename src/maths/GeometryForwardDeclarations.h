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

/*
 * Forward declarations for the GeometryOnSphere derivations.
 *
 * Because these classes inherit from class GPlatesUtils::ReferenceCount (well indirectly)
 * any usage of 'intrusive_ptr' will rely on it. And including 'utils/ReferenceCount.h'
 * won't help because the compiler needs to know it can convert from PointOnSphere,
 * for example, to ReferenceCount and it can't know that from the forward declaration only.
 *
 * So these forward declarations will work fine if no methods are called on
 * the 'intrusive_ptr'. For example, copy constructor, destructor, constructor.
 * Typically this will mean your methods or functions cannot be defined in the header file.
 */
namespace GPlatesMaths
{
	class MultiPointOnSphere;
	class PointOnSphere;
	class PolylineOnSphere;
	class PolygonOnSphere;
}

#endif  // GPLATES_MATHS_GEOMETRYFORWARDDECLARATIONS_H
