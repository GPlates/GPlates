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

#include "SmallCircleCoverageMesh.h"

#include "PointOnSphere.h"
#include "PolygonOnSphere.h"


GPlatesMaths::SmallCircleCoverageMeshBuilder::SmallCircleCoverageMeshBuilder(
		SmallCircleCoverageMesh &coverage_mesh,
		const BoundingSmallCircle &small_circle_bounds,
		unsigned int depth_to_generate_mesh) :
	d_coverage_mesh(coverage_mesh),
	d_small_circle_bounds(small_circle_bounds),
	d_depth_to_generate_mesh(depth_to_generate_mesh)
{
}


void
GPlatesMaths::SmallCircleCoverageMeshBuilder::add_coverage_triangles()
{
	HierarchicalTriangularMeshTraversal htm;

	const RecursionContext recursion_context;

	htm.visit(*this, recursion_context);
}


void
GPlatesMaths::SmallCircleCoverageMeshBuilder::visit(
		const HierarchicalTriangularMeshTraversal::Triangle &triangle,
		const RecursionContext &recursion_context)
{
	RecursionContext children_recursion_context(recursion_context);

	if (recursion_context.test_against_bounds)
	{
		const PointOnSphere triangle_vertices[3] =
		{
			PointOnSphere(triangle.vertex0), PointOnSphere(triangle.vertex1), PointOnSphere(triangle.vertex2)
		};

		// Create a polygon from the triangle vertices.
		const PolygonOnSphere::non_null_ptr_to_const_type triangle_poly =
				PolygonOnSphere::create_on_heap(triangle_vertices, triangle_vertices + 3);

		// Test the triangle against the small circle bounds.
		// We test as a filled polygon in case the triangle completely surrounds the
		// small circle in which case we want the result to be intersection not outside.
		//
		// Since the point-in-polygon test will only ever get called once on this polygon
		// it's not worth it creating one to pass in.
		const BoundingSmallCircle::Result bounds_result =
				d_small_circle_bounds.test_filled_polygon(*triangle_poly);

		// If the triangle is completely outside the small circle bounds then return early.
		if (bounds_result == BoundingSmallCircle::OUTSIDE_BOUNDS)
		{
			return;
		}

		// If the triangle is completely inside the small circle bounds then
		// it's children don't need to test against the bounds either since they
		// are fully contained by their parent.
		if (bounds_result == BoundingSmallCircle::INSIDE_BOUNDS)
		{
			children_recursion_context.test_against_bounds = false;
		}
	}

	// If we're at the correct depth then add the triangle to our mesh.
	if (recursion_context.depth == d_depth_to_generate_mesh)
	{
		const SmallCircleCoverageMesh::Triangle mesh_triangle =
		{
			triangle.vertex0,
			triangle.vertex1,
			triangle.vertex2
		};

		d_coverage_mesh.mesh.push_back(mesh_triangle);

		return;
	}

	// Recurse into the child triangles.
	++children_recursion_context.depth;
	triangle.visit_children(*this, children_recursion_context);
}
