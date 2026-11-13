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
#include <vector>
#include <boost/make_shared.hpp>
#include <boost/optional.hpp>

#include "PolygonPartitioner.h"

#include "ConstGeometryOnSphereVisitor.h"
#include "SmallCircleBounds.h"


namespace
{
	using GPlatesMaths::PolygonPartitioner;
	namespace Overlay = GPlatesMaths::PolygonOverlay;

	typedef PolygonPartitioner::provenance_seq_type provenance_seq_type;
	typedef boost::shared_ptr<const provenance_seq_type> provenance_ptr_type;
	typedef PolygonPartitioner::partitioned_geometry_seq_type partitioned_geometry_seq_type;


	provenance_ptr_type
	make_table(
			const provenance_seq_type &provenance)
	{
		return boost::make_shared<const provenance_seq_type>(provenance);
	}


	PolygonPartitioner::Result
	to_partition_result(
			Overlay::Result result)
	{
		switch (result)
		{
		case Overlay::RESULT_INSIDE:
			return PolygonPartitioner::GEOMETRY_INSIDE;
		case Overlay::RESULT_OUTSIDE:
			return PolygonPartitioner::GEOMETRY_OUTSIDE;
		case Overlay::RESULT_INTERSECTING:
		default:
			return PolygonPartitioner::GEOMETRY_INTERSECTING;
		}
	}


	/**
	 * Whether a subject whose outline lies within @a subject_bounds is certainly outside
	 * @a partitioning_polygon without an overlay (spherical-geometry.md section 5.1): the two
	 * bounding circles are disjoint, and the partitioning polygon's interior lies within its own
	 * circle, and so does the subject's when it is a polygon (@a subject_polygon; none for a
	 * polyline, whose outline is all there is).
	 */
	bool
	is_outside_by_bounds(
			const GPlatesMaths::BoundingSmallCircle &subject_bounds,
			const GPlatesMaths::PolygonOnSphere *subject_polygon,
			const GPlatesMaths::PolygonOnSphere &partitioning_polygon)
	{
		const GPlatesMaths::BoundingSmallCircle &partitioning_bounds =
				partitioning_polygon.get_bounding_small_circle();
		if (GPlatesMaths::intersect(subject_bounds, partitioning_bounds))
		{
			return false;
		}
		return partitioning_polygon.is_outside_bounding_small_circle_outside_polygon(
						partitioning_bounds) &&
				(!subject_polygon ||
					subject_polygon->is_outside_bounding_small_circle_outside_polygon(
							subject_bounds));
	}


	/**
	 * Overlay a polygon subject with the partitioning polygon. The result follows the result
	 * contract (section 5.1): GEOMETRY_INSIDE if the difference is empty, GEOMETRY_OUTSIDE if the
	 * intersection is (after clean-up), and only for GEOMETRY_INTERSECTING are @a inside and
	 * @a outside filled (else the subject is returned whole by the caller).
	 *
	 * With or without lists the overlay is the same: only the output geometries are not built when
	 * no list wants them.
	 */
	PolygonPartitioner::Result
	overlay_polygon(
			const GPlatesMaths::PolygonOnSphere &subject,
			const GPlatesMaths::PolygonOnSphere &partitioning_polygon,
			const Overlay::Options &options,
			std::vector<Overlay::OutputPolygon> &inside,
			std::vector<Overlay::OutputPolygon> &outside)
	{
		if (is_outside_by_bounds(
				subject.get_bounding_small_circle(),
				&subject,
				partitioning_polygon))
		{
			return PolygonPartitioner::GEOMETRY_OUTSIDE;
		}

		Overlay::Overlay overlay(subject, partitioning_polygon, options);
		const PolygonPartitioner::Result result = to_partition_result(overlay.classify());
		if (result != PolygonPartitioner::GEOMETRY_INTERSECTING)
		{
			return result;
		}

		const Overlay::Diagnostics &diagnostics = overlay.get_diagnostics();
		const double area_dropped_before = diagnostics.sliver_area_dropped;
		overlay.extract(Overlay::INTERSECTION, inside);
		const double area_dropped_after_inside = diagnostics.sliver_area_dropped;
		overlay.extract(Overlay::DIFFERENCE, outside);
		const double inside_area_dropped = area_dropped_after_inside - area_dropped_before;
		const double outside_area_dropped =
				diagnostics.sliver_area_dropped - area_dropped_after_inside;

		// Pieces kept but collapsed by clean-up, or dropped by the sliver filter, leave one side
		// empty: the subject is then whole. If both are emptied (only the filter can, as when it
		// drops both halves of a subject thinner than its width), it is whole on the side that
		// lost the larger area; on a tie (no filter), outside.
		if (inside.empty() && outside.empty())
		{
			return (inside_area_dropped > outside_area_dropped)
					? PolygonPartitioner::GEOMETRY_INSIDE
					: PolygonPartitioner::GEOMETRY_OUTSIDE;
		}
		if (inside.empty())
		{
			outside.clear();
			return PolygonPartitioner::GEOMETRY_OUTSIDE;
		}
		if (outside.empty())
		{
			return PolygonPartitioner::GEOMETRY_INSIDE;
		}
		return PolygonPartitioner::GEOMETRY_INTERSECTING;
	}


	//! Whether @a points hold two different stored positions (a polyline needs two).
	bool
	has_two_positions(
			const std::vector<GPlatesMaths::PointOnSphere> &points)
	{
		for (const GPlatesMaths::PointOnSphere &point : points)
		{
			// Coordinates, not PointOnSphere's operator== (which has a tolerance).
			if (point.position_vector().x() != points.front().position_vector().x() ||
				point.position_vector().y() != points.front().position_vector().y() ||
				point.position_vector().z() != points.front().position_vector().z())
			{
				return true;
			}
		}
		return false;
	}


	/**
	 * Overlay a polyline subject with the partitioning polygon (section 5.2):
	 * as @a overlay_polygon, with the runs of each class in @a runs for GEOMETRY_INTERSECTING. A
	 * run at one position (if one can arise) is dropped as a collapsed region is; a side left with
	 * no run makes the polyline whole, as for a polygon (section 5.1). A polyline's classification
	 * is cheap and never throws.
	 */
	PolygonPartitioner::Result
	overlay_polyline(
			const GPlatesMaths::PolylineOnSphere &subject,
			const GPlatesMaths::PolygonOnSphere &partitioning_polygon,
			const Overlay::Options &options,
			std::vector<Overlay::OutputPolyline> &runs)
	{
		if (is_outside_by_bounds(
				subject.get_bounding_small_circle(),
				nullptr,
				partitioning_polygon))
		{
			return PolygonPartitioner::GEOMETRY_OUTSIDE;
		}

		Overlay::Overlay overlay(subject, partitioning_polygon, options);
		const PolygonPartitioner::Result result = to_partition_result(overlay.classify());
		if (result != PolygonPartitioner::GEOMETRY_INTERSECTING)
		{
			return result;
		}

		overlay.extract_polylines(runs);
		const auto is_collapsed = [](const Overlay::OutputPolyline &run)
		{
			return !has_two_positions(run.points);
		};
		runs.erase(std::remove_if(runs.begin(), runs.end(), is_collapsed), runs.end());
		bool any_inside = false;
		bool any_outside = false;
		for (const Overlay::OutputPolyline &run : runs)
		{
			(run.inside ? any_inside : any_outside) = true;
		}
		if (!any_inside || !any_outside)
		{
			runs.clear();
			return any_inside
					? PolygonPartitioner::GEOMETRY_INSIDE
					: PolygonPartitioner::GEOMETRY_OUTSIDE;
		}
		return result;
	}


	//! An output polygon's provenance, in the order of its points (exterior, then interiors).
	provenance_ptr_type
	get_polygon_provenance(
			const Overlay::OutputPolygon &output_polygon)
	{
		boost::shared_ptr<provenance_seq_type> provenance =
				boost::make_shared<provenance_seq_type>(output_polygon.exterior.provenances);
		for (const Overlay::OutputRing &interior : output_polygon.interiors)
		{
			provenance->insert(
					provenance->end(),
					interior.provenances.begin(),
					interior.provenances.end());
		}
		return provenance;
	}


	/**
	 * Partitions one geometry into lists of partitioned geometries, with provenance when
	 * @a cascade_provenance is given.
	 */
	class GeometryPartitioner :
			public GPlatesMaths::ConstGeometryOnSphereVisitor
	{
	public:
		GeometryPartitioner(
				const PolygonPartitioner &polygon_partitioner,
				const provenance_ptr_type &input_provenance,
				const PolygonPartitioner::CascadeProvenance *cascade_provenance,
				boost::optional<partitioned_geometry_seq_type &> partitioned_geometries_inside,
				boost::optional<partitioned_geometry_seq_type &> partitioned_geometries_outside) :
			d_polygon_partitioner(polygon_partitioner),
			d_input_provenance(input_provenance),
			d_cascade_provenance(cascade_provenance),
			d_partitioned_geometries_inside(partitioned_geometries_inside),
			d_partitioned_geometries_outside(partitioned_geometries_outside),
			d_result(PolygonPartitioner::GEOMETRY_OUTSIDE)
		{  }


		PolygonPartitioner::Result
		partition_geometry(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry_to_be_partitioned)
		{
			d_geometry = geometry_to_be_partitioned;
			d_result = PolygonPartitioner::GEOMETRY_OUTSIDE;

			geometry_to_be_partitioned->accept_visitor(*this);

			return d_result;
		}

	protected:
		virtual
		void
		visit_multi_point_on_sphere(
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
		{
			if (!d_partitioned_geometries_inside &&
				!d_partitioned_geometries_outside)
			{
				d_result = d_polygon_partitioner.partition_multipoint(multi_point_on_sphere);
				return;
			}

			std::vector<GPlatesMaths::PointOnSphere> points[2];  // outside, inside
			provenance_seq_type provenances[2];
			bool any_intersecting = false;
			unsigned int index = 0;
			for (const GPlatesMaths::PointOnSphere &point : *multi_point_on_sphere)
			{
				const PolygonPartitioner::Result point_result =
						d_polygon_partitioner.partition_point(point);
				// A point on the boundary is inside.
				const unsigned int side =
						(point_result == PolygonPartitioner::GEOMETRY_OUTSIDE) ? 0 : 1;
				any_intersecting = any_intersecting ||
						point_result == PolygonPartitioner::GEOMETRY_INTERSECTING;
				points[side].push_back(point);
				if (d_cascade_provenance)
				{
					provenances[side].push_back(input_point_provenance(index));
				}
				++index;
			}

			d_result = (any_intersecting || (!points[0].empty() && !points[1].empty()))
					? PolygonPartitioner::GEOMETRY_INTERSECTING
					: (points[1].empty()
							? PolygonPartitioner::GEOMETRY_OUTSIDE
							: PolygonPartitioner::GEOMETRY_INSIDE);

			// A multi-point wholly on one side is itself (as for every geometry, section 5.1).
			if (d_result != PolygonPartitioner::GEOMETRY_INTERSECTING)
			{
				add_whole(d_result);
				return;
			}
			for (unsigned int side = 0; side < 2; ++side)
			{
				boost::optional<PolygonPartitioner::partitioned_geometry_seq_type &> &list =
						side ? d_partitioned_geometries_inside : d_partitioned_geometries_outside;
				if (list && !points[side].empty())
				{
					list->push_back(
							PolygonPartitioner::PartitionedGeometry(
									GPlatesMaths::MultiPointOnSphere::create(
										points[side].begin(),
										points[side].end()),
									d_cascade_provenance
											? make_table(provenances[side])
											: provenance_ptr_type()));
				}
			}
		}

		virtual
		void
		visit_point_on_sphere(
				GPlatesMaths::PointGeometryOnSphere::non_null_ptr_to_const_type point_on_sphere)
		{
			d_result = d_polygon_partitioner.partition_point(point_on_sphere->position());

			// A point on the boundary is inside.
			add_whole(d_result == PolygonPartitioner::GEOMETRY_OUTSIDE
					? PolygonPartitioner::GEOMETRY_OUTSIDE
					: PolygonPartitioner::GEOMETRY_INSIDE);
		}

		virtual
		void
		visit_polygon_on_sphere(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
		{
			if (!d_partitioned_geometries_inside &&
				!d_partitioned_geometries_outside)
			{
				d_result = d_polygon_partitioner.partition_polygon(polygon_on_sphere);
				return;
			}

			std::vector<Overlay::OutputPolygon> outputs[2];  // outside, inside
			d_result = overlay_polygon(
					*polygon_on_sphere,
					*d_polygon_partitioner.get_partitioning_polygon(),
					get_overlay_options(),
					outputs[1],
					outputs[0]);
			if (d_result != PolygonPartitioner::GEOMETRY_INTERSECTING)
			{
				add_whole(d_result);
				return;
			}

			for (unsigned int side = 0; side < 2; ++side)
			{
				boost::optional<PolygonPartitioner::partitioned_geometry_seq_type &> &list =
						side ? d_partitioned_geometries_inside : d_partitioned_geometries_outside;
				if (!list)
				{
					continue;
				}
				for (const Overlay::OutputPolygon &output : outputs[side])
				{
					list->push_back(
							PolygonPartitioner::PartitionedGeometry(
									Overlay::create_polygon(output),
									d_cascade_provenance
											? get_polygon_provenance(output)
											: provenance_ptr_type()));
				}
			}
		}

		virtual
		void
		visit_polyline_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
		{
			if (!d_partitioned_geometries_inside &&
				!d_partitioned_geometries_outside)
			{
				d_result = d_polygon_partitioner.partition_polyline(polyline_on_sphere);
				return;
			}

			std::vector<Overlay::OutputPolyline> runs;
			d_result = overlay_polyline(
					*polyline_on_sphere,
					*d_polygon_partitioner.get_partitioning_polygon(),
					get_overlay_options(),
					runs);
			if (d_result != PolygonPartitioner::GEOMETRY_INTERSECTING)
			{
				add_whole(d_result);
				return;
			}

			for (const Overlay::OutputPolyline &run : runs)
			{
				boost::optional<partitioned_geometry_seq_type &> &list = run.inside
						? d_partitioned_geometries_inside
						: d_partitioned_geometries_outside;
				if (list)
				{
					list->push_back(
							PolygonPartitioner::PartitionedGeometry(
									GPlatesMaths::PolylineOnSphere::create(
										run.points.begin(),
										run.points.end()),
									d_cascade_provenance
											? make_table(run.provenances)
											: provenance_ptr_type()));
				}
			}
		}

	private:
		const PolygonPartitioner &d_polygon_partitioner;
		provenance_ptr_type d_input_provenance;
		const PolygonPartitioner::CascadeProvenance *d_cascade_provenance;
		boost::optional<partitioned_geometry_seq_type &> d_partitioned_geometries_inside;
		boost::optional<partitioned_geometry_seq_type &> d_partitioned_geometries_outside;

		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> d_geometry;
		PolygonPartitioner::Result d_result;


		Overlay::Options
		get_overlay_options() const
		{
			Overlay::Options options;
			options.clip_speed = d_polygon_partitioner.get_partition_point_speed_and_memory();
			options.sliver_min_width = d_polygon_partitioner.get_sliver_min_width();
			if (d_cascade_provenance)
			{
				options.want_provenance = true;
				if (d_input_provenance)
				{
					options.subject_provenance = d_input_provenance.get();
					options.original_subject = d_cascade_provenance->original_geometry;
				}
				options.cascade_clips = d_cascade_provenance->partitioning_polygons;
				options.clip_index = d_cascade_provenance->clip_index;
			}
			return options;
		}

		//! The provenance of the input's point @a index (its own vertex, if it has no table).
		Overlay::Provenance
		input_point_provenance(
				unsigned int index) const
		{
			if (d_input_provenance)
			{
				return (*d_input_provenance)[index];
			}
			Overlay::Provenance provenance;
			provenance.subject = Overlay::Location{ 0, index, boost::none };
			return provenance;
		}

		//! The input itself, in the list of @a side (inside or outside), with its own table.
		void
		add_whole(
				PolygonPartitioner::Result side)
		{
			boost::optional<PolygonPartitioner::partitioned_geometry_seq_type &> &list =
					(side == PolygonPartitioner::GEOMETRY_INSIDE)
							? d_partitioned_geometries_inside
							: d_partitioned_geometries_outside;
			if (!list)
			{
				return;
			}
			provenance_ptr_type provenance;
			if (d_cascade_provenance)
			{
				provenance = d_input_provenance
						? d_input_provenance
						: PolygonPartitioner::create_identity_provenance(**d_geometry);
			}
			list->push_back(PolygonPartitioner::PartitionedGeometry(*d_geometry, provenance));
		}
	};


	/**
	 * The identity provenance of a geometry's points (each its own vertex, ring by ring).
	 */
	class IdentityProvenance :
			public GPlatesMaths::ConstGeometryOnSphereVisitor
	{
	public:
		provenance_seq_type provenance;

	protected:
		virtual
		void
		visit_multi_point_on_sphere(
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
		{
			add_ring(0, multi_point_on_sphere->number_of_points());
		}

		virtual
		void
		visit_point_on_sphere(
				GPlatesMaths::PointGeometryOnSphere::non_null_ptr_to_const_type /*point_on_sphere*/)
		{
			add_ring(0, 1);
		}

		virtual
		void
		visit_polygon_on_sphere(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
		{
			add_ring(0, polygon_on_sphere->number_of_vertices_in_exterior_ring());
			for (unsigned int i = 0; i < polygon_on_sphere->number_of_interior_rings(); ++i)
			{
				add_ring(i + 1, polygon_on_sphere->number_of_vertices_in_interior_ring(i));
			}
		}

		virtual
		void
		visit_polyline_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
		{
			add_ring(0, polyline_on_sphere->number_of_vertices());
		}

	private:
		void
		add_ring(
				unsigned int ring,
				unsigned int num_points)
		{
			for (unsigned int index = 0; index < num_points; ++index)
			{
				Overlay::Provenance point;
				point.subject = Overlay::Location{ ring, index, boost::none };
				provenance.push_back(point);
			}
		}
	};
}


GPlatesMaths::PolygonPartitioner::PolygonPartitioner(
		const PolygonOnSphere::non_null_ptr_to_const_type &partitioning_polygon,
		PolygonOnSphere::PointInPolygonSpeedAndMemory partition_point_speed_and_memory,
		boost::optional<double> sliver_min_width) :
	d_partitioning_polygon(partitioning_polygon),
	d_partition_point_speed_and_memory(partition_point_speed_and_memory),
	d_sliver_min_width(sliver_min_width)
{
}


GPlatesMaths::PolygonPartitioner::Result
GPlatesMaths::PolygonPartitioner::partition_geometry(
		const GeometryOnSphere::non_null_ptr_to_const_type &geometry_to_be_partitioned,
		boost::optional<partitioned_geometry_seq_type &> partitioned_geometries_inside,
		boost::optional<partitioned_geometry_seq_type &> partitioned_geometries_outside) const
{
	GeometryPartitioner geometry_partitioner(
			*this,
			provenance_ptr_type(),
			nullptr,
			partitioned_geometries_inside,
			partitioned_geometries_outside);

	return geometry_partitioner.partition_geometry(geometry_to_be_partitioned);
}


GPlatesMaths::PolygonPartitioner::Result
GPlatesMaths::PolygonPartitioner::partition_geometry(
		const PartitionedGeometry &geometry_to_be_partitioned,
		boost::optional<partitioned_geometry_seq_type &> partitioned_geometries_inside,
		boost::optional<partitioned_geometry_seq_type &> partitioned_geometries_outside,
		const CascadeProvenance &cascade_provenance) const
{
	GeometryPartitioner geometry_partitioner(
			*this,
			geometry_to_be_partitioned.provenance,
			&cascade_provenance,
			partitioned_geometries_inside,
			partitioned_geometries_outside);

	return geometry_partitioner.partition_geometry(geometry_to_be_partitioned.geometry);
}


GPlatesMaths::PolygonPartitioner::Result
GPlatesMaths::PolygonPartitioner::partition_polyline(
		const PolylineOnSphere::non_null_ptr_to_const_type &polyline_to_be_partitioned,
		boost::optional<partitioned_polyline_seq_type &> partitioned_polylines_inside,
		boost::optional<partitioned_polyline_seq_type &> partitioned_polylines_outside) const
{
	PolygonOverlay::Options options;
	options.clip_speed = d_partition_point_speed_and_memory;
	std::vector<PolygonOverlay::OutputPolyline> runs;
	const Result result = overlay_polyline(
			*polyline_to_be_partitioned,
			*d_partitioning_polygon,
			options,
			runs);

	if (result != GEOMETRY_INTERSECTING)
	{
		boost::optional<partitioned_polyline_seq_type &> &list = (result == GEOMETRY_INSIDE)
				? partitioned_polylines_inside
				: partitioned_polylines_outside;
		if (list)
		{
			list->push_back(polyline_to_be_partitioned);
		}
		return result;
	}

	for (const PolygonOverlay::OutputPolyline &run : runs)
	{
		boost::optional<partitioned_polyline_seq_type &> &list =
				run.inside ? partitioned_polylines_inside : partitioned_polylines_outside;
		if (list)
		{
			list->push_back(PolylineOnSphere::create(run.points.begin(), run.points.end()));
		}
	}
	return result;
}


GPlatesMaths::PolygonPartitioner::Result
GPlatesMaths::PolygonPartitioner::partition_polygon(
		const PolygonOnSphere::non_null_ptr_to_const_type &polygon_to_be_partitioned,
		boost::optional<partitioned_polygon_seq_type &> partitioned_polygons_inside,
		boost::optional<partitioned_polygon_seq_type &> partitioned_polygons_outside) const
{
	PolygonOverlay::Options options;
	options.clip_speed = d_partition_point_speed_and_memory;
	options.sliver_min_width = d_sliver_min_width;
	std::vector<PolygonOverlay::OutputPolygon> inside;
	std::vector<PolygonOverlay::OutputPolygon> outside;
	const Result result = overlay_polygon(
			*polygon_to_be_partitioned,
			*d_partitioning_polygon,
			options,
			inside,
			outside);

	if (result != GEOMETRY_INTERSECTING)
	{
		boost::optional<partitioned_polygon_seq_type &> &list = (result == GEOMETRY_INSIDE)
				? partitioned_polygons_inside
				: partitioned_polygons_outside;
		if (list)
		{
			list->push_back(polygon_to_be_partitioned);
		}
		return result;
	}

	if (partitioned_polygons_inside)
	{
		for (const PolygonOverlay::OutputPolygon &output : inside)
		{
			partitioned_polygons_inside->push_back(PolygonOverlay::create_polygon(output));
		}
	}
	if (partitioned_polygons_outside)
	{
		for (const PolygonOverlay::OutputPolygon &output : outside)
		{
			partitioned_polygons_outside->push_back(PolygonOverlay::create_polygon(output));
		}
	}
	return result;
}


GPlatesMaths::PolygonPartitioner::Result
GPlatesMaths::PolygonPartitioner::partition_point(
		const PointOnSphere &point_to_be_partitioned) const
{
	return d_partitioning_polygon->is_point_in_polygon(
					point_to_be_partitioned,
					d_partition_point_speed_and_memory)
			? GEOMETRY_INSIDE
			: GEOMETRY_OUTSIDE;
}


GPlatesMaths::PolygonPartitioner::Result
GPlatesMaths::PolygonPartitioner::partition_multipoint(
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


boost::shared_ptr<const GPlatesMaths::PolygonPartitioner::provenance_seq_type>
GPlatesMaths::PolygonPartitioner::create_identity_provenance(
		const GeometryOnSphere &geometry)
{
	IdentityProvenance identity;
	geometry.accept_visitor(identity);
	return boost::make_shared<const provenance_seq_type>(identity.provenance);
}
