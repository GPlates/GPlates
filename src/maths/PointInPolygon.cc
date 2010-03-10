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

#include <cmath>

#include "PointInPolygon.h"

#include "LatLonPoint.h"
#include "PointOnSphere.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


#if 0
	#define DEBUG_BOUNDS
	#define DEBUG_POINT_IN_POLYGON
#endif


#ifdef _MSC_VER
#define copysign _copysign
#endif


namespace GPlatesMaths
{
	namespace PointInPolygon
	{
		namespace
		{
			/////////////////////////////////////////////////////////////////////////////////
			// FIXME: Replace with an algorithm that doesn't use
			// make_lat_lon_point() since that can take about 1,000 cpu cycles
			// to execute.
			/////////////////////////////////////////////////////////////////////////////////



			/** 
			* struct to hold plate polygon data from resolving topology feature 
			*/
			struct PlatePolygon 
			{
				PlatePolygon(
						const PolygonOnSphere::non_null_ptr_to_const_type &polygon) :
					d_polygon(polygon),
					d_pole(0),
					d_max_lat(-91),
					d_min_lat(91),
					d_max_lon(-181),
					d_min_lon(181)
				{  }


				PolygonOnSphere::non_null_ptr_to_const_type d_polygon;

				/**
	 			* Polar polygon status:
	 			* 0 = no pole in polygon; 1 = North Pole; -1 = South Pole in polygon
				*/
				int d_pole;

				/** geographic bounds */
				double d_max_lat;
				double d_min_lat;
				double d_max_lon;
				double d_min_lon;
			};


			void
			compute_bounds(
					PlatePolygon &plate)
			{
				// tmp vars
				double dlon = 0;
				double lon_sum = 0.0;

				// re-set initial default values to opposite extreame value
				plate.d_max_lat = -91;
				plate.d_min_lat = 91;
				plate.d_max_lon = -181;
				plate.d_min_lon = 181;

				// re-set polar value to default : 
				// 0 = no pole contained in polygon
				plate.d_pole = 0;

				// loop over rotated vertices
				PolygonOnSphere::vertex_const_iterator iter;
				PolygonOnSphere::vertex_const_iterator next;

				for ( iter = plate.d_polygon->vertex_begin();
					iter != plate.d_polygon->vertex_end();
					++iter )
				{
					// get coords for iter vertex
					LatLonPoint v1 = make_lat_lon_point(*iter);

					// coords for next vertex in list
					next = iter;
					++next;
					if ( next == plate.d_polygon->vertex_end() )
					{
						next = plate.d_polygon->vertex_begin();
					}

					// coords for next vextex
					LatLonPoint v2 = make_lat_lon_point(*next);

					double v1lat = v1.latitude();
					double v1lon = v1.longitude();

					// double v2lat = v2.latitude(); // not used
					double v2lon = v2.longitude();

					plate.d_min_lon = (v1lon < plate.d_min_lon) ? v1lon : plate.d_min_lon;
					plate.d_max_lon = (v1lon > plate.d_max_lon) ? v1lon : plate.d_max_lon;

					plate.d_min_lat = (v1lat < plate.d_min_lat) ? v1lat : plate.d_min_lat;
					plate.d_max_lat = (v1lat > plate.d_max_lat) ? v1lat : plate.d_max_lat;

					dlon = v1lon - v2lon;

					if (fabs (dlon) > 180.0) 
					{
						dlon = copysign (360.0 - fabs (dlon), -dlon);
					}

					lon_sum += dlon;

				}

			#ifdef DEBUG_BOUNDS
			std::cout << "PointInPolygon::compute_bounds: max_lat = " << plate.d_max_lat << std::endl;
			std::cout << "PointInPolygon::compute_bounds: min_lat = " << plate.d_min_lat << std::endl;
			std::cout << "PointInPolygon::compute_bounds: max_lon = " << plate.d_max_lon << std::endl;
			std::cout << "PointInPolygon::compute_bounds: min_lon = " << plate.d_min_lon << std::endl;
			std::cout << "PointInPolygon::compute_bounds: lon_sum = " << lon_sum << std::endl;
			#endif

				//
				// dermine if the platepolygon contains the pole
				//
				if ( fabs(fabs(lon_sum) - 360.0) < 1.0e-8 )
				{
					if ( fabs(plate.d_max_lat) > fabs(plate.d_min_lat) ) 
					{
						plate.d_pole = static_cast<int>(copysign(1.0, plate.d_max_lat));
					}
					else 
					{
						plate.d_pole = static_cast<int>(copysign(1.0, plate.d_min_lat));
					}
				}
			#ifdef DEBUG_BOUNDS
			std::cout << "PointInPolygon::compute_bounds: plate.d_pole = " << plate.d_pole << std::endl;
			#endif
			}


			int 
			is_point_in_on_out_counter ( 
				const PointOnSphere &test_point,
				const PlatePolygon &plate,
				int &count_north,
				int &count_south)
			{
			#ifdef DEBUG_POINT_IN_POLYGON
			std::cout << "PointInPolygon::is_point_in_on_out_counter: " << std::endl;
			#endif

				// local tmp vars
				// using Real lets us use operator== 
				Real W, E, S, N, x_lat;
				Real dlon;
				Real lon, lon1, lon2;

				// coords of test test_point p
				LatLonPoint p = make_lat_lon_point(test_point);

				// test_point's coords are plon, plat
				double plon = p.longitude();
				double plat = p.latitude();

				// re-set number of crossings
				count_south = 0;
				count_north = 0;

				// Compute meridian through P and count all the crossings with 
				// segments of polygon boundary 

				// loop over vertices
				// form segments for each vertex pair
				PolygonOnSphere::vertex_const_iterator iter;
				PolygonOnSphere::vertex_const_iterator next;

				// loop over rotated vertices using access iterators
				for ( iter = plate.d_polygon->vertex_begin();
					iter != plate.d_polygon->vertex_end();
					++iter )
				{
					// get coords for iter vertex
					LatLonPoint v1 = make_lat_lon_point(*iter);

					// identifiy next vertex 
					next = iter; 
					++next;
					if ( next == plate.d_polygon->vertex_end() )
					{
						next = plate.d_polygon->vertex_begin();
					}

					// coords for next vextex
					LatLonPoint v2 = make_lat_lon_point(*next);

					double v1lat = v1.latitude();
					double v1lon = v1.longitude();

					double v2lat = v2.latitude();
					double v2lon = v2.longitude();

			#ifdef DEBUG_POINT_IN_POLYGON
			std::cout << "is_point_in_on_out_counter: v1 = " << v1lat << ", " << v1lon << std::endl;
			std::cout << "is_point_in_on_out_counter: v2 = " << v2lat << ", " << v2lon << std::endl; 
			#endif
					// Copy the two vertex longitudes 
					// since we need to mess with them
					lon1 = v1lon;
					lon2 = v2lon;

					// delta in lon
					dlon = lon2 - lon1;

					if ( dlon > 180.0 )
					{
						// Jumped across Greenwhich going westward
						lon2 -= 360.0;
					} 
					else if ( dlon < -180.0 )
					{
						// Jumped across Greenwhich going eastward
						lon1 -= 360.0;
					}

					// set lon limits for this segment
					if (lon1 <= lon2) 
					{	
						/* segment goes W to E (or N-S) */
						W = lon1;
						E = lon2;
					}
					else 
					{			
						/* segment goes E to W */
						W = lon2;
						E = lon1;
					}
					
					/* Local copy of plon, adjusted given the segment lon range */
					lon = plon;	

					/* Make sure we rewind way west for starters */
					while (lon > W) lon -= 360.0;	

					/* Then make sure we wind to inside the lon range or way east */
					while (lon < W) lon += 360.0;	

					/* Not crossing this segment */
					if (lon > E) continue; // to next vertex

					/* Special case of N-S segment: does P lie on it? */
					if (dlon == 0.0) 
					{	
						if ( v2lat < v1lat ) 
						{	
							/* Get N and S limits for segment */
							S = v2lat;
							N = v1lat;
						}
						else 
						{
							N = v2lat;
							S = v1lat;
						}

						/* P is not on this segment */
						if ( plat < S || plat > N ) continue; // to next vertex

						/* P is on segment boundary; we are done */
						return (1);	
					}

					/* Calculate latitude at intersection */
					x_lat = v1lat + (( v2lat - v1lat) / (lon2 - lon1)) * (lon - lon1);

					/* P is on S boundary */
					if ( x_lat == plat ) return (1);	

					// Only allow cutting a vertex at end of a segment 
					// to avoid duplicates 
					if (lon == lon1) continue;	

					if (x_lat > plat)	
					{
						/* Cut is north of P */
						++count_north;
					}
					else
					{
						/* Cut is south of P */
						++count_south;
					}

				} // end of loop over vertices

				return (0);	
			}


			/* This function is used to see if some point p is located 
			 * inside, outside, or on the boundary of the plate polygon
			 * Returns the following values:
			 *	POINT_OUTSIDE_POLYGON:	p is outside of S
			 *	POINT_INSIDE_POLYGON:	p is inside of S
			 *	POINT_ON_POLYGON:	p is on boundary of S
			 */
			Result 
			is_point_in_on_out ( 
				const PointOnSphere &test_point,
				const PlatePolygon &plate)
			{
				/* Algorithm:
				 *
				 * Case 1: The polygon S contains a geographical pole
				 *	   a) if P is beyond the far latitude then P is outside
				 *	   b) Compute meridian through P and count intersections:
				 *		odd: P is outside; even: P is inside
				 *
				 * Case 2: S does not contain a pole
				 *	   a) If P is outside range of latitudes then P is outside
				 *	   c) Compute meridian through P and count intersections:
				 *		odd: P is inside; even: P is outside
				 *
				 * In all cases, we check if P is on the outline of S
				 *
				 */
				
				// counters for the number of crossings of a meridian 
				// through p and and the segments of this polygon
				int count_north = 0;
				int count_south = 0;


				// coords of test point p
				LatLonPoint p = make_lat_lon_point(test_point);

				// test point's plat
				double plat = p.latitude();
					
			#ifdef DEBUG_POINT_IN_POLYGON
			std::cout << "PointInPolygon::is_point_in_on_out: " << std::endl;
			std::cout << "llp" << p << std::endl;
			std::cout << "pos" << test_point << std::endl;
			#endif

				if (plate.d_pole) 
				{	
			#ifdef DEBUG_POINT_IN_POLYGON
			std::cout << "PointInPolygon::is_point_in_on_out: plate contains N or S pole." << std::endl;
			#endif
					/* Case 1 of an enclosed polar cap */

					/* N polar cap */
					if (plate.d_pole == 1) 
					{	
						/* South of a N polar cap */
						if (plat < plate.d_min_lat) return (POINT_OUTSIDE_POLYGON);	

						/* Clearly inside of a N polar cap */
						if (plat > plate.d_max_lat) return (POINT_INSIDE_POLYGON);	
					}

					/* S polar cap */
					if (plate.d_pole == -1) 
					{	
						/* North of a S polar cap */
						if (plat > plate.d_max_lat) return (POINT_OUTSIDE_POLYGON);

						/* North of a S polar cap */
						if (plat < plate.d_min_lat) return (POINT_INSIDE_POLYGON);	
					}
				
					// Tally up number of intersections between polygon 
					// and meridian through p 
					
					if ( is_point_in_on_out_counter(test_point, plate, count_north, count_south) ) 
					{
						/* Found P is on S */
						return (POINT_ON_POLYGON);	
					}
				
					if (plate.d_pole == 1 && count_north % 2 == 0) 
					{
						return (POINT_INSIDE_POLYGON);
					}

					if (plate.d_pole == -1 && count_south % 2 == 0) 
					{
						return (POINT_INSIDE_POLYGON);
					}
				
					return (POINT_OUTSIDE_POLYGON);
				}
				
				/* Here is Case 2.  */

				// First check latitude range 
				if (plat < plate.d_min_lat || plat > plate.d_max_lat) 
				{
					return (POINT_OUTSIDE_POLYGON);
				}
				
				// Longitudes are tricker and are tested with the 
				// tallying of intersections 
				
				if ( is_point_in_on_out_counter( test_point, plate, count_north, count_south ) ) 
				{
					/* Found P is on S */
					return (POINT_ON_POLYGON);	
				}

				if (count_north % 2) return (POINT_INSIDE_POLYGON);
				
				/* Nothing triggered the tests; we are outside */
				return (POINT_OUTSIDE_POLYGON);	
			}
		}
	}
}


GPlatesMaths::PointInPolygon::optimised_polygon_type
GPlatesMaths::PointInPolygon::create_optimised_polygon(
		const PolygonOnSphere::non_null_ptr_to_const_type &polygon)
{
	PlatePolygon optimised_polygon(polygon);

	// compute polygon lat/lon bounds
	compute_bounds( optimised_polygon );

	return boost::any(optimised_polygon);
}


GPlatesMaths::PointInPolygon::Result
GPlatesMaths::PointInPolygon::test_point_in_polygon(
		const PointOnSphere &point,
		const optimised_polygon_type &opaque_optimised_polygon)
{
	// Convert from opaque type.
	const PlatePolygon* optimised_polygon =
			boost::any_cast<const PlatePolygon>(&opaque_optimised_polygon);

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			optimised_polygon != NULL,
			GPLATES_ASSERTION_SOURCE);

	// Apply the point in polygon test to the point: 
	return is_point_in_on_out(point, *optimised_polygon);
}


GPlatesMaths::PointInPolygon::Result
GPlatesMaths::PointInPolygon::test_point_in_polygon(
		const PointOnSphere &point,
		const PolygonOnSphere::non_null_ptr_to_const_type &polygon)
{
	PlatePolygon optimised_polygon(polygon);

	// compute polygon lat/lon bounds
	compute_bounds( optimised_polygon );

	// Apply the point in polygon test to the point: 
	return is_point_in_on_out(point, optimised_polygon);
}
