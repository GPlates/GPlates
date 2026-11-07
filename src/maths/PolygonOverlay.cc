/* $Id$ */

/**
 * \file
 * $Revision$
 * $Date$
 *
 * Copyright (C) 2026 The University of Sydney, Australia
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
#include <cmath>
#include <limits>
#include <map>
#include <ostream>
#include <sstream>
#include <unordered_map>
#include <utility>

#include "PolygonOverlay.h"

#include "AngularDistance.h"
#include "AngularExtent.h"
#include "GeometryDistance.h"
#include "LatLonPoint.h"
#include "MathsUtils.h"
#include "PointInPolygon.h"
#include "PolygonOrientation.h"
#include "SmallCircleBounds.h"
#include "UnitVector3D.h"
#include "Vector3D.h"

#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"


namespace GPlatesMaths
{
	namespace PolygonOverlay
	{
		namespace
		{
			/**
			 * The angle between two unit vectors, accurate near zero (an arc cosine of their
			 * dot product cannot resolve less than about 1.5e-8 radians, far above the
			 * overlay's scale).
			 */
			double
			angle_between(
					const UnitVector3D &a,
					const UnitVector3D &b)
			{
				return std::atan2(cross(a, b).magnitude().dval(), dot(a, b).dval());
			}


			/**
			 * The angle fraction along @a segment of a point on it, or within the thickness of it.
			 *
			 * The point is projected onto the segment's great circle first, as GeometryIntersect's
			 * ordering key is (a touching vertex can be up to the thickness off the segment). The
			 * result is clamped to [0, 1]. A zero-length segment gives 0.
			 */
			double
			ratio_along_segment(
					const GreatCircleArc &segment,
					const PointOnSphere &point)
			{
				if (segment.is_zero_length())
				{
					return 0.0;
				}

				const UnitVector3D &axis = segment.rotation_axis();
				const Vector3D p(point.position_vector());
				const Vector3D projected = p - dot(p, axis) * Vector3D(axis);
				if (projected.magSqrd().dval() <= 0)
				{
					return 0.0;
				}

				const UnitVector3D &start = segment.start_point().position_vector();
				const UnitVector3D &end = segment.end_point().position_vector();
				const UnitVector3D projected_unit = projected.get_normalisation();

				const double segment_angle = angle_between(start, end);
				if (segment_angle <= 0)
				{
					return 0.0;
				}

				// Signed angle from the start, positive towards the end (about the axis).
				const double along = std::atan2(
						dot(cross(start, projected_unit), axis).dval(),
						dot(start, projected_unit).dval());

				return (std::max)(0.0, (std::min)(1.0, along / segment_angle));
			}


			//! Whether a record names the start vertex of its segment on @a side.
			bool
			is_record_at_vertex(
					const GeometryIntersect::Intersection &record,
					Side side)
			{
				return side == SUBJECT
						? record.is_on_segment1_start()
						: record.is_on_segment2_start();
			}

			unsigned int
			record_segment(
					const GeometryIntersect::Intersection &record,
					Side side)
			{
				return side == SUBJECT ? record.segment_index1 : record.segment_index2;
			}


			/**
			 * The most floating rays at one vertex whose placements are enumerated (more keep the
			 * measured order; reached by one generated case, 'shared#2240 seed 12', which the
			 * prototype comparison's BundleRule test pins).
			 */
			const unsigned int BUNDLE_CAP = 6;


			/**
			 * Whether two points are the same stored position, coordinate for coordinate.
			 *
			 * PointOnSphere's 'operator==' is not this: it compares the dot product with a
			 * tolerance of 1e-12, so points about 1.4e-6 radians apart (the thickness) compare
			 * equal.
			 */
			bool
			is_same_position(
					const PointOnSphere &a,
					const PointOnSphere &b)
			{
				const UnitVector3D &u = a.position_vector();
				const UnitVector3D &v = b.position_vector();
				return u.x().dval() == v.x().dval() &&
						u.y().dval() == v.y().dval() &&
						u.z().dval() == v.z().dval();
			}


			/**
			 * Union-find over record indices, by path halving.
			 */
			unsigned int
			find_root(
					std::vector<unsigned int> &parent,
					unsigned int index)
			{
				while (parent[index] != index)
				{
					parent[index] = parent[parent[index]];
					index = parent[index];
				}
				return index;
			}


			/**
			 * The hemisphere check's reference point's distance from both inputs' outlines: ten
			 * times the thickness, so that its point-in-polygon tests, threshold off, have a
			 * margin.
			 */
			const double HEMISPHERE_CHECK_MARGIN = 10.0;
		}
	}
}


boost::optional<bool>
GPlatesMaths::PolygonOverlay::is_ring_inside_ring(
		const std::vector<PointOnSphere> &ring_points,
		const PolygonOnSphere &other_ring,
		const AngularExtent &margin)
{
	boost::optional<PointOnSphere> farthest;
	double farthest_distance = 0;
	const auto consider = [&](const PointOnSphere &point)
	{
		const boost::optional<AngularDistance> distance =
				minimum_distance(point, other_ring, false, margin);
		const double angle = distance
				? distance->calculate_angle().dval()
				: (std::numeric_limits<double>::max)();  // farther than the margin
		if (angle > farthest_distance)
		{
			farthest = point;
			farthest_distance = angle;
		}
		return !distance;
	};
	for (const PointOnSphere &point : ring_points)
	{
		if (consider(point))
		{
			return other_ring.is_point_in_polygon(
					point, PolygonOnSphere::ADAPTIVE, false);
		}
	}
	for (unsigned int j = 0; j < ring_points.size(); ++j)
	{
		const Vector3D sum =
				Vector3D(ring_points[j].position_vector()) +
				Vector3D(ring_points[(j + 1) % ring_points.size()].position_vector());
		if (sum.magSqrd().dval() > 0)
		{
			consider(PointOnSphere(sum.get_normalisation()));
		}
	}
	if (!farthest)
	{
		return boost::none;
	}
	return other_ring.is_point_in_polygon(*farthest, PolygonOnSphere::ADAPTIVE, false);
}


double
GPlatesMaths::PolygonOverlay::get_mean_width(
		const std::vector<PointOnSphere> &ring,
		double *area)
{
	// A fan of signed triangles from an apex, each by Van Oosterom and Strackee's formula: the sum
	// is the enclosed area modulo 4 pi for any apex no ring point is antipodal to, and that is not
	// in the plane of a ring lying on one great circle (every triangle is then degenerate, each
	// term 0 or 2 pi by the sign of its denominator, and the sum is not the area: the vertices'
	// mean direction measured a 36-point equator ring 0, and the filter dropped a hemisphere).
	// So the apex is the ring's vector area, sum(a_i x a_(i+1)): the plane's normal for a ring on
	// a great circle, near the centre for any other. Where that is rounding (a ring of no area,
	// whichever apex is used), the vertices' mean direction, or the first vertex if that cancels
	// too. The perimeter by atan2, which keeps a short edge's precision.
	if (ring.empty())
	{
		if (area)
		{
			*area = 0.0;
		}
		return 0.0;
	}
	Vector3D vector_area(0, 0, 0);
	Vector3D sum(0, 0, 0);
	for (unsigned int i = 0; i < ring.size(); ++i)
	{
		const UnitVector3D &a = ring[i].position_vector();
		vector_area = vector_area + cross(a, ring[(i + 1) % ring.size()].position_vector());
		sum = sum + Vector3D(a);
	}
	// A cross product's components carry rounding of about 1e-16 an edge.
	const Vector3D apex =
			(vector_area.magSqrd() > 1e-18) ? Vector3D(vector_area.get_normalisation())
			: (sum.magSqrd() > 1e-12) ? Vector3D(sum.get_normalisation())
			: Vector3D(ring.front().position_vector());

	double fan_area = 0.0;
	double perimeter = 0.0;
	for (unsigned int i = 0; i < ring.size(); ++i)
	{
		const Vector3D a(ring[i].position_vector());
		const Vector3D b(ring[(i + 1) % ring.size()].position_vector());
		const Vector3D a_cross_b = cross(a, b);
		perimeter += std::atan2(std::sqrt(a_cross_b.magSqrd().dval()), dot(a, b).dval());
		fan_area += 2 * std::atan2(
				dot(apex, a_cross_b).dval(),
				1 + dot(apex, a).dval() + dot(a, b).dval() + dot(b, apex).dval());
	}
	double smaller_side = std::fmod(fan_area, 4 * PI);
	if (smaller_side < 0)
	{
		smaller_side += 4 * PI;
	}
	smaller_side = (std::min)(smaller_side, 4 * PI - smaller_side);

	if (area)
	{
		*area = smaller_side;
	}
	return (perimeter > 0) ? 2 * smaller_side / perimeter : 0.0;
}


GPlatesMaths::PolygonOverlay::Overlay::Overlay(
		const PolygonOnSphere &subject,
		const PolygonOnSphere &clip,
		const Options &options) :
	d_subject_polygon(&subject),
	d_subject_polyline(nullptr),
	d_clip(clip),
	d_options(options)
{
	// The subject goes first: an SS record is placed at the first geometry's vertex, which is also
	// the output position of an overlay vertex holding a subject vertex (so the two agree).
	GeometryIntersect::intersect(d_graph, subject, clip);

	add_polygon_rings(SUBJECT, subject);
	add_polygon_rings(CLIP, clip);

	build();
}


GPlatesMaths::PolygonOverlay::Overlay::Overlay(
		const PolylineOnSphere &subject,
		const PolygonOnSphere &clip,
		const Options &options) :
	d_subject_polygon(nullptr),
	d_subject_polyline(&subject),
	d_clip(clip),
	d_options(options)
{
	GeometryIntersect::intersect(d_graph, subject, clip);

	add_polyline_ring(subject);
	add_polygon_rings(CLIP, clip);

	build();
}


void
GPlatesMaths::PolygonOverlay::Overlay::add_polygon_rings(
		Side side,
		const PolygonOnSphere &polygon)
{
	d_side_rings_begin[side] = d_rings.size();

	RingView exterior;
	exterior.side = side;
	exterior.ring_index = 0;
	exterior.first_segment_index = 0;
	exterior.num_segments = polygon.number_of_segments_in_exterior_ring();
	exterior.closed = true;
	exterior.segments = &*polygon.exterior_ring_begin();
	exterior.first_stop = exterior.end_stop = 0;
	d_rings.push_back(exterior);

	unsigned int first_segment_index = exterior.num_segments;
	const unsigned int num_interior_rings = polygon.number_of_interior_rings();
	for (unsigned int interior_index = 0; interior_index < num_interior_rings; ++interior_index)
	{
		RingView interior;
		interior.side = side;
		interior.ring_index = interior_index + 1;
		interior.first_segment_index = first_segment_index;
		interior.num_segments = polygon.number_of_segments_in_interior_ring(interior_index);
		interior.closed = true;
		interior.segments = &*polygon.interior_ring_begin(interior_index);
		interior.first_stop = interior.end_stop = 0;
		d_rings.push_back(interior);

		first_segment_index += interior.num_segments;
	}

	d_side_rings_end[side] = d_rings.size();
}


void
GPlatesMaths::PolygonOverlay::Overlay::add_polyline_ring(
		const PolylineOnSphere &polyline)
{
	d_side_rings_begin[SUBJECT] = d_rings.size();

	RingView ring;
	ring.side = SUBJECT;
	ring.ring_index = 0;
	ring.first_segment_index = 0;
	ring.num_segments = polyline.number_of_segments();
	ring.closed = false;
	ring.segments = &*polyline.begin();
	ring.first_stop = ring.end_stop = 0;
	d_rings.push_back(ring);

	d_side_rings_end[SUBJECT] = d_rings.size();
}


unsigned int
GPlatesMaths::PolygonOverlay::Overlay::ring_of_segment(
		Side side,
		unsigned int segment_index) const
{
	for (unsigned int ring = d_side_rings_begin[side]; ring < d_side_rings_end[side]; ++ring)
	{
		const RingView &ring_view = d_rings[ring];
		// A polyline's last vertex is the start of the fictitious segment 'num_segments'.
		const unsigned int end_index = ring_view.first_segment_index + ring_view.num_vertices();
		if (segment_index < end_index)
		{
			return ring;
		}
	}

	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
}


void
GPlatesMaths::PolygonOverlay::Overlay::build()
{
	build_vertices();
	build_pieces();

	// Where an input touches itself on a stretch it shares with the other input, the stretch is
	// cut on one side only; cut the other side there too, and build again (once: the cuts make the
	// pieces common, so none is found the second time).
	if (find_self_touch_cuts())
	{
		d_stops.clear();
		d_stop_records.clear();
		d_pieces.clear();
		for (Vertex &vertex : d_vertices)
		{
			vertex.ends.clear();
		}
		d_diagnostics.internal_pieces = 0;
		d_diagnostics.spikes = 0;
		d_diagnostics.common_pairs = 0;
		d_diagnostics.common_ambiguous = 0;
		build_pieces();
	}

	build_rays();
}


void
GPlatesMaths::PolygonOverlay::Overlay::build_pieces()
{
	for (unsigned int ring = 0; ring < d_rings.size(); ++ring)
	{
		build_stops_and_pieces(ring);
	}

	apply_loop_and_spike_rules();
	find_common_pairs();
}


void
GPlatesMaths::PolygonOverlay::Overlay::build_vertices()
{
	const GeometryIntersect::intersection_seq_type &records = d_graph.unordered_intersections;
	const unsigned int num_records = records.size();

	//
	// Union-find over record indices. A record that names an original vertex is joined with the
	// first record that named the same vertex. Crossings name none, so each is its own vertex.
	//
	std::vector<unsigned int> parent(num_records);
	for (unsigned int r = 0; r < num_records; ++r)
	{
		parent[r] = r;
	}

	// Key: (side, global segment index of the named vertex) -> the first record naming it.
	std::map<std::pair<unsigned int, unsigned int>, unsigned int> first_record_naming;
	for (unsigned int r = 0; r < num_records; ++r)
	{
		for (unsigned int side = SUBJECT; side <= CLIP; ++side)
		{
			if (!is_record_at_vertex(records[r], static_cast<Side>(side)))
			{
				continue;
			}

			const std::pair<unsigned int, unsigned int> key(
					side,
					record_segment(records[r], static_cast<Side>(side)));
			auto inserted = first_record_naming.insert(std::make_pair(key, r));
			if (!inserted.second)
			{
				const unsigned int root_a = find_root(parent, r);
				const unsigned int root_b = find_root(parent, inserted.first->second);
				if (root_a != root_b)
				{
					// Keep the lower index as the root (so a group's root is its lowest record).
					if (root_a < root_b)
					{
						parent[root_b] = root_a;
					}
					else
					{
						parent[root_a] = root_b;
					}
				}
			}
		}
	}

	//
	// The groups, in order of their lowest record index, become the vertices. Since the root of
	// each group is its lowest record, visiting records in index order meets each root first.
	//
	d_vertex_of_record.assign(num_records, 0);
	std::vector<unsigned int> vertex_of_root(num_records, num_records);
	std::vector<std::vector<unsigned int>> vertex_record_lists;
	for (unsigned int r = 0; r < num_records; ++r)
	{
		const unsigned int root = find_root(parent, r);
		if (vertex_of_root[root] == num_records)
		{
			vertex_of_root[root] = vertex_record_lists.size();
			vertex_record_lists.push_back(std::vector<unsigned int>());
		}
		d_vertex_of_record[r] = vertex_of_root[root];
		vertex_record_lists[vertex_of_root[root]].push_back(r);
	}

	d_vertices.reserve(vertex_record_lists.size());
	d_vertex_records.reserve(num_records);
	for (const std::vector<unsigned int> &vertex_record_list : vertex_record_lists)
	{
		Vertex vertex{
				static_cast<unsigned int>(d_vertex_records.size()),
				static_cast<unsigned int>(vertex_record_list.size()),
				boost::none,
				boost::none,
				records[vertex_record_list.front()].position,
				std::vector<End>()};

		for (unsigned int r : vertex_record_list)
		{
			d_vertex_records.push_back(r);

			for (unsigned int side = SUBJECT; side <= CLIP; ++side)
			{
				boost::optional<RingVertex> &ring_vertex =
						(side == SUBJECT) ? vertex.subject_vertex : vertex.clip_vertex;
				if (ring_vertex || !is_record_at_vertex(records[r], static_cast<Side>(side)))
				{
					continue;
				}

				const unsigned int segment_index =
						record_segment(records[r], static_cast<Side>(side));
				const unsigned int ring = ring_of_segment(static_cast<Side>(side), segment_index);
				ring_vertex = RingVertex(
						d_rings[ring].ring_index,
						segment_index - d_rings[ring].first_segment_index);
			}
		}

		// The output position: the subject's vertex if the group holds one (so the subject's
		// vertices stay exact, for scalar coverages), else the clip's, else the record's.
		if (vertex.subject_vertex)
		{
			const RingView &ring =
					d_rings[d_side_rings_begin[SUBJECT] + vertex.subject_vertex->ring];
			vertex.position = ring.vertex(vertex.subject_vertex->index);
		}
		else if (vertex.clip_vertex)
		{
			const RingView &ring =
					d_rings[d_side_rings_begin[CLIP] + vertex.clip_vertex->ring];
			vertex.position = ring.vertex(vertex.clip_vertex->index);
		}

		d_vertices.push_back(vertex);
	}
}


void
GPlatesMaths::PolygonOverlay::Overlay::build_stops_and_pieces(
		unsigned int ring)
{
	RingView &ring_view = d_rings[ring];
	const Side side = ring_view.side;
	const GeometryIntersect::intersection_seq_type &records = d_graph.unordered_intersections;
	const std::vector<unsigned int> &ordered = (side == SUBJECT)
			? d_graph.geometry1_ordered_intersections
			: d_graph.geometry2_ordered_intersections;

	//
	// This ring's records, by ring-local segment (a polyline's last vertex is 'num_segments'). The
	// ordered sequence runs ring by ring, so the records of one segment keep their order along it.
	//
	const unsigned int num_vertices = ring_view.num_vertices();
	std::vector<std::vector<unsigned int>> at_vertex_records(num_vertices);
	std::vector<std::vector<unsigned int>> inside_segment_records(num_vertices);
	for (unsigned int r : ordered)
	{
		const unsigned int segment_index = record_segment(records[r], side);
		if (segment_index < ring_view.first_segment_index ||
			segment_index >= ring_view.first_segment_index + num_vertices)
		{
			continue;
		}

		const unsigned int local = segment_index - ring_view.first_segment_index;
		if (is_record_at_vertex(records[r], side))
		{
			at_vertex_records[local].push_back(r);
		}
		else
		{
			inside_segment_records[local].push_back(r);
		}
	}

	//
	// The stops, in ring order: per vertex, its records (one stop per distinct overlay vertex) or
	// the original vertex itself; then the records inside the segment leaving it. Two records of
	// different vertices tied along a segment (GeometryIntersect compares their angles as cosines
	// in doubles, resolving about 1.5e-8 radians near a segment's start, and sorts ties in
	// discovery order) come in either order: the piece between them is then a zero-length single
	// arc whose class is deduced (two crossings of a twist lying on the other input), and either
	// order is a valid output, differing only in ring structure.
	//
	// The self-touch cuts that follow the stop of vertex 'vertex' on segment 'k', if any.
	std::vector<std::tuple<unsigned int, unsigned int, unsigned int>> self_touch_cuts_added;
	const auto add_self_touch_cuts = [&](unsigned int k, unsigned int vertex)
	{
		const auto key = std::make_tuple(ring, k, vertex);
		const auto cuts = d_self_touch_cuts.find(key);
		if (cuts == d_self_touch_cuts.end() ||
			std::find(self_touch_cuts_added.begin(), self_touch_cuts_added.end(), key) !=
					self_touch_cuts_added.end())
		{
			return;  // none, or already added after another record of the vertex
		}
		self_touch_cuts_added.push_back(key);
		for (const std::pair<double, unsigned int> &cut : cuts->second)
		{
			// A stop of the vertex with no record of this ring (none names it).
			d_stops.push_back(Stop{
					Stop::RECORD,
					k,
					k,
					false,
					cut.second,
					static_cast<unsigned int>(d_stop_records.size()),
					0,
					d_vertices[cut.second].position,
					cut.first});
		}
	};

	ring_view.first_stop = d_stops.size();
	for (unsigned int k = 0; k < num_vertices; ++k)
	{
		const unsigned int segment_in = ring_view.closed
				? (k + num_vertices - 1) % num_vertices
				: (k == 0 ? 0 : k - 1);

		if (!at_vertex_records[k].empty())
		{
			// Every record at ring vertex 'k' names that vertex, so identity makes them one overlay
			// vertex, and they are one stop (a corner's two touches of one subject vertex, say).
			const unsigned int vertex = d_vertex_of_record[at_vertex_records[k].front()];
			d_stops.push_back(Stop{
					Stop::RECORD,
					segment_in,
					k,
					true,
					vertex,
					static_cast<unsigned int>(d_stop_records.size()),
					static_cast<unsigned int>(at_vertex_records[k].size()),
					ring_view.vertex(k),
					0.0});
			for (unsigned int r : at_vertex_records[k])
			{
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						d_vertex_of_record[r] == vertex,
						GPLATES_ASSERTION_SOURCE);
				d_stop_records.push_back(r);
			}
			add_self_touch_cuts(k, vertex);
		}
		else
		{
			const bool dangling = !ring_view.closed && (k == 0 || k == num_vertices - 1);
			d_stops.push_back(Stop{
					dangling ? Stop::DANGLING_END : Stop::ORIGINAL_VERTEX,
					segment_in,
					k,
					true,
					0,
					static_cast<unsigned int>(d_stop_records.size()),
					0,
					ring_view.vertex(k),
					0.0});
		}

		for (unsigned int r : inside_segment_records[k])
		{
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					k < ring_view.num_segments,
					GPLATES_ASSERTION_SOURCE);

			d_stops.push_back(Stop{
					Stop::RECORD,
					k,
					k,
					false,
					d_vertex_of_record[r],
					static_cast<unsigned int>(d_stop_records.size()),
					1,
					records[r].position,
					ratio_along_segment(ring_view.segments[k], records[r].position)});
			d_stop_records.push_back(r);
			add_self_touch_cuts(k, d_vertex_of_record[r]);
		}
	}
	ring_view.end_stop = d_stops.size();

	//
	// The pieces: between consecutive record stops (cyclically for a closed ring); for a polyline,
	// also between a dangling end and its nearest record stop. A ring with no record stop is one
	// closed piece.
	//
	std::vector<unsigned int> boundaries;
	for (unsigned int s = ring_view.first_stop; s < ring_view.end_stop; ++s)
	{
		if (d_stops[s].kind == Stop::RECORD ||
			d_stops[s].kind == Stop::DANGLING_END)
		{
			boundaries.push_back(s);
		}
	}

	const unsigned int num_ring_stops = ring_view.end_stop - ring_view.first_stop;
	const auto next_stop = [&](unsigned int s)
	{
		return (s + 1 == ring_view.end_stop) ? ring_view.first_stop : s + 1;
	};

	const auto add_piece = [&](unsigned int start, unsigned int end, bool closed)
	{
		Piece piece{
				ring, start, end, closed, false,
				boost::none, boost::none, boost::none, false, boost::none};

		unsigned int num_interior_stops = 0;
		if (closed)
		{
			// Every stop of the ring.
			num_interior_stops = num_ring_stops;
		}
		else
		{
			for (unsigned int s = next_stop(start); s != end; s = next_stop(s))
			{
				++num_interior_stops;
			}
			if (start == end)
			{
				// One record stop on the ring: the piece goes all the way round, back to itself.
				num_interior_stops = num_ring_stops - 1;
			}
		}

		unsigned int s = next_stop(start);
		for (unsigned int n = 0; n < num_interior_stops; ++n, s = next_stop(s))
		{
			if (d_stops[s].kind == Stop::ORIGINAL_VERTEX)
			{
				piece.has_interior_vertex = true;
			}
		}

		if (!closed &&
			num_interior_stops == 0 &&
			d_stops[start].segment_out == d_stops[end].segment_in)
		{
			piece.single_arc_segment = d_stops[start].segment_out;
		}

		if (!closed)
		{
			if (d_stops[start].kind == Stop::RECORD)
			{
				piece.start_vertex = d_stops[start].vertex;
			}
			if (d_stops[end].kind == Stop::RECORD)
			{
				piece.end_vertex = d_stops[end].vertex;
			}
		}

		const unsigned int piece_index = d_pieces.size();
		d_pieces.push_back(piece);

		if (piece.start_vertex)
		{
			d_vertices[*piece.start_vertex].ends.push_back(End(piece_index, false));
		}
		if (piece.end_vertex)
		{
			d_vertices[*piece.end_vertex].ends.push_back(End(piece_index, true));
		}
	};

	if (ring_view.closed)
	{
		if (boundaries.empty())
		{
			add_piece(ring_view.first_stop, ring_view.first_stop, true/*closed*/);
			return;
		}

		for (unsigned int b = 0; b < boundaries.size(); ++b)
		{
			add_piece(boundaries[b], boundaries[(b + 1) % boundaries.size()], false/*closed*/);
		}
	}
	else
	{
		// A polyline: its first and last stops are always boundaries (a record or a dangling end).
		if (boundaries.size() == 2 &&
			d_stops[boundaries[0]].kind == Stop::DANGLING_END &&
			d_stops[boundaries[1]].kind == Stop::DANGLING_END)
		{
			// No record: the whole polyline is one piece.
			Piece piece{
					ring, boundaries[0], boundaries[1], true, false,
					boost::none, boost::none, boost::none, false, boost::none};
			for (unsigned int s = boundaries[0] + 1; s < boundaries[1]; ++s)
			{
				if (d_stops[s].kind == Stop::ORIGINAL_VERTEX)
				{
					piece.has_interior_vertex = true;
				}
			}
			d_pieces.push_back(piece);
			return;
		}

		for (unsigned int b = 0; b + 1 < boundaries.size(); ++b)
		{
			// No wrap-around.
			add_piece(boundaries[b], boundaries[b + 1], false/*closed*/);
		}
	}
}


std::vector<unsigned int>
GPlatesMaths::PolygonOverlay::Overlay::get_stop_records(
		const Stop &stop) const
{
	return std::vector<unsigned int>(
			d_stop_records.begin() + stop.first_record,
			d_stop_records.begin() + stop.first_record + stop.num_records);
}


std::vector<unsigned int>
GPlatesMaths::PolygonOverlay::Overlay::get_vertex_records(
		const Vertex &vertex) const
{
	return std::vector<unsigned int>(
			d_vertex_records.begin() + vertex.first_record,
			d_vertex_records.begin() + vertex.first_record + vertex.num_records);
}


std::vector<GPlatesMaths::PointOnSphere>
GPlatesMaths::PolygonOverlay::Overlay::get_piece_points(
		const Piece &piece) const
{
	const RingView &ring_view = d_rings[piece.ring];

	std::vector<PointOnSphere> points;
	if (piece.closed && ring_view.closed)
	{
		for (unsigned int s = ring_view.first_stop; s < ring_view.end_stop; ++s)
		{
			points.push_back(d_stops[s].position);
		}
		return points;
	}

	unsigned int s = piece.start_stop;
	points.push_back(d_stops[s].position);
	do
	{
		s = (s + 1 == ring_view.end_stop) ? ring_view.first_stop : s + 1;
		points.push_back(d_stops[s].position);
	}
	while (s != piece.end_stop);

	return points;
}


void
GPlatesMaths::PolygonOverlay::Overlay::apply_loop_and_spike_rules()
{
	//
	// The loop rule: a single arc (no interior original vertex) from a vertex back to itself lies
	// within the vertex's identity group, so it bounds nothing. A loop with an interior original
	// vertex (the wedge's tip, x, apex, x) is real geometry and stays.
	//
	for (Piece &piece : d_pieces)
	{
		if (!piece.closed &&
			!piece.has_interior_vertex &&
			piece.start_vertex &&
			piece.end_vertex &&
			*piece.start_vertex == *piece.end_vertex)
		{
			piece.internal = true;
			++d_diagnostics.internal_pieces;
		}
	}

	//
	// The spike rule: two consecutive single arcs of one ring going from vertex A to vertex B and
	// straight back to A are both internal (the hairpin they bound is no wider than the identity
	// groups at its ends). The pieces are stored ring by ring, in ring order.
	//
	unsigned int ring_first_piece = 0;
	while (ring_first_piece < d_pieces.size())
	{
		unsigned int ring_end_piece = ring_first_piece + 1;
		while (ring_end_piece < d_pieces.size() &&
			d_pieces[ring_end_piece].ring == d_pieces[ring_first_piece].ring)
		{
			++ring_end_piece;
		}

		// Consecutive pieces of a closed ring wrap round; a polyline's last and first do not meet.
		const unsigned int num_ring_pieces = ring_end_piece - ring_first_piece;
		const unsigned int num_pairs = d_rings[d_pieces[ring_first_piece].ring].closed
				? num_ring_pieces
				: num_ring_pieces - 1;
		for (unsigned int i = 0; i < num_pairs; ++i)
		{
			Piece &a = d_pieces[ring_first_piece + i];
			Piece &b = d_pieces[ring_first_piece + (i + 1) % num_ring_pieces];
			if (&a == &b ||
				a.closed || b.closed ||
				a.has_interior_vertex || b.has_interior_vertex ||
				a.internal || b.internal)
			{
				continue;
			}

			// A dangling end (of a polyline) is not a vertex.
			if (!a.start_vertex || !a.end_vertex || !b.start_vertex || !b.end_vertex)
			{
				continue;
			}

			if (*a.end_vertex == *b.start_vertex &&
				*b.end_vertex == *a.start_vertex &&
				*a.start_vertex != *a.end_vertex)
			{
				// Only where the other input passes the tip at most once (two ends of its pieces
				// not yet internal): with two passages the tip is a self-touch of the other input,
				// the hairpin can lie between them, and removing it would remove a crossing there
				// (spherical-geometry.md section 5.2, the spike rule).
				const Side side = d_rings[a.ring].side;
				unsigned int other_ends = 0;
				for (const End &end : d_vertices[*a.end_vertex].ends)
				{
					const Piece &piece = d_pieces[end.piece];
					if (d_rings[piece.ring].side != side && !piece.internal)
					{
						++other_ends;
					}
				}
				if (other_ends > 2)
				{
					continue;
				}

				a.internal = b.internal = true;
				d_diagnostics.internal_pieces += 2;
				++d_diagnostics.spikes;
			}
		}

		ring_first_piece = ring_end_piece;
	}

	for (Vertex &vertex : d_vertices)
	{
		vertex.ends.erase(
				std::remove_if(
						vertex.ends.begin(),
						vertex.ends.end(),
						[this](const End &end) { return d_pieces[end.piece].internal; }),
				vertex.ends.end());
	}
}


void
GPlatesMaths::PolygonOverlay::Overlay::find_common_pairs()
{
	//
	// A subject piece and a clip piece that are both single arcs between the same two (distinct)
	// vertices run together: an original vertex of either inside the stretch would itself be a
	// record. Any other mix between one pair of vertices pairs nothing, and is counted.
	//
	struct PairPieces
	{
		std::vector<unsigned int> side_pieces[2];
	};

	const auto pair_key = [](unsigned int a, unsigned int b)
	{
		return (static_cast<unsigned long long>((std::min)(a, b)) << 32) | (std::max)(a, b);
	};

	std::unordered_map<unsigned long long, PairPieces> pairs;
	// The keys in the order first seen, so that the result is deterministic.
	std::vector<unsigned long long> key_order;
	for (unsigned int p = 0; p < d_pieces.size(); ++p)
	{
		const Piece &piece = d_pieces[p];
		if (piece.closed ||
			piece.internal ||
			piece.has_interior_vertex ||
			!piece.start_vertex ||
			!piece.end_vertex ||
			*piece.start_vertex == *piece.end_vertex)
		{
			continue;
		}

		const unsigned long long key = pair_key(*piece.start_vertex, *piece.end_vertex);
		auto inserted = pairs.insert(std::make_pair(key, PairPieces()));
		if (inserted.second)
		{
			key_order.push_back(key);
		}
		inserted.first->second.side_pieces[d_rings[piece.ring].side].push_back(p);
	}

	for (unsigned long long key : key_order)
	{
		const PairPieces &pair = pairs[key];
		if (pair.side_pieces[SUBJECT].size() == 1 &&
			pair.side_pieces[CLIP].size() == 1)
		{
			const unsigned int subject_piece = pair.side_pieces[SUBJECT][0];
			const unsigned int clip_piece = pair.side_pieces[CLIP][0];
			d_pieces[subject_piece].common = clip_piece;
			d_pieces[clip_piece].common = subject_piece;
			++d_diagnostics.common_pairs;
		}
		else if (!pair.side_pieces[SUBJECT].empty() &&
			!pair.side_pieces[CLIP].empty())
		{
			++d_diagnostics.common_ambiguous;
		}
	}
}


bool
GPlatesMaths::PolygonOverlay::Overlay::find_self_touch_cuts()
{
	//
	// A stretch the two inputs share is cut by every record on it, on both sides, so its pieces
	// pair as common: but GeometryIntersect never intersects an input with itself, so where one
	// input touches itself on the stretch (a hole's tip on its own exterior's edge, which the other
	// input shares), the other input's segment is cut there (a record names the tip) and the
	// touched segment is not (section 5.2). Its one piece then faces a chain of the
	// other input's pieces, neither pairs, and they are band pieces with arbitrary classes, from
	// which the sector walk can deduce wrong classes that every check accepts. So: a single-arc
	// piece with no partner, whose two vertices the other input joins by a chain of single-arc
	// pieces through vertices within the thickness of the piece's segment and in order along it,
	// is cut at those vertices.
	//
	const double h = GeometryIntersect::Intersection::get_on_segment_start_threshold_sine();
	const unsigned int MAX_CHAIN = 16;
	const double ALONG_ROUNDING = 1e-12;  // radians

	d_self_touch_cuts.clear();
	for (unsigned int p = 0; p < d_pieces.size(); ++p)
	{
		const Piece &piece = d_pieces[p];
		if (piece.closed ||
			piece.internal ||
			piece.common ||
			!piece.single_arc_segment ||
			!piece.start_vertex ||
			!piece.end_vertex ||
			*piece.start_vertex == *piece.end_vertex)
		{
			continue;
		}
		const RingView &ring_view = d_rings[piece.ring];
		const GreatCircleArc &segment = ring_view.segments[*piece.single_arc_segment];
		const UnitVector3D &segment_start = segment.start_point().position_vector();
		// The segment's own plane, even for a segment GreatCircleArc calls zero length (under
		// about 1e-6 radians, where it has no rotation axis): the 'd4' catalogue's short edge.
		// (In doubles: Vector3D's normalisation refuses so short a normal.)
		const Vector3D normal = cross(segment_start, segment.end_point().position_vector());
		const double normal_length = std::sqrt(normal.magSqrd().dval());
		if (!(normal_length > 0))
		{
			continue;
		}
		const double axis[3] = {
				normal.x().dval() / normal_length,
				normal.y().dval() / normal_length,
				normal.z().dval() / normal_length };
		const auto dot_axis = [&axis](const Vector3D &v)
		{
			return axis[0] * v.x().dval() + axis[1] * v.y().dval() + axis[2] * v.z().dval();
		};

		// The signed angle along the segment's great circle, from its start, of a point's
		// projection onto it.
		const auto along = [&](const PointOnSphere &point)
		{
			const UnitVector3D &x = point.position_vector();
			return std::atan2(dot_axis(cross(segment_start, x)), dot(segment_start, x).dval());
		};
		const double along_start = along(d_stops[piece.start_stop].position);
		const double along_end = along(d_stops[piece.end_stop].position);
		if (!(along_start < along_end))
		{
			continue;
		}

		// A chain of the other input's pieces from the start vertex to the end vertex, each step
		// further along the segment and within its thickness (a depth-first search; chains are a
		// few pieces long).
		const Side other_side = (ring_view.side == SUBJECT) ? CLIP : SUBJECT;
		std::vector<unsigned int> chain{ *piece.start_vertex };
		std::vector<double> chain_along{ along_start };
		std::vector<unsigned int> next_end_index{ 0 };
		bool found = false;
		while (!chain.empty() && !found)
		{
			const Vertex &vertex = d_vertices[chain.back()];
			if (next_end_index.back() >= vertex.ends.size() || chain.size() > MAX_CHAIN)
			{
				chain.pop_back();
				chain_along.pop_back();
				next_end_index.pop_back();
				continue;
			}
			const End end = vertex.ends[next_end_index.back()++];
			const Piece &other = d_pieces[end.piece];
			if (d_rings[other.ring].side != other_side ||
				other.common ||
				!other.single_arc_segment ||
				!other.start_vertex ||
				!other.end_vertex)
			{
				continue;
			}
			const unsigned int next = end.at_end ? *other.start_vertex : *other.end_vertex;
			if (next == *piece.end_vertex)
			{
				found = chain.size() > 1;  // one piece would have paired
				continue;
			}
			const PointOnSphere &position = d_vertices[next].position;
			const double next_along = along(position);
			// Further along by more than rounding, so that a vertex at a segment's end (where two
			// pieces of the ring meet) is not taken for one inside it, one way in doubles and the
			// other in the prototype's arithmetic.
			if (next_along <= chain_along.back() + ALONG_ROUNDING ||
				next_along >= along_end - ALONG_ROUNDING ||
				std::fabs(dot_axis(Vector3D(position.position_vector()))) > h)
			{
				continue;
			}
			chain.push_back(next);
			chain_along.push_back(next_along);
			next_end_index.push_back(0);
		}
		if (!found)
		{
			continue;
		}

		const double segment_angle =
				angle_between(segment_start, segment.end_point().position_vector());
		std::vector<std::pair<double, unsigned int>> &cuts = d_self_touch_cuts[std::make_tuple(
				piece.ring, *piece.single_arc_segment, *piece.start_vertex)];
		for (unsigned int i = 1; i < chain.size(); ++i)
		{
			cuts.push_back(std::make_pair(chain_along[i] / segment_angle, chain[i]));
			++d_diagnostics.self_touch_cuts;
		}
	}
	return !d_self_touch_cuts.empty();
}


bool
GPlatesMaths::PolygonOverlay::Overlay::measure_ray(
		const Vertex &vertex,
		const End &end,
		Ray &ray) const
{
	const Piece &piece = d_pieces[end.piece];
	const RingView &ring_view = d_rings[piece.ring];

	// The piece's stops, leaving the vertex.
	std::vector<unsigned int> stops(1, piece.start_stop);
	for (unsigned int s = piece.start_stop; stops.size() == 1 || s != piece.end_stop; )
	{
		s = (s + 1 == ring_view.end_stop) ? ring_view.first_stop : s + 1;
		stops.push_back(s);
	}
	if (end.at_end)
	{
		std::reverse(stops.begin(), stops.end());
	}

	const UnitVector3D &centre = vertex.position.position_vector();

	//
	// The first arc of non-zero length: its direction is the exact tangent of its segment's great
	// circle at the arc's start (a zero-length segment has no rotation axis). Arriving at the
	// piece's end, an arc between ring-order stops 'i - 1' and 'i' lies on stop i's arriving
	// segment, and is walked backwards.
	//
	for (unsigned int i = 0; i + 1 < stops.size(); ++i)
	{
		const Stop &from = d_stops[stops[i]];
		const Stop &to = d_stops[stops[i + 1]];
		const GreatCircleArc &segment =
				ring_view.segments[end.at_end ? from.segment_in : from.segment_out];
		if (segment.is_zero_length())
		{
			continue;
		}

		const UnitVector3D &start = from.position.position_vector();
		const Vector3D tangent = cross(segment.rotation_axis(), start);
		ray.direction = end.at_end ? -tangent : tangent;

		// The cone (sections 1 and 5.2): 0 when the arc starts at the vertex's output
		// position (the same stored point) on its segment's great circle, pi when the arc is no
		// longer than its offset 'delta', else the planar bound atan2(delta, L - delta). At these
		// scales the spherical correction to the bound is far below rounding.
		//
		// The offset is the start's distance from the output position plus how far the arc's start
		// and far end lie off the segment's great circle (prototype configuration 'r5o'):
		// a stop at a touch record of the other polygon's vertex (and a self-touch cut) is at that
		// vertex, up to the thickness off this ring's segment, so the arc between start and far end
		// can leave tens of degrees from the segment's tangent, which a cone of the distance alone
		// did not bound (in four dense shared-edge cases of the hunt the true order was then not
		// admissible). Only those stops are measured: a vertex of this ring, and a crossing, lie on
		// the segment by construction, as the prototype's exact zero. (Not by a threshold: a
		// crossing's stored position, the normalised cross product of two nearly parallel planes,
		// lies off them by up to about 2.4e-11 radians in a dense shared edge, and a threshold of
		// 1e-12 gave those rays cones of up to 1.5e-5 radians the prototype gives 0.)
		const UnitVector3D &axis = segment.rotation_axis();
		const auto offset_from_segment = [this, &axis](const Stop &stop)
		{
			if (stop.at_vertex ||
				(stop.num_records != 0 &&
					d_graph.unordered_intersections[d_stop_records[stop.first_record]].type ==
							GeometryIntersect::Intersection::SEGMENTS_CROSS))
			{
				return 0.0;
			}
			const double sine = std::fabs(dot(axis, stop.position.position_vector()).dval());
			return std::asin((std::min)(1.0, sine));
		};
		const UnitVector3D &far = to.position.position_vector();
		const double from_output_position = is_same_position(from.position, vertex.position)
				? 0.0
				: angle_between(centre, start);
		ray.delta = from_output_position + offset_from_segment(from) + offset_from_segment(to);
		ray.first_arc_length = angle_between(start, far);
		ray.reach = angle_between(centre, far);
		if (ray.delta == 0.0)
		{
			ray.cone = 0.0;
		}
		else if (ray.first_arc_length <= ray.delta)
		{
			ray.cone = PI;
		}
		else
		{
			ray.cone = std::atan2(ray.delta, ray.first_arc_length - ray.delta);
		}
		return true;
	}

	// Every arc of the piece lies on a zero-length segment: no direction, and a cone of pi (it
	// floats at a vertex of six or more ends, and bounds no sector an order is needed for below).
	ray.direction = boost::none;
	ray.delta = angle_between(centre, d_stops[stops.front()].position.position_vector());
	ray.first_arc_length = 0.0;
	ray.reach = angle_between(centre, d_stops[stops.back()].position.position_vector());
	ray.cone = PI;
	return false;
}


std::vector<GPlatesMaths::PolygonOverlay::Ray>
GPlatesMaths::PolygonOverlay::Overlay::sort_rays_anticlockwise(
		const PointOnSphere &centre_point,
		const std::vector<Ray> &rays)
{
	//
	// The measured rotational order, anticlockwise about the centre seen from outside the sphere,
	// by two sign tests on normalised directions: the half-plane relative to the first ray (ties
	// broken by the dot product), then the orientation within a half-plane. The insertion places
	// each ray after every ray it does not precede, as the prototype's '_angular_sort_float' does,
	// so near-ties fall where it leaves them (section 5.2: measured harmless).
	// Rays with no direction go last, in end order; the order is not used where they occur unless
	// the vertex has six or more ends, where they float.
	//
	std::vector<Ray> sorted;
	std::vector<Ray> no_direction;
	std::vector<UnitVector3D> sorted_directions;

	const UnitVector3D &centre = centre_point.position_vector();
	boost::optional<UnitVector3D> first_direction;

	const auto half_plane = [&](const UnitVector3D &d)
	{
		const double side = dot(centre, cross(*first_direction, d)).dval();
		if (side > 0)
		{
			return 0;
		}
		if (side < 0)
		{
			return 1;
		}
		return dot(*first_direction, d).dval() > 0 ? 0 : 1;
	};

	const auto precedes = [&](const UnitVector3D &a, const UnitVector3D &b)
	{
		const int half_a = half_plane(a);
		const int half_b = half_plane(b);
		if (half_a != half_b)
		{
			return half_a < half_b;
		}
		return dot(centre, cross(a, b)).dval() > 0;
	};

	for (const Ray &ray : rays)
	{
		if (!ray.direction || ray.direction->is_zero_magnitude())
		{
			no_direction.push_back(ray);
			continue;
		}

		const UnitVector3D direction = ray.direction->get_normalisation();
		if (!first_direction)
		{
			first_direction = direction;
		}

		unsigned int i = 0;
		while (i < sorted.size() && precedes(sorted_directions[i], direction))
		{
			++i;
		}
		sorted.insert(sorted.begin() + i, ray);
		sorted_directions.insert(sorted_directions.begin() + i, direction);
	}

	sorted.insert(sorted.end(), no_direction.begin(), no_direction.end());
	return sorted;
}


void
GPlatesMaths::PolygonOverlay::Overlay::build_rays()
{
	for (unsigned int v = 0; v < d_vertices.size(); ++v)
	{
		Vertex &vertex = d_vertices[v];

		std::vector<Ray> rays;
		unsigned int num_ends = 0;
		for (const End &end : vertex.ends)
		{
			const Piece &piece = d_pieces[end.piece];
			const Side side = d_rings[piece.ring].side;

			// The clip end of a common pair joins its subject end's ray.
			if (piece.common && side == CLIP)
			{
				continue;
			}

			Ray ray;
			ray.flips_subject = (side == SUBJECT);
			ray.flips_clip = (side == CLIP);
			if (side == SUBJECT)
			{
				ray.subject_end = end;
			}
			else
			{
				ray.clip_end = end;
			}
			measure_ray(vertex, end, ray);
			++num_ends;

			if (piece.common)
			{
				// A common pair's pieces have distinct end vertices, so the partner's end here is
				// whichever of its ends is this vertex.
				const Piece &partner = d_pieces[*piece.common];
				const End partner_end(*piece.common, *partner.start_vertex != v);
				ray.flips_clip = true;
				ray.clip_end = partner_end;
				++num_ends;

				Ray partner_ray;
				const bool partner_has_direction = measure_ray(vertex, partner_end, partner_ray);

				// The subject's direction, unless the subject's first arc is zero length and the
				// clip's is not (a single arc: its one segment).
				const bool subject_zero_length = piece.single_arc_segment &&
						d_rings[piece.ring].segments[*piece.single_arc_segment].is_zero_length();
				if ((subject_zero_length || !ray.direction) && partner_has_direction)
				{
					ray.direction = partner_ray.direction;
				}

				// The wider cone of the two.
				if (partner_ray.cone > ray.cone)
				{
					ray.delta = partner_ray.delta;
					ray.first_arc_length = partner_ray.first_arc_length;
					ray.reach = partner_ray.reach;
					ray.cone = partner_ray.cone;
				}
			}

			if (!ray.direction)
			{
				++d_diagnostics.zero_length_rays;
			}
			rays.push_back(ray);
		}

		d_diagnostics.max_ends = (std::max)(d_diagnostics.max_ends, num_ends);
		vertex.rays = sort_rays_anticlockwise(vertex.position, rays);

		// The bundle rule, at six or more ends where neither input passes once (the orders depend
		// on geometry only).
		if (num_ends >= 6 && !one_passage_side(vertex))
		{
			vertex.bundle = build_bundle(vertex);
		}
	}
}


bool
GPlatesMaths::PolygonOverlay::Overlay::is_inside_other(
		Side side,
		const PointOnSphere &point) const
{
	// Threshold off: a test point of a piece is either an original vertex in no record (so more
	// than the thickness from the other input's outline, about 1e10 times point-in-polygon's
	// rounding) or a midpoint tested in order of margin.
	//
	// Point-in-polygon is not exact near an edge GreatCircleArc calls zero length (under about
	// 1e-6 radians): its crossing count stands such an edge's crossing in by the edge's end point
	// (PointInPolygon.cc), so within about the edge's length of it the answer can be wrong (a point
	// 0.019 thicknesses inside a polygon was called outside at every speed), and minimum_distance
	// is off there too. A vertex in no record is beyond that zone; only a band piece's midpoint
	// test can be affected, and its class is arbitrary by nature.
	return side == SUBJECT
			? d_clip.is_point_in_polygon(
					point,
					d_options.clip_speed,
					false/*use_point_on_polygon_threshold*/)
			: d_subject_polygon->is_point_in_polygon(
					point,
					PolygonOnSphere::ADAPTIVE,
					false/*use_point_on_polygon_threshold*/);
}


bool
GPlatesMaths::PolygonOverlay::Overlay::solve_four_end_vertex(
		const Vertex &vertex,
		std::vector<Deduction> *deductions) const
{
	//
	// The geometry-free rule (section 5.2, classification step 2): with four ends, two of
	// each input and none common, nothing needs the rotational order, which at a snapped touch
	// can disagree with the topology the records encode.
	//
	std::vector<unsigned int> side_pieces[2];
	for (const Ray &ray : vertex.rays)
	{
		if (ray.subject_end)
		{
			side_pieces[SUBJECT].push_back(ray.subject_end->piece);
		}
		else
		{
			side_pieces[CLIP].push_back(ray.clip_end->piece);
		}
	}

	const auto known = [this](unsigned int piece)
	{
		return d_pieces[piece].piece_class != CLASS_UNKNOWN;
	};
	const auto inside = [this](unsigned int piece)
	{
		return d_pieces[piece].piece_class == CLASS_INSIDE;
	};

	const bool is_crossing_record = vertex.num_records == 1 &&
			d_graph.unordered_intersections[d_vertex_records[vertex.first_record]].type ==
					GeometryIntersect::Intersection::SEGMENTS_CROSS;
	if (is_crossing_record)
	{
		// One crossing record: both arcs' ends lie beyond the thickness on opposite sides, so each
		// input's two classes differ. One known class fixes its own input's other class only.
		for (const std::vector<unsigned int> &pieces : side_pieces)
		{
			const bool known0 = known(pieces[0]);
			const bool known1 = known(pieces[1]);
			if (known0 && known1)
			{
				if (inside(pieces[0]) == inside(pieces[1]))
				{
					return false;
				}
			}
			else if (deductions && (known0 || known1))
			{
				const unsigned int known_piece = known0 ? pieces[0] : pieces[1];
				const unsigned int unknown_piece = known0 ? pieces[1] : pieces[0];
				deductions->push_back(
						Deduction{unknown_piece, !inside(known_piece), METHOD_DEDUCED});
			}
		}
		return true;
	}

	// Touch records: each input crosses iff its two classes differ, and the two must agree. Three
	// known classes fix the fourth.
	const auto num_known_of = [&](Side side)
	{
		return static_cast<unsigned int>(known(side_pieces[side][0])) +
				static_cast<unsigned int>(known(side_pieces[side][1]));
	};
	const unsigned int num_known[2] = { num_known_of(SUBJECT), num_known_of(CLIP) };
	const auto crosses = [&](Side side)
	{
		return inside(side_pieces[side][0]) != inside(side_pieces[side][1]);
	};

	if (num_known[SUBJECT] == 2 && num_known[CLIP] == 2)
	{
		return crosses(SUBJECT) == crosses(CLIP);
	}

	for (Side known_side : { SUBJECT, CLIP })
	{
		const Side other_side = (known_side == SUBJECT) ? CLIP : SUBJECT;
		if (num_known[known_side] == 2 && num_known[other_side] == 1 && deductions)
		{
			const std::vector<unsigned int> &pieces = side_pieces[other_side];
			const unsigned int known_piece = known(pieces[0]) ? pieces[0] : pieces[1];
			const unsigned int unknown_piece = known(pieces[0]) ? pieces[1] : pieces[0];
			const bool value = crosses(known_side) ? !inside(known_piece) : inside(known_piece);
			deductions->push_back(Deduction{unknown_piece, value, METHOD_DEDUCED});
		}
	}
	return true;
}


GPlatesMaths::PolygonOverlay::VertexSolution
GPlatesMaths::PolygonOverlay::Overlay::solve_vertex(
		unsigned int vertex_index,
		std::vector<Deduction> *deductions,
		bool use_free_rule) const
{
	const Vertex &vertex = d_vertices[vertex_index];
	const std::vector<Ray> &rays = vertex.rays;
	const unsigned int num_rays = rays.size();

	VertexSolution solution;
	if (num_rays == 0)
	{
		return solution;
	}

	// Four ends, two of each input, none common: the geometry-free rule.
	if (num_rays == 4 && use_free_rule)
	{
		unsigned int num_subject_rays = 0;
		bool any_common = false;
		for (const Ray &ray : rays)
		{
			num_subject_rays += ray.subject_end ? 1 : 0;
			any_common = any_common || ray.is_common();
		}
		if (!any_common && num_subject_rays == 2)
		{
			solution.free_rule = true;
			solution.consistent = solve_four_end_vertex(vertex, deductions);
			solution.num_consistent_orders = solution.consistent ? 1 : 0;
			return solution;
		}
	}

	//
	// The sector walk (section 5.2, classification step 2). Each classified ray fixes the
	// memberships of the two sectors beside it: a subject ray of class c fixes 'in clip = c', a
	// clip ray fixes 'in subject', a common ray 'in subject xor in clip' (0 same side, 1 opposite).
	//
	enum ConstraintKind { NONE, IN_SUBJECT, IN_CLIP, XOR };
	std::vector<std::pair<ConstraintKind, bool>> constraints(num_rays, std::make_pair(NONE, false));
	for (unsigned int i = 0; i < num_rays; ++i)
	{
		const Ray &ray = rays[i];
		if (ray.is_common())
		{
			const CommonClass common_class = d_pieces[ray.subject_end->piece].common_class;
			if (common_class != COMMON_UNKNOWN)
			{
				constraints[i] = std::make_pair(XOR, common_class == COMMON_OPPOSITE_SIDES);
			}
		}
		else if (ray.subject_end)
		{
			const Class piece_class = d_pieces[ray.subject_end->piece].piece_class;
			if (piece_class != CLASS_UNKNOWN)
			{
				constraints[i] = std::make_pair(IN_CLIP, piece_class == CLASS_INSIDE);
			}
		}
		else
		{
			const Class piece_class = d_pieces[ray.clip_end->piece].piece_class;
			if (piece_class != CLASS_UNKNOWN)
			{
				constraints[i] = std::make_pair(IN_SUBJECT, piece_class == CLASS_INSIDE);
			}
		}
	}

	std::vector<unsigned int> measured(num_rays);
	for (unsigned int i = 0; i < num_rays; ++i)
	{
		measured[i] = i;
	}

	//
	// The one-passage rule (section 5.2, classification step 2): at six
	// or more ends where one input, X, has exactly two (it passes the vertex once), any classes of
	// the other input's rays can be realised by some order, so the measured order adds nothing and
	// is not consulted. X's boundary divides the neighbourhood into an inside half and an outside
	// half; walking the inside half from X's end a to its end b crosses every other ray inside X,
	// so m_in(b) = m_in(a) xor (number of other rays inside X, mod 2), where m_in(e) is the other
	// input's membership on the inside-X side of end e: its class for a plain ray, and for a common
	// ray 1 if same side, 0 if opposite (inside X, the memberships agree iff same side).
	// Combinatorial; one unknown follows, all known is checked. The order for the linking is built
	// from the classes: a, the other rays inside X (in measured relative order), b, the rest.
	//
	std::vector<std::vector<unsigned int>> built_orders;
	const boost::optional<Side> once = one_passage_side(vertex);
	if (once)
	{
		solution.one_passage = true;

		std::vector<unsigned int> x_ends;
		// The other input's rays (never common: a common ray is an end of X).
		std::vector<unsigned int> others;
		for (unsigned int i = 0; i < num_rays; ++i)
		{
			const bool is_x_end = (*once == SUBJECT)
					? bool(rays[i].subject_end)
					: bool(rays[i].clip_end);
			(is_x_end ? x_ends : others).push_back(i);
		}

		// The class bit of a ray's piece (a common ray's lives on its subject piece).
		const auto piece_of = [&rays](unsigned int r)
		{
			const Ray &ray = rays[r];
			return (ray.is_common() || ray.subject_end)
					? ray.subject_end->piece
					: ray.clip_end->piece;
		};
		const auto m_in = [&](unsigned int r) -> boost::optional<bool>
		{
			const Piece &piece = d_pieces[piece_of(r)];
			if (!piece.is_classified())
			{
				return boost::none;
			}
			return rays[r].is_common()
					? piece.common_class == COMMON_SAME_SIDE   // (opposite ? 1 : 0) xor 1
					: piece.piece_class == CLASS_INSIDE;
		};

		const unsigned int a = x_ends[0];
		const unsigned int b = x_ends[1];
		const boost::optional<bool> m_a = m_in(a);
		const boost::optional<bool> m_b = m_in(b);
		bool n_inside = false;
		std::vector<unsigned int> unknown_others;
		for (unsigned int r : others)
		{
			const Piece &piece = d_pieces[piece_of(r)];
			if (!piece.is_classified())
			{
				unknown_others.push_back(r);
			}
			else if (piece.piece_class == CLASS_INSIDE)
			{
				n_inside = !n_inside;
			}
		}

		const unsigned int num_unknowns = (m_a ? 0 : 1) + (m_b ? 0 : 1) + unknown_others.size();
		if (num_unknowns == 0 && *m_b != (*m_a != n_inside))
		{
			solution.consistent = false;
		}
		else if (num_unknowns == 1 && deductions)
		{
			// The one unknown term follows from the relation.
			const auto deduce_end = [&](unsigned int r, bool m)
			{
				// For a common ray, m_in = same side, so the deduced class is m itself.
				deductions->push_back(Deduction{
						piece_of(r),
						m,
						rays[r].is_common() ? METHOD_SECTOR : METHOD_DEDUCED});
			};
			if (!m_a)
			{
				deduce_end(a, *m_b != n_inside);
			}
			else if (!m_b)
			{
				deduce_end(b, *m_a != n_inside);
			}
			else
			{
				// The parity the unknown ray must contribute: inside X iff it is odd.
				deductions->push_back(Deduction{
						piece_of(unknown_others.front()),
						(*m_a != *m_b) != n_inside,
						METHOD_DEDUCED});
			}
		}

		if (!solution.consistent || num_unknowns != 0)
		{
			solution.order = measured;
			solution.in_subject.assign(num_rays, boost::none);
			solution.in_clip.assign(num_rays, boost::none);
			solution.num_consistent_orders = solution.consistent ? 1 : 0;
			return solution;
		}

		// Every class known and consistent: the order for the linking, and its mirror (the other
		// realisation, if the walk finds no placement on the first).
		std::vector<unsigned int> inside, outside;
		for (unsigned int i = 0; i < num_rays; ++i)
		{
			const unsigned int r = (a + i) % num_rays;  // the measured order, from a
			if (std::find(others.begin(), others.end(), r) != others.end())
			{
				(d_pieces[piece_of(r)].piece_class == CLASS_INSIDE ? inside : outside).push_back(r);
			}
		}
		for (unsigned int mirror = 0; mirror < 2; ++mirror)
		{
			const std::vector<unsigned int> &first = mirror ? outside : inside;
			const std::vector<unsigned int> &second = mirror ? inside : outside;
			std::vector<unsigned int> order(1, a);
			order.insert(order.end(), first.begin(), first.end());
			order.push_back(b);
			order.insert(order.end(), second.begin(), second.end());
			built_orders.push_back(order);
		}
	}

	// The orders to try: the one-passage rule's, or at six or more ends the bundle rule's
	// admissible orders (the measured one first), or the measured order.
	const std::vector<std::vector<unsigned int>> measured_only(1, measured);
	const std::vector<std::vector<unsigned int>> &orders = once
			? built_orders
			: (vertex.bundle ? vertex.bundle->orders : measured_only);

	// For each order, the prefix parities, and the starts (memberships of the sector after the
	// order's first ray) that meet every constraint.
	class OrderSolution
	{
	public:
		unsigned int order;
		bool in_subject;
		bool in_clip;
	};
	std::vector<OrderSolution> solutions;
	std::vector<std::vector<bool>> prefix_subject(orders.size()), prefix_clip(orders.size());
	for (unsigned int o = 0; o < orders.size(); ++o)
	{
		const std::vector<unsigned int> &order = orders[o];
		prefix_subject[o].assign(num_rays, false);
		prefix_clip[o].assign(num_rays, false);
		for (unsigned int i = 1; i < num_rays; ++i)
		{
			prefix_subject[o][i] = prefix_subject[o][i - 1] != rays[order[i]].flips_subject;
			prefix_clip[o][i] = prefix_clip[o][i - 1] != rays[order[i]].flips_clip;
		}

		for (bool a : { false, true })
		{
			for (bool b : { false, true })
			{
				bool ok = true;
				for (unsigned int i = 0; ok && i < num_rays; ++i)
				{
					const std::pair<ConstraintKind, bool> &constraint = constraints[order[i]];
					if (constraint.first == NONE)
					{
						continue;
					}
					// The sectors before and after ray i.
					for (unsigned int sector : { (i + num_rays - 1) % num_rays, i })
					{
						const bool in_subject = a != prefix_subject[o][sector];
						const bool in_clip = b != prefix_clip[o][sector];
						const bool got = (constraint.first == IN_SUBJECT)
								? in_subject
								: (constraint.first == IN_CLIP ? in_clip : in_subject != in_clip);
						if (got != constraint.second)
						{
							ok = false;
							break;
						}
					}
				}
				if (ok)
				{
					solutions.push_back(OrderSolution{o, a, b});
				}
			}
		}
	}

	if (solutions.empty())
	{
		solution.consistent = false;
		solution.order = orders[0];
		solution.in_subject.assign(num_rays, boost::none);
		solution.in_clip.assign(num_rays, boost::none);
		return solution;
	}

	//
	// The order used for pairing, among the consistent ones: the one with the fewest inversions
	// (Kendall distance) against the measured order, ties by enumeration order (the bundle
	// rule, section 5.2). Unbundled, the measured order is the only one.
	//
	std::vector<unsigned int> consistent_orders;
	for (const OrderSolution &order_solution : solutions)
	{
		if (consistent_orders.empty() || consistent_orders.back() != order_solution.order)
		{
			consistent_orders.push_back(order_solution.order);
		}
	}
	const auto inversions = [num_rays](const std::vector<unsigned int> &order)
	{
		// Linearised from the measured order's first ray (ray 0), whose own order is 0, 1, 2, ...
		const unsigned int start = std::find(order.begin(), order.end(), 0u) - order.begin();
		unsigned int count = 0;
		for (unsigned int i = 0; i < num_rays; ++i)
		{
			for (unsigned int j = i + 1; j < num_rays; ++j)
			{
				if (order[(start + i) % num_rays] > order[(start + j) % num_rays])
				{
					++count;
				}
			}
		}
		return count;
	};
	unsigned int chosen = consistent_orders.front();
	if (!once && vertex.bundle && consistent_orders.size() > 1)
	{
		unsigned int fewest = inversions(orders[chosen]);
		for (unsigned int o : consistent_orders)
		{
			const unsigned int count = inversions(orders[o]);
			if (count < fewest)
			{
				fewest = count;
				chosen = o;
			}
		}
	}
	solution.num_consistent_orders = once ? 1 : consistent_orders.size();
	if (once)
	{
		// Whether the built order differs from the measured one, cyclically.
		const std::vector<unsigned int> &order = orders[chosen];
		const unsigned int start = std::find(order.begin(), order.end(), 0u) - order.begin();
		for (unsigned int i = 0; i < num_rays; ++i)
		{
			solution.moved = solution.moved || order[(start + i) % num_rays] != i;
		}
	}
	else
	{
		solution.moved = (chosen != 0);
	}

	if (deductions)
	{
		// A class for an unclassified ray only if every solution agrees on it.
		for (unsigned int r = 0; r < num_rays; ++r)
		{
			const Ray &ray = rays[r];
			const unsigned int piece =
					ray.subject_end ? ray.subject_end->piece : ray.clip_end->piece;
			if (d_pieces[piece].is_classified())
			{
				continue;
			}

			boost::optional<bool> agreed;
			bool disagree = false;
			for (const OrderSolution &order_solution : solutions)
			{
				const std::vector<unsigned int> &order = orders[order_solution.order];
				const unsigned int j = std::find(order.begin(), order.end(), r) - order.begin();
				const unsigned int o = order_solution.order;
				const bool in_subject = order_solution.in_subject != prefix_subject[o][j];
				const bool in_clip = order_solution.in_clip != prefix_clip[o][j];
				const bool value = ray.is_common()
						? (in_subject == in_clip)  // same side
						: (ray.subject_end ? in_clip : in_subject);
				if (agreed && *agreed != value)
				{
					disagree = true;
					break;
				}
				agreed = value;
			}
			if (!disagree)
			{
				deductions->push_back(Deduction{
						piece,
						*agreed,
						ray.is_common() ? METHOD_SECTOR : METHOD_DEDUCED});
			}
		}
	}

	// The memberships of the chosen order's sectors, where its solutions agree.
	solution.order = orders[chosen];
	boost::optional<bool> start_subject, start_clip;
	bool subject_known = true, clip_known = true;
	for (const OrderSolution &order_solution : solutions)
	{
		if (order_solution.order != chosen)
		{
			continue;
		}
		if (start_subject && *start_subject != order_solution.in_subject)
		{
			subject_known = false;
		}
		if (start_clip && *start_clip != order_solution.in_clip)
		{
			clip_known = false;
		}
		start_subject = order_solution.in_subject;
		start_clip = order_solution.in_clip;
	}
	for (unsigned int i = 0; i < num_rays; ++i)
	{
		solution.in_subject.push_back(subject_known
				? boost::optional<bool>(*start_subject != prefix_subject[chosen][i])
				: boost::none);
		solution.in_clip.push_back(clip_known
				? boost::optional<bool>(*start_clip != prefix_clip[chosen][i])
				: boost::none);
	}

	return solution;
}


bool
GPlatesMaths::PolygonOverlay::Overlay::deduce_to_fixed_point()
{
	bool any_change = false;
	bool changed = true;
	while (changed)
	{
		changed = false;
		for (unsigned int v = 0; v < d_vertices.size(); ++v)
		{
			std::vector<Deduction> deductions;
			solve_vertex(v, &deductions);
			for (const Deduction &deduction : deductions)
			{
				Piece &piece = d_pieces[deduction.piece];
				if (piece.is_classified())
				{
					// Deduced already, from this vertex's other end of the same piece.
					continue;
				}

				if (piece.common)
				{
					piece.common_class = deduction.inside_or_same_side
							? COMMON_SAME_SIDE
							: COMMON_OPPOSITE_SIDES;
					++d_diagnostics.sector_classes;
				}
				else
				{
					piece.piece_class =
							deduction.inside_or_same_side ? CLASS_INSIDE : CLASS_OUTSIDE;
					++d_diagnostics.deduced;
				}
				piece.method = deduction.method;
				changed = true;
				any_change = true;
			}
		}
	}
	return any_change;
}


GPlatesMaths::PolygonOverlay::Result
GPlatesMaths::PolygonOverlay::Overlay::classify()
{
	if (d_subject_polyline)
	{
		classify_polyline_pieces();

		bool any_inside = false;
		bool any_outside = false;
		const std::pair<unsigned int, unsigned int> range =
				ring_piece_range(d_side_rings_begin[SUBJECT]);
		for (unsigned int p = range.first; p < range.second; ++p)
		{
			if (!d_pieces[p].internal)
			{
				(d_pieces[p].piece_class == CLASS_INSIDE ? any_inside : any_outside) = true;
			}
		}
		return !any_inside ? RESULT_OUTSIDE : (!any_outside ? RESULT_INSIDE : RESULT_INTERSECTING);
	}

	classify_pieces();
	return check_and_repair();
}


void
GPlatesMaths::PolygonOverlay::Overlay::classify_polyline_pieces()
{
	//
	// A polyline subject (section 5.2). The clip's pieces bound nothing here and
	// are not classified. There is no sector walk: a polyline divides nothing, so the vertex rules
	// of the polygon overlay do not apply.
	//
	const std::pair<unsigned int, unsigned int> range =
			ring_piece_range(d_side_rings_begin[SUBJECT]);

	// (1) A piece along the clip's outline (a common pair) gets the along class (section 5.2).
	for (unsigned int p = range.first; p < range.second; ++p)
	{
		Piece &piece = d_pieces[p];
		if (!piece.internal && piece.common)
		{
			piece.piece_class = d_options.polyline_along_is_inside ? CLASS_INSIDE : CLASS_OUTSIDE;
			piece.method = METHOD_ALONG;
			++d_diagnostics.along_pieces;
		}
	}

	// (2) A piece with a test point is tested there, exactly (threshold off): its interior original
	// vertices, then a dangling end (an end point that is no record is more than the thickness from
	// the clip's outline). Exact with margin.
	for (unsigned int p = range.first; p < range.second; ++p)
	{
		Piece &piece = d_pieces[p];
		if (piece.internal || piece.common)
		{
			continue;
		}
		std::vector<unsigned int> test_stops;
		for (unsigned int s = piece.start_stop + 1; s < piece.end_stop; ++s)
		{
			if (d_stops[s].kind == Stop::ORIGINAL_VERTEX)
			{
				test_stops.push_back(s);
			}
		}
		for (unsigned int s : { piece.start_stop, piece.end_stop })
		{
			if (d_stops[s].kind == Stop::DANGLING_END)
			{
				test_stops.push_back(s);
			}
		}
		if (test_stops.empty())
		{
			continue;
		}
		const bool inside = is_inside_other(SUBJECT, d_stops[test_stops.front()].position);
		piece.piece_class = inside ? CLASS_INSIDE : CLASS_OUTSIDE;
		piece.method = METHOD_VERTEX_TEST;
		++d_diagnostics.vertex_tests;
#ifdef GPLATES_DEBUG
		for (unsigned int s : test_stops)
		{
			if (is_inside_other(SUBJECT, d_stops[s].position) != inside)
			{
				++d_diagnostics.piece_side_disagreements;
				break;
			}
		}
#endif
	}

	//
	// (3) Deduction only across a vertex that is one crossing (one LL record) with exactly two of
	// the polyline's pieces, neither along the outline: their classes differ. Never across a touch
	// (the polyline can bounce), never from an along piece (so that its class cannot leak). Two
	// tested pieces there with one class mean a missed intersection: counted (both have a margin,
	// and are kept). Then (4) the stalled pieces at their chords' midpoints, by margin, as for a
	// polygon.
	//
	const GeometryIntersect::intersection_seq_type &records = d_graph.unordered_intersections;
	std::vector<bool> disagreement_counted(d_vertices.size(), false);
	std::vector<boost::optional<double>> margins(d_pieces.size());
	while (true)
	{
		bool changed = true;
		while (changed)
		{
			changed = false;
			for (unsigned int v = 0; v < d_vertices.size(); ++v)
			{
				const Vertex &vertex = d_vertices[v];
				if (vertex.num_records != 1 ||
					records[d_vertex_records[vertex.first_record]].type !=
							GeometryIntersect::Intersection::SEGMENTS_CROSS)
				{
					continue;
				}
				std::vector<unsigned int> polyline_pieces;
				for (const End &end : vertex.ends)
				{
					if (d_rings[d_pieces[end.piece].ring].side == SUBJECT)
					{
						polyline_pieces.push_back(end.piece);
					}
				}
				if (polyline_pieces.size() != 2)
				{
					continue;
				}
				Piece &a = d_pieces[polyline_pieces[0]];
				Piece &b = d_pieces[polyline_pieces[1]];
				if (a.common || b.common)
				{
					continue;
				}
				const bool a_known = a.piece_class != CLASS_UNKNOWN;
				const bool b_known = b.piece_class != CLASS_UNKNOWN;
				if (a_known != b_known)
				{
					Piece &unknown = a_known ? b : a;
					const Piece &known = a_known ? a : b;
					unknown.piece_class =
							(known.piece_class == CLASS_INSIDE) ? CLASS_OUTSIDE : CLASS_INSIDE;
					unknown.method = METHOD_DEDUCED;
					++d_diagnostics.deduced;
					changed = true;
				}
				else if (a_known && a.piece_class == b.piece_class && !disagreement_counted[v])
				{
					disagreement_counted[v] = true;
					++d_diagnostics.ll_disagreements;
				}
			}
		}

		boost::optional<unsigned int> best_piece;
		double best_margin = -1.0;
		for (unsigned int p = range.first; p < range.second; ++p)
		{
			const Piece &piece = d_pieces[p];
			if (piece.internal || piece.piece_class != CLASS_UNKNOWN)
			{
				continue;
			}
			if (!margins[p])
			{
				const Vector3D sum =
						Vector3D(d_stops[piece.start_stop].position.position_vector()) +
						Vector3D(d_stops[piece.end_stop].position.position_vector());
				const boost::optional<AngularDistance> distance =
						minimum_distance(PointOnSphere(sum.get_normalisation()), d_clip);
				margins[p] = distance ? distance->calculate_angle().dval() : 0.0;
			}
			if (*margins[p] > best_margin)
			{
				best_margin = *margins[p];
				best_piece = p;
			}
		}
		if (!best_piece)
		{
			break;
		}
		Piece &piece = d_pieces[*best_piece];
		const Vector3D sum =
				Vector3D(d_stops[piece.start_stop].position.position_vector()) +
				Vector3D(d_stops[piece.end_stop].position.position_vector());
		piece.piece_class = is_inside_other(SUBJECT, PointOnSphere(sum.get_normalisation()))
				? CLASS_INSIDE
				: CLASS_OUTSIDE;
		piece.method = METHOD_MIDPOINT_TEST;
		++d_diagnostics.midpoint_tests;
	}
}


void
GPlatesMaths::PolygonOverlay::Overlay::classify_pieces()
{
	// A polyline subject is classified by its own rules (section 5.2).
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_subject_polygon != nullptr,
			GPLATES_ASSERTION_SOURCE);

	//
	// (a) Vertex tests: a non-common piece with an original vertex is classified by exact
	// point-in-polygon at its first one. In debug builds every one is tested, since two on
	// different sides mean a crossing with no record.
	//
	for (Piece &piece : d_pieces)
	{
		if (piece.internal || piece.common || !piece.has_interior_vertex)
		{
			continue;
		}

		const RingView &ring_view = d_rings[piece.ring];
		std::vector<unsigned int> vertex_stops;
		if (piece.closed)
		{
			for (unsigned int s = ring_view.first_stop; s < ring_view.end_stop; ++s)
			{
				vertex_stops.push_back(s);
			}
		}
		else
		{
			for (unsigned int s = piece.start_stop; ; )
			{
				s = (s + 1 == ring_view.end_stop) ? ring_view.first_stop : s + 1;
				if (s == piece.end_stop)
				{
					break;
				}
				if (d_stops[s].kind == Stop::ORIGINAL_VERTEX)
				{
					vertex_stops.push_back(s);
				}
			}
		}

		const bool inside = is_inside_other(ring_view.side, d_stops[vertex_stops.front()].position);
		piece.piece_class = inside ? CLASS_INSIDE : CLASS_OUTSIDE;
		piece.method = METHOD_VERTEX_TEST;
		++d_diagnostics.vertex_tests;

#ifdef GPLATES_DEBUG
		for (unsigned int s : vertex_stops)
		{
			if (is_inside_other(ring_view.side, d_stops[s].position) != inside)
			{
				++d_diagnostics.piece_side_disagreements;
				break;
			}
		}
#endif
	}

	//
	// (b) Deduction to a fixed point, then (c) the stalled single arcs, one at a time, by margin:
	// the one whose midpoint is farthest from the other input's outline is tested first, then
	// deduction resumes. A class decided within the band of rounding must never be the source of a
	// deduction for a piece with a margin (section 5.2, classification step 3). The distance is
	// rounded (a cosine, resolving about 1.5e-8 radians), but it only orders the tests.
	//
	std::vector<boost::optional<double>> margins(d_pieces.size());
	while (true)
	{
		deduce_to_fixed_point();

		boost::optional<unsigned int> best_piece;
		double best_margin = -1.0;
		for (unsigned int p = 0; p < d_pieces.size(); ++p)
		{
			const Piece &piece = d_pieces[p];
			if (piece.internal || piece.common || piece.is_classified())
			{
				continue;
			}

			if (!margins[p])
			{
				const Vector3D sum =
						Vector3D(d_stops[piece.start_stop].position.position_vector()) +
						Vector3D(d_stops[piece.end_stop].position.position_vector());
				const PointOnSphere midpoint(sum.get_normalisation());
				const boost::optional<AngularDistance> distance =
						minimum_distance(midpoint, other_polygon(d_rings[piece.ring].side));
				margins[p] = distance ? distance->calculate_angle().dval() : 0.0;
			}

			if (*margins[p] > best_margin)
			{
				best_margin = *margins[p];
				best_piece = p;
			}
		}

		if (!best_piece)
		{
			break;
		}

		Piece &piece = d_pieces[*best_piece];
		const Vector3D sum =
				Vector3D(d_stops[piece.start_stop].position.position_vector()) +
				Vector3D(d_stops[piece.end_stop].position.position_vector());
		const bool inside = is_inside_other(
				d_rings[piece.ring].side,
				PointOnSphere(sum.get_normalisation()));
		piece.piece_class = inside ? CLASS_INSIDE : CLASS_OUTSIDE;
		piece.method = METHOD_MIDPOINT_TEST;
		++d_diagnostics.midpoint_tests;
	}

	//
	// (d) Anchorless chains (section 5.2, classification step 4): a common pair still
	// unclassified belongs to a chain that no non-common piece reaches (identical rings, say). One
	// arc of the chain is classified by exact crossing parities from each input's reference point,
	// and deduction then carries the class along the chain.
	//
	// Each pass classifies at least one unclassified piece, or sets aside a chain none of whose
	// unclassified arcs has a direction (left unresolved, and counted below), so the loop ends.
	std::vector<bool> set_aside(d_pieces.size(), false);
	while (true)
	{
		boost::optional<unsigned int> start;
		for (unsigned int p = 0; p < d_pieces.size(); ++p)
		{
			const Piece &piece = d_pieces[p];
			if (!piece.internal &&
				piece.common &&
				d_rings[piece.ring].side == SUBJECT &&
				!piece.is_classified() &&
				!set_aside[p])
			{
				start = p;
				break;
			}
		}
		if (!start)
		{
			break;
		}

		const std::vector<unsigned int> chain = get_common_chain(*start);
		d_diagnostics.anchorless_arcs_max =
				(std::max)(d_diagnostics.anchorless_arcs_max, unsigned(chain.size()));
		if (!classify_anchorless_chain(chain))
		{
			for (unsigned int p : chain)
			{
				set_aside[p] = true;
			}
			continue;
		}
		++d_diagnostics.anchorless_resolved;
		deduce_to_fixed_point();
	}

	// (e) A common clip piece takes its subject partner's class.
	for (Piece &piece : d_pieces)
	{
		if (piece.common && d_rings[piece.ring].side == CLIP)
		{
			const Piece &partner = d_pieces[*piece.common];
			piece.common_class = partner.common_class;
			piece.method = partner.common_class != COMMON_UNKNOWN ? METHOD_SECTOR : METHOD_NONE;
		}
	}
}


GPlatesMaths::PolygonOverlay::Result
GPlatesMaths::PolygonOverlay::Overlay::check_and_repair()
{
	// This pass's counters (it can run again, after a class is set by hand in a test).
	d_diagnostics.unresolved_pieces = 0;
	d_diagnostics.check_failures = 0;
	d_diagnostics.repairs = 0;
	d_diagnostics.whole_chain_repairs = 0;
	d_diagnostics.trusted_flipped = 0;
	d_diagnostics.one_passage_vertices = 0;
	d_diagnostics.one_passage_moved = 0;
	d_diagnostics.both_pass_twice = 0;
	d_diagnostics.bundle_vertices = 0;
	d_diagnostics.bundle_floating_max = 0;
	d_diagnostics.bundle_admissible_max = 0;
	d_diagnostics.bundle_placements_max = 0;
	d_diagnostics.bundle_ambiguous = 0;
	d_diagnostics.bundle_capped = 0;
	d_diagnostics.bundle_moved = 0;

	unsigned int num_failing_before_repair = 0;
	for (unsigned int v = 0; v < d_vertices.size(); ++v)
	{
		num_failing_before_repair += solve_vertex(v).consistent ? 0 : 1;
	}
	d_diagnostics.check_failures_before_repair = num_failing_before_repair;
	if (num_failing_before_repair != 0)
	{
		repair();
	}

	for (const Piece &piece : d_pieces)
	{
		// A common pair counts once, as its subject piece.
		const bool counts = !piece.common || d_rings[piece.ring].side == SUBJECT;
		if (!piece.internal && !piece.is_classified() && counts)
		{
			++d_diagnostics.unresolved_pieces;
		}
	}

	// The checks: every vertex must have a consistent placement of its sectors' memberships.
	for (unsigned int v = 0; v < d_vertices.size(); ++v)
	{
		const VertexSolution solution = solve_vertex(v);
		if (!solution.consistent)
		{
			++d_diagnostics.check_failures;
		}
		if (solution.one_passage)
		{
			++d_diagnostics.one_passage_vertices;
			d_diagnostics.one_passage_moved += (solution.consistent && solution.moved) ? 1 : 0;
		}
		{
			unsigned int subject_ends = 0, clip_ends = 0;
			for (const Ray &ray : d_vertices[v].rays)
			{
				subject_ends += ray.subject_end ? 1 : 0;
				clip_ends += ray.clip_end ? 1 : 0;
			}
			d_diagnostics.both_pass_twice += (subject_ends >= 4 && clip_ends >= 4) ? 1 : 0;
		}

		const boost::optional<Bundle> &bundle = d_vertices[v].bundle;
		if (bundle && !bundle->floating.empty())
		{
			const unsigned int num_floating = bundle->floating.size();
			const unsigned int num_orders = bundle->orders.size();
			++d_diagnostics.bundle_vertices;
			d_diagnostics.bundle_floating_max =
					(std::max)(d_diagnostics.bundle_floating_max, num_floating);
			d_diagnostics.bundle_admissible_max =
					(std::max)(d_diagnostics.bundle_admissible_max, num_orders);
			d_diagnostics.bundle_placements_max =
					(std::max)(d_diagnostics.bundle_placements_max, solution.num_consistent_orders);
			d_diagnostics.bundle_ambiguous += solution.num_consistent_orders > 1 ? 1 : 0;
			d_diagnostics.bundle_capped += bundle->capped ? 1 : 0;
			d_diagnostics.bundle_moved += solution.moved ? 1 : 0;
		}
	}

	// Never a silent wrong answer (section 5.6).
	if ((d_diagnostics.check_failures != 0 || d_diagnostics.unresolved_pieces != 0) &&
		d_options.throw_if_inconsistent)
	{
		static const char *const kinds[] = { "LL", "SS", "SL", "LS" };
		std::ostringstream message;
		message << "The overlay's classes are inconsistent after " << d_diagnostics.repairs
				<< " repairs: " << d_diagnostics.check_failures
				<< " vertices have no consistent arrangement and "
				<< d_diagnostics.unresolved_pieces << " pieces are unresolved.";
		boost::optional<PointOnSphere> position;
		const boost::optional<unsigned int> failing = find_failing_vertex();
		if (failing)
		{
			const Vertex &vertex = d_vertices[*failing];
			position = vertex.position;
			const LatLonPoint lat_lon = make_lat_lon_point(vertex.position);
			message << " The first is at latitude " << lat_lon.latitude() << ", longitude "
					<< lat_lon.longitude() << ", with records";
			for (unsigned int r : get_vertex_records(vertex))
			{
				const GeometryIntersect::Intersection &record = d_graph.unordered_intersections[r];
				message << " " << kinds[record.type] << "(" << record.segment_index1 << ","
						<< record.segment_index2 << ")";
			}
			message << ".";
		}
		throw InconsistentOverlayException(
				GPLATES_EXCEPTION_SOURCE,
				message.str(),
				d_diagnostics,
				position);
	}

	return get_result_from_classes();
}


GPlatesMaths::PolygonOverlay::Result
GPlatesMaths::PolygonOverlay::Overlay::get_result_from_classes() const
{
	//
	// The result, from what the intersection and the difference keep (section 5.2): the
	// intersection keeps subject pieces inside the clip, clip pieces inside the subject and
	// same-side common pairs; the difference keeps subject pieces outside the clip, clip pieces
	// inside the subject and opposite-sides common pairs.
	//
	bool intersection_keeps = false;
	bool difference_keeps = false;
	for (const Piece &piece : d_pieces)
	{
		if (piece.internal)
		{
			continue;
		}

		const Side side = d_rings[piece.ring].side;
		if (!piece.is_classified())
		{
			intersection_keeps = difference_keeps = true;
		}
		else if (piece.common)
		{
			if (side == SUBJECT)
			{
				bool &keeps = (piece.common_class == COMMON_SAME_SIDE)
						? intersection_keeps
						: difference_keeps;
				keeps = true;
			}
		}
		else if (side == SUBJECT)
		{
			(piece.piece_class == CLASS_INSIDE ? intersection_keeps : difference_keeps) = true;
		}
		else if (piece.piece_class == CLASS_INSIDE)
		{
			intersection_keeps = difference_keeps = true;
		}
	}

	if (!intersection_keeps)
	{
		return RESULT_OUTSIDE;
	}
	if (!difference_keeps)
	{
		return RESULT_INSIDE;
	}
	return RESULT_INTERSECTING;
}


void
GPlatesMaths::PolygonOverlay::Overlay::dump(
		std::ostream &output) const
{
	static const char *const kinds[] = { "LL", "SS", "SL", "LS" };
	static const char *const classes[] = { "none", "in", "out" };
	static const char *const common_classes[] = { "none", "same", "opposite" };
	static const char *const methods[] = {
			"none", "vertex", "deduced", "midpoint", "sector", "anchorless", "repaired", "along" };
	const double h = GeometryIntersect::Intersection::get_on_segment_start_threshold_sine();

	for (unsigned int r = 0; r < d_graph.unordered_intersections.size(); ++r)
	{
		const GeometryIntersect::Intersection &record = d_graph.unordered_intersections[r];
		output << "r" << r << ": " << kinds[record.type] << "(" << record.segment_index1 << ","
				<< record.segment_index2 << ")\n";
	}

	for (unsigned int v = 0; v < d_vertices.size(); ++v)
	{
		const Vertex &vertex = d_vertices[v];
		output << "v" << v << ": records [";
		for (unsigned int r : get_vertex_records(vertex))
		{
			output << " r" << r;
		}
		output << " ] rays [";
		for (const Ray &ray : vertex.rays)
		{
			output << " " << (ray.is_common() ? "PQ*" : (ray.subject_end ? "P" : "Q"))
					<< (ray.subject_end ? ray.subject_end->piece : ray.clip_end->piece);
		}
		output << " ]\n";
	}

	for (unsigned int p = 0; p < d_pieces.size(); ++p)
	{
		const Piece &piece = d_pieces[p];
		const RingView &ring = d_rings[piece.ring];
		output << "p" << p << " " << (ring.side == SUBJECT ? "P" : "Q") << ring.ring_index << "[";
		if (piece.closed)
		{
			output << "closed";
		}
		else
		{
			const auto name = [](const boost::optional<unsigned int> &vertex)
			{
				return vertex ? "v" + std::to_string(*vertex) : std::string("end");
			};
			output << name(piece.start_vertex) << "->" << name(piece.end_vertex);
		}

		const std::vector<PointOnSphere> points = get_piece_points(piece);
		double length = 0;
		for (unsigned int i = 0; i + 1 < points.size(); ++i)
		{
			length += angle_between(points[i].position_vector(), points[i + 1].position_vector());
		}
		output << "] len=" << length / h << "h cls="
				<< (piece.common ? common_classes[piece.common_class] : classes[piece.piece_class])
				<< " " << methods[piece.method]
				<< (piece.common ? " common" : "") << (piece.internal ? " internal" : "") << "\n";
	}
}


GPlatesMaths::PolygonOverlay::Bundle
GPlatesMaths::PolygonOverlay::Overlay::build_bundle(
		const Vertex &vertex)
{
	const std::vector<Ray> &rays = vertex.rays;
	const unsigned int num_rays = rays.size();

	Bundle bundle;
	std::vector<unsigned int> measured(num_rays);
	for (unsigned int i = 0; i < num_rays; ++i)
	{
		measured[i] = i;
	}

	//
	// Anchored and floating rays (the bundle rule, section 5.2). In order of certainty
	// (smallest cone, then largest reach; ties in measured order), a ray is anchored if, against
	// every ray already anchored, the angle between their directions is at least the sum of their
	// cones, so that no admissible position of one can pass the other. A ray with no direction
	// floats. Rounded: a flip at the margin moves one ray between anchored and floating, which
	// either changes nothing or offers a swap across a neighbour it cannot be told from.
	//
	std::vector<unsigned int> certainty = measured;
	std::stable_sort(
			certainty.begin(),
			certainty.end(),
			[&rays](unsigned int a, unsigned int b)
			{
				if (rays[a].cone != rays[b].cone)
				{
					return rays[a].cone < rays[b].cone;
				}
				return rays[a].reach > rays[b].reach;
			});

	std::vector<UnitVector3D> directions;
	directions.reserve(num_rays);
	for (const Ray &ray : rays)
	{
		directions.push_back(
				(ray.direction && !ray.direction->is_zero_magnitude())
						? ray.direction->get_normalisation()
						: vertex.position.position_vector());  // unused: such a ray floats
	}
	const auto has_direction = [&rays](unsigned int r)
	{
		return rays[r].direction && !rays[r].direction->is_zero_magnitude();
	};

	std::vector<unsigned int> anchored_by_certainty;
	for (unsigned int r : certainty)
	{
		// A ray with no direction, or a cone of pi (an arc no longer than its offset), never
		// anchors (section 5.2).
		bool separable = has_direction(r) && rays[r].cone < PI;
		for (unsigned int a : anchored_by_certainty)
		{
			if (!separable)
			{
				break;
			}
			const double cones = rays[r].cone + rays[a].cone;
			if (cones > 0.0 && angle_between(directions[r], directions[a]) < cones)
			{
				separable = false;
			}
		}
		(separable ? anchored_by_certainty : bundle.floating).push_back(r);
	}
	bundle.anchored = anchored_by_certainty;
	std::sort(bundle.anchored.begin(), bundle.anchored.end());  // back in the measured cyclic order

	bundle.orders.push_back(measured);
	if (bundle.floating.empty())
	{
		return bundle;
	}
	if (bundle.floating.size() > BUNDLE_CAP)
	{
		bundle.capped = true;
		return bundle;
	}
	if (bundle.anchored.empty())
	{
		// Every ray floats: no frame to place them in, so the measured order is kept.
		return bundle;
	}

	//
	// Admissible orders (the bundle rule, section 5.2). The reference ray is the most certain
	// anchored one; every ray's angle about the vertex is measured from it, and its interval is
	// that angle plus or minus its cone plus the reference's. An order places the floating rays
	// into the gaps between the anchored rays (which keep their measured order), in any
	// permutation within a gap, such that the angles can be chosen non-decreasing round the turn,
	// each inside its interval. The
	// enumeration order is deterministic: gaps in anchored order, floating rays in certainty order,
	// permutations lexicographic (the prototype's itertools order).
	//
	const unsigned int reference = anchored_by_certainty.front();
	std::vector<unsigned int> anchored = bundle.anchored;
	std::rotate(
			anchored.begin(),
			std::find(anchored.begin(), anchored.end(), reference),
			anchored.end());

	const UnitVector3D &centre = vertex.position.position_vector();
	const UnitVector3D &d0 = directions[reference];
	const UnitVector3D e1 = (Vector3D(d0) - dot(d0, centre) * Vector3D(centre)).get_normalisation();
	const UnitVector3D e2 = cross(centre, e1).get_normalisation();
	std::vector<double> angles(num_rays, 0.0);
	std::vector<double> widths(num_rays);
	for (unsigned int r = 0; r < num_rays; ++r)
	{
		if (r != reference && has_direction(r))
		{
			const double angle = std::atan2(
					dot(directions[r], e2).dval(),
					dot(directions[r], e1).dval());
			angles[r] = angle < 0 ? angle + 2 * PI : angle;
		}
		widths[r] = rays[r].cone + rays[reference].cone;
	}

	const auto admissible = [&](const std::vector<unsigned int> &order)
	{
		// Greedy earliest position, which is complete for interval feasibility; order[0] is the
		// reference, at angle 0. A width of pi or more admits any position.
		const double two_pi = 2 * PI;
		double previous = 0.0;
		for (unsigned int i = 1; i < order.size(); ++i)
		{
			const unsigned int r = order[i];
			if (widths[r] >= PI)
			{
				continue;
			}
			bool placed = false;
			for (double shift : { -two_pi, 0.0, two_pi })
			{
				const double low = angles[r] - widths[r] + shift;
				const double high = angles[r] + widths[r] + shift;
				if (high < previous || low > two_pi)
				{
					continue;
				}
				const double position = (std::max)(previous, low);
				if (position <= high && position <= two_pi)
				{
					previous = position;
					placed = true;
					break;
				}
			}
			if (!placed)
			{
				return false;
			}
		}
		return true;
	};

	const unsigned int num_anchored = anchored.size();
	const unsigned int num_floating = bundle.floating.size();
	std::vector<std::vector<unsigned int>> orders;
	// The gap of each floating ray: gap g is after anchored[g].
	std::vector<unsigned int> gaps(num_floating, 0);
	while (true)
	{
		std::vector<std::vector<unsigned int>> per_gap(num_anchored);
		for (unsigned int f = 0; f < num_floating; ++f)
		{
			// Positions in the floating list, so that the permutations are lexicographic.
			per_gap[gaps[f]].push_back(f);
		}

		// Every combination of a permutation per gap, the first gap's outermost.
		std::vector<std::vector<unsigned int>> permutations = per_gap;
		while (true)
		{
			std::vector<unsigned int> order;
			for (unsigned int g = 0; g < num_anchored; ++g)
			{
				order.push_back(anchored[g]);
				for (unsigned int f : permutations[g])
				{
					order.push_back(bundle.floating[f]);
				}
			}
			if (admissible(order))
			{
				orders.push_back(order);
			}

			// The next combination: advance the last gap's permutation, carrying leftwards.
			int g = num_anchored - 1;
			for ( ; g >= 0; --g)
			{
				if (std::next_permutation(permutations[g].begin(), permutations[g].end()))
				{
					break;
				}
				// next_permutation has reset this gap to its first permutation.
			}
			if (g < 0)
			{
				break;
			}
		}

		// The next gap assignment, the last floating ray varying fastest.
		int f = num_floating - 1;
		for ( ; f >= 0; --f)
		{
			if (++gaps[f] < num_anchored)
			{
				break;
			}
			gaps[f] = 0;
		}
		if (f < 0)
		{
			break;
		}
	}

	// The measured order first (it is always admissible), so that index 0 means "unchanged".
	const auto is_measured = [num_rays](const std::vector<unsigned int> &order)
	{
		const unsigned int start = std::find(order.begin(), order.end(), 0u) - order.begin();
		for (unsigned int i = 0; i < num_rays; ++i)
		{
			if (order[(start + i) % num_rays] != i)
			{
				return false;
			}
		}
		return true;
	};
	const auto measured_order = std::find_if(orders.begin(), orders.end(), is_measured);
	if (measured_order == orders.end())
	{
		orders.insert(orders.begin(), measured);
	}
	else
	{
		std::iter_swap(orders.begin(), measured_order);
	}
	bundle.orders = orders;
	return bundle;
}


boost::optional<GPlatesMaths::PolygonOverlay::Side>
GPlatesMaths::PolygonOverlay::Overlay::one_passage_side(
		const Vertex &vertex)
{
	// Ends per input, a common ray counting as an end of both.
	unsigned int subject_ends = 0;
	unsigned int clip_ends = 0;
	for (const Ray &ray : vertex.rays)
	{
		subject_ends += ray.subject_end ? 1 : 0;
		clip_ends += ray.clip_end ? 1 : 0;
	}

	if (subject_ends + clip_ends < 6)
	{
		return boost::none;
	}
	if (subject_ends == 2 && clip_ends >= 4)
	{
		return SUBJECT;
	}
	if (clip_ends == 2 && subject_ends >= 4)
	{
		return CLIP;
	}
	return boost::none;
}


std::vector<unsigned int>
GPlatesMaths::PolygonOverlay::Overlay::get_common_chain(
		unsigned int start_piece) const
{
	// The common subject pieces reachable from the start through vertices whose every ray is
	// common (the prototype's '_chain_of').
	std::vector<unsigned int> chain(1, start_piece);
	std::vector<bool> seen(d_pieces.size(), false);
	seen[start_piece] = true;
	std::vector<unsigned int> todo(1, start_piece);
	while (!todo.empty())
	{
		const Piece &piece = d_pieces[todo.back()];
		todo.pop_back();
		for (const boost::optional<unsigned int> &v : { piece.start_vertex, piece.end_vertex })
		{
			const std::vector<Ray> &rays = d_vertices[*v].rays;
			const auto is_not_common = [](const Ray &ray) { return !ray.is_common(); };
			if (std::any_of(rays.begin(), rays.end(), is_not_common))
			{
				continue;
			}
			for (const Ray &ray : rays)
			{
				const unsigned int next = ray.subject_end->piece;
				if (!seen[next])
				{
					seen[next] = true;
					chain.push_back(next);
					todo.push_back(next);
				}
			}
		}
	}
	return chain;
}


bool
GPlatesMaths::PolygonOverlay::Overlay::classify_anchorless_chain(
		const std::vector<unsigned int> &chain)
{
	const UnitVector3D &subject_outside = d_subject_polygon->get_outside_point();
	const UnitVector3D &clip_outside = d_clip.get_outside_point();

	//
	// The arc whose great circle is farthest from both reference points (the largest of the smaller
	// of the two |dot(R, axis)|), so that each crossing arc from a reference point meets that great
	// circle only at the arc's midpoint X, well away from both ends.
	//
	boost::optional<unsigned int> best;
	double best_score = -1.0;
	for (unsigned int p : chain)
	{
		// Only an unclassified arc: on consistent data every arc of the chain is, but on
		// inconsistent data deduction can have reached part of it, and its class must not be
		// overwritten (nor the loop above make no progress).
		const Piece &piece = d_pieces[p];
		const GreatCircleArc &segment = d_rings[piece.ring].segments[*piece.single_arc_segment];
		if (segment.is_zero_length() || piece.is_classified())
		{
			continue;
		}
		const UnitVector3D &axis = segment.rotation_axis();
		const double score = (std::min)(
				std::fabs(dot(subject_outside, axis).dval()),
				std::fabs(dot(clip_outside, axis).dval()));
		if (score > best_score)
		{
			best_score = score;
			best = p;
		}
	}
	if (!best)
	{
		return false;
	}

	Piece &piece = d_pieces[*best];
	const Piece &partner = d_pieces[*piece.common];
	const RingView &subject_ring = d_rings[piece.ring];
	const RingView &clip_ring = d_rings[partner.ring];
	const UnitVector3D &axis = subject_ring.segments[*piece.single_arc_segment].rotation_axis();
	const UnitVector3D midpoint = (
			Vector3D(d_stops[piece.start_stop].position.position_vector()) +
			Vector3D(d_stops[piece.end_stop].position.position_vector())).get_normalisation();

	//
	// Each input's membership on its reference point's side of the chain, from the parity of the
	// crossings from that point to X leaving out the input's own segment along the chain. The
	// outside point is outside its polygon; if its distance from the chain's great circle is below
	// about 1e-6 radians (where the crossing arc could meet that great circle twice), or it is
	// within that of X or its antipode, the polygon's second reference point is used, with its
	// cached status.
	//
	const auto membership = [&](
			const PolygonOnSphere &polygon,
			const UnitVector3D &outside_point,
			unsigned int excluded_segment,
			UnitVector3D &reference) -> bool
	{
		const double min_sine = 1e-6;
		bool status = false;
		reference = outside_point;
		if (std::fabs(dot(outside_point, axis).dval()) <= min_sine ||
			cross(outside_point, midpoint).magSqrd().dval() <= min_sine * min_sine)
		{
			reference = polygon.get_second_reference_point().point;
			status = polygon.get_second_reference_point().is_in_polygon;
		}
		return status != PointInPolygon::get_crossing_parity_excluding_segment(
				polygon, reference, midpoint, excluded_segment);
	};

	UnitVector3D subject_reference = subject_outside;
	UnitVector3D clip_reference = clip_outside;
	const bool in_subject = membership(
			*d_subject_polygon,
			subject_outside,
			subject_ring.first_segment_index + *piece.single_arc_segment,
			subject_reference);
	bool in_clip = membership(
			d_clip,
			clip_outside,
			clip_ring.first_segment_index + *partner.single_arc_segment,
			clip_reference);

	// The two memberships describe the reference points' sides; crossing the chain flips both.
	if ((dot(subject_reference, axis).dval() > 0) != (dot(clip_reference, axis).dval() > 0))
	{
		in_clip = !in_clip;
	}

	piece.common_class = (in_subject == in_clip) ? COMMON_SAME_SIDE : COMMON_OPPOSITE_SIDES;
	piece.method = METHOD_ANCHORLESS;
	return true;
}


boost::optional<unsigned int>
GPlatesMaths::PolygonOverlay::Overlay::find_failing_vertex() const
{
	for (unsigned int v = 0; v < d_vertices.size(); ++v)
	{
		if (!solve_vertex(v).consistent)
		{
			return v;
		}
	}
	return boost::none;
}


void
GPlatesMaths::PolygonOverlay::Overlay::set_piece_class(
		unsigned int piece_index,
		bool inside_or_same_side,
		Method method)
{
	Piece &piece = d_pieces[piece_index];
	if (piece.common)
	{
		Piece &partner = d_pieces[*piece.common];
		piece.common_class = partner.common_class =
				inside_or_same_side ? COMMON_SAME_SIDE : COMMON_OPPOSITE_SIDES;
		piece.method = partner.method = method;
	}
	else
	{
		piece.piece_class = inside_or_same_side ? CLASS_INSIDE : CLASS_OUTSIDE;
		piece.method = method;
	}
}


void
GPlatesMaths::PolygonOverlay::Overlay::repair()
{
	//
	// Repair (section 5.2, checks and repair): at the first failing vertex, flip
	// the least reliable classified piece there (by method, then shortest first) whose flip makes
	// the vertex consistent; repeat, up to the round cap. A common piece's flip flips its partner.
	// Where the vertex is a common chain's end, one more candidate flips the whole chain at once,
	// ranked with the sector classes and tried before any single piece of that rank: flipping a
	// chain piece by piece fails the chain's interior vertices the next round, towards the cap
	// (run 3's inversion residue). Measured: no repair in any normal Phase 4a run of the chosen
	// configuration, so this is a safety net; every flip is counted.
	//
	const auto rank = [](Method method)
	{
		switch (method)
		{
		case METHOD_MIDPOINT_TEST:
			return 0;
		case METHOD_DEDUCED:
			return 1;
		case METHOD_SECTOR:
		case METHOD_ANCHORLESS:  // a chain's class, like the sector walk's (the prototype has none)
			return 2;
		case METHOD_REPAIRED:
			return 3;
		case METHOD_VERTEX_TEST:
			return 4;
		default:
			return 5;
		}
	};

	const auto piece_length = [this](unsigned int p)
	{
		const std::vector<PointOnSphere> points = get_piece_points(d_pieces[p]);
		double length = 0;
		for (unsigned int i = 0; i + 1 < points.size(); ++i)
		{
			length += angle_between(points[i].position_vector(), points[i + 1].position_vector());
		}
		return length;
	};

	const auto flip = [this](unsigned int p)
	{
		Piece &piece = d_pieces[p];
		if (piece.common)
		{
			const CommonClass flipped = (piece.common_class == COMMON_SAME_SIDE)
					? COMMON_OPPOSITE_SIDES
					: COMMON_SAME_SIDE;
			piece.common_class = d_pieces[*piece.common].common_class = flipped;
		}
		else
		{
			piece.piece_class = (piece.piece_class == CLASS_INSIDE) ? CLASS_OUTSIDE : CLASS_INSIDE;
		}
	};

	class Candidate
	{
	public:
		int rank;
		bool whole_chain;
		double length;
		std::vector<unsigned int> pieces;
	};

	for (unsigned int round = 0; round < d_options.max_repair_rounds; ++round)
	{
		const boost::optional<unsigned int> failing = find_failing_vertex();
		if (!failing)
		{
			return;
		}
		const Vertex &vertex = d_vertices[*failing];

		// The classified pieces at the vertex (a common pair as its subject piece), each once.
		std::vector<Candidate> candidates;
		std::vector<unsigned int> seen;
		bool has_non_common_ray = false;
		for (const Ray &ray : vertex.rays)
		{
			has_non_common_ray = has_non_common_ray || !ray.is_common();
			for (const boost::optional<End> &end : { ray.subject_end, ray.clip_end })
			{
				if (!end)
				{
					continue;
				}
				const Piece &piece = d_pieces[end->piece];
				const bool counts = !piece.common || d_rings[piece.ring].side == SUBJECT;
				if (!counts ||
					!piece.is_classified() ||
					std::find(seen.begin(), seen.end(), end->piece) != seen.end())
				{
					continue;
				}
				seen.push_back(end->piece);
				candidates.push_back(Candidate{
						rank(piece.method),
						false,
						piece_length(end->piece),
						std::vector<unsigned int>(1, end->piece)});
			}
		}

		// At a chain's end, each chain through the vertex flipped as a whole.
		if (has_non_common_ray)
		{
			for (const Ray &ray : vertex.rays)
			{
				if (ray.is_common() && d_pieces[ray.subject_end->piece].is_classified())
				{
					const std::vector<unsigned int> chain =
							get_common_chain(ray.subject_end->piece);
					if (chain.size() > 1)
					{
						candidates.push_back(Candidate{ rank(METHOD_SECTOR), true, 0.0, chain });
					}
				}
			}
		}

		std::stable_sort(
				candidates.begin(),
				candidates.end(),
				[](const Candidate &a, const Candidate &b)
				{
					if (a.rank != b.rank)
					{
						return a.rank < b.rank;
					}
					if (a.whole_chain != b.whole_chain)
					{
						return a.whole_chain;
					}
					return a.length < b.length;
				});

		bool fixed = false;
		for (const Candidate &candidate : candidates)
		{
			for (unsigned int p : candidate.pieces)
			{
				flip(p);
			}
			if (solve_vertex(*failing).consistent)
			{
				for (unsigned int p : candidate.pieces)
				{
					Piece &piece = d_pieces[p];
					if (piece.method == METHOD_VERTEX_TEST)
					{
						++d_diagnostics.trusted_flipped;
					}
					piece.method = METHOD_REPAIRED;
					if (piece.common)
					{
						d_pieces[*piece.common].method = METHOD_REPAIRED;
					}
				}
				++d_diagnostics.repairs;
				d_diagnostics.whole_chain_repairs += candidate.whole_chain ? 1 : 0;
				fixed = true;
				break;
			}
			for (unsigned int p : candidate.pieces)
			{
				flip(p);  // back
			}
		}
		if (!fixed)
		{
			return;
		}
	}
}


boost::optional<bool>
GPlatesMaths::PolygonOverlay::Overlay::is_kept(
		const Piece &piece,
		Operation operation) const
{
	//
	// The keep-rule (section 5.2, linking): with f the operation's function of
	// (in subject, in clip), a subject piece of class c is kept iff f(1, c) != f(0, c), a clip piece
	// iff f(c, 1) != f(c, 0), a same-side chain iff f(1, 1) != f(0, 0), an opposite-sides chain iff
	// f(1, 0) != f(0, 1): the output differs across the piece.
	//
	if (!piece.is_classified())
	{
		return boost::none;
	}
	if (piece.common)
	{
		return piece.common_class == COMMON_SAME_SIDE
				? is_in_output(operation, true, true) != is_in_output(operation, false, false)
				: is_in_output(operation, true, false) != is_in_output(operation, false, true);
	}
	const bool c = (piece.piece_class == CLASS_INSIDE);
	return d_rings[piece.ring].side == SUBJECT
			? is_in_output(operation, true, c) != is_in_output(operation, false, c)
			: is_in_output(operation, c, true) != is_in_output(operation, c, false);
}


void
GPlatesMaths::PolygonOverlay::Overlay::extract_rings(
		Operation operation,
		std::vector<OutputRing> &rings) const
{
	// A polyline subject's output is extract_polylines' (a dangling end has no ray).
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_subject_polygon,
			GPLATES_ASSERTION_SOURCE);

	// A ray, as (vertex, index into its rays).
	typedef std::pair<unsigned int, unsigned int> ray_ref_type;
	const ray_ref_type no_ray((std::numeric_limits<unsigned int>::max)(), 0);

	// The ray carrying each piece end (2 * piece + at_end).
	std::vector<ray_ref_type> ray_of_end(2 * d_pieces.size(), no_ray);
	for (unsigned int v = 0; v < d_vertices.size(); ++v)
	{
		const std::vector<Ray> &rays = d_vertices[v].rays;
		for (unsigned int i = 0; i < rays.size(); ++i)
		{
			for (const boost::optional<End> &end : { rays[i].subject_end, rays[i].clip_end })
			{
				if (end)
				{
					ray_of_end[2 * end->piece + (end->at_end ? 1 : 0)] = ray_ref_type(v, i);
				}
			}
		}
	}

	// The piece end a ray carries (a common ray: its subject piece's).
	const auto ray_end = [this](const ray_ref_type &ray_ref)
	{
		const Ray &ray = d_vertices[ray_ref.first].rays[ray_ref.second];
		return ray.subject_end ? *ray.subject_end : *ray.clip_end;
	};

	//
	// Pair the kept ends at every vertex (section 5.2, linking).
	//
	std::vector<std::vector<boost::optional<unsigned int>>> partner(d_vertices.size());
	unsigned int num_pairing_failures = 0;
	for (unsigned int v = 0; v < d_vertices.size(); ++v)
	{
		const Vertex &vertex = d_vertices[v];
		const std::vector<Ray> &rays = vertex.rays;
		const unsigned int num_rays = rays.size();
		partner[v].assign(num_rays, boost::none);
		if (num_rays == 0)
		{
			continue;
		}

		std::vector<unsigned int> kept;
		bool unresolved = false;
		for (unsigned int i = 0; i < num_rays; ++i)
		{
			const boost::optional<bool> keep =
					is_kept(d_pieces[ray_end(ray_ref_type(v, i)).piece], operation);
			if (!keep)
			{
				unresolved = true;
			}
			else if (*keep)
			{
				kept.push_back(i);
			}
		}
		if (unresolved)
		{
			++d_diagnostics.unresolved_vertices;
			++num_pairing_failures;
			continue;
		}
		if (kept.size() % 2 != 0)
		{
			++num_pairing_failures;
			continue;
		}
		if (kept.empty())
		{
			continue;
		}

		const auto pair = [&](unsigned int a, unsigned int b)
		{
			partner[v][a] = b;
			partner[v][b] = a;
		};

		// The free rule's vertex: four ends, two of each input, none common.
		std::vector<unsigned int> subject_rays, clip_rays;
		bool any_common = false;
		for (unsigned int i = 0; i < num_rays; ++i)
		{
			any_common = any_common || rays[i].is_common();
			(rays[i].subject_end ? subject_rays : clip_rays).push_back(i);
		}
		const auto inside = [&](unsigned int i)
		{
			return d_pieces[ray_end(ray_ref_type(v, i)).piece].piece_class == CLASS_INSIDE;
		};
		if (num_rays == 4 && !any_common && subject_rays.size() == 2)
		{
			if (kept.size() == 2)
			{
				pair(kept[0], kept[1]);
			}
			else if (inside(subject_rays[0]) != inside(subject_rays[1]))
			{
				// A crossing: the sector (mP, mQ) in the output is bounded by the subject ray whose
				// class is mQ and the clip ray whose class is mP; pair those.
				for (bool m_subject : { false, true })
				{
					for (bool m_clip : { false, true })
					{
						if (is_in_output(operation, m_subject, m_clip))
						{
							const unsigned int s = (inside(subject_rays[0]) == m_clip)
									? subject_rays[0]
									: subject_rays[1];
							const unsigned int c = (inside(clip_rays[0]) == m_subject)
									? clip_rays[0]
									: clip_rays[1];
							pair(s, c);
						}
					}
				}
			}
			else
			{
				// A bounce: each input's two ends pair (the split pairing, section 5.2).
				pair(subject_rays[0], subject_rays[1]);
				pair(clip_rays[0], clip_rays[1]);
			}
			continue;
		}

		if (kept.size() == 2)
		{
			// One pairing, no geometry (also a chain-interior vertex, where only the memberships'
			// xor is known).
			pair(kept[0], kept[1]);
			continue;
		}

		if (!any_common && (subject_rays.empty() || clip_rays.empty()))
		{
			//
			// Only one input has ends here (the other's were a loop or a spike), so the vertex is
			// no intersection for linking: ring continuity (section 5.2). Each kept end pairs
			// with the end of the next non-internal piece of its ring beyond it, at the same stop.
			// (No valid input shows the rule: where the input's passages do not cross, any
			// non-crossing pairing gives the same rings after the split, and the polygon checks
			// skip inputs that cross. A mutation of it survived every case.)
			//
			bool ok = true;
			for (unsigned int i : kept)
			{
				const End end = ray_end(ray_ref_type(v, i));
				const unsigned int ring = d_pieces[end.piece].ring;
				const std::pair<unsigned int, unsigned int> range = ring_piece_range(ring);
				const unsigned int num_ring_pieces = range.second - range.first;
				unsigned int p = end.piece;
				do
				{
					p = range.first + (end.at_end
							? (p - range.first + 1) % num_ring_pieces
							: (p - range.first + num_ring_pieces - 1) % num_ring_pieces);
				}
				while (d_pieces[p].internal && p != end.piece);

				const ray_ref_type neighbour = ray_of_end[2 * p + (end.at_end ? 0 : 1)];
				if (neighbour.first != v ||
					!is_kept(d_pieces[p], operation).get_value_or(false))
				{
					ok = false;
					break;
				}
				pair(i, neighbour.second);
			}
			if (!ok)
			{
				++num_pairing_failures;
			}
			continue;
		}

		//
		// Otherwise the sector walk (without the free rule) on the order used: the output
		// membership of each sector is f(in subject, in clip); a ray is kept iff the output differs
		// across it (counted if not as the keep-rule says); consecutive kept rays pair across
		// output sectors.
		//
		const VertexSolution solution = solve_vertex(v, nullptr, false/*use_free_rule*/);
		bool known = solution.consistent;
		for (unsigned int j = 0; known && j < num_rays; ++j)
		{
			known = solution.in_subject[j] && solution.in_clip[j];
		}
		bool all_common = true;
		for (unsigned int i = 0; i < num_rays; ++i)
		{
			all_common = all_common && rays[i].is_common();
		}
		if (!known && solution.consistent && all_common && !solution.order.empty())
		{
			//
			// Every ray common (both inputs pass the vertex twice along the same two
			// boundaries: identical inputs that touch themselves): a
			// common ray fixes only 'in subject != in clip', so the sectors' memberships are
			// known up to swapping both, and which sectors are the output is not. Pair
			// consecutive kept rays in the order used, in either phase: a non-crossing pairing,
			// which the split then cuts into the one non-crossing decomposition (a ring through
			// the vertex twice is cut there into the arcs that return to it).
			//
			std::vector<unsigned int> sequence;
			for (unsigned int j = 0; j < solution.order.size(); ++j)
			{
				if (std::find(kept.begin(), kept.end(), solution.order[j]) != kept.end())
				{
					sequence.push_back(solution.order[j]);
				}
			}
			for (unsigned int j = 0; j + 1 < sequence.size(); j += 2)
			{
				pair(sequence[j], sequence[j + 1]);
			}
			continue;
		}
		if (!known)
		{
			++d_diagnostics.unresolved_vertices;
			++num_pairing_failures;
			continue;
		}

		std::vector<bool> out(num_rays);
		for (unsigned int j = 0; j < num_rays; ++j)
		{
			out[j] = is_in_output(operation, *solution.in_subject[j], *solution.in_clip[j]);
		}
		// Position j's ray separates sector j - 1 (before it) from sector j (after it).
		std::vector<unsigned int> sequence;
		for (unsigned int j = 0; j < num_rays; ++j)
		{
			if (out[(j + num_rays - 1) % num_rays] != out[j])
			{
				sequence.push_back(j);
			}
		}
		std::vector<unsigned int> kept_by_sector;
		for (unsigned int j : sequence)
		{
			kept_by_sector.push_back(solution.order[j]);
		}
		std::sort(kept_by_sector.begin(), kept_by_sector.end());
		if (kept_by_sector != kept)
		{
			++d_diagnostics.keep_mismatches;
		}
		if (sequence.size() % 2 != 0)
		{
			++num_pairing_failures;
			continue;
		}
		// Start at a kept ray followed by an output sector.
		if (!out[sequence.front()])
		{
			std::rotate(sequence.begin(), sequence.begin() + 1, sequence.end());
		}
		for (unsigned int j = 0; j < sequence.size(); j += 2)
		{
			pair(solution.order[sequence[j]], solution.order[sequence[j + 1]]);
		}
	}

	//
	// Walk the pairs into rings: from an unused paired ray, along its piece (its interior stops,
	// reversed when leaving from the piece's end), to the ray at the piece's other end, then to
	// that ray's partner; until back at the start. Deterministic: rays in vertex and ray order.
	//
	class Entry
	{
	public:
		boost::optional<unsigned int> vertex;   // an overlay vertex, or an interior stop's point
		PointOnSphere position;

		//! At a vertex: the walked piece's stop here; else the stop itself (for provenance).
		unsigned int stop;

		//! At a vertex: the walked piece's stop at its other end (where the next entry arrives).
		unsigned int far_stop;

		Provenance provenance;
	};
	std::vector<std::vector<Entry>> walked;
	std::vector<std::vector<bool>> used(d_vertices.size());
	for (unsigned int v = 0; v < d_vertices.size(); ++v)
	{
		used[v].assign(d_vertices[v].rays.size(), false);
	}
	for (unsigned int v0 = 0; v0 < d_vertices.size(); ++v0)
	{
		for (unsigned int i0 = 0; i0 < d_vertices[v0].rays.size(); ++i0)
		{
			if (used[v0][i0] || !partner[v0][i0])
			{
				continue;
			}

			std::vector<Entry> ring;
			ray_ref_type ray(v0, i0);
			bool closed = false;
			for (unsigned int guard = 0; guard <= 2 * d_pieces.size(); ++guard)
			{
				used[ray.first][ray.second] = true;
				const End end = ray_end(ray);
				const Piece &piece = d_pieces[end.piece];
				ring.push_back(Entry{
						ray.first,
						d_vertices[ray.first].position,
						end.at_end ? piece.end_stop : piece.start_stop,
						end.at_end ? piece.start_stop : piece.end_stop,
						Provenance() });

				// The piece's interior stops, in the direction walked.
				std::vector<unsigned int> stops;
				const RingView &ring_view = d_rings[piece.ring];
				for (unsigned int s = piece.start_stop; ; )
				{
					s = (s + 1 == ring_view.end_stop) ? ring_view.first_stop : s + 1;
					if (s == piece.end_stop)
					{
						break;
					}
					stops.push_back(s);
				}
				if (end.at_end)
				{
					std::reverse(stops.begin(), stops.end());
				}
				for (unsigned int s : stops)
				{
					ring.push_back(Entry{ boost::none, d_stops[s].position, s, s, Provenance() });
				}

				const ray_ref_type arrive = ray_of_end[2 * end.piece + (end.at_end ? 0 : 1)];
				used[arrive.first][arrive.second] = true;
				const boost::optional<unsigned int> next = partner[arrive.first][arrive.second];
				if (!next)
				{
					++num_pairing_failures;
					break;
				}
				ray = ray_ref_type(arrive.first, *next);
				if (ray == ray_ref_type(v0, i0))
				{
					closed = true;
					break;
				}
			}
			if (!closed)
			{
				++num_pairing_failures;
				continue;
			}
			walked.push_back(ring);
		}
	}

	// A kept closed piece (a ring with no record) is a ring on its own.
	for (const Piece &piece : d_pieces)
	{
		if (piece.closed &&
			d_rings[piece.ring].closed &&
			is_kept(piece, operation).get_value_or(false))
		{
			std::vector<Entry> ring;
			const RingView &ring_view = d_rings[piece.ring];
			for (unsigned int s = ring_view.first_stop; s < ring_view.end_stop; ++s)
			{
				ring.push_back(Entry{ boost::none, d_stops[s].position, s, s, Provenance() });
			}
			walked.push_back(ring);
		}
	}

	//
	// Split (section 5.2, split and clean-up): a ring passing an overlay
	// vertex twice (by identity) is v, A, v, B; it is cut into v, A and v, B, until no ring repeats
	// a vertex.
	//
	std::vector<std::vector<Entry>> split;
	while (!walked.empty())
	{
		std::vector<Entry> ring = walked.back();
		walked.pop_back();

		std::vector<int> first_seen(d_vertices.size(), -1);
		boost::optional<std::pair<unsigned int, unsigned int>> cut;
		for (unsigned int j = 0; j < ring.size(); ++j)
		{
			if (!ring[j].vertex)
			{
				continue;
			}
			if (first_seen[*ring[j].vertex] >= 0)
			{
				cut = std::make_pair(static_cast<unsigned int>(first_seen[*ring[j].vertex]), j);
				break;
			}
			first_seen[*ring[j].vertex] = j;
		}
		if (!cut)
		{
			split.push_back(ring);
			continue;
		}
		++d_diagnostics.rings_split;
		walked.push_back(std::vector<Entry>(ring.begin() + cut->first, ring.begin() + cut->second));
		std::vector<Entry> rest(ring.begin() + cut->second, ring.end());
		rest.insert(rest.end(), ring.begin(), ring.begin() + cut->first);
		walked.push_back(rest);
	}
	std::reverse(split.begin(), split.end());  // in walk order

	//
	// Clean-up (section 5.2), last and counted: consecutive repeated positions (exact: they come
	// from one stored position) and a last position equal to the first are removed; a ring with
	// fewer than three distinct positions (a region the records had already collapsed) is dropped.
	//
	for (std::vector<Entry> &ring : split)
	{
		// Provenance after the split (section 5.4): at a vertex, from the stops of the pieces
		// this ring arrives and leaves along; a vertex entry arrives at the previous one's far
		// stop, cyclically, so that each half of a split ring has its own.
		if (d_options.want_provenance)
		{
			boost::optional<unsigned int> arriving_stop;
			for (const Entry &entry : ring)
			{
				if (entry.vertex)
				{
					arriving_stop = entry.far_stop;
				}
			}
			for (Entry &entry : ring)
			{
				if (entry.vertex)
				{
					entry.provenance = get_vertex_provenance(
							*entry.vertex, entry.position, arriving_stop, entry.stop);
					arriving_stop = entry.far_stop;
				}
				else
				{
					entry.provenance = get_original_vertex_provenance(entry.stop);
				}
			}
		}

		std::vector<PointOnSphere> points;
		provenance_seq_type provenances;
		for (const Entry &entry : ring)
		{
			if (!points.empty() && is_same_position(points.back(), entry.position))
			{
				++d_diagnostics.repeated_positions_removed;
				continue;
			}
			points.push_back(entry.position);
			if (d_options.want_provenance)
			{
				provenances.push_back(entry.provenance);
			}
		}
		while (points.size() > 1 && is_same_position(points.front(), points.back()))
		{
			points.pop_back();
			if (d_options.want_provenance)
			{
				provenances.pop_back();
			}
			++d_diagnostics.repeated_positions_removed;
		}

		unsigned int num_distinct = 0;
		for (unsigned int j = 0; j < points.size() && num_distinct < 3; ++j)
		{
			bool repeated = false;
			for (unsigned int k = 0; k < j; ++k)
			{
				repeated = repeated || is_same_position(points[k], points[j]);
			}
			num_distinct += repeated ? 0 : 1;
		}
		if (num_distinct < 3)
		{
			++d_diagnostics.collapsed_rings_dropped;
			continue;
		}

		rings.push_back(OutputRing{ points, provenances });
	}

	d_diagnostics.pairing_failures += num_pairing_failures;
	if (num_pairing_failures != 0 && d_options.throw_if_inconsistent)
	{
		throw InconsistentOverlayException(
				GPLATES_EXCEPTION_SOURCE,
				"The overlay's kept piece ends could not be paired into rings at "
						+ std::to_string(num_pairing_failures) + " vertices.",
				d_diagnostics,
				boost::none);
	}
}


void
GPlatesMaths::PolygonOverlay::Overlay::extract(
		Operation operation,
		std::vector<OutputPolygon> &polygons) const
{
	// Only a polygon subject's output is rings.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_subject_polygon,
			GPLATES_ASSERTION_SOURCE);

	std::vector<OutputRing> rings;
	extract_rings(operation, rings);
	const unsigned int num_rings = rings.size();

	std::vector<PolygonOnSphere::non_null_ptr_to_const_type> ring_polygons;
	for (const OutputRing &ring : rings)
	{
		ring_polygons.push_back(PolygonOnSphere::create(ring.points));
	}

	//
	// The hemisphere check (sections 5.2 and 5.6): the output read by parity
	// over its rings must be f(in subject, in clip). Given rings along the right edges, the parity
	// over their smaller sides is everywhere either the output or its complement (each ring whose
	// output side is its larger side complements the reading), so one point clear of both inputs'
	// outlines decides which. A complement means the output exceeds a hemisphere (a union or a
	// symmetric difference can), or that the classes are wrong (an intersection or a difference
	// cannot exceed one); the check cannot tell which, so it says neither. (Not the output side of
	// each ring from its pieces' classes: a band piece's class can put the output on the side
	// opposite to the ring's own, the thickness-sized rings of the 'tinyhole' hunt all of whose
	// pieces are band pieces, and a ring of an input whose rings cross has no one side.)
	//
	const double h = GeometryIntersect::Intersection::get_on_segment_start_threshold_sine();
	const AngularExtent margin = AngularExtent::create_from_angle(HEMISPHERE_CHECK_MARGIN * h);
	boost::optional<UnitVector3D> reference_point;
	{
		std::vector<UnitVector3D> candidates;
		for (const PolygonOnSphere *polygon : { d_subject_polygon, &d_clip })
		{
			const UnitVector3D &centre = polygon->get_bounding_small_circle().get_centre();
			candidates.push_back(centre);
			candidates.push_back(-centre);
		}
		for (const UnitVector3D &axis : {
				UnitVector3D::xBasis(), UnitVector3D::yBasis(), UnitVector3D::zBasis() })
		{
			candidates.push_back(axis);
			candidates.push_back(-axis);
		}
		for (const UnitVector3D &candidate : candidates)
		{
			const PointOnSphere point(candidate);
			if (!minimum_distance(point, *d_subject_polygon, false, margin) &&
				!minimum_distance(point, d_clip, false, margin))
			{
				reference_point = candidate;
				break;
			}
		}
	}
	if (reference_point)
	{
		const PointOnSphere point(*reference_point);
		bool read_in_output = false;
		for (const auto &ring_polygon : ring_polygons)
		{
			if (ring_polygon->is_point_in_polygon(point, PolygonOnSphere::ADAPTIVE, false))
			{
				read_in_output = !read_in_output;
			}
		}
		const bool in_output = is_in_output(
				operation,
				d_subject_polygon->is_point_in_polygon(point, PolygonOnSphere::ADAPTIVE, false),
				d_clip.is_point_in_polygon(point, d_options.clip_speed, false));
		if (read_in_output != in_output)
		{
			++d_diagnostics.hemisphere_check_failures;
			if (d_options.throw_if_inconsistent)
			{
				throw InconsistentOverlayException(
						GPLATES_EXCEPTION_SOURCE,
						(operation == UNION || operation == SYMMETRIC_DIFFERENCE)
								? "The output's rings would mean its complement: the output "
										"exceeds a hemisphere, or the classes are wrong."
								: "The output's rings would mean its complement: the classes are "
										"wrong.",
						d_diagnostics,
						point);
			}
		}
	}
	else
	{
		++d_diagnostics.hemisphere_check_skipped;
	}

	//
	// Grouping (section 5.2): by containment with parity depth, as the output is read. A ring's
	// depth is the number of rings containing it, and its parent the containing ring of greatest
	// depth. (The rings do not cross but within an overlay vertex, and the number of rings is
	// small: every pair is tested.) The containment test's margin, as the oracle's: ten
	// thicknesses, or twice the widest record cluster if wider (is_ring_inside_ring).
	//
	double cluster_width = 0.0;
	for (const Vertex &vertex : d_vertices)
	{
		const std::vector<unsigned int> vertex_records = get_vertex_records(vertex);
		for (unsigned int a : vertex_records)
		{
			for (unsigned int b : vertex_records)
			{
				cluster_width = (std::max)(cluster_width, angle_between(
						d_graph.unordered_intersections[a].position.position_vector(),
						d_graph.unordered_intersections[b].position.position_vector()));
			}
		}
	}
	const AngularExtent grouping_margin = AngularExtent::create_from_angle(
			(std::max)(HEMISPHERE_CHECK_MARGIN * h, 2 * cluster_width));
	std::vector<std::vector<unsigned int>> containers(num_rings);
	for (unsigned int r = 0; r < num_rings; ++r)
	{
		for (unsigned int other = 0; other < num_rings; ++other)
		{
			if (other == r)
			{
				continue;
			}
			const boost::optional<bool> inside =
					is_ring_inside_ring(rings[r].points, *ring_polygons[other], grouping_margin);
			if (!inside)
			{
				++d_diagnostics.coincident_rings;  // a degenerate output clean-up did not catch
			}
			else if (*inside)
			{
				containers[r].push_back(other);
			}
		}
	}
	std::vector<boost::optional<unsigned int>> parent(num_rings);
	for (unsigned int r = 0; r < num_rings; ++r)
	{
		for (unsigned int other : containers[r])
		{
			if (!parent[r] || containers[other].size() > containers[*parent[r]].size())
			{
				parent[r] = other;
			}
		}
	}

	//
	// Orientation (section 5.2, developer's decision): the output on each ring's left, so an
	// exterior anticlockwise (its interior, the smaller side, on its left) and a hole clockwise.
	// By PolygonOrientation, which projects a ring onto a tangent plane where it can: the spherical
	// area is zero for a ring the thickness across, since every edge of it is zero length to
	// GreatCircleArc. A ring crossing itself (where an input ring does) has an area weighted by
	// winding number, and no orientation has the output on its left all round.
	//
	const auto oriented = [&](unsigned int r, bool exterior)
	{
		OutputRing ring = rings[r];
		const bool interior_on_left =
				PolygonOrientation::calculate_polygon_orientation(*ring_polygons[r]) ==
						PolygonOrientation::COUNTERCLOCKWISE;
		if (interior_on_left != exterior)
		{
			std::reverse(ring.points.begin(), ring.points.end());
			std::reverse(ring.provenances.begin(), ring.provenances.end());
		}
		return ring;
	};

	//
	// The optional sliver filter (section 5.5), by each ring's mean width (get_mean_width).
	//
	std::vector<double> ring_areas(num_rings, 0.0);
	const auto is_sliver = [&](unsigned int r)
	{
		return d_options.sliver_min_width &&
				get_mean_width(rings[r].points, &ring_areas[r]) < *d_options.sliver_min_width;
	};

	// Even depth is an exterior; odd depth a hole of its parent, which has even depth (an island
	// inside a hole is an exterior of its own, filtered on its own). Decided outermost first: a
	// ring inside a dropped ring is dropped with it, uncounted, since a dropped exterior's region
	// goes to the other output and a dropped hole is filled, with all that lies inside either (an
	// island of a filled hole, kept, would overlap its polygon).
	std::vector<unsigned int> outermost_first(num_rings);
	for (unsigned int r = 0; r < num_rings; ++r)
	{
		outermost_first[r] = r;
	}
	std::stable_sort(
			outermost_first.begin(),
			outermost_first.end(),
			[&containers](unsigned int a, unsigned int b)
			{
				return containers[a].size() < containers[b].size();
			});
	std::vector<bool> dropped(num_rings, false);
	for (unsigned int r : outermost_first)
	{
		if (parent[r] && dropped[*parent[r]])
		{
			dropped[r] = true;
		}
		else if (containers[r].size() % 2 == 0)
		{
			if (is_sliver(r))
			{
				dropped[r] = true;
				++d_diagnostics.sliver_exteriors_dropped;
				d_diagnostics.sliver_area_dropped += ring_areas[r];
			}
		}
		else if (parent[r] && containers[*parent[r]].size() % 2 == 0 && is_sliver(r))
		{
			dropped[r] = true;
			++d_diagnostics.sliver_holes_dropped;
		}
	}

	std::vector<boost::optional<unsigned int>> polygon_of_ring(num_rings);
	for (unsigned int r = 0; r < num_rings; ++r)
	{
		if (containers[r].size() % 2 == 0)
		{
			if (dropped[r])
			{
				continue;
			}
			polygon_of_ring[r] = polygons.size();
			polygons.push_back(OutputPolygon{ oriented(r, true), std::vector<OutputRing>() });
		}
	}
	for (unsigned int r = 0; r < num_rings; ++r)
	{
		if (containers[r].size() % 2 == 0 || dropped[r])
		{
			continue;
		}
		if (!parent[r] || !polygon_of_ring[*parent[r]])
		{
			// The containment is not a nesting (it is, for rings that do not cross): kept as an
			// exterior, and counted.
			++d_diagnostics.grouping_failures;
			polygon_of_ring[r] = polygons.size();
			polygons.push_back(OutputPolygon{ oriented(r, true), std::vector<OutputRing>() });
			continue;
		}
		polygons[*polygon_of_ring[*parent[r]]].interiors.push_back(oriented(r, false));
	}
}


void
GPlatesMaths::PolygonOverlay::Overlay::extract_polylines(
		std::vector<OutputPolyline> &polylines) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_subject_polyline,
			GPLATES_ASSERTION_SOURCE);

	const std::pair<unsigned int, unsigned int> range =
			ring_piece_range(d_side_rings_begin[SUBJECT]);

	// The class of each piece; an internal one takes the class before it (after it, at the start).
	std::vector<Class> classes;
	for (unsigned int p = range.first; p < range.second; ++p)
	{
		classes.push_back(d_pieces[p].internal ? CLASS_UNKNOWN : d_pieces[p].piece_class);
	}
	for (unsigned int i = 0; i < classes.size(); ++i)
	{
		if (d_pieces[range.first + i].internal)
		{
			if (i > 0)
			{
				classes[i] = classes[i - 1];  // already the class of a non-internal piece before
			}
			else
			{
				for (unsigned int j = 1; j < classes.size(); ++j)
				{
					if (!d_pieces[range.first + j].internal)
					{
						classes[i] = classes[j];
						break;
					}
				}
			}
		}
	}

	// Each stop's own position (the record's on the polyline, or the polyline's vertex), not its
	// vertex's output position as a polygon's rings use (where every ring through the vertex must
	// share it): an identity vertex can extend far along a dense run of touches (the 'along'
	// generator's), so its output position can be far off the polyline, and a polyline's adjacent
	// runs need only share the stop between them.
	const auto stop_position = [](const Stop &stop) -> const PointOnSphere &
	{
		return stop.position;
	};

	boost::optional<Class> run_class;
	for (unsigned int i = 0; i < classes.size(); ++i)
	{
		const Piece &piece = d_pieces[range.first + i];
		if (!run_class || *run_class != classes[i])
		{
			polylines.push_back(OutputPolyline{
					std::vector<PointOnSphere>(),
					classes[i] == CLASS_INSIDE,
					provenance_seq_type() });
			run_class = classes[i];
		}
		std::vector<PointOnSphere> &points = polylines.back().points;
		for (unsigned int s = piece.start_stop; s <= piece.end_stop; ++s)
		{
			const PointOnSphere &position = stop_position(d_stops[s]);
			if (points.empty() || !is_same_position(points.back(), position))
			{
				points.push_back(position);
				if (d_options.want_provenance)
				{
					polylines.back().provenances.push_back(get_polyline_stop_provenance(s));
				}
			}
		}
	}
}


unsigned int
GPlatesMaths::PolygonOverlay::Overlay::ring_of_stop(
		unsigned int stop_index) const
{
	unsigned int ring = 0;
	while (!(stop_index >= d_rings[ring].first_stop && stop_index < d_rings[ring].end_stop))
	{
		++ring;
	}
	return ring;
}


GPlatesMaths::PolygonOverlay::Provenance
GPlatesMaths::PolygonOverlay::Overlay::get_vertex_provenance(
		unsigned int vertex_index,
		const PointOnSphere &position,
		boost::optional<unsigned int> arriving_stop,
		boost::optional<unsigned int> leaving_stop) const
{
	//
	// Section 5.4: a vertex is output at one position, so its provenance is that
	// position's. A vertex holding a subject vertex is that vertex; otherwise the point is on the
	// subject only where the walk runs along a subject piece here (else it is a cut), at the ratio
	// of the position's projection onto that piece's segment. Every vertex lies on the clip.
	//
	const Vertex &vertex = d_vertices[vertex_index];

	// The walked pieces' stop at this vertex of one side (none if neither walked piece is).
	const auto stop_of_side = [&](Side side) -> boost::optional<unsigned int>
	{
		for (const boost::optional<unsigned int> &stop : { leaving_stop, arriving_stop })
		{
			if (stop && d_rings[ring_of_stop(*stop)].side == side)
			{
				return stop;
			}
		}
		return boost::none;
	};
	// A segment location of a stop inside a segment, by projecting the position.
	const auto segment_location = [&](unsigned int stop_index)
	{
		const Stop &stop = d_stops[stop_index];
		const unsigned int ring = ring_of_stop(stop_index);
		const RingView &ring_view = d_rings[ring];
		return std::make_pair(ring, Location{
				ring_view.ring_index,
				stop.segment_out,
				ratio_along_segment(ring_view.segments[stop.segment_out], position) });
	};

	Provenance provenance;
	provenance.clip_index = d_options.clip_index;

	if (vertex.clip_vertex)
	{
		provenance.clip =
				Location{ vertex.clip_vertex->ring, vertex.clip_vertex->index, boost::none };
	}
	else if (const boost::optional<unsigned int> stop = stop_of_side(CLIP))
	{
		provenance.clip = segment_location(*stop).second;
	}
	else
	{
		// The clip's segment of the vertex's first record (no clip piece is walked here).
		const GeometryIntersect::Intersection &record =
				d_graph.unordered_intersections[d_vertex_records[vertex.first_record]];
		const unsigned int ring = ring_of_segment(CLIP, record.segment_index2);
		const RingView &ring_view = d_rings[ring];
		const unsigned int segment = record.segment_index2 - ring_view.first_segment_index;
		provenance.clip = Location{
				ring_view.ring_index,
				segment,
				ratio_along_segment(ring_view.segments[segment], position) };
	}

	// The subject's location, after the clip's: a vertex lies on this clip, so an earlier clip's
	// location from a cascade's table (section 5.4) does not replace this one.
	if (vertex.subject_vertex)
	{
		const RingView &ring = d_rings[d_side_rings_begin[SUBJECT] + vertex.subject_vertex->ring];
		compose_subject_location(
				ring,
				Location{ vertex.subject_vertex->ring, vertex.subject_vertex->index, boost::none },
				provenance);
	}
	else if (const boost::optional<unsigned int> stop = stop_of_side(SUBJECT))
	{
		const std::pair<unsigned int, Location> location = segment_location(*stop);
		compose_subject_location(d_rings[location.first], location.second, provenance);
	}
	return provenance;
}


GPlatesMaths::PolygonOverlay::Provenance
GPlatesMaths::PolygonOverlay::Overlay::get_original_vertex_provenance(
		unsigned int stop_index) const
{
	const RingView &ring_view = d_rings[ring_of_stop(stop_index)];
	const Location location{ ring_view.ring_index, d_stops[stop_index].segment_out, boost::none };

	Provenance provenance;
	provenance.clip_index = d_options.clip_index;
	if (ring_view.side == SUBJECT)
	{
		compose_subject_location(ring_view, location, provenance);
	}
	else
	{
		provenance.clip = location;  // a vertex of the clip inside the subject (a cut)
	}
	return provenance;
}


GPlatesMaths::PolygonOverlay::Provenance
GPlatesMaths::PolygonOverlay::Overlay::get_polyline_stop_provenance(
		unsigned int stop_index) const
{
	const Stop &stop = d_stops[stop_index];
	if (stop.kind != Stop::RECORD)
	{
		return get_original_vertex_provenance(stop_index);
	}

	// A record on the polyline: the stop itself (the polyline's output is at its stops), and the
	// clip's location of the stop's own record, not its vertex's (an identity vertex along a dense
	// run of touches can name a clip vertex far from the stop). A self-touch cut
	// names no record of the polyline: its vertex's.
	const RingView &ring_view = d_rings[d_side_rings_begin[SUBJECT]];
	Provenance provenance;
	provenance.clip_index = d_options.clip_index;
	if (stop.num_records != 0)
	{
		const GeometryIntersect::Intersection &record =
				d_graph.unordered_intersections[d_stop_records[stop.first_record]];
		const RingView &clip_ring = d_rings[ring_of_segment(CLIP, record.segment_index2)];
		const unsigned int segment = record.segment_index2 - clip_ring.first_segment_index;
		provenance.clip = record.is_on_segment2_start()
				? Location{ clip_ring.ring_index, segment, boost::none }
				: Location{
						clip_ring.ring_index,
						segment,
						ratio_along_segment(clip_ring.segments[segment], stop.position) };
	}
	else
	{
		provenance.clip =
				get_vertex_provenance(stop.vertex, stop.position, boost::none, boost::none).clip;
	}
	compose_subject_location(
			ring_view,
			stop.at_vertex
					? Location{ 0, stop.segment_out, boost::none }
					: Location{ 0, stop.segment_out, stop.ratio },
			provenance);
	return provenance;
}


void
GPlatesMaths::PolygonOverlay::Overlay::compose_subject_location(
		const RingView &ring,
		const Location &location,
		Provenance &provenance) const
{
	const provenance_seq_type *table = d_options.subject_provenance;
	if (!table)
	{
		provenance.subject = location;
		return;
	}

	// The table's index of a vertex of the subject (its rings' points in order).
	const auto table_index = [&](unsigned int ring_index, unsigned int vertex_index)
	{
		unsigned int index = 0;
		for (unsigned int r = d_side_rings_begin[SUBJECT]; r < d_side_rings_end[SUBJECT]; ++r)
		{
			if (d_rings[r].ring_index == ring_index)
			{
				break;
			}
			index += d_rings[r].num_vertices();
		}
		return index + vertex_index;
	};
	unsigned int num_points = 0;
	for (unsigned int r = d_side_rings_begin[SUBJECT]; r < d_side_rings_end[SUBJECT]; ++r)
	{
		num_points += d_rings[r].num_vertices();
	}
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			table->size() == num_points,
			GPLATES_ASSERTION_SOURCE);

	// A vertex of the piece: its own provenance (from the original, or an earlier cut).
	if (!location.ratio)
	{
		const Provenance &entry = (*table)[table_index(location.ring, location.index)];
		provenance.subject = entry.subject;
		if (!provenance.clip)
		{
			provenance.clip = entry.clip;
			provenance.clip_index = entry.clip_index;
		}
		return;
	}

	//
	// A point inside the piece's segment j, between its points a = j and b = j + 1 (cyclically
	// for a ring): every edge of a piece is a sub-arc of one edge of its own input, by induction
	// through the cascade, so a and b lie on one segment S of the original (or of an earlier clip),
	// and the ratio along S is t_a + t (t_b - t_a). Not quite exactly: a piece's vertex can be off
	// S by up to its identity group's extent (its output position), so a composed location is a
	// projection; and not always (below).
	//
	const unsigned int a_index = location.index;
	const unsigned int b_index =
			(location.index + 1 == ring.num_vertices()) ? 0 : location.index + 1;
	const Provenance &a = (*table)[table_index(location.ring, a_index)];
	const Provenance &b = (*table)[table_index(location.ring, b_index)];

	// The segment two locations of one input lie on, and their ratios along it. @a input is that
	// input, for a polygon's wrap-around (vertex 0 ends its ring's last segment); without it (or
	// for a polyline, none) no wrap-around is assumed, so that a pair of locations on no one
	// segment is a mismatch, never a wrong segment.
	const auto shared_segment = [](
			const Location &x,
			const Location &y,
			const PolygonOnSphere *input)
			-> boost::optional<std::tuple<unsigned int, unsigned int, double, double>>
	{
		if (x.ring != y.ring)
		{
			return boost::none;
		}
		boost::optional<unsigned int> last_segment;
		if (input && x.ring <= input->number_of_interior_rings())
		{
			const unsigned int num_vertices = (x.ring == 0)
					? input->number_of_vertices_in_exterior_ring()
					: input->number_of_vertices_in_interior_ring(x.ring - 1);
			last_segment = num_vertices - 1;
		}
		const auto on_segment = [&](const Location &l, unsigned int segment)
				-> boost::optional<double>
		{
			if (l.ratio)
			{
				return l.index == segment ? boost::optional<double>(*l.ratio) : boost::none;
			}
			if (l.index == segment)
			{
				return 0.0;
			}
			if (l.index == segment + 1 ||
				(l.index == 0 && last_segment && segment == *last_segment))
			{
				return 1.0;
			}
			return boost::none;
		};
		// The segments either location can be on: its own, or the two at a vertex.
		std::vector<unsigned int> candidates;
		for (const Location *l : { &x, &y })
		{
			candidates.push_back(l->index);
			if (!l->ratio && l->index > 0)
			{
				candidates.push_back(l->index - 1);
			}
			if (!l->ratio && l->index == 0 && last_segment)
			{
				candidates.push_back(*last_segment);
			}
		}
		for (unsigned int segment : candidates)
		{
			const boost::optional<double> tx = on_segment(x, segment);
			const boost::optional<double> ty = on_segment(y, segment);
			if (tx && ty)
			{
				return std::make_tuple(x.ring, segment, *tx, *ty);
			}
		}
		return boost::none;
	};

	// The edge is along the original subject if both ends are on one of its segments; else along
	// an earlier clip if both are on one of its segments (an edge from the subject's outline to a
	// cut point is along the earlier clip, though its first end is on the subject too).
	const double t = *location.ratio;
	if (a.subject && b.subject)
	{
		if (const auto segment = shared_segment(
				*a.subject,
				*b.subject,
				dynamic_cast<const PolygonOnSphere *>(d_options.original_subject)))
		{
			provenance.subject = Location{
					std::get<0>(*segment),
					std::get<1>(*segment),
					std::get<2>(*segment) +
							t * (std::get<3>(*segment) - std::get<2>(*segment)) };
			return;
		}
	}
	if (a.clip && b.clip && a.clip_index == b.clip_index)
	{
		// Both ends on an earlier clip's outline: a point of an earlier cut.
		if (const auto segment = shared_segment(
				*a.clip,
				*b.clip,
				(d_options.cascade_clips && a.clip_index < d_options.cascade_clips->size())
						? (*d_options.cascade_clips)[a.clip_index]
						: nullptr))
		{
			provenance.subject = boost::none;
			if (!provenance.clip)
			{
				provenance.clip = Location{
						std::get<0>(*segment),
						std::get<1>(*segment),
						std::get<2>(*segment) +
							t * (std::get<3>(*segment) - std::get<2>(*segment)) };
				provenance.clip_index = a.clip_index;
			}
			return;
		}
	}

	// The induction fails where an identity vertex holds two vertices of one input (dense shared
	// edges): a piece's edge then spans the group, from a vertex before it to one after it, so
	// its ends are on no one segment; and where a point at an input's corner lies within the
	// thickness of both its segments but names only one (section 5.4). No subject location,
	// counted (never a wrong value; a coverage interpolates such a point along its run, as a cut
	// point, section 5.4): a subject location lost if both ends had one, else nothing lost.
	provenance.subject = boost::none;
	++((a.subject && b.subject)
			? d_diagnostics.provenance_subject_mismatches
			: d_diagnostics.provenance_clip_mismatches);
}


std::pair<unsigned int, unsigned int>
GPlatesMaths::PolygonOverlay::Overlay::ring_piece_range(
		unsigned int ring) const
{
	// The pieces are stored ring by ring.
	unsigned int first = 0;
	while (first < d_pieces.size() && d_pieces[first].ring != ring)
	{
		++first;
	}
	unsigned int end = first;
	while (end < d_pieces.size() && d_pieces[end].ring == ring)
	{
		++end;
	}
	return std::make_pair(first, end);
}
