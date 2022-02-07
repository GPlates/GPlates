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
 
#ifndef GPLATES_MATHS_SPHERICALSUBDIVISION_H
#define GPLATES_MATHS_SPHERICALSUBDIVISION_H

#include "UnitVector3D.h"
#include "Vector3D.h"


namespace GPlatesMaths
{
	namespace SphericalSubdivision
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
				 *         const GPlatesMaths::SphericalSubdivision::HierarchicalTriangularMeshTraversal::Triangle &,
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
			 *         const GPlatesMaths::SphericalSubdivision::HierarchicalTriangularMeshTraversal::Triangle &,
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


		/**
		 * Allows clients to recursively traverse a subdivided Rhombic Triacontahedron in a quad-tree manner.
		 *
		 * There are 30 quad faces using 32 vertices.
		 *
		 * This produces a more uniform distribution of vertices compared to the Hierarchical Triangular Mesh.
		 */
		class RhombicTriacontahedronTraversal
		{
		public:

			//! Default constructor.
			RhombicTriacontahedronTraversal();


			/**
			 * A quad patch in the subdivided Rhombic Triacontahedron.
			 */
			class Quad
			{
			public:
				Quad(
						const UnitVector3D &vertex0_,
						const UnitVector3D &vertex1_,
						const UnitVector3D &vertex2_,
						const UnitVector3D &vertex3_) :
					vertex0(vertex0_),
					vertex1(vertex1_),
					vertex2(vertex2_),
					vertex3(vertex3_)
				{  }


				/**
				 * Visits the thirty top-level quad faces that cover the sphere.
				 *
				 * The visitor class must have the method:
				 *
				 *    void
				 *    VisitorType::visit(
				 *         const GPlatesMaths::SphericalSubdivision::RhombicTriacontahedronTraversal::Quad &,
				 *         RecursionContextType &);
				 *
				 * It will get called for each of the four child quads of this quad.
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
				const UnitVector3D &vertex3;
			};



			/**
			 * Visits the thirty top-level quad faces that cover the sphere.
			 *
			 * The visitor class must have the method:
			 *
			 *    void
			 *    VisitorType::visit(
			 *         const GPlatesMaths::SphericalSubdivision::RhombicTriacontahedronTraversal::Quad &,
			 *         RecursionContextType &);
			 *
			 * It will get called for each of the thirty top-level quad faces.
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

			static
			UnitVector3D
			normalise(
					const double &x,
					const double &y,
					const double &z)
			{
				return Vector3D(x, y, z).get_normalisation();
			}

			static
			UnitVector3D
			normalise(
					const Vector3D &v)
			{
				return v.get_normalisation();
			}


			static const double GOLDEN_RATIO;
			static const double GOLDEN_RATIO_2;
			static const double GOLDEN_RATIO_3;


			UnitVector3D vertex2, vertex4, vertex6, vertex8;
			UnitVector3D vertex11, vertex12, vertex13, vertex16;
			UnitVector3D vertex17, vertex18, vertex20, vertex23;
			UnitVector3D vertex27, vertex28, vertex30, vertex31;
			UnitVector3D vertex33, vertex34, vertex36, vertex37;
			UnitVector3D vertex38, vertex41, vertex45, vertex46;
			UnitVector3D vertex47, vertex50, vertex51, vertex52;
			UnitVector3D vertex54, vertex56, vertex58, vertex60;
		};
	}
}


//
// Implementation
//


namespace GPlatesMaths
{
	namespace SphericalSubdivision
	{
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


		template <class VisitorType, typename RecursionContextType>
		void
		RhombicTriacontahedronTraversal::visit(
				VisitorType &visitor,
				RecursionContextType &recursion_context) const
		{
			const Quad quad0(vertex2, vertex11, vertex12, vertex4);
			visitor.visit(quad0, recursion_context);

			const Quad quad1(vertex2, vertex4, vertex6, vertex8);
			visitor.visit(quad1, recursion_context);

			const Quad quad2(vertex2, vertex8, vertex17, vertex18);
			visitor.visit(quad2, recursion_context);

			const Quad quad3(vertex2, vertex18, vertex37, vertex20);
			visitor.visit(quad3, recursion_context);

			const Quad quad4(vertex2, vertex20, vertex27, vertex11);
			visitor.visit(quad4, recursion_context);

			const Quad quad5(vertex11, vertex27, vertex28, vertex12);
			visitor.visit(quad5, recursion_context);

			const Quad quad6(vertex4, vertex12, vertex13, vertex6);
			visitor.visit(quad6, recursion_context);

			const Quad quad7(vertex8, vertex6, vertex16, vertex17);
			visitor.visit(quad7, recursion_context);

			const Quad quad8(vertex18, vertex17, vertex36, vertex37);
			visitor.visit(quad8, recursion_context);

			const Quad quad9(vertex20, vertex37, vertex38, vertex27);
			visitor.visit(quad9, recursion_context);

			const Quad quad10(vertex27, vertex38, vertex54, vertex45);
			visitor.visit(quad10, recursion_context);

			const Quad quad11(vertex27, vertex45, vertex46, vertex28);
			visitor.visit(quad11, recursion_context);

			const Quad quad12(vertex12, vertex28, vertex46, vertex30);
			visitor.visit(quad12, recursion_context);

			const Quad quad13(vertex12, vertex30, vertex31, vertex13);
			visitor.visit(quad13, recursion_context);

			const Quad quad14(vertex6, vertex13, vertex31, vertex23);
			visitor.visit(quad14, recursion_context);

			const Quad quad15(vertex6, vertex23, vertex33, vertex16);
			visitor.visit(quad15, recursion_context);

			const Quad quad16(vertex17, vertex16, vertex33, vertex34);
			visitor.visit(quad16, recursion_context);

			const Quad quad17(vertex17, vertex34, vertex51, vertex36);
			visitor.visit(quad17, recursion_context);

			const Quad quad18(vertex37, vertex36, vertex51, vertex52);
			visitor.visit(quad18, recursion_context);

			const Quad quad19(vertex37, vertex52, vertex54, vertex38);
			visitor.visit(quad19, recursion_context);

			const Quad quad20(vertex45, vertex54, vertex56, vertex46);
			visitor.visit(quad20, recursion_context);

			const Quad quad21(vertex30, vertex46, vertex47, vertex31);
			visitor.visit(quad21, recursion_context);

			const Quad quad22(vertex23, vertex31, vertex41, vertex33);
			visitor.visit(quad22, recursion_context);

			const Quad quad23(vertex34, vertex33, vertex50, vertex51);
			visitor.visit(quad23, recursion_context);

			const Quad quad24(vertex52, vertex51, vertex60, vertex54);
			visitor.visit(quad24, recursion_context);

			const Quad quad25(vertex54, vertex60, vertex58, vertex56);
			visitor.visit(quad25, recursion_context);

			const Quad quad26(vertex46, vertex56, vertex58, vertex47);
			visitor.visit(quad26, recursion_context);

			const Quad quad27(vertex47, vertex58, vertex41, vertex31);
			visitor.visit(quad27, recursion_context);

			const Quad quad28(vertex41, vertex58, vertex50, vertex33);
			visitor.visit(quad28, recursion_context);

			const Quad quad29(vertex50, vertex58, vertex60, vertex51);
			visitor.visit(quad29, recursion_context);
		}


		template <class VisitorType, typename RecursionContextType>
		void
		RhombicTriacontahedronTraversal::Quad::visit_children(
				VisitorType &visitor,
				RecursionContextType &recursion_context) const
		{
			const UnitVector3D centre(normalise(Vector3D(vertex0) + Vector3D(vertex1) + Vector3D(vertex2) + Vector3D(vertex3)));
			const UnitVector3D edge_midpoint01(normalise(Vector3D(vertex0) + Vector3D(vertex1)));
			const UnitVector3D edge_midpoint12(normalise(Vector3D(vertex1) + Vector3D(vertex2)));
			const UnitVector3D edge_midpoint23(normalise(Vector3D(vertex2) + Vector3D(vertex3)));
			const UnitVector3D edge_midpoint30(normalise(Vector3D(vertex3) + Vector3D(vertex0)));

			const Quad quad0(vertex0, edge_midpoint01, centre, edge_midpoint30);
			visitor.visit(quad0, recursion_context);

			const Quad quad1(edge_midpoint01, vertex1, edge_midpoint12, centre);
			visitor.visit(quad1, recursion_context);

			const Quad quad2(centre, edge_midpoint12, vertex2, edge_midpoint23);
			visitor.visit(quad2, recursion_context);

			const Quad quad3(edge_midpoint30, centre, edge_midpoint23, vertex3);
			visitor.visit(quad3, recursion_context);
		}
	}
}

#endif // GPLATES_MATHS_SPHERICALSUBDIVISION_H
