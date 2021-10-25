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

#ifndef GPLATES_MATHS_POLYGONFAN_H
#define GPLATES_MATHS_POLYGONFAN_H

#include <boost/optional.hpp>

#include "MultiPointOnSphere.h"
#include "PolygonOnSphere.h"
#include "PolylineOnSphere.h"
#include "UnitVector3D.h"

#include "utils/ReferenceCount.h"


namespace GPlatesMaths
{
	/**
	 * A triangular fan mesh with apex at the centroid of a polygon.
	 *
	 * Can also be generated from a polyline by closing the gap between the first and last vertices.
	 *
	 * Can also be generated from a multipoint by considering the order of points to form the
	 * concave circumference of a polygon.
	 *
	 * This differs from @a PolygonMesh which contains mesh triangles *only* in the interior fill region
	 * of the polygon. @a PolygonFan, on the other hand, can have overlapping triangles and triangles
	 * *outside* the interior region of the polygon if the polygon is concave.
	 *
	 * This can be used when rendering interior fill region masks (the same fill region you'd get
	 * if you used the more expensive @a PolygonMesh) by rendering all triangles and using
	 * graphics hardware stencil buffer to invert each pixel's stencil mask each time pixel is rendered.
	 */
	class PolygonFan :
			public GPlatesUtils::ReferenceCount<PolygonFan>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a PolygonFan.
		typedef GPlatesUtils::non_null_intrusive_ptr<PolygonFan> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a PolygonFan.
		typedef GPlatesUtils::non_null_intrusive_ptr<const PolygonFan> non_null_ptr_to_const_type;


		/**
		 * A fan mesh triangle.
		 *
		 * Contains three vertex indices into the vertex array returned by 
		 */
		class Triangle
		{
		public:
			/**
			 * Returns the index into the array of mesh vertices.
			 *
			 * The returned index is used to look up a vertex in the array returned
			 * by "PolygonFan::get_vertices()".
			 *
			 * @param triangle_vertex_index is either 0, 1 or 2.
			 */
			unsigned int
			get_mesh_vertex_index(
					unsigned int triangle_vertex_index) const
			{
				return d_vertex_indices[triangle_vertex_index];
			}

		private:
			unsigned int d_vertex_indices[3];

			// NOTE: Constructor does not initialise.
			Triangle()
			{  }

			friend class PolygonFan;
		};


		//! A fan mesh vertex.
		class Vertex
		{
		public:
			//! Returns the vertex.
			const UnitVector3D &
			get_vertex() const
			{
				return d_vertex;
			}

		private:
			UnitVector3D d_vertex;

			explicit
			Vertex(
					const UnitVector3D &vertex) :
				d_vertex(vertex)
			{  }

			friend class PolygonFan;
		};


		/**
		 * Creates a @a PolygonFan object from a @a PolygonOnSphere.
		 *
		 * If the polygon has any interior rings then they are also added to the fan as
		 * separated rings.
		 */
		static
		non_null_ptr_to_const_type
		create(
				const PolygonOnSphere::non_null_ptr_to_const_type &polygon);


		/**
		 * Creates a @a PolygonFan object from a @a PolylineOnSphere.
		 *
		 * A polygon is formed from the polyline by joining the first and last vertices.
		 *
		 * Returns boost::none if less than three vertices (required to form a polygon).
		 */
		static
		boost::optional<non_null_ptr_to_const_type>
		create(
				const PolylineOnSphere::non_null_ptr_to_const_type &polyline);


		/**
		 * Creates a @a PolygonFan object from a @a MultiPointOnSphere.
		 *
		 * A polygon is formed from the multipoint by treating the order of points in the multipoint
		 * as the vertices of a polygon.
		 *
		 * Returns boost::none if less than three vertices (required to form a polygon).
		 */
		static
		boost::optional<non_null_ptr_to_const_type>
		create(
				const MultiPointOnSphere::non_null_ptr_to_const_type &multi_point);


		/**
		 * Creates a @a PolygonFan object from a @a GeometryOnSphere.
		 *
		 * Returns boost::none if less than three vertices (required to form a polygon).
		 *
		 * Note that @a PointOnSphere is the only @a GeometryOnSphere derivation not handled.
		 * Because can't have a polygon fan mesh with only a single point.
		 */
		static
		boost::optional<non_null_ptr_to_const_type>
		create(
				const GeometryOnSphere::non_null_ptr_to_const_type &geometry_on_sphere);


		/**
		 * Returns the sequence of triangles that form the polygon fan mesh.
		 *
		 * The vertex indices in each triangles index into the vertex array returned by @a get_vertices.
		 */
		const std::vector<Triangle> &
		get_triangles() const
		{
			return d_triangles;
		}


		/**
		 * Returns the sequence of vertices indexed by the triangles in the polygon fan mesh.
		 */
		const std::vector<Vertex> &
		get_vertices() const
		{
			return d_vertices;
		}
		
	private:
		/**
		 * The fan mesh triangles.
		 */
		std::vector<Triangle> d_triangles;

		/**
		 * The fan mesh vertices.
		 */
		std::vector<Vertex> d_vertices;


		//! Default constructor starts off with no triangles or vertices.
		PolygonFan()
		{  }


		/**
		 * Adds a fan ring to this fan mesh using the specified range of points as the ring boundary.
		 */
		template <typename PointOnSphereForwardIter>
		void
		add_fan_ring(
				const PointOnSphereForwardIter ring_points_begin,
				const unsigned int num_ring_points,
				const GPlatesMaths::UnitVector3D &centroid);
	};
}

#endif // GPLATES_MATHS_POLYGONFAN_H
