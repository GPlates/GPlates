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
#include <string>
#include <vector>
#include <boost/optional/optional_io.hpp>
#include <gtest/gtest.h>

#include "maths/GeometryIntersect.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolygonOverlay.h"
#include "maths/PolylineOnSphere.h"
#include "maths/UnitVector3D.h"
#include "maths/Vector3D.h"


//
// Tests of PolygonOverlay, the overlay of a subject (polygon or polyline) with a clip polygon
// under polygon partitioning and the Boolean operations.
//
// The cases are families of the Phase 4a catalogue (the planning repository's
// 'plans/polygon-intersection-tools/overlay_catalogue.py'), built as there in local tangent
// coordinates with distances in units of the thickness, H, but at a centre on no axis (as
// GeometryIntersectTest.cc does), so that nothing lines up with a coordinate axis by accident.
//

namespace
{
	using GPlatesMaths::GeometryIntersect::Intersection;
	using GPlatesMaths::PointOnSphere;
	using GPlatesMaths::PolygonOnSphere;
	using GPlatesMaths::PolylineOnSphere;
	using GPlatesMaths::UnitVector3D;
	using GPlatesMaths::Vector3D;
	namespace Overlay = GPlatesMaths::PolygonOverlay;

	//! The thickness, as a sine (a distance in radians, for these small distances).
	const double H = Intersection::get_on_segment_start_threshold_sine();


	PointOnSphere
	lat_lon_radians(
			double lat,
			double lon)
	{
		return PointOnSphere(UnitVector3D(
				std::cos(lat) * std::cos(lon),
				std::cos(lat) * std::sin(lon),
				std::sin(lat)));
	}


	/**
	 * Local tangent coordinates at a point: (east, north) offsets, in radians for small offsets.
	 */
	class Frame
	{
	public:

		explicit
		Frame(
				const PointOnSphere &centre) :
			d_centre(centre.position_vector()),
			d_east(cross(UnitVector3D(0, 0, 1), centre.position_vector()).get_normalisation()),
			d_north(cross(centre.position_vector(), d_east).get_normalisation())
		{  }

		PointOnSphere
		at(
				double east,
				double north) const
		{
			return PointOnSphere(
					(Vector3D(d_centre) + east * d_east + north * d_north).get_normalisation());
		}

	private:
		UnitVector3D d_centre;
		UnitVector3D d_east;
		UnitVector3D d_north;
	};

	//! The default frame, at a centre on no axis.
	const Frame F(lat_lon_radians(0.37, 0.81));


	typedef std::vector<PointOnSphere> ring_type;

	ring_type
	square(
			double x0,
			double y0,
			double x1,
			double y1)
	{
		return ring_type{ F.at(x0, y0), F.at(x1, y0), F.at(x1, y1), F.at(x0, y1) };
	}

	PolygonOnSphere::non_null_ptr_to_const_type
	polygon(
			const ring_type &exterior,
			const std::vector<ring_type> &interiors = {})
	{
		return PolygonOnSphere::create(exterior, interiors);
	}


	//! The records' types, as the catalogue writes them, sorted ("LL LL SL").
	std::string
	record_kinds(
			const Overlay::Overlay &overlay)
	{
		std::vector<std::string> kinds;
		for (const Intersection &record : overlay.get_graph().unordered_intersections)
		{
			switch (record.type)
			{
			case Intersection::SEGMENTS_CROSS:
				kinds.push_back("LL");
				break;
			case Intersection::SEGMENT_START_ON_SEGMENT_START:
				kinds.push_back("SS");
				break;
			case Intersection::SEGMENT1_START_ON_SEGMENT2:
				kinds.push_back("SL");
				break;
			case Intersection::SEGMENT2_START_ON_SEGMENT1:
				kinds.push_back("LS");
				break;
			}
		}
		std::sort(kinds.begin(), kinds.end());

		std::string result;
		for (const std::string &kind : kinds)
		{
			result += (result.empty() ? "" : " ") + kind;
		}
		return result;
	}


	//! The pieces of one ring (by side and ring index), in ring order.
	std::vector<const Overlay::Piece *>
	ring_pieces(
			const Overlay::Overlay &overlay,
			Overlay::Side side,
			unsigned int ring_index)
	{
		std::vector<const Overlay::Piece *> pieces;
		for (const Overlay::Piece &piece : overlay.get_pieces())
		{
			const Overlay::RingView &ring = overlay.get_rings()[piece.ring];
			if (ring.side == side && ring.ring_index == ring_index)
			{
				pieces.push_back(&piece);
			}
		}
		return pieces;
	}


	/**
	 * The structural invariants every overlay must satisfy, whatever the input.
	 */
	void
	check_structure(
			const Overlay::Overlay &overlay)
	{
		const std::vector<Overlay::Vertex> &vertices = overlay.get_vertices();
		const std::vector<Overlay::Stop> &stops = overlay.get_stops();
		const unsigned int num_records = overlay.get_graph().unordered_intersections.size();

		// Every record is in exactly one vertex, and vertices list their records ascending.
		std::vector<unsigned int> record_count(num_records, 0);
		for (unsigned int v = 0; v < vertices.size(); ++v)
		{
			const std::vector<unsigned int> records = overlay.get_vertex_records(vertices[v]);
			ASSERT_FALSE(records.empty());
			EXPECT_TRUE(std::is_sorted(records.begin(), records.end()));
			for (unsigned int r : records)
			{
				++record_count[r];
				EXPECT_EQ(overlay.get_vertex_of_record(r), v);
			}
		}
		for (unsigned int r = 0; r < num_records; ++r)
		{
			EXPECT_EQ(record_count[r], 1u) << "record " << r;
		}

		// Every record is in exactly one stop of each side's rings.
		for (unsigned int side = Overlay::SUBJECT; side <= Overlay::CLIP; ++side)
		{
			std::vector<unsigned int> stop_count(num_records, 0);
			for (const Overlay::RingView &ring : overlay.get_rings())
			{
				if (ring.side != side)
				{
					continue;
				}
				for (unsigned int s = ring.first_stop; s < ring.end_stop; ++s)
				{
					for (unsigned int r : overlay.get_stop_records(stops[s]))
					{
						++stop_count[r];
						EXPECT_EQ(stops[s].kind, Overlay::Stop::RECORD);
						EXPECT_EQ(stops[s].vertex, overlay.get_vertex_of_record(r));
					}
				}
			}
			for (unsigned int r = 0; r < num_records; ++r)
			{
				EXPECT_EQ(stop_count[r], 1u) << "side " << side << " record " << r;
			}
		}

		// Each end registered at a vertex is the piece's end there; each polygon has an even
		// number of ends at a vertex (it arrives and leaves), except a polyline's.
		for (unsigned int v = 0; v < vertices.size(); ++v)
		{
			unsigned int ends_per_side[2] = { 0, 0 };
			for (const Overlay::End &end : vertices[v].ends)
			{
				const Overlay::Piece &piece = overlay.get_pieces()[end.piece];
				EXPECT_EQ(
						end.at_end ? piece.end_vertex : piece.start_vertex,
						boost::optional<unsigned int>(v));
				++ends_per_side[overlay.get_rings()[piece.ring].side];
			}
			EXPECT_EQ(ends_per_side[Overlay::CLIP] % 2, 0u) << "vertex " << v;
		}

		// A piece's first and last points are the positions of its end stops.
		for (const Overlay::Piece &piece : overlay.get_pieces())
		{
			const std::vector<PointOnSphere> points = overlay.get_piece_points(piece);
			ASSERT_GE(points.size(), piece.closed ? 2u : 2u);
			if (!piece.closed)
			{
				EXPECT_EQ(points.front(), stops[piece.start_stop].position);
				EXPECT_EQ(points.back(), stops[piece.end_stop].position);
			}
		}
	}
}


// Two squares overlapping at a corner: two crossings, each its own vertex with four ends, and two
// pieces per ring, each with an original vertex to test.
TEST(PolygonOverlayTest, StructureTwoSquares)
{
	const auto P = polygon(square(-0.02, -0.02, 0.005, 0.005));
	const auto Q = polygon(square(-0.005, -0.005, 0.02, 0.02));
	const Overlay::Overlay overlay(*P, *Q);
	check_structure(overlay);

	EXPECT_EQ(record_kinds(overlay), "LL LL");
	ASSERT_EQ(overlay.get_vertices().size(), 2u);
	for (const Overlay::Vertex &vertex : overlay.get_vertices())
	{
		EXPECT_EQ(vertex.num_records, 1u);
		EXPECT_EQ(vertex.ends.size(), 4u);
		EXPECT_FALSE(vertex.subject_vertex);
		EXPECT_FALSE(vertex.clip_vertex);
	}

	for (Overlay::Side side : { Overlay::SUBJECT, Overlay::CLIP })
	{
		const auto pieces = ring_pieces(overlay, side, 0);
		ASSERT_EQ(pieces.size(), 2u);
		for (const Overlay::Piece *piece : pieces)
		{
			EXPECT_FALSE(piece->closed);
			EXPECT_TRUE(piece->has_interior_vertex);
			EXPECT_FALSE(piece->single_arc_segment);
			EXPECT_NE(piece->start_vertex, piece->end_vertex);
		}
	}
}


// No record: every ring is one closed piece (the polygon wholly inside, doc 5.1's first bug).
TEST(PolygonOverlayTest, StructureClipInsideSubject)
{
	const auto P = polygon(square(-0.02, -0.02, 0.02, 0.02));
	const auto Q = polygon(square(-0.005, -0.005, 0.005, 0.005));
	const Overlay::Overlay overlay(*P, *Q);
	check_structure(overlay);

	EXPECT_TRUE(overlay.get_graph().unordered_intersections.empty());
	EXPECT_TRUE(overlay.get_vertices().empty());
	ASSERT_EQ(overlay.get_pieces().size(), 2u);
	for (const Overlay::Piece &piece : overlay.get_pieces())
	{
		EXPECT_TRUE(piece.closed);
		EXPECT_TRUE(piece.has_interior_vertex);
		EXPECT_EQ(overlay.get_piece_points(piece).size(), 4u);
	}
}


// The clip crosses only the subject's hole: its interior ring takes part (doc 5.1's second bug).
TEST(PolygonOverlayTest, StructureHoleCrossedOnly)
{
	const auto P = polygon(
			square(-0.02, -0.02, 0.02, 0.02),
			{ square(-0.005, -0.005, 0.005, 0.005) });
	const auto Q = polygon(square(-0.002, -0.002, 0.002, 0.01));
	const Overlay::Overlay overlay(*P, *Q);
	check_structure(overlay);

	EXPECT_EQ(record_kinds(overlay), "LL LL");
	EXPECT_EQ(ring_pieces(overlay, Overlay::SUBJECT, 0).size(), 1u);
	EXPECT_TRUE(ring_pieces(overlay, Overlay::SUBJECT, 0)[0]->closed);
	EXPECT_EQ(ring_pieces(overlay, Overlay::SUBJECT, 1).size(), 2u);
	EXPECT_EQ(ring_pieces(overlay, Overlay::CLIP, 0).size(), 2u);
}


// The acute wedge (catalogue 4.1): a subject vertex x on the bisector of a 30 degree corner of the
// clip, within the thickness of both its edges but not of its apex. The two touch records name x
// and are one vertex; the clip's tip piece runs x, apex, x; so six ends meet there.
TEST(PolygonOverlayTest, StructureAcuteWedge)
{
	const double half_angle = 15.0 * 3.14159265358979323846 / 180.0;
	const double L = 0.02;
	const double r = 2.0 * H;
	const ring_type wedge{
			F.at(0.0, 0.0),
			F.at(-L * std::sin(half_angle), -L * std::cos(half_angle)),
			F.at(0.0, -1.5 * L),
			F.at(L * std::sin(half_angle), -L * std::cos(half_angle)) };
	const ring_type straight{
			F.at(-0.04, -r), F.at(0.0, -r), F.at(0.04, -r), F.at(0.04, -0.05), F.at(-0.04, -0.05) };

	const auto P = polygon(straight);
	const auto Q = polygon(wedge);
	const Overlay::Overlay overlay(*P, *Q);
	check_structure(overlay);

	EXPECT_EQ(record_kinds(overlay), "SL SL");
	ASSERT_EQ(overlay.get_vertices().size(), 1u);
	const Overlay::Vertex &vertex = overlay.get_vertices()[0];
	EXPECT_EQ(vertex.num_records, 2u);
	ASSERT_TRUE(vertex.subject_vertex);
	EXPECT_EQ(vertex.subject_vertex->ring, 0u);
	EXPECT_EQ(vertex.subject_vertex->index, 1u);
	EXPECT_FALSE(vertex.clip_vertex);
	EXPECT_EQ(vertex.position, F.at(0.0, -r));  // the subject's vertex, exactly
	EXPECT_EQ(vertex.ends.size(), 6u);

	// The subject's ring has one stop holding both records, and one piece all the way round.
	const auto subject_pieces = ring_pieces(overlay, Overlay::SUBJECT, 0);
	ASSERT_EQ(subject_pieces.size(), 1u);
	EXPECT_EQ(subject_pieces[0]->start_stop, subject_pieces[0]->end_stop);
	EXPECT_EQ(overlay.get_stops()[subject_pieces[0]->start_stop].num_records, 2u);
	EXPECT_TRUE(subject_pieces[0]->has_interior_vertex);

	// The clip's ring: the tip piece (x, apex, x) and the rest, both from the vertex to itself.
	const auto clip_pieces = ring_pieces(overlay, Overlay::CLIP, 0);
	ASSERT_EQ(clip_pieces.size(), 2u);
	unsigned int num_tip_pieces = 0;
	for (const Overlay::Piece *piece : clip_pieces)
	{
		EXPECT_EQ(piece->start_vertex, boost::optional<unsigned int>(0));
		EXPECT_EQ(piece->end_vertex, boost::optional<unsigned int>(0));
		EXPECT_TRUE(piece->has_interior_vertex);
		const std::vector<PointOnSphere> points = overlay.get_piece_points(*piece);
		if (points.size() == 3)
		{
			++num_tip_pieces;
			EXPECT_EQ(points[1], F.at(0.0, 0.0));
		}
	}
	EXPECT_EQ(num_tip_pieces, 1u);
}


// A twist on the clip (catalogue 4.3): the subject's two crossing edges both cross the clip's top
// edge near the crossing X. Crossings name no original vertex, so every record is its own vertex,
// even where two lie at one point.
TEST(PolygonOverlayTest, StructureTwistOnClip)
{
	const ring_type bowtie{
			F.at(-0.01, 0.01), F.at(0.01, -0.01), F.at(0.01, 0.01), F.at(-0.01, -0.01) };
	for (double d : { 3.0 * H, 0.0 })
	{
		SCOPED_TRACE(d / H);
		const auto P = polygon(bowtie);
		const auto Q = polygon(ring_type{
				F.at(-0.03, d), F.at(0.03, d), F.at(0.03, -0.03), F.at(-0.03, -0.03) });
		const Overlay::Overlay overlay(*P, *Q);
		check_structure(overlay);

		EXPECT_EQ(record_kinds(overlay), "LL LL LL LL");
		EXPECT_EQ(overlay.get_vertices().size(), 4u);
		for (const Overlay::Vertex &vertex : overlay.get_vertices())
		{
			EXPECT_EQ(vertex.num_records, 1u);
			EXPECT_EQ(vertex.ends.size(), 4u);
		}
	}
}


// Two clip vertices within the thickness of one subject vertex (catalogue 4.6): every record naming
// any of them is one vertex, and the clip's short edge between them is a single arc from that
// vertex back to itself (the loop the loop rule will make internal).
TEST(PolygonOverlayTest, StructureTwoClipVerticesAtOneSubjectVertex)
{
	const double d = 0.8 * H;
	const ring_type clip{
			F.at(0.0, 0.0), F.at(0.0, -0.02), F.at(0.02, -0.02), F.at(0.02, 0.0), F.at(d, 0.0) };
	const ring_type subject{
			F.at(-0.01, 0.003), F.at(d / 2, 0.3 * H), F.at(0.01, -0.003),
			F.at(0.01, -0.03), F.at(-0.01, -0.03) };
	const auto P = polygon(subject);
	const auto Q = polygon(clip);
	const Overlay::Overlay overlay(*P, *Q);
	check_structure(overlay);

	// The vertex holding subject vertex 1 also holds clip vertices 0 and 4.
	bool found = false;
	for (unsigned int v = 0; v < overlay.get_vertices().size(); ++v)
	{
		const Overlay::Vertex &vertex = overlay.get_vertices()[v];
		if (!vertex.subject_vertex || vertex.subject_vertex->index != 1)
		{
			continue;
		}
		found = true;
		EXPECT_EQ(vertex.position, F.at(d / 2, 0.3 * H));

		std::vector<unsigned int> clip_vertices;
		for (unsigned int r : overlay.get_vertex_records(vertex))
		{
			const Intersection &record = overlay.get_graph().unordered_intersections[r];
			if (record.is_on_segment2_start())
			{
				clip_vertices.push_back(record.segment_index2);
			}
		}
		std::sort(clip_vertices.begin(), clip_vertices.end());
		clip_vertices.erase(
				std::unique(clip_vertices.begin(), clip_vertices.end()),
				clip_vertices.end());
		EXPECT_EQ(clip_vertices, (std::vector<unsigned int>{ 0, 4 }));

		unsigned int num_loops = 0;
		for (const Overlay::Piece *piece : ring_pieces(overlay, Overlay::CLIP, 0))
		{
			if (piece->start_vertex == boost::optional<unsigned int>(v) &&
				piece->end_vertex == boost::optional<unsigned int>(v) &&
				piece->single_arc_segment)
			{
				++num_loops;
				EXPECT_EQ(*piece->single_arc_segment, 4u);  // the short last edge, (d, 0) to (0, 0)
			}
		}
		EXPECT_EQ(num_loops, 1u);
	}
	EXPECT_TRUE(found);
}


// Neighbouring plates sharing an edge exactly (catalogue 4.5): both shared vertices are SS records,
// and each polygon has a single arc between the two vertices (the future common pair).
TEST(PolygonOverlayTest, StructureSharedEdge)
{
	const auto P = polygon(square(-0.02, -0.01, 0.0, 0.01));
	const auto Q = polygon(square(0.0, -0.01, 0.02, 0.01));
	const Overlay::Overlay overlay(*P, *Q);
	check_structure(overlay);

	EXPECT_EQ(record_kinds(overlay), "SS SS");
	ASSERT_EQ(overlay.get_vertices().size(), 2u);
	for (const Overlay::Vertex &vertex : overlay.get_vertices())
	{
		EXPECT_TRUE(vertex.subject_vertex);
		EXPECT_TRUE(vertex.clip_vertex);
		EXPECT_EQ(vertex.ends.size(), 4u);
	}

	for (Overlay::Side side : { Overlay::SUBJECT, Overlay::CLIP })
	{
		unsigned int num_single_arcs = 0;
		for (const Overlay::Piece *piece : ring_pieces(overlay, side, 0))
		{
			EXPECT_NE(piece->start_vertex, piece->end_vertex);
			if (piece->single_arc_segment)
			{
				++num_single_arcs;
			}
		}
		EXPECT_EQ(num_single_arcs, 1u);
	}
}


// A polyline across the clip: dangling ends bound its first and last pieces; and a polyline ending
// on the clip's edge has its last vertex as a record (the fictitious one-past-the-last segment).
TEST(PolygonOverlayTest, StructurePolyline)
{
	const auto Q = polygon(square(-0.01, -0.01, 0.01, 0.01));
	{
		const auto line = PolylineOnSphere::create(
				ring_type{ F.at(-0.02, 0.0), F.at(0.0, 0.002), F.at(0.02, 0.0) });
		const Overlay::Overlay overlay(*line, *Q);
		check_structure(overlay);

		EXPECT_EQ(record_kinds(overlay), "LL LL");
		const auto pieces = ring_pieces(overlay, Overlay::SUBJECT, 0);
		ASSERT_EQ(pieces.size(), 3u);
		EXPECT_FALSE(pieces[0]->start_vertex);
		EXPECT_TRUE(pieces[0]->end_vertex);
		EXPECT_TRUE(pieces[1]->start_vertex);
		EXPECT_TRUE(pieces[1]->end_vertex);
		EXPECT_TRUE(pieces[1]->has_interior_vertex);
		EXPECT_TRUE(pieces[2]->start_vertex);
		EXPECT_FALSE(pieces[2]->end_vertex);
		EXPECT_EQ(overlay.get_stops()[pieces[0]->start_stop].kind, Overlay::Stop::DANGLING_END);
		EXPECT_EQ(overlay.get_stops()[pieces[2]->end_stop].kind, Overlay::Stop::DANGLING_END);
	}
	{
		const auto line = PolylineOnSphere::create(
				ring_type{ F.at(-0.02, 0.0), F.at(-0.005, 0.002), F.at(0.01, 0.0) });
		const Overlay::Overlay overlay(*line, *Q);
		check_structure(overlay);

		EXPECT_EQ(record_kinds(overlay), "LL SL");
		const auto pieces = ring_pieces(overlay, Overlay::SUBJECT, 0);
		ASSERT_EQ(pieces.size(), 2u);
		ASSERT_TRUE(pieces[1]->end_vertex);
		const Overlay::Vertex &last = overlay.get_vertices()[*pieces[1]->end_vertex];
		ASSERT_TRUE(last.subject_vertex);
		EXPECT_EQ(last.subject_vertex->index, 2u);
		EXPECT_EQ(last.position, F.at(0.01, 0.0));
	}
	{
		// No record: the whole polyline is one piece.
		const auto line = PolylineOnSphere::create(
				ring_type{ F.at(-0.005, 0.0), F.at(0.0, 0.002), F.at(0.005, 0.0) });
		const Overlay::Overlay overlay(*line, *Q);
		check_structure(overlay);

		const auto pieces = ring_pieces(overlay, Overlay::SUBJECT, 0);
		ASSERT_EQ(pieces.size(), 1u);
		EXPECT_TRUE(pieces[0]->closed);
		EXPECT_TRUE(pieces[0]->has_interior_vertex);
	}
}
