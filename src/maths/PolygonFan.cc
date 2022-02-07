/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#include <boost/cast.hpp>
#include <QDebug>

#include "PolygonFan.h"

#include "Centroid.h"
#include "ConstGeometryOnSphereVisitor.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "utils/Profile.h"


namespace GPlatesMaths
{
	namespace
	{
		/**
		 * Creates a @a PolygonFan from a @a GeometryOnSphere.
		 */
		class CreatePolygonFanFromGeometryOnSphere :
				public ConstGeometryOnSphereVisitor
		{
		public:
			/**
			 * Returns the optionally created @a PolygonFan after visiting a @a GeometryOnSphere.
			 */
			boost::optional<PolygonFan::non_null_ptr_to_const_type>
			get_polygon_fan() const
			{
				return d_polygon_fan;
			}


			virtual
			void
			visit_multi_point_on_sphere(
					MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
			{
				d_polygon_fan = PolygonFan::create(multi_point_on_sphere);
			}

			virtual
			void
			visit_point_on_sphere(
					PointOnSphere::non_null_ptr_to_const_type /*point_on_sphere*/)
			{
				// Do nothing - can't create a polygon fan mesh from a single point.
			}

			virtual
			void
			visit_polygon_on_sphere(
					PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
			{
				d_polygon_fan = PolygonFan::create(polygon_on_sphere);
			}

			virtual
			void
			visit_polyline_on_sphere(
					PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
			{
				d_polygon_fan = PolygonFan::create(polyline_on_sphere);
			}

		private:
			boost::optional<PolygonFan::non_null_ptr_to_const_type> d_polygon_fan;
		};
	}
}


template <typename PointOnSphereForwardIter>
void
GPlatesMaths::PolygonFan::initialise(
		const PointOnSphereForwardIter polygon_points_begin,
		const unsigned int num_points,
		const GPlatesMaths::UnitVector3D &centroid)
{
	// Need at least three points for a polygon.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			num_points >= 3,
			GPLATES_ASSERTION_SOURCE);

	d_triangles.reserve(num_points);
	d_vertices.reserve(num_points + 2); // Includes the fan apex vertex and polygon-closing vertex.

	unsigned int vertex_index = 0;

	// First (apex) vertex is the centroid.
	d_vertices.push_back(Vertex(centroid));
	++vertex_index;

	// The remaining vertices form the boundary.
	PointOnSphereForwardIter polygon_points_iter = polygon_points_begin;
	for (unsigned int n = 0; n < num_points; ++n, ++vertex_index, ++polygon_points_iter)
	{
		d_vertices.push_back(Vertex(polygon_points_iter->position_vector()));

		Triangle triangle;

		triangle.d_vertex_indices[0] = 0; // Centroid.
		triangle.d_vertex_indices[1] = vertex_index; // Current boundary point.
		triangle.d_vertex_indices[2] = vertex_index + 1; // Next boundary point.

		d_triangles.push_back(triangle);
	}

	// Wraparound back to the first boundary vertex to close off the polygon.
	d_vertices.push_back(Vertex(polygon_points_begin->position_vector()));
}


GPlatesMaths::PolygonFan::non_null_ptr_to_const_type
GPlatesMaths::PolygonFan::create(
		const PolygonOnSphere::non_null_ptr_to_const_type &polygon)
{
	non_null_ptr_type polygon_fan(new PolygonFan());

	// Create the polygon fan mesh from the polygon vertices.
	polygon_fan->initialise(
			polygon->vertex_begin(),
			polygon->number_of_vertices(),
			polygon->get_boundary_centroid());

	return non_null_ptr_to_const_type(polygon_fan);
}


boost::optional<GPlatesMaths::PolygonFan::non_null_ptr_to_const_type>
GPlatesMaths::PolygonFan::create(
		const PolylineOnSphere::non_null_ptr_to_const_type &polyline)
{
	// Need at least three vertices to form a polygon.
	if (polyline->number_of_vertices() < 3)
	{
		return boost::none;
	}

	non_null_ptr_type polygon_fan(new PolygonFan());

	// Create the polygon fan mesh from the polyline vertices.
	// The first and last vertices will close off to form a polygon.
	polygon_fan->initialise(
			polyline->vertex_begin(),
			polyline->number_of_vertices(),
			polyline->get_centroid());

	return non_null_ptr_to_const_type(polygon_fan);
}


boost::optional<GPlatesMaths::PolygonFan::non_null_ptr_to_const_type>
GPlatesMaths::PolygonFan::create(
		const MultiPointOnSphere::non_null_ptr_to_const_type &multi_point)
{
	// Need at least three points to form a polygon.
	if (multi_point->number_of_points() < 3)
	{
		return boost::none;
	}

	non_null_ptr_type polygon_fan(new PolygonFan());

	// Create the polygon fan mesh from the multi-point vertices.
	// A polygon is formed from the multipoint by treating the order of points in the multipoint
	// as the vertices of a polygon.
	polygon_fan->initialise(
			multi_point->begin(),
			multi_point->number_of_points(),
			multi_point->get_centroid());

	return non_null_ptr_to_const_type(polygon_fan);
}


boost::optional<GPlatesMaths::PolygonFan::non_null_ptr_to_const_type>
GPlatesMaths::PolygonFan::create(
		const GeometryOnSphere::non_null_ptr_to_const_type &geometry_on_sphere)
{
	CreatePolygonFanFromGeometryOnSphere visitor;

	geometry_on_sphere->accept_visitor(visitor);

	return visitor.get_polygon_fan();
}
