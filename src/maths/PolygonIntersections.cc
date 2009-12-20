/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include <vector>

#include "PolygonIntersections.h"

#include "GreatCircleArc.h"
#include "PointInPolygon.h"
#include "PolylineIntersections.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


namespace
{
	/**
	 * Sequential partitioned polylines that are inside and/or overlapping with the
	 * partitioning polygon's boundary can really be merged into a single polygon since
	 * we are classifying them all as inside; this class keeps track of this.
	 */
	class InsidePartitionedPolylineMerger
	{
	public:
		/**
		 * Construct with the list of polylines that are inside the partitioning polygon.
		 */
		InsidePartitionedPolylineMerger(
				GPlatesMaths::PolygonIntersections::partitioned_polyline_seq_type &inside_list) :
			d_inside_polyline_list(inside_list)
		{  }


		/**
		 * Add a polyline that's inside (or overlapping the boundary) of the
		 * partitioning polygon.
		 */
		void
		add_inside_polyline(
				const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type &inside_polyline)
		{
			d_inside_polylines.push_back(inside_polyline);
		}


		/**
		 * We've come to the end of a contiguous sequence of polylines that are
		 * inside (or overlapping the boundary) the partitioning polygon.
		 * So merge them into a single polyline and output that to the caller's inside list.
		 */
		void
		merge_inside_polylines_and_output()
		{
			if (d_inside_polylines.empty())
			{
				return;
			}

			if (d_inside_polylines.size() == 1)
			{
				// There's only one polyline so just output it and return.
				d_inside_polyline_list.push_back(d_inside_polylines[0]);
				// Clear for the next merge.
				d_inside_polylines.clear();
				return;
			}

			// Get the number of merged points.
			inside_polyline_seq_type::size_type num_merged_polyline_points = 0;
			inside_polyline_seq_type::const_iterator merged_iter = d_inside_polylines.begin();
			inside_polyline_seq_type::const_iterator merged_end = d_inside_polylines.end();
			for ( ; merged_iter != merged_end; ++merged_iter)
			{
				num_merged_polyline_points += (*merged_iter)->number_of_vertices();
			}

			// Merge all the points into one sequence.
			std::vector<GPlatesMaths::PointOnSphere> merged_polyline_points;
			merged_polyline_points.reserve(num_merged_polyline_points);
			merged_iter = d_inside_polylines.begin();
			for ( ; merged_iter != merged_end; ++merged_iter)
			{
				const GPlatesMaths::PolylineOnSphere &polyline = **merged_iter;
				merged_polyline_points.insert(
						merged_polyline_points.end(),
						polyline.vertex_begin(),
						polyline.vertex_end());
			}

			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type merged_polyline =
					GPlatesMaths::PolylineOnSphere::create_on_heap(
							merged_polyline_points.begin(), merged_polyline_points.end());

			// Add to the caller's inside polyline sequence.
			d_inside_polyline_list.push_back(merged_polyline);

			// Clear for the next merge.
			d_inside_polylines.clear();
		}

	private:
		typedef std::vector<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>
				inside_polyline_seq_type;

		inside_polyline_seq_type d_inside_polylines;
		GPlatesMaths::PolygonIntersections::partitioned_polyline_seq_type &d_inside_polyline_list;
	};


	bool
	do_adjacent_great_circle_arcs_bend_left(
			const GPlatesMaths::GreatCircleArc &polygon_prev_gca,
			const GPlatesMaths::GreatCircleArc &polygon_next_gca)
	{
		// Unless the two GCAs are parallel there they will form a smaller acute angle
		// on one side and a larger obtuse angle on the other side.
		// If the acute angle is to the left (meaning the next GCA bends to the left
		// relative to the previous GCA when following along the vertices) then the
		// cross product vector of the GCAs will be in the same hemisphere as the
		// intersection point (where the two GCAs meet) otherwise it will be in the
		// opposite hemisphere.
		// If the GCAs are parallel or nearly parallel then the cross product will be
		// zero or close to it and the comparison with zero will be error-prone but this
		// is ok since parallel GCAs mean we don't really care whether the acute angle
		// is to the left or right since both angles are close to 180 degrees and
		// it won't really affect calculations down the line.
		return dot(
				polygon_next_gca.start_point().position_vector(),
				cross(polygon_prev_gca.rotation_axis(), polygon_next_gca.rotation_axis()))
						> 0;
	}
}


GPlatesMaths::PolygonIntersections::PolygonIntersections(
		const PolygonOnSphere::non_null_ptr_to_const_type &partitioning_polygon) :
	d_partitioning_polygon(partitioning_polygon),
	d_point_in_polygon_tester(
			PointInPolygon::create_optimised_polygon(partitioning_polygon)),
	d_partitioning_polygon_orientation(
			PolygonOrientation::calculate_polygon_orientation(*partitioning_polygon))
{
}


GPlatesMaths::PolygonIntersections::Result
GPlatesMaths::PolygonIntersections::partition_polyline(
		const PolylineOnSphere::non_null_ptr_to_const_type &polyline_to_be_partitioned,
		partitioned_polyline_seq_type &partitioned_polylines_inside,
		partitioned_polyline_seq_type &partitioned_polylines_outside)
{
	// Partition the geometry to be partitioned against the partitioning polygon.
	// NOTE: The first argument is the partitioning polygon - this means it corresponds
	// to the first sequence in the returned graph.
	boost::shared_ptr<const PolylineIntersections::Graph> partitioned_polylines_graph =
			PolylineIntersections::partition_intersecting_geometries(
					*d_partitioning_polygon, *polyline_to_be_partitioned);

	// If there were no intersections then the polyline to be partitioned must be either
	// fully inside or fully outside the partitioning polygon - find out which.
	if (!partitioned_polylines_graph)
	{
		// Choose any two points on the polyline and to see if it's inside the
		// partitioning polygon. Any points will do. Pick the first and second points.
		// It's a polyline so we know it has at least two vertices.
		Result result = partition_polyline_or_polygon_fully_inside_or_outside(
				*polyline_to_be_partitioned->vertex_begin(),
				*++polyline_to_be_partitioned->vertex_begin());

		if (result == GEOMETRY_OUTSIDE)
		{
			partitioned_polylines_outside.push_back(polyline_to_be_partitioned);
		}
		else
		{
			// Count intersecting as inside even though it shouldn't have intersected
			// otherwise we'd have an intersection graph.
			partitioned_polylines_inside.push_back(polyline_to_be_partitioned);
			result = GEOMETRY_INSIDE;
		}
		return result;
	}

	// Determine which partitioned polylines are inside/outside the partitioning polygon
	// and add to the appropriate lists.
	partition_geometry(
			*partitioned_polylines_graph,
			partitioned_polylines_inside,
			partitioned_polylines_outside);

	// There were intersections.
	return GEOMETRY_INTERSECTING;
}


GPlatesMaths::PolygonIntersections::Result
GPlatesMaths::PolygonIntersections::partition_polygon(
		const PolygonOnSphere::non_null_ptr_to_const_type &polygon_to_be_partitioned,
		partitioned_polyline_seq_type &partitioned_polylines_inside,
		partitioned_polyline_seq_type &partitioned_polylines_outside)
{
	// Partition the geometry to be partitioned against the partitioning polygon.
	// NOTE: The first argument is the partitioning polygon - this means it corresponds
	// to the first sequence in the returned graph.
	boost::shared_ptr<const PolylineIntersections::Graph> partitioned_polylines_graph =
			PolylineIntersections::partition_intersecting_geometries(
					*d_partitioning_polygon, *polygon_to_be_partitioned);

	// If there were no intersections then the polygon to be partitioned must be either
	// fully inside or fully outside the partitioning polygon - find out which.
	if (!partitioned_polylines_graph)
	{
		// Choose any two points on the polygon and to see if it's inside the
		// partitioning polygon. Any points will do. Pick the first and second points.
		// It's a polygon so we know it has at least three vertices.
		const Result result = partition_polyline_or_polygon_fully_inside_or_outside(
				*polygon_to_be_partitioned->vertex_begin(),
				*++polygon_to_be_partitioned->vertex_begin());

		// Count intersecting (in this case just touching) as inside even though
		// it shouldn't have intersected otherwise we'd have an intersection graph.
		return (result == GEOMETRY_OUTSIDE) ? GEOMETRY_OUTSIDE : GEOMETRY_INSIDE;
	}

	// Determine which partitioned polylines are inside/outside the partitioning polygon
	// and add to the appropriate lists.
	partition_geometry(
			*partitioned_polylines_graph,
			partitioned_polylines_inside,
			partitioned_polylines_outside);

	// There were intersections.
	return GEOMETRY_INTERSECTING;
}


GPlatesMaths::PolygonIntersections::Result
GPlatesMaths::PolygonIntersections::partition_point(
		const PointOnSphere &point_to_be_partitioned)
{
	const GPlatesMaths::PointInPolygon::Result point_in_polygon_result =
			GPlatesMaths::PointInPolygon::test_point_in_polygon(
					point_to_be_partitioned,
					d_point_in_polygon_tester);

	switch (point_in_polygon_result)
	{
	case PointInPolygon::POINT_OUTSIDE_POLYGON:
	default:
		return GEOMETRY_OUTSIDE;
	case PointInPolygon::POINT_INSIDE_POLYGON:
		return GEOMETRY_INSIDE;
	case PointInPolygon::POINT_ON_POLYGON:
		return GEOMETRY_INTERSECTING;
	}

	// Shouldn't get here.
	return GEOMETRY_OUTSIDE;
}


GPlatesMaths::PolygonIntersections::Result
GPlatesMaths::PolygonIntersections::partition_multipoint(
		const MultiPointOnSphere &multipoint_to_be_partitioned,
		partitioned_point_seq_type &partitioned_points_inside,
		partitioned_point_seq_type &partitioned_points_outside)
{
	bool any_inside_points = false;
	bool any_outside_points = false;
	bool any_intersecting_points = false;

	// Iterate over the points in the multipoint and test each one.
	MultiPointOnSphere::const_iterator multipoint_iter = multipoint_to_be_partitioned.begin();
	MultiPointOnSphere::const_iterator multipoint_end = multipoint_to_be_partitioned.end();
	for ( ; multipoint_iter != multipoint_end; ++multipoint_iter)
	{
		const PointOnSphere &point = *multipoint_iter;

		const Result point_result = partition_point(point);
		switch (point_result)
		{
		case GEOMETRY_OUTSIDE:
		default:
			partitioned_points_outside.push_back(point);
			any_outside_points = true;
			break;

		case GEOMETRY_INSIDE:
			partitioned_points_inside.push_back(point);
			any_inside_points = true;
			break;

		case GEOMETRY_INTERSECTING:
			// Classify points on boundary as inside.
			partitioned_points_inside.push_back(point);
			any_intersecting_points = true;
			break;
		}
	}

	// If there were any points on the boundary or there are points inside
	// and outside then classify that as intersecting.
	if (any_intersecting_points ||
			(any_inside_points && any_outside_points))
	{
		return GEOMETRY_INTERSECTING;
	}

	// Only one of inside or outside lists has any points in it.

	if (any_inside_points)
	{
		return GEOMETRY_INSIDE;
	}

	// No intersecting points, no inside points so only outside points remain.
	// It might be possible (although I don't think so) to have a multipoint with
	// no points in it - in which case returning outside is appropriate.
	return GEOMETRY_OUTSIDE;
}


GPlatesMaths::PolygonIntersections::Result
GPlatesMaths::PolygonIntersections::partition_polyline_or_polygon_fully_inside_or_outside(
		const GPlatesMaths::PointOnSphere &arbitrary_point_on_geometry1,
		const GPlatesMaths::PointOnSphere &arbitrary_point_on_geometry2)
{
	// Choose any point in the polygon and see if it's inside the
	// partitioning polygon. Any point will do - we choose the start point.
	// Note: We don't add the polyline to either inside/outside list
	// because that is used only if an intersection occurred.
	Result result = partition_point(arbitrary_point_on_geometry1);

	// Determine which caller's partitioned polygon list to append to.
	if (result == GEOMETRY_INTERSECTING)
	{
		// The first vertex touched the partitioning polygon.
		// This should have been picked up by PolylineIntersections so we
		// probably wouldn't get here but just in case test another point
		// - of course the next point might also be touching in which case
		// we might be returning an incorrect result (unlikely though).
		result = partition_point(arbitrary_point_on_geometry2);
	}

	return result;
}


void
GPlatesMaths::PolygonIntersections::partition_geometry(
		const GPlatesMaths::PolylineIntersections::Graph &partitioned_polylines_graph,
		partitioned_polyline_seq_type &partitioned_polylines_inside,
		partitioned_polyline_seq_type &partitioned_polylines_outside)
{
	InsidePartitionedPolylineMerger inside_partitioned_polyline_merger(
			partitioned_polylines_inside);

	// Iterate over the partitioned polylines of the geometry being partitioned.
	// NOTE: The geometry that was partitioned is the second sequence in the graph.
	PolylineIntersections::Graph::partitioned_polyline_ptr_to_const_seq_type::const_iterator
			partitioned_polyline_iter = partitioned_polylines_graph.partitioned_polylines2.begin();
	PolylineIntersections::Graph::partitioned_polyline_ptr_to_const_seq_type::const_iterator
			partitioned_polyline_end = partitioned_polylines_graph.partitioned_polylines2.end();
	for ( ; partitioned_polyline_iter != partitioned_polyline_end; ++partitioned_polyline_iter)
	{
		const PolylineIntersections::Graph::partitioned_polyline_ptr_to_const_type &
				partitioned_poly = *partitioned_polyline_iter;

		if (partitioned_poly->is_overlapping)
		{
			// If the partitioned polyline overlaps with the partitioning polygon
			// then we classify that as inside the polygon.
			inside_partitioned_polyline_merger.add_inside_polyline(
					partitioned_poly->polyline);
			continue;
		}

		const GreatCircleArc *polyline_gca = NULL;
		bool reverse_inside_outside = false;

		PolylineIntersections::Graph::intersection_ptr_to_const_type intersection =
				partitioned_poly->prev_intersection;
		if (intersection)
		{
			// Get the first GCA of the current partitioned polyline - the GCA
			// that touches the intersection point.
			polyline_gca = &*partitioned_poly->polyline->begin();
		}
		else // no previous intersection...
		{
			// We must be the first polyline of the sequence and
			// it doesn't start at a T-junction.
			// Use the intersection at the end of the partitioned polyline instead.
			intersection = partitioned_poly->next_intersection;

			// It's not possible for a partitioned polyline to
			// have no intersections at either end.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					intersection, GPLATES_ASSERTION_SOURCE);

			// Get the last GCA of the current partitioned polyline - the GCA
			// that touches the intersection point.
			polyline_gca = &*--partitioned_poly->polyline->end();

			// The end of 'polyline_gca' is now touching the intersection point
			// instead of the beginning touching it - so we have to reverse the
			// interpretation of the inside/outside test.
			reverse_inside_outside = true;
		}

		// Get the partitioning polyline just prior to the intersection point.
		// NOTE: The partitioning polygon is the first sequence in the graph.
		const GreatCircleArc &polygon_prev_gca = intersection->prev_partitioned_polyline1
				// Get the last GCA of the polyline (of the partitioning polygon) just
				// prior to the intersection point - this GCA touches the intersection point.
				? *--intersection->prev_partitioned_polyline1->polyline->end()
				// There's no previous polyline in the partitioning polygon so the
				// intersection point must coincide with the polygon's start point.
				// Use the last GCA of the last polyline in the partitioning polygon instead.
				: *--partitioned_polylines_graph.partitioned_polylines1.back()->polyline->end();

		// Get the partitioning polyline just after the intersection point.
		// NOTE: The partitioning polygon is the first sequence in the graph.
		const GreatCircleArc &polygon_next_gca = intersection->next_partitioned_polyline1
				// Get the first GCA of the polyline (of the partitioning polygon) just
				// after the intersection point - this GCA touches the intersection point.
				? *intersection->next_partitioned_polyline1->polyline->begin()
				// There's no next polyline in the partitioning polygon so the
				// intersection point must coincide with the polygon's end point.
				// Use the first GCA of the first polyline in the partitioning polygon instead.
				: *partitioned_polylines_graph.partitioned_polylines1.front()->polyline->begin();

		// Determine if the current partitioned polyline is inside or outside
		// by seeing if its chosen GCA is inside or outside.
		const bool is_partitioned_poly_inside = reverse_inside_outside ^
				is_gca_inside_partitioning_polygon(
						*polyline_gca, polygon_prev_gca, polygon_next_gca);

		if (is_partitioned_poly_inside)
		{
			// Add inside polyline to the merger instead of the caller's inside list.
			inside_partitioned_polyline_merger.add_inside_polyline(
					partitioned_poly->polyline);
		}
		else
		{
			// Add to the list of outside polylines.
			partitioned_polylines_outside.push_back(partitioned_poly->polyline);

			// We've come across an outside polyline so merge any inside polylines
			// we've accumulated so far and append resulting polyline to the
			// caller's inside list.
			inside_partitioned_polyline_merger.merge_inside_polylines_and_output();
		}
	}

	// If there are any inside polylines accumulated then merge them and
	// append resulting polyline to the caller's inside list.
	inside_partitioned_polyline_merger.merge_inside_polylines_and_output();
}


bool
GPlatesMaths::PolygonIntersections::is_gca_inside_partitioning_polygon(
		const GreatCircleArc &polyline_gca,
		const GreatCircleArc &polygon_prev_gca,
		const GreatCircleArc &polygon_next_gca) const
{
	// It is assumed that 'polygon_prev_gca's end point equals
	// 'polygon_next_gca's start point.

	// When testing to see if 'polyline_gca' is to the left or right of the
	// two GCAs from the partitioning polygon the test needs to be done
	// to see if 'polyline_gca' falls in the acute angle region.
	// For example,
	//
	//    ^               ^
	//     \               \ A /
	// left \  right        \ /
	//       ^               ^
	//      /               / \
	//     /               / B \
	//
	// ...we can test for the left region by testing left of both arcs.
	// If we tried to test for the right region instead we would try testing
	// right of both arcs but that would miss regions A and B shown above
	// which are supposed to be part of the right region.
	if (do_adjacent_great_circle_arcs_bend_left(
			polygon_prev_gca, polygon_next_gca))
	{
		// If the adjacent arcs on the partitioning polygon bend to the left then
		// left is the narrowest region.
		// This region is inside the polygon if the polygon's vertices
		// are counter-clockwise and outside if they are clockwise.

		// See if the polyline GCA is in the narrow region to the left.
		if (do_adjacent_great_circle_arcs_bend_left(
				polygon_prev_gca, polyline_gca) &&
			do_adjacent_great_circle_arcs_bend_left(
				polygon_next_gca, polyline_gca))
		{
			// The polyline GCA is in the narrow region to the left.
			return d_partitioning_polygon_orientation == PolygonOrientation::COUNTERCLOCKWISE;
		}

		// The polyline GCA was not in the narrow region to the left
		// so invert the test for inside.
		return d_partitioning_polygon_orientation == PolygonOrientation::CLOCKWISE;
	}

	// Adjacent args on the partitioning polygon bend to the right making
	// it the narrowest region.
	// This region is inside the polygon if the polygon's vertices
	// are clockwise and outside if they are counter-clockwise.

	// See if the polyline GCA is in the narrow region to the right.
	// We do this by testing that the polyline GCA is not to the left
	// of either GCA of the partitioning polygon.
	if (!do_adjacent_great_circle_arcs_bend_left(
			polygon_prev_gca, polyline_gca) &&
		!do_adjacent_great_circle_arcs_bend_left(
			polygon_next_gca, polyline_gca))
	{
		// The polyline GCA is in the narrow region to the right.
		return d_partitioning_polygon_orientation == PolygonOrientation::CLOCKWISE;
	}

	// The polyline GCA was not in the narrow region to the right
	// so invert the test for inside.
	return d_partitioning_polygon_orientation == PolygonOrientation::COUNTERCLOCKWISE;
}
