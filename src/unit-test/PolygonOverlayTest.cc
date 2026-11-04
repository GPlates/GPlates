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


// A ray with no direction (catalogue 4.4, d4 hole-between, length 0.5h): the subject's segment u-w,
// shorter than GreatCircleArc's zero length, runs between two touches of the clip's edge, with the
// apex of a subject hole touching that edge between them, so it is not common. Its rays have no
// direction and a cone of pi; the vertices it ends at have four ends, so no order is needed.
TEST(PolygonOverlayTest, RaysWithoutDirection)
{
	const double half = 0.25 * H * std::sqrt(0.5);
	const auto Q = polygon(square(-0.03, -0.03, 0.03, 0.0));
	const auto P = polygon(
			ring_type{ F.at(-half, half), F.at(half, -half), F.at(0.01, -0.01), F.at(0.02, 0.01) },
			{ ring_type{ F.at(0.3 * half, 0.0), F.at(0.003, -0.001), F.at(0.004, 0.001) } });
	Overlay::Overlay overlay(*P, *Q);
	check_structure(overlay);
	classify_and_check(overlay);

	EXPECT_EQ(overlay.get_diagnostics().zero_length_rays, 2u);
	unsigned int num_rays_without_direction = 0;
	for (const Overlay::Vertex &vertex : overlay.get_vertices())
	{
		for (const Overlay::Ray &ray : vertex.rays)
		{
			if (!ray.direction)
			{
				++num_rays_without_direction;
				EXPECT_EQ(ray.cone, GPlatesMaths::PI);
				EXPECT_FALSE(ray.is_common());
				EXPECT_EQ(vertex.ends.size(), 4u);
			}
		}
	}
	EXPECT_EQ(num_rays_without_direction, 2u);
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
