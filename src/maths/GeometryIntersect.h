/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#ifndef GPLATES_MATHS_GEOMETRYINTERSECT_H
#define GPLATES_MATHS_GEOMETRYINTERSECT_H

#include <vector>

#include "PointOnSphere.h"
#include "PolygonOnSphere.h"
#include "PolylineOnSphere.h"


namespace GPlatesMaths
{
	namespace GeometryIntersect
	{
		/**
		 * Location of an intersection between two geometries.
		 *
		 * Currently the geometry types can be polyline and polygon. Later this may get extended to
		 * points and multi-points if there's a need to know if points "touch" polylines/polygons,
		 * more likely the geometry distance functions (in GeometryDistance) could be used for that.
		 */
		class Intersection
		{
		public:

			/**
			 * Type of intersection, whether two great circle arc segments cross or touch.
			 *
			 * Note: In all cases an intersection is not *on* the *end* point of a segment.
			 *       A segment end point is instead recorded as the *start* point of the *next* segment.
			 *       For the last segment of *polylines* there is no next segment
			 *       (however for polygons the next segment is the first segment due to ring wraparound).
			 *       This means the last point in a *polyline* (ie, end point of last segment) is
			 *       actually recorded as the start point of the fictitious one-past-the-last segment.
			 *       So, in this case, care needs to be taken not to access outside the range of valid segments.
			 *       The reason for the fictitious one-past-the-last segment (similar to generic begin/end iterators)
			 *       is it provides a more intuitive logic for *vertex-touching* intersections.
			 */
			enum Type
			{
				// Both segments cross each other.
				//
				// Both segment indices will *not* be a fictitious one-past-the-last segment.
				SEGMENTS_CROSS,

				// The start points of both segments coincide (but neither start point is on the other
				// segment's end point). The intersection position is arbitrarily chosen to be the
				// start point of the segment belonging to first geometry. Both start points are coincident
				// within a numerical threshold so the difference in position should be very small.
				//
				// Either/both segment indices *can* be a fictitious one-past-the-last segment.
				// This happens when either/both geometries intersect at the last vertex in their geometry(s).
				// Only applies to polylines (not polygons) as noted above.
				SEGMENT_START_ON_SEGMENT_START,

				// The start point of the segment belonging to first geometry lies *on* the
				// segment belonging to the second geometry (but not on its start or end point).
				// The intersection is the start point of the segment belonging to the first geometry.
				// It can be slightly off the segment belonging to the second geometry (within a numerical threshold).
				//
				// Only the segment index of the first geometry *can* be a fictitious one-past-the-last segment.
				// This happens when the last vertex in the first geometry touches any segment of the second geometry.
				// Only applies to polylines (not polygons) as noted above.
				SEGMENT1_START_ON_SEGMENT2,

				// The start point of the segment belonging to second geometry lies *on* the
				// segment belonging to the first geometry (but not on its start or end point).
				// The intersection is the start point of the segment belonging to the second geometry.
				// It can be slightly off the segment belonging to the first geometry (within a numerical threshold).
				//
				// Only the segment index of the second geometry *can* be a fictitious one-past-the-last segment.
				// This happens when the last vertex in the second geometry touches any segment of the first geometry.
				// Only applies to polylines (not polygons) as noted above.
				SEGMENT2_START_ON_SEGMENT1
			};

			Intersection(
					Type type_,
					const PointOnSphere &position_,
					int segment_index1_,
					int segment_index2_,
					double angle_in_segment1_ = 0.0,
					double angle_in_segment2_ = 0.0) :
				type(type_),
				position(position_),
				segment_index1(segment_index1_),
				segment_index2(segment_index2_),
				angle_in_segment1(angle_in_segment1_),
				angle_in_segment2(angle_in_segment2_)
			{  }


			Type type;
			PointOnSphere position;

			//
			// A segment index can be equal to the number of segments in the respective geometry.
			// In other words, it can be the fictitious one-past-the-last segment.
			//
			// In this case the value of 'type' will represent an intersection with the start of the
			// fictitious segment which actually means an intersection with the last point in the polyline.
			// Note that this only applies to polylines because, for polygons, the end point of the
			// last segment (in an exterior/interior ring) is also the start point of that ring, and hence
			// an intersection is recorded at the start point of the *first* segment (index zero).
			//

			//! Segment index within the first geometry.
			int segment_index1;
			//! Segment index within the second geometry.
			int segment_index2;

			//! Angle (radians) from segment start point to intersection along segment in first geometry.
			double angle_in_segment1;
			//! Angle (radians) from segment start point to intersection along segment in second geometry.
			double angle_in_segment2;
		};

		//! Typedef for a sequence of @a Intersection.
		typedef std::vector<Intersection> intersection_seq_type;


		/**
		 * Contains the results of intersecting two geometries.
		 *
		 * Currently the geometry types can be polyline and polygon. Later this may get extended to
		 * points and multi-points if there's a need to know if points "touch" polylines/polygons,
		 * more likely the geometry distance functions (in GeometryDistance) could be used for that.
		 */
		class Graph
		{
		public:

			/**
			 * The *unordered* intersections.
			 *
			 * These points are not necessarily ordered in any particular way
			 * - this is just a sequence storage container.
			 */
			intersection_seq_type unordered_intersections;

			//
			// The intersections *ordered* along each original geometry.
			//
			// There is one sequence for each of the two original geometries.
			// Each sequence is ordered such that the first intersection is closest to the beginning
			// of the respective geometry (and last intersection closed to the end).
			//
			// Each integer indexes into @a unordered_intersections, and the number of intersections
			// in each sequence matches the number in @a unordered_intersections.
			//
			std::vector<unsigned int> geometry1_ordered_intersections;
			std::vector<unsigned int> geometry2_ordered_intersections;
		};


		//
		// The following functions find all points of intersection between two geometries.
		//
		// An extremely small threshold is used to achieve robustness in the presence of finite
		// numerical precision. As such it can also detect when a start or end point of a segment
		// of one geometry *touches* another segment (see @a Intersection).
		//
		// There are essentially 9 types of segment-segment intersections...
		//
		//             ^        ^        ^
		//             |        |        |
		//             |        |        |
		//             |        |        |
		//     o------>o    o---o--->    o------->
		//
		//         ES          LS           SS
		//
		//
		//             ^        ^        ^
		//             |        |        |
		//     o------>|    o---+--->    o------->
		//             |        |        |
		//             o        o        o
		//
		//         EL          LL           SL
		//
		//
		//     o------>^    o---^--->    ^o------->
		//             |        |        |
		//             |        |        |
		//             |        |        |
		//             o        o        o
		//
		//         EE          LE           SE
		//
		// ...where 'o' represents the start point of a segment and '>' or '^' represent the end point of a segment.
		// Each diagram above has a 2-letter code where each letter can be 'S' for start point of segment,
		// 'E' for end point of segment or 'L' for line (or middle) part of segment (between start/end points).
		// The first letter is for the first geometry's segment, and the second for the second geometry's segment.
		// However, as noted in @a Intersection, we reduce the number of intersection types from 9 to 4 types.
		// So the only types of intersection are LL, SS, LS and SL. In other words, we've removed any types
		// involving an *end* point of a segment as these are equivalent to the *start* point of the next segment.
		//
		// However, for the last segment of *polylines* there is no next segment (for polygons the next segment
		// is the first segment due to ring wraparound). This means the last point in a *polyline* (ie, end point
		// of last segment) is actually recorded as the start point of the fictitious one-past-the-last segment.
		// So, in this case, care needs to be taken not to access outside the range of valid segments.
		// The reason for the fictitious one-past-the-last segment (similar to generic begin/end iterators)
		// and for reducing the number of intersection types from 9 to 4 is:
		//  - enables a more intuitive logic for the *vertex-touching* intersections (SS, LS and SL), and
		//  - avoids duplicate intersections (at end of one segment and start of next segment; which are the same point).
		//
		// Note that overlapping segments are also handled by the 4 intersection types (SS, SS, LS and SL).
		// Some overlap examples include...
		//
		//     -->o-------->   -->o------>     o--------->     -->o---->o--->
		//     -->o----->o->   <-----o<---     -->o-->o-->     <--o<----o<---
		//
		// ...that generate the following respective intersections...
		//
		//       SS and LS      SL and LS     SL, LS and LS       SS and SS
		//
		// ...and the same diagrams but only showing those segments that contribute to the above intersections...
		//
		//        o-------->      o----->      o--------->        o---->o--->
		//        o----->o->   <-----o         -->o-->o-->     <--o<----o
		//
		//            =             =               =                 =
		//
		//        o-------->      o----->      o--------->        o---->
		//        o----->      <-----o         -->             <--o
		//       SS              SL  LS       SL                  SS
		//
		//            +                             +                 +
		//
		//        o-------->                   o--------->              o--->
		//               o->                      o-->             <----o
		//              LS                       LS                    SS
		//
		//                                          +
		//
		//                                     o--------->
		//                                            o-->
		//                                           LS
		//
		// ...note that in some cases more than one intersection can be generated per segment pair being tested,
		// such as the second diagram above ("SL and LS").
		//
		// The following functions can be used when you need to know which vertices in the partitioned
		// sections (between intersections) are associated with which vertices in the original geometries.
		// For example, if tracking a quantity (such as a scalar value or velocity vector) at each
		// vertex of the original geometry, then these should be correctly associated with vertices
		// in the partitioned sections (by using segment/vertex indices).
		//
		// The following functions are also used by higher-level intersection code such as PolylineIntersections
		// to do the crucial work of finding intersections (but the partitioned polyline geometries
		// returned by PolylineIntersections lose knowledge of which vertices in the partitioned polylines
		// are associated with which vertices in the original unpartitioned polylines since there is no need
		// to track scalar values (for example) at vertices in that usage scenario).
		//


		/**
		 * Find all points of intersection of @a polyline1 and @a polyline2, and store them in the
		 * returned @a Graph object.
		 *
		 * Returns false if no intersections are found (in which case the returned graph is empty).
		 */
		bool
		intersect(
				Graph &intersection_graph,
				const PolylineOnSphere &polyline1,
				const PolylineOnSphere &polyline2);


		/**
		 * Find all points of intersection of @a polygon1 and @a polygon2, and store them in the
		 * returned @a Graph object.
		 *
		 * If polygon interior rings are included (the default) then intersections are searched in
		 * both the exterior ring and any interior rings (otherwise just the exterior ring).
		 * In either case the segment indices in @a Intersection can be used with 'PolygonOnSphere::get_segment()'.
		 *
		 * Returns false if no intersections are found (in which case the returned graph is empty).
		 */
		bool
		intersect(
				Graph &intersection_graph,
				const PolygonOnSphere &polygon1,
				const PolygonOnSphere &polygon2,
				bool include_polygon1_interior_rings = true,
				bool include_polygon2_interior_rings = true);


		/**
		 * Find all points of intersection of @a polyline and @a polygon, and store them in the
		 * returned @a Graph object.
		 *
		 * If polygon interior rings are included (the default) then intersections are searched in
		 * both the exterior ring and any interior rings (otherwise just the exterior ring).
		 * In either case the segment indices in @a Intersection can be used with 'PolygonOnSphere::get_segment()'.
		 *
		 * Returns false if no intersections are found (in which case the returned graph is empty).
		 */
		bool
		intersect(
				Graph &intersection_graph,
				const PolylineOnSphere &polyline,
				const PolygonOnSphere &polygon,
				bool include_polygon_interior_rings = true);


		/**
		 * Find all points of intersection of @a polygon and @a polyline, and store them in the
		 * returned @a Graph object.
		 *
		 * If polygon interior rings are included (the default) then intersections are searched in
		 * both the exterior ring and any interior rings (otherwise just the exterior ring).
		 * In either case the segment indices in @a Intersection can be used with 'PolygonOnSphere::get_segment()'.
		 *
		 * Returns false if no intersections are found (in which case the returned graph is empty).
		 */
		bool
		intersect(
				Graph &intersection_graph,
				const PolygonOnSphere &polygon,
				const PolylineOnSphere &polyline,
				bool include_polygon_interior_rings = true);
	}
}

#endif // GPLATES_MATHS_GEOMETRYINTERSECT_H
