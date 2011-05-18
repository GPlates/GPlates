/* $Id$ */

/**
 * \file .
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
#include <iostream>

#include "IsCloseEnoughChecker.h" 
#include "DualGeometryVisitor.h"

#include "maths/SphereSettings.h"

bool
GPlatesDataMining::is_close_enough(
		const GPlatesMaths::GeometryOnSphere& g1, 
		const  GPlatesMaths::GeometryOnSphere& g2,
		const double range)
{
	using namespace GPlatesMaths::Spherical;
	IsCloseEnoughChecker checker(range);
	DualGeometryVisitor< IsCloseEnoughChecker > dual_visitor(g1, g2, &checker);
	dual_visitor.apply();
	return checker.is_close_enough();
}

/*******************************************
* TODO: Implement the calculation of distance 
* between arbitrary two geometries.
* Refer to lwgeom_distance_spheroid() function
* in /postgis-1.5.2/liblwgeom/lwgeodetic.c
* The algorithm should be something like the one
* described in http://cgm.cs.mcgill.ca/~orm/mind2p.html
********************************************/

/*
* Point to point
*/
void
GPlatesDataMining::IsCloseEnoughChecker::execute(
		const GPlatesMaths::PointOnSphere* point1,
		const GPlatesMaths::PointOnSphere* point2)
{
	real_t closeness = acos(calculate_closeness(*point1, *point2)) * DEFAULT_RADIUS_OF_EARTH;

	#ifdef _DEBUG
	std::cout << "Checking if a point is close to another point."<< std::endl;
	std::cout << "The distance is: " << closeness <<std::endl;
	#endif

	if(d_calculate_distance_flag)
	{
		d_distance = closeness.dval();
	}
	
	using namespace GPlatesMaths::Spherical;
	d_is_close_enough =
		(closeness - SphereSettings::instance().distance_tolerance()) > real_t(d_range) ? false : true;
}


/*
* Point to polygon
*/
void
GPlatesDataMining::IsCloseEnoughChecker::execute(
		const GPlatesMaths::PointOnSphere* point,
		const GPlatesMaths::PolygonOnSphere* polygon)
{
	using namespace GPlatesMaths::PointInPolygon;
	//if the point is inside a polygon, we assume the distance is 0
	//TODO: maybe need a flag to control the assumption.
	if(POINT_OUTSIDE_POLYGON != is_point_in_polygon(*point, polygon->get_non_null_pointer()))
	{
		if(d_calculate_distance_flag)
		{
			d_distance = 0.0;
		}
		d_is_close_enough = true;
		return;
	}
	
	#ifdef _DEBUG
	std::cout << "Checking if a point is close to a polygon."<< std::endl;
	#endif
	
	test_proximity(polygon,point,d_range);
}

/*
* Point to polyline
*/
void
GPlatesDataMining::IsCloseEnoughChecker::execute(
		const GPlatesMaths::PointOnSphere* point,
		const GPlatesMaths::PolylineOnSphere* polyline)
{
	#ifdef _DEBUG
	std::cout << "Checking if a point is close to a polyline."<< std::endl;
	#endif

	test_proximity(polyline, point, d_range);
}

/*
* Point to multi points
*/
void
GPlatesDataMining::IsCloseEnoughChecker::execute(
		const GPlatesMaths::PointOnSphere* point,
		const GPlatesMaths::MultiPointOnSphere* multi_point)
{
	#ifdef _DEBUG
	std::cout << "Checking if a point is close to a multipoint."<< std::endl;
	#endif

	test_proximity(multi_point, point, d_range);
}

void
GPlatesDataMining::IsCloseEnoughChecker::execute(
		const GPlatesMaths::PolylineOnSphere* polyline1,
		const GPlatesMaths::PolylineOnSphere* polyline2)
{
	//TODO...
	return ;
}
void
GPlatesDataMining::IsCloseEnoughChecker::execute(
		const GPlatesMaths::PolylineOnSphere* polyline,
		const GPlatesMaths::PolygonOnSphere* polygon)
{
	//TODO...
	return ;
}

void
GPlatesDataMining::IsCloseEnoughChecker::execute(
		const GPlatesMaths::PolylineOnSphere* polyline,
		const GPlatesMaths::MultiPointOnSphere* multi_point)
{
	//TODO...
	return ;
}

/*
* algorithm to calculate the 2-dimensional cartesian minimum distance.
* http://cgm.cs.mcgill.ca/~orm/mind2p.html
*/
void
GPlatesDataMining::IsCloseEnoughChecker::execute(
		const GPlatesMaths::PolygonOnSphere* polygon1,
		const GPlatesMaths::PolygonOnSphere* polygon2)
{
	//TODO...
	return ;
}
void
GPlatesDataMining::IsCloseEnoughChecker::execute(
		const GPlatesMaths::PolygonOnSphere* polygon,
		const GPlatesMaths::MultiPointOnSphere* multi_point)
{
	//TODO...
	return ;
}


void
GPlatesDataMining::IsCloseEnoughChecker::execute(
		const GPlatesMaths::MultiPointOnSphere* multi_point1,
		const GPlatesMaths::MultiPointOnSphere* multi_point2)
{
	//TODO...
	return ;
}

template<class GeometryType>
boost::optional<double>
GPlatesDataMining::IsCloseEnoughChecker::test_proximity(
		const GeometryType* geometry, 
		const GPlatesMaths::PointOnSphere* point,
		const double range)
{

	real_t closeness;
	double thresh_hold = 0;
	
	if(range > (DEFAULT_RADIUS_OF_EARTH * PI) )
	{
		thresh_hold = -1;
	}
	else if(range <= 0)
	{
		thresh_hold = 1;
	}
	else
	{
		thresh_hold = std::cos(range / DEFAULT_RADIUS_OF_EARTH);
	}

	ProximityCriteria proximity_criteria(
			*point,
			thresh_hold);
	
	ProximityHitDetail::maybe_null_ptr_type hit = 
		geometry->test_proximity(proximity_criteria);

	if(hit)
	{
		d_is_close_enough = true;
		d_distance = acos( hit->closeness() ) * DEFAULT_RADIUS_OF_EARTH;

		#ifdef _DEBUG
		std::cout << "The distance is: " << d_distance <<std::endl;
		#endif
		if(d_distance > range)//this could happen due to the float point precision problem.
			d_distance = range;

		return d_distance;
	}
	else
	{
		d_is_close_enough = false;
		return boost::none;
	}
}



