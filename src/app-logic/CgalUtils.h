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

#include <QDebug>

#include "global/CompilerWarnings.h"

PUSH_MSVC_WARNINGS
DISABLE_MSVC_WARNING( 4005 ) // For Boost 1.44 and Visual Studio 2010.
// #include <CGAL/Triangulation_2.h>
// #include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Origin.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/Delaunay_triangulation_3.h>
#include <CGAL/Triangulation_hierarchy_3.h>
#include <CGAL/Triangulation_conformer_2.h>
#include <CGAL/Delaunay_mesher_2.h>
#include <CGAL/Delaunay_mesh_face_base_2.h>
#include <CGAL/Delaunay_mesh_size_criteria_2.h>
#include <CGAL/Interpolation_traits_2.h>
#include <CGAL/natural_neighbor_coordinates_2.h>
#include <CGAL/surface_neighbor_coordinates_3.h>
#include <CGAL/interpolation_functions.h>
#include <CGAL/Interpolation_gradient_fitting_traits_2.h>
#include <CGAL/sibson_gradient_fitting.h>
POP_MSVC_WARNINGS

#include "maths/LatLonPoint.h"
#include "maths/PointOnSphere.h"
 
namespace GPlatesAppLogic
{
	namespace CgalUtils
	{
		// Basic CGAL typedefs.
		typedef CGAL::Exact_predicates_inexact_constructions_kernel cgal_kernel_type;
		typedef cgal_kernel_type::FT cgal_coord_type;

		// 2D Triangulation types
		typedef CGAL::Triangulation_vertex_base_2<cgal_kernel_type> cgal_triangulation_vertex_base_2_type;

		typedef CGAL::Delaunay_triangulation_2<cgal_kernel_type> cgal_delaunay_triangulation_2_type;
		typedef cgal_delaunay_triangulation_2_type::Finite_faces_iterator cgal_finite_faces_2_iterator;

		typedef CGAL::Delaunay_mesh_face_base_2<cgal_kernel_type> cgal_delaunay_mesh_face_base_2_type;
		typedef CGAL::Triangulation_data_structure_2<cgal_triangulation_vertex_base_2_type, cgal_delaunay_mesh_face_base_2_type> cgal_triangulation_data_structure_2_type;
		typedef CGAL::Delaunay_mesh_size_criteria_2<cgal_delaunay_triangulation_2_type> cgal_delaunay_mesh_size_criteria_2_type;
		typedef CGAL::Delaunay_mesher_2<cgal_delaunay_triangulation_2_type, cgal_delaunay_mesh_size_criteria_2_type> cgal_mesher_2_type;



		// 2D + Constraints 
		typedef CGAL::Constrained_Delaunay_triangulation_2<cgal_kernel_type, cgal_triangulation_data_structure_2_type, CGAL::Exact_predicates_tag> cgal_constrained_delaunay_triangulation_2_type;
//		typedef CGAL::Delaunay_mesh_size_criteria_2<cgal_constrained_delaunay_triangulation_2_type> cgal_delaunay_mesh_size_criteria_2_type;
//		typedef CGAL::Delaunay_mesher_2<cgal_constrained_delaunay_triangulation_2_type, cgal_delaunay_mesh_size_criteria_2_type> cgal_mesher_2_type;
		typedef cgal_constrained_delaunay_triangulation_2_type::Finite_faces_iterator cgal_constrained_finite_faces_iterator;
		typedef cgal_constrained_delaunay_triangulation_2_type::Vertex_handle cgal_constrained_vertex_handle;

		// 3D Triangulation types
		typedef CGAL::Triangulation_vertex_base_3<cgal_kernel_type> cgal_triangulation_vertex_base_3_type;
		typedef CGAL::Triangulation_hierarchy_vertex_base_3<cgal_triangulation_vertex_base_3_type> cgal_hierarchy_vertex_base_3_type;
		typedef CGAL::Triangulation_data_structure_3<cgal_hierarchy_vertex_base_3_type> cgal_triangulation_data_structure_3_type;
		typedef CGAL::Delaunay_triangulation_3<cgal_kernel_type, cgal_triangulation_data_structure_3_type> cgal_delaunay_triangulation_3_type;
		typedef CGAL::Triangulation_hierarchy_3<cgal_delaunay_triangulation_3_type> cgal_triangulation_hierarchy_3_type; // Dh in CGAL examples

		typedef cgal_triangulation_hierarchy_3_type::Finite_cells_iterator    cgal_th3_finite_cells_iterator;
		typedef cgal_triangulation_hierarchy_3_type::Finite_vertices_iterator cgal_th3_finite_vertices_iterator;
		typedef cgal_triangulation_hierarchy_3_type::Finite_facets_iterator   cgal_th3_finite_facets_iterator;
		typedef cgal_triangulation_hierarchy_3_type::Cell_handle              cgal_th3_cell_handle;
		typedef cgal_triangulation_hierarchy_3_type::Vertex_handle            cgal_th3_vertex_handle;
		typedef cgal_triangulation_hierarchy_3_type::Point                    cgal_th3_point;

		/* 
		typedef CGAL::Triangulation_vertex_base_3<cgal_kernel_type> cgal_triangulation_vertex_base_3_type;
		typedef CGAL::Triangulation_hierarchy_vertex_base_3<cgal_triangulation_vertex_base_3_type> cgal_triangulation_hierarchy_vertex_base_3_type;
		typedef CGAL::Triangulation_data_structure_3<cgal_triangulation_hierarchy_vertex_base_3_type> cgal_triangulation_vertex_base_3_type;
		*/

		//! Typedef for a point that our CGAL algorithms can use internally.
		typedef cgal_kernel_type::Point_2 cgal_point_2_type;
		typedef cgal_kernel_type::Vector_2 cgal_vector_2_type;

		typedef cgal_kernel_type::Point_3 cgal_point_3_type;
		typedef cgal_kernel_type::Vector_3 cgal_vector_3_type;


		typedef CGAL::Interpolation_traits_2<cgal_kernel_type> cgal_traits_type;

		typedef std::vector< std::pair<cgal_point_2_type, cgal_coord_type> >
				cgal_point_coordinate_vector_type;

		typedef std::vector< std::pair< cgal_point_3_type, cgal_coord_type> > 
				cgal_point_coordinate_vector_3_type;

		/* from   http://www.cgal.org/Manual/3.2/doc_html/cgal_manual/Interpolation/Chapter_main.html
		std::map<Point, Coord_type, K::Less_xy_2> function_values;
  		typedef CGAL::Data_access< std::map<Point, Coord_type, K::Less_xy_2 > > Value_access;
		*/

		//! Typedefs for interpolations in 2 D
		typedef std::map<cgal_point_2_type, cgal_coord_type, cgal_kernel_type::Less_xy_2 >
				cgal_map_point_2_to_value_type; // Point_value_map in examples

		typedef CGAL::Data_access<cgal_map_point_2_to_value_type> cgal_point_2_value_access_type;

		typedef std::map<cgal_point_2_type, cgal_vector_2_type, cgal_kernel_type::Less_xy_2 >
				cgal_map_point_2_to_vector_type; // Point_vector_map in examples

		typedef CGAL::Data_access<cgal_map_point_2_to_vector_type> cgal_point_2_vector_access_type;


		//! Typedefs for interpolations in 3 D
		typedef std::map<cgal_point_3_type, cgal_coord_type, cgal_kernel_type::Less_xy_3 >
				cgal_map_point_3_to_value_type; // Point_value_map in examples

		typedef CGAL::Data_access<cgal_map_point_3_to_value_type> cgal_point_3_value_access_type;

		typedef std::map<cgal_point_3_type, cgal_vector_3_type, cgal_kernel_type::Less_xy_3 >
				cgal_map_point_3_to_vector_type; // Point_vector_map in examples

		typedef CGAL::Data_access<cgal_map_point_3_to_vector_type> cgal_point_3_vector_access_type;


		//
		// Set up some extra typedefs.
		//

		//! Typedef for result of an interpolation triangulation query.
		typedef std::pair<cgal_point_coordinate_vector_type, cgal_coord_type>
				interpolate_triangulation_query_type;

		//! Typedef for result of an interpolation triangulation 3 query.
		typedef std::pair<cgal_point_coordinate_vector_3_type, cgal_coord_type>
				interpolate_triangulation_query_3_type;

		/**
		 * Convert a @a PointOnSphere to a point type that we use in our CGAL algorithms.
		 *
		 * This is a separate function because the conversion could be expensive and can
		 * be done once and reused if possible.
		 */
		cgal_point_2_type
		convert_point_to_cgal_2(
				const GPlatesMaths::PointOnSphere &point);

		cgal_point_3_type
		convert_point_to_cgal_3(
				const GPlatesMaths::PointOnSphere &point);

		/**
		 * Convert a cgal point to a @a PointOnSphere.
		 */
		GPlatesMaths::PointOnSphere
		convert_point_from_cgal_2(
				const cgal_point_2_type &point);

		//GPlatesMaths::PointOnSphere
		GPlatesMaths::PointOnSphere
		convert_point_from_cgal_3(
				const cgal_point_3_type &point);

		/**
		 * Inserts a range of @a PointOnSphere points into a delaunay triangulation.
		 */
		template <typename PointOnSphereForwardIterator>
		void
		insert_points_into_delaunay_triangulation_2(
				cgal_delaunay_triangulation_2_type &delaunay_triangulation_2,
				PointOnSphereForwardIterator points_begin,
				PointOnSphereForwardIterator points_end);

		/**
		 * Inserts a range of @a PointOnSphere points into a contrained delaunay triangulation.
		 */
		template <typename PointOnSphereForwardIterator>
		void
		insert_points_into_constrained_delaunay_triangulation_2(
				cgal_constrained_delaunay_triangulation_2_type &constrained_delaunay_triangulation_2,
				PointOnSphereForwardIterator points_begin,
				PointOnSphereForwardIterator points_end);

		/** 
		 * Inserts a range of @a PointOnSphere points (converted to XXX ) into a cgal_triangulation_hierarchy_3_type
		 */ 
		template <typename PointOnSphereForwardIterator>
		void
		insert_points_into_delaunay_triangulation_hierarchy_3(
				cgal_triangulation_hierarchy_3_type &cgal_triangulation_hierarchy,
				PointOnSphereForwardIterator points_begin,
				PointOnSphereForwardIterator points_end);

		// 
		// 2D 
		//

		/**
		 * Tests if @a point is in @a triangulation and returns a query object 
		 * that may be used to interpolate values mapped to the triangulation points
		 * using @a interpolate_triangulation.
		 *
		 * Returns false if point is not in the triangulation.
		 */
		boost::optional<interpolate_triangulation_query_type>
		query_interpolate_triangulation_2(
				const cgal_point_2_type &point,
				const cgal_delaunay_triangulation_2_type &triangulation);

		/** 
		 * FIXME!
		 */
		cgal_vector_2_type
		gradient_2(
				const cgal_point_2_type &test_point,
				const cgal_delaunay_triangulation_2_type &triangulation,
				const cgal_map_point_2_to_value_type &function_values); // a Point_value_map in examples

			//	cgal_map_point_2_to_vector_type &function_gradients); // Point_value_map in examples

		/**
		 * Interpolates the values in @a map_point_to_value to the point that
		 * was used in the query @a query_interpolate_triangulation_2.
		 *
		 * Each point in the triangulation used in @a query_interpolate_triangulation_2
		 * should have a mapped value stored in @a map_point_to_value.
		 */
		double
		interpolate_triangulation_2(
				const interpolate_triangulation_query_type &point_in_triangulation_query,
				const cgal_map_point_2_to_value_type &map_point_to_value); // Point_value_map in examples


		//
		// 3D 
		//

		/** 
		 * Tests if @a point is in @a triangulation and returns query object 
		 * that may be used to interpolate values mapped to the triangulation points
		 * using @a interpolate_triangulation.
		 *
		 * Returns false if point is not in the triangulation.
		 */
		boost::optional<interpolate_triangulation_query_3_type>
		query_interpolate_triangulation_3(
				const cgal_point_3_type &test_point,
				const cgal_triangulation_hierarchy_3_type &triangulation);

		void
		gradient_3(
				const cgal_point_3_type &test_point,
				const cgal_triangulation_hierarchy_3_type &triangulation,
				cgal_map_point_3_to_value_type &function_values,
				cgal_map_point_3_to_vector_type &function_gradients);

		/**
		 * Interpolates the values in @a map_point_to_value to the point that
		 * was used in the query @a query_interpolate_triangulation.
		 *
		 * Each point in the triangulation used in @a query_interpolate_triangulation
		 * should have a mapped value stored in @a map_point_to_value.
		 */
		double
		interpolate_triangulation_3(
				const interpolate_triangulation_query_3_type &point_in_triangulation_query,
				const cgal_map_point_3_to_value_type &map_point_3_to_value);

	}


	//
	// Implementation
	//
	namespace CgalUtils
	{
		template <typename PointOnSphereForwardIterator>
		void
		insert_points_into_delaunay_triangulation_2(
				cgal_delaunay_triangulation_2_type &delaunay_triangulation_2,
				PointOnSphereForwardIterator points_begin,
				PointOnSphereForwardIterator points_end)
		{
			std::vector<cgal_point_2_type> cgal_points;

			// Loop over the points and convert them.
			for (PointOnSphereForwardIterator points_iter = points_begin;
				points_iter != points_end;
				++points_iter)
			{
				cgal_points.push_back(
						convert_point_to_cgal_2(*points_iter));
			}

			// Build the Triangulation.
			delaunay_triangulation_2.insert(cgal_points.begin(), cgal_points.end());
		}

		template <typename PointOnSphereForwardIterator>
		void
		insert_points_into_constrained_delaunay_triangulation_2(
				cgal_constrained_delaunay_triangulation_2_type &constrained_delaunay_triangulation_2,
				PointOnSphereForwardIterator points_begin,
				PointOnSphereForwardIterator points_end)
		{

			// variables for the loops
			PointOnSphereForwardIterator pos_iter;
			cgal_constrained_vertex_handle v1, v2;
			std::vector<cgal_constrained_vertex_handle> vertex_handles;
			std::vector<cgal_constrained_vertex_handle>::iterator v1_iter;
			std::vector<cgal_constrained_vertex_handle>::iterator v2_iter;

			// Double check for identical start and end points 
			if ( *points_begin == *(points_end - 1) )
			{
//qDebug() << "begin == end";
				points_end = points_end - 1;
			}

			// Loop over the points, convert them, and add them to the triangulation
			for ( pos_iter = points_begin; pos_iter != points_end; ++pos_iter)
			{

				vertex_handles.push_back(
					constrained_delaunay_triangulation_2.insert( convert_point_to_cgal_2(*pos_iter) )
				);
			}

			// Do not add a constraint for a single point; it is a seed point 
			if ( points_begin == (points_end - 1) )
			{
//qDebug() << "single point";
				return;
			}

			// Loop over the vertex handles, insert constrained edges
			for ( v1_iter = vertex_handles.begin(); v1_iter != vertex_handles.end() ; ++v1_iter)
			{
				v2_iter = v1_iter + 1;

				// close the loop
				if (v2_iter == vertex_handles.end() ) { v2_iter = vertex_handles.begin(); } 

				// get the vertex handles and insert the constraint 
				v1 = *v1_iter;
				v2 = *v2_iter;
				constrained_delaunay_triangulation_2.insert_constraint(v1, v2);
			}
		}

		template <typename PointOnSphereForwardIterator>
		void
		insert_seed_points_into_mesh(
				GPlatesAppLogic::CgalUtils::cgal_mesher_2_type &mesher,
				PointOnSphereForwardIterator points_begin,
				PointOnSphereForwardIterator points_end)
		{
qDebug() << "insert_seed_points_into_mesh: ";
			std::vector<cgal_point_2_type> seed_points;

			// Loop over the points and convert them.
			for (PointOnSphereForwardIterator points_iter = points_begin;
				points_iter != points_end;
				++points_iter)
			{
				seed_points.push_back( convert_point_to_cgal_2(*points_iter));
			}

			mesher.clear_seeds();

			mesher.set_seeds(
				seed_points.begin(), 
				seed_points.end(),
				false);  // flag
/* from 
http://www.cgal.org/Manual/last/doc_html/cgal_manual/Mesh_2_ref/Class_Delaunay_mesher_2.html
If flag=true, the mesh domain is the union of the bounded connected components including at least one seed. 
If flag=false, the domain is the union of the bounded components including no seed. 
*/
		}

		//
		//
		template <typename PointOnSphereForwardIterator>
		void
		insert_points_into_delaunay_triangulation_hierarchy_3(
				cgal_triangulation_hierarchy_3_type &triangulation,
				PointOnSphereForwardIterator points_begin,
				PointOnSphereForwardIterator points_end)
		{
			std::vector<cgal_point_3_type> cgal_points;

			// Loop over the points and convert them.
			for (PointOnSphereForwardIterator points_iter = points_begin;
				points_iter != points_end;
				++points_iter)
			{
				cgal_points.push_back(
						convert_point_to_cgal_3(*points_iter));
			}

			// Build the Triangulation.
			triangulation.insert( cgal_points.begin(), cgal_points.end() );
		}
	}
}

		/* 
		From CGAL-3.5/examples/Mesh_2/mesh_with_seeds.cpp, and CGAL-3.5/examples/Mesh_2/mesh_class.cpp
		typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
		typedef CGAL::Triangulation_vertex_base_2<K> Vb;
		typedef CGAL::Delaunay_mesh_face_base_2<K> Fb;
		typedef CGAL::Triangulation_data_structure_2<Vb, Fb> Tds;
		typedef CGAL::Constrained_Delaunay_triangulation_2<K, Tds> CDT;
		typedef CGAL::Delaunay_mesh_size_criteria_2<CDT> Criteria;
		typedef CGAL::Delaunay_mesher_2<CDT, Criteria> Mesher;
		*/

#endif // GPLATES_APP_LOGIC_CGALUTILS_H

