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

#include <QDebug>
#include <boost/none.hpp>
#include "GeometryCreationUtils.h"

#include "maths/GeometryOnSphere.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"


namespace
{
	/**
	 * Simple typedef for convenient calling of some of MultiPointOnSphere's members.
	 */
	typedef GPlatesMaths::MultiPointOnSphere multi_point_type;

	/**
	 * Simple typedef for convenient calling of some of PolygonOnSphere's members.
	 */
	typedef GPlatesMaths::PolygonOnSphere polygon_type;

	/**
	 * Simple typedef for convenient calling of some of PolylineOnSphere's members.
	 */
	typedef GPlatesMaths::PolylineOnSphere polyline_type;

	/**
	 * This typedef is used wherever geometry (of some unknown type) is expected.
	 * It is a boost::optional because creation of geometry may fail for various reasons.
	 */
	typedef boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> geometry_opt_ptr_type;

}


geometry_opt_ptr_type
GPlatesUtils::create_point_on_sphere(
		const std::vector<GPlatesMaths::PointOnSphere> &points,
		GPlatesUtils::GeometryConstruction::GeometryConstructionValidity &validity)
{
	if (points.size() > 0) {
		validity = GeometryConstruction::VALID;
		return points.front().clone_as_geometry();
	} else {
		validity = GeometryConstruction::INVALID_INSUFFICIENT_POINTS;
		return boost::none;
	}
}


geometry_opt_ptr_type
GPlatesUtils::create_polyline_on_sphere(
		const std::vector<GPlatesMaths::PointOnSphere> &points,
		GPlatesUtils::GeometryConstruction::GeometryConstructionValidity &validity)
{
	// Set up the return-parameter for the evaluate_construction_parameter_validity() function.
	std::pair<
		std::vector<GPlatesMaths::PointOnSphere>::const_iterator, 
		std::vector<GPlatesMaths::PointOnSphere>::const_iterator>
			invalid_points;
	
	// Evaluate construction parameter validity, and create.
	polyline_type::ConstructionParameterValidity polyline_validity = 
			polyline_type::evaluate_construction_parameter_validity(points, invalid_points);

	// Create the polyline and return it - if we can.
	switch (polyline_validity)
	{
	case polyline_type::VALID:
		validity = GeometryConstruction::VALID;
		// Note that create_on_heap gives us a PolylineOnSphere::non_null_ptr_to_const_type,
		// while we are returning it as an optional GeometryOnSphere::non_null_ptr_to_const_type.
		return GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type(
				polyline_type::create_on_heap(points));
	
	case polyline_type::INVALID_INSUFFICIENT_DISTINCT_POINTS:
		// TODO:  In the future, it would be nice to highlight any invalid points
		// for the user, since this information is returned in 'invalid_points'.
		validity = GeometryConstruction::INVALID_INSUFFICIENT_POINTS;
		return boost::none;

	case polyline_type::INVALID_ANTIPODAL_SEGMENT_ENDPOINTS:
		// TODO:  In the future, it would be nice to highlight any invalid points
		// for the user, since this information is returned in 'invalid_points'.
		validity = GeometryConstruction::INVALID_ANTIPODAL_SEGMENT_ENDPOINTS;
		return boost::none;
		
	default:
		// FIXME: Exception.
		qDebug() << "UNKNOWN/UNHANDLED polyline evaluate_construction_parameter_validity," << validity;
		return boost::none;
	}
}


geometry_opt_ptr_type
GPlatesUtils::create_polygon_on_sphere(
		const std::vector<GPlatesMaths::PointOnSphere> &points,
		GPlatesUtils::GeometryConstruction::GeometryConstructionValidity &validity)
{
	// Set up the return-parameter for the evaluate_construction_parameter_validity() function.
	std::pair<
		std::vector<GPlatesMaths::PointOnSphere>::const_iterator, 
		std::vector<GPlatesMaths::PointOnSphere>::const_iterator>
			invalid_points;

	// Evaluate construction parameter validity, and create.
	polygon_type::ConstructionParameterValidity polygon_validity = 
			polygon_type::evaluate_construction_parameter_validity(points, invalid_points);

	// Create the polygon and return it - if we can.
	switch (polygon_validity)
	{
	case polygon_type::VALID:
		validity = GeometryConstruction::VALID;
		// Note that create_on_heap gives us a PolygonOnSphere::non_null_ptr_to_const_type,
		// while we are returning it as an optional GeometryOnSphere::non_null_ptr_to_const_type.
		return GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type(
				polygon_type::create_on_heap(points));
	
	case polygon_type::INVALID_INSUFFICIENT_DISTINCT_POINTS:
		// TODO:  In the future, it would be nice to highlight any invalid points
		// for the user, since this information is returned in 'invalid_points'.
		validity = GeometryConstruction::INVALID_INSUFFICIENT_POINTS;
		return boost::none;

	case polygon_type::INVALID_ANTIPODAL_SEGMENT_ENDPOINTS:
		// TODO:  In the future, it would be nice to highlight any invalid points
		// for the user, since this information is returned in 'invalid_points'.
		validity = GeometryConstruction::INVALID_ANTIPODAL_SEGMENT_ENDPOINTS;
		return boost::none;
		
	default:
		// FIXME: Exception.
		qDebug() << "UNKNOWN/UNHANDLED polygon evaluate_construction_parameter_validity," << validity;
		return boost::none;
	}
}


geometry_opt_ptr_type
GPlatesUtils::create_multipoint_on_sphere(
		const std::vector<GPlatesMaths::PointOnSphere> &points,
		GPlatesUtils::GeometryConstruction::GeometryConstructionValidity &validity)
{
	// Evaluate construction parameter validity, and create.
	multi_point_type::ConstructionParameterValidity multi_point_validity = 
			multi_point_type::evaluate_construction_parameter_validity(points);

	// Create the multi_point and return it - if we can.
	switch (multi_point_validity)
	{
	case multi_point_type::VALID:
		validity = GeometryConstruction::VALID;
		// Note that create_on_heap gives us a MultiPointOnSphere::non_null_ptr_to_const_type,
		// while we are returning it as an optional GeometryOnSphere::non_null_ptr_to_const_type.
		return GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type(
				multi_point_type::create_on_heap(points));

	case multi_point_type::INVALID_INSUFFICIENT_POINTS:
		// TODO:  In the future, it would be nice to highlight any invalid points
		// for the user, since this information is returned in 'invalid_points'.
		validity = GeometryConstruction::INVALID_INSUFFICIENT_POINTS;
		return boost::none;

	default:
		// FIXME: Exception.
		qDebug() << "UNKNOWN/UNHANDLED multi-point evaluate_construction_parameter_validity," << validity;
		return boost::none;
	}
}

