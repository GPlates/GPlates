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

#ifndef _GPLATES_MATHS_SPHERIALDISTANCE_H_
#define _GPLATES_MATHS_SPHERIALDISTANCE_H_

#include <boost/optional.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/utility/enable_if.hpp>

#include "types.h"
#include "MathsUtils.h"
#include "SphereSettings.h"

namespace GPlatesMaths
{
	class GreatCircleArc;
	class PointOnSphere;
	class PolylineOnSphere;
	class PolygonOnSphere;
	class MultiPointOnSphere;
	
	namespace Spherical
	{
		/*
		* Check if the two arcs intersect.
		*/
		bool 
		do_great_circle_arcs_intersect(
				const GreatCircleArc  &arc_1,
				const GreatCircleArc  &arc_2,
				PointOnSphere *intersection_point = NULL);
		

		/*
		* Return the dot-product distance between a point and a arc.
		*/
		real_t 
		min_dot_product_distance_between_great_circle_arc_and_point(
				const GreatCircleArc &arc,
				const PointOnSphere  &point,
				PointOnSphere *closest_point = NULL);


		/*
		* Return the dot-product distance between two arcs.
		* 
		*/
		real_t
		min_dot_product_distance_between_great_circle_arcs(
				const GreatCircleArc &arc1,
				const GreatCircleArc &arc2,
				PointOnSphere *closest_point_1 = NULL,
				PointOnSphere *closest_point_2 = NULL);


		/*
		* @a dot_product_distance calculate the dot product distance between two geometries.
		* The function will quite early if a interim distance, which is closer than 
		* @ainterest_region, has been found, which in turn will cause the return value
		* not reliable. In this case, Caller needs to compare the return value with @ainterest_region. 
		* The value of @ainterest_region should be in the range [-1,1], which denotes cos(angle).
		* Minus one(-1) means largest and plus one(+1) means zero distance. 
		*/
		namespace details
		{
			/*
			* TODO:
			*/
			real_t 
			min_dot_product_distance(
					const PointOnSphere  &point_1,
					const PointOnSphere  &point_2,
					const real_t threshold = MaxDotProductDistanceOnSphere);

			/*
			* Calculate the dot product distance between a point and polyline
			* The function will quite early if a interim distance, which is closer than 
			* @ainterest_region, has been found, which in turn will cause the return value
			* not reliable. In this case, Caller needs to compare the return value with @ainterest_region. 
			* The value of @ainterest_region should be in the range [-1,1], which denotes cos(angle).
			* Minus one means largest and plus one means zero distance. 
			*/
			real_t
			min_dot_product_distance(
					const PointOnSphere		&point,
					const PolylineOnSphere  &polyline,
					const real_t threshold = MaxDotProductDistanceOnSphere);

			/*
			* TODO:
			*/
			real_t 
			min_dot_product_distance(
					const PointOnSphere		&point,
					const PolygonOnSphere   &polygon,
					const real_t threshold = MaxDotProductDistanceOnSphere,
					const bool distance_to_boundary = false);

			/*
			* TODO:
			*/
			real_t 
			min_dot_product_distance(
					const PointOnSphere			&point,
					const MultiPointOnSphere	&multipoint,
					const real_t threshold = MaxDotProductDistanceOnSphere);

			
			/*
			* TODO:
			*/
			inline
			real_t 
			min_dot_product_distance(
					const PolylineOnSphere	&polyline,
					const PointOnSphere		&point,
					const real_t threshold = MaxDotProductDistanceOnSphere)
			{
				return min_dot_product_distance(
						point,
						polyline,
						threshold);
			}


			/*
			* TODO:
			*/
			real_t 
			min_dot_product_distance(
					const PolylineOnSphere	&polyline_1,
					const PolylineOnSphere	&polyline_2,
					const real_t threshold = MaxDotProductDistanceOnSphere);

			/*
			* TODO:
			*/
			real_t 
			min_dot_product_distance(
					const PolylineOnSphere	&polyline,
					const PolygonOnSphere	&polygon,
					const real_t threshold = MaxDotProductDistanceOnSphere,
					const bool distance_to_boundary = false);
			
			/*
			* TODO:
			*/
			real_t 
			min_dot_product_distance(
					const PolylineOnSphere		&polyline,
					const MultiPointOnSphere	&multipoint,
					const real_t threshold = MaxDotProductDistanceOnSphere);

			/*
			* TODO:
			*/
			inline
			real_t 
			min_dot_product_distance(
					const PolygonOnSphere	&polygon,
					const PointOnSphere		&point,
					const real_t threshold = MaxDotProductDistanceOnSphere,
					const bool distance_to_boundary = false)
			{
				return min_dot_product_distance(
						point,
						polygon,
						threshold,
						distance_to_boundary);
			}
			
			/*
			* TODO:
			*/
			inline
			real_t 
			min_dot_product_distance(
					const PolygonOnSphere	&polygon,
					const PolylineOnSphere	&polyline,
					const real_t threshold = MaxDotProductDistanceOnSphere,
					const bool distance_to_boundary = false)
			{
				return min_dot_product_distance(
						polyline,
						polygon,
						threshold,
						distance_to_boundary);
			}
			
			
			/*
			* TODO:
			*/
			real_t 
			min_dot_product_distance(
					const PolygonOnSphere	&polygon_1,
					const PolygonOnSphere	&polygon_2,
					const real_t threshold = MaxDotProductDistanceOnSphere,
					const bool distance_to_boundary = false);

			/*
			* TODO:
			*/
			real_t 
			min_dot_product_distance(
					const PolygonOnSphere		&polygon,
					const MultiPointOnSphere	&multipoint,
					const real_t threshold = MaxDotProductDistanceOnSphere,
					const bool distance_to_boundary = false);


			/*
			* TODO:
			*/
			inline
			real_t 
			min_dot_product_distance(
					const MultiPointOnSphere	&multipoint,
					const PointOnSphere			&point,
					const real_t threshold = MaxDotProductDistanceOnSphere)
			{
				return min_dot_product_distance(
						point,
						multipoint,
						threshold);
			}

			/*
			* TODO:
			*/
			inline
			real_t 
			min_dot_product_distance(
					const MultiPointOnSphere	&multipoint,
					const PolylineOnSphere		&polyline,
					const real_t threshold = MaxDotProductDistanceOnSphere)
			{
				return min_dot_product_distance(
						polyline,
						multipoint,
						threshold);
			}

			/*
			* TODO:
			*/
			inline
			real_t 
			min_dot_product_distance(
					const MultiPointOnSphere	&multipoint,
					const PolygonOnSphere		&polygon,
					const real_t threshold = MaxDotProductDistanceOnSphere,
					const bool distance_to_boundary = false)
			{
				return min_dot_product_distance(
						polygon,
						multipoint,
						threshold,
						distance_to_boundary);
			}


			/*
			* TODO:
			*/
			real_t 
			min_dot_product_distance(
					const MultiPointOnSphere	&multipoint_1,
					const MultiPointOnSphere	&multipoint_2,
					const real_t threshold = MaxDotProductDistanceOnSphere);

		}// end of namespace details

		typedef boost::mpl::vector
			<
				PointOnSphere,
				PolylineOnSphere,
				PolygonOnSphere,
				MultiPointOnSphere
			> geometry_types;
		
		/*
		* TODO:
		*/
		template< class Geometry_1, class Geometry_2>
		inline
		typename boost::enable_if<
				typename boost::mpl::and_<
						typename boost::mpl::contains<geometry_types, Geometry_1>,
						typename boost::mpl::contains<geometry_types, Geometry_2> >, real_t>::type
		min_dot_product_distance(
				const Geometry_1 &g1,
				const Geometry_2 &g2)
		{
			return details::min_dot_product_distance(g1,g2);
		}


		/*
		* TODO:
		*/
		template< class Geometry_1, class Geometry_2>
		inline
		typename boost::disable_if<
				typename boost::mpl::and_<
						typename boost::mpl::contains<geometry_types, Geometry_1>,
						typename boost::mpl::contains<geometry_types, Geometry_2> > >::type
		min_dot_product_distance(
				const Geometry_1 &g1,
				const Geometry_2 &g2)
		{
			 BOOST_MPL_ASSERT_MSG(
					false,
					Geometry_Type_Not_Support,
					(const Geometry_1&, const Geometry_2&));
		}


		/*
		* TODO:
		*/
		template< class Geometry_1, class Geometry_2>
		inline
		typename boost::enable_if<
				typename boost::mpl::and_<
						typename boost::mpl::contains<geometry_types, Geometry_1>,
						typename boost::mpl::contains<geometry_types, Geometry_2> >, bool>::type
		is_min_dot_product_distance_below_threshold(
				const Geometry_1 &g1,
				const Geometry_2 &g2,
				const real_t &threshold)
		{
			return details::min_dot_product_distance(g1,g2,threshold) > threshold;
		}


		/*
		* TODO:
		*/
		template< class Geometry_1, class Geometry_2>
		inline
		typename boost::disable_if<
				typename boost::mpl::and_<
						typename boost::mpl::contains<geometry_types, Geometry_1>,
						typename boost::mpl::contains<geometry_types, Geometry_2> > >::type
		is_min_dot_product_distance_below_threshold(
				const Geometry_1 &g1,
				const Geometry_2 &g2,
				const real_t &threshold)
		{
			BOOST_MPL_ASSERT_MSG(
					false,
					Geometry_Type_Not_Support,
					(const Geometry_1&, const Geometry_2&));
		}


		/*
		* TODO:
		*/
		template< class Geometry_1, class Geometry_2>
		inline
		typename boost::enable_if<
				typename boost::mpl::and_<
						typename boost::mpl::contains<geometry_types, Geometry_1>,
						typename boost::mpl::contains<geometry_types, Geometry_2> >, boost::optional<real_t> >::type
		min_dot_product_distance_below_threshold(
				const Geometry_1 &g1,
				const Geometry_2 &g2,
				const real_t &threshold)
		{
			boost::optional<real_t> temp_min_dot_product_distance = details::min_dot_product_distance(g1,g2);
			if(*temp_min_dot_product_distance < threshold) //if the distance greater than threshold.
			{
				return boost::none;
			}
			return temp_min_dot_product_distance;
		}


		/*
		* TODO:
		*/
		template< class Geometry_1, class Geometry_2>
		inline
		typename boost::disable_if<
				typename boost::mpl::and_<
						typename boost::mpl::contains<geometry_types, Geometry_1>,
						typename boost::mpl::contains<geometry_types, Geometry_2> > >::type
		min_dot_product_distance_below_threshold(
				const Geometry_1 &g1,
				const Geometry_2 &g2,
				const real_t &threshold)
		{
			BOOST_MPL_ASSERT_MSG(
					false,
					Geometry_Type_Not_Support,
					(const Geometry_1&, const Geometry_2&));
		}

	}//end of namespace Spherical
}

#endif  // _GPLATES_MATHS_SPHERIALDISTANCE_H_
