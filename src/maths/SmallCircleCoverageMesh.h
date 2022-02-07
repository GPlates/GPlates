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
 
#ifndef GPLATES_MATHS_SMALLCIRCLECOVERAGEMESH_H
#define GPLATES_MATHS_SMALLCIRCLECOVERAGEMESH_H

#include "SmallCircleBounds.h"
#include "HierarchicalTriangularMesh.h"
#include "UnitVector3D.h"


namespace GPlatesMaths
{
	/**
	 * The generated mesh that completely covers the region bounded by a small circle.
	 *
	 * Use @a SmallCircleCoverageMeshBuilder to create this.
	 */
	class SmallCircleCoverageMesh
	{
	public:
		//! A mesh triangle.
		struct Triangle
		{
			UnitVector3D vertex0;
			UnitVector3D vertex1;
			UnitVector3D vertex2;
		};


		/**
		 * The mesh triangles.
		 *
		 * There is no sharing of vertices between triangles - each triangle
		 * has its own copy of vertices.
		 */
		std::vector<Triangle> mesh;
	};


	/**
	 * Used to recurse into a hierarchical triangular mesh and generate a triangle
	 * mesh that completely covers the region bounded by a small circle.
	 */
	class SmallCircleCoverageMeshBuilder
	{
	public:
		//! Constructor.
		SmallCircleCoverageMeshBuilder(
				SmallCircleCoverageMesh &coverage_mesh,
				const BoundingSmallCircle &small_circle_bounds,
				unsigned int depth_to_generate_mesh);


		/**
		 * Adds coverage mesh triangles that completely cover the small circle bounds passed
		 * into constructor - triangles are added to the coverage mesh passed into constructor.
		 */
		void
		add_coverage_triangles();


	private:
		/**
		 * Keeps track of the recursion depth and whether we need to test child triangles
		 * against the small circle bounds (don't have to if parent is completely inside).
		 */
		struct RecursionContext
		{
			RecursionContext() :
				depth(0),
				test_against_bounds(true)
			{  }

			unsigned int depth;
			bool test_against_bounds;
		};


		//! The target for the generated mesh.
		SmallCircleCoverageMesh &d_coverage_mesh;

		//! Defines the small circle region that the coverage mesh will overlap.
		const BoundingSmallCircle &d_small_circle_bounds;

		//! The depth at which to generate mesh triangles.
		unsigned int d_depth_to_generate_mesh;


	public: // Must be public so @a HierarchicalTriangularMeshTraversal class can visit it.
		/**
		 * Visits a triangle in the hierarchical triangular mesh.
		 */
		void
		visit(
				const HierarchicalTriangularMeshTraversal::Triangle &triangle,
				const RecursionContext &recursion_context);
	};
}

#endif // GPLATES_MATHS_SMALLCIRCLECOVERAGEMESH_H
