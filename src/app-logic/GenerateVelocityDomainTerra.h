/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
#ifndef GENERATE_VELOCITY_DOMAIN_CITCOMS_H
#define GENERATE_VELOCITY_DOMAIN_CITCOMS_H

#include <vector>

#include "maths/PointOnSphere.h"
#include "maths/MultiPointOnSphere.h"

namespace GPlatesAppLogic
{
	namespace GenerateVelocityDomainCitcoms
	{
		/**
		 *      Given the resolution, return the mesh diamond geometries
		 *      There are 12 MultiPoint Geometries in this case.
		 */
		void
		generate_mesh_geometries(
				int node_x, 
				std::vector<GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type> &geometries);
		
		/**
		 *      Given the resolution and index number,
		 *      return the mesh diamond geometry
		 */
		GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type
		generate_mesh_geometry(
				int node_x,
				unsigned index);
	}
}
#endif

