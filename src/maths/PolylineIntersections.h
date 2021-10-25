/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2005, 2006, 2008 The University of Sydney, Australia
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

#ifndef GPLATES_MATHS_POLYLINEINTERSECTIONS_H
#define GPLATES_MATHS_POLYLINEINTERSECTIONS_H

#include <vector>

#include "PointOnSphere.h"
#include "PolygonOnSphere.h"
#include "PolylineOnSphere.h"

#include "utils/ReferenceCount.h"


namespace GPlatesMaths
{
	namespace PolylineIntersections
	{
		class Intersection;

		/**
		 * A section of one of the two original intersected geometries.
		 */
		class PartitionedPolyline :
				public GPlatesUtils::ReferenceCount<PartitionedPolyline>
		{
		public:

			typedef GPlatesUtils::non_null_intrusive_ptr<PartitionedPolyline> non_null_ptr_type;
			typedef GPlatesUtils::non_null_intrusive_ptr<const PartitionedPolyline> non_null_ptr_to_const_type;

			static
			non_null_ptr_type
			create(
					PolylineOnSphere::non_null_ptr_to_const_type polyline_)
			{
				return non_null_ptr_type(new PartitionedPolyline(polyline_));
			}


			/**
			 * The actual partitioned polyline geometry.
			 */
			PolylineOnSphere::non_null_ptr_to_const_type polyline;

			/**
			 * The previous intersection if there is one.
			 *
			 * If NULL then there's no previous intersection;
			 * meaning this polyline is the first partitioned polyline
			 * in one of the original geometries; although it's still
			 * possible for the first polyline to start with a non-NULL
			 * intersection (for example a T-junction).
			 */
			const Intersection *prev_intersection;

			/**
			 * The next intersection if there is one.
			 *
			 * If NULL then there's no next intersection;
			 * meaning this polyline is the last partitioned polyline
			 * in one of the original geometries; although it's still
			 * possible for the last polyline to end with a non-NULL
			 * intersection (for example a T-junction).
			 */
			const Intersection *next_intersection;

		private:
			PartitionedPolyline(
					PolylineOnSphere::non_null_ptr_to_const_type polyline_) :
				polyline(polyline_),
				prev_intersection(NULL),
				next_intersection(NULL)
			{  }
		};

		//! Typedef for sequence of pointers to const @a PartitionedPolyline.
		typedef std::vector<PartitionedPolyline::non_null_ptr_to_const_type> partitioned_polyline_ptr_to_const_seq_type;


		/**
		 * A point of intersection of the two original intersected geometries;
		 */
		class Intersection :
				public GPlatesUtils::ReferenceCount<Intersection>
		{
		public:

			typedef GPlatesUtils::non_null_intrusive_ptr<Intersection> non_null_ptr_type;
			typedef GPlatesUtils::non_null_intrusive_ptr<const Intersection> non_null_ptr_to_const_type;

			static
			non_null_ptr_type
			create(
					const PointOnSphere &intersection_point_)
			{
				return non_null_ptr_type(new Intersection(intersection_point_));
			}


			/**
			 * The point of intersection.
			 */
			PointOnSphere intersection_point;

			/**
			 * The previous partitioned polyline from the first original geometry.
			 *
			 * If NULL then this intersection is a T-junction.
			 */
			const PartitionedPolyline *prev_partitioned_polyline1;

			/**
			 * The next partitioned polyline from the first original geometry.
			 *
			 * If NULL then this intersection is a T-junction.
			 */
			const PartitionedPolyline *next_partitioned_polyline1;

			/**
			 * The previous partitioned polyline from the second original geometry.
			 *
			 * If NULL then this intersection is a T-junction.
			 */
			const PartitionedPolyline *prev_partitioned_polyline2;

			/**
			 * The next partitioned polyline from the second original geometry.
			 *
			 * If NULL then this intersection is a T-junction.
			 */
			const PartitionedPolyline *next_partitioned_polyline2;

		private:
			Intersection(
					const PointOnSphere &intersection_point_) :
				intersection_point(intersection_point_),
				prev_partitioned_polyline1(NULL),
				next_partitioned_polyline1(NULL),
				prev_partitioned_polyline2(NULL),
				next_partitioned_polyline2(NULL)
			{  }
		};

		//! Typedef for sequence of pointers to const @a Intersection.
		typedef std::vector<Intersection::non_null_ptr_to_const_type> intersection_ptr_to_const_seq_type;


		/**
		 * Contains the results of intersecting two geometries in a form where
		 * the resulting partitioned polylines from each geometry can be traversed and queried.
		 */
		class Graph
		{
		public:

			/**
			 * The *unordered* intersection points.
			 *
			 * These points are not necessarily ordered in any particular way
			 * - this is just a sequence storage container.
			 */
			intersection_ptr_to_const_seq_type unordered_intersections;


			/**
			 * The intersections *ordered* along the first original geometry (from its start to end).
			 */
			intersection_ptr_to_const_seq_type geometry1_ordered_intersections;

			/**
			 * The intersections *ordered* along the second original geometry (from its start to end).
			 */
			intersection_ptr_to_const_seq_type geometry2_ordered_intersections;


			/**
			 * The partitioned polylines belonging to the first original geometry.
			 *
			 * The order and orientation of the polylines is the same as for the
			 * original geometry (it just has extra polylines inserted at intersection
			 * points where necessary).
			 */
			partitioned_polyline_ptr_to_const_seq_type partitioned_polylines1;

			/**
			 * The partitioned polylines belonging to the second original geometry.
			 *
			 * The order and orientation of the polylines is the same as for the
			 * original geometry (it just has extra polylines inserted at intersection
			 * points where necessary).
			 */
			partitioned_polyline_ptr_to_const_seq_type partitioned_polylines2;
		};


		/**
		 * Find all points of intersection of @a polyline1 and @a polyline2, and store them in the
		 * returned @a Graph object; partition @a polyline1 and @a polyline2 at these points of
		 * intersection, and store these new, polylines in 'partitioned_polylines1' and
		 * 'partitioned_polylines2' in the returned @a Graph object;
		 * return false if no intersections found.
		 *
		 * Other points of note:
		 *
		 *  - If no intersections are found then false is returned and the graph is empty.
		 *
		 *  - If this function is passed the same polyline as both polyline arguments, there shouldn't
		 *    be any problems -- the function will simply think it has been given two polylines which
		 *    overlap at every line-segment, and will partition the polyline at every vertex.
		 *
		 *  - Each specified polyline (@a polyline1 and @a polyline2) can be self-intersecting.
		 *    However self-intersection points are not included in the graph
		 *    (unless they happen to coincide with a point where @a polyline1 intersects @a polyline2).
		 *    Only points where @a polyline1 intersects @a polyline2 are recorded as intersections.
		 *
		 *  - We no longer provide a flag to indicate along which partitions the two original
		 *    geometries overlap since this is tricky in the presence of self-interesting polylines.
		 *
		 *    For example...
		 *
		 *             ___              ___
		 *      A     |   |      A     |   |
		 *       \    |   /       \    |   /
		 *        ----+---   ->    x---y--z   +    x---y--z
		 *       /    |   \            |          /        \
		 *      B     A    B           A         B          B
		 *
		 *    ...where polyline A self-intersects and also intersects polyline B across the horizontal line
		 *    where both A and B overlap. However the order of intersections along each polyline differs.
		 *    Polyline A intersects B at x, z, y (in that order along A) in second graph.
		 *    Polyline B intersects A at x, y, z (in that order along B) in third graph.
		 *    So the horizontal overlap is one partition in A (section x->z) but two partitions in B
		 *    (sections x->y and y->z). So to record overlap we'd need to insert a non-intersection vertex
		 *    in A in the middle of its x->z section where y is because, as far as A is concerned, y is not
		 *    in section x->z (y is actually after z along A). And inserting non-intersection vertices
		 *    unnecessarily complicates things.
		 */
		bool
		partition(
				Graph &partition_graph,
				const PolylineOnSphere &polyline1,
				const PolylineOnSphere &polyline2);


		/**
		 * Another overload of @a partition; intersects two polygons.
		 *
		 * The polygons are treated as if they were polylines - that is you can think of them as polylines
		 * that just happen to have their endpoints coincident. So no special treatment of the fact
		 * that they are polygons is taken into account.
		 *
		 * See the overload of @a partition taking two polylines arguments for more details.
		 *
		 * TODO: Remove this when proper polygon intersections are implemented (on top of GeometryIntersect).
		 */
		bool
		partition(
				Graph &partition_graph,
				const PolygonOnSphere &polygon1,
				const PolygonOnSphere &polygon2);


		/**
		 * Another overload of @a partition; intersects a polyline and a polygon.
		 *
		 * The polygon is treated as if it was a polyline - that is you can think of it as a polyline
		 * that just happens to have its endpoints coincident. So no special treatment of the fact
		 * that it is a polygon is taken into account.
		 *
		 * See the overload of @a partition taking two polylines arguments for more details.
		 *
		 * TODO: Remove this when proper polygon intersections are implemented (on top of GeometryIntersect).
		 */
		bool
		partition(
				Graph &partition_graph,
				const PolylineOnSphere &polyline,
				const PolygonOnSphere &polygon);


		/**
		 * Another overload of @a partition; intersects a polygon and a polyline.
		 *
		 * The polygon is treated as if it was a polyline - that is you can think of it as a polyline
		 * that just happens to have its endpoints coincident. So no special treatment of the fact
		 * that it is a polygon is taken into account.
		 *
		 * See the overload of @a partition taking two polylines arguments for more details.
		 *
		 * TODO: Remove this when proper polygon intersections are implemented (on top of GeometryIntersect).
		 */
		bool
		partition(
				Graph &partition_graph,
				const PolygonOnSphere &polygon,
				const PolylineOnSphere &polyline);
	}
}

#endif  // GPLATES_MATHS_POLYLINEINTERSECTIONS_H
