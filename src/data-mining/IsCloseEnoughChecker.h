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
#ifndef GPLATESDATAMINING_ISCLOSEENOUGHCHECKER_H
#define GPLATESDATAMINING_ISCLOSEENOUGHCHECKER_H

#include <vector>
#include <bitset>

#include <QDebug>

#include <boost/optional.hpp>

#include "maths/ProximityHitDetail.h"
#include "maths/ProximityCriteria.h"
#include "maths/MathsUtils.h"
#include "maths/PointInPolygon.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"

namespace GPlatesDataMining
{
	using namespace GPlatesMaths;

	const double DEFAULT_RADIUS_OF_EARTH_KMS = 6378.1;

	/*
	* TODO: more comments
	* Until function. Users are encouraged to use this function.
	*/
	bool
	is_close_enough(
			const GeometryOnSphere& g1, 
			const GeometryOnSphere& g2,
			const double range);

	/*
	* TODO:
	*/
	class IsCloseEnoughChecker
	{
	public:

		IsCloseEnoughChecker(
				const double range,
				bool calculate_distance_flag = false) :
			d_is_close_enough(false),
			d_calculate_distance_flag(calculate_distance_flag),
			d_distance(0.0),
			d_range(range)
		{ }

		
		inline
		bool
		is_close_enough()
		{
		  return d_is_close_enough;
		}


		inline
		boost::optional<double>
		distance()
		{
			if(d_calculate_distance_flag)
			{
				return d_distance;
			}
			else
			{
				return boost::none;
			}
		}

		/*
		* Overload all possible composition
		* Tried template, quite tricky.
		* To make it maintainable, choose easy and clumsy way.
		*/
		void
		execute(
				const PointOnSphere* point1,
				const PointOnSphere* point2);

		void
		execute(
				const PointOnSphere* point,
				const PolylineOnSphere* polyline);

		void
		execute(
				const PointOnSphere* point,
				const PolygonOnSphere* polygon);

		void
		execute(
				const PointOnSphere* point,
				const MultiPointOnSphere* multi_point);

		inline
		void
		execute(
				const PolylineOnSphere* polyline,
				const PointOnSphere* point)
		{
			execute(point, polyline);
		}

		void
		execute(
				const PolylineOnSphere* polyline1,
				const PolylineOnSphere* polyline2);

		void
		execute(
				const PolylineOnSphere* polyline,
				const PolygonOnSphere* polygon);

		void
		execute(
				const PolylineOnSphere* polyline,
				const MultiPointOnSphere* multi_point);

		inline
		void
		execute(
				const PolygonOnSphere* polygon,
				const PointOnSphere* point)
		{
			execute(point, polygon);
		}

		inline
		void
		execute(
				const PolygonOnSphere* polygon,
				const PolylineOnSphere* polyline)
		{
			execute(polyline, polygon);
		}

		void
		execute(
				const PolygonOnSphere* polygon1,
				const PolygonOnSphere* polygon2);

		void
		execute(
				const PolygonOnSphere* polygon,
				const MultiPointOnSphere* multi_point);

		inline
		void
		execute(
				const MultiPointOnSphere* multi_point,
				const PointOnSphere* point)
		{
			execute(point, multi_point);
		}

		inline
		void
		execute(
				const MultiPointOnSphere* multi_point,
				const PolylineOnSphere* polyline)
		{
			execute(polyline, multi_point);
		}

		inline
		void
		execute(
				const MultiPointOnSphere* multi_point,
				const PolygonOnSphere* polygon)
		{
			execute(polygon, multi_point);
		}

		void
		execute(
				const MultiPointOnSphere* multi_point1,
				const MultiPointOnSphere* multi_point2);

	protected:

		/*
		* TODO: use this function temporarily
		* a new faster function should be implemented.
		*/
		template<class GeometryType>
		boost::optional<double>
		test_proximity(
				const GeometryType*, 
				const PointOnSphere*,
				const double);
		
	private:
		bool d_is_close_enough;
		bool d_calculate_distance_flag;
		double d_distance;
		const double d_range;
	};
}


#endif




