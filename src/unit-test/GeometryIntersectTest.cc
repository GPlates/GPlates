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

#include <cmath>
#include <iterator>
#include <vector>
#include <gtest/gtest.h>

#include "maths/GeometryIntersect.h"
#include "maths/GreatCircleArc.h"
#include "maths/MathsUtils.h"
#include "maths/PointInPolygon.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/UnitVector3D.h"
#include "maths/Vector3D.h"


//
// Tests of GeometryIntersect, the intersection primitive under topology resolving and partitioning.
// The cases are those of the Phase 3 audit (docs/plans/polygon-intersections/phase3-audit.md,
// section 6.1), with the expectations of its fixes F1 (the thickness is the point-on-outline
// threshold) and F2 (a touch is ordered by its projection onto the touched segment).
//
// Geometries are built in local tangent coordinates (east, north) at a centre that is not on any
// axis, so that nothing lines up with a coordinate axis by accident. Distances are in units of the
// thickness, H, so that the cases stay on the intended side of it whatever its value.
//

namespace
{
	using GPlatesMaths::GeometryIntersect::Graph;
	using GPlatesMaths::GeometryIntersect::Intersection;
	using GPlatesMaths::PointOnSphere;
	using GPlatesMaths::PolygonOnSphere;
	using GPlatesMaths::PolylineOnSphere;
	using GPlatesMaths::UnitVector3D;
	using GPlatesMaths::Vector3D;

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


	PolylineOnSphere::non_null_ptr_to_const_type
	polyline(
			const std::vector<PointOnSphere> &points)
	{
		return PolylineOnSphere::create(points);
	}

	PolygonOnSphere::non_null_ptr_to_const_type
	polygon(
			const std::vector<PointOnSphere> &exterior,
			const std::vector<std::vector<PointOnSphere>> &interiors = {})
	{
		return PolygonOnSphere::create(exterior, interiors);
	}


	/**
	 * The angle between two points, accurate near zero (an arc cosine of their dot product cannot
	 * resolve less than about 1.5e-8 radians).
	 */
	double
	angle_between(
			const PointOnSphere &a,
			const PointOnSphere &b)
	{
		return std::atan2(
				cross(a.position_vector(), b.position_vector()).magnitude().dval(),
				dot(a.position_vector(), b.position_vector()).dval());
	}


	//! The intersections in order along the first (or second) geometry.
	std::vector<Intersection>
	ordered(
			const Graph &graph,
			bool along_geometry1)
	{
		const std::vector<unsigned int> &order = along_geometry1
				? graph.geometry1_ordered_intersections
				: graph.geometry2_ordered_intersections;

		std::vector<Intersection> intersections;
		for (const unsigned int index : order)
		{
			intersections.push_back(graph.unordered_intersections[index]);
		}
		return intersections;
	}


	Intersection::Type
	mirrored(
			Intersection::Type type)
	{
		switch (type)
		{
		case Intersection::SEGMENT1_START_ON_SEGMENT2:
			return Intersection::SEGMENT2_START_ON_SEGMENT1;
		case Intersection::SEGMENT2_START_ON_SEGMENT1:
			return Intersection::SEGMENT1_START_ON_SEGMENT2;
		default:
			return type;
		}
	}


	/**
	 * Checks that intersecting @a a with @a b, and @a b with @a a, report the same contacts: the
	 * same count, mirrored types and segment indices, the same positions (an SS record can differ by
	 * up to the thickness, since it is placed at the first geometry's vertex), and the two orders
	 * swapped.
	 */
	void
	expect_symmetric(
			const PolylineOnSphere &a,
			const PolylineOnSphere &b)
	{
		Graph ab;
		Graph ba;
		const bool ab_intersect = GPlatesMaths::GeometryIntersect::intersect(ab, a, b);
		const bool ba_intersect = GPlatesMaths::GeometryIntersect::intersect(ba, b, a);
		ASSERT_EQ(ab_intersect, ba_intersect);
		ASSERT_EQ(ab.unordered_intersections.size(), ba.unordered_intersections.size());

		// Along a in (a, b) is along a in (b, a), where a is the second geometry.
		const std::vector<Intersection> ab_along_a = ordered(ab, true);
		const std::vector<Intersection> ba_along_a = ordered(ba, false);
		for (unsigned int i = 0; i < ab_along_a.size(); ++i)
		{
			SCOPED_TRACE(testing::Message() << "intersection " << i << " along the first geometry");
			const Intersection &x = ab_along_a[i];
			const Intersection &y = ba_along_a[i];
			EXPECT_EQ(x.type, mirrored(y.type));
			EXPECT_EQ(x.segment_index1, y.segment_index2);
			EXPECT_EQ(x.segment_index2, y.segment_index1);
			const double tolerance = (x.type == Intersection::SEGMENT_START_ON_SEGMENT_START) ? 1.01 * H : 1e-12;
			EXPECT_LE(angle_between(x.position, y.position), tolerance);
		}
	}
}


//
// 1. A proper crossing.
//

TEST(GeometryIntersectTest, proper_crossing)
{
	const double a = 0.01;
	const auto p = polyline({ F.at(-a, 0), F.at(a, 0) });
	const auto q = polyline({ F.at(0, -a), F.at(0, a) });

	Graph graph;
	ASSERT_TRUE(GPlatesMaths::GeometryIntersect::intersect(graph, *p, *q));
	ASSERT_EQ(graph.unordered_intersections.size(), 1u);

	const Intersection &x = graph.unordered_intersections[0];
	EXPECT_EQ(x.type, Intersection::SEGMENTS_CROSS);
	EXPECT_EQ(x.segment_index1, 0u);
	EXPECT_EQ(x.segment_index2, 0u);
	// The two great circles cross at the frame's centre.
	EXPECT_LE(angle_between(x.position, F.at(0, 0)), 1e-12);
	EXPECT_NEAR(x.angle_in_segment1.calculate_angle().dval(), angle_between(F.at(-a, 0), F.at(0, 0)), 1e-12);
	EXPECT_NEAR(x.angle_in_segment2.calculate_angle().dval(), angle_between(F.at(0, -a), F.at(0, 0)), 1e-12);

	expect_symmetric(*p, *q);
}


//
// 2. A vertex shared bit for bit: one SS record, at every kind of vertex.
//

TEST(GeometryIntersectTest, shared_vertex)
{
	const double a = 0.01;
	const PointOnSphere v = F.at(0, 0);
	// Q's vertex v is the start of its segment 1.
	const auto q = polyline({ F.at(0, -a), v, F.at(a, a) });

	struct Case { const char *name; std::vector<PointOnSphere> p; unsigned int q_index; unsigned int p_index; };
	const Case cases[] = {
		{ "inner vertex", { F.at(-a, 0.2 * a), v, F.at(-0.2 * a, a) }, 1, 1 },
		{ "first vertex", { v, F.at(-a, 0.3 * a) }, 1, 0 },
		// P's last vertex: the start of its fictitious segment, index number_of_segments.
		{ "last vertex", { F.at(-a, 0.3 * a), v }, 1, 1 },
	};
	for (const Case &c : cases)
	{
		SCOPED_TRACE(c.name);
		const auto p = polyline(c.p);
		Graph graph;
		ASSERT_TRUE(GPlatesMaths::GeometryIntersect::intersect(graph, *q, *p));
		ASSERT_EQ(graph.unordered_intersections.size(), 1u);
		const Intersection &x = graph.unordered_intersections[0];
		EXPECT_EQ(x.type, Intersection::SEGMENT_START_ON_SEGMENT_START);
		EXPECT_EQ(x.segment_index1, c.q_index);
		EXPECT_EQ(x.segment_index2, c.p_index);
		EXPECT_EQ(x.angle_in_segment1.calculate_angle().dval(), 0.0);
		EXPECT_EQ(x.angle_in_segment2.calculate_angle().dval(), 0.0);
		EXPECT_EQ(x.position, v);
		expect_symmetric(*q, *p);
	}

	// Both geometries' last vertices: both indices fictitious.
	{
		SCOPED_TRACE("both last vertices");
		const auto q_ending = polyline({ F.at(0, -a), v });
		const auto p = polyline({ F.at(-a, 0.3 * a), v });
		Graph graph;
		ASSERT_TRUE(GPlatesMaths::GeometryIntersect::intersect(graph, *q_ending, *p));
		ASSERT_EQ(graph.unordered_intersections.size(), 1u);
		const Intersection &x = graph.unordered_intersections[0];
		EXPECT_EQ(x.type, Intersection::SEGMENT_START_ON_SEGMENT_START);
		EXPECT_EQ(x.segment_index1, 1u);
		EXPECT_EQ(x.segment_index2, 1u);
	}

	// A polygon ring whose first vertex is the shared vertex: index 0, no fictitious index.
	{
		SCOPED_TRACE("polygon's first vertex");
		const auto ring = polygon({ v, F.at(-a, 0.3 * a), F.at(-a, a) });
		Graph graph;
		ASSERT_TRUE(GPlatesMaths::GeometryIntersect::intersect(graph, *q, *ring));
		ASSERT_EQ(graph.unordered_intersections.size(), 1u);
		const Intersection &x = graph.unordered_intersections[0];
		EXPECT_EQ(x.type, Intersection::SEGMENT_START_ON_SEGMENT_START);
		EXPECT_EQ(x.segment_index1, 1u);
		EXPECT_EQ(x.segment_index2, 0u);
	}
}


//
// 3. A vertex on an edge: a touch within the thickness, nothing beyond it.
//

TEST(GeometryIntersectTest, vertex_on_edge)
{
	const double a = 0.01;
	const auto q = polyline({ F.at(-a, 0), F.at(a, 0) });
	// The touch's key is the distance along Q to the vertex's projection, which is F.at(0.1 a, 0).
	const double expected_angle = angle_between(F.at(-a, 0), F.at(0.1 * a, 0));

	struct Case { const char *name; double off; std::vector<PointOnSphere> p; unsigned int p_index; };
	const Case cases[] = {
		{ "inner vertex", 0.99, { F.at(0, 0.5 * a), F.at(0.1 * a, 0.99 * H), F.at(0.2 * a, 0.5 * a) }, 1 },
		{ "first vertex", 0.99, { F.at(0.1 * a, 0.99 * H), F.at(0.2 * a, 0.5 * a) }, 0 },
		{ "last vertex", 0.99, { F.at(0, 0.5 * a), F.at(0.1 * a, 0.99 * H) }, 1 },
	};
	for (const Case &c : cases)
	{
		SCOPED_TRACE(c.name);
		const auto p = polyline(c.p);
		Graph graph;
		ASSERT_TRUE(GPlatesMaths::GeometryIntersect::intersect(graph, *q, *p));
		ASSERT_EQ(graph.unordered_intersections.size(), 1u);
		const Intersection &x = graph.unordered_intersections[0];
		EXPECT_EQ(x.type, Intersection::SEGMENT2_START_ON_SEGMENT1);
		EXPECT_EQ(x.segment_index1, 0u);
		EXPECT_EQ(x.segment_index2, c.p_index);
		EXPECT_EQ(x.position, F.at(0.1 * a, 0.99 * H));
		EXPECT_NEAR(x.angle_in_segment1.calculate_angle().dval(), expected_angle, 1e-12);
		expect_symmetric(*q, *p);
	}

	// Just beyond the thickness: no contact at all.
	const auto p_beyond = polyline({ F.at(0, 0.5 * a), F.at(0.1 * a, 1.01 * H), F.at(0.2 * a, 0.5 * a) });
	Graph graph;
	EXPECT_FALSE(GPlatesMaths::GeometryIntersect::intersect(graph, *q, *p_beyond));
	EXPECT_TRUE(graph.empty());
}


//
// 4. Overlaps: every vertex inside a stretch where the two run together is a record, and nothing else.
//

TEST(GeometryIntersectTest, overlaps)
{
	const double a = 0.01;
	// Q has a vertex at the centre, inside the stretch along which P runs.
	const auto q = polyline({ F.at(-a, 0), F.at(0, 0), F.at(a, 0) });

	{
		SCOPED_TRACE("interleaved");
		const auto p = polyline({ F.at(-0.5 * a, 0), F.at(0.2 * a, 0), F.at(0.6 * a, 0) });
		Graph graph;
		ASSERT_TRUE(GPlatesMaths::GeometryIntersect::intersect(graph, *q, *p));
		const std::vector<Intersection> along_p = ordered(graph, false);
		ASSERT_EQ(along_p.size(), 4u);
		// P's first vertex, Q's vertex, P's middle vertex, P's last vertex, in that order.
		EXPECT_EQ(along_p[0].position, F.at(-0.5 * a, 0));
		EXPECT_EQ(along_p[1].position, F.at(0, 0));
		EXPECT_EQ(along_p[2].position, F.at(0.2 * a, 0));
		EXPECT_EQ(along_p[3].position, F.at(0.6 * a, 0));
		EXPECT_EQ(along_p[1].type, Intersection::SEGMENT1_START_ON_SEGMENT2);
		EXPECT_EQ(along_p[3].segment_index2, 2u);  // P's last vertex
		expect_symmetric(*q, *p);
	}
	{
		SCOPED_TRACE("reversed");
		const auto p = polyline({ F.at(0.6 * a, 0), F.at(0.2 * a, 0), F.at(-0.5 * a, 0) });
		Graph graph;
		ASSERT_TRUE(GPlatesMaths::GeometryIntersect::intersect(graph, *q, *p));
		const std::vector<Intersection> along_p = ordered(graph, false);
		ASSERT_EQ(along_p.size(), 4u);
		EXPECT_EQ(along_p[0].position, F.at(0.6 * a, 0));
		EXPECT_EQ(along_p[1].position, F.at(0.2 * a, 0));
		EXPECT_EQ(along_p[2].position, F.at(0, 0));
		EXPECT_EQ(along_p[3].position, F.at(-0.5 * a, 0));
		expect_symmetric(*q, *p);
	}
	{
		SCOPED_TRACE("the whole edge");
		const auto q_edge = polyline({ F.at(-a, 0), F.at(a, 0) });
		const auto p = polyline({ F.at(-a, 0), F.at(a, 0) });
		Graph graph;
		ASSERT_TRUE(GPlatesMaths::GeometryIntersect::intersect(graph, *q_edge, *p));
		const std::vector<Intersection> along_p = ordered(graph, false);
		ASSERT_EQ(along_p.size(), 2u);
		EXPECT_EQ(along_p[0].type, Intersection::SEGMENT_START_ON_SEGMENT_START);
		EXPECT_EQ(along_p[1].type, Intersection::SEGMENT_START_ON_SEGMENT_START);
		EXPECT_EQ(along_p[1].segment_index1, 1u);
		EXPECT_EQ(along_p[1].segment_index2, 1u);
	}
	{
		SCOPED_TRACE("round the corner");
		const auto q_corner = polyline({ F.at(-a, 0), F.at(0, 0), F.at(0, a) });
		const auto p = polyline({ F.at(-a, 0), F.at(0, 0), F.at(0, a) });
		Graph graph;
		ASSERT_TRUE(GPlatesMaths::GeometryIntersect::intersect(graph, *q_corner, *p));
		const std::vector<Intersection> along_p = ordered(graph, false);
		ASSERT_EQ(along_p.size(), 3u);
		for (const Intersection &x : along_p)
		{
			EXPECT_EQ(x.type, Intersection::SEGMENT_START_ON_SEGMENT_START);
		}
	}
}


//
// 5. Zero-length segments.
//

TEST(GeometryIntersectTest, zero_length_segments)
{
	const double a = 0.01;
	const auto q = polyline({ F.at(-a, 0), F.at(0, 0), F.at(0, a) });

	{
		// A 0.5 H segment of P straddling Q's first edge: both its ends are within the thickness, so
		// two touches and no crossing.
		SCOPED_TRACE("straddling an edge");
		const auto p = polyline({
				F.at(-0.5 * a, 0.5 * a), F.at(-0.5 * a, 0.25 * H), F.at(-0.5 * a, -0.25 * H), F.at(-0.5 * a, -0.5 * a) });
		ASSERT_TRUE(std::next(p->begin(), 1)->is_zero_length());
		Graph graph;
		ASSERT_TRUE(GPlatesMaths::GeometryIntersect::intersect(graph, *q, *p));
		const std::vector<Intersection> along_p = ordered(graph, false);
		ASSERT_EQ(along_p.size(), 2u);
		for (const Intersection &x : along_p)
		{
			EXPECT_EQ(x.type, Intersection::SEGMENT2_START_ON_SEGMENT1);
		}
		EXPECT_EQ(along_p[0].segment_index2, 1u);
		EXPECT_EQ(along_p[1].segment_index2, 2u);
		expect_symmetric(*q, *p);
	}
	{
		// A zero-length segment of P at Q's vertex: both of its ends coincide with the vertex.
		SCOPED_TRACE("at a vertex");
		const auto p = polyline({
				F.at(-0.5 * a, -0.5 * a), F.at(-0.2 * H, -0.2 * H), F.at(0.2 * H, 0.2 * H), F.at(0.5 * a, 0.3 * a) });
		ASSERT_TRUE(std::next(p->begin(), 1)->is_zero_length());
		Graph graph;
		ASSERT_TRUE(GPlatesMaths::GeometryIntersect::intersect(graph, *q, *p));
		const std::vector<Intersection> along_p = ordered(graph, false);
		ASSERT_EQ(along_p.size(), 2u);
		for (const Intersection &x : along_p)
		{
			EXPECT_EQ(x.type, Intersection::SEGMENT_START_ON_SEGMENT_START);
			EXPECT_EQ(x.segment_index1, 1u);
		}
		expect_symmetric(*q, *p);
	}
}


//
// 6. A short segment crossing an edge: a crossing needs both ends beyond the thickness.
//

TEST(GeometryIntersectTest, short_crossing_segment)
{
	const double a = 0.01;
	const auto q = polyline({ F.at(-a, 0), F.at(a, 0) });

	struct Case { const char *name; double length; double degrees; unsigned int records; Intersection::Type type; };
	const Case cases[] = {
		// At a right angle: shorter than twice the thickness, two touches; longer, one crossing.
		{ "1.5 H at 90 degrees", 1.5 * H, 90, 2, Intersection::SEGMENT2_START_ON_SEGMENT1 },
		{ "3 H at 90 degrees", 3 * H, 90, 1, Intersection::SEGMENTS_CROSS },
		// At 20 degrees a 3 H segment's ends are only 0.51 H from the edge: still two touches.
		{ "3 H at 20 degrees", 3 * H, 20, 2, Intersection::SEGMENT2_START_ON_SEGMENT1 },
	};
	for (const Case &c : cases)
	{
		SCOPED_TRACE(c.name);
		const double radians = c.degrees * GPlatesMaths::PI / 180.0;
		const double de = 0.5 * c.length * std::cos(radians);
		const double dn = 0.5 * c.length * std::sin(radians);
		// The short segment, between two long arms that stay well away from the edge.
		const auto p = polyline({ F.at(-de - 0.3 * a, dn + 0.5 * a), F.at(-de, dn), F.at(de, -dn), F.at(de + 0.3 * a, -dn - 0.5 * a) });
		Graph graph;
		ASSERT_TRUE(GPlatesMaths::GeometryIntersect::intersect(graph, *q, *p));
		const std::vector<Intersection> along_p = ordered(graph, false);
		ASSERT_EQ(along_p.size(), c.records);
		for (const Intersection &x : along_p)
		{
			EXPECT_EQ(x.type, c.type);
		}
		if (c.records == 2)
		{
			EXPECT_NEAR(angle_between(along_p[0].position, along_p[1].position), c.length, 1e-3 * H);
		}
		expect_symmetric(*q, *p);
	}
}


//
// 7. Ordering within a segment: a touch near the segment's start sorts by where along the segment
// it is, not by the angle to the touching vertex (fix F2, audit D1).
//

TEST(GeometryIntersectTest, ordering_within_a_segment)
{
	// P's first segment runs east from the centre. Q's vertex v touches it from above, 3.0e-6 along
	// and 0.99 H off; another edge of Q crosses it at 3.15e-6. The chord angle to v is
	// sqrt(3.0e-6^2 + (0.99 H)^2) = 3.3e-6, beyond the crossing, so the old key sorted v second.
	const auto p = polyline({ F.at(0, 0), F.at(0.01, 0) });
	const PointOnSphere v = F.at(3.0e-6, 0.99 * H);
	const auto q = polyline({ F.at(2.9e-6, 0.001), v, F.at(3.15e-6, 0.001), F.at(3.15e-6, -0.001) });

	Graph graph;
	ASSERT_TRUE(GPlatesMaths::GeometryIntersect::intersect(graph, *q, *p));
	const std::vector<Intersection> along_p = ordered(graph, false);
	ASSERT_EQ(along_p.size(), 2u);
	EXPECT_EQ(along_p[0].type, Intersection::SEGMENT1_START_ON_SEGMENT2);
	EXPECT_EQ(along_p[0].position, v);
	EXPECT_EQ(along_p[1].type, Intersection::SEGMENTS_CROSS);
	EXPECT_NEAR(along_p[0].angle_in_segment2.calculate_angle().dval(), 3.0e-6, 1e-9);
	EXPECT_NEAR(along_p[1].angle_in_segment2.calculate_angle().dval(), 3.15e-6, 1e-9);

	// Records on different segments, and well apart on one segment, sort by segment then distance.
	const auto p2 = polyline({ F.at(-0.01, 0), F.at(0, 0), F.at(0.01, 0) });
	const auto q2 = polyline({
			F.at(-0.007, 0.001), F.at(-0.007, -0.001), F.at(-0.003, -0.001), F.at(-0.003, 0.001),
			F.at(0.004, 0.001), F.at(0.004, -0.001) });
	Graph graph2;
	ASSERT_TRUE(GPlatesMaths::GeometryIntersect::intersect(graph2, *q2, *p2));
	const std::vector<Intersection> along_p2 = ordered(graph2, false);
	ASSERT_EQ(along_p2.size(), 3u);
	EXPECT_EQ(along_p2[0].segment_index2, 0u);
	EXPECT_EQ(along_p2[1].segment_index2, 0u);
	EXPECT_EQ(along_p2[2].segment_index2, 1u);
	EXPECT_LT(along_p2[0].angle_in_segment2.calculate_angle().dval(), along_p2[1].angle_in_segment2.calculate_angle().dval());
}


//
// 8. Rings: segment indices run through the exterior ring and then each interior ring.
//

TEST(GeometryIntersectTest, rings)
{
	const double a = 0.01;
	const auto q = polygon(
			{ F.at(-a, -a), F.at(a, -a), F.at(a, a), F.at(-a, a) },
			{ { F.at(-0.5 * a, -0.5 * a), F.at(0.5 * a, -0.5 * a), F.at(0.5 * a, 0.5 * a), F.at(-0.5 * a, 0.5 * a) } });
	ASSERT_EQ(q->number_of_segments_in_exterior_ring(), 4u);

	// A line crossing the exterior's left edge (its last segment), the hole's left and right edges
	// (global indices 7 and 5) and the exterior's right edge.
	const auto p = polyline({ F.at(-2 * a, 0.1 * a), F.at(2 * a, 0.1 * a) });
	{
		Graph graph;
		ASSERT_TRUE(GPlatesMaths::GeometryIntersect::intersect(graph, *q, *p));
		const std::vector<Intersection> along_p = ordered(graph, false);
		ASSERT_EQ(along_p.size(), 4u);
		EXPECT_EQ(along_p[0].segment_index1, 3u);
		EXPECT_EQ(along_p[1].segment_index1, 7u);
		EXPECT_EQ(along_p[2].segment_index1, 5u);
		EXPECT_EQ(along_p[3].segment_index1, 1u);
		// Along Q: exterior first, then the hole.
		const std::vector<Intersection> along_q = ordered(graph, true);
		EXPECT_EQ(along_q[0].segment_index1, 1u);
		EXPECT_EQ(along_q[1].segment_index1, 3u);
		EXPECT_EQ(along_q[2].segment_index1, 5u);
		EXPECT_EQ(along_q[3].segment_index1, 7u);
	}
	{
		SCOPED_TRACE("exterior ring only");
		Graph graph;
		ASSERT_TRUE(GPlatesMaths::GeometryIntersect::intersect(graph, *q, *p, false));
		EXPECT_EQ(graph.unordered_intersections.size(), 2u);
	}
	{
		// A contact at a ring's first vertex, which is also its last segment's end, is recorded on
		// its first segment.
		SCOPED_TRACE("at the ring's first vertex");
		const auto p_corner = polyline({ F.at(-2 * a, -0.5 * a), F.at(-a, -a), F.at(-0.5 * a, -2 * a) });
		Graph graph;
		ASSERT_TRUE(GPlatesMaths::GeometryIntersect::intersect(graph, *q, *p_corner));
		ASSERT_EQ(graph.unordered_intersections.size(), 1u);
		EXPECT_EQ(graph.unordered_intersections[0].type, Intersection::SEGMENT_START_ON_SEGMENT_START);
		EXPECT_EQ(graph.unordered_intersections[0].segment_index1, 0u);
	}
	{
		// A hole touching the exterior at a vertex of both, against a vertex of P there: one record
		// per ring.
		SCOPED_TRACE("hole touching the exterior");
		const auto q_touching = polygon(
				{ F.at(-a, -a), F.at(a, -a), F.at(a, a), F.at(-a, a) },
				{ { F.at(a, a), F.at(0, 0.5 * a), F.at(0.5 * a, 0) } });
		const auto p_outside = polyline({ F.at(1.5 * a, 2 * a), F.at(a, a), F.at(2 * a, 1.5 * a) });
		Graph graph;
		ASSERT_TRUE(GPlatesMaths::GeometryIntersect::intersect(graph, *q_touching, *p_outside));
		const std::vector<Intersection> along_q = ordered(graph, true);
		ASSERT_EQ(along_q.size(), 2u);
		EXPECT_EQ(along_q[0].type, Intersection::SEGMENT_START_ON_SEGMENT_START);
		EXPECT_EQ(along_q[0].segment_index1, 2u);
		EXPECT_EQ(along_q[1].type, Intersection::SEGMENT_START_ON_SEGMENT_START);
		EXPECT_EQ(along_q[1].segment_index1, 4u);
	}
}


//
// 9. A near-antipodal edge: its plane is still accurate enough.
//

TEST(GeometryIntersectTest, near_antipodal_edge)
{
	// Q's edge runs from the centre nearly to its antipode, through the frame's north direction.
	const UnitVector3D c = F.at(0, 0).position_vector();
	const UnitVector3D north = F.at(0, 1e-3).position_vector();
	const UnitVector3D n = (Vector3D(north) - dot(north, c).dval() * c).get_normalisation();
	const UnitVector3D e = cross(n, c).get_normalisation();
	const PointOnSphere far_end((-1.0 * Vector3D(c) + 2e-6 * n).get_normalisation());
	const auto q = polygon({ PointOnSphere(c), far_end, PointOnSphere((-1.0 * Vector3D(e)).get_normalisation()) });

	// A short arc of P crossing the long edge at n (90 degrees along it).
	const auto p = polyline({
			PointOnSphere((Vector3D(n) - 1e-3 * e).get_normalisation()),
			PointOnSphere((Vector3D(n) + 1e-3 * e).get_normalisation()) });
	Graph graph;
	ASSERT_TRUE(GPlatesMaths::GeometryIntersect::intersect(graph, *q, *p));
	ASSERT_EQ(graph.unordered_intersections.size(), 1u);
	const Intersection &x = graph.unordered_intersections[0];
	EXPECT_EQ(x.type, Intersection::SEGMENTS_CROSS);
	EXPECT_EQ(x.segment_index1, 0u);
	EXPECT_LE(angle_between(x.position, PointOnSphere(n)), 1e-9);
}


//
// 10. Nearly parallel arcs: the cross product of their planes is still accurate.
//

TEST(GeometryIntersectTest, near_parallel_crossing)
{
	// Q along the equator; P crossing it at lon 0.1 at an angle of about 2e-6 radians, with its
	// ends 2e-6 off the equator (beyond the thickness), symmetric about the crossing.
	const auto q = polyline({ lat_lon_radians(0, -1.2), lat_lon_radians(0, 1.2) });
	const auto p = polyline({ lat_lon_radians(-2e-6, 0.1 - 1.0), lat_lon_radians(2e-6, 0.1 + 1.0) });
	Graph graph;
	ASSERT_TRUE(GPlatesMaths::GeometryIntersect::intersect(graph, *q, *p));
	ASSERT_EQ(graph.unordered_intersections.size(), 1u);
	const Intersection &x = graph.unordered_intersections[0];
	EXPECT_EQ(x.type, Intersection::SEGMENTS_CROSS);
	EXPECT_LE(angle_between(x.position, lat_lon_radians(0, 0.1)), 1e-9);

	// Ends only 0.5e-6 off: within the thickness, so two touches (at P's first and last vertices).
	const auto p_touching = polyline({ lat_lon_radians(-0.5e-6, 0.1 - 1.0), lat_lon_radians(0.5e-6, 0.1 + 1.0) });
	Graph graph_touching;
	ASSERT_TRUE(GPlatesMaths::GeometryIntersect::intersect(graph_touching, *q, *p_touching));
	const std::vector<Intersection> along_p = ordered(graph_touching, false);
	ASSERT_EQ(along_p.size(), 2u);
	EXPECT_EQ(along_p[0].type, Intersection::SEGMENT2_START_ON_SEGMENT1);
	EXPECT_EQ(along_p[0].segment_index2, 0u);
	EXPECT_EQ(along_p[1].type, Intersection::SEGMENT2_START_ON_SEGMENT1);
	EXPECT_EQ(along_p[1].segment_index2, 1u);
}


//
// 11. Bounding-tree pruning finds every contact.
//

namespace
{
	std::vector<PointOnSphere>
	circle(
			const Frame &frame,
			double east,
			double north,
			double radius,
			unsigned int num_vertices)
	{
		std::vector<PointOnSphere> points;
		for (unsigned int i = 0; i < num_vertices; ++i)
		{
			const double theta = 2 * GPlatesMaths::PI * i / num_vertices;
			points.push_back(frame.at(east + radius * std::cos(theta), north + radius * std::sin(theta)));
		}
		return points;
	}
}

TEST(GeometryIntersectTest, pruning)
{
	const double radius = 0.1;
	const auto q = polygon(
			circle(F, 0, 0, radius, 200),
			{ circle(F, 0.05, 0, 0.02, 50), circle(F, -0.05, 0, 0.02, 50) });

	{
		// A single touch, 0.99 H outside the middle of one exterior edge, by a vertex of a long P
		// whose arms leave it at 45 degrees: found although nothing else is near.
		SCOPED_TRACE("one touch");
		const std::vector<PointOnSphere> ring = circle(F, 0, 0, radius, 200);
		// The midpoint of the edge from vertex 50 to vertex 51 (segment 50), and the directions there
		// away from the circle's centre and along the edge.
		const UnitVector3D mid = (Vector3D(ring[50].position_vector()) + Vector3D(ring[51].position_vector())).get_normalisation();
		const Vector3D away = Vector3D(mid) - Vector3D(F.at(0, 0).position_vector());
		const UnitVector3D tangent_out = (away - dot(away, mid).dval() * mid).get_normalisation();
		const UnitVector3D along = cross(mid, tangent_out).get_normalisation();
		const Vector3D x_vec = Vector3D(mid) + 0.99 * H * tangent_out;
		std::vector<PointOnSphere> p_points;
		for (int i = 50; i >= 1; --i)
		{
			p_points.push_back(PointOnSphere((x_vec + (i * 1e-3) * (Vector3D(tangent_out) - Vector3D(along))).get_normalisation()));
		}
		p_points.push_back(PointOnSphere(x_vec.get_normalisation()));
		for (int i = 1; i <= 50; ++i)
		{
			p_points.push_back(PointOnSphere((x_vec + (i * 1e-3) * (Vector3D(tangent_out) + Vector3D(along))).get_normalisation()));
		}
		const auto p = polyline(p_points);
		ASSERT_EQ(p->number_of_segments(), 100u);
		Graph graph;
		ASSERT_TRUE(GPlatesMaths::GeometryIntersect::intersect(graph, *q, *p));
		ASSERT_EQ(graph.unordered_intersections.size(), 1u);
		EXPECT_EQ(graph.unordered_intersections[0].type, Intersection::SEGMENT2_START_ON_SEGMENT1);
		EXPECT_EQ(graph.unordered_intersections[0].segment_index1, 50u);
		EXPECT_EQ(graph.unordered_intersections[0].segment_index2, 50u);
	}
	{
		// A zigzag of 100 segments in and out of the exterior ring: 100 crossings, whichever geometry
		// is first (so the tree descent splits each geometry's nodes in turn).
		SCOPED_TRACE("zigzag");
		std::vector<PointOnSphere> zigzag;
		for (unsigned int i = 0; i <= 100; ++i)
		{
			const double theta = 2 * GPlatesMaths::PI * (i + 0.37) / 101;
			const double r = (i % 2 == 0) ? radius - 0.01 : radius + 0.01;
			zigzag.push_back(F.at(r * std::cos(theta), r * std::sin(theta)));
		}
		const auto p = polyline(zigzag);
		Graph graph;
		ASSERT_TRUE(GPlatesMaths::GeometryIntersect::intersect(graph, *q, *p));
		EXPECT_EQ(graph.unordered_intersections.size(), 100u);
		for (const Intersection &x : graph.unordered_intersections)
		{
			EXPECT_LT(x.segment_index1, 200u);  // the exterior ring
		}
		Graph swapped;
		ASSERT_TRUE(GPlatesMaths::GeometryIntersect::intersect(swapped, *p, *q));
		EXPECT_EQ(swapped.unordered_intersections.size(), 100u);

		// A two-segment P (one leaf) against the many-leafed Q, both ways round.
		const auto p_short = polyline({ F.at(-0.2, 0.001), F.at(0, 0.002), F.at(0.2, 0.001) });
		Graph short_graph;
		ASSERT_TRUE(GPlatesMaths::GeometryIntersect::intersect(short_graph, *q, *p_short));
		Graph short_swapped;
		ASSERT_TRUE(GPlatesMaths::GeometryIntersect::intersect(short_swapped, *p_short, *q));
		// The exterior twice and each hole twice.
		EXPECT_EQ(short_graph.unordered_intersections.size(), 6u);
		EXPECT_EQ(short_swapped.unordered_intersections.size(), 6u);
	}
	{
		// Two straight polylines end to end on one great circle, 0.99 H apart: their end vertices
		// coincide. Each end vertex is its bounding circles' extreme point, so at every level of
		// both trees the circles are 0.99 H apart, and only their expansion by the thickness keeps
		// the pair from being pruned.
		SCOPED_TRACE("end to end");
		std::vector<PointOnSphere> q_points;
		std::vector<PointOnSphere> p_points;
		for (int i = 0; i <= 50; ++i)
		{
			q_points.push_back(F.at((i - 50) * 2e-3, 0));
			p_points.push_back(F.at(0.99 * H + i * 2e-3, 0));
		}
		const auto q_line = polyline(q_points);
		const auto p = polyline(p_points);
		Graph graph;
		ASSERT_TRUE(GPlatesMaths::GeometryIntersect::intersect(graph, *q_line, *p));
		ASSERT_EQ(graph.unordered_intersections.size(), 1u);
		EXPECT_EQ(graph.unordered_intersections[0].type, Intersection::SEGMENT_START_ON_SEGMENT_START);
		EXPECT_EQ(graph.unordered_intersections[0].segment_index1, 50u);  // Q's fictitious segment
		EXPECT_EQ(graph.unordered_intersections[0].segment_index2, 0u);
		expect_symmetric(*q_line, *p);
	}
}


//
// 12. A vertex in a corner's wedge: one touch record per edge, all kept (audit D3, documented
// behaviour; see the comment on Graph).
//

namespace
{
	//! A frame at the north-west corner of a 1 by 1 degree square of Q, lat -0.5 to 0.5, lon 0 to 1.
	const PointOnSphere CORNER = lat_lon_radians(0.5 * GPlatesMaths::PI / 180, 0);
	const Frame AT_CORNER(CORNER);

	PolygonOnSphere::non_null_ptr_to_const_type
	square_q()
	{
		const double d = GPlatesMaths::PI / 180;
		// Segment 2 runs west into the corner, and segment 3 runs south out of it.
		return polygon({
				lat_lon_radians(-0.5 * d, 0), lat_lon_radians(-0.5 * d, d), lat_lon_radians(0.5 * d, d), CORNER });
	}
}

TEST(GeometryIntersectTest, corner_wedge)
{
	const auto q = square_q();

	// A vertex x of P at (a east, a south) of the corner, inside it, with P arriving from the
	// north-west so that the corner lies on its arriving segment.
	for (const double a_over_h : { 0.5, 0.72, 0.99 })
	{
		SCOPED_TRACE(testing::Message() << "right angle, a = " << a_over_h << " H");
		const double a = a_over_h * H;
		const PointOnSphere x = AT_CORNER.at(a, -a);
		const auto p = polyline({ AT_CORNER.at(-0.01, 0.003), x, AT_CORNER.at(0.01, -0.003) });
		Graph graph;
		ASSERT_TRUE(GPlatesMaths::GeometryIntersect::intersect(graph, *q, *p));
		const std::vector<Intersection> along_p = ordered(graph, false);
		if (a_over_h < 0.7)
		{
			// Within the thickness of the corner: one SS.
			ASSERT_EQ(along_p.size(), 1u);
			EXPECT_EQ(along_p[0].type, Intersection::SEGMENT_START_ON_SEGMENT_START);
			EXPECT_EQ(along_p[0].segment_index1, 3u);
			continue;
		}
		// The corner on P's arriving segment, then x on each of the two edges.
		ASSERT_EQ(along_p.size(), 3u);
		EXPECT_EQ(along_p[0].type, Intersection::SEGMENT1_START_ON_SEGMENT2);
		EXPECT_EQ(along_p[0].segment_index1, 3u);
		EXPECT_EQ(along_p[0].segment_index2, 0u);
		for (unsigned int i = 1; i < 3; ++i)
		{
			EXPECT_EQ(along_p[i].type, Intersection::SEGMENT2_START_ON_SEGMENT1);
			EXPECT_EQ(along_p[i].segment_index2, 1u);
			EXPECT_EQ(along_p[i].position, x);
		}
		EXPECT_EQ(along_p[1].segment_index1 + along_p[2].segment_index1, 2u + 3u);
	}

	// A 30 degree corner of Q, opening south, with x on the bisector: inside the wedge (up to
	// H / sin(15 degrees) = 3.86 H) one touch per edge, both at x; beyond it, P cuts the corner.
	const double s15 = std::sin(15 * GPlatesMaths::PI / 180);
	const double c15 = std::cos(15 * GPlatesMaths::PI / 180);
	const auto q30 = polygon({
			CORNER, AT_CORNER.at(0.02 * s15, -0.02 * c15), AT_CORNER.at(0.03, -0.03), AT_CORNER.at(-0.02 * s15, -0.02 * c15) });
	for (const double r_over_h : { 1.2, 2.0, 3.8, 4.2 })
	{
		SCOPED_TRACE(testing::Message() << "30 degrees, r = " << r_over_h << " H");
		const PointOnSphere x = AT_CORNER.at(0, -r_over_h * H);
		const auto p = polyline({ AT_CORNER.at(-0.01, 0), x, AT_CORNER.at(0.01, 0) });
		Graph graph;
		ASSERT_TRUE(GPlatesMaths::GeometryIntersect::intersect(graph, *q30, *p));
		const std::vector<Intersection> along_p = ordered(graph, false);
		ASSERT_EQ(along_p.size(), 2u);
		const Intersection::Type expected = (r_over_h < 3.86)
				? Intersection::SEGMENT2_START_ON_SEGMENT1
				: Intersection::SEGMENTS_CROSS;
		for (const Intersection &touch : along_p)
		{
			EXPECT_EQ(touch.type, expected);
			if (expected == Intersection::SEGMENT2_START_ON_SEGMENT1)
			{
				EXPECT_EQ(touch.position, x);
			}
		}
		// One record on each of the corner's two edges (segment 0 out of it, segment 3 into it).
		EXPECT_EQ(along_p[0].segment_index1 + along_p[1].segment_index1, 0u + 3u);
	}
}


//
// 13. Symmetry, beyond the pairs checked in the cases above: a polygon against a polyline.
//

TEST(GeometryIntersectTest, symmetry_of_polygon_and_polyline)
{
	const double a = 0.01;
	const auto q = polygon({ F.at(-a, -a), F.at(a, -a), F.at(a, a), F.at(-a, a) });
	// A crossing, a touch by P's vertex, and a vertex shared bit for bit.
	const auto p = polyline({ F.at(-2 * a, 0), F.at(0, 0.5 * H - a), F.at(a, a), F.at(2 * a, 1.5 * a) });

	Graph qp;
	Graph pq;
	ASSERT_TRUE(GPlatesMaths::GeometryIntersect::intersect(qp, *q, *p));
	ASSERT_TRUE(GPlatesMaths::GeometryIntersect::intersect(pq, *p, *q));
	ASSERT_EQ(qp.unordered_intersections.size(), pq.unordered_intersections.size());
	const std::vector<Intersection> qp_along_p = ordered(qp, false);
	const std::vector<Intersection> pq_along_p = ordered(pq, true);
	for (unsigned int i = 0; i < qp_along_p.size(); ++i)
	{
		SCOPED_TRACE(testing::Message() << "intersection " << i);
		EXPECT_EQ(qp_along_p[i].type, mirrored(pq_along_p[i].type));
		EXPECT_EQ(qp_along_p[i].segment_index1, pq_along_p[i].segment_index2);
		EXPECT_EQ(qp_along_p[i].segment_index2, pq_along_p[i].segment_index1);
	}
}


//
// 14. No intersection, and a stale graph is cleared.
//

TEST(GeometryIntersectTest, no_intersection_clears_the_graph)
{
	const double a = 0.01;
	const auto p = polyline({ F.at(-a, 0), F.at(a, 0) });
	const auto crossing = polyline({ F.at(0, -a), F.at(0, a) });
	const auto apart = polyline({ F.at(-a, 0.5 * a), F.at(a, 0.5 * a) });

	Graph graph;
	ASSERT_TRUE(GPlatesMaths::GeometryIntersect::intersect(graph, *p, *crossing));
	ASSERT_FALSE(graph.empty());

	EXPECT_FALSE(GPlatesMaths::GeometryIntersect::intersect(graph, *p, *apart));
	EXPECT_TRUE(graph.empty());
	EXPECT_TRUE(graph.geometry1_ordered_intersections.empty());
	EXPECT_TRUE(graph.geometry2_ordered_intersections.empty());
}


//
// 15. A twist of P on an edge of Q: two crossings at one point, adjacent along Q.
//

TEST(GeometryIntersectTest, twist_on_an_edge)
{
	const double a = 0.01;
	const auto q = polyline({ F.at(-a, 0), F.at(a, 0) });
	// The bow-tie A, D, C, B: its edges A-D and C-B cross at the centre, on Q's edge.
	const auto p = polygon({ F.at(-0.5 * a, 0.5 * a), F.at(0.5 * a, -0.5 * a), F.at(0.5 * a, 0.5 * a), F.at(-0.5 * a, -0.5 * a) });

	Graph graph;
	ASSERT_TRUE(GPlatesMaths::GeometryIntersect::intersect(graph, *q, *p));
	const std::vector<Intersection> along_q = ordered(graph, true);
	ASSERT_EQ(along_q.size(), 4u);
	for (const Intersection &x : along_q)
	{
		EXPECT_EQ(x.type, Intersection::SEGMENTS_CROSS);
	}
	// The two middle crossings are A-D (segment 0) and C-B (segment 2), at the centre.
	EXPECT_EQ(along_q[1].segment_index2 + along_q[2].segment_index2, 0u + 2u);
	EXPECT_LE(angle_between(along_q[1].position, F.at(0, 0)), 1e-12);
	EXPECT_LE(angle_between(along_q[2].position, F.at(0, 0)), 1e-12);
	EXPECT_NEAR(along_q[1].angle_in_segment1.calculate_angle().dval(), along_q[2].angle_in_segment1.calculate_angle().dval(), 1e-12);
}


//
// 16. The constants the code relies on.
//

TEST(GeometryIntersectTest, constants)
{
	// The thickness is at least the longest a zero-length arc can be, so a zero-length segment
	// cannot let another segment tunnel between its end points.
	EXPECT_LE(
			Intersection::get_on_segment_start_threshold_cosine(),
			GPlatesMaths::GreatCircleArc::get_zero_length_threshold_cosine().dval());

	// The square of the thickness exceeds EPSILON, so the cross product of two crossing segments'
	// planes is never zero as a real_t (add_segments_crossing_intersection relies on it).
	EXPECT_GT(H * H, GPlatesMaths::EPSILON);

	// The thickness is point-in-polygon's point-on-outline threshold (fix F1).
	EXPECT_EQ(
			Intersection::get_on_segment_start_threshold_cosine(),
			GPlatesMaths::PointInPolygon::get_point_on_polygon_threshold_cosine());
}
