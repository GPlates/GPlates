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
 
#ifndef GPLATES_MATHS_HIERARCHICALTRIANGULARMESH_H
#define GPLATES_MATHS_HIERARCHICALTRIANGULARMESH_H

#include "UnitVector3D.h"


namespace GPlatesMaths
{
	/**
	 * Allows clients to recursively traverse a Hierarchical Triangular Mesh.
	 *
	 * See "The Hierarchical Triangular Mesh"
	 *    Peter Z. Kunszt, Alexander S. Szalay and Aniruddha R. Thakar
	 * ...for more details.
	 * We follow the same convention for ordering of vertices, etc.
	 */
	class HierarchicalTriangularMeshTraversal
	{
	public:
		//! Default constructor.
		HierarchicalTriangularMeshTraversal();


		/**
		 * A spherical triangle in the Hierarchical Triangular Mesh.
		 */
		class Triangle
		{
		public:
			Triangle(
					const UnitVector3D &vertex0_,
					const UnitVector3D &vertex1_,
					const UnitVector3D &vertex2_) :
				vertex0(vertex0_),
				vertex1(vertex1_),
				vertex2(vertex2_)
			{  }


			/**
			 * Visits the eight top-level spherical triangles that cover the sphere.
			 *
			 * The visitor class must have the method:
			 *
			 *    void
			 *    VisitorType::visit(
			 *         const GPlatesMaths::HierarchicalTriangularMeshTraversal::Triangle &,
			 *         RecursionContextType &);
			 *
			 * It will get called for each of the four child triangles of this triangle.
			 *
			 * @param recursion_context is arbitrary type (at minimum it can be the recursion depth).
			 *        It simply gets passed to the visitor's 'visit()' method.
			 */
			template <class VisitorType, typename RecursionContextType>
			void
			visit_children(
					VisitorType &visitor,
					RecursionContextType &recursion_context) const;


			// Note that we can use references due to the way the hierarchy is visited.
			// This saves a lot of copying during traversal.
			const UnitVector3D &vertex0;
			const UnitVector3D &vertex1;
			const UnitVector3D &vertex2;
		};



		/**
		 * Visits the eight top-level spherical triangles that cover the sphere.
		 *
		 * The visitor class must have the method:
		 *
		 *    void
		 *    VisitorType::visit(
		 *         const GPlatesMaths::HierarchicalTriangularMeshTraversal::Triangle &,
		 *         RecursionContextType &);
		 *
		 * It will get called for each of the eight top-level spherical triangles.
		 *
		 * @param recursion_context is arbitrary type (at minimum it can be the recursion depth).
		 *        It simply gets passed to the visitor's 'visit()' method.
		 */
		template <class VisitorType, typename RecursionContextType>
		void
		visit(
				VisitorType &visitor,
				RecursionContextType &recursion_context) const;

	private:
		UnitVector3D vertex0;
		UnitVector3D vertex1;
		UnitVector3D vertex2;
		UnitVector3D vertex3;
		UnitVector3D vertex4;
		UnitVector3D vertex5;
	};


	//
	// Implementation
	//


	inline
	HierarchicalTriangularMeshTraversal::HierarchicalTriangularMeshTraversal() :
		vertex0(0, 0, 1),
		vertex1(1, 0, 0),
		vertex2(0, 1, 0),
		vertex3(-1, 0, 0),
		vertex4(0, -1, 0),
		vertex5(0, 0, -1)
	{  }


	template <class VisitorType, typename RecursionContextType>
	void
	HierarchicalTriangularMeshTraversal::visit(
			VisitorType &visitor,
			RecursionContextType &recursion_context) const
	{
		const Triangle triangle0(vertex1, vertex5, vertex2);
		visitor.visit(triangle0, recursion_context);

		const Triangle triangle1(vertex2, vertex5, vertex3);
		visitor.visit(triangle1, recursion_context);

		const Triangle triangle2(vertex3, vertex5, vertex4);
		visitor.visit(triangle2, recursion_context);

		const Triangle triangle3(vertex4, vertex5, vertex1);
		visitor.visit(triangle3, recursion_context);

		const Triangle triangle4(vertex1, vertex0, vertex4);
		visitor.visit(triangle4, recursion_context);

		const Triangle triangle5(vertex4, vertex0, vertex3);
		visitor.visit(triangle5, recursion_context);

		const Triangle triangle6(vertex3, vertex0, vertex2);
		visitor.visit(triangle6, recursion_context);

		const Triangle triangle7(vertex2, vertex0, vertex1);
		visitor.visit(triangle7, recursion_context);
	}


	template <class VisitorType, typename RecursionContextType>
	void
	HierarchicalTriangularMeshTraversal::Triangle::visit_children(
			VisitorType &visitor,
			RecursionContextType &recursion_context) const
	{
		const UnitVector3D edge_midpoint0(
				(Vector3D(vertex1) + Vector3D(vertex2)).get_normalisation());

		const UnitVector3D edge_midpoint1(
				(Vector3D(vertex2) + Vector3D(vertex0)).get_normalisation());

		const UnitVector3D edge_midpoint2(
				(Vector3D(vertex0) + Vector3D(vertex1)).get_normalisation());

		const Triangle triangle0(vertex0, edge_midpoint2, edge_midpoint1);
		visitor.visit(triangle0, recursion_context);

		const Triangle triangle1(vertex1, edge_midpoint0, edge_midpoint2);
		visitor.visit(triangle1, recursion_context);

		const Triangle triangle2(vertex2, edge_midpoint1, edge_midpoint0);
		visitor.visit(triangle2, recursion_context);

		const Triangle triangle3(edge_midpoint0, edge_midpoint1, edge_midpoint2);
		visitor.visit(triangle3, recursion_context);
	}
}

#endif // GPLATES_MATHS_HIERARCHICALTRIANGULARMESH_H
