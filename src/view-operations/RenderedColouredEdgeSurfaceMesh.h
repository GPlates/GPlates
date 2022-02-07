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

#ifndef GPLATES_VIEWOPERATIONS_RENDEREDCOLOUREDEDGESURFACEMESH_H
#define GPLATES_VIEWOPERATIONS_RENDEREDCOLOUREDEDGESURFACEMESH_H

#include <vector>

#include "RenderedGeometryImpl.h"
#include "RenderedGeometryVisitor.h"

#include "maths/PointOnSphere.h"
#include "maths/PolylineOnSphere.h"

#include "gui/ColourProxy.h"


namespace GPlatesViewOperations
{
	/**
	 * A non-filled edge mesh on the surface of the globe where each edge has its own colour.
	 */
	class RenderedColouredEdgeSurfaceMesh :
		public RenderedGeometryImpl
	{
	public:

		//! A mesh edge.
		struct Edge
		{
			Edge(
					unsigned int vertex_index1,
					unsigned int vertex_index2,
					// TODO: Change this to Colour once the deferred (until painting) colouring has been removed...
					const GPlatesGui::ColourProxy &colour_) :
				colour(colour_)
			{
				vertex_indices[0] = vertex_index1;
				vertex_indices[1] = vertex_index2;
			}

			/**
			 * Indices into the vertex array returned by the parent class @a get_mesh_vertices method.
			 */
			unsigned int vertex_indices[2];

			// TODO: Change this to Colour once the deferred (until painting) colouring has been removed.
			GPlatesGui::ColourProxy colour;
		};

		typedef std::vector<Edge> edge_seq_type;

		typedef std::vector<GPlatesMaths::PointOnSphere> vertex_seq_type;


		/**
		 * Construct from a sequence of edges and a sequence of vertices (@a PointOnSphere).
		 */
		template <typename EdgeForwardIter, typename PointOnSphereForwardIter>
		RenderedColouredEdgeSurfaceMesh(
				EdgeForwardIter edges_begin,
				EdgeForwardIter edges_end,
				PointOnSphereForwardIter vertices_begin,
				PointOnSphereForwardIter vertices_end,
				float line_width_hint) :
			d_mesh_edges(edges_begin, edges_end),
			d_mesh_vertices(vertices_begin, vertices_end),
			d_line_width_hint(line_width_hint)
		{  }


		/**
		 * Returns the mesh edges.
		 *
		 * NOTE: The edges are to be rendered as lines.
		 */
		const edge_seq_type &
		get_mesh_edges() const
		{
			return d_mesh_edges;
		}

		/**
		 * Returns the mesh vertices.
		 */
		const vertex_seq_type &
		get_mesh_vertices() const
		{
			return d_mesh_vertices;
		}

		float
		get_line_width_hint() const
		{
			return d_line_width_hint;
		}


		virtual
		void
		accept_visitor(
				ConstRenderedGeometryVisitor& visitor)
		{
			visitor.visit_rendered_coloured_edge_surface_mesh(*this);
		}

		virtual
		GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
		test_proximity(
				const GPlatesMaths::ProximityCriteria &criteria) const
		{
			edge_seq_type::const_iterator edges_iter = d_mesh_edges.begin();
			edge_seq_type::const_iterator edges_end = d_mesh_edges.end();
			for ( ; edges_iter != edges_end; ++edges_iter)
			{
				const Edge &edge = *edges_iter;

				// Test proximity to the current edge.
				const GPlatesMaths::PointOnSphere edge_points[2] =
				{
					d_mesh_vertices[edge.vertex_indices[0]],
					d_mesh_vertices[edge.vertex_indices[1]]
				};
				const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type edge_polyline =
						GPlatesMaths::PolylineOnSphere::create_on_heap(
								edge_points,
								edge_points + 2);

				// In addition to testing the edge interior we also test for closeness to the
				// edge outline in case it borders the entire mesh (ie, user might click 'close'
				// to the mesh but still outside the entire mesh within the closeness threshold).
				GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type outline_hit =
						edge_polyline->test_proximity(criteria);
				if (outline_hit)
				{
					// TODO: We should probably use 'PolylineOnSphere::is_close_to()' instead of
					// 'PolylineOnSphere::test_proximity()' and iterate over all edges to find
					// the closest one instead of just returning when first found close edge.
					return outline_hit;
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

		edge_seq_type d_mesh_edges;
		vertex_seq_type d_mesh_vertices;
		float d_line_width_hint;
	};
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDCOLOUREDEDGESURFACEMESH_H
