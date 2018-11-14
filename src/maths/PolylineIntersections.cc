/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2018 The University of Sydney, Australia
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

#include <boost/optional.hpp>
#include <utility>
#include <vector>

#include "PolylineIntersections.h"

#include "GeometryIntersect.h"


namespace GPlatesMaths
{
	namespace PolylineIntersections
	{
		/**
		 * Clear the graph.
		 */
		void
		clear(
				Graph &graph)
		{
			graph.unordered_intersections.clear();
			graph.geometry1_ordered_intersections.clear();
			graph.geometry2_ordered_intersections.clear();
			graph.partitioned_polylines1.clear();
			graph.partitioned_polylines2.clear();
		}


		/**
		 * Create and add intersections using the intersection graph.
		 */
		void
		add_intersections(
				Graph &graph,
				const GeometryIntersect::Graph &intersection_graph)
		{
			const unsigned int num_intersections = intersection_graph.unordered_intersections.size();

			// Create and add the unordered intersections using the intersection graph.
			graph.unordered_intersections.reserve(num_intersections);
			for (unsigned int u = 0; u < num_intersections; ++u)
			{
				const GeometryIntersect::Intersection &intersection = intersection_graph.unordered_intersections[u];

				graph.unordered_intersections.push_back(
						Intersection::create(intersection.position));
			}

			// Add the geometry-ordered intersections that reference the above unordered intersections.
			graph.geometry1_ordered_intersections.reserve(num_intersections);
			graph.geometry2_ordered_intersections.reserve(num_intersections);
			for (unsigned int i = 0; i < num_intersections; ++i)
			{
				graph.geometry1_ordered_intersections.push_back(
						graph.unordered_intersections[
								intersection_graph.geometry1_ordered_intersections[i]]);
				graph.geometry2_ordered_intersections.push_back(
						graph.unordered_intersections[
								intersection_graph.geometry2_ordered_intersections[i]]);
			}
		}


		/**
		 * Create a polyline partitioned between two intersections, or
		 * between start of geometry and the end intersection (if no start intersection), or
		 * between the start intersection and end of geometry (if no end intersection).
		 */
		template <typename VertexIteratorType>
		PolylineOnSphere::non_null_ptr_to_const_type
		create_partitioned_polyline(
				const VertexIteratorType vertices_begin,
				const VertexIteratorType vertices_end,
				const unsigned int num_segments,
				const unsigned int GeometryIntersect::Intersection::*segment_index_ptr,
				const AngularDistance GeometryIntersect::Intersection::*angle_in_segment_ptr,
				// This is either SEGMENT1_START_ON_SEGMENT2 or SEGMENT2_START_ON_SEGMENT1 depending
				// on whether we're handling the first or second geometry...
				const GeometryIntersect::Intersection::Type enum_segment_starts_on_other_segment,
				boost::optional<const GeometryIntersect::Intersection &> start_intersection = boost::none,
				boost::optional<const GeometryIntersect::Intersection &> end_intersection = boost::none)
		{
			// The first vertex index of original geometry that gets copied into partitioned polyline.
			unsigned int start_vertex_index;
			if (start_intersection)
			{
				// Start at the end point of the segment containing the start intersection
				// (the "+1" increments from start of segment to end of segment, which is also start of next segment).
				//
				// GeometryIntersect guarantees an intersection point will not be recorded at the *end* point
				// of a segment since that would instead be recorded as the *start* point of the *next* segment
				// (which can be the fictitious one-past-the-last segment).
				// So our start intersection point can never replace the end point of the segment it intersects.
				start_vertex_index = start_intersection.get().*segment_index_ptr + 1;

				// However, if we only have a start intersection (and no end intersection) on the end point of the
				// last segment (which is actually recorded as the start of the fictitious one-past-the-last segment)
				// then we need to copy the last point in the original geometry, so that we have two points for
				// our partitioned polyline (the start intersection and last point in the original geometry).
				if (!end_intersection)
				{
					if (start_vertex_index == num_segments + 1)
					{
						if (start_intersection->type == GeometryIntersect::Intersection::SEGMENT_START_ON_SEGMENT_START ||
							start_intersection->type == enum_segment_starts_on_other_segment)
						{
							--start_vertex_index;  // 'num_segments' is the index of the last vertex.
						}
					}
				}
			}
			else
			{
				// There's no start intersection so start at the first vertex of the original geometry.
				start_vertex_index = 0;
			}

			// The end (one-past-the-last) vertex index of original geometry that gets copied into partitioned polyline.
			unsigned int end_vertex_index;
			if (end_intersection)
			{
				// End at the start point of the segment containing the end intersection
				// (the "+1" increments one-past-the-start of segment).
				end_vertex_index = end_intersection.get().*segment_index_ptr + 1;

				// GeometryIntersect guarantees an intersection point will not be recorded at the *end* point
				// of a segment since that would instead be recorded as the *start* point of the *next* segment
				// (which can be the fictitious one-past-the-last segment). But it can be recorded at the
				// *start* point of a segment. So our end intersection point *can* replace the start point
				// of the segment it intersects (this differs from the start intersection which can *never*
				// replace the end point of the segment it intersects).
				// If the end intersection does coincide with the start of a segment then we need to copy
				// one less point from the original geometry (that point being the start of the segment).
				//
				// If the end intersection point *is* at the start point of segment.
				if (end_intersection->type == GeometryIntersect::Intersection::SEGMENT_START_ON_SEGMENT_START ||
					end_intersection->type == enum_segment_starts_on_other_segment)
				{
					// Only copy one less point from original geometry if we have one to copy.
					if (start_intersection)
					{
						// It's possible the end intersection is on the same segment as the start intersection
						// (in which case both intersections must coincide with that segment's start point) and
						// so there are no original geometry points left to remove. In this case we'll just
						// be outputting the two coincident intersection points.
						if (end_vertex_index - start_vertex_index > 0)
						{
							--end_vertex_index;
						}
					}
					else // no start intersection (ie, start_vertex_index == 0)...
					{
						// It's possible the end intersection is on the first segment of original geometry
						// (and coincident with that segment's start point), in which case we need to copy
						// the start point from the original geometry (since we don't have a start intersection
						// and we need at least two points to create a polyline).
						if (end_vertex_index - start_vertex_index > 1)
						{
							--end_vertex_index;
						}
					}
				}
			}
			else
			{
				// There's no end intersection so end at the last vertex of the original geometry.
				//
				// Note that, for polygon rings, we actually consider the last vertex to be the ring
				// start vertex (since that's the end of the last segment in the polygon ring).
				// In other words the vertex iterator type should be treating the polygon ring
				// as if it was a polyline.
				//
				// Note that the end vertex index is one-past-the-last vertex (at index 'num_segments').
				end_vertex_index = num_segments + 1;
			}

			// The points that will form the partitioned polyline.
			std::vector<PointOnSphere> partitioned_polyline_points;
			// Reserve enough space for the original geometry points (that contribute) and
			// a maximum of two intersection points.
			partitioned_polyline_points.reserve(end_vertex_index - start_vertex_index + 2);

			if (start_intersection)
			{
				// Add the start intersection.
				partitioned_polyline_points.push_back(start_intersection->position);
			}

			// Copy the original geometry vertices, if any, between the two intersections
			// (or between start of geometry and end intersection,
			//  or between start intersection and end of geometry).
			if (end_vertex_index > start_vertex_index)
			{
				VertexIteratorType start_vertices_iter = vertices_begin;
				std::advance(start_vertices_iter, start_vertex_index);

				// Note that the end iterator is one-past-the-last vertex that gets copied.
				VertexIteratorType end_vertices_iter = vertices_begin;
				std::advance(end_vertices_iter, end_vertex_index);

				partitioned_polyline_points.insert(
						partitioned_polyline_points.end(),
						start_vertices_iter,
						end_vertices_iter);
			}

			if (end_intersection)
			{
				// Add the end intersection.
				partitioned_polyline_points.push_back(end_intersection->position);
			}

			// We should always have the minimum of two points to create a polyline because:
			// - If we have two intersections then each adds a point.
			// - If we only have a start intersection on the end point of the last segment
			//   (which is actually recorded as the start of the fictitious one-past-the-last segment)
			//   then the code above ensures we copy the last point in the original geometry, so we
			//   have the start intersection and last point in the original geometry.
			// - If we only have an end intersection on the start point of the first segment then the
			//   code above ensures we copy the first point in the original geometry, so we have the
			//   first point in the original geometry and the end intersection.
			// - If we have no intersections (which shouldn't happen) then we use the original geometry
			//   (which should have at least two points).
			return PolylineOnSphere::create_on_heap(partitioned_polyline_points);
		}


		/**
		 * Create and add partitioned polylines, for one of the two geometries, using the intersection graph.
		 */
		template <typename VertexIteratorType>
		void
		add_partitioned_polylines(
				const VertexIteratorType vertices_begin,
				const VertexIteratorType vertices_end,
				const unsigned int num_segments,
				Graph &graph,
				partitioned_polyline_ptr_to_const_seq_type Graph::*partitioned_polylines_ptr,
				const PartitionedPolyline * Intersection::*prev_partitioned_polyline_ptr,
				const PartitionedPolyline * Intersection::*next_partitioned_polyline_ptr,
				const GeometryIntersect::Graph &intersection_graph,
				const std::vector<unsigned int> GeometryIntersect::Graph::*geometry_intersections_ptr,
				const unsigned int GeometryIntersect::Intersection::*segment_index_ptr,
				const AngularDistance GeometryIntersect::Intersection::*angle_in_segment_ptr,
				// This is either SEGMENT1_START_ON_SEGMENT2 or SEGMENT2_START_ON_SEGMENT1 depending
				// on whether we're handling the first or second geometry...
				const GeometryIntersect::Intersection::Type enum_segment_starts_on_other_segment)
		{
			const unsigned int num_intersections = intersection_graph.unordered_intersections.size();

			// There is one more partitioned polyline than intersections (unless there are T-junctions).
			(graph.*partitioned_polylines_ptr).reserve(num_intersections + 1);

			//
			// Create the partitioned polylines prior to each intersection.
			//
			for (unsigned int i = 0; i < num_intersections; ++i)
			{
				const unsigned int intersection_index = (intersection_graph.*geometry_intersections_ptr)[i];

				const GeometryIntersect::Intersection &intersection = intersection_graph.unordered_intersections[intersection_index];
				// Cast away const since we need to modify the intersection.
				Intersection *partition_intersection = const_cast<Intersection *>(graph.unordered_intersections[intersection_index].get());

				// If there's a previous intersection along the current geometry then set its next partitioned polyline pointer.
				if (i != 0)
				{
					const unsigned int prev_intersection_index = (intersection_graph.*geometry_intersections_ptr)[i - 1];

					const GeometryIntersect::Intersection &prev_intersection = intersection_graph.unordered_intersections[prev_intersection_index];
					// Cast away const since we need to modify the intersection.
					Intersection *prev_partition_intersection = const_cast<Intersection *>(graph.unordered_intersections[prev_intersection_index].get());

					PartitionedPolyline::non_null_ptr_type partitioned_polyline = PartitionedPolyline::create(
							create_partitioned_polyline(
									vertices_begin,
									vertices_end,
									num_segments,
									segment_index_ptr,
									angle_in_segment_ptr,
									enum_segment_starts_on_other_segment,
									prev_intersection/*start_intersection*/,
									intersection/*end_intersection*/));
					(graph.*partitioned_polylines_ptr).push_back(partitioned_polyline);

					// Link together the current partitioned polyline and the current intersection.
					partition_intersection->*prev_partitioned_polyline_ptr = partitioned_polyline.get();
					partitioned_polyline->next_intersection = partition_intersection;

					// Link together the current partitioned polyline and the previous intersection.
					prev_partition_intersection->*next_partitioned_polyline_ptr = partitioned_polyline.get();
					partitioned_polyline->prev_intersection = prev_partition_intersection;
				}
				else // i == 0 ...
				{
					// If the intersection is at the start of the first segment then it's a T-junction and
					// we don't need to create a previous partitioned polyline (can leave pointer as NULL).
					if (intersection.*segment_index_ptr == 0)
					{
						if (intersection.type == GeometryIntersect::Intersection::SEGMENT_START_ON_SEGMENT_START ||
							intersection.type == enum_segment_starts_on_other_segment)
						{
							continue;
						}
					}

					PartitionedPolyline::non_null_ptr_type first_partitioned_polyline = PartitionedPolyline::create(
							create_partitioned_polyline(
									vertices_begin,
									vertices_end,
									num_segments,
									segment_index_ptr,
									angle_in_segment_ptr,
									enum_segment_starts_on_other_segment,
									boost::none/*start_intersection*/,
									intersection/*end_intersection*/));
					(graph.*partitioned_polylines_ptr).push_back(first_partitioned_polyline);

					// Link together the first partitioned polyline and the first intersection.
					partition_intersection->*prev_partitioned_polyline_ptr = first_partitioned_polyline.get();
					first_partitioned_polyline->next_intersection = partition_intersection;
				}
			}

			//
			// Create the last partitioned polyline (after the last intersection).
			//

			const unsigned int last_intersection_index = (intersection_graph.*geometry_intersections_ptr)[num_intersections - 1];

			const GeometryIntersect::Intersection &last_intersection = intersection_graph.unordered_intersections[last_intersection_index];
			// Cast away const since we need to modify the intersection.
			Intersection *last_partition_intersection = const_cast<Intersection *>(graph.unordered_intersections[last_intersection_index].get());

			// If the intersection is at the end of the last segment then it's a T-junction and
			// we don't need to create a next partitioned polyline (can leave pointer as NULL).
			//
			// Note that the end of the last segment is recorded as the start of the fictitious
			// one-past-the-last segment. This only applies to polylines since, for polygon rings,
			// there actually is a one-past-the-last segment (it's the first segment). So for polygons
			// we'll never get a segment index equal to 'num_segments' (and hence never return early here).
			if (last_intersection.*segment_index_ptr == num_segments)
			{
				if (last_intersection.type == GeometryIntersect::Intersection::SEGMENT_START_ON_SEGMENT_START ||
					last_intersection.type == enum_segment_starts_on_other_segment)
				{
					return;
				}
			}

			PartitionedPolyline::non_null_ptr_type last_partitioned_polyline = PartitionedPolyline::create(
					create_partitioned_polyline(
							vertices_begin,
							vertices_end,
							num_segments,
							segment_index_ptr,
							angle_in_segment_ptr,
							enum_segment_starts_on_other_segment,
							last_intersection/*start_intersection*/,
							boost::none/*end_intersection*/));
			(graph.*partitioned_polylines_ptr).push_back(last_partitioned_polyline);

			// Link together the last partitioned polyline and the last intersection.
			last_partition_intersection->*next_partitioned_polyline_ptr = last_partitioned_polyline.get();
			last_partitioned_polyline->prev_intersection = last_partition_intersection;
		}


		/**
		 * Partition two polyline/polygon geometries.
		 */
		template <typename VertexIterator1Type, typename VertexIterator2Type>
		void
		partition_geometries(
				Graph &graph,
				const GeometryIntersect::Graph &intersection_graph,
				const VertexIterator1Type vertices1_begin,
				const VertexIterator1Type vertices1_end,
				const unsigned int num_segments1,
				const VertexIterator2Type vertices2_begin,
				const VertexIterator2Type vertices2_end,
				const unsigned int num_segments2)
		{
			// Create and add the intersections using the intersection graph.
			add_intersections(graph, intersection_graph);

			// Create and add the partitioned polylines for each geometry using the intersection graph.
			add_partitioned_polylines(
					vertices1_begin,
					vertices1_end,
					num_segments1,
					graph,
					&Graph::partitioned_polylines1,
					&Intersection::prev_partitioned_polyline1,
					&Intersection::next_partitioned_polyline1,
					intersection_graph,
					&GeometryIntersect::Graph::geometry1_ordered_intersections,
					&GeometryIntersect::Intersection::segment_index1,
					&GeometryIntersect::Intersection::angle_in_segment1,
					GeometryIntersect::Intersection::SEGMENT1_START_ON_SEGMENT2);
			add_partitioned_polylines(
					vertices2_begin,
					vertices2_end,
					num_segments2,
					graph,
					&Graph::partitioned_polylines2,
					&Intersection::prev_partitioned_polyline2,
					&Intersection::next_partitioned_polyline2,
					intersection_graph,
					&GeometryIntersect::Graph::geometry2_ordered_intersections,
					&GeometryIntersect::Intersection::segment_index2,
					&GeometryIntersect::Intersection::angle_in_segment2,
					GeometryIntersect::Intersection::SEGMENT2_START_ON_SEGMENT1);
		}
	}
}


bool
GPlatesMaths::PolylineIntersections::partition(
		Graph &graph,
		const PolylineOnSphere &polyline1,
		const PolylineOnSphere &polyline2)
{
	// Make sure we start with an empty graph.
	clear(graph);

	GeometryIntersect::Graph intersection_graph;
	if (!GeometryIntersect::intersect(
			intersection_graph,
			polyline1,
			polyline2))
	{
		return false;
	}

	partition_geometries(
			graph,
			intersection_graph,
			polyline1.vertex_begin(),
			polyline1.vertex_end(),
			polyline1.number_of_segments(),
			polyline2.vertex_begin(),
			polyline2.vertex_end(),
			polyline2.number_of_segments());

	return true;
}


bool
GPlatesMaths::PolylineIntersections::partition(
		Graph &graph,
		const PolygonOnSphere &polygon1,
		const PolygonOnSphere &polygon2)
{
	// Make sure we start with an empty graph.
	clear(graph);

	//
	// Note: We only intersect polygon exterior rings.
	//       And we treat polygon rings as if they were polylines which means we use an iterator that
	//       iterates over one extra vertex, compared to the usual ring vertex iterator, and hence the
	//       last vertex is the end point of the last ring segment (which is also the first vertex of ring).
	//
	// TODO: Remove polygon overloads of this function when proper polygon intersections
	//       are implemented (on top of GeometryIntersect).
	//

	GeometryIntersect::Graph intersection_graph;
	if (!GeometryIntersect::intersect(
			intersection_graph,
			polygon1,
			polygon2,
			false/*include_polygon1_interior_rings*/,
			false/*include_polygon2_interior_rings*/))
	{
		return false;
	}

	partition_geometries(
			graph,
			intersection_graph,
			// Vertices in exterior ring as if it was a polyline...
			polygon1.exterior_polyline_vertex_begin(),
			polygon1.exterior_polyline_vertex_end(),
			polygon1.number_of_segments_in_exterior_ring(),
			// Vertices in exterior ring as if it was a polyline...
			polygon2.exterior_polyline_vertex_begin(),
			polygon2.exterior_polyline_vertex_end(),
			polygon2.number_of_segments_in_exterior_ring());

	return true;
}


bool
GPlatesMaths::PolylineIntersections::partition(
		Graph &graph,
		const PolylineOnSphere &polyline,
		const PolygonOnSphere &polygon)
{
	// Make sure we start with an empty graph.
	clear(graph);

	//
	// Note: We only intersect polygon exterior rings.
	//       And we treat polygon rings as if they were polylines which means we use an iterator that
	//       iterates over one extra vertex, compared to the usual ring vertex iterator, and hence the
	//       last vertex is the end point of the last ring segment (which is also the first vertex of ring).
	//
	// TODO: Remove polygon overloads of this function when proper polygon intersections
	//       are implemented (on top of GeometryIntersect).
	//

	GeometryIntersect::Graph intersection_graph;
	if (!GeometryIntersect::intersect(
			intersection_graph,
			polyline,
			polygon,
			false/*include_polygon_interior_rings*/))
	{
		return false;
	}

	partition_geometries(
			graph,
			intersection_graph,
			polyline.vertex_begin(),
			polyline.vertex_end(),
			polyline.number_of_segments(),
			// Vertices in exterior ring as if it was a polyline...
			polygon.exterior_polyline_vertex_begin(),
			polygon.exterior_polyline_vertex_end(),
			polygon.number_of_segments_in_exterior_ring());

	return true;
}


bool
GPlatesMaths::PolylineIntersections::partition(
		Graph &graph,
		const PolygonOnSphere &polygon,
		const PolylineOnSphere &polyline)
{
	// Make sure we start with an empty graph.
	clear(graph);

	//
	// Note: We only intersect polygon exterior rings.
	//       And we treat polygon rings as if they were polylines which means we use an iterator that
	//       iterates over one extra vertex, compared to the usual ring vertex iterator, and hence the
	//       last vertex is the end point of the last ring segment (which is also the first vertex of ring).
	//
	// TODO: Remove polygon overloads of this function when proper polygon intersections
	//       are implemented (on top of GeometryIntersect).
	//

	GeometryIntersect::Graph intersection_graph;
	if (!GeometryIntersect::intersect(
			intersection_graph,
			polygon,
			polyline,
			false/*include_polygon_interior_rings*/))
	{
		return false;
	}

	partition_geometries(
			graph,
			intersection_graph,
			// Vertices in exterior ring as if it was a polyline...
			polygon.exterior_polyline_vertex_begin(),
			polygon.exterior_polyline_vertex_end(),
			polygon.number_of_segments_in_exterior_ring(),
			polyline.vertex_begin(),
			polyline.vertex_end(),
			polyline.number_of_segments());

	return true;
}
