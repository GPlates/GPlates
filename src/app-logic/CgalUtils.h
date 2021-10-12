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

#ifndef GPLATES_APP_LOGIC_CGALUTILS_H
#define GPLATES_APP_LOGIC_CGALUTILS_H

#include <map>
#include <utility>
#include <vector>
#include <boost/optional.hpp>

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
//#include <CGAL/Triangulation_2.h>
#include <CGAL/Delaunay_triangulation_2.h>
// #include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Interpolation_traits_2.h>
#include <CGAL/natural_neighbor_coordinates_2.h>
#include <CGAL/interpolation_functions.h>

#include "maths/LatLonPoint.h"
#include "maths/PointOnSphere.h"


namespace GPlatesAppLogic
{
	namespace CgalUtils
	{
		//
		// Set up some CGAL typedefs.
		//

		typedef CGAL::Exact_predicates_inexact_constructions_kernel cgal_kernel_type;

		typedef CGAL::Delaunay_triangulation_2<cgal_kernel_type> cgal_delaunay_triangulation_type;

		typedef cgal_delaunay_triangulation_type::Finite_faces_iterator cgal_finite_faces_iterator;

		//! Typedef for a point that our CGAL algorithms can use internally.
		typedef cgal_kernel_type::Point_2 cgal_point_2_type;

		typedef cgal_delaunay_triangulation_type::Point cgal_point_type;

		typedef cgal_kernel_type::FT cgal_coord_type;

		typedef CGAL::Interpolation_traits_2<cgal_kernel_type> cgal_traits_type;

		typedef std::vector< std::pair<cgal_point_2_type, cgal_coord_type> >
				cgal_point_coordinate_vector_type;

		typedef std::map<cgal_point_2_type, cgal_coord_type, cgal_kernel_type::Less_xy_2 >
				cgal_map_point_to_value_type;

		typedef CGAL::Data_access<cgal_map_point_to_value_type> cgal_value_access_type;


		//
		// Set up some extra typedefs.
		//

		//! Typedef for result of an interpolation triangulation query.
		typedef std::pair<cgal_point_coordinate_vector_type, cgal_coord_type>
				interpolate_triangulation_query_type;


		/**
		 * Convert a @a PointOnSphere to a point type that we use in our CGAL algorithms.
		 *
		 * This is a separate function because the conversion could be expensive and can
		 * be done once and reused if possible.
		 */
		cgal_point_2_type
		convert_point_to_cgal(
				const GPlatesMaths::PointOnSphere &point);


		/**
		 * Convert a cgal point to a @a PointOnSphere.
		 */
		GPlatesMaths::PointOnSphere
		convert_point_from_cgal(
				const cgal_point_2_type &point);


		/**
		 * Inserts a range of @a PointOnSphere points into a delaunay triangulation.
		 */
		template <typename PointOnSphereForwardIterator>
		void
		insert_points_into_delaunay_triangulation(
				cgal_delaunay_triangulation_type &delaunay_triangulation,
				PointOnSphereForwardIterator points_begin,
				PointOnSphereForwardIterator points_end);


		/**
		 * Tests if @a point is in @a triangulation and returns query that
		 * can be used to interpolate values mapped to the triangulation points
		 * using @a interpolate_triangulation.
		 *
		 * Returns false if point is not in the triangulation.
		 */
		boost::optional<interpolate_triangulation_query_type>
		query_interpolate_triangulation(
				const cgal_point_2_type &point,
				const cgal_delaunay_triangulation_type &triangulation);


		/**
		 * Interpolates the values in @a map_point_to_value to the point that
		 * was used in the query @a query_interpolate_triangulation.
		 *
		 * Each point in the triangulation used in @a query_interpolate_triangulation
		 * should have a mapped value stored in @a map_point_to_value.
		 */
		double
		interpolate_triangulation(
				const interpolate_triangulation_query_type &point_in_triangulation_query,
				const cgal_map_point_to_value_type &map_point_to_value);
	}


	//
	// Implementation
	//
	namespace CgalUtils
	{
		template <typename PointOnSphereForwardIterator>
		void
		insert_points_into_delaunay_triangulation(
				cgal_delaunay_triangulation_type &delaunay_triangulation,
				PointOnSphereForwardIterator points_begin,
				PointOnSphereForwardIterator points_end)
		{
			std::vector<cgal_point_type> cgal_points;

			// Loop over the points and convert them.
			for (PointOnSphereForwardIterator points_iter = points_begin;
				points_iter != points_end;
				++points_iter)
			{
				cgal_points.push_back(
						convert_point_to_cgal(*points_iter));
			}

			// Build the Triangulation.
			delaunay_triangulation.insert(cgal_points.begin(), cgal_points.end());
		}
	}
}

#endif // GPLATES_APP_LOGIC_CGALUTILS_H
