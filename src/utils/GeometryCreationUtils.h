/* $Id$ */

/**
 * \file 
 * A collection of functions to aid in the creation of GeometryOnSphere
 * derivations from user input.
 *
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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
 
#ifndef GPLATES_UTILS_GEOMETRYCREATIONUTILS_H
#define GPLATES_UTILS_GEOMETRYCREATIONUTILS_H

#include <boost/optional.hpp>

#include "maths/GeometryOnSphere.h"
#include "maths/GeometryForwardDeclarations.h"


namespace GPlatesUtils
{
	/**
	 * Enumerates all possible return values from GeometryOnSphere construction-parameter
	 * validation functions. This takes advantage of the fact that some invalid states
	 * (e.g. insufficient points) are common to several different GeometryOnSphere derivations.
	 * 
	 * The downside is that each create_*_on_sphere() function needs to map the type-specific
	 * validity states to this enumeration - but it should be checking the return value anyway.
	 */
	namespace GeometryConstruction
	{
		enum GeometryConstructionValidity
		{
			VALID,
			INVALID_INSUFFICIENT_POINTS,
			INVALID_ANTIPODAL_SEGMENT_ENDPOINTS
		};
	}


	/**
	 * Creates a single PointOnSphere (assuming >= 1 points are provided).
	 * If you supply more than one point, the others will get ignored.
	 *
	 * @a validity is a return-parameter. It will be set to
	 * GeometryConstruction::VALID if everything went ok. In the event
	 * of construction problems occuring, it will indicate why construction
	 * failed.
	 *
	 * Note we are returning a possibly-none boost::optional of
	 * GeometryOnSphere::non_null_ptr_to_const_type.
	 */
	boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
	create_point_on_sphere(
			const std::vector<GPlatesMaths::PointOnSphere> &points,
			GeometryConstruction::GeometryConstructionValidity &validity);


	/**
	 * Creates a single PolylineOnSphere (assuming >= 2 distinct points are provided).
	 *
	 * @a validity is a return-parameter. It will be set to
	 * GeometryConstruction::VALID if everything went ok. In the event
	 * of construction problems occuring, it will indicate why construction
	 * failed.
	 *
	 * Note we are returning a possibly-none boost::optional of
	 * GeometryOnSphere::non_null_ptr_to_const_type.
	 */
	boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
	create_polyline_on_sphere(
			const std::vector<GPlatesMaths::PointOnSphere> &points,
			GeometryConstruction::GeometryConstructionValidity &validity);


	/**
	 * Creates a single PolygonOnSphere (assuming >= 3 distinct points are provided).
	 *
	 * @a validity is a return-parameter. It will be set to
	 * GeometryConstruction::VALID if everything went ok. In the event
	 * of construction problems occuring, it will indicate why construction
	 * failed.
	 *
	 * Note we are returning a possibly-none boost::optional of
	 * GeometryOnSphere::non_null_ptr_to_const_type.
	 */
	boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
	create_polygon_on_sphere(
			const std::vector<GPlatesMaths::PointOnSphere> &points,
			GeometryConstruction::GeometryConstructionValidity &validity);


	/**
	 * Creates a single MultiPointOnSphere (assuming >= 1 point is provided).
	 *
	 * @a validity is a return-parameter. It will be set to
	 * GeometryConstruction::VALID if everything went ok. In the event
	 * of construction problems occuring, it will indicate why construction
	 * failed.
	 *
	 * Note we are returning a possibly-none boost::optional of
	 * GeometryOnSphere::non_null_ptr_to_const_type.
	 */
	boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
	create_multipoint_on_sphere(
			const std::vector<GPlatesMaths::PointOnSphere> &points,
			GeometryConstruction::GeometryConstructionValidity &validity);


}


#endif // GPLATES_UTILS_GEOMETRYCREATIONUTILS_H
