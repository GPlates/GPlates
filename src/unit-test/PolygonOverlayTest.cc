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
#include "maths/MathsUtils.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolygonOverlay.h"
#include "maths/PolylineOnSphere.h"
#include "maths/UnitVector3D.h"
#include "maths/Vector3D.h"

#include "unit-test/PolygonOverlayOracle.h"


//
// Tests of PolygonOverlay, the overlay of a subject (polygon or polyline) with a clip polygon
// under polygon partitioning and the Boolean operations.
//
// The cases are families of the Phase 4a catalogue (of the overlay's prototype, kept outside this
// repository; PolygonOverlayPrototypeTest.cc compares with all of it), built as there in local
// tangent coordinates with distances in units of the thickness, H, but at a centre on no axis (as
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


	/**
	 * Whether two points are the same, coordinate for coordinate. (PointOnSphere's 'operator=='
	 * has a tolerance of about the thickness, which is the scale these tests work at.)
	 */
	bool
	same(
			const PointOnSphere &a,
			const PointOnSphere &b)
	{
		return a.position_vector().x().dval() == b.position_vector().x().dval() &&
				a.position_vector().y().dval() == b.position_vector().y().dval() &&
				a.position_vector().z().dval() == b.position_vector().z().dval();
	}


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


	//! The rays of a vertex as a string of their kinds in the measured order ("PQ* P Q").
	std::string
	ray_kinds(
			const Overlay::Vertex &vertex)
	{
		std::string result;
		for (const Overlay::Ray &ray : vertex.rays)
		{
			result += result.empty() ? "" : " ";
			result += ray.is_common() ? "PQ*" : (ray.subject_end ? "P" : "Q");
		}
		return result;
	}

	//! Whether two strings are the same cyclic sequence of space-separated words.
	bool
	same_cycle(
			const std::string &a,
			const std::string &b)
	{
		const std::string doubled = b + " " + b;
		return a.size() == b.size() &&
				(" " + doubled + " ").find(" " + a + " ") != std::string::npos;
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
	 * The classes of one ring's non-internal pieces, sorted, as the prototype's report writes them:
	 * "in", "out", "same" or "opposite" ("?" if unclassified).
	 */
	std::string
	ring_classes(
			const Overlay::Overlay &overlay,
			Overlay::Side side,
			unsigned int ring_index)
	{
		std::vector<std::string> classes;
		for (const Overlay::Piece *piece : ring_pieces(overlay, side, ring_index))
		{
			if (piece->internal)
			{
				continue;
			}
			if (piece->common)
			{
				classes.push_back(
						piece->common_class == Overlay::COMMON_SAME_SIDE ? "same" :
						(piece->common_class == Overlay::COMMON_OPPOSITE_SIDES ? "opposite" : "?"));
			}
			else
			{
				classes.push_back(
						piece->piece_class == Overlay::CLASS_INSIDE ? "in" :
						(piece->piece_class == Overlay::CLASS_OUTSIDE ? "out" : "?"));
			}
		}
		std::sort(classes.begin(), classes.end());

		std::string result;
		for (const std::string &piece_class : classes)
		{
			result += (result.empty() ? "" : " ") + piece_class;
		}
		return result;
	}


	/**
	 * Classify, and check what every classified overlay must satisfy: no check failure and no
	 * unresolved piece.
	 */
	Overlay::Result
	classify_and_check(
			Overlay::Overlay &overlay)
	{
		const Overlay::Result result = overlay.classify();
		EXPECT_EQ(overlay.get_diagnostics().check_failures, 0u);
		EXPECT_EQ(overlay.get_diagnostics().repairs, 0u);
		EXPECT_EQ(overlay.get_diagnostics().unresolved_pieces, 0u);
		EXPECT_EQ(overlay.get_diagnostics().piece_side_disagreements, 0u);
		return result;
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

		// Internal pieces have no ends at any vertex; a common pair is mutual, between one subject
		// and one clip piece with the same two end vertices.
		for (unsigned int p = 0; p < overlay.get_pieces().size(); ++p)
		{
			const Overlay::Piece &piece = overlay.get_pieces()[p];
			if (piece.common)
			{
				const Overlay::Piece &partner = overlay.get_pieces()[*piece.common];
				EXPECT_EQ(partner.common, boost::optional<unsigned int>(p));
				EXPECT_NE(
						overlay.get_rings()[piece.ring].side,
						overlay.get_rings()[partner.ring].side);
				EXPECT_TRUE(
						(piece.start_vertex == partner.start_vertex &&
								piece.end_vertex == partner.end_vertex) ||
						(piece.start_vertex == partner.end_vertex &&
								piece.end_vertex == partner.start_vertex));
				EXPECT_FALSE(piece.internal);
			}
		}
		for (const Overlay::Vertex &vertex : vertices)
		{
			unsigned int num_ray_ends = 0;
			for (const Overlay::End &end : vertex.ends)
			{
				EXPECT_FALSE(overlay.get_pieces()[end.piece].internal);
			}
			for (const Overlay::Ray &ray : vertex.rays)
			{
				EXPECT_EQ(ray.flips_subject, bool(ray.subject_end));
				EXPECT_EQ(ray.flips_clip, bool(ray.clip_end));
				num_ray_ends += ray.is_common() ? 2 : 1;
				EXPECT_GE(ray.cone, 0.0);
			}
			EXPECT_EQ(num_ray_ends, vertex.ends.size());
		}

		// A piece's first and last points are the positions of its end stops.
		for (const Overlay::Piece &piece : overlay.get_pieces())
		{
			const std::vector<PointOnSphere> points = overlay.get_piece_points(piece);
			ASSERT_GE(points.size(), piece.closed ? 2u : 2u);
			if (!piece.closed)
			{
				EXPECT_TRUE(same(points.front(), stops[piece.start_stop].position));
				EXPECT_TRUE(same(points.back(), stops[piece.end_stop].position));
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
	EXPECT_TRUE(same(vertex.position, F.at(0.0, -r)));  // the subject's vertex, exactly
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
			EXPECT_TRUE(same(points[1], F.at(0.0, 0.0)));
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
		EXPECT_TRUE(same(vertex.position, F.at(d / 2, 0.3 * H)));

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
		EXPECT_TRUE(same(last.position, F.at(0.01, 0.0)));
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



// The loop rule (catalogue 4.6): the clip's short edge between its two vertices at one subject
// vertex is a single arc from that vertex to itself, so it is internal and leaves no ray; four ends
// remain.
TEST(PolygonOverlayTest, LoopRuleTwoClipVerticesAtOneSubjectVertex)
{
	const double d = 0.8 * H;
	const auto Q = polygon(ring_type{
			F.at(0.0, 0.0), F.at(0.0, -0.02), F.at(0.02, -0.02), F.at(0.02, 0.0), F.at(d, 0.0) });
	const auto P = polygon(ring_type{
			F.at(-0.01, 0.003), F.at(d / 2, 0.3 * H), F.at(0.01, -0.003),
			F.at(0.01, -0.03), F.at(-0.01, -0.03) });
	const Overlay::Overlay overlay(*P, *Q);
	check_structure(overlay);

	EXPECT_EQ(overlay.get_diagnostics().internal_pieces, 1u);
	EXPECT_EQ(overlay.get_diagnostics().spikes, 0u);
	for (const Overlay::Piece &piece : overlay.get_pieces())
	{
		const bool is_loop = piece.start_vertex && piece.start_vertex == piece.end_vertex &&
				!piece.has_interior_vertex;
		EXPECT_EQ(piece.internal, is_loop);
	}
	for (const Overlay::Vertex &vertex : overlay.get_vertices())
	{
		EXPECT_EQ(vertex.ends.size(), 4u);
		EXPECT_TRUE(same_cycle(ray_kinds(vertex), "P Q P Q")) << ray_kinds(vertex);
	}
}


// The loop rule leaves the wedge's tip piece (x, apex, x) alone: it has an original vertex. The six
// rays all start at x, the vertex's output position, so every cone is zero; anticlockwise they are
// east, the two tip rays, west, and the two rays of the rest of the clip (catalogue 4.1).
TEST(PolygonOverlayTest, RaysAcuteWedge)
{
	const double half_angle = 15.0 * 3.14159265358979323846 / 180.0;
	const double L = 0.02;
	const double r = 2.0 * H;
	const auto Q = polygon(ring_type{
			F.at(0.0, 0.0),
			F.at(-L * std::sin(half_angle), -L * std::cos(half_angle)),
			F.at(0.0, -1.5 * L),
			F.at(L * std::sin(half_angle), -L * std::cos(half_angle)) });
	const auto P = polygon(ring_type{
			F.at(-0.04, -r), F.at(0.0, -r), F.at(0.04, -r),
			F.at(0.04, -0.05), F.at(-0.04, -0.05) });
	const Overlay::Overlay overlay(*P, *Q);
	check_structure(overlay);

	EXPECT_EQ(overlay.get_diagnostics().internal_pieces, 0u);
	ASSERT_EQ(overlay.get_vertices().size(), 1u);
	const Overlay::Vertex &vertex = overlay.get_vertices()[0];
	EXPECT_TRUE(same_cycle(ray_kinds(vertex), "P Q Q P Q Q")) << ray_kinds(vertex);
	for (const Overlay::Ray &ray : vertex.rays)
	{
		ASSERT_TRUE(ray.direction);
		EXPECT_EQ(ray.delta, 0.0);
		EXPECT_EQ(ray.cone, 0.0);
	}

	// The two clip rays between the subject's rays on the north side are the tip piece's.
	const std::vector<Overlay::Ray> &rays = vertex.rays;
	for (unsigned int i = 0; i < rays.size(); ++i)
	{
		const Overlay::Ray &a = rays[i];
		const Overlay::Ray &b = rays[(i + 1) % rays.size()];
		if (a.clip_end && b.clip_end)
		{
			// Adjacent clip rays belong to one piece (its two ends).
			EXPECT_EQ(a.clip_end->piece, b.clip_end->piece);
		}
	}
}


// Neighbouring plates sharing an edge (catalogue 4.5), exactly and with the clip's edge offset by
// half the thickness: the shared edge is a common pair, and each of its vertices has three rays,
// the common one first-class. With the offset, the clip's arcs start at its own vertices, off the
// subject's, so the clip-only ray and the common ray have small cones; the subject-only ray none.
TEST(PolygonOverlayTest, CommonPairSharedEdge)
{
	for (double d : { 0.0, 0.5 * H })
	{
		SCOPED_TRACE(d / H);
		const auto P = polygon(square(-0.02, -0.01, 0.0, 0.01));
		const auto Q = polygon(square(d, -0.01, 0.02, 0.01));
		const Overlay::Overlay overlay(*P, *Q);
		check_structure(overlay);

		EXPECT_EQ(record_kinds(overlay), "SS SS");
		EXPECT_EQ(overlay.get_diagnostics().common_pairs, 1u);
		EXPECT_EQ(overlay.get_diagnostics().common_ambiguous, 0u);
		EXPECT_EQ(overlay.get_diagnostics().max_ends, 4u);
		for (const Overlay::Vertex &vertex : overlay.get_vertices())
		{
			EXPECT_TRUE(same_cycle(ray_kinds(vertex), "PQ* P Q") ||
					same_cycle(ray_kinds(vertex), "PQ* Q P")) << ray_kinds(vertex);
			for (const Overlay::Ray &ray : vertex.rays)
			{
				ASSERT_TRUE(ray.direction);
				if (d == 0.0 || (ray.subject_end && !ray.clip_end))
				{
					EXPECT_EQ(ray.cone, 0.0);
				}
				else
				{
					EXPECT_NEAR(ray.delta, d, 1e-3 * H);
					EXPECT_GT(ray.cone, 0.0);
					EXPECT_LT(ray.cone, 1e-3);
				}
			}
		}
	}
}


// A subject segment shorter than twice the thickness, crossing the clip's edge at right angles
// (catalogue 4.4, D4): both its ends touch the clip's edge, as two vertices, and the segment and
// the clip's stretch between the touches are a common pair, as the prototype finds.
TEST(PolygonOverlayTest, CommonPairShortSegmentAcrossEdge)
{
	const double half = 0.75 * H;
	const auto Q = polygon(square(-0.03, -0.03, 0.03, 0.0));
	const auto P = polygon(ring_type{
			F.at(0.0, half), F.at(0.0, -half), F.at(0.01, -0.01), F.at(0.02, 0.01) });
	const Overlay::Overlay overlay(*P, *Q);
	check_structure(overlay);

	EXPECT_EQ(record_kinds(overlay), "LL SL SL");
	EXPECT_EQ(overlay.get_diagnostics().common_pairs, 1u);
	unsigned int num_touch_vertices = 0;
	for (const Overlay::Vertex &vertex : overlay.get_vertices())
	{
		if (vertex.subject_vertex)
		{
			++num_touch_vertices;
			EXPECT_TRUE(same_cycle(ray_kinds(vertex), "PQ* P Q") ||
					same_cycle(ray_kinds(vertex), "PQ* Q P")) << ray_kinds(vertex);
		}
		else
		{
			EXPECT_TRUE(same_cycle(ray_kinds(vertex), "P Q P Q")) << ray_kinds(vertex);
		}
	}
	EXPECT_EQ(num_touch_vertices, 2u);
}


// The spike rule: the clip runs from the subject's corner a along its top edge to corner b and
// straight back to a (its vertices within the thickness of a, b, a), so the two single arcs of the
// hairpin are internal.
TEST(PolygonOverlayTest, SpikeRule)
{
	const auto P = polygon(square(-0.01, -0.01, 0.01, 0.01));
	const auto Q = polygon(ring_type{
			F.at(-0.01, 0.01 + 0.3 * H),
			F.at(0.01, 0.01 + 0.3 * H),
			F.at(-0.01, 0.01 - 0.3 * H),
			F.at(-0.02, 0.0),
			F.at(-0.02, 0.03) });
	const Overlay::Overlay overlay(*P, *Q);
	check_structure(overlay);

	EXPECT_EQ(overlay.get_diagnostics().spikes, 1u);
	EXPECT_EQ(overlay.get_diagnostics().internal_pieces, 2u);
	unsigned int num_internal_clip_pieces = 0;
	for (const Overlay::Piece *piece : ring_pieces(overlay, Overlay::CLIP, 0))
	{
		if (piece->internal)
		{
			++num_internal_clip_pieces;
			EXPECT_TRUE(piece->single_arc_segment);
		}
	}
	EXPECT_EQ(num_internal_clip_pieces, 2u);
}



//
// Classification (phase4b-design.md section 3.7). The expected classes are the prototype's for the
// same catalogue cases.
//

// Two squares overlapping at a corner: each ring has one piece inside the other square and one
// outside, all by vertex tests.
TEST(PolygonOverlayTest, ClassifyTwoSquares)
{
	const auto P = polygon(square(-0.02, -0.02, 0.005, 0.005));
	const auto Q = polygon(square(-0.005, -0.005, 0.02, 0.02));
	Overlay::Overlay overlay(*P, *Q);
	EXPECT_EQ(classify_and_check(overlay), Overlay::RESULT_INTERSECTING);

	EXPECT_EQ(ring_classes(overlay, Overlay::SUBJECT, 0), "in out");
	EXPECT_EQ(ring_classes(overlay, Overlay::CLIP, 0), "in out");
	EXPECT_EQ(overlay.get_diagnostics().vertex_tests, 4u);
	EXPECT_EQ(overlay.get_diagnostics().deduced, 0u);
	EXPECT_EQ(overlay.get_diagnostics().midpoint_tests, 0u);
}


// No record (doc 5.1's first bug): the clip wholly inside the subject is a ring inside it, so the
// subject is cut (a hole in the difference), not passed whole to the outside.
TEST(PolygonOverlayTest, ClassifyClipInsideSubject)
{
	const auto P = polygon(square(-0.02, -0.02, 0.02, 0.02));
	const auto Q = polygon(square(-0.005, -0.005, 0.005, 0.005));
	Overlay::Overlay overlay(*P, *Q);
	EXPECT_EQ(classify_and_check(overlay), Overlay::RESULT_INTERSECTING);

	EXPECT_EQ(ring_classes(overlay, Overlay::SUBJECT, 0), "out");
	EXPECT_EQ(ring_classes(overlay, Overlay::CLIP, 0), "in");
}


// The subject wholly inside the clip, and wholly outside it: the whole subject is one class.
TEST(PolygonOverlayTest, ClassifyWhole)
{
	const auto inner = polygon(square(-0.005, -0.005, 0.005, 0.005));
	const auto outer = polygon(square(-0.02, -0.02, 0.02, 0.02));
	const auto far = polygon(square(0.03, 0.03, 0.04, 0.04));
	{
		Overlay::Overlay overlay(*inner, *outer);
		EXPECT_EQ(classify_and_check(overlay), Overlay::RESULT_INSIDE);
	}
	{
		Overlay::Overlay overlay(*far, *outer);
		EXPECT_EQ(classify_and_check(overlay), Overlay::RESULT_OUTSIDE);
	}
}


// The clip crosses only the subject's hole (doc 5.1's second bug): the hole's piece inside the
// clip is deduced across a crossing (the LL rule), the exterior is outside.
TEST(PolygonOverlayTest, ClassifyHoleCrossedOnly)
{
	const auto P = polygon(
			square(-0.02, -0.02, 0.02, 0.02),
			{ square(-0.005, -0.005, 0.005, 0.005) });
	const auto Q = polygon(square(-0.002, -0.002, 0.002, 0.01));
	Overlay::Overlay overlay(*P, *Q);
	EXPECT_EQ(classify_and_check(overlay), Overlay::RESULT_INTERSECTING);

	EXPECT_EQ(ring_classes(overlay, Overlay::SUBJECT, 0), "out");
	EXPECT_EQ(ring_classes(overlay, Overlay::SUBJECT, 1), "in out");
	EXPECT_EQ(ring_classes(overlay, Overlay::CLIP, 0), "in out");
	EXPECT_EQ(overlay.get_diagnostics().deduced, 1u);
}


// The clip's apex on the subject's edge (walk-through 6.4's case): one touch, four ends, the free
// rule; the clip is inside, the subject outside it.
TEST(PolygonOverlayTest, ClassifyApexOnEdge)
{
	const auto P = polygon(square(-0.02, -0.02, 0.02, 0.02));
	const auto Q = polygon(ring_type{ F.at(0.0, 0.02), F.at(0.01, 0.0), F.at(-0.01, 0.0) });
	Overlay::Overlay overlay(*P, *Q);
	EXPECT_EQ(classify_and_check(overlay), Overlay::RESULT_INTERSECTING);

	EXPECT_EQ(record_kinds(overlay), "LS");
	EXPECT_EQ(ring_classes(overlay, Overlay::SUBJECT, 0), "out");
	EXPECT_EQ(ring_classes(overlay, Overlay::CLIP, 0), "in");
}


// The acute wedge: six ends at one vertex, the sector walk on the measured order; the clip's tip
// (x, apex, x) lies outside the subject.
TEST(PolygonOverlayTest, ClassifyAcuteWedge)
{
	const double half_angle = 15.0 * 3.14159265358979323846 / 180.0;
	const double L = 0.02;
	const double r = 2.0 * H;
	const auto Q = polygon(ring_type{
			F.at(0.0, 0.0),
			F.at(-L * std::sin(half_angle), -L * std::cos(half_angle)),
			F.at(0.0, -1.5 * L),
			F.at(L * std::sin(half_angle), -L * std::cos(half_angle)) });
	const auto P = polygon(ring_type{
			F.at(-0.04, -r), F.at(0.0, -r), F.at(0.04, -r),
			F.at(0.04, -0.05), F.at(-0.04, -0.05) });
	Overlay::Overlay overlay(*P, *Q);
	EXPECT_EQ(classify_and_check(overlay), Overlay::RESULT_INTERSECTING);

	EXPECT_EQ(ring_classes(overlay, Overlay::SUBJECT, 0), "out");
	EXPECT_EQ(ring_classes(overlay, Overlay::CLIP, 0), "in out");
	for (const Overlay::Piece *piece : ring_pieces(overlay, Overlay::CLIP, 0))
	{
		if (overlay.get_piece_points(*piece).size() == 3)
		{
			EXPECT_EQ(piece->piece_class, Overlay::CLASS_OUTSIDE);  // the tip
		}
	}
}


// Neighbouring plates sharing an edge: the common pair is opposite sides (by the sector walk), and
// the rest of each is outside the other, so the subject is wholly outside.
TEST(PolygonOverlayTest, ClassifySharedEdge)
{
	for (double d : { 0.0, 0.5 * H, 2.0 * H })
	{
		SCOPED_TRACE(d / H);
		const auto P = polygon(square(-0.02, -0.01, 0.0, 0.01));
		const auto Q = polygon(square(d, -0.01, 0.02, 0.01));
		Overlay::Overlay overlay(*P, *Q);
		EXPECT_EQ(classify_and_check(overlay), Overlay::RESULT_OUTSIDE);
		if (d <= H)
		{
			EXPECT_EQ(ring_classes(overlay, Overlay::SUBJECT, 0), "opposite out");
			EXPECT_EQ(ring_classes(overlay, Overlay::CLIP, 0), "opposite out");
		}
	}

	// The clip inside the subject, sharing its east edge: the common pair is same side, and the
	// subject is cut.
	for (double d : { 0.0, 0.5 * H })
	{
		SCOPED_TRACE(d / H);
		const auto P = polygon(square(-0.02, -0.02, 0.02, 0.02));
		const auto Q = polygon(square(0.005, -0.005, 0.02 - d, 0.005));
		Overlay::Overlay overlay(*P, *Q);
		EXPECT_EQ(classify_and_check(overlay), Overlay::RESULT_INTERSECTING);
		EXPECT_EQ(overlay.get_diagnostics().common_pairs, 1u);
		EXPECT_EQ(ring_classes(overlay, Overlay::CLIP, 0), "in same");
	}
}


// A narrow neck of the clip (catalogue 4.5): a subject apex x in a neck narrower than the
// thickness touches both its sides; the clip's piece round the far side is outside the subject.
TEST(PolygonOverlayTest, ClassifyNeck)
{
	for (double w : { 0.5 * H, 1.0 * H, 1.5 * H, 1.9 * H, 2.5 * H, 4.0 * H })
	{
		for (double apex : { 0.0, 0.2 * w })
		{
			SCOPED_TRACE(w / H);
			SCOPED_TRACE(apex / H);
			const auto Q = polygon(ring_type{
					F.at(-0.02, 0.02), F.at(-w / 2, 0.005),
					F.at(-w / 2, -0.005), F.at(-0.02, -0.02),
					F.at(0.02, -0.02), F.at(w / 2, -0.005), F.at(w / 2, 0.005), F.at(0.02, 0.02) });
			const auto P = polygon(
					ring_type{ F.at(apex, 0.0), F.at(0.03, 0.01), F.at(-0.03, 0.01) });
			Overlay::Overlay overlay(*P, *Q);
			EXPECT_EQ(classify_and_check(overlay), Overlay::RESULT_INTERSECTING);
		}
	}
}


// The T-intersection (walk-through section 3): the subject's edge A-B passes just above the
// clip's corner and touches its top edge at B. A test at the longest arc's midpoint could fall
// within rounding of the clip; the vertex tests decide both rings, outside each other.
TEST(PolygonOverlayTest, ClassifyTIntersection)
{
	const auto Q = polygon(ring_type{
			F.at(0.0, 0.0), F.at(0.03, 0.0), F.at(0.03, -0.03), F.at(0.0, -0.03) });
	const double a = -0.003;
	const double b = 0.01;
	for (double yC : { 1.1 * H, 2.0 * H, 10.0 * H })
	{
		for (double delta : { -0.9 * H, 0.0, 0.5 * H, 0.9 * H })
		{
			SCOPED_TRACE(yC / H);
			SCOPED_TRACE(delta / H);
			const double yA = (yC * (b - a) + delta * (-a)) / b;
			const auto P = polygon(ring_type{ F.at(a, yA), F.at(b, -delta), F.at(b, 0.02) });
			Overlay::Overlay overlay(*P, *Q);
			EXPECT_EQ(classify_and_check(overlay), Overlay::RESULT_OUTSIDE);
			EXPECT_EQ(ring_classes(overlay, Overlay::SUBJECT, 0), "out");
			EXPECT_EQ(ring_classes(overlay, Overlay::CLIP, 0), "out");
		}
	}
}



// The touch rule (four ends, no crossing record): the clip dips from inside onto the subject's top
// edge at two vertices, T1 and T2. The subject's single arc between them has no vertex to test; at
// each touch the clip's two pieces are both inside the subject, so the clip does not cross there,
// so neither does the subject, and the arc takes the class of the subject's other piece (outside).
TEST(PolygonOverlayTest, ClassifyTouchRuleDeduction)
{
	const auto P = polygon(square(-0.02, -0.02, 0.02, 0.02));
	const auto Q = polygon(ring_type{
			F.at(-0.005, 0.02), F.at(0.0, 0.01), F.at(0.005, 0.02),
			F.at(0.01, -0.01), F.at(-0.01, -0.01) });
	Overlay::Overlay overlay(*P, *Q);
	check_structure(overlay);
	EXPECT_EQ(classify_and_check(overlay), Overlay::RESULT_INTERSECTING);

	EXPECT_EQ(record_kinds(overlay), "LS LS");
	EXPECT_EQ(ring_classes(overlay, Overlay::SUBJECT, 0), "out out");
	EXPECT_EQ(ring_classes(overlay, Overlay::CLIP, 0), "in in");
	EXPECT_EQ(overlay.get_diagnostics().deduced, 1u);
	EXPECT_EQ(overlay.get_diagnostics().midpoint_tests, 0u);
	for (const Overlay::Piece *piece : ring_pieces(overlay, Overlay::SUBJECT, 0))
	{
		if (!piece->has_interior_vertex)
		{
			EXPECT_EQ(piece->method, Overlay::METHOD_DEDUCED);
		}
	}
}


// A touch from inside or outside, or a shared edge, leaves the subject whole: inside or outside.
TEST(PolygonOverlayTest, ClassifyResultOfTouches)
{
	const auto Q = polygon(square(-0.02, -0.02, 0.02, 0.0));
	{
		// The subject's apex on the clip's top edge, from inside.
		const auto P = polygon(
				ring_type{ F.at(0.0, 0.0), F.at(0.005, -0.01), F.at(-0.005, -0.01) });
		Overlay::Overlay overlay(*P, *Q);
		EXPECT_EQ(classify_and_check(overlay), Overlay::RESULT_INSIDE);
		EXPECT_EQ(record_kinds(overlay), "SL");
	}
	{
		// From outside.
		const auto P = polygon(ring_type{ F.at(0.0, 0.0), F.at(0.005, 0.01), F.at(-0.005, 0.01) });
		Overlay::Overlay overlay(*P, *Q);
		EXPECT_EQ(classify_and_check(overlay), Overlay::RESULT_OUTSIDE);
		EXPECT_EQ(record_kinds(overlay), "SL");
	}
	{
		// The subject's top edge along part of the clip's, the subject inside the clip.
		const auto P = polygon(square(-0.01, -0.01, 0.01, 0.0));
		Overlay::Overlay overlay(*P, *Q);
		EXPECT_EQ(classify_and_check(overlay), Overlay::RESULT_INSIDE);
	}
}


// A subject segment shorter than GreatCircleArc's zero length (0.5h) between two touches of the
// clip's edge, with the apex of a subject hole touching that edge between them (catalogue 4.4, d4
// hole-between): a self-touch of the subject on the stretch it shares with the clip, so the segment
// is cut at the apex (phase4b-design.md 3.5), its own plane standing in for the axis GreatCircleArc
// has not for it, and both parts pair as common. (Before the cut its rays had no direction, the
// only case that had such rays; none now does.)
TEST(PolygonOverlayTest, ShortSegmentSelfTouch)
{
	const double half = 0.25 * H * std::sqrt(0.5);
	const auto Q = polygon(square(-0.03, -0.03, 0.03, 0.0));
	const auto P = polygon(
			ring_type{ F.at(-half, half), F.at(half, -half), F.at(0.01, -0.01), F.at(0.02, 0.01) },
			{ ring_type{ F.at(0.3 * half, 0.0), F.at(0.003, -0.001), F.at(0.004, 0.001) } });
	Overlay::Overlay overlay(*P, *Q);
	check_structure(overlay);
	classify_and_check(overlay);
	EXPECT_EQ(overlay.get_diagnostics().self_touch_cuts, 1u);
	EXPECT_EQ(overlay.get_diagnostics().common_pairs, 2u);
}



// Anchorless chains (phase4b-design.md section 3.9, option 3): rings that run together all the way
// round meet no non-common piece, so the sector walk has no anchor. Identical polygons are one
// same-side chain, the subject wholly inside; a subject equal to a hole of the clip is an
// opposite-sides chain, the subject wholly outside; and the same with the clip's vertices moved
// within the thickness.
TEST(PolygonOverlayTest, ClassifyAnchorless)
{
	// The clip's jittered copy moves each vertex by less than the thickness.
	const double corners[4][2] = {
			{ -0.01, -0.008 }, { 0.012, -0.01 }, { 0.009, 0.011 }, { -0.011, 0.007 } };
	const double jitter[4][2] = { { 0.4, -0.3 }, { -0.5, 0.2 }, { 0.3, 0.6 }, { -0.2, -0.4 } };
	ring_type ring, jittered;
	for (unsigned int i = 0; i < 4; ++i)
	{
		ring.push_back(F.at(corners[i][0], corners[i][1]));
		jittered.push_back(F.at(
				corners[i][0] + jitter[i][0] * H,
				corners[i][1] + jitter[i][1] * H));
	}
	const ring_type reversed(ring.rbegin(), ring.rend());

	for (const ring_type *clip_ring : std::vector<const ring_type *>{ &ring, &reversed, &jittered })
	{
		SCOPED_TRACE(clip_ring == &ring ? "identical" :
				(clip_ring == &reversed ? "reversed" : "jittered"));
		{
			const auto P = polygon(ring);
			const auto Q = polygon(*clip_ring);
			Overlay::Overlay overlay(*P, *Q);
			EXPECT_EQ(classify_and_check(overlay), Overlay::RESULT_INSIDE);
			EXPECT_EQ(overlay.get_diagnostics().anchorless_resolved, 1u);
			EXPECT_EQ(ring_classes(overlay, Overlay::SUBJECT, 0), "same same same same");
		}
		{
			const auto P = polygon(ring);
			const auto Q = polygon(square(-0.03, -0.03, 0.03, 0.03), { *clip_ring });
			Overlay::Overlay overlay(*P, *Q);
			EXPECT_EQ(classify_and_check(overlay), Overlay::RESULT_OUTSIDE);
			EXPECT_EQ(overlay.get_diagnostics().anchorless_resolved, 1u);
			EXPECT_EQ(
					ring_classes(overlay, Overlay::SUBJECT, 0),
					"opposite opposite opposite opposite");
		}
	}
}



//
// Repair and failure (phase4b-design.md sections 3.9 and 9). No normal input needs repair, so the
// inconsistencies are provoked: a class set wrong by hand, then the check and repair run again.
//

namespace
{
	//! The pieces of one ring whose class is @a piece_class (by ring_classes' names).
	std::vector<unsigned int>
	pieces_with_class(
			const Overlay::Overlay &overlay,
			Overlay::Side side,
			const std::string &piece_class)
	{
		std::vector<unsigned int> pieces;
		for (unsigned int p = 0; p < overlay.get_pieces().size(); ++p)
		{
			const Overlay::Piece &piece = overlay.get_pieces()[p];
			const std::string name = piece.common
					? (piece.common_class == Overlay::COMMON_SAME_SIDE ? "same" : "opposite")
					: (piece.piece_class == Overlay::CLASS_INSIDE ? "in" : "out");
			if (overlay.get_rings()[piece.ring].side == side &&
				!piece.internal &&
				name == piece_class)
			{
				pieces.push_back(p);
			}
		}
		return pieces;
	}
}


// One class set wrong (as if deduced) is flipped back: the two crossings it ends at fail the LL
// rule, and the least reliable class there is the wrong one.
TEST(PolygonOverlayTest, RepairSingleFlip)
{
	const auto P = polygon(square(-0.02, -0.02, 0.005, 0.005));
	const auto Q = polygon(square(-0.005, -0.005, 0.02, 0.02));
	Overlay::Overlay overlay(*P, *Q);
	ASSERT_EQ(classify_and_check(overlay), Overlay::RESULT_INTERSECTING);

	const std::vector<unsigned int> inside = pieces_with_class(overlay, Overlay::SUBJECT, "in");
	ASSERT_EQ(inside.size(), 1u);
	overlay.set_piece_class(inside.front(), false, Overlay::METHOD_DEDUCED);

	EXPECT_EQ(overlay.check_and_repair(), Overlay::RESULT_INTERSECTING);
	const Overlay::Diagnostics &diagnostics = overlay.get_diagnostics();
	EXPECT_EQ(diagnostics.check_failures_before_repair, 2u);
	EXPECT_EQ(diagnostics.check_failures, 0u);
	EXPECT_EQ(diagnostics.repairs, 1u);
	EXPECT_EQ(diagnostics.trusted_flipped, 0u);
	EXPECT_EQ(overlay.get_pieces()[inside.front()].piece_class, Overlay::CLASS_INSIDE);
	EXPECT_EQ(overlay.get_pieces()[inside.front()].method, Overlay::METHOD_REPAIRED);
}


// A common chain set wrong as a whole is flipped back as a whole: neighbouring plates share an
// edge cut into several common pairs by each other's vertices; set every one 'same', and the
// chain's interior vertices stay consistent while both its ends fail.
TEST(PolygonOverlayTest, RepairWholeChain)
{
	const auto P = polygon(ring_type{
			F.at(-0.02, -0.01), F.at(0.0, -0.01), F.at(0.0, -0.005), F.at(0.0, 0.005),
			F.at(0.0, 0.01), F.at(-0.02, 0.01) });
	const auto Q = polygon(ring_type{
			F.at(0.0, -0.01), F.at(0.02, -0.01), F.at(0.02, 0.01), F.at(0.0, 0.01),
			F.at(0.0, 0.003), F.at(0.0, 0.0), F.at(0.0, -0.003) });
	Overlay::Overlay overlay(*P, *Q);
	ASSERT_EQ(classify_and_check(overlay), Overlay::RESULT_OUTSIDE);

	const std::vector<unsigned int> chain =
			pieces_with_class(overlay, Overlay::SUBJECT, "opposite");
	ASSERT_GT(chain.size(), 2u);
	for (unsigned int p : chain)
	{
		overlay.set_piece_class(p, true/*same side*/, Overlay::METHOD_SECTOR);
	}

	EXPECT_EQ(overlay.check_and_repair(), Overlay::RESULT_OUTSIDE);
	const Overlay::Diagnostics &diagnostics = overlay.get_diagnostics();
	EXPECT_EQ(diagnostics.check_failures_before_repair, 2u);
	EXPECT_EQ(diagnostics.check_failures, 0u);
	EXPECT_EQ(diagnostics.repairs, 1u);
	EXPECT_EQ(diagnostics.whole_chain_repairs, 1u);
	for (unsigned int p : chain)
	{
		EXPECT_EQ(overlay.get_pieces()[p].common_class, Overlay::COMMON_OPPOSITE_SIDES);
		EXPECT_EQ(overlay.get_pieces()[p].method, Overlay::METHOD_REPAIRED);
	}
}


// Without repair (no rounds), the inconsistency is never a silent wrong answer: it throws, with
// the diagnostics and the failing vertex; counted instead when asked not to throw.
TEST(PolygonOverlayTest, InconsistentOverlayThrows)
{
	const auto P = polygon(square(-0.02, -0.02, 0.005, 0.005));
	const auto Q = polygon(square(-0.005, -0.005, 0.02, 0.02));
	Overlay::Options options;
	options.max_repair_rounds = 0;
	{
		Overlay::Overlay overlay(*P, *Q, options);
		overlay.classify();
		const std::vector<unsigned int> inside = pieces_with_class(overlay, Overlay::SUBJECT, "in");
		ASSERT_EQ(inside.size(), 1u);
		overlay.set_piece_class(inside.front(), false, Overlay::METHOD_DEDUCED);
		try
		{
			overlay.check_and_repair();
			ADD_FAILURE() << "no exception";
		}
		catch (const Overlay::InconsistentOverlayException &exception)
		{
			EXPECT_EQ(exception.get_diagnostics().check_failures, 2u);
			EXPECT_EQ(exception.get_diagnostics().repairs, 0u);
			ASSERT_TRUE(exception.get_vertex_position());
		}
	}
	{
		options.throw_if_inconsistent = false;
		Overlay::Overlay overlay(*P, *Q, options);
		overlay.classify();
		const std::vector<unsigned int> inside = pieces_with_class(overlay, Overlay::SUBJECT, "in");
		overlay.set_piece_class(inside.front(), false, Overlay::METHOD_DEDUCED);
		EXPECT_NO_THROW(overlay.check_and_repair());
		EXPECT_EQ(overlay.get_diagnostics().check_failures, 2u);
	}
}



//
// Linking, split and clean-up (phase4b-design.md sections 3.10 and 3.11): the rings of each
// operation, checked by the output oracle (section 11.3).
//

namespace
{
	const Overlay::Operation ALL_OPERATIONS[] = {
			Overlay::INTERSECTION, Overlay::DIFFERENCE, Overlay::REVERSE_DIFFERENCE,
			Overlay::UNION, Overlay::SYMMETRIC_DIFFERENCE };

	//! The number of points of each output ring of @a operation, sorted.
	std::vector<unsigned int>
	ring_sizes(
			const Overlay::Overlay &overlay,
			Overlay::Operation operation)
	{
		std::vector<Overlay::OutputRing> rings;
		overlay.extract_rings(operation, rings);
		std::vector<unsigned int> sizes;
		for (const Overlay::OutputRing &ring : rings)
		{
			sizes.push_back(ring.points.size());
		}
		std::sort(sizes.begin(), sizes.end());
		return sizes;
	}

	//! Every operation's output passes the oracle, as rings and as polygons, and its polygons'
	//! rings have the output on their left.
	void
	check_oracle(
			const PolygonOnSphere &P,
			const PolygonOnSphere &Q,
			const Overlay::Overlay &overlay)
	{
		const std::vector<PointOnSphere> samples = PolygonOverlayOracle::get_samples(P, Q, overlay);
		EXPECT_GT(samples.size(), 100u);
		for (Overlay::Operation operation : ALL_OPERATIONS)
		{
			SCOPED_TRACE(operation);
			std::vector<Overlay::OutputRing> rings;
			overlay.extract_rings(operation, rings);
			EXPECT_EQ(PolygonOverlayOracle::count_failures(P, Q, operation, rings, samples), 0u);

			// Grouped into polygons, and oriented with the output on the left.
			std::vector<Overlay::OutputPolygon> polygons;
			overlay.extract(operation, polygons);
			EXPECT_EQ(PolygonOverlayOracle::count_failures(P, Q, operation, polygons, samples), 0u);
			EXPECT_EQ(PolygonOverlayOracle::count_misoriented_rings(polygons), 0u);
		}
		if (PolygonOverlayOracle::is_area_sum_applicable(P, Q))
		{
			EXPECT_LT(
					PolygonOverlayOracle::area_sum_error(P, overlay),
					PolygonOverlayOracle::area_sum_tolerance(P, Q));
		}
		const Overlay::Diagnostics &diagnostics = overlay.get_diagnostics();
		EXPECT_EQ(diagnostics.pairing_failures, 0u);
		EXPECT_EQ(diagnostics.keep_mismatches, 0u);
		EXPECT_EQ(diagnostics.coincident_rings, 0u);
		EXPECT_EQ(diagnostics.grouping_failures, 0u);
		EXPECT_EQ(diagnostics.hemisphere_check_failures, 0u);
		EXPECT_EQ(diagnostics.hemisphere_check_skipped, 0u);
	}
}


// Two squares overlapping at a corner, under the five operations: the intersection and the two
// differences are one ring each, the union one ring of eight points, the symmetric difference two.
TEST(PolygonOverlayTest, LinkTwoSquares)
{
	const auto P = polygon(square(-0.02, -0.02, 0.005, 0.005));
	const auto Q = polygon(square(-0.005, -0.005, 0.02, 0.02));
	Overlay::Overlay overlay(*P, *Q);
	classify_and_check(overlay);

	EXPECT_EQ(ring_sizes(overlay, Overlay::INTERSECTION), (std::vector<unsigned int>{ 4 }));
	EXPECT_EQ(ring_sizes(overlay, Overlay::DIFFERENCE), (std::vector<unsigned int>{ 6 }));
	EXPECT_EQ(ring_sizes(overlay, Overlay::REVERSE_DIFFERENCE), (std::vector<unsigned int>{ 6 }));
	EXPECT_EQ(ring_sizes(overlay, Overlay::UNION), (std::vector<unsigned int>{ 8 }));
	EXPECT_EQ(
			ring_sizes(overlay, Overlay::SYMMETRIC_DIFFERENCE),
			(std::vector<unsigned int>{ 6, 6 }));
	check_oracle(*P, *Q, overlay);
}


// The clip wholly inside the subject: no record; the difference is the subject's ring and the
// clip's (its hole, once grouped), the intersection the clip's ring, the union the subject's.
TEST(PolygonOverlayTest, LinkClipInsideSubject)
{
	const auto P = polygon(square(-0.02, -0.02, 0.02, 0.02));
	const auto Q = polygon(square(-0.005, -0.005, 0.005, 0.005));
	Overlay::Overlay overlay(*P, *Q);
	classify_and_check(overlay);

	EXPECT_EQ(ring_sizes(overlay, Overlay::INTERSECTION), (std::vector<unsigned int>{ 4 }));
	EXPECT_EQ(ring_sizes(overlay, Overlay::DIFFERENCE), (std::vector<unsigned int>{ 4, 4 }));
	EXPECT_EQ(ring_sizes(overlay, Overlay::REVERSE_DIFFERENCE), std::vector<unsigned int>());
	EXPECT_EQ(ring_sizes(overlay, Overlay::UNION), (std::vector<unsigned int>{ 4 }));
	check_oracle(*P, *Q, overlay);
}


// Walk-through 6.4: the clip's apex on the subject's edge. The difference is the subject with the
// clip as a hole touching its exterior at the apex: the apex is a bounce of four kept ends, which
// the free rule pairs by input, so no walk passes it twice (the split is pinned by the wedge, in
// LinkCatalogueFamilies).
TEST(PolygonOverlayTest, LinkPinch)
{
	const auto P = polygon(square(-0.02, -0.02, 0.02, 0.02));
	const auto Q = polygon(ring_type{ F.at(0.0, 0.02), F.at(0.01, 0.0), F.at(-0.01, 0.0) });
	Overlay::Overlay overlay(*P, *Q);
	classify_and_check(overlay);

	EXPECT_EQ(ring_sizes(overlay, Overlay::DIFFERENCE), (std::vector<unsigned int>{ 3, 5 }));
	EXPECT_EQ(ring_sizes(overlay, Overlay::INTERSECTION), (std::vector<unsigned int>{ 3 }));
	check_oracle(*P, *Q, overlay);
}


// The catalogue's hard configurations, under every operation: the acute wedge (whose tip x, apex, x
// is a collapsed region, dropped by clean-up), shared edges, the twist on the clip.
TEST(PolygonOverlayTest, LinkCatalogueFamilies)
{
	{
		SCOPED_TRACE("wedge");
		const double half_angle = 15.0 * 3.14159265358979323846 / 180.0;
		const double L = 0.02;
		const double r = 2.0 * H;
		const auto Q = polygon(ring_type{
				F.at(0.0, 0.0),
				F.at(-L * std::sin(half_angle), -L * std::cos(half_angle)),
				F.at(0.0, -1.5 * L),
				F.at(L * std::sin(half_angle), -L * std::cos(half_angle)) });
		const auto P = polygon(ring_type{
				F.at(-0.04, -r), F.at(0.0, -r), F.at(0.04, -r),
				F.at(0.04, -0.05), F.at(-0.04, -0.05) });
		Overlay::Overlay overlay(*P, *Q);
		classify_and_check(overlay);

		// The tip x, apex, x is a collapsed ring: walked, a ring passes x twice and is cut there,
		// and clean-up drops the collapsed part (the only unit test that reaches the split).
		EXPECT_EQ(ring_sizes(overlay, Overlay::INTERSECTION), (std::vector<unsigned int>{ 4 }));
		EXPECT_EQ(ring_sizes(overlay, Overlay::DIFFERENCE), (std::vector<unsigned int>{ 4, 5 }));
		for (Overlay::Operation operation :
				{ Overlay::REVERSE_DIFFERENCE, Overlay::UNION, Overlay::SYMMETRIC_DIFFERENCE })
		{
			ring_sizes(overlay, operation);
		}
		EXPECT_EQ(overlay.get_diagnostics().rings_split, 2u);
		EXPECT_EQ(overlay.get_diagnostics().collapsed_rings_dropped, 3u);
		check_oracle(*P, *Q, overlay);
	}
	for (double d : { 0.0, 0.5 * H, 2.0 * H })
	{
		SCOPED_TRACE("shared edge");
		const auto P = polygon(square(-0.02, -0.01, 0.0, 0.01));
		const auto Q = polygon(square(d, -0.01, 0.02, 0.01));
		Overlay::Overlay overlay(*P, *Q);
		classify_and_check(overlay);
		check_oracle(*P, *Q, overlay);
		EXPECT_EQ(ring_sizes(overlay, Overlay::INTERSECTION), std::vector<unsigned int>());
	}
	for (double d : { 3.0 * H, 0.0 })
	{
		SCOPED_TRACE("twist");
		const auto P = polygon(ring_type{
				F.at(-0.01, 0.01), F.at(0.01, -0.01), F.at(0.01, 0.01), F.at(-0.01, -0.01) });
		const auto Q = polygon(ring_type{
				F.at(-0.03, d), F.at(0.03, d), F.at(0.03, -0.03), F.at(-0.03, -0.03) });
		Overlay::Overlay overlay(*P, *Q);
		classify_and_check(overlay);
		check_oracle(*P, *Q, overlay);
	}
}



//
// Grouping, the output side and orientation (phase4b-design.md sections 4.2 and 4.3): output
// polygons with holes, checked by the oracle read polygon by polygon.
//

namespace
{
	//! The number of holes of each output polygon of @a operation, sorted.
	std::vector<unsigned int>
	polygon_holes(
			const Overlay::Overlay &overlay,
			Overlay::Operation operation)
	{
		std::vector<Overlay::OutputPolygon> polygons;
		overlay.extract(operation, polygons);
		std::vector<unsigned int> holes;
		for (const Overlay::OutputPolygon &polygon : polygons)
		{
			holes.push_back(polygon.interiors.size());
		}
		std::sort(holes.begin(), holes.end());
		return holes;
	}

	ring_type
	reversed(
			ring_type ring)
	{
		std::reverse(ring.begin(), ring.end());
		return ring;
	}

	//! A ring of @a num_points round a small circle of angular @a radius, anticlockwise.
	ring_type
	cap(
			const PointOnSphere &centre,
			double radius,
			unsigned int num_points)
	{
		const UnitVector3D &c = centre.position_vector();
		const UnitVector3D east = cross(UnitVector3D(0, 0, 1), c).get_normalisation();
		const UnitVector3D north = cross(c, east).get_normalisation();
		ring_type ring;
		for (unsigned int i = 0; i < num_points; ++i)
		{
			const double a = 2 * GPlatesMaths::PI * i / num_points;
			const Vector3D along = std::cos(a) * Vector3D(east) + std::sin(a) * Vector3D(north);
			const Vector3D point = std::cos(radius) * Vector3D(c) + std::sin(radius) * along;
			ring.push_back(PointOnSphere(point.get_normalisation()));
		}
		return ring;
	}

	//! Whether two rings are the same cyclic sequence of points in the same direction, within
	//! @a tolerance radians point for point.
	bool
	same_directed_ring(
			const std::vector<PointOnSphere> &a,
			const std::vector<PointOnSphere> &b,
			double tolerance)
	{
		const unsigned int n = a.size();
		if (b.size() != n)
		{
			return false;
		}
		for (unsigned int start = 0; start < n; ++start)
		{
			bool same = true;
			for (unsigned int i = 0; same && i < n; ++i)
			{
				same = PolygonOverlayOracle::angle_between(
						a[i].position_vector(), b[(start + i) % n].position_vector()) <= tolerance;
			}
			if (same)
			{
				return true;
			}
		}
		return false;
	}
}


// Holes in both polygons, each wholly inside the other polygon: the intersection is one polygon
// with both holes; in each difference the other's hole is an island; the union fills both.
TEST(PolygonOverlayTest, GroupHolesInBoth)
{
	const auto P = polygon(
			square(-0.03, -0.02, 0.01, 0.02),
			{ square(-0.008, -0.005, -0.002, 0.005) });
	const auto Q = polygon(
			square(-0.01, -0.025, 0.03, 0.025),
			{ square(0.002, -0.005, 0.008, 0.005) });
	Overlay::Overlay overlay(*P, *Q);
	classify_and_check(overlay);
	check_oracle(*P, *Q, overlay);

	EXPECT_EQ(polygon_holes(overlay, Overlay::INTERSECTION), (std::vector<unsigned int>{ 2 }));
	EXPECT_EQ(polygon_holes(overlay, Overlay::DIFFERENCE), (std::vector<unsigned int>{ 0, 0 }));
	EXPECT_EQ(
			polygon_holes(overlay, Overlay::REVERSE_DIFFERENCE),
			(std::vector<unsigned int>{ 0, 0 }));
	EXPECT_EQ(polygon_holes(overlay, Overlay::UNION), (std::vector<unsigned int>{ 0 }));
	EXPECT_EQ(
			polygon_holes(overlay, Overlay::SYMMETRIC_DIFFERENCE),
			(std::vector<unsigned int>{ 0, 0, 0, 0 }));
}


// Holes in both polygons, each crossing the other's outline: in the intersection they are
// notches, in the union holes.
TEST(PolygonOverlayTest, GroupHolesCrossing)
{
	const auto P = polygon(
			square(-0.03, -0.02, 0.01, 0.02),
			{ square(-0.015, -0.005, -0.005, 0.005) });
	const auto Q = polygon(
			square(-0.01, -0.025, 0.03, 0.025),
			{ square(0.005, -0.008, 0.015, 0.008) });
	Overlay::Overlay overlay(*P, *Q);
	classify_and_check(overlay);
	check_oracle(*P, *Q, overlay);

	EXPECT_EQ(polygon_holes(overlay, Overlay::INTERSECTION), (std::vector<unsigned int>{ 0 }));
	EXPECT_EQ(polygon_holes(overlay, Overlay::UNION), (std::vector<unsigned int>{ 2 }));
}


// An island in a hole: the clip inside the subject's hole is an exterior of its own in the union.
// And the subject's own island in its hole (an interior ring inside an interior ring, which
// point-in-polygon reads by parity as inside): the output side of its pieces needs its depth
// among the subject's rings, not whether it is an interior ring.
TEST(PolygonOverlayTest, GroupIslandsInHoles)
{
	{
		SCOPED_TRACE("clip in the subject's hole");
		const auto P = polygon(
				square(-0.03, -0.03, 0.03, 0.03),
				{ square(-0.02, -0.02, 0.02, 0.02) });
		const auto Q = polygon(square(-0.01, -0.01, 0.01, 0.01));
		Overlay::Overlay overlay(*P, *Q);
		classify_and_check(overlay);
		check_oracle(*P, *Q, overlay);

		EXPECT_EQ(polygon_holes(overlay, Overlay::UNION), (std::vector<unsigned int>{ 0, 1 }));
		EXPECT_EQ(polygon_holes(overlay, Overlay::INTERSECTION), std::vector<unsigned int>());
		EXPECT_EQ(polygon_holes(overlay, Overlay::DIFFERENCE), (std::vector<unsigned int>{ 1 }));
		EXPECT_EQ(
				polygon_holes(overlay, Overlay::REVERSE_DIFFERENCE),
				(std::vector<unsigned int>{ 0 }));
		EXPECT_EQ(
				polygon_holes(overlay, Overlay::SYMMETRIC_DIFFERENCE),
				(std::vector<unsigned int>{ 0, 1 }));
	}
	{
		SCOPED_TRACE("the subject's island in its hole");
		const auto P = polygon(
				square(-0.03, -0.03, 0.03, 0.03),
				{ square(-0.02, -0.02, 0.02, 0.02), square(-0.012, -0.012, 0.012, 0.012) });
		const auto Q = polygon(square(0.005, -0.004, 0.04, 0.004));
		Overlay::Overlay overlay(*P, *Q);
		classify_and_check(overlay);
		check_oracle(*P, *Q, overlay);

		// The clip cuts the island and the annulus.
		EXPECT_EQ(
				polygon_holes(overlay, Overlay::INTERSECTION),
				(std::vector<unsigned int>{ 0, 0 }));
		EXPECT_EQ(polygon_holes(overlay, Overlay::DIFFERENCE), (std::vector<unsigned int>{ 0, 0 }));
	}
	{
		SCOPED_TRACE("a hole three deep");
		const auto P = polygon(
				square(-0.03, -0.03, 0.03, 0.03),
				{ square(-0.02, -0.02, 0.02, 0.02), square(-0.012, -0.012, 0.012, 0.012) });
		const auto Q = polygon(square(-0.005, -0.005, 0.005, 0.005));
		Overlay::Overlay overlay(*P, *Q);
		classify_and_check(overlay);
		check_oracle(*P, *Q, overlay);

		// The clip's ring is inside three rings: a hole of the island, not of the exterior.
		std::vector<Overlay::OutputPolygon> polygons;
		overlay.extract(Overlay::DIFFERENCE, polygons);
		ASSERT_EQ(polygons.size(), 2u);
		for (const Overlay::OutputPolygon &output_polygon : polygons)
		{
			EXPECT_EQ(output_polygon.interiors.size(), 1u);
		}
	}
}


// The orientation does not depend on the inputs': with input rings reversed, every operation's
// polygons have the same rings in the same direction (the output on the left).
TEST(PolygonOverlayTest, OrientationIndependentOfInputs)
{
	const ring_type P_exterior = square(-0.03, -0.02, 0.01, 0.02);
	const ring_type P_hole = square(-0.015, -0.005, -0.005, 0.005);
	const ring_type Q_exterior = square(-0.01, -0.025, 0.03, 0.025);
	const ring_type Q_hole = square(0.005, -0.008, 0.015, 0.008);
	const auto P = polygon(P_exterior, { P_hole });
	const auto Q = polygon(Q_exterior, { Q_hole });
	const auto P_reversed = polygon(reversed(P_exterior), { reversed(P_hole) });
	const auto Q_reversed = polygon(reversed(Q_exterior), { Q_hole });  // a hole the same way
	Overlay::Overlay overlay(*P, *Q);
	Overlay::Overlay overlay_reversed(*P_reversed, *Q_reversed);
	classify_and_check(overlay);
	classify_and_check(overlay_reversed);

	for (Overlay::Operation operation : ALL_OPERATIONS)
	{
		SCOPED_TRACE(operation);
		const auto all_rings = [operation](const Overlay::Overlay &o)
		{
			std::vector<Overlay::OutputPolygon> polygons;
			o.extract(operation, polygons);
			std::vector<Overlay::OutputRing> rings;
			for (const Overlay::OutputPolygon &polygon : polygons)
			{
				rings.push_back(polygon.exterior);
				rings.insert(rings.end(), polygon.interiors.begin(), polygon.interiors.end());
			}
			return rings;
		};
		const std::vector<Overlay::OutputRing> rings = all_rings(overlay);
		const std::vector<Overlay::OutputRing> rings_reversed = all_rings(overlay_reversed);
		ASSERT_EQ(rings.size(), rings_reversed.size());
		for (const Overlay::OutputRing &ring : rings)
		{
			bool found = false;
			for (const Overlay::OutputRing &ring_reversed : rings_reversed)
			{
				found = found || same_directed_ring(ring.points, ring_reversed.points, 1e-12);
			}
			EXPECT_TRUE(found);
		}
	}
}


// A union over a hemisphere throws, never returning its complement (phase4b-design.md section
// 5.2), unless told only to count; the intersection and the differences of the same inputs do not
// throw. The symmetric difference of two overlapping caps is two crescents, each inside an input;
// but that of two polygons sharing an edge is joined across it (the opposite-sides chain is not
// kept), and exceeds a hemisphere with their union. Under a hemisphere, the union is returned.
TEST(PolygonOverlayTest, UnionOverHemisphereThrows)
{
	{
		SCOPED_TRACE("over a hemisphere");
		const double radius = 80.0 * GPlatesMaths::PI / 180.0;
		const auto P = polygon(cap(lat_lon_radians(0.1, 0.0), radius, 72));
		const double longitude = 100.0 * GPlatesMaths::PI / 180.0;
		const auto Q = polygon(cap(lat_lon_radians(-0.1, longitude), radius, 72));
		Overlay::Overlay overlay(*P, *Q);
		classify_and_check(overlay);

		std::vector<Overlay::OutputPolygon> polygons;
		EXPECT_THROW(
				overlay.extract(Overlay::UNION, polygons),
				Overlay::InconsistentOverlayException);

		EXPECT_EQ(polygon_holes(overlay, Overlay::INTERSECTION), (std::vector<unsigned int>{ 0 }));
		EXPECT_EQ(polygon_holes(overlay, Overlay::DIFFERENCE), (std::vector<unsigned int>{ 0 }));
		EXPECT_EQ(
				polygon_holes(overlay, Overlay::SYMMETRIC_DIFFERENCE),
				(std::vector<unsigned int>{ 0, 0 }));
		for (Overlay::Operation operation : ALL_OPERATIONS)
		{
			if (operation == Overlay::UNION)
			{
				continue;
			}
			polygons.clear();
			overlay.extract(operation, polygons);
			EXPECT_EQ(PolygonOverlayOracle::count_misoriented_rings(polygons), 0u);
		}
		EXPECT_EQ(overlay.get_diagnostics().hemisphere_check_failures, 1u);  // the union's

		// Not throwing, the union is counted and its rings returned as read.
		Overlay::Options options;
		options.throw_if_inconsistent = false;
		Overlay::Overlay counting(*P, *Q, options);
		counting.classify();
		polygons.clear();
		counting.extract(Overlay::UNION, polygons);
		EXPECT_EQ(counting.get_diagnostics().hemisphere_check_failures, 1u);
	}
	{
		SCOPED_TRACE("sharing an edge, over a hemisphere");
		const double degree = GPlatesMaths::PI / 180.0;
		const PointOnSphere A = lat_lon_radians(-60 * degree, 0.0);
		const PointOnSphere B = lat_lon_radians(60 * degree, 0.0);
		const auto P = polygon(ring_type{
				A,
				lat_lon_radians(-60 * degree, 120 * degree),
				lat_lon_radians(60 * degree, 120 * degree),
				B });
		const auto Q = polygon(ring_type{
				B,
				lat_lon_radians(60 * degree, -120 * degree),
				lat_lon_radians(-60 * degree, -120 * degree),
				A });
		Overlay::Overlay overlay(*P, *Q);
		classify_and_check(overlay);
		EXPECT_EQ(overlay.get_diagnostics().common_pairs, 1u);

		std::vector<Overlay::OutputPolygon> polygons;
		EXPECT_THROW(
				overlay.extract(Overlay::UNION, polygons),
				Overlay::InconsistentOverlayException);
		EXPECT_THROW(
				overlay.extract(Overlay::SYMMETRIC_DIFFERENCE, polygons),
				Overlay::InconsistentOverlayException);
		EXPECT_EQ(polygon_holes(overlay, Overlay::INTERSECTION), std::vector<unsigned int>());
		EXPECT_EQ(polygon_holes(overlay, Overlay::DIFFERENCE), (std::vector<unsigned int>{ 0 }));
	}
	{
		SCOPED_TRACE("under a hemisphere");
		const double radius = 50.0 * GPlatesMaths::PI / 180.0;
		const auto P = polygon(cap(lat_lon_radians(0.1, 0.0), radius, 72));
		const double longitude = 60.0 * GPlatesMaths::PI / 180.0;
		const auto Q = polygon(cap(lat_lon_radians(-0.1, longitude), radius, 72));
		Overlay::Overlay overlay(*P, *Q);
		classify_and_check(overlay);
		std::vector<Overlay::OutputPolygon> polygons;
		overlay.extract(Overlay::UNION, polygons);
		ASSERT_EQ(polygons.size(), 1u);
		EXPECT_TRUE(polygons[0].interiors.empty());
		EXPECT_EQ(PolygonOverlayOracle::count_misoriented_rings(polygons), 0u);
		EXPECT_EQ(overlay.get_diagnostics().hemisphere_check_failures, 0u);
	}
}


//
// Self-touching inputs (phase5-review-3.md 1.1 and 1.2), and pairings that only a rare vertex
// shows (its section 2).
//

namespace
{
	//! A square with a triangular hole whose tip lies @a d inside its bottom edge.
	PolygonOnSphere::non_null_ptr_to_const_type
	square_with_touching_hole(
			double d)
	{
		return polygon(
				square(-0.03, -0.03, 0.03, 0.03),
				{ ring_type{ F.at(0.0, -0.03 + d), F.at(0.01, -0.01), F.at(-0.01, -0.01) } });
	}

	//! A ring through @a centre twice: two triangular lobes, at angles @a a and @a a + pi.
	ring_type
	pinched_eight(
			double a)
	{
		const auto at = [a](double along, double across)
		{
			return F.at(
					along * std::cos(a) - across * std::sin(a),
					along * std::sin(a) + across * std::cos(a));
		};
		return ring_type{
				F.at(0.0, 0.0), at(0.02, 0.005), at(0.02, -0.005),
				F.at(0.0, 0.0), at(-0.02, -0.005), at(-0.02, 0.005) };
	}
}


// Identical inputs that touch themselves: at the touch, every ray is common, so the sector walk
// knows the memberships only up to swapping both, and pairs consecutive kept rays (review 3 1.1:
// the intersection and the union threw).
TEST(PolygonOverlayTest, IdenticalSelfTouching)
{
	{
		SCOPED_TRACE("a hole touching the exterior");
		const auto P = square_with_touching_hole(0.0);
		const auto Q = square_with_touching_hole(0.0);
		Overlay::Overlay overlay(*P, *Q);
		classify_and_check(overlay);
		check_oracle(*P, *Q, overlay);
		EXPECT_EQ(polygon_holes(overlay, Overlay::INTERSECTION), (std::vector<unsigned int>{ 1 }));
		EXPECT_EQ(polygon_holes(overlay, Overlay::UNION), (std::vector<unsigned int>{ 1 }));
	}
	{
		SCOPED_TRACE("two holes touching at a corner");
		const auto rings = std::vector<ring_type>{
				square(-0.02, -0.02, 0.0, 0.0), square(0.0, 0.0, 0.02, 0.02) };
		const auto P = polygon(square(-0.03, -0.03, 0.03, 0.03), rings);
		const auto Q = polygon(square(-0.03, -0.03, 0.03, 0.03), rings);
		Overlay::Overlay overlay(*P, *Q);
		classify_and_check(overlay);
		check_oracle(*P, *Q, overlay);
		EXPECT_EQ(polygon_holes(overlay, Overlay::INTERSECTION), (std::vector<unsigned int>{ 2 }));
	}
	{
		SCOPED_TRACE("a pinched figure of eight");
		const auto P = polygon(pinched_eight(0.0));
		const auto Q = polygon(pinched_eight(0.0));
		Overlay::Overlay overlay(*P, *Q);
		classify_and_check(overlay);
		check_oracle(*P, *Q, overlay);
		EXPECT_EQ(ring_sizes(overlay, Overlay::INTERSECTION), (std::vector<unsigned int>{ 3, 3 }));
	}
	{
		SCOPED_TRACE("a square");
		const auto P = polygon(square(-0.03, -0.03, 0.03, 0.03));
		Overlay::Overlay overlay(*P, *P);
		classify_and_check(overlay);
		check_oracle(*P, *P, overlay);
	}
}


// A stretch the inputs share, which one of them touches itself on (a hole's tip on its own
// exterior's edge): GeometryIntersect cuts the other input's segment at the tip and not the
// touched one, so the stretch is cut on one side only. The overlay cuts the touched segment there
// too, so that the stretch pairs as common (review 3 1.2: the intersection and the difference were
// silently wrong, the union threw).
TEST(PolygonOverlayTest, SelfTouchOnSharedEdge)
{
	for (double d : { 0.0, 0.5 * H })
	{
		for (bool hole_in_subject : { true, false })
		{
			SCOPED_TRACE(d);
			SCOPED_TRACE(hole_in_subject);
			const auto with_hole = square_with_touching_hole(d);
			const auto without = polygon(square(-0.03, -0.03, 0.03, 0.03));
			const PolygonOnSphere &P = hole_in_subject ? *with_hole : *without;
			const PolygonOnSphere &Q = hole_in_subject ? *without : *with_hole;
			Overlay::Overlay overlay(P, Q);
			classify_and_check(overlay);
			EXPECT_EQ(overlay.get_diagnostics().self_touch_cuts, 1u);
			EXPECT_EQ(overlay.get_diagnostics().common_pairs, 5u);
			check_oracle(P, Q, overlay);
		}
	}
}


// A bounce of four kept ends (the union of two polygons touching at a corner of one on the
// other's edge): the free rule pairs each input's ends, giving two rings that touch.
TEST(PolygonOverlayTest, LinkBounceFourKept)
{
	const auto P = polygon(square(-0.02, -0.02, 0.0, 0.02));
	const auto Q = polygon(ring_type{
			F.at(0.0, 0.0), F.at(0.02, -0.01), F.at(0.03, 0.0), F.at(0.02, 0.01) });
	Overlay::Overlay overlay(*P, *Q);
	classify_and_check(overlay);
	check_oracle(*P, *Q, overlay);
	EXPECT_EQ(ring_sizes(overlay, Overlay::UNION), (std::vector<unsigned int>{ 4, 5 }));
}


// A vertex both inputs pass twice, their lobes alternating about it (two pinched figures of
// eight, crossed): under the union, four rings of three.
TEST(PolygonOverlayTest, LinkBothPassTwice)
{
	const auto P = polygon(pinched_eight(0.0));
	const auto Q = polygon(pinched_eight(0.5 * GPlatesMaths::PI));
	Overlay::Overlay overlay(*P, *Q);
	classify_and_check(overlay);
	check_oracle(*P, *Q, overlay);
	EXPECT_EQ(ring_sizes(overlay, Overlay::UNION), (std::vector<unsigned int>{ 3, 3, 3, 3 }));
	EXPECT_EQ(
			ring_sizes(overlay, Overlay::SYMMETRIC_DIFFERENCE),
			(std::vector<unsigned int>{ 3, 3, 3, 3 }));
	EXPECT_EQ(ring_sizes(overlay, Overlay::INTERSECTION), std::vector<unsigned int>());
}


//
// A polyline subject (phase4b-design.md section 6.1): classification and output, checked by the
// oracle along the polyline (section 11.3).
//

namespace
{
	//! The class (inside?) and point count of each output polyline, in order.
	std::vector<std::pair<bool, unsigned int>>
	polyline_runs(
			const Overlay::Overlay &overlay)
	{
		std::vector<Overlay::OutputPolyline> polylines;
		overlay.extract_polylines(polylines);
		std::vector<std::pair<bool, unsigned int>> runs;
		for (const Overlay::OutputPolyline &polyline : polylines)
		{
			runs.push_back(std::make_pair(polyline.inside, unsigned(polyline.points.size())));
		}
		return runs;
	}

	//! The output passes the oracle along the polyline.
	void
	check_polyline_oracle(
			const PolylineOnSphere &L,
			const PolygonOnSphere &Q,
			const Overlay::Overlay &overlay)
	{
		std::vector<Overlay::OutputPolyline> polylines;
		overlay.extract_polylines(polylines);
		unsigned int num_tested = 0;
		EXPECT_EQ(
				PolygonOverlayOracle::count_polyline_failures(L, Q, overlay, polylines, num_tested),
				0u);
		EXPECT_GT(num_tested, 100u);
		EXPECT_EQ(overlay.get_diagnostics().ll_disagreements, 0u);
	}

	typedef std::vector<std::pair<bool, unsigned int>> runs_type;
}


// A polyline across the clip, in and out twice: each piece with an original vertex is tested there;
// and a straight one across it, whose middle piece, a single arc between two crossings, is deduced.
TEST(PolygonOverlayTest, PolylineAcross)
{
	const auto Q = polygon(square(-0.01, -0.01, 0.01, 0.01));
	{
		const auto L = PolylineOnSphere::create(ring_type{
				F.at(-0.02, 0.0), F.at(0.0, 0.005), F.at(0.02, 0.0), F.at(0.005, -0.02),
				F.at(0.0, 0.0) });
		Overlay::Overlay overlay(*L, *Q);
		EXPECT_EQ(overlay.classify(), Overlay::RESULT_INTERSECTING);
		EXPECT_EQ(
				polyline_runs(overlay),
				(runs_type{ { false, 2 }, { true, 3 }, { false, 4 }, { true, 2 } }));
		EXPECT_EQ(overlay.get_diagnostics().vertex_tests, 4u);
		check_polyline_oracle(*L, *Q, overlay);
	}
	{
		const auto L = PolylineOnSphere::create(ring_type{ F.at(-0.02, 0.003), F.at(0.02, 0.003) });
		Overlay::Overlay overlay(*L, *Q);
		EXPECT_EQ(overlay.classify(), Overlay::RESULT_INTERSECTING);
		EXPECT_EQ(
				polyline_runs(overlay),
				(runs_type{ { false, 2 }, { true, 2 }, { false, 2 } }));
		EXPECT_EQ(overlay.get_diagnostics().deduced, 1u);
		check_polyline_oracle(*L, *Q, overlay);
	}
}


// A polyline with no record: wholly inside or wholly outside, one run, the polyline itself.
TEST(PolygonOverlayTest, PolylineNoRecord)
{
	const auto Q = polygon(square(-0.01, -0.01, 0.01, 0.01));
	{
		const auto L = PolylineOnSphere::create(
				ring_type{ F.at(-0.005, 0.0), F.at(0.0, 0.005), F.at(0.005, 0.0) });
		Overlay::Overlay overlay(*L, *Q);
		EXPECT_EQ(overlay.classify(), Overlay::RESULT_INSIDE);
		EXPECT_EQ(polyline_runs(overlay), (runs_type{ { true, 3 } }));
	}
	{
		const auto L = PolylineOnSphere::create(
				ring_type{ F.at(0.02, 0.0), F.at(0.03, 0.005), F.at(0.04, 0.0) });
		Overlay::Overlay overlay(*L, *Q);
		EXPECT_EQ(overlay.classify(), Overlay::RESULT_OUTSIDE);
		EXPECT_EQ(polyline_runs(overlay), (runs_type{ { false, 3 } }));
	}
}


// Along the clip's edge (requirements Q2): the piece along it is inside, decided, so that a
// polyline running along a plate boundary goes to the inside list only; outside if asked. It is
// never a source of deduction, so its class does not leak to the pieces beside it.
TEST(PolygonOverlayTest, PolylineAlong)
{
	const auto Q = polygon(square(-0.01, -0.01, 0.01, 0.01));
	const auto L = PolylineOnSphere::create(ring_type{
			F.at(-0.02, 0.01), F.at(-0.005, 0.01), F.at(0.005, 0.01), F.at(0.02, 0.01) });
	for (bool along_is_inside : { true, false })
	{
		SCOPED_TRACE(along_is_inside);
		Overlay::Options options;
		options.polyline_along_is_inside = along_is_inside;
		Overlay::Overlay overlay(*L, *Q, options);
		overlay.classify();
		// Corner to vertex, vertex to vertex, vertex to corner.
		EXPECT_EQ(overlay.get_diagnostics().along_pieces, 3u);
		if (along_is_inside)
		{
			EXPECT_EQ(
					polyline_runs(overlay),
					(runs_type{ { false, 2 }, { true, 4 }, { false, 2 } }));
		}
		else
		{
			EXPECT_EQ(polyline_runs(overlay), (runs_type{ { false, 4 + 2 } }));
		}
		check_polyline_oracle(*L, *Q, overlay);
	}
}


// A touch: a vertex of the polyline on the clip's edge with both neighbours outside. No deduction
// across a touch (the polyline bounces), so both pieces are tested and the polyline is outside.
// And an end on the clip's edge: the output starts at the end point itself.
TEST(PolygonOverlayTest, PolylineTouchAndEnd)
{
	const auto Q = polygon(square(-0.01, -0.01, 0.01, 0.01));
	{
		SCOPED_TRACE("touch");
		const auto L = PolylineOnSphere::create(
				ring_type{ F.at(-0.01, 0.02), F.at(0.0, 0.01), F.at(0.01, 0.02) });
		Overlay::Overlay overlay(*L, *Q);
		EXPECT_EQ(overlay.classify(), Overlay::RESULT_OUTSIDE);
		EXPECT_EQ(overlay.get_diagnostics().deduced, 0u);
		EXPECT_EQ(polyline_runs(overlay), (runs_type{ { false, 3 } }));
		check_polyline_oracle(*L, *Q, overlay);
	}
	{
		SCOPED_TRACE("end on the edge");
		const PointOnSphere end = F.at(0.0, 0.01);
		const auto L =
				PolylineOnSphere::create(ring_type{ F.at(0.0, 0.03), F.at(0.005, 0.02), end });
		Overlay::Overlay overlay(*L, *Q);
		EXPECT_EQ(overlay.classify(), Overlay::RESULT_OUTSIDE);
		std::vector<Overlay::OutputPolyline> polylines;
		overlay.extract_polylines(polylines);
		ASSERT_EQ(polylines.size(), 1u);
		EXPECT_TRUE(same(polylines[0].points.back(), end));
		check_polyline_oracle(*L, *Q, overlay);
	}
}


// Through the point where the clip's exterior and a hole touch (a pinch): outside before it, inside
// after (the hole is beside, not ahead). The crossing of the exterior's edge and the hole's vertex
// are two vertices at one point (no original vertex in common), so the inside run starts with
// both (near-zero output edges, PLAN.md, step 14).
TEST(PolygonOverlayTest, PolylinePinch)
{
	const auto Q = polygon(
			square(-0.02, -0.02, 0.02, 0.02),
			{ ring_type{ F.at(0.0, -0.02), F.at(0.01, 0.0), F.at(-0.01, 0.0) } });
	const auto L = PolylineOnSphere::create(ring_type{ F.at(-0.01, -0.03), F.at(0.01, -0.01) });
	Overlay::Overlay overlay(*L, *Q);
	overlay.classify();
	EXPECT_EQ(polyline_runs(overlay), (runs_type{ { false, 2 }, { true, 3 } }));
	check_polyline_oracle(*L, *Q, overlay);
}


//
// Provenance (phase4b-design.md section 7): every output point's resolves to it, and composes
// through a cascade (the outside pieces of one plate cut by the next).
//

namespace
{
	typedef std::vector<std::vector<PointOnSphere>> rings_type;

	//! A polygon's rings' points, in get_geometry_points order.
	rings_type
	rings_of(
			const PolygonOnSphere &polygon)
	{
		rings_type rings{ ring_type(
				polygon.exterior_ring_vertex_begin(), polygon.exterior_ring_vertex_end()) };
		for (unsigned int i = 0; i < polygon.number_of_interior_rings(); ++i)
		{
			rings.push_back(ring_type(
					polygon.interior_ring_vertex_begin(i), polygon.interior_ring_vertex_end(i)));
		}
		return rings;
	}

	//! Every point of @a polygons resolves to it, against the original subject and the clips.
	unsigned int
	provenance_failures(
			const std::vector<Overlay::OutputPolygon> &polygons,
			const rings_type &subject,
			const std::vector<rings_type> &clips,
			double tolerance)
	{
		unsigned int failures = 0;
		for (const Overlay::OutputPolygon &polygon : polygons)
		{
			failures += PolygonOverlayOracle::count_provenance_failures(
					subject,
					clips,
					polygon.exterior.points,
					polygon.exterior.provenances,
					tolerance);
			for (const Overlay::OutputRing &interior : polygon.interiors)
			{
				failures += PolygonOverlayOracle::count_provenance_failures(
						subject, clips, interior.points, interior.provenances, tolerance);
			}
		}
		return failures;
	}
}


// The provenance of each kind of point (section 7.1), on holes too: two squares overlapping at a
// corner, the subject with a hole. Original vertices of either input, crossings (a subject segment
// and a clip segment), and the clip's vertex inside the subject (a cut point, with no subject
// location).
TEST(PolygonOverlayTest, Provenance)
{
	const auto P = polygon(
			square(-0.02, -0.02, 0.005, 0.005),
			{ square(-0.015, -0.015, -0.008, -0.008) });
	const auto Q = polygon(square(-0.005, -0.005, 0.02, 0.02));
	Overlay::Options options;
	options.want_provenance = true;
	Overlay::Overlay overlay(*P, *Q, options);
	classify_and_check(overlay);

	for (Overlay::Operation operation : ALL_OPERATIONS)
	{
		SCOPED_TRACE(operation);
		std::vector<Overlay::OutputPolygon> polygons;
		overlay.extract(operation, polygons);
		EXPECT_EQ(provenance_failures(polygons, rings_of(*P), { rings_of(*Q) }, 2 * H), 0u);
	}

	// The intersection: the clip's corner (-0.005, -0.005) inside the subject is a cut point.
	std::vector<Overlay::OutputPolygon> polygons;
	overlay.extract(Overlay::INTERSECTION, polygons);
	unsigned int num_cut = 0, num_crossing = 0, num_subject_vertex = 0;
	for (const Overlay::OutputPolygon &polygon : polygons)
	{
		for (const Overlay::OutputRing *ring : { &polygon.exterior })
		{
			for (const Overlay::Provenance &provenance : ring->provenances)
			{
				num_cut +=
						(!provenance.subject && provenance.clip && !provenance.clip->ratio) ? 1 : 0;
				num_crossing += (provenance.subject && provenance.subject->ratio &&
						provenance.clip && provenance.clip->ratio) ? 1 : 0;
				num_subject_vertex += (provenance.subject && !provenance.subject->ratio) ? 1 : 0;
			}
		}
	}
	EXPECT_GE(num_cut, 1u);
	EXPECT_GE(num_crossing, 2u);
	EXPECT_GE(num_subject_vertex, 1u);
	EXPECT_EQ(overlay.get_diagnostics().provenance_subject_mismatches, 0u);
}


// A two-step cascade (section 7.2): the subject's pieces outside plate 1 are cut by plate 2 with
// their provenance tables, and every output point resolves against the original subject, or plate
// 1 or plate 2 by its clip index. Plate 2 both beside plate 1 (sharing its edge, along which the
// pieces' cut points run) and cutting the pieces again (composing a ratio along a segment of a
// piece that is a sub-arc of the original's). The subject's ring starts at its top right corner,
// so that its closing segment, the right edge, is cut by plate 2: composing a ratio on it needs
// the ring's size (vertex 0 ends that segment).
TEST(PolygonOverlayTest, ProvenanceCascade)
{
	const auto P = polygon(
			ring_type{ F.at(0.03, 0.02), F.at(-0.03, 0.02), F.at(-0.03, -0.02), F.at(0.03, -0.02) },
			{ square(-0.006, -0.006, 0.004, 0.004) });
	const auto plate_1 = polygon(square(-0.05, -0.05, 0.0, 0.05));
	for (double plate_2_left : { 0.0, 0.012 })
	{
		SCOPED_TRACE(plate_2_left);
		const auto plate_2 = polygon(square(plate_2_left, -0.01, 0.05, 0.05));

		Overlay::Options options;
		options.want_provenance = true;
		Overlay::Overlay first(*P, *plate_1, options);
		classify_and_check(first);
		std::vector<Overlay::OutputPolygon> outside;
		first.extract(Overlay::DIFFERENCE, outside);
		ASSERT_FALSE(outside.empty());

		for (const Overlay::OutputPolygon &piece : outside)
		{
			std::vector<ring_type> interiors;
			Overlay::provenance_seq_type table = piece.exterior.provenances;
			for (const Overlay::OutputRing &interior : piece.interiors)
			{
				interiors.push_back(interior.points);
				table.insert(table.end(), interior.provenances.begin(), interior.provenances.end());
			}
			const auto S = polygon(piece.exterior.points, interiors);

			Overlay::Options second_options;
			second_options.want_provenance = true;
			second_options.subject_provenance = &table;
			second_options.clip_index = 1;
			for (const ring_type &ring : rings_of(*P))
			{
				second_options.subject_ring_sizes.push_back(ring.size());
			}
			second_options.clip_ring_sizes = { { 4 } };  // plate 1, a square
			Overlay::Overlay second(*S, *plate_2, second_options);
			classify_and_check(second);
			for (Overlay::Operation operation : { Overlay::INTERSECTION, Overlay::DIFFERENCE })
			{
				SCOPED_TRACE(operation);
				std::vector<Overlay::OutputPolygon> polygons;
				second.extract(operation, polygons);
				EXPECT_EQ(
						provenance_failures(
								polygons,
								rings_of(*P),
								{ rings_of(*plate_1), rings_of(*plate_2) },
								2 * H),
						0u);
			}
			EXPECT_EQ(second.get_diagnostics().provenance_subject_mismatches, 0u);
		}
	}
}


// A polyline's runs carry provenance too, and compose through a cascade.
TEST(PolygonOverlayTest, ProvenancePolyline)
{
	const ring_type line_points{ F.at(-0.03, 0.001), F.at(0.0, 0.004), F.at(0.03, 0.002) };
	const auto L = PolylineOnSphere::create(line_points);
	const auto plate_1 = polygon(square(-0.05, -0.05, -0.01, 0.05));
	const auto plate_2 = polygon(square(0.01, -0.05, 0.05, 0.05));

	Overlay::Options options;
	options.want_provenance = true;
	Overlay::Overlay first(*L, *plate_1, options);
	first.classify();
	std::vector<Overlay::OutputPolyline> runs;
	first.extract_polylines(runs);
	ASSERT_EQ(runs.size(), 2u);
	for (const Overlay::OutputPolyline &run : runs)
	{
		EXPECT_EQ(
				PolygonOverlayOracle::count_provenance_failures(
						{ line_points },
						{ rings_of(*plate_1) },
						run.points,
						run.provenances,
						2 * H),
				0u);
	}

	// The run outside plate 1, cut by plate 2.
	const Overlay::OutputPolyline &outside = runs[1];
	ASSERT_FALSE(outside.inside);
	const auto piece = PolylineOnSphere::create(outside.points);
	Overlay::Options second_options;
	second_options.want_provenance = true;
	second_options.subject_provenance = &outside.provenances;
	second_options.clip_index = 1;
	Overlay::Overlay second(*piece, *plate_2, second_options);
	second.classify();
	std::vector<Overlay::OutputPolyline> second_runs;
	second.extract_polylines(second_runs);
	EXPECT_EQ(second_runs.size(), 2u);
	for (const Overlay::OutputPolyline &run : second_runs)
	{
		EXPECT_EQ(
				PolygonOverlayOracle::count_provenance_failures(
						{ line_points },
						{ rings_of(*plate_1), rings_of(*plate_2) },
						run.points,
						run.provenances,
						2 * H),
				0u);
	}
	EXPECT_EQ(second.get_diagnostics().provenance_subject_mismatches, 0u);
}


// Provenance at a vertex where a ring is split (review 4 1.1): the clip is the subject's square
// with a hole whose tip touches its bottom edge, so the tip holds a clip vertex and no subject
// vertex, and the intersection's walk passes it twice, once along the subject's (shared) edge and
// once along the hole. After the split, each half takes its location from its own pieces: on the
// subject's bottom edge in the exterior, a cut in the hole.
TEST(PolygonOverlayTest, ProvenanceAtSplit)
{
	const auto P = polygon(square(-0.03, -0.03, 0.03, 0.03));
	const auto Q = square_with_touching_hole(0.0);
	Overlay::Options options;
	options.want_provenance = true;
	Overlay::Overlay overlay(*P, *Q, options);
	classify_and_check(overlay);
	for (Overlay::Operation operation : ALL_OPERATIONS)
	{
		SCOPED_TRACE(operation);
		std::vector<Overlay::OutputRing> rings;
		overlay.extract_rings(operation, rings);
		for (const Overlay::OutputRing &ring : rings)
		{
			EXPECT_EQ(
					PolygonOverlayOracle::count_provenance_failures(
							rings_of(*P), { rings_of(*Q) }, ring.points, ring.provenances, 2 * H),
					0u);
			EXPECT_EQ(
					PolygonOverlayOracle::count_missing_subject_locations(
							*P, *Q, ring.points, ring.provenances),
					0u);
		}
	}
}
