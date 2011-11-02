/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#include "global/CompilerWarnings.h"
PUSH_MSVC_WARNINGS
DISABLE_MSVC_WARNING(4503)
#include <map>
POP_MSVC_WARNINGS

#include <boost/cast.hpp>

#ifdef NDEBUG
#      define HAVE_NDEBUG
#      undef NDEBUG
#endif

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Delaunay_mesher_2.h>
#include <CGAL/Delaunay_mesh_face_base_2.h>
#include <CGAL/Delaunay_mesh_size_criteria_2.h>
#include <CGAL/Cartesian.h>
#include <CGAL/Polygon_2.h>

#ifdef HAVE_NDEBUG
#      define NDEBUG
#endif

#include <QDebug>

#include "PolygonMesh.h"

#include "Centroid.h"
#include "ConstGeometryOnSphereVisitor.h"

#include "utils/Profile.h"


namespace GPlatesMaths
{
	namespace
	{
		/**
		 * Creates a @a PolygonMesh from a @a GeometryOnSphere.
		 */
		class CreatePolygonMeshFromGeometryOnSphere :
				public ConstGeometryOnSphereVisitor
		{
		public:
			/**
			 * Returns the optionally created @a PolygonMesh after visiting a @a GeometryOnSphere.
			 */
			boost::optional<PolygonMesh::non_null_ptr_to_const_type>
			get_polygon_mesh() const
			{
				return d_polygon_mesh;
			}


			virtual
			void
			visit_multi_point_on_sphere(
					MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
			{
				d_polygon_mesh = PolygonMesh::create(multi_point_on_sphere);
			}

			virtual
			void
			visit_point_on_sphere(
					PointOnSphere::non_null_ptr_to_const_type /*point_on_sphere*/)
			{
				// Do nothing - can't create a polygon mesh from a single point.
			}

			virtual
			void
			visit_polygon_on_sphere(
					PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
			{
				d_polygon_mesh = PolygonMesh::create(polygon_on_sphere);
			}

			virtual
			void
			visit_polyline_on_sphere(
					PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
			{
				d_polygon_mesh = PolygonMesh::create(polyline_on_sphere);
			}

		private:
			boost::optional<PolygonMesh::non_null_ptr_to_const_type> d_polygon_mesh;
		};


		/**
		 * Projects a unit vector point onto the plane whose normal is @a plane_normal and
		 * returns normalised version of projected point.
		 */
		GPlatesMaths::UnitVector3D
		get_orthonormal_vector(
				const UnitVector3D &point,
				const UnitVector3D &plane_normal)
		{
			// The projection of 'point' in the direction of 'plane_normal'.
			Vector3D proj = dot(point, plane_normal) * plane_normal;

			// The projection of 'point' perpendicular to the direction of
			// 'plane_normal'.
			return (Vector3D(point) - proj).get_normalisation();
		}
	}
}


DISABLE_GCC_WARNING("-Wshadow")

template <typename PointOnSphereForwardIter>
bool
GPlatesMaths::PolygonMesh::initialise(
		PointOnSphereForwardIter polygon_points_begin,
		PointOnSphereForwardIter polygon_points_end)
{
	//PROFILE_FUNC();

	typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
	typedef CGAL::Triangulation_vertex_base_2<K> Vb;
	typedef CGAL::Delaunay_mesh_face_base_2<K> Fb;
	typedef CGAL::Triangulation_data_structure_2<Vb, Fb> Tds;
	typedef CGAL::Constrained_Delaunay_triangulation_2<K, Tds> CDT;
	typedef CGAL::Delaunay_mesh_size_criteria_2<CDT> Criteria;

	typedef CGAL::Cartesian<double> KP;
	typedef KP::Point_2 Point;
	typedef CGAL::Polygon_2<KP> Polygon_2;

	if (polygon_points_begin == polygon_points_end)
	{
		// There are no vertices.
		qWarning() << "PolygonMesh::initialise: no vertices specified.";
		return false;
	}

	// Clip each polygon to the current cube face.
	//
	// Instead, for now, just project onto an arbitrary plane.

	// Calculate the sum of vertex positions.
	Vector3D summed_vertex_position =
			Centroid::calculate_sum_points(polygon_points_begin, polygon_points_end);

	// If the magnitude of the summed vertex position is zero then all the points averaged
	// to zero and hence we cannot get a plane normal to project onto.
	// This most likely happens when the vertices roughly form a great circle arc and hence
	// then are two possible projection directions and hence you could assign the orientation
	// to be either clockwise or counter-clockwise.
	// If this happens we'll just choose one orientation arbitrarily.
	if (summed_vertex_position.magSqrd() <= 0.0)
	{
		// Arbitrarily pick a vertex as the centroid.
		// Not a good solution but it's extremely unlikely a polygon will sum to zero.
		summed_vertex_position = Vector3D(polygon_points_begin->position_vector());
	}

	// Calculate a unit vector from the sum to use as our plane normal.
	const GPlatesMaths::UnitVector3D proj_plane_normal =
			summed_vertex_position.get_normalisation();

	// First try starting with the global x axis - if it's too close to the plane normal
	// then choose the global y axis.
	GPlatesMaths::UnitVector3D proj_plane_x_axis_test_point(0, 0, 1); // global x-axis
	if (dot(proj_plane_x_axis_test_point, proj_plane_normal) > 1 - 1e-2)
	{
		proj_plane_x_axis_test_point = GPlatesMaths::UnitVector3D(0, 1, 0); // global y-axis
	}
	const GPlatesMaths::UnitVector3D proj_plane_axis_x = get_orthonormal_vector(
			proj_plane_x_axis_test_point, proj_plane_normal);

	// Determine the y axis of the plane.
	const GPlatesMaths::UnitVector3D proj_plane_axis_y(
			cross(proj_plane_normal, proj_plane_axis_x));

	Polygon_2 polygon_2;

	// If the first vertex of the polygon is the same as (or extremely close to) the
	// last vertex then CGAL will complain that the polygon is not a simple polygon.
	// We avoid this by skipping the last vertex in this case.
	PointOnSphereForwardIter last_polygon_points_iter = polygon_points_end;
	--last_polygon_points_iter;
	if (*polygon_points_begin == *last_polygon_points_iter)
	{
		--polygon_points_end;
	}

	// Iterate through the vertices again and project onto the plane.
	for (PointOnSphereForwardIter vertex_iter = polygon_points_begin;
		vertex_iter != polygon_points_end;
		++vertex_iter)
	{
		const GPlatesMaths::PointOnSphere &vertex = *vertex_iter;
		const GPlatesMaths::UnitVector3D &point = vertex.position_vector();

		const GPlatesMaths::real_t proj_point_z = dot(proj_plane_normal, point);
		// For now, if any point isn't localised on the plane then discard polygon.
		if (proj_point_z < 0.15)
		{
			qWarning() << "PolygonMesh::initialise: Unable to project polygon - it's too big.";
			return false;
		}
		const GPlatesMaths::real_t inv_proj_point_z = 1.0 / proj_point_z;

		const GPlatesMaths::real_t proj_point_x = inv_proj_point_z * dot(proj_plane_axis_x, point);
		const GPlatesMaths::real_t proj_point_y = inv_proj_point_z * dot(proj_plane_axis_y, point);

		polygon_2.push_back(Point(proj_point_x.dval(), proj_point_y.dval()));
	}

	// For now, if the polygon is not simple (ie, it's self-intersecting) then discard polygon.
	if (!polygon_2.is_simple())
	{
		qWarning() << "PolygonMesh::initialise: Unable to mesh polygon - it's self-intersecting.";
		return false;
	}

	// Use a set in case CGAL merges any vertices.
	std::map<CDT::Vertex_handle, unsigned int/*vertex index*/> unique_vertex_handles;
	std::vector<CDT::Vertex_handle> vertex_handles;
	CDT cdt;
	for (Polygon_2::Vertex_iterator vert_iter = polygon_2.vertices_begin();
		vert_iter != polygon_2.vertices_end();
		++vert_iter)
	{
		CDT::Vertex_handle vertex_handle = cdt.insert(CDT::Point(vert_iter->x(), vert_iter->y()));
		if (unique_vertex_handles.insert(
				std::map<CDT::Vertex_handle, unsigned int>::value_type(
						vertex_handle, vertex_handles.size())).second)
		{
			vertex_handles.push_back(vertex_handle);
		}
	}

	// For now, if the polygon has less than three vertices then discard it.
	// This can happen if CGAL determines two points are close enough to be merged.
	if (vertex_handles.size() < 3)
	{
		qWarning() << "PolygonMesh::initialise: Polygon has less than 3 vertices after triangulation.";
		return false;
	}

	// Add the boundary constraints.
	for (std::size_t vert_index = 1; vert_index < vertex_handles.size(); ++vert_index)
	{
		cdt.insert_constraint(vertex_handles[vert_index - 1], vertex_handles[vert_index]);
	}
	cdt.insert_constraint(vertex_handles[vertex_handles.size() - 1], vertex_handles[0]);

	// Mesh the domain of the triangulation - the area bounded by constraints.
	//PROFILE_BEGIN(cgal_refine_triangulation, "CGAL::refine_Delaunay_mesh_2");
	CGAL::refine_Delaunay_mesh_2(cdt, Criteria(0.125, 0.25));
	//PROFILE_END(cgal_refine_triangulation);

	// Iterate over the mesh triangles and collect the triangles belonging to the domain.
	typedef std::map<CDT::Vertex_handle, std::size_t/*vertex index*/> mesh_map_type;
	mesh_map_type mesh_vertex_handles;
	for (CDT::Finite_faces_iterator triangle_iter = cdt.finite_faces_begin();
		triangle_iter != cdt.finite_faces_end();
		++triangle_iter)
	{
		if (!triangle_iter->is_in_domain())
		{
			continue;
		}

		Triangle triangle;

		for (unsigned int tri_vert_index = 0; tri_vert_index < 3; ++tri_vert_index)
		{
			CDT::Vertex_handle vertex_handle = triangle_iter->vertex(tri_vert_index);

			std::pair<mesh_map_type::iterator, bool> p = mesh_vertex_handles.insert(
					mesh_map_type::value_type(
							vertex_handle, d_vertices.size()));

			const std::size_t mesh_vertex_index = p.first->second;
			if (p.second)
			{
				// Unproject the mesh point back onto the sphere.
				const CDT::Point &point2d = vertex_handle->point();
				const GPlatesMaths::UnitVector3D point3d =
					(GPlatesMaths::Vector3D(proj_plane_normal) +
					point2d.x() * proj_plane_axis_x +
					point2d.y() * proj_plane_axis_y).get_normalisation();

				d_vertices.push_back(Vertex(point3d));
			}

			triangle.d_vertex_indices[tri_vert_index] = mesh_vertex_index;
		}

		d_triangles.push_back(triangle);
	}

	return true;
}

// See above.
ENABLE_GCC_WARNING("-Wshadow")


boost::optional<GPlatesMaths::PolygonMesh::non_null_ptr_to_const_type>
GPlatesMaths::PolygonMesh::create(
		const PolygonOnSphere::non_null_ptr_to_const_type &polygon)
{
	non_null_ptr_type polygon_mesh(new PolygonMesh());

	// Attempt to create the polygon mesh from the polygon vertices.
	if (!polygon_mesh->initialise(polygon->vertex_begin(), polygon->vertex_end()))
	{
		return boost::none;
	}

	return non_null_ptr_to_const_type(polygon_mesh);
}


boost::optional<GPlatesMaths::PolygonMesh::non_null_ptr_to_const_type>
GPlatesMaths::PolygonMesh::create(
		const PolylineOnSphere::non_null_ptr_to_const_type &polyline)
{
	non_null_ptr_type polygon_mesh(new PolygonMesh());

	// Attempt to create the polygon mesh from the polyline vertices.
	// The first and last vertices will close off to form a polygon.
	if (!polygon_mesh->initialise(polyline->vertex_begin(), polyline->vertex_end()))
	{
		return boost::none;
	}

	return non_null_ptr_to_const_type(polygon_mesh);
}


boost::optional<GPlatesMaths::PolygonMesh::non_null_ptr_to_const_type>
GPlatesMaths::PolygonMesh::create(
		const MultiPointOnSphere::non_null_ptr_to_const_type &multi_point)
{
	non_null_ptr_type polygon_mesh(new PolygonMesh());

	// Attempt to create the polygon mesh from the multi-point vertices.
	// A polygon is formed from the multipoint by treating the order of points in the multipoint
	// as the vertices of a polygon.
	if (!polygon_mesh->initialise(multi_point->begin(), multi_point->end()))
	{
		return boost::none;
	}

	return non_null_ptr_to_const_type(polygon_mesh);
}


boost::optional<GPlatesMaths::PolygonMesh::non_null_ptr_to_const_type>
GPlatesMaths::PolygonMesh::create(
		const GeometryOnSphere::non_null_ptr_to_const_type &geometry_on_sphere)
{
	CreatePolygonMeshFromGeometryOnSphere visitor;

	geometry_on_sphere->accept_visitor(visitor);

	return visitor.get_polygon_mesh();
}


// NOTE: Do not remove this.
// This is here to avoid CGAL related compile errors on MacOS.
// It seems we only need this at the end of the file - perhaps it's something to do with
// template instantiations happening at the end of the translation unit.
DISABLE_GCC_WARNING("-Wshadow")
