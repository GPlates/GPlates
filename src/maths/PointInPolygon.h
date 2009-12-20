/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATES_MATHS_POINTINPOLYGON_H
#define GPLATES_MATHS_POINTINPOLYGON_H

#include <boost/any.hpp>

#include "PolygonOnSphere.h"


namespace GPlatesMaths
{
	class PointOnSphere;

	namespace PointInPolygon
	{
		/**
		 * Result of testing where a point is located relative to a polygon.
		 */
		enum Result
		{
			POINT_OUTSIDE_POLYGON,
			POINT_ON_POLYGON,
			POINT_INSIDE_POLYGON
		};


		/**
		 * Typedef for an optimised polygon memento. 
		 *
		 * Being a memento it is only understand by the implementation of this class.
		 */
		typedef boost::any optimised_polygon_type;


		/**
		 * Creates a polygon that's optimised for point-in-polygon tests from
		 * @a polygon.
		 */
		optimised_polygon_type
		create_optimised_polygon(
				const PolygonOnSphere::non_null_ptr_to_const_type &polygon);


		/**
		 * Tests if @a point is outside, inside or on boundary of a polygon.
		 *
		 * Using this function you can test multiple points against the same polygon
		 * more efficiently than using the other overload of @a test_point_in_polygon.
		 */
		Result
		test_point_in_polygon(
				const PointOnSphere &point,
				const optimised_polygon_type &optimised_polygon);


		/**
		 * Tests if @a point is outside, inside or on boundary of @a polygon.
		 *
		 * This is useful when only testing a single point against a specific polygon.
		 * Otherwise use the other overload of @a test_point_in_polygon.
		 *
		 * Internally does the equivalent of calling @a create_optimised_polygon
		 * followed by @a test_point_in_polygon.
		 */
		Result
		test_point_in_polygon(
				const PointOnSphere &point,
				const PolygonOnSphere::non_null_ptr_to_const_type &polygon);
	}
}

#endif // GPLATES_MATHS_POINTINPOLYGON_H
