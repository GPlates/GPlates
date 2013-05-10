/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
 * Copyright (C) 2012 2013 California Institute of Technology 
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

#ifndef GPLATES_APP_LOGIC_RESOLVEDTRIANGULATIONUTILS_H
#define GPLATES_APP_LOGIC_RESOLVEDTRIANGULATIONUTILS_H

#include <map>
#include <utility>
#include <vector>

#include <CGAL/Origin.h> // Must come before <CGAL/barycenter.h> since it uses it but doesn't define it.
#include <CGAL/barycenter.h>
#include <CGAL/centroid.h>

#include <QDebug>

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "maths/PointOnSphere.h"

#include "utils/Profile.h"


namespace GPlatesAppLogic
{
	namespace ResolvedTriangulation
	{
		/**
		 * Convenient utility class to assign indices (starting at zero) to triangulation vertices.
		 *
		 * This is useful for rendering triangle meshes (in OpenGL) using vertex-indexed triangle meshes.
		 */
		template <class TriangulationType>
		class VertexIndices
		{
		public:

			//! Typedef for the triangulation.
			typedef TriangulationType triangulation_type;

			//! Typedef for a sequence of vertices (vertex handles).
			typedef std::vector<typename triangulation_type::Vertex_handle> vertex_seq_type;


			/**
			 * Adds @a vertex and returns the index assigned to @a vertex.
			 *
			 * Assigns the next index (starting from zero) if @a vertex has not been seen before,
			 * otherwise returns the existing (previously allocated) index.
			 */
			unsigned int
			add_vertex(
					typename triangulation_type::Vertex_handle vertex)
			{
				// Attempt to insert the vertex into the map.
				std::pair<typename vertex_index_map_type::iterator, bool> vertex_insert_result =
						d_vertex_index_map.insert(
								typename vertex_index_map_type::value_type(vertex, d_vertices.size()/*index*/));
				if (vertex_insert_result.second)
				{
					// Insertion successful - first time seen vertex - add vertex to the sequence.
					d_vertices.push_back(vertex);
				}

				return vertex_insert_result.first->second;
			}

			/**
			 * Returns the sequence of (unique) vertices added by @a add_vertex.
			 */
			const vertex_seq_type &
			get_vertices() const
			{
				return d_vertices;
			}

		private:
			/**
			 * Keeps track of indices assigned to vertices.
			 */
			typedef std::map<typename triangulation_type::Vertex_handle, unsigned int> vertex_index_map_type;

			vertex_index_map_type d_vertex_index_map;
			vertex_seq_type d_vertices;
		};


		/**
		 * Interpolates the function values in @a function_value according to @a natural_neighbor_coordinates.
		 *
		 * This function is essentially the same as CGAL::linear_interpolation except that the
		 * interpolated value does not need to be a scalar, and the interpolation coordinates and
		 * norm are packed into @a natural_neighbor_coordinates.
		 * So 'Functor' should have the same concept as CGAL::Data_access.
		 *
		 * Each point in the triangulation used in @a natural_neighbor_coordinates
		 * should be have a valid value returned by @a function_value (eg, if @a function_value is
		 * a CGAL::Data_access then the std::map passed into it should store a function value for
		 * each 2D point in the natural neighbor coordinates).
		 *
		 * The value type (returned by 'Functor') should support addition with itself and support
		 * multiplication by a scalar (eg, a GPlatesMaths::Vector3D).
		 */
		template <class Functor, class CoordType, class Point2Type>
		typename Functor::result_type::first_type
		linear_interpolation_2(
				const std::pair<
						std::vector< std::pair<Point2Type, CoordType> >,
						CoordType> &natural_neighbor_coordinates_2,
				const Functor &function_value);


		/**
		 * For the test point p0 in the triangle formed by p1 p2 p3, compute the
		 * barycentric coordinates, b1, b2, b3 normalized by b0 (such that b1 + b2 + b3 = 1).
		 */
		template <class Point2Type, typename CoordType>
		void
		get_barycentric_coords_2(
				const Point2Type &p0,
				const Point2Type &p1,
				const Point2Type &p2,
				const Point2Type &p3,
				CoordType &b0,
				CoordType &b1,
				CoordType &b2,
				CoordType &b3);


		/**
		 * Convert a Cartesian (x,y,z) point from PointOnSphere to a 3D CGAL point type.
		 */
		template <class Point3Type>
		Point3Type
		convert_point_on_sphere_to_point_3(
				const GPlatesMaths::PointOnSphere &point);


		/**
		 * Convert a Cartesian (x,y,z) point from a 3D CGAL point type to PointOnSphere.
		 */
		template <class Point3Type>
		GPlatesMaths::PointOnSphere
		convert_point_3_to_point_on_sphere(
				const Point3Type &point_3);
	}
}

//
// Implementation
//

namespace GPlatesAppLogic
{
	namespace ResolvedTriangulation
	{
		template <class Functor, class CoordType, class Point2Type>
		typename Functor::result_type::first_type
		linear_interpolation_2(
				const std::pair<
						std::vector< std::pair<Point2Type, CoordType> >,
						CoordType> &natural_neighbor_coordinates_2,
				const Functor &function_value)
		{
			typedef typename Functor::result_type::first_type value_type;
			typedef std::vector< std::pair<Point2Type, CoordType> > point_2_coordinate_vector_type;

			const point_2_coordinate_vector_type &coords = natural_neighbor_coordinates_2.first;
			const CoordType &norm = natural_neighbor_coordinates_2.second;

			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					norm > 0,
					GPLATES_ASSERTION_SOURCE);
			const CoordType inv_norm = 1 / norm;

			value_type result = value_type();

			// Interpolate the function values.
			typename point_2_coordinate_vector_type::const_iterator coords_iter = coords.begin();
			typename point_2_coordinate_vector_type::const_iterator coords_end = coords.end();
			for( ; coords_iter != coords_end; ++coords_iter)
			{
				const Point2Type &point_2 = coords_iter->first;
				typename Functor::result_type value = function_value(point_2);
				// Make sure a function value was found for the current 2D point (of triangulation).
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						value.second,
						GPLATES_ASSERTION_SOURCE);

				result = result + value.first * (coords_iter->second * inv_norm);
			}

			return result;
		}


		template <class Point2Type, typename CoordType>
		void
		get_barycentric_coords_2(
				const Point2Type &p0,
				const Point2Type &p1,
				const Point2Type &p2,
				const Point2Type &p3,
				CoordType &b0,
				CoordType &b1,
				CoordType &b2,
				CoordType &b3)
		{
			PROFILE_FUNC();
		#if 0 
			from: 
			http://steve.hollasch.net/cgindex/math/barycentric.html
			
		A fail-proof method is to compute the barycentric coordinates. For a triangle {(x1,y1), (x2,y2), (x3,y3)} and some point (x0,y0), calculate

		b0 =  (x2 - x1) * (y3 - y1) - (x3 - x1) * (y2 - y1)
		b1 = ((x2 - x0) * (y3 - y0) - (x3 - x0) * (y2 - y0)) / b0 
		b2 = ((x3 - x0) * (y1 - y0) - (x1 - x0) * (y3 - y0)) / b0
		b3 = ((x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0)) / b0 

		Then if b1, b2, and b3 are all > 0, (x0,y0) is strictly inside the triangle; 
		if bi = 0 and the other two coordinates are positive, (x0,y0) lies on the edge opposite (xi,yi); 
		if bi and bj = 0, (x0,y0) lies on (xk,yk); if bi < 0, (x0,y0) lies outside the edge opposite (xi,yi); 
		if all three coordinates are negative, something else is wrong. 
		This method does not depend on the cyclic order of the vertices. 
		#endif

			b0 =  ( p2.x() - p1.x() ) * ( p3.y() - p1.y() ) - ( p3.x() - p1.x() ) * ( p2.y() - p1.y() );
			const CoordType inv_b0 = 1.0 / b0;

			b1 = (( p2.x() - p0.x() ) * ( p3.y() - p0.y() ) - ( p3.x() - p0.x() ) * ( p2.y() - p0.y() )) * inv_b0;
			b2 = (( p3.x() - p0.x() ) * ( p1.y() - p0.y() ) - ( p1.x() - p0.x() ) * ( p3.y() - p0.y() )) * inv_b0;
			b3 = (( p1.x() - p0.x() ) * ( p2.y() - p0.y() ) - ( p2.x() - p0.x() ) * ( p1.y() - p0.y() )) * inv_b0;
		}


		template <class Point3Type>
		Point3Type
		convert_point_on_sphere_to_point_3(
				const GPlatesMaths::PointOnSphere &point)
		{
			// Create a 3D point from the point on sphere.
			return Point3Type(
					point.position_vector().x().dval(), 
					point.position_vector().y().dval(), 
					point.position_vector().z().dval());
		}


		template <class Point3Type>
		GPlatesMaths::PointOnSphere
		convert_point_3_to_point_on_sphere(
				const Point3Type &point_3)
		{
			// Create a 3D point on sphere from a 3D CGAL point.
			return GPlatesMaths::PointOnSphere(
					GPlatesMaths::UnitVector3D(
							point_3.x(),
							point_3.y(),
							point_3.z()));
		}
	}
}

#endif // GPLATES_APP_LOGIC_RESOLVEDTRIANGULATIONUTILS_H
