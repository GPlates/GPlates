/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#ifndef GPLATES_VIEWOPERATIONS_RENDEREDCOLOUREDTRIANGLESURFACEMESH_H
#define GPLATES_VIEWOPERATIONS_RENDEREDCOLOUREDTRIANGLESURFACEMESH_H

#include <vector>

#include "RenderedGeometryImpl.h"
#include "RenderedGeometryVisitor.h"

#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolygonProximityHitDetail.h"
#include "maths/ProximityCriteria.h"

#include "gui/ColourProxy.h"


namespace GPlatesViewOperations
{
	/**
	 * A filled triangle mesh on the surface of the globe where each triangle is filled
	 * with its own colour.
	 */
	class RenderedColouredTriangleSurfaceMesh :
		public RenderedGeometryImpl
	{
	public:

		//! A mesh triangle.
		struct Triangle
		{
			Triangle(
					unsigned int vertex_index1,
					unsigned int vertex_index2,
					unsigned int vertex_index3,
					// TODO: Change this to Colour once the deferred (until painting) colouring has been removed...
					const GPlatesGui::ColourProxy &colour_) :
				colour(colour_)
			{
				vertex_indices[0] = vertex_index1;
				vertex_indices[1] = vertex_index2;
				vertex_indices[2] = vertex_index3;
			}

			/**
			 * Indices into the vertex array returned by the parent class @a get_mesh_vertices method.
			 */
			unsigned int vertex_indices[3];

			// TODO: Change this to Colour once the deferred (until painting) colouring has been removed.
			GPlatesGui::ColourProxy colour;
		};

		typedef std::vector<Triangle> triangle_seq_type;

		typedef std::vector<GPlatesMaths::PointOnSphere> vertex_seq_type;


		/**
		 * Construct from a sequence of triangles and a sequence of vertices (@a PointOnSphere).
		 */
		template <typename TriangleForwardIter, typename PointOnSphereForwardIter>
		RenderedColouredTriangleSurfaceMesh(
				TriangleForwardIter triangles_begin,
				TriangleForwardIter triangles_end,
				PointOnSphereForwardIter vertices_begin,
				PointOnSphereForwardIter vertices_end) :
			d_mesh_triangles(triangles_begin, triangles_end),
			d_mesh_vertices(vertices_begin, vertices_end)
		{  }


		/**
		 * Returns the mesh triangles.
		 *
		 * NOTE: The triangles should be rendered as filled.
		 * It does not make sense to render the triangles edges (as line segments) because the
		 * triangles adjacent to it could have different colours so the edge colour is unknown.
		 */
		const triangle_seq_type &
		get_mesh_triangles() const
		{
			return d_mesh_triangles;
		}

		/**
		 * Returns the mesh vertices.
		 */
		const vertex_seq_type &
		get_mesh_vertices() const
		{
			return d_mesh_vertices;
		}


		virtual
		void
		accept_visitor(
				ConstRenderedGeometryVisitor& visitor)
		{
			visitor.visit_rendered_coloured_triangle_surface_mesh(*this);
		}

		virtual
		GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
		test_proximity(
				const GPlatesMaths::ProximityCriteria &criteria) const
		{
			triangle_seq_type::const_iterator triangles_iter = d_mesh_triangles.begin();
			triangle_seq_type::const_iterator triangles_end = d_mesh_triangles.end();
			for ( ; triangles_iter != triangles_end; ++triangles_iter)
			{
				const Triangle &triangle = *triangles_iter;

				const GPlatesMaths::PointOnSphere triangle_points[3] =
				{
					d_mesh_vertices[triangle.vertex_indices[0]],
					d_mesh_vertices[triangle.vertex_indices[1]],
					d_mesh_vertices[triangle.vertex_indices[2]]
				};
				const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type triangle_polygon =
						GPlatesMaths::PolygonOnSphere::create_on_heap(
								triangle_points,
								triangle_points + 3);

				// In addition to testing the triangle interior we also test for closeness to the
				// triangle outline in case it borders the entire mesh (ie, user might click 'close'
				// to the mesh but still outside the entire mesh within the closeness threshold).
				//
				// Also by doing this test before the point-in-polygon fill test, and then returning
				// immediately, we get the benefit that if the test point is close to an edge of
				// the triangle mesh then its closeness will not necessarily be 1.0 (distance zero),
				// like a successful fill test will return, and hence the proximity will get sorted
				// nicely with respect to other 'line' geometries under the test point (with the fill
				// test this mesh will always be the first sorted choice due to zero proximity distance).
				GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type outline_hit =
						triangle_polygon->test_proximity(criteria);
				if (outline_hit)
				{
					// TODO: We should probably use 'PolygonOnSphere::is_close_to()' instead of
					// 'PolygonOnSphere::test_proximity()' and iterate over all triangles to find
					// the closest one instead of just returning when first found close triangle.
					return outline_hit;
				}

				// The mesh is filled (see comment in @a get_mesh_triangles) so see if the test point
				// is inside the current triangle's interior.
				const GPlatesMaths::PointInPolygon::Result point_in_polygon_result =
						triangle_polygon->is_point_in_polygon(
								criteria.test_point(),
								// We don't need anything fast since this is typically a user click point
								// (ie, a single point tested against the polygon). In any case the
								// polygon is going to be destroyed after this test...
								GPlatesMaths::PolygonOnSphere::LOW_SPEED_NO_SETUP_NO_MEMORY_USAGE);

				if (point_in_polygon_result == GPlatesMaths::PointInPolygon::POINT_INSIDE_POLYGON)
				{
					// The point is inside the polygon, hence it touches the polygon and therefore
					// has a closeness distance of zero (which is a dot product closeness of 1.0).
					return make_maybe_null_ptr(
							GPlatesMaths::PolygonProximityHitDetail::create(
									triangle_polygon,
									1.0/*closeness*/));
				}
			}

			// No hit.
			return GPlatesMaths::ProximityHitDetail::null;
		}

		virtual
		GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
		test_vertex_proximity(
				const GPlatesMaths::ProximityCriteria &criteria) const
		{
			vertex_seq_type::const_iterator vertices_iter = d_mesh_vertices.begin();
			vertex_seq_type::const_iterator vertices_end = d_mesh_vertices.end();
			for ( ; vertices_iter != vertices_end; ++vertices_iter)
			{
				const GPlatesMaths::PointOnSphere &vertex = *vertices_iter;

				GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type hit =
						vertex.test_vertex_proximity(criteria);
				if (hit)
				{
					return hit;
				}
			}

			// No hit.
			return GPlatesMaths::ProximityHitDetail::null;
		}		

	private:

		triangle_seq_type d_mesh_triangles;
		vertex_seq_type d_mesh_vertices;
	};
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDCOLOUREDTRIANGLESURFACEMESH_H
