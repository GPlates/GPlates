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

#include <list>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include "GreatCircleArc.h"
#include "PointOnSphere.h"
#include "PolygonOnSphere.h"
#include "PolylineOnSphere.h"


namespace GPlatesMaths {

	namespace PolylineIntersections {

		/**
		 * Contains the results of intersecting two geometries in a form where
		 * the resulting partitioned polylines from each geometry can be traversed and queried.
		 */
		class Graph :
				public boost::noncopyable
		{
		public:
			class Intersection;

			//! Typedef for pointer to const @a Intersection.
			typedef boost::shared_ptr<const Intersection>
					intersection_ptr_to_const_type;
			//! Typedef for pointer to non-const @a Intersection.
			typedef boost::shared_ptr<Intersection>
					intersection_ptr_type;
			//! Typedef for weak pointer to const @a Intersection.
			typedef boost::weak_ptr<const Intersection>
					intersection_weak_ptr_to_const_type;

			//! Typedef for sequence of pointers to const @a Intersection.
			typedef std::vector< intersection_ptr_to_const_type >
					intersection_ptr_to_const_seq_type;
			//! Typedef for sequence of pointers to non-const @a Intersection.
			typedef std::vector< intersection_ptr_type >
					intersection_ptr_seq_type;

			class PartitionedPolyline;

			//! Typedef for pointer to const @a PartitionedPolyline.
			typedef boost::shared_ptr<const PartitionedPolyline>
					partitioned_polyline_ptr_to_const_type;
			//! Typedef for pointer to non-const @a PartitionedPolyline.
			typedef boost::shared_ptr<PartitionedPolyline>
					partitioned_polyline_ptr_type;
			//! Typedef for weak pointer to const @a PartitionedPolyline.
			typedef boost::weak_ptr<const PartitionedPolyline>
					partitioned_polyline_weak_ptr_to_const_type;

			//! Typedef for sequence of pointers to const @a PartitionedPolyline.
			typedef std::vector< partitioned_polyline_ptr_to_const_type >
					partitioned_polyline_ptr_to_const_seq_type;
			//! Typedef for sequence of pointers to non-const @a PartitionedPolyline.
			typedef std::vector< partitioned_polyline_ptr_type >
					partitioned_polyline_ptr_seq_type;

			/**
			 * A section of one of the two original intersected geometries;
			 */
			class PartitionedPolyline :
					public boost::noncopyable
			{
			public:
				PartitionedPolyline(
						PolylineOnSphere::non_null_ptr_to_const_type polyline_,
						bool is_overlapping_);

				/**
				 * The actual partitioned polyline geometry.
				 */
				PolylineOnSphere::non_null_ptr_to_const_type polyline;

				/**
				 * This is true if this partitioned polyline is a section of the
				 * two original geometries where their linear extents overlap -
				 * in which case this is where the graph of partitioned polylines
				 * converges to a polyline section rather than just an intersection point.
				 * For example the ascii art looks like:
				 *
				 *    \/                  \/
				 *     |    rather than   /\
				 *     |
				 *    /\
				 *
				 * ...where there are one or more contiguous overlapping polylines.
				 * Each overlapping polyline will be bounded by an @a Intersection
				 * as is the case for all partitioned polylines.
				 */
				bool is_overlapping;

				/**
				 * The previous intersection if there is one.
				 *
				 * If NULL or false then there's no previous intersection;
				 * meaning this polyline is the first partitioned polyline
				 * in one of the original geometries; although it's still
				 * possible for the first polyline to start with a non-NULL
				 * intersection (for example a T-junction).
				 *
				 * The weak_ptr is to avoid memory leaks caused by shared_ptr cycles.
				 */
				intersection_weak_ptr_to_const_type prev_intersection;

				//! A convenience function for returning a shared_ptr instead of a weak_ptr.
				intersection_ptr_to_const_type
				get_prev_intersection() const
				{
					return prev_intersection.lock();
				}

				/**
				 * The next intersection if there is one.
				 *
				 * If NULL or false then there's no next intersection;
				 * meaning this polyline is the last partitioned polyline
				 * in one of the original geometries; although it's still
				 * possible for the last polyline to end with a non-NULL
				 * intersection (for example a T-junction).
				 *
				 * The weak_ptr is to avoid memory leaks caused by shared_ptr cycles.
				 */
				intersection_weak_ptr_to_const_type next_intersection;

				//! A convenience function for returning a shared_ptr instead of a weak_ptr.
				intersection_ptr_to_const_type
				get_next_intersection() const
				{
					return next_intersection.lock();
				}
			};

			/**
			 * A point of intersection of the two original intersected geometries;
			 */
			class Intersection :
					public boost::noncopyable
			{
			public:
				Intersection(
						const PointOnSphere &intersection_point_);

				/**
				 * The point of intersection.
				 */
				PointOnSphere intersection_point;

				/**
				 * The previous partitioned polyline from the first original geometry.
				 * If NULL or false then this intersection is a T-junction.
				 *
				 * The weak_ptr is to avoid memory leaks caused by shared_ptr cycles.
				 */
				partitioned_polyline_weak_ptr_to_const_type prev_partitioned_polyline1;

				//! A convenience function for returning a shared_ptr instead of a weak_ptr.
				partitioned_polyline_ptr_to_const_type
				get_prev_partitioned_polyline1() const
				{
					return prev_partitioned_polyline1.lock();
				}

				/**
				 * The next partitioned polyline from the first original geometry.
				 * If NULL or false then this intersection is a T-junction.
				 *
				 * The weak_ptr is to avoid memory leaks caused by shared_ptr cycles.
				 */
				partitioned_polyline_weak_ptr_to_const_type next_partitioned_polyline1;

				//! A convenience function for returning a shared_ptr instead of a weak_ptr.
				partitioned_polyline_ptr_to_const_type
				get_next_partitioned_polyline1() const
				{
					return next_partitioned_polyline1.lock();
				}

				/**
				 * The previous partitioned polyline from the second original geometry.
				 * If NULL or false then this intersection is a T-junction.
				 *
				 * The weak_ptr is to avoid memory leaks caused by shared_ptr cycles.
				 */
				partitioned_polyline_weak_ptr_to_const_type prev_partitioned_polyline2;

				//! A convenience function for returning a shared_ptr instead of a weak_ptr.
				partitioned_polyline_ptr_to_const_type
				get_prev_partitioned_polyline2() const
				{
					return prev_partitioned_polyline2.lock();
				}

				/**
				 * The next partitioned polyline from the second original geometry.
				 * If NULL or false then this intersection is a T-junction.
				 *
				 * The weak_ptr is to avoid memory leaks caused by shared_ptr cycles.
				 */
				partitioned_polyline_weak_ptr_to_const_type next_partitioned_polyline2;

				//! A convenience function for returning a shared_ptr instead of a weak_ptr.
				partitioned_polyline_ptr_to_const_type
				get_next_partitioned_polyline2() const
				{
					return next_partitioned_polyline2.lock();
				}
			};


			Graph(
					const intersection_ptr_seq_type &intersections_,
					const partitioned_polyline_ptr_seq_type &partitioned_polylines1_,
					const partitioned_polyline_ptr_seq_type &partitioned_polylines2_);

			/**
			 * The intersection points.
			 *
			 * These points are not necessarily ordered in any particular way
			 * - this is just a sequence storage container.
			 */
			intersection_ptr_to_const_seq_type intersections;

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
		 * Find all points of intersection (with one exception; see
		 * below) of @a polyline1 and @a polyline2, and append them to
		 * 'intersections' in the returned @a Graph object; partition
		 * @a polyline1 and @a polyline2 at these points of intersection,
		 * and store these new, non-intersecting polylines in
		 * 'partitioned_polylines1' and 'partitioned_polylines2' in the
		 * returned @a Graph object; return false if no intersections found.
		 *
		 * The "one exception" mentioned above is this:  If the
		 * polylines touch endpoint-to-endpoint, this will not be
		 * counted as an intersection.
		 *
		 * Other points of note:
		 *
		 *  - If no intersections are found then false is returned
		 *    (the returned boost shared pointer can be tested like a boolean).
		 *    The point here is that the returned @a Graph object
		 *    does not contain the list of non-intersecting polylines
		 *    (which would mean that @a polyline1 and @a polyline2
		 *    would be stored in it if they were found not to
		 *    intersect); rather, the returned @a Graph object is defined
		 *    as the list of polylines obtained by partitioning
		 *    @a polyline1 and @a polyline2 if they are found to intersect.
		 *
		 *  - It is assumed that neither @a polyline1 nor @a polyline2
		 *    is self-intersecting.  That should be handled elsewhere.
		 *    No guarantees are made about what will happen if either
		 *    of the polylines @em is self-intersecting.
		 *
		 *  - Bearing in mind the earlier point about self-intersecting
		 *    polylines, if there are line-segments of the polylines
		 *    which are overlapping (that is, the line-segments share a
		 *    linear extent of non-zero length rather than the usual
		 *    point-like intersection), the shared extent will be
		 *    partitioned into a single line-segment.  Also, @em two
		 *    points will be appended to the list of intersection points
		 *    -- the two endpoints of the shared extent.
		 *
		 *  - Bearing in mind the earlier point about self-intersecting
		 *    polylines, if this function is passed the same polyline
		 *    as both polyline arguments, there shouldn't be any
		 *    problems -- the function will simply think it has been
		 *    given two polylines which overlap at every line-segment,
		 *    and will partition the polyline at every vertex.
		 *
		 *  - This function is strongly exception-safe (that is, if it
		 *    terminates due to an exception, program state will remain
		 *    unchanged) and exception-neutral (that is, any exceptions
		 *    are propagated to the caller). The strong exception-safe
		 *    property is a result of the fact that this function has
		 *    no non-const arguments (the results are returned in the
		 *    return parameter).
		 */
		boost::shared_ptr<const Graph>
		partition_intersecting_geometries(
		 const PolylineOnSphere &polyline1,
		 const PolylineOnSphere &polyline2);


		/**
		 * Another overload of @a partition_intersecting_geometries;
		 * intersects two polygons.
		 *
		 * The polygons are treated as if they were polylines - that is
		 * you can think of them as polylines that just happens to have
		 * there endpoints coincident. So no special treatment of the fact
		 * that they are polygons is taken into account.
		 *
		 * See the overload of @a partition_intersecting_geometries taking
		 * two polylines arguments for more details. The exception regarding
		 * the two geometries touching endpoint-to-endpoint mentioned there
		 * applies here too.
		 */
		boost::shared_ptr<const Graph>
		partition_intersecting_geometries(
		 const PolygonOnSphere &polygon1,
		 const PolygonOnSphere &polygon2);


		/**
		 * Another overload of @a partition_intersecting_geometries;
		 * intersects a polyline and a polygon.
		 *
		 * The polygon is treated as if it was a polyline - that is
		 * you can think of it as a polyline that just happens to have
		 * its endpoints coincident. So no special treatment of the fact
		 * that it is a polygon is taken into account.
		 *
		 * See the overload of @a partition_intersecting_geometries taking
		 * two polylines arguments for more details. The exception regarding
		 * the two geometries touching endpoint-to-endpoint mentioned there
		 * applies here too.
		 */
		boost::shared_ptr<const Graph>
		partition_intersecting_geometries(
		 const PolylineOnSphere &polyline,
		 const PolygonOnSphere &polygon);


		/**
		 * Another overload of @a partition_intersecting_geometries;
		 * intersects a polygon and a polyline.
		 *
		 * The polygons are treated as if they were polylines - that is
		 * you can think of them as polylines that just happen to have
		 * there endpoints coincident. So no special treatment of the fact
		 * that they are polygons is taken into account.
		 *
		 * See the overload of @a partition_intersecting_geometries taking
		 * two polylines arguments for more details. The exception regarding
		 * the two geometries touching endpoint-to-endpoint mentioned there
		 * applies here too.
		 */
		boost::shared_ptr<const Graph>
		partition_intersecting_geometries(
		 const PolygonOnSphere &polygon,
		 const PolylineOnSphere &polyline);


		/**
		 * Evaluate and return whether or not @a polyline_set is
		 * self-intersecting.
		 *
		 * All points of pointlike intersection will be appended to
		 * @a intersection_points; all segments of linear overlap will
		 * be appended to @a overlap_segments.
		 *
		 * Note that, in contrast to the function
		 * @a partition_intersecting_polylines, this function will not
		 * append the endpoints of @a overlap_segments to
		 * @a intersection_points.
		 *
		 * Other points of note:
		 *
		 *  - It is assumed that individual polylines are never
		 *    self-intersecting.  That should be handled elsewhere. 
		 *    This function tests whether any polyline in the set
		 *    intersects with any other.
		 *
		 *  - If polylines touch endpoint-to-endpoint-only, this will
		 *    not be counted as an intersection.
		 *
		 *  - This function does not assume that either non-const list
		 *    is empty; it will simply append items to the end of the
		 *    list.  (In particular, this function will not attempt to
		 *    ensure that items in either list are unique within that
		 *    list.  That should be handled elsewhere.)
		 *
		 *  - This function @em will assume that the polylines in
		 *    @a polyline_set are unique.  No guarantees are made about
		 *    what will happen if there @em are duplicates in the set.
		 *
		 *  - This function is strongly exception-safe (that is, if it
		 *    terminates due to an exception, program state will remain
		 *    unchanged) and exception-neutral (that is, any exceptions
		 *    are propagated to the caller).
		 */
		bool
		polyline_set_is_self_intersecting(
		 const std::list< PolylineOnSphere::non_null_ptr_to_const_type > &polyline_set,
		 std::list< PointOnSphere > &intersection_points,
		 std::list< GreatCircleArc > &overlap_segments);

	}
}

#endif  // GPLATES_MATHS_POLYLINEINTERSECTIONS_H
