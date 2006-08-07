/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef GPLATES_MATHS_POLYLINEINTERSECTIONS_H
#define GPLATES_MATHS_POLYLINEINTERSECTIONS_H

#include <list>
#include "PointOnSphere.h"
#include "PolyLineOnSphere.h"
#include "GreatCircleArc.h"

namespace GPlatesMaths {

	namespace PolylineIntersections {

		/**
		 * Find all points of intersection (with one exception; see
		 * below) of @a polyline1 and @a polyline2, and append them to
		 * @a intersection_points; partition @a polyline1 and
		 * @a polyline2 at these points of intersection, and append
		 * these new, non-intersecting polylines to
		 * @a partitioned_polylines; return the number of points of
		 * intersection found.
		 *
		 * The "one exception" mentioned above is this:  If the
		 * polylines touch endpoint-to-endpoint, this will not be
		 * counted as an intersection.
		 *
		 * Other points of note:
		 *
		 *  - If no intersections are found, nothing will be appended
		 *    to @a intersection_points or @a partitioned_polylines. 
		 *    The point here is that @a partitioned_polylines is
		 *    @em not defined as the list of non-intersecting polylines
		 *    (which would mean that @a polyline1 and @a polyline2
		 *    would be appended to it if they were found not to
		 *    intersect); rather, it is defined as the list of
		 *    polylines obtained by partitioning @a polyline1 and
		 *    @a polyline2 if they are found to intersect.
		 *
		 *  - It is assumed that neither @a polyline1 nor @a polyline2
		 *    is self-intersecting.  That should be handled elsewhere.
		 *    No guarantees are made about what will happen if either
		 *    of the polylines @em is self-intersecting.
		 *
		 *  - This function does not assume that either list is empty;
		 *    it will simply append items to the end of the list.  (In
		 *    particular, this function will not attempt to ensure that
		 *    items in either list are unique within that list.  That
		 *    should be handled elsewhere.)  The value returned will be
		 *    the number of items @em added to @a intersection_points
		 *    rather than the total number of items in
		 *    @a intersection_points.
		 *
		 *  - Bearing in mind the earlier point about self-intersecting
		 *    polylines, if there are line-segments of the polylines
		 *    which are overlapping (that is, the line-segments share a
		 *    linear extent of non-zero length rather than the usual
		 *    point-like intersection), the shared extent will be
		 *    partitioned into a single line-segment.  Also, @em two
		 *    points will be appended to @a intersection_points -- the
		 *    two endpoints of the shared extent.
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
		 *    are propagated to the caller).
		 */
		std::list< PointOnSphere >::size_type
		partition_intersecting_polylines(
		 const PolyLineOnSphere &polyline1,
		 const PolyLineOnSphere &polyline2,
		 std::list< PointOnSphere > &intersection_points,
		 std::list< PolyLineOnSphere > &partitioned_polylines);


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
		 const std::list< PolyLineOnSphere > &polyline_set,
		 std::list< PointOnSphere > &intersection_points,
		 std::list< GreatCircleArc > &overlap_segments);

	}

}

#endif  // GPLATES_MATHS_POLYLINEINTERSECTIONS_H
