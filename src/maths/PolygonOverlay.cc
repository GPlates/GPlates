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
#include <ostream>
#include <unordered_map>
#include <utility>

#include "PolygonOverlay.h"

#include "GeometryDistance.h"
#include "MathsUtils.h"
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
			 * measured order; never reached in Phase 4a's 3013 random cases, most was 4).
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

	apply_loop_and_spike_rules();
	find_common_pairs();
	build_rays();
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
				// (phase4a-run4.md sections 2.4 and 3.3).
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

		// The cone (phase4b-design.md section 3.8): 0 when the arc starts at the vertex's output
		// position (the same stored point), pi when the arc is no longer than its offset from
		// there, else the planar bound atan2(delta, L - delta). At these scales the spherical
		// correction to the bound is far below rounding.
		ray.delta = is_same_position(from.position, vertex.position)
				? 0.0
				: angle_between(centre, start);
		ray.first_arc_length = angle_between(start, to.position.position_vector());
		ray.reach = angle_between(centre, to.position.position_vector());
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
	// so near-ties fall where it leaves them (phase4b-design.md section 3.6: measured harmless).
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
	// The geometry-free rule (phase4b-design.md section 3.7.1, case 1): with four ends, two of
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
		std::vector<Deduction> *deductions) const
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
	if (num_rays == 4)
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
	// The sector walk (phase4b-design.md section 3.7.1, case 2). Each classified ray fixes the
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
	// The one-passage rule (phase4a-run4.md section 3.1; phase4b-design.md 3.7.1, case 1b): at six
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
	// (Kendall distance) against the measured order, ties by enumeration order (phase4b-design.md
	// section 3.8, step 3). Unbundled, the measured order is the only one.
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
	// A polyline subject is classified by its own rules (phase4b-design.md section 6.1).
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
	// deduction for a piece with a margin (overlay-walkthrough.md section 7.5). The distance is
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

	// (d) Anchorless chains: phase4b-design.md section 3.9, option 3 (a later step).

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

	//
	// The result, from what the intersection and the difference keep (phase4b-design.md section
	// 5.1): the intersection keeps subject pieces inside the clip, clip pieces inside the subject
	// and same-side common pairs; the difference keeps subject pieces outside the clip, clip pieces
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
	// Anchored and floating rays (phase4b-design.md section 3.8, step 1). In order of certainty
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
		// anchors (phase4a-run4.md).
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
	if (bundle.floating.empty() || bundle.anchored.empty())
	{
		return bundle;
	}
	if (bundle.floating.size() > BUNDLE_CAP)
	{
		bundle.capped = true;
		return bundle;
	}

	//
	// Admissible orders (step 2). The reference ray is the most certain anchored one; every ray's
	// angle about the vertex is measured from it, and its interval is that angle plus or minus its
	// cone plus the reference's. An order places the floating rays into the gaps between the
	// anchored rays (which keep their measured order), in any permutation within a gap, such that
	// the angles can be chosen non-decreasing round the turn, each inside its interval. The
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
