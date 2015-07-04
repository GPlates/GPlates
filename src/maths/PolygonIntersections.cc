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

#include <algorithm>
#include <iterator>
#include <vector>
#include <boost/optional.hpp>
#include <QDebug>

#include "PolygonIntersections.h"

#include "GreatCircleArc.h"
#include "PointInPolygon.h"
#include "PolylineIntersections.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/ConstGeometryOnSphereVisitor.h"


namespace
{
	using GPlatesMaths::PolygonIntersections;
	using GPlatesMaths::GeometryOnSphere;
	using GPlatesMaths::MultiPointOnSphere;
	using GPlatesMaths::PointOnSphere;
	using GPlatesMaths::PolygonOnSphere;
	using GPlatesMaths::PolylineOnSphere;

	class GeometryPartitioner :
			public GPlatesMaths::ConstGeometryOnSphereVisitor
	{
	public:
		GeometryPartitioner(
				const PolygonIntersections &polygon_intersections,
				PolygonIntersections::partitioned_geometry_seq_type &partitioned_geometries_inside,
				PolygonIntersections::partitioned_geometry_seq_type &partitioned_geometries_outside) :
			d_polygon_intersections(polygon_intersections),
			d_partitioned_geometries_inside(partitioned_geometries_inside),
			d_partitioned_geometries_outside(partitioned_geometries_outside)
		{  }


		PolygonIntersections::Result
		partition_geometry(
				const GeometryOnSphere::non_null_ptr_to_const_type &geometry_to_be_partitioned)
		{
			d_result = PolygonIntersections::GEOMETRY_OUTSIDE;

			geometry_to_be_partitioned->accept_visitor(*this);

			return d_result;
		}

	protected:
		virtual
		void
		visit_multi_point_on_sphere(
				MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
		{
			boost::optional<MultiPointOnSphere::non_null_ptr_to_const_type>
					partitioned_multipoint_inside,
					partitioned_multipoint_outside;
			d_result = d_polygon_intersections.partition_multipoint(
					multi_point_on_sphere,
					partitioned_multipoint_inside,
					partitioned_multipoint_outside);

			if (partitioned_multipoint_inside)
			{
				d_partitioned_geometries_inside.push_back(*partitioned_multipoint_inside);
			}
			if (partitioned_multipoint_outside)
			{
				d_partitioned_geometries_outside.push_back(*partitioned_multipoint_outside);
			}
		}

		virtual
		void
		visit_point_on_sphere(
				PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
		{
			d_result = d_polygon_intersections.partition_point(*point_on_sphere);

			if (d_result == PolygonIntersections::GEOMETRY_OUTSIDE)
			{
				d_partitioned_geometries_outside.push_back(point_on_sphere);
			}
			else
			{
				d_partitioned_geometries_inside.push_back(point_on_sphere);
			}
		}

		virtual
		void
		visit_polygon_on_sphere(
				PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
		{
			PolygonIntersections::partitioned_polyline_seq_type partitioned_polylines_inside;
			PolygonIntersections::partitioned_polyline_seq_type partitioned_polylines_outside;
			d_result = d_polygon_intersections.partition_polygon(
					polygon_on_sphere,
					partitioned_polylines_inside,
					partitioned_polylines_outside);

			if (d_result == PolygonIntersections::GEOMETRY_INSIDE)
			{
				d_partitioned_geometries_inside.push_back(polygon_on_sphere);
			}
			else if (d_result == PolygonIntersections::GEOMETRY_OUTSIDE)
			{
				d_partitioned_geometries_outside.push_back(polygon_on_sphere);
			}

			std::copy(partitioned_polylines_inside.begin(), partitioned_polylines_inside.end(),
					std::back_inserter(d_partitioned_geometries_inside));
			std::copy(partitioned_polylines_outside.begin(), partitioned_polylines_outside.end(),
					std::back_inserter(d_partitioned_geometries_outside));
		}

		virtual
		void
		visit_polyline_on_sphere(
				PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
		{
			PolygonIntersections::partitioned_polyline_seq_type partitioned_polylines_inside;
			PolygonIntersections::partitioned_polyline_seq_type partitioned_polylines_outside;
			d_result = d_polygon_intersections.partition_polyline(
					polyline_on_sphere,
					partitioned_polylines_inside,
					partitioned_polylines_outside);

			std::copy(partitioned_polylines_inside.begin(), partitioned_polylines_inside.end(),
					std::back_inserter(d_partitioned_geometries_inside));
			std::copy(partitioned_polylines_outside.begin(), partitioned_polylines_outside.end(),
					std::back_inserter(d_partitioned_geometries_outside));
		}

	private:
		const PolygonIntersections &d_polygon_intersections;

		PolygonIntersections::Result d_result;
		PolygonIntersections::partitioned_geometry_seq_type &
				d_partitioned_geometries_inside;
		PolygonIntersections::partitioned_geometry_seq_type &
				d_partitioned_geometries_outside;
	};


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


	/**
	 * Get first (or last) non-zero length GCA of polyline.
	 *
	 * Returns none if polyline has only zero length GCA's (ie, if polyline is coincident with a point).
	 */
	boost::optional<const GPlatesMaths::GreatCircleArc &>
	get_first_or_last_non_zero_great_circle_arc(
			const GPlatesMaths::PolylineOnSphere &polyline,
			bool get_first)
	{
		if (get_first)
		{
			// Get the first (non-zero length) GCA of the polyline.
			GPlatesMaths::PolylineOnSphere::const_iterator gca_iter = polyline.begin();
			do
			{
				if (!gca_iter->is_zero_length())
				{
					return *gca_iter;
				}

				++gca_iter;
			}
			while (gca_iter != polyline.end());
		}
		else
		{
			// Get the last (non-zero length) GCA of the polyline.
			GPlatesMaths::PolylineOnSphere::const_iterator gca_iter = polyline.end();
			do
			{
				--gca_iter;

				if (!gca_iter->is_zero_length())
				{
					return *gca_iter;
				}
			}
			while (gca_iter != polyline.begin());
		}

		// All GCA's of the polyline are zero-length.
		return boost::none;
	}


	/**
	 * Precondition: the GCA's are not zero length.
	 */
	bool
	do_adjacent_great_circle_arcs_bend_left(
			const GPlatesMaths::GreatCircleArc &prev_gca,
			const GPlatesMaths::GreatCircleArc &next_gca,
			const GPlatesMaths::PointOnSphere &intersection_point)
	{
		// Unless the two GCAs are parallel there they will form a smaller acute angle
		// on one side and a larger obtuse angle on the other side.
		// If the acute angle is to the left (meaning the next GCA bends to the left
		// relative to the previous GCA when following along the vertices) then the
		// cross product vector of the GCAs will be in the same hemisphere as the
		// intersection point (where the two GCAs meet) otherwise it will be in the
		// opposite hemisphere.
		return dot(
				intersection_point.position_vector(),
				cross(prev_gca.rotation_axis(), next_gca.rotation_axis())).dval()
						> 0;
	}
}


GPlatesMaths::PolygonIntersections::PolygonIntersections(
		const PolygonOnSphere::non_null_ptr_to_const_type &partitioning_polygon) :
	d_partitioning_polygon(partitioning_polygon),
	d_partitioning_polygon_orientation(
			partitioning_polygon->get_orientation())
{
}


GPlatesMaths::PolygonIntersections::Result
GPlatesMaths::PolygonIntersections::partition_geometry(
		const GeometryOnSphere::non_null_ptr_to_const_type &geometry_to_be_partitioned,
		partitioned_geometry_seq_type &partitioned_geometries_inside,
		partitioned_geometry_seq_type &partitioned_geometries_outside) const
{
	GeometryPartitioner geometry_partitioner(
			*this, partitioned_geometries_inside, partitioned_geometries_outside);

	return geometry_partitioner.partition_geometry(geometry_to_be_partitioned);
}


GPlatesMaths::PolygonIntersections::Result
GPlatesMaths::PolygonIntersections::partition_polyline(
		const PolylineOnSphere::non_null_ptr_to_const_type &polyline_to_be_partitioned,
		partitioned_polyline_seq_type &partitioned_polylines_inside,
		partitioned_polyline_seq_type &partitioned_polylines_outside) const
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
	partition_intersecting_geometry(
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
		partitioned_polyline_seq_type &partitioned_polylines_outside) const
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
	partition_intersecting_geometry(
			*partitioned_polylines_graph,
			partitioned_polylines_inside,
			partitioned_polylines_outside);

	// There were intersections.
	return GEOMETRY_INTERSECTING;
}


GPlatesMaths::PolygonIntersections::Result
GPlatesMaths::PolygonIntersections::partition_point(
		const PointOnSphere &point_to_be_partitioned) const
{
	const PointInPolygon::Result point_in_polygon_result =
			d_partitioning_polygon->is_point_in_polygon(
					point_to_be_partitioned,
					// Use high speed point-in-poly testing since we're being used for
					// generalised cookie-cutting and we could be asked to test lots of points.
					// For example, very dense velocity meshes go through this path.
					PolygonOnSphere::HIGH_SPEED_HIGH_SETUP_HIGH_MEMORY_USAGE);

	switch (point_in_polygon_result)
	{
	case PointInPolygon::POINT_OUTSIDE_POLYGON:
	default:
		return GEOMETRY_OUTSIDE;
	case PointInPolygon::POINT_INSIDE_POLYGON:
		return GEOMETRY_INSIDE;
// 	case PointInPolygon::POINT_ON_POLYGON:
// 		return GEOMETRY_INTERSECTING;
	}

	// Shouldn't get here.
	return GEOMETRY_OUTSIDE;
}


GPlatesMaths::PolygonIntersections::Result
GPlatesMaths::PolygonIntersections::partition_multipoint(
		const MultiPointOnSphere::non_null_ptr_to_const_type &multipoint_to_be_partitioned,
		boost::optional<MultiPointOnSphere::non_null_ptr_to_const_type> &partitioned_multipoint_inside,
		boost::optional<MultiPointOnSphere::non_null_ptr_to_const_type> &partitioned_multipoint_outside) const
{
	bool any_intersecting_points = false;

	std::vector<PointOnSphere> partitioned_points_inside;
	std::vector<PointOnSphere> partitioned_points_outside;

	// Iterate over the points in the multipoint and test each one.
	MultiPointOnSphere::const_iterator multipoint_iter = multipoint_to_be_partitioned->begin();
	MultiPointOnSphere::const_iterator multipoint_end = multipoint_to_be_partitioned->end();
	for ( ; multipoint_iter != multipoint_end; ++multipoint_iter)
	{
		const PointOnSphere &point = *multipoint_iter;

		const Result point_result = partition_point(point);
		switch (point_result)
		{
		case GEOMETRY_OUTSIDE:
		default:
			partitioned_points_outside.push_back(point);
			break;

		case GEOMETRY_INSIDE:
			partitioned_points_inside.push_back(point);
			break;

		case GEOMETRY_INTERSECTING:
			// Classify points on boundary as inside.
			partitioned_points_inside.push_back(point);
			any_intersecting_points = true;
			break;
		}
	}

	if (!partitioned_points_inside.empty())
	{
		partitioned_multipoint_inside = MultiPointOnSphere::create_on_heap(
				partitioned_points_inside.begin(), partitioned_points_inside.end());
	}
	if (!partitioned_points_outside.empty())
	{
		partitioned_multipoint_outside = MultiPointOnSphere::create_on_heap(
				partitioned_points_outside.begin(), partitioned_points_outside.end());
	}

	// If there were any points on the boundary or there are points inside
	// and outside then classify that as intersecting.
	if (any_intersecting_points ||
			(!partitioned_points_inside.empty() && !partitioned_points_outside.empty()))
	{
		return GEOMETRY_INTERSECTING;
	}

	// Only one of inside or outside lists has any points in it.

	if (!partitioned_points_inside.empty())
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
		const PointOnSphere &arbitrary_point_on_geometry1,
		const PointOnSphere &arbitrary_point_on_geometry2) const
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
GPlatesMaths::PolygonIntersections::partition_intersecting_geometry(
		const PolylineIntersections::Graph &partitioned_polylines_graph,
		partitioned_polyline_seq_type &partitioned_polylines_inside,
		partitioned_polyline_seq_type &partitioned_polylines_outside) const
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
			inside_partitioned_polyline_merger.add_inside_polyline(partitioned_poly->polyline);
			continue;
		}

		// Whether the partitioned polyline if previous to the intersection point.
		//
		// By default (when has a previous intersection) the partitioned polyline is
		// the next polyline after the intersection (which means it's not the previous).
		bool is_prev_partitioned_polyline = false;

		const PolylineIntersections::Graph::Intersection *intersection = partitioned_poly->prev_intersection;
		// If no previous intersection...
		if (intersection == NULL)
		{
			// We must be the first polyline of the sequence and it doesn't start at a T-junction.
			// Use the intersection at the end of the partitioned polyline instead.
			intersection = partitioned_poly->next_intersection;

			// It's not possible for a partitioned polyline to have no intersections at either end.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					intersection != NULL,
					GPLATES_ASSERTION_SOURCE);

			// The end of the partitioned polyline is now touching the intersection point
			// instead of the start touching it.
			is_prev_partitioned_polyline = true;
		}

		// Get the partitioning polyline just prior to the intersection point.
		// NOTE: The partitioning polygon is the first sequence in the graph.
		const PolylineIntersections::Graph::PartitionedPolyline &prev_partitioning_polygon =
				intersection->prev_partitioned_polyline1
						? *intersection->prev_partitioned_polyline1
						// There's no previous polyline in the partitioning polygon so the
						// intersection point must coincide with the polygon's start point.
						// Use the last polyline in the partitioning polygon instead.
						: *partitioned_polylines_graph.partitioned_polylines1.back();

		// Get the partitioning polyline just after the intersection point.
		// NOTE: The partitioning polygon is the first sequence in the graph.
		const PolylineIntersections::Graph::PartitionedPolyline &next_partitioning_polygon =
				intersection->next_partitioned_polyline1
						? *intersection->next_partitioned_polyline1
						// There's no next polyline in the partitioning polygon so the
						// intersection point must coincide with the polygon's end point.
						// Use the first polyline in the partitioning polygon instead.
						: *partitioned_polylines_graph.partitioned_polylines1.front();

		// Determine if the current partitioned polyline is inside or outside the partitioning polygon.
		const bool is_partitioned_poly_inside =
				is_partitioned_polyline_inside_partitioning_polygon(
						intersection->intersection_point,
						*prev_partitioning_polygon.polyline,
						*next_partitioning_polygon.polyline,
						*partitioned_poly->polyline,
						is_prev_partitioned_polyline);

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
GPlatesMaths::PolygonIntersections::is_partitioned_polyline_inside_partitioning_polygon(
		const PointOnSphere &intersection_point,
		const PolylineOnSphere &prev_partitioning_polygon,
		const PolylineOnSphere &next_partitioning_polygon,
		const PolylineOnSphere &partitioned_polyline,
		bool is_prev_partitioned_polyline) const
{
	// Get first (or last) non-zero length GCA of the partitioning and partitioned polylines.

	boost::optional<const GPlatesMaths::GreatCircleArc &> prev_partitioning_polygon_gca =
			get_first_or_last_non_zero_great_circle_arc(
					prev_partitioning_polygon,
					false/*get_first*/);

	boost::optional<const GPlatesMaths::GreatCircleArc &> next_partitioning_polygon_gca =
			get_first_or_last_non_zero_great_circle_arc(
					next_partitioning_polygon,
					true/*get_first*/);

	boost::optional<const GPlatesMaths::GreatCircleArc &> partitioned_polyline_gca =
			get_first_or_last_non_zero_great_circle_arc(
					partitioned_polyline,
					!is_prev_partitioned_polyline/*get_first*/);

	// If any polyline is coincident with a point then consider the partitioned polyline inside.
	// It shouldn't really happen anyway.
	if (!prev_partitioning_polygon_gca ||
		!next_partitioning_polygon_gca ||
		!partitioned_polyline_gca)
	{
		return true;
	}

	// It is assumed that 'prev_partitioning_polygon's end point equals
	// 'next_partitioning_polygon's start point.

	/*
	 * When testing to see if 'partitioned_polyline' is to the left or right of the
	 * two GCAs from the partitioning polygon the test needs to be done
	 * to see if 'partitioned_polyline' falls in the acute angle region.
	 * For example,
	 *
	 *    ^               ^
	 *     \               \ A /
	 * left \  right        \ /
	 *       ^               ^
	 *      /               / \
	 *     /               / B \
	 *
	 * ...we can test for the left region by testing left of both arcs.
	 * If we tried to test for the right region instead we would try testing
	 * right of both arcs but that would miss regions A and B shown above
	 * which are supposed to be part of the right region.
	 */
	if (do_adjacent_great_circle_arcs_bend_left(
			prev_partitioning_polygon_gca.get(),
			next_partitioning_polygon_gca.get(),
			intersection_point))
	{
		// If the adjacent arcs on the partitioning polygon bend to the left then
		// left is the narrowest region.
		// This region is inside the polygon if the polygon's vertices
		// are counter-clockwise and outside if they are clockwise.

		// See if the polyline GCA is in the narrow region to the left.
		//
		// If the partitioned polyline is prior to (previous) the intersection point so
		// we have to reverse the interpretation of the left/right test.
		if ((is_prev_partitioned_polyline ^
				do_adjacent_great_circle_arcs_bend_left(
						prev_partitioning_polygon_gca.get(),
						partitioned_polyline_gca.get(),
						intersection_point)) &&
			(is_prev_partitioned_polyline ^
				do_adjacent_great_circle_arcs_bend_left(
						next_partitioning_polygon_gca.get(),
						partitioned_polyline_gca.get(),
						intersection_point)))
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
	//
	// If the partitioned polyline is prior to (previous) the intersection point so
	// we have to reverse the interpretation of the left/right test.
	if ((is_prev_partitioned_polyline ^
			!do_adjacent_great_circle_arcs_bend_left(
					prev_partitioning_polygon_gca.get(),
					partitioned_polyline_gca.get(),
					intersection_point)) &&
		(is_prev_partitioned_polyline ^
			!do_adjacent_great_circle_arcs_bend_left(
					next_partitioning_polygon_gca.get(),
					partitioned_polyline_gca.get(),
					intersection_point)))
	{
		// The polyline GCA is in the narrow region to the right.
		return d_partitioning_polygon_orientation == PolygonOrientation::CLOCKWISE;
	}

	// The polyline GCA was not in the narrow region to the right
	// so invert the test for inside.
	return d_partitioning_polygon_orientation == PolygonOrientation::COUNTERCLOCKWISE;
}
