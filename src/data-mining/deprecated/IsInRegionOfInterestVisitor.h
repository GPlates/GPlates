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
#ifndef GPLATESDATAMINING_ISINREGIONOFINTERESTVISITOR_H
#define GPLATESDATAMINING_ISINREGIONOFINTERESTVISITOR_H

#include <vector>
#include <bitset>

#include <QDebug>

#include <boost/variant.hpp>
#include <boost/variant/get.hpp>

#include "canvas-tools/MeasureDistanceState.h"

#include "feature-visitors/GeometryFinder.h"

#include "maths/ProximityHitDetail.h"
#include "maths/ProximityCriteria.h"
#include "maths/ConstGeometryOnSphereVisitor.h"
#include "maths/MathsUtils.h"
#include "maths/PointInPolygon.h"

#include "model/FeatureHandle.h"

namespace GPlatesDataMining
{
	using namespace GPlatesMaths;

	static const double RADIUS_OF_EARTH = 6378.1; 

	typedef  MultiPointOnSphere::non_null_ptr_to_const_type MultiPointPtr;
	typedef  PointOnSphere::non_null_ptr_to_const_type PointPtr;
	typedef  PolygonOnSphere::non_null_ptr_to_const_type PolygonPtr;
	typedef  PolylineOnSphere::non_null_ptr_to_const_type PolylinePtr;

	/*
	* TODO: more comments
	* Until function. Users are suppose to use this function.
	*/
	bool
	is_close_enough(
			const GeometryOnSphere& g1, 
			const GeometryOnSphere& g2,
			const double range);
	
	//BELOW FUNCTIONS ARE NOT INTENDED TO BE USED OUTSIDE
	namespace
	{
		/*
		* Overload all possible composition
		* Tried template, quite tricky.
		* To make it maintainable, choose easy and clumsy way.
		*/
		bool
		is_close_enough_internal(
				PointPtr point1,
				PointPtr point2,
				double range);

		bool
		is_close_enough_internal(
				PointPtr point,
				PolylinePtr polyline,
				double range);

		bool
		is_close_enough_internal(
				PointPtr point,
				PolygonPtr polygon,
				double range);

		bool
		is_close_enough_internal(
				PointPtr point,
				MultiPointPtr multi_point,
				double range);

		inline
		bool
		is_close_enough_internal(
				PolylinePtr polyline,
				PointPtr point,
				double range)
		{
			return is_close_enough_internal(point, polyline, range);
		}

		bool
		is_close_enough_internal(
				PolylinePtr polyline1,
				PolylinePtr polyline2,
				double range);

		bool
		is_close_enough_internal(
				PolylinePtr polyline,
				PolygonPtr polygon,
				double range);

		bool
		is_close_enough_internal(
				PolylinePtr polyline,
				MultiPointPtr multi_point,
				double range);

		inline
		bool
		is_close_enough_internal(
				PolygonPtr polygon,
				PointPtr point,
				double range)
		{
			return is_close_enough_internal(point, polygon, range);
		}

		inline
		bool
		is_close_enough_internal(
				PolygonPtr polygon,
				PolylinePtr polyline,
				double range)
		{
			return is_close_enough_internal(polyline, polygon, range);
		}

		bool
		is_close_enough_internal(
				PolygonPtr polygon1,
				PolygonPtr polygon2,
				double range);

		bool
		is_close_enough_internal(
				PolygonPtr polygon,
				MultiPointPtr multi_point,
				double range);

		inline
		bool
		is_close_enough_internal(
				MultiPointPtr multi_point,
				PointPtr point,
				double range)
		{
			return is_close_enough_internal(point, multi_point, range);
		}

		inline
		bool
		is_close_enough_internal(
				MultiPointPtr multi_point,
				PolylinePtr polyline,
				double range)
		{
			return is_close_enough_internal(polyline, multi_point, range);
		}

		inline
		bool
		is_close_enough_internal(
				MultiPointPtr multi_point,
				PolygonPtr polygon,
				double range)
		{
			return is_close_enough_internal(polygon, multi_point, range);
		}

		bool
		is_close_enough_internal(
				MultiPointPtr multi_point1,
				MultiPointPtr multi_point2,
				double range);

		/*
		* Point to point
		*/
		bool
		is_close_enough_internal(
				PointPtr point1,
				PointPtr point2,
				double range)
		{

			real_t closeness = acos(calculate_closeness(*point1, *point2)) * RADIUS_OF_EARTH;

			#ifdef _DEBUG
			std::cout << "determining if a point is in the region of interest of a point."
				<< std::endl;
			std::cout << "The closeness for the point-point is: " << closeness <<std::endl;
			#endif
			
			if(closeness.is_precisely_greater_than(range))
			{
				return false;
			}
			else
			{
				return true;
			}
		}

		/*
		* Point to polygon
		*/
		bool
		is_close_enough_internal(
				PointPtr point,
				PolygonPtr polygon,
				double range)
		{
		#if 0
			using namespace GPlatesMaths::PointInPolygon;
			if(POINT_OUTSIDE_POLYGON != test_point_in_polygon(*point.get(), polygon))
			{
				return true;
			}
		#endif
			return true;
		}

		/*
		* Point to polyline
		*/
		bool
		is_close_enough_internal(
				PointPtr point,
				PolylinePtr polyline,
				double range)
		{
			real_t closeness;
			double thresh_hold = 0;
			
			if(range > (RADIUS_OF_EARTH * PI) )
			{
				thresh_hold = -1;
			}
			else if(range <= 0)
			{
				thresh_hold = 1;
			}
			else
			{
				thresh_hold = std::cos(range / RADIUS_OF_EARTH);
			}

			ProximityCriteria proximity_criteria(
					*point,
					thresh_hold);

			#ifdef _DEBUG
			std::cout << "thresh_hold is: " << thresh_hold <<std::endl;
			#endif
			
			ProximityHitDetail::maybe_null_ptr_type hit = 
				polyline->test_proximity(
						proximity_criteria);

			if(hit)
			{
				return true;
			}
			else
			{
				return false;
			}
		}

		/*
		* Point to multi points
		*/
		bool
		is_close_enough_internal(
				PointPtr point,
				MultiPointPtr multi_point,
				double range)
		{
			//TODO...
			return true;
		}

		bool
		is_close_enough_internal(
				PolylinePtr polyline1,
				PolylinePtr polyline2,
				double range)
		{
			//TODO...
			return true;
		}
		bool
		is_close_enough_internal(
				PolylinePtr polyline,
				PolygonPtr polygon,
				double range)
		{
			//TODO...
			return true;
		}

		bool
		is_close_enough_internal(
				PolylinePtr polyline,
				MultiPointPtr multi_point,
				double range)
		{
			//TODO...
			return true;
		}
		bool
		is_close_enough_internal(
				PolygonPtr polygon1,
				PolygonPtr polygon2,
				double range)
		{
			//TODO...
			return true;
		}
		bool
		is_close_enough_internal(
				PolygonPtr polygon,
				MultiPointPtr multi_point,
				double range)
		{
			//TODO...
			return true;
		}
		bool
		is_close_enough_internal(
				MultiPointPtr multi_point1,
				MultiPointPtr multi_point2,
				double range)
		{
			//TODO...
			return true;
		}

	
		class IsInReigonOfInterestDispatchVisitor :
			public GPlatesMaths::ConstGeometryOnSphereVisitor
		{
		public:

			template < class GeometryType >
			friend class IsInReigonOfInterestCheckerVisitor;
		
			IsInReigonOfInterestDispatchVisitor(
					GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type base_geometry,
					double range) :
				d_base_geometry(base_geometry),
				d_ROI_range(range),
				d_distance(0),
				d_is_in_reigon_of_interest(false)
			{}
			 
			virtual
			inline
			void
			visit_multi_point_on_sphere(
					MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
			{ 
				dispatch(multi_point_on_sphere);
			}

			virtual
			inline
			void
			visit_point_on_sphere(
					PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
			{ 
				dispatch(point_on_sphere);
			}

			virtual
			inline
			void
			visit_polygon_on_sphere(
					PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
			{ 
				dispatch(polygon_on_sphere);
			}

			virtual
			inline
			void
			visit_polyline_on_sphere(
					PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
			{
				dispatch(polyline_on_sphere);
			}

			inline
			bool
			is_in_reigon_of_interest()
			{
				return d_is_in_reigon_of_interest;
			}

			inline
			double
			distance()
			{
				return d_distance;
			}

		protected:
			
			template< class GeometryType >
			void
			dispatch(
					GeometryType geometry );

			boost::optional< GeometryOnSphere::non_null_ptr_to_const_type > d_base_geometry;
			double d_ROI_range;
			double d_distance;
			bool   d_is_in_reigon_of_interest;
		private:
			IsInReigonOfInterestDispatchVisitor();
		};

		/*
		*
		*
		*/
		template < class GeometryType >
		class IsInReigonOfInterestCheckerVisitor :
			public GPlatesMaths::ConstGeometryOnSphereVisitor
		{
		public:
			IsInReigonOfInterestCheckerVisitor(
					GeometryType geometry,
					IsInReigonOfInterestDispatchVisitor* parent) : 
				d_candidate_geometry(geometry),
				d_parent_ptr(parent)
			{
			}

			virtual
			inline
			void
			visit_multi_point_on_sphere(
					MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
			{ 
				d_parent_ptr->d_is_in_reigon_of_interest =
					is_close_enough_internal(
							multi_point_on_sphere,
							*d_candidate_geometry,
							d_parent_ptr->d_ROI_range);
			}

			virtual
			inline
			void
			visit_point_on_sphere(
					PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
			{ 
				d_parent_ptr->d_is_in_reigon_of_interest =
					is_close_enough_internal(
							point_on_sphere,
							*d_candidate_geometry,
							d_parent_ptr->d_ROI_range);
			}

			virtual
			inline
			void
			visit_polygon_on_sphere(
					PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
			{ 
				d_parent_ptr->d_is_in_reigon_of_interest =
					is_close_enough_internal(
							polygon_on_sphere,
							*d_candidate_geometry,
							d_parent_ptr->d_ROI_range);
			}

			virtual
			inline
			void
			visit_polyline_on_sphere(
					PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
			{
				d_parent_ptr->d_is_in_reigon_of_interest =
					is_close_enough_internal(
							polyline_on_sphere,
							*d_candidate_geometry,
							d_parent_ptr->d_ROI_range);
			}

		private:
			boost::optional< GeometryType > d_candidate_geometry;
			IsInReigonOfInterestDispatchVisitor* d_parent_ptr;
		};


		template< class GeometryType > 
		void
		IsInReigonOfInterestDispatchVisitor::dispatch(
				GeometryType geometry)
		{
			IsInReigonOfInterestCheckerVisitor< GeometryType > 
				checker_visitor( geometry, this);

			(*d_base_geometry)->accept_visitor(
					checker_visitor);
		}
	}
}


#endif




