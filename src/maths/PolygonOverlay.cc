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
#include <map>
#include <utility>

#include "PolygonOverlay.h"

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
		}
	}
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

	for (unsigned int ring = 0; ring < d_rings.size(); ++ring)
	{
		build_stops_and_pieces(ring);
	}
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
	// the original vertex itself; then the records inside the segment leaving it.
	//
	ring_view.first_stop = d_stops.size();
	for (unsigned int k = 0; k < num_vertices; ++k)
	{
		const unsigned int segment_in = ring_view.closed
				? (k + num_vertices - 1) % num_vertices
				: (k == 0 ? 0 : k - 1);

		if (!at_vertex_records[k].empty())
		{
			bool first_stop_at_vertex = true;
			for (unsigned int r : at_vertex_records[k])
			{
				const unsigned int vertex = d_vertex_of_record[r];

				// Records of one overlay vertex at the same point of the ring are one stop.
				if (!first_stop_at_vertex && d_stops.back().vertex == vertex)
				{
					d_stop_records.push_back(r);
					++d_stops.back().num_records;
					continue;
				}

				// A second, different overlay vertex at the same ring vertex follows the first at
				// zero distance along segment 'k', so it arrives along that segment.
				d_stops.push_back(Stop{
						Stop::RECORD,
						first_stop_at_vertex ? segment_in : k,
						k,
						true,
						vertex,
						static_cast<unsigned int>(d_stop_records.size()),
						1,
						ring_view.vertex(k),
						0.0});
				d_stop_records.push_back(r);
				first_stop_at_vertex = false;
			}
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
		Piece piece{ring, start, end, closed, false, boost::none, boost::none, boost::none};

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
					boost::none, boost::none, boost::none};
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
