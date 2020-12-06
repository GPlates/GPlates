/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2020 The University of Sydney, Australia
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

#include <map>
#include <utility>  // std::pair
#include <boost/optional.hpp>

#include "GeometryCrossing.h"



namespace GPlatesMaths
{
	namespace GeometryCrossing
	{
		/**
		 * Typedef for mapping segment indices of geometry1 to intersections.
		 *
		 * Records any intersections that touch a vertex of either geometry by mapping the
		 * segment index of geometry1 (arbitrary) to the intersection index (in intersection graph).
		 * This will be used to find overlapping sections in both geometries.
		 */
		typedef std::multimap<
				unsigned int/*geometry1_segment_index*/,
				unsigned int/*intersection_index*/>
						vertex_intersection_map_type;


		/**
		 * Returns true if any intersections touch a vertex of either geometry.
		 *
		 * The most likely case is none of the intersections touch vertices.
		 */
		bool
		contains_vertex_intersections(
				const GeometryIntersect::Graph &intersection_graph)
		{
			for (const auto &intersection : intersection_graph.unordered_intersections)
			{
				if (intersection.type != GeometryIntersect::Intersection::SEGMENTS_CROSS)
				{
					return true;
				}
			}

			return false;
		}


		boost::optional<unsigned int/*intersection_index*/>
		find_vertex_crossing(
				const GeometryIntersect::Intersection &vertex_intersection,
				vertex_intersection_map_type &vertex_intersection_map,
				const GeometryIntersect::intersection_seq_type &intersections)
		{
			return boost::none;
		}


		GPlatesMaths::GeometryIntersect::Graph
		find_vertex_crossings(
				const GeometryIntersect::Graph &intersection_graph)
		{
			GeometryIntersect::Graph crossing_graph;

			// Map all intersection graph intersections (that we retain) to intersections in the crossing graph.
			//
			// Segment-segment crossings are trivial - they definitely a *crossing* so they get added immediately.
			//
			// Vertex crossings are non-trivial (and additionally can include portion of both geometries that overlap)...
			// Only the start or end vertex of the overlap portion of both geometries is retained as an intersection
			// (and even it's not always retained if the overlap does not eventually result in a crossing).
			// Either way, all the vertices *inside* the overlapping portion (which are also intersections) are not retained.
			// So we need to track which vertex intersections are retained and where they end up in the crossing graph.
			typedef std::map<
					unsigned int/*intersection_graph_intersection_index*/,
					unsigned int/*crossing_graph_intersection_index*/>
							intersection_graph_to_crossing_graph_map_type;
			intersection_graph_to_crossing_graph_map_type intersection_graph_to_crossing_graph_map;

			// Record any intersections that touch a vertex of either geometry by mapping the segment index of
			// geometry1 (arbitrary) to the intersection index (in intersection graph).
			// This will be used to find overlapping sections in both geometries.
			vertex_intersection_map_type vertex_intersection_map;

			const GeometryIntersect::intersection_seq_type &intersections = intersection_graph.unordered_intersections;

			// Each intersection is either a segment-segment crossing (which gets immediately added to the crossing output) or part
			// of an overlap between the two geometries (and requires further processing to determine if it's a crossing or not).
			for (unsigned int intersection_index = 0; intersection_index < intersections.size(); ++intersection_index)
			{
				const GeometryIntersect::Intersection &intersection = intersections[intersection_index];

				if (intersection.type == GeometryIntersect::Intersection::SEGMENTS_CROSS)
				{
					// The intersection does not touch vertices so can simply add it to the crossing graph
					// (since both geometries definitely cross at this intersection).
					intersection_graph_to_crossing_graph_map[intersection_index] = crossing_graph.unordered_intersections.size();
					crossing_graph.unordered_intersections.push_back(intersection);
				}
				else
				{
					// Record which segment (of geometry1) the intersection came from.
					vertex_intersection_map.insert(
							std::make_pair(intersection.segment_index1, intersection_index));
				}
			}

			// Go through the vertex intersections and determine which ones are crossings.
			while (!vertex_intersection_map.empty())
			{
				// Extract the first vertex intersection in the map.
				auto vertex_intersection_index_iter = vertex_intersection_map.begin();
				const GeometryIntersect::Intersection &vertex_intersection = intersections[vertex_intersection_index_iter->second];
				vertex_intersection_map.erase(vertex_intersection_index_iter);

				// Starting at the extracted vertex intersection, find any geometry overlap and
				// find the vertex crossing (if any).
				boost::optional<unsigned int/*intersection_index*/> vertex_crossing =
						find_vertex_crossing(
								vertex_intersection,
								vertex_intersection_map,
								intersections);
				if (vertex_crossing)
				{
					const unsigned int vertex_crossing_intersection_index = vertex_crossing.get();
					const GeometryIntersect::Intersection &vertex_crossing_intersection = intersections[vertex_crossing_intersection_index];

					// Add intersection to crossing graph.
					intersection_graph_to_crossing_graph_map[vertex_crossing_intersection_index] =
							crossing_graph.unordered_intersections.size();
					crossing_graph.unordered_intersections.push_back(vertex_crossing_intersection);
				}
			}

			// Add the crossing graph intersection indices for geometry1.
			for (unsigned int geometry1_index : intersection_graph.geometry1_ordered_intersections)
			{
				auto crossing_graph_intersection_index_iter = intersection_graph_to_crossing_graph_map.find(geometry1_index);

				// Not all vertex intersections are retained. Only add those that are.
				if (crossing_graph_intersection_index_iter != intersection_graph_to_crossing_graph_map.end())
				{
					const unsigned int crossing_graph_intersection_index = crossing_graph_intersection_index_iter->second;
					crossing_graph.geometry1_ordered_intersections.push_back(crossing_graph_intersection_index);
				}
			}

			// Add the crossing graph intersection indices for geometry2.
			for (unsigned int geometry2_index : intersection_graph.geometry2_ordered_intersections)
			{
				auto crossing_graph_intersection_index_iter = intersection_graph_to_crossing_graph_map.find(geometry2_index);

				// Not all vertex intersections are retained. Only add those that are.
				if (crossing_graph_intersection_index_iter != intersection_graph_to_crossing_graph_map.end())
				{
					const unsigned int crossing_graph_intersection_index = crossing_graph_intersection_index_iter->second;
					crossing_graph.geometry2_ordered_intersections.push_back(crossing_graph_intersection_index);
				}
			}

			return crossing_graph;
		}
	}
}


GPlatesMaths::GeometryIntersect::Graph
GPlatesMaths::GeometryCrossing::find_crossings(
		GeometryIntersect::Graph &intersection_graph)
{
	if (!contains_vertex_intersections(intersection_graph))
	{
		// No vertex intersections, so all intersections are segment-segment crossings which are
		// definitely crossings, so nothing to do. Just return the intersection graph.
		return intersection_graph;
	}

	return find_vertex_crossings(intersection_graph);
}
