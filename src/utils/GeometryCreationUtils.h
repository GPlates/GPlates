/* $Id$ */

/**
 * \file 
 * A collection of functions to aid in the creation of GeometryOnSphere
 * derivations from user input.
 *
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2009 The University of Sydney, Australia
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

#include <iterator>
#include <vector>
#include <boost/none.hpp>
#include <boost/optional.hpp>
#include <QDebug>

#include "maths/GeometryOnSphere.h"
#include "maths/GeometryType.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"


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
	 * Creates a @a GeometryOnSphere according to the specified geometry type and points.
	 *
	 * @a validity is a return-parameter. It will be set to
	 * GeometryConstruction::VALID if everything went ok. In the event
	 * of construction problems occuring, it will indicate why construction
	 * failed.
	 */
	template <typename ForwardIterPointOnSphere>
	boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
	create_geometry_on_sphere(
			GPlatesMaths::GeometryType::Value geometry_type,
			ForwardIterPointOnSphere begin_points_on_sphere,
			ForwardIterPointOnSphere end_points_on_sphere,
			GPlatesUtils::GeometryConstruction::GeometryConstructionValidity &validity);


	/**
	 * An overload of @a create_geometry_on_sphere accepting a vector of points.
	 */
	inline
	boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
	create_geometry_on_sphere(
			GPlatesMaths::GeometryType::Value geometry_type,
			const std::vector<GPlatesMaths::PointOnSphere> &points,
			GeometryConstruction::GeometryConstructionValidity &validity)
	{
		return create_geometry_on_sphere(geometry_type, points.begin(), points.end(), validity);
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
	 * PointOnSphere::non_null_ptr_to_const_type.
	 */
	template <typename ForwardIterPointOnSphere>
	boost::optional<GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type>
	create_point_on_sphere(
			ForwardIterPointOnSphere begin_points_on_sphere,
			ForwardIterPointOnSphere end_points_on_sphere,
			GPlatesUtils::GeometryConstruction::GeometryConstructionValidity &validity);


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
	 * PointOnSphere::non_null_ptr_to_const_type.
	 */
	inline
	boost::optional<GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type>
	create_point_on_sphere(
			const std::vector<GPlatesMaths::PointOnSphere> &points,
			GeometryConstruction::GeometryConstructionValidity &validity)
	{
		return create_point_on_sphere(points.begin(), points.end(), validity);
	}

	/**
	 * Creates a single PolylineOnSphere (assuming >= 2 distinct points are provided).
	 *
	 * @a validity is a return-parameter. It will be set to
	 * GeometryConstruction::VALID if everything went ok. In the event
	 * of construction problems occuring, it will indicate why construction
	 * failed.
	 *
	 * Note we are returning a possibly-none boost::optional of
	 * PolylineOnSphere::non_null_ptr_to_const_type.
	 */
	template <typename ForwardIterPointOnSphere>
	boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>
	create_polyline_on_sphere(
			ForwardIterPointOnSphere begin_points_on_sphere,
			ForwardIterPointOnSphere end_points_on_sphere,
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
	 * PolylineOnSphere::non_null_ptr_to_const_type.
	 */
	inline
	boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>
	create_polyline_on_sphere(
			const std::vector<GPlatesMaths::PointOnSphere> &points,
			GeometryConstruction::GeometryConstructionValidity &validity)
	{
		return create_polyline_on_sphere(points.begin(), points.end(), validity);
	}


	/**
	 * Creates a single PolygonOnSphere (assuming >= 3 distinct points are provided).
	 *
	 * @a validity is a return-parameter. It will be set to
	 * GeometryConstruction::VALID if everything went ok. In the event
	 * of construction problems occuring, it will indicate why construction
	 * failed.
	 *
	 * Note we are returning a possibly-none boost::optional of
	 * PolygonOnSphere::non_null_ptr_to_const_type.
	 */
	template <typename ForwardIterPointOnSphere>
	boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type>
	create_polygon_on_sphere(
			ForwardIterPointOnSphere begin_points_on_sphere,
			ForwardIterPointOnSphere end_points_on_sphere,
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
	 * PolygonOnSphere::non_null_ptr_to_const_type.
	 */
	inline
	boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type>
	create_polygon_on_sphere(
			const std::vector<GPlatesMaths::PointOnSphere> &points,
			GeometryConstruction::GeometryConstructionValidity &validity)
	{
		return create_polygon_on_sphere(points.begin(), points.end(), validity);
	}


	/**
	 * Creates a single MultiPointOnSphere (assuming >= 1 point is provided).
	 *
	 * @a validity is a return-parameter. It will be set to
	 * GeometryConstruction::VALID if everything went ok. In the event
	 * of construction problems occuring, it will indicate why construction
	 * failed.
	 *
	 * Note we are returning a possibly-none boost::optional of
	 * MultiPointOnSphere::non_null_ptr_to_const_type.
	 */
	template <typename ForwardIterPointOnSphere>
	boost::optional<GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type>
	create_multipoint_on_sphere(
			ForwardIterPointOnSphere begin_points_on_sphere,
			ForwardIterPointOnSphere end_points_on_sphere,
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
	 * MultiPointOnSphere::non_null_ptr_to_const_type.
	 */
	inline
	boost::optional<GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type>
	create_multipoint_on_sphere(
			const std::vector<GPlatesMaths::PointOnSphere> &points,
			GeometryConstruction::GeometryConstructionValidity &validity)
	{
		return create_multipoint_on_sphere(points.begin(), points.end(), validity);
	}
}

//
// Implementation of template functions.
//

namespace GPlatesUtils
{
	template <typename ForwardIterPointOnSphere>
	boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
	create_geometry_on_sphere(
			GPlatesMaths::GeometryType::Value geometry_type,
			ForwardIterPointOnSphere begin_points_on_sphere,
			ForwardIterPointOnSphere end_points_on_sphere,
			GPlatesUtils::GeometryConstruction::GeometryConstructionValidity &validity)
	{
		switch (geometry_type)
		{
		case GPlatesMaths::GeometryType::POINT:
			{
				boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> geometry;

				boost::optional<GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type> point_on_sphere =
						create_point_on_sphere(begin_points_on_sphere, end_points_on_sphere, validity);
				if (point_on_sphere)
				{
					geometry = GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type(point_on_sphere.get());
				}

				return geometry;
			}

		case GPlatesMaths::GeometryType::MULTIPOINT:
			{
				boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> geometry;

				boost::optional<GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type> multipoint_on_sphere =
						create_multipoint_on_sphere(begin_points_on_sphere, end_points_on_sphere, validity);
				if (multipoint_on_sphere)
				{
					geometry = GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type(multipoint_on_sphere.get());
				}

				return geometry;
			}

		case GPlatesMaths::GeometryType::POLYLINE:
			{
				boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> geometry;

				boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> polyline_on_sphere =
						create_polyline_on_sphere(begin_points_on_sphere, end_points_on_sphere, validity);
				if (polyline_on_sphere)
				{
					geometry = GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type(polyline_on_sphere.get());
				}

				return geometry;
			}

		case GPlatesMaths::GeometryType::POLYGON:
			{
				boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> geometry;

				boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> polygon_on_sphere =
						create_polygon_on_sphere(begin_points_on_sphere, end_points_on_sphere, validity);
				if (polygon_on_sphere)
				{
					geometry = GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type(polygon_on_sphere.get());
				}

				return geometry;
			}

		case GPlatesMaths::GeometryType::NONE:
		default:
			break;
		}

		return boost::none;
	}


	template <typename ForwardIterPointOnSphere>
	boost::optional<GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type>
	create_point_on_sphere(
			ForwardIterPointOnSphere begin_points_on_sphere,
			ForwardIterPointOnSphere end_points_on_sphere,
			GeometryConstruction::GeometryConstructionValidity &validity)
	{
		if (begin_points_on_sphere != end_points_on_sphere) {
			validity = GeometryConstruction::VALID;
			return begin_points_on_sphere->clone_as_point();
		} else {
			validity = GeometryConstruction::INVALID_INSUFFICIENT_POINTS;
			return boost::none;
		}
	}


	template <typename ForwardIterPointOnSphere>
	boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>
	create_polyline_on_sphere(
			ForwardIterPointOnSphere begin_points_on_sphere,
			ForwardIterPointOnSphere end_points_on_sphere,
			GeometryConstruction::GeometryConstructionValidity &validity)
	{
		// Set up the return-parameter for the evaluate_construction_parameter_validity() function.
		std::pair<ForwardIterPointOnSphere, ForwardIterPointOnSphere> invalid_points;
		
		// Evaluate construction parameter validity, and create.
		GPlatesMaths::PolylineOnSphere::ConstructionParameterValidity polyline_validity = 
				GPlatesMaths::PolylineOnSphere::evaluate_construction_parameter_validity(
						begin_points_on_sphere, end_points_on_sphere, invalid_points);

		// Create the polyline and return it - if we can.
		switch (polyline_validity)
		{
		case GPlatesMaths::PolylineOnSphere::VALID:
			validity = GeometryConstruction::VALID;
			// Note that create_on_heap gives us a PolylineOnSphere::non_null_ptr_to_const_type,
			// while we are returning it as an optional GeometryOnSphere::non_null_ptr_to_const_type.
			return GPlatesMaths::PolylineOnSphere::create_on_heap(
					begin_points_on_sphere, end_points_on_sphere);
		
		case GPlatesMaths::PolylineOnSphere::INVALID_INSUFFICIENT_DISTINCT_POINTS:
			// TODO:  In the future, it would be nice to highlight any invalid points
			// for the user, since this information is returned in 'invalid_points'.
			validity = GeometryConstruction::INVALID_INSUFFICIENT_POINTS;
			return boost::none;

		case GPlatesMaths::PolylineOnSphere::INVALID_ANTIPODAL_SEGMENT_ENDPOINTS:
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


	template <typename ForwardIterPointOnSphere>
	boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type>
	create_polygon_on_sphere(
			ForwardIterPointOnSphere begin_points_on_sphere,
			ForwardIterPointOnSphere end_points_on_sphere,
			GeometryConstruction::GeometryConstructionValidity &validity)
	{
		// Set up the return-parameter for the evaluate_construction_parameter_validity() function.
		std::pair<ForwardIterPointOnSphere, ForwardIterPointOnSphere> invalid_points;

		// Evaluate construction parameter validity, and create.
		GPlatesMaths::PolygonOnSphere::ConstructionParameterValidity polygon_validity = 
				GPlatesMaths::PolygonOnSphere::evaluate_construction_parameter_validity(
						begin_points_on_sphere, end_points_on_sphere, invalid_points);

		// Create the polygon and return it - if we can.
		switch (polygon_validity)
		{
		case GPlatesMaths::PolygonOnSphere::VALID:
			validity = GeometryConstruction::VALID;
			// Note that create_on_heap gives us a PolygonOnSphere::non_null_ptr_to_const_type,
			// while we are returning it as an optional GeometryOnSphere::non_null_ptr_to_const_type.
			return GPlatesMaths::PolygonOnSphere::create_on_heap(
					begin_points_on_sphere, end_points_on_sphere);
		
		case GPlatesMaths::PolygonOnSphere::INVALID_INSUFFICIENT_DISTINCT_POINTS:
			// TODO:  In the future, it would be nice to highlight any invalid points
			// for the user, since this information is returned in 'invalid_points'.
			validity = GeometryConstruction::INVALID_INSUFFICIENT_POINTS;
			return boost::none;

		case GPlatesMaths::PolygonOnSphere::INVALID_ANTIPODAL_SEGMENT_ENDPOINTS:
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


	template <typename ForwardIterPointOnSphere>
	boost::optional<GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type>
	create_multipoint_on_sphere(
			ForwardIterPointOnSphere begin_points_on_sphere,
			ForwardIterPointOnSphere end_points_on_sphere,
			GeometryConstruction::GeometryConstructionValidity &validity)
	{
		// Evaluate construction parameter validity, and create.
		GPlatesMaths::MultiPointOnSphere::ConstructionParameterValidity multi_point_validity = 
				GPlatesMaths::MultiPointOnSphere::evaluate_construction_parameter_validity(
						begin_points_on_sphere, end_points_on_sphere);

		// Create the multi_point and return it - if we can.
		switch (multi_point_validity)
		{
		case GPlatesMaths::MultiPointOnSphere::VALID:
			validity = GeometryConstruction::VALID;
			// Note that create_on_heap gives us a MultiPointOnSphere::non_null_ptr_to_const_type,
			// while we are returning it as an optional GeometryOnSphere::non_null_ptr_to_const_type.
			return GPlatesMaths::MultiPointOnSphere::create_on_heap(
					begin_points_on_sphere, end_points_on_sphere);

		case GPlatesMaths::MultiPointOnSphere::INVALID_INSUFFICIENT_POINTS:
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

}


#endif // GPLATES_UTILS_GEOMETRYCREATIONUTILS_H
