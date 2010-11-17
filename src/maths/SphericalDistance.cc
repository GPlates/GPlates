/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004, 2005, 2006 The University of Sydney, Australia
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

#include <math.h>
#include <vector>

#include "types.h"
#include "GreatCircle.h"
#include "GreatCircleArc.h"
#include "LatLonPoint.h"
#include "MultiPointOnSphere.h"
#include "PointOnSphere.h"
#include "PointInPolygon.h"
#include "PolylineOnSphere.h"
#include "PolygonOnSphere.h"
#include "SphericalDistance.h"
#include "UnitVector3D.h"
#include "Vector3D.h"

namespace
{
	using namespace GPlatesMaths;
	/*
	* TODO:
	*/
	template <class MultiEdgesGeometry_1, class MultiEdgesGeometry_2>
	real_t
	min_dot_product_distance_of_two_multi_edges_geometries(
			const MultiEdgesGeometry_1 &geo_1,
			const MultiEdgesGeometry_2 &geo_2,
			const real_t threshold)
	{
		using namespace GPlatesMaths;
		using namespace GPlatesMaths::Spherical;
		real_t temp_min_distance = MaxDotProductDistanceOnSphere;
		real_t min_distance_ret = MaxDotProductDistanceOnSphere;
		typename MultiEdgesGeometry_1::const_iterator iter_1 = geo_1.begin(), iter_end_1 = geo_1.end();
		typename MultiEdgesGeometry_2::const_iterator iter_2 = geo_2.begin(), iter_end_2 = geo_2.end();
		for(; iter_1 != iter_end_1; iter_1++)
		{
			for(; iter_2 != iter_end_2; iter_2++)
			{
				if(do_great_circle_arcs_intersect(*iter_1, *iter_2))
				{
					return 0.0;
				}
				temp_min_distance = min_dot_product_distance_between_great_circle_arcs(*iter_1,*iter_2);
				if(temp_min_distance > min_distance_ret)//the greater the dot product, the closer the distance.		
				{						 
					min_distance_ret = temp_min_distance;      
				}       
				if(min_distance_ret > threshold || ZeroDotProductDistanceOnSphere == min_distance_ret)
				{
					return min_distance_ret;
				}
			}
		}
		return min_distance_ret;
	}


	/*
	* TODO:
	*/
	template <class Geometry>
	bool
	test_multi_vertex_in_polygon(
			const Geometry& geo,
			const GPlatesMaths::PolygonOnSphere& polygon)
	{
		using namespace GPlatesMaths;
		typename Geometry::VertexConstIterator v_iter = geo.vertex_begin();
		typename Geometry::VertexConstIterator v_iter_end = geo.vertex_end();
		bool inside = true;
		for(; v_iter != v_iter_end; v_iter++)
		{
			using namespace PointInPolygon;
			if(POINT_OUTSIDE_POLYGON == test_point_in_polygon(*v_iter, polygon.get_non_null_pointer()))
			{
				inside = false;
				break;
			}
		}
		return inside;
	}


	/*
	* TODO:
	*/
	bool
	is_point_on_arc_given_they_on_same_great_circle(
			const GreatCircleArc	&arc,
			const PointOnSphere		&point)
	{
		real_t arc_length = 
			dot(arc.start_point().position_vector(), arc.end_point().position_vector());
		real_t point_to_arc_start_length =
			dot(point.position_vector(),arc.start_point().position_vector());
		real_t point_to_arc_end_length =
			dot(point.position_vector(),arc.end_point().position_vector());
		return	point_to_arc_start_length <= arc_length ||
				point_to_arc_end_length <= arc_length;
	}

}


bool 
GPlatesMaths::Spherical::do_great_circle_arcs_intersect(
		const GreatCircleArc  &arc_1,
		const GreatCircleArc  &arc_2,
		PointOnSphere *intersection_point)
{
	if ( ! arcs_are_near_each_other(arc_1, arc_2)) 
	{
		//fast check to see if it is possible that the two arcs could intersect.
		return false;
	}
	
	if (arc_1.is_zero_length() && arc_2.is_zero_length()) //when both of arcs are points.
	{
		if(arc_1.start_point() == arc_2.start_point())
		{
			*intersection_point = arc_1.start_point();
			return true;
		}
		else
		{
			return false;
		}
		
	}else if(arc_1.is_zero_length())//when arc1 is point
	{
		if(arc_1.start_point().lies_on_gca(arc_2))
		{
			*intersection_point = arc_1.start_point();
			return true;
		}
		else
		{
			return false;
		}
	}else if(arc_2.is_zero_length())//when arc2 is point.
	{
		if(arc_2.start_point().lies_on_gca(arc_1))
		{
			*intersection_point = arc_2.start_point();
			return true;
		}
		else
		{
			return false;
		}
	}

	GPlatesMaths::Vector3D v = cross(arc_1.rotation_axis(), arc_2.rotation_axis());
	if (v.magSqrd() <= 0.0)//when two arcs are on the same great circle.
	{
		// arcs have same rotation axis (or opposite rotation axis)
		// so must lie on same great circle
		if(is_point_on_arc_given_they_on_same_great_circle(arc_2, arc_1.start_point())) 
		{
			*intersection_point = arc_1.start_point();
			return true;
		}
		if(is_point_on_arc_given_they_on_same_great_circle(arc_2, arc_1.end_point())) 
		{
			*intersection_point = arc_1.end_point();
			return true;
		}
		if(is_point_on_arc_given_they_on_same_great_circle(arc_1, arc_2.start_point())) 
		{
			*intersection_point = arc_2.start_point();
			return true;
		}
		if(is_point_on_arc_given_they_on_same_great_circle(arc_1, arc_2.start_point())) 
		{
			*intersection_point = arc_2.end_point();
			return true;
		}
		return false;
	}

	GPlatesMaths::UnitVector3D normalised_v = v.get_normalisation();

	PointOnSphere inter_point1(normalised_v);
	PointOnSphere inter_point2( -normalised_v);
	if( is_point_on_arc_given_they_on_same_great_circle(arc_1,inter_point1) 
		&&
		is_point_on_arc_given_they_on_same_great_circle(arc_2,inter_point1) )
	{
		*intersection_point = inter_point1;
		return true;
	}
	else if (
		is_point_on_arc_given_they_on_same_great_circle(arc_1,inter_point2) 
		&&
		is_point_on_arc_given_they_on_same_great_circle(arc_2,inter_point2) )
	{
		*intersection_point = inter_point2;
		return true;
	}
	else
	{
		return false;
	}
}


real_t 
GPlatesMaths::Spherical::min_dot_product_distance_between_great_circle_arc_and_point(
		const GreatCircleArc &arc,
		const PointOnSphere  &point,
		PointOnSphere *closest_point)
{
	//if the arc is a point.
	if(arc.is_zero_length()) 
	{
		if(closest_point)
		{
			*closest_point = arc.start_point().position_vector();
		}
		return dot(
					point.position_vector(), 
					arc.start_point().position_vector());
	}
	else
	{
		const UnitVector3D &n = arc.rotation_axis(); 
		const UnitVector3D &t = point.position_vector();

		const UnitVector3D &a = arc.start_point().position_vector();
		const UnitVector3D &b = arc.end_point().position_vector();

		//calculate the closest point (c) on the great circle 
		Vector3D proj = dot(t, n) * n;
		Vector3D perp = GPlatesMaths::Vector3D(t) - proj;
		const UnitVector3D c = perp.get_normalisation();
		
		real_t closeness_a_to_b = dot(a, b);
		real_t closeness_c_to_a = dot(c, a);
		real_t closeness_c_to_b = dot(c, b);

		if (closeness_c_to_a >= closeness_a_to_b &&
			closeness_c_to_b >= closeness_a_to_b) 
		{
			//if the great circle arc containers c
			if(closest_point)
			{
				*closest_point = c;
			}		
			return dot(t, c);
		} 
		else 
		{
			if (closeness_c_to_a >= closeness_c_to_b) 
			{
				//if start point is closer.
				if(closest_point)
				{
					*closest_point = arc.start_point().position_vector();
				}		
				return dot(t, a);
			}
			else
			{
				//if end point is closer.
				if(closest_point)
				{
					*closest_point = arc.end_point().position_vector();
				}		
				return dot(t, b);
			}
		}
	}
}


/*
* TODO: describe the algorithm
*/ 
real_t 
GPlatesMaths::Spherical::min_dot_product_distance_between_great_circle_arcs(
		const GreatCircleArc &arc1,
		const GreatCircleArc &arc2,
		PointOnSphere *closest_point_1,
		PointOnSphere *closest_point_2)
{
	PointOnSphere arc1_to_arc2_start;
	PointOnSphere arc1_to_arc2_end;
	PointOnSphere arc2_to_arc1_start;
	PointOnSphere arc2_to_arc1_end;
    real_t distance_arc1_to_arc2_start = 
			min_dot_product_distance_between_great_circle_arc_and_point(arc1, arc2.start_point(), &arc1_to_arc2_start);
    real_t distance_arc1_to_arc2_end = 
			min_dot_product_distance_between_great_circle_arc_and_point(arc1, arc2.end_point(), &arc1_to_arc2_end);
    real_t distance_arc2_to_arc1_start = 
			min_dot_product_distance_between_great_circle_arc_and_point(arc2, arc1.start_point(), &arc2_to_arc1_start);
    real_t distance_arc2_to_arc1_end = 
			min_dot_product_distance_between_great_circle_arc_and_point(arc2, arc1.end_point(), &arc2_to_arc1_end);
		
    real_t distance = distance_arc1_to_arc2_start;
	if(closest_point_1 && closest_point_2)
	{
		*closest_point_1 = arc1_to_arc2_start;
		*closest_point_2 = arc2.start_point().position_vector();
	}
    
	if ( distance_arc1_to_arc2_end < distance )
    {
        distance = distance_arc1_to_arc2_end;
		if(closest_point_1 && closest_point_2)
		{
			*closest_point_1 = arc1_to_arc2_end;
			*closest_point_2 = arc2.end_point().position_vector();
		}
    }

    if ( distance_arc2_to_arc1_start < distance )
    {
        distance = distance_arc2_to_arc1_start;
		if(closest_point_1 && closest_point_2)
		{
			*closest_point_1 = arc1.start_point().position_vector();
			*closest_point_2 = arc2_to_arc1_start;
		}
	}

    if ( distance_arc2_to_arc1_end < distance )
    {
        distance = distance_arc2_to_arc1_end;
		if(distance_arc2_to_arc1_start < distance)
		{
			*closest_point_1 = arc1.end_point().position_vector();
		    *closest_point_2 = arc2_to_arc1_end;
		}
    }

    return distance;
}


real_t 
GPlatesMaths::Spherical::details::min_dot_product_distance(
		const PointOnSphere  &point_1,
		const PointOnSphere  &point_2,
		const real_t threshold)
{
	return dot(point_1.position_vector(), point_2.position_vector());
}


real_t 
GPlatesMaths::Spherical::details::min_dot_product_distance(
		const PointOnSphere		&point,
		const PolylineOnSphere  &polyline,
		const real_t threshold)
{
	real_t temp_min_distance = MaxDotProductDistanceOnSphere;
	real_t min_distance_ret = MaxDotProductDistanceOnSphere;
	PolylineOnSphere::const_iterator iter = polyline.begin();
	PolylineOnSphere::const_iterator iter_end = polyline.end();
	for ( ; iter != iter_end; ++iter) 
	{
		temp_min_distance = min_dot_product_distance_between_great_circle_arc_and_point(*iter, point);
		if(temp_min_distance > min_distance_ret)//the greater the dot product, the closer the distance.		
		{						 
			min_distance_ret = temp_min_distance;      
		}       
		if(min_distance_ret > threshold || ZeroDotProductDistanceOnSphere == min_distance_ret )
		{
			return min_distance_ret;
		}
	}
	return min_distance_ret;
}


/*
* TODO: probably we need to consider the interior rings of polygon(holes?)
*/
real_t 
GPlatesMaths::Spherical::details::min_dot_product_distance(
		const PointOnSphere		&point,
		const PolygonOnSphere   &polygon,
		const real_t threshold,
		const bool distance_to_boundary)
{
	using namespace PointInPolygon;
	if( (!distance_to_boundary) && 
		(POINT_OUTSIDE_POLYGON != test_point_in_polygon(point, polygon.get_non_null_pointer())))
	{
		return 0.0;
	}
	
	real_t temp_min_distance = MaxDotProductDistanceOnSphere;
	real_t min_distance_ret = MaxDotProductDistanceOnSphere;
	PolygonOnSphere::const_iterator iter = polygon.begin();
	PolygonOnSphere::const_iterator iter_end = polygon.end();
	for ( ; iter != iter_end; ++iter) 
	{
		const GreatCircleArc &the_gca = *iter;
		temp_min_distance =min_dot_product_distance_between_great_circle_arc_and_point(the_gca,point);
		if(temp_min_distance > min_distance_ret)//the greater the dot product, the closer the distance.			
		{						 
			min_distance_ret = temp_min_distance;      
		}       
		if(min_distance_ret > threshold || ZeroDotProductDistanceOnSphere == min_distance_ret )
		{
			return min_distance_ret;
		}
	}
	return min_distance_ret;
}


real_t 
GPlatesMaths::Spherical::details::min_dot_product_distance(
		const PointOnSphere			&point,
		const MultiPointOnSphere	&multipoint,
		const real_t threshold)
{
	real_t temp_min_distance = MaxDotProductDistanceOnSphere;
	real_t min_distance_ret = MaxDotProductDistanceOnSphere;
	MultiPointOnSphere::const_iterator iter = multipoint.begin(), iter_end = multipoint.end();
	for ( ; iter != iter_end; ++iter) 
	{
		temp_min_distance = min_dot_product_distance(*iter, point);
		if(temp_min_distance > min_distance_ret)//the greater the dot product, the closer the distance.			
		{						 
			min_distance_ret = temp_min_distance;      
		}       
		if(min_distance_ret > threshold || ZeroDotProductDistanceOnSphere == min_distance_ret)
		{
			return min_distance_ret;
		}
	}
	return min_distance_ret;
}


real_t 
GPlatesMaths::Spherical::details::min_dot_product_distance(
		const PolylineOnSphere	&polyline_1,
		const PolylineOnSphere	&polyline_2,
		const real_t threshold)
{
	return min_dot_product_distance_of_two_multi_edges_geometries(polyline_1,polyline_2,threshold);
}


/*
* TODO: probably we need to consider the interior rings of polygon(holes?)
*/
real_t 
GPlatesMaths::Spherical::details::min_dot_product_distance(
		const PolylineOnSphere	&polyline,
		const PolygonOnSphere	&polygon,
		const real_t threshold,
		const bool distance_to_boundary)
{
	if(!distance_to_boundary && test_multi_vertex_in_polygon(polyline,polygon))
	{
		return 0.0;
	}
	return  min_dot_product_distance_of_two_multi_edges_geometries(polyline,polygon,threshold);
}


real_t 
GPlatesMaths::Spherical::details::min_dot_product_distance(
		const PolylineOnSphere		&polyline,
		const MultiPointOnSphere	&multipoint,
		const real_t threshold)
{
	real_t temp_min_distance = MaxDotProductDistanceOnSphere;
	real_t min_distance_ret = MaxDotProductDistanceOnSphere;
	MultiPointOnSphere::const_iterator iter = multipoint.begin(), iter_end = multipoint.end();
	for ( ; iter != iter_end; ++iter) 
	{
		temp_min_distance = min_dot_product_distance(*iter, polyline, threshold);
		if(temp_min_distance > min_distance_ret)//the greater the dot product, the closer the distance.			
		{						 
			min_distance_ret = temp_min_distance;      
		}       
		if(min_distance_ret > threshold || ZeroDotProductDistanceOnSphere == min_distance_ret)
		{
			return min_distance_ret;
		}
	}
	return min_distance_ret;
}


/*
* TODO: probably we need to consider the interior rings of polygon(holes?)
*/
real_t 
GPlatesMaths::Spherical::details::min_dot_product_distance(
		const PolygonOnSphere	&polygon_1,
		const PolygonOnSphere	&polygon_2,
		const real_t threshold,
		const bool distance_to_boundary)
{
	//TODO: should filter out no-hopers first
	if(
		!distance_to_boundary && 
		(test_multi_vertex_in_polygon(polygon_1,polygon_2) ||
		 test_multi_vertex_in_polygon(polygon_2,polygon_1)))
	{
		return 0.0;
	}
	return  min_dot_product_distance_of_two_multi_edges_geometries(polygon_1,polygon_2,threshold);
}


/*
* TODO: probably we need to consider the interior rings of polygon(holes?)
*/
real_t 
GPlatesMaths::Spherical::details::min_dot_product_distance(
		const PolygonOnSphere		&polygon,
		const MultiPointOnSphere	&multipoint,
		const real_t threshold,
		const bool distance_to_boundary)
{
	real_t temp_min_distance = MaxDotProductDistanceOnSphere;
	real_t min_distance_ret = MaxDotProductDistanceOnSphere;
	MultiPointOnSphere::const_iterator iter = multipoint.begin();
	MultiPointOnSphere::const_iterator iter_end = multipoint.end();
	for(; iter != iter_end; iter++)
	{
		temp_min_distance = min_dot_product_distance(*iter,polygon,threshold,distance_to_boundary);
		if(temp_min_distance > min_distance_ret)//the greater the dot product, the closer the distance.			
		{						 
			min_distance_ret = temp_min_distance;      
		}       
		if(min_distance_ret > threshold || ZeroDotProductDistanceOnSphere == min_distance_ret)
		{
			return min_distance_ret;
		}
	}
	return min_distance_ret;
}


real_t 
GPlatesMaths::Spherical::details::min_dot_product_distance(
		const MultiPointOnSphere	&multipoint_1,
		const MultiPointOnSphere	&multipoint_2,
		const real_t threshold)
{
	real_t temp_min_distance = MaxDotProductDistanceOnSphere;
	real_t min_distance_ret = MaxDotProductDistanceOnSphere;
	MultiPointOnSphere::const_iterator iter_1 = multipoint_1.begin(), iter_1_end = multipoint_1.end();
	MultiPointOnSphere::const_iterator iter_2 = multipoint_2.begin(), iter_2_end = multipoint_2.end();
	for ( ; iter_1 != iter_1_end; ++iter_1) 
	{
		for ( ; iter_2 != iter_2_end; ++iter_2) 
		{
			temp_min_distance = min_dot_product_distance(*iter_1, *iter_2);
			if(temp_min_distance > min_distance_ret)//the greater the dot product, the closer the distance.			
			{						 
				min_distance_ret = temp_min_distance;      
			}       
			if(min_distance_ret > threshold || ZeroDotProductDistanceOnSphere == min_distance_ret)
			{
				return min_distance_ret;
			}
		}
	}
	return min_distance_ret;
}






