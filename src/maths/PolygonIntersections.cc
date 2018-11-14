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
#include <boost/utility/in_place_factory.hpp>
#include <QDebug>

#include "PolygonIntersections.h"

#include "ConstGeometryOnSphereVisitor.h"
#include "GreatCircleArc.h"
#include "PointInPolygon.h"
#include "PolylineIntersections.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


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
				boost::optional<PolygonIntersections::partitioned_geometry_seq_type &> partitioned_geometries_inside,
				boost::optional<PolygonIntersections::partitioned_geometry_seq_type &> partitioned_geometries_outside) :
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
			if (!d_partitioned_geometries_inside &&
				!d_partitioned_geometries_outside)
			{
				d_result = d_polygon_intersections.partition_multipoint(multi_point_on_sphere);
				return;
			}

			PolygonIntersections::partitioned_point_seq_type partitioned_points_inside;
			PolygonIntersections::partitioned_point_seq_type partitioned_points_outside;
			d_result = d_polygon_intersections.partition_multipoint(
					multi_point_on_sphere,
					partitioned_points_inside,
					partitioned_points_outside);

			if (d_partitioned_geometries_inside &&
				!partitioned_points_inside.empty())
			{
				d_partitioned_geometries_inside->push_back(
						MultiPointOnSphere::create_on_heap(
								partitioned_points_inside.begin(),
								partitioned_points_inside.end()));
			}
			if (d_partitioned_geometries_outside &&
				!partitioned_points_outside.empty())
			{
				d_partitioned_geometries_outside->push_back(
						MultiPointOnSphere::create_on_heap(
								partitioned_points_outside.begin(),
								partitioned_points_outside.end()));
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
				if (d_partitioned_geometries_outside)
				{
					d_partitioned_geometries_outside->push_back(point_on_sphere);
				}
			}
			else
			{
				if (d_partitioned_geometries_inside)
				{
					d_partitioned_geometries_inside->push_back(point_on_sphere);
				}
			}
		}

		virtual
		void
		visit_polygon_on_sphere(
				PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
		{
			if (!d_partitioned_geometries_inside &&
				!d_partitioned_geometries_outside)
			{
				d_result = d_polygon_intersections.partition_polygon(polygon_on_sphere);
				return;
			}

			PolygonIntersections::partitioned_polyline_seq_type partitioned_polylines_inside;
			PolygonIntersections::partitioned_polyline_seq_type partitioned_polylines_outside;
			d_result = d_polygon_intersections.partition_polygon(
					polygon_on_sphere,
					partitioned_polylines_inside,
					partitioned_polylines_outside);

			// NOTE: 'PolygonIntersections::partition_polygon()' only returns partitioned *polylines*
			// if there was an intersection, otherwise the inside/outside polylines are empty.
			// Hence if there was no intersection then we add the inside or outside *polygon*.
			if (d_result == PolygonIntersections::GEOMETRY_INSIDE)
			{
				if (d_partitioned_geometries_inside)
				{
					d_partitioned_geometries_inside->push_back(polygon_on_sphere);
				}
			}
			else if (d_result == PolygonIntersections::GEOMETRY_OUTSIDE)
			{
				if (d_partitioned_geometries_outside)
				{
					d_partitioned_geometries_outside->push_back(polygon_on_sphere);
				}
			}
			else // GEOMETRY_INTERSECTING...
			{
				if (d_partitioned_geometries_inside)
				{
					std::copy(partitioned_polylines_inside.begin(), partitioned_polylines_inside.end(),
							std::back_inserter(d_partitioned_geometries_inside.get()));
				}
				if (d_partitioned_geometries_outside)
				{
					std::copy(partitioned_polylines_outside.begin(), partitioned_polylines_outside.end(),
							std::back_inserter(d_partitioned_geometries_outside.get()));
				}
			}
		}

		virtual
		void
		visit_polyline_on_sphere(
				PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
		{
			if (!d_partitioned_geometries_inside &&
				!d_partitioned_geometries_outside)
			{
				d_result = d_polygon_intersections.partition_polyline(polyline_on_sphere);
				return;
			}

			PolygonIntersections::partitioned_polyline_seq_type partitioned_polylines_inside;
			PolygonIntersections::partitioned_polyline_seq_type partitioned_polylines_outside;
			d_result = d_polygon_intersections.partition_polyline(
					polyline_on_sphere,
					partitioned_polylines_inside,
					partitioned_polylines_outside);

			if (d_partitioned_geometries_inside)
			{
				std::copy(partitioned_polylines_inside.begin(), partitioned_polylines_inside.end(),
						std::back_inserter(d_partitioned_geometries_inside.get()));
			}
			if (d_partitioned_geometries_outside)
			{
				std::copy(partitioned_polylines_outside.begin(), partitioned_polylines_outside.end(),
						std::back_inserter(d_partitioned_geometries_outside.get()));
			}
		}

	private:
		const PolygonIntersections &d_polygon_intersections;

		PolygonIntersections::Result d_result;
		boost::optional<PolygonIntersections::partitioned_geometry_seq_type &> d_partitioned_geometries_inside;
		boost::optional<PolygonIntersections::partitioned_geometry_seq_type &> d_partitioned_geometries_outside;
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
		explicit
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
		const PolygonOnSphere::non_null_ptr_to_const_type &partitioning_polygon,
		PolygonOnSphere::PointInPolygonSpeedAndMemory partition_point_speed_and_memory) :
	d_partitioning_polygon(partitioning_polygon),
	d_partitioning_polygon_orientation(partitioning_polygon->get_orientation()),
	d_partition_point_speed_and_memory(partition_point_speed_and_memory)
{
}


GPlatesMaths::PolygonIntersections::Result
GPlatesMaths::PolygonIntersections::partition_geometry(
		const GeometryOnSphere::non_null_ptr_to_const_type &geometry_to_be_partitioned,
		boost::optional<partitioned_geometry_seq_type &> partitioned_geometries_inside,
		boost::optional<partitioned_geometry_seq_type &> partitioned_geometries_outside) const
{
	GeometryPartitioner geometry_partitioner(
			*this, partitioned_geometries_inside, partitioned_geometries_outside);

	return geometry_partitioner.partition_geometry(geometry_to_be_partitioned);
}


GPlatesMaths::PolygonIntersections::Result
GPlatesMaths::PolygonIntersections::partition_polyline(
		const PolylineOnSphere::non_null_ptr_to_const_type &polyline_to_be_partitioned,
		boost::optional<partitioned_polyline_seq_type &> partitioned_polylines_inside,
		boost::optional<partitioned_polyline_seq_type &> partitioned_polylines_outside) const
{
	// Partition the geometry to be partitioned against the partitioning polygon.
	//
	// If there were no intersections then the polyline to be partitioned must be either
	// fully inside or fully outside the partitioning polygon - find out which.
	PolylineIntersections::Graph partitioned_polylines_graph;
	if (!PolylineIntersections::partition(
			partitioned_polylines_graph,
			// NOTE: The first geometry specified is the partitioning polygon.
			// This means it corresponds to the first sequence in the returned graph...
			*d_partitioning_polygon,
			*polyline_to_be_partitioned))
	{
		// Choose any point on the polyline and to see if it's inside the partitioning polygon.
		// Any point will do. Pick the first point.
		if (is_non_intersecting_polyline_or_polygon_fully_inside_partitioning_polygon(
				*polyline_to_be_partitioned->vertex_begin()))
		{
			if (partitioned_polylines_inside)
			{
				partitioned_polylines_inside->push_back(polyline_to_be_partitioned);
			}

			return GEOMETRY_INSIDE;
		}
		else
		{
			if (partitioned_polylines_outside)
			{
				partitioned_polylines_outside->push_back(polyline_to_be_partitioned);
			}

			return GEOMETRY_OUTSIDE;
		}
	}

	// Determine which partitioned polylines are inside/outside the partitioning polygon
	// and add to the appropriate lists.
	partition_intersecting_geometry(
			partitioned_polylines_graph,
			partitioned_polylines_inside,
			partitioned_polylines_outside);

	// There were intersections.
	return GEOMETRY_INTERSECTING;
}


GPlatesMaths::PolygonIntersections::Result
GPlatesMaths::PolygonIntersections::partition_polygon(
		const PolygonOnSphere::non_null_ptr_to_const_type &polygon_to_be_partitioned,
		boost::optional<partitioned_polyline_seq_type &> partitioned_polylines_inside,
		boost::optional<partitioned_polyline_seq_type &> partitioned_polylines_outside) const
{
	// Partition the geometry to be partitioned against the partitioning polygon.
	//
	// If there were no intersections then the polygon to be partitioned must be either
	// fully inside or fully outside the partitioning polygon - find out which.
	PolylineIntersections::Graph partitioned_polylines_graph;
	if (!PolylineIntersections::partition(
			partitioned_polylines_graph,
			// NOTE: The first argument is the partitioning polygon.
			// This means it corresponds to the first sequence in the returned graph...
			*d_partitioning_polygon,
			*polygon_to_be_partitioned))
	{
		// Choose any point on the polygon and to see if it's inside the partitioning polygon.
		// Any point will do. Pick the first point.
		if (is_non_intersecting_polyline_or_polygon_fully_inside_partitioning_polygon(
				*polygon_to_be_partitioned->exterior_ring_vertex_begin()))
		{
			return GEOMETRY_INSIDE;
		}
		else
		{
			return GEOMETRY_OUTSIDE;
		}
	}

	// Determine which partitioned polylines are inside/outside the partitioning polygon
	// and add to the appropriate lists.
	partition_intersecting_geometry(
			partitioned_polylines_graph,
			partitioned_polylines_inside,
			partitioned_polylines_outside);

	// There were intersections.
	return GEOMETRY_INTERSECTING;
}


GPlatesMaths::PolygonIntersections::Result
GPlatesMaths::PolygonIntersections::partition_point(
		const PointOnSphere &point_to_be_partitioned) const
{
	return d_partitioning_polygon->is_point_in_polygon(
					point_to_be_partitioned,
					d_partition_point_speed_and_memory)
			? GEOMETRY_INSIDE
			: GEOMETRY_OUTSIDE;
}


GPlatesMaths::PolygonIntersections::Result
GPlatesMaths::PolygonIntersections::partition_multipoint(
		const MultiPointOnSphere::non_null_ptr_to_const_type &multipoint_to_be_partitioned,
		boost::optional<partitioned_point_seq_type &> partitioned_points_inside_opt,
		boost::optional<partitioned_point_seq_type &> partitioned_points_outside_opt) const
{
	bool any_intersecting_points = false;

	// Use our own storage or caller's storage depending on whether caller provided any.
	boost::optional<partitioned_point_seq_type> partitioned_points_inside_storage;
	if (!partitioned_points_inside_opt)
	{
		partitioned_points_inside_storage = partitioned_point_seq_type();
	}
	partitioned_point_seq_type &partitioned_points_inside = partitioned_points_inside_opt
			? partitioned_points_inside_opt.get()
			: partitioned_points_inside_storage.get();

	// Use our own storage or caller's storage depending on whether caller provided any.
	boost::optional<partitioned_point_seq_type> partitioned_points_outside_storage;
	if (!partitioned_points_outside_opt)
	{
		partitioned_points_outside_storage = partitioned_point_seq_type();
	}
	partitioned_point_seq_type &partitioned_points_outside = partitioned_points_outside_opt
			? partitioned_points_outside_opt.get()
			: partitioned_points_outside_storage.get();

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

		// Note: We shouldn't actually get here with point partitioning.
		// Results are either inside or outside. But we test just in case this changes...
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


bool
GPlatesMaths::PolygonIntersections::is_non_intersecting_polyline_or_polygon_fully_inside_partitioning_polygon(
		const PointOnSphere &arbitrary_point_on_geometry) const
{
	// PolylineIntersections has guaranteed there are no intersections within an extremely small
	// threshold distance of the partitioning polygon. So we know the polyline (or polygon) to be
	// partitioned is either fully inside or fully outside the partitioning polygon. If it's fully
	// outside then we don't want the point-in-polygon test to return true if the point is *very*
	// close to the partitioning polygon, so we turn of point-on-polygon threshold testing.
	return d_partitioning_polygon->is_point_in_polygon(
					arbitrary_point_on_geometry,
					d_partition_point_speed_and_memory,
					// Note we turned off point-on-polygon outline threshold testing...
					false/*use_point_on_polygon_threshold*/);
}


void
GPlatesMaths::PolygonIntersections::partition_intersecting_geometry(
		const PolylineIntersections::Graph &partitioned_polylines_graph,
		boost::optional<partitioned_polyline_seq_type &> partitioned_polylines_inside,
		boost::optional<partitioned_polyline_seq_type &> partitioned_polylines_outside) const
{
	if (!partitioned_polylines_inside &&
		!partitioned_polylines_outside)
	{
		return;
	}

	boost::optional<InsidePartitionedPolylineMerger> inside_partitioned_polyline_merger;
	if (partitioned_polylines_inside)
	{
		inside_partitioned_polyline_merger =
				boost::in_place(boost::ref(partitioned_polylines_inside.get()));
	}

	// Iterate over the partitioned polylines of the geometry being partitioned.
	// NOTE: The geometry that was partitioned is the second sequence in the graph.
	PolylineIntersections::partitioned_polyline_ptr_to_const_seq_type::const_iterator
			partitioned_polyline_iter = partitioned_polylines_graph.partitioned_polylines2.begin();
	PolylineIntersections::partitioned_polyline_ptr_to_const_seq_type::const_iterator
			partitioned_polyline_end = partitioned_polylines_graph.partitioned_polylines2.end();
	for ( ; partitioned_polyline_iter != partitioned_polyline_end; ++partitioned_polyline_iter)
	{
		const PolylineIntersections::PartitionedPolyline::non_null_ptr_to_const_type &
				partitioned_poly = *partitioned_polyline_iter;

		// Determine if the current partitioned polyline is inside or outside the partitioning polygon.
		const bool is_partitioned_poly_inside =
				is_partitioned_polyline_inside_partitioning_polygon(
						partitioned_polylines_graph,
						*partitioned_poly);

		if (is_partitioned_poly_inside)
		{
			// Add inside polyline to the merger instead of the caller's inside list.
			if (inside_partitioned_polyline_merger)
			{
				inside_partitioned_polyline_merger->add_inside_polyline(partitioned_poly->polyline);
			}
		}
		else
		{
			// Add to the list of outside polylines.
			if (partitioned_polylines_outside)
			{
				partitioned_polylines_outside->push_back(partitioned_poly->polyline);
			}

			// We've come across an outside polyline so merge any inside polylines
			// we've accumulated so far and append resulting polyline to the
			// caller's inside list.
			if (inside_partitioned_polyline_merger)
			{
				inside_partitioned_polyline_merger->merge_inside_polylines_and_output();
			}
		}
	}

	// If there are any inside polylines accumulated then merge them and
	// append resulting polyline to the caller's inside list.
	if (inside_partitioned_polyline_merger)
	{
		inside_partitioned_polyline_merger->merge_inside_polylines_and_output();
	}
}


bool
GPlatesMaths::PolygonIntersections::is_partitioned_polyline_inside_partitioning_polygon(
		const PolylineIntersections::Graph &partitioned_polylines_graph,
		const PolylineIntersections::PartitionedPolyline &partitioned_poly) const
{
	// Whether the partitioned polyline if previous to the intersection point.
	//
	// By default (when has a previous intersection) the partitioned polyline is
	// the next polyline after the intersection (which means it's not the previous).
	bool is_prev_partitioned_polyline = false;

	const PolylineIntersections::Intersection *intersection = partitioned_poly.prev_intersection;
	// If no previous intersection...
	if (intersection == NULL)
	{
		// We must be the first polyline of the sequence and it doesn't start at a T-junction.
		// Use the intersection at the end of the partitioned polyline instead.
		intersection = partitioned_poly.next_intersection;

		// It's not possible for a partitioned polyline to have no intersections at either end.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				intersection != NULL,
				GPLATES_ASSERTION_SOURCE);

		// The end of the partitioned polyline is now touching the intersection point
		// instead of the start touching it.
		is_prev_partitioned_polyline = true;
	}

	//
	// Get the previous and next non-zero-length GCA of the partitioning polygon (at the intersection).
	//

	// Get the non-zero-length great circle arc of the partitioning polygon just prior to the intersection point.
	// NOTE: The partitioning polygon is the first sequence in the graph.
	boost::optional<const GPlatesMaths::GreatCircleArc &> prev_partitioning_polygon_gca;

	// It's possible the an entire partitioned polyline of the partitioning polygon is made up of zero-length arcs,
	// in which case we consider the previous partitioned polyline (until we've searched all its partitioned polylines).
	const PolylineIntersections::PartitionedPolyline *prev_partitioning_polygon = intersection->prev_partitioned_polyline1;
	for (unsigned int p = 0; p < partitioned_polylines_graph.partitioned_polylines1.size(); ++p)
	{
		if (prev_partitioning_polygon == NULL)
		{
			// There's no previous polyline in the partitioning polygon so the current intersection point must
			// coincide with the polygon's start point. Use the last polyline in the partitioning polygon instead.
			prev_partitioning_polygon = partitioned_polylines_graph.partitioned_polylines1.back().get();
		}

		prev_partitioning_polygon_gca = get_first_or_last_non_zero_great_circle_arc(
				*prev_partitioning_polygon->polyline,
				false/*get_first*/);
		if (prev_partitioning_polygon_gca)
		{
			break;
		}

		// Get previous polyline in the partitioning polygon.
		const PolylineIntersections::Intersection *prev_intersection = prev_partitioning_polygon->prev_intersection;
		if (prev_intersection)
		{
			prev_partitioning_polygon = prev_intersection->prev_partitioned_polyline1;
		}
		else
		{
			// There's no previous intersection in the partitioning polygon so use the last polyline.
			prev_partitioning_polygon = partitioned_polylines_graph.partitioned_polylines1.back().get();
		}
	}

	// Get the non-zero-length great circle arc of the partitioning polygon just past to the intersection point.
	// NOTE: The partitioning polygon is the first sequence in the graph.
	boost::optional<const GPlatesMaths::GreatCircleArc &> next_partitioning_polygon_gca;

	// It's possible the an entire partitioned polyline of the partitioning polygon is made up of zero-length arcs,
	// in which case we consider the next partitioned polyline (until we've searched all its partitioned polylines).
	const PolylineIntersections::PartitionedPolyline *next_partitioning_polygon = intersection->next_partitioned_polyline1;
	for (unsigned int p = 0; p < partitioned_polylines_graph.partitioned_polylines1.size(); ++p)
	{
		if (next_partitioning_polygon == NULL)
		{
			// There's no next polyline in the partitioning polygon so the current intersection point must
			// coincide with the polygon's end point. Use the first polyline in the partitioning polygon instead.
			next_partitioning_polygon = partitioned_polylines_graph.partitioned_polylines1.front().get();
		}

		next_partitioning_polygon_gca = get_first_or_last_non_zero_great_circle_arc(
				*next_partitioning_polygon->polyline,
				true/*get_first*/);
		if (next_partitioning_polygon_gca)
		{
			break;
		}

		// Get next polyline in the partitioning polygon.
		const PolylineIntersections::Intersection *next_intersection = next_partitioning_polygon->next_intersection;
		if (next_intersection)
		{
			next_partitioning_polygon = next_intersection->next_partitioned_polyline1;
		}
		else
		{
			// There's no next intersection in the partitioning polygon so use the first polyline.
			next_partitioning_polygon = partitioned_polylines_graph.partitioned_polylines1.front().get();
		}
	}

	//
	// Get first (or last) non-zero length GCA of the partitioning and partitioned polylines.
	//

	boost::optional<const GPlatesMaths::GreatCircleArc &> partitioned_polyline_gca =
			get_first_or_last_non_zero_great_circle_arc(
					*partitioned_poly.polyline,
					!is_prev_partitioned_polyline/*get_first*/);

	// If a non-zero great circle arc cannot be found for either the previous or next polyline of the
	// partitioning polygon (at intersection point) then there's not much we can do (so just return true).
	// This shouldn't really happen anyway.
	// 
	// However if the *partitioned* polyline is coincident with a point then consider it inside the polygon
	// (since we know it is *on* the polygon).
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
			intersection->intersection_point))
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
						intersection->intersection_point)) &&
			(is_prev_partitioned_polyline ^
				do_adjacent_great_circle_arcs_bend_left(
						next_partitioning_polygon_gca.get(),
						partitioned_polyline_gca.get(),
						intersection->intersection_point)))
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
					intersection->intersection_point)) &&
		(is_prev_partitioned_polyline ^
			!do_adjacent_great_circle_arcs_bend_left(
					next_partitioning_polygon_gca.get(),
					partitioned_polyline_gca.get(),
					intersection->intersection_point)))
	{
		// The polyline GCA is in the narrow region to the right.
		return d_partitioning_polygon_orientation == PolygonOrientation::CLOCKWISE;
	}

	// The polyline GCA was not in the narrow region to the right
	// so invert the test for inside.
	return d_partitioning_polygon_orientation == PolygonOrientation::COUNTERCLOCKWISE;
}
