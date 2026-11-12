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
#include <memory>
#include <vector>
#include <boost/optional/optional_io.hpp>
#include <gtest/gtest.h>

#include "app-logic/GeometryCookieCutter.h"
#include "app-logic/GeometryUtils.h"
#include "app-logic/ReconstructMethodRegistry.h"
#include "app-logic/ReconstructionGeometryUtils.h"
#include "app-logic/ReconstructionTreeCreator.h"

#include "maths/GeometryDistance.h"
#include "maths/GeometryIntersect.h"
#include "maths/LatLonPoint.h"
#include "maths/MathsUtils.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolygonPartitioner.h"
#include "maths/PolylineOnSphere.h"
#include "maths/SmallCircleBounds.h"
#include "maths/UnitVector3D.h"
#include "maths/Vector3D.h"

#include "model/FeatureCollectionHandle.h"
#include "model/FeatureHandle.h"
#include "model/FeatureType.h"
#include "model/PropertyName.h"
#include "model/TopLevelPropertyInline.h"

#include "property-values/GpmlPlateId.h"

#include "unit-test/PolygonOverlayOracle.h"


// The partition layer (spherical-geometry.md section 5.1): PolygonPartitioner on the overlay, its
// result contract with or without lists, and the cascade of GeometryCookieCutter with provenance
// (section 5.4). Geometries are boxes in degrees, edges along meridians and great circles.
namespace
{
	using GPlatesMaths::GeometryOnSphere;
	using GPlatesMaths::MultiPointOnSphere;
	using GPlatesMaths::PointOnSphere;
	using GPlatesMaths::PolygonOnSphere;
	using GPlatesMaths::PolygonPartitioner;
	using GPlatesMaths::PolylineOnSphere;
	namespace Overlay = GPlatesMaths::PolygonOverlay;

	typedef std::vector<PointOnSphere> ring_type;
	typedef GPlatesAppLogic::GeometryCookieCutter CookieCutter;

	//! The thickness, as a sine (a distance in radians, for these small distances).
	const double H =
			GPlatesMaths::GeometryIntersect::Intersection::get_on_segment_start_threshold_sine();

	//! Areas agree to rounding: every output vertex is an input vertex or a computed crossing.
	const double AREA_TOLERANCE = 1e-12;


	PointOnSphere
	at(
			double lat,
			double lon)
	{
		return GPlatesMaths::make_point_on_sphere(GPlatesMaths::LatLonPoint(lat, lon));
	}

	//! A box, anticlockwise seen from outside the sphere (or clockwise, @a reversed).
	ring_type
	box(
			double lat_0,
			double lat_1,
			double lon_0,
			double lon_1,
			bool reversed = false)
	{
		ring_type ring{ at(lat_0, lon_0), at(lat_0, lon_1), at(lat_1, lon_1), at(lat_1, lon_0) };
		if (reversed)
		{
			std::reverse(ring.begin(), ring.end());
		}
		return ring;
	}

	PolygonOnSphere::non_null_ptr_to_const_type
	polygon(
			const ring_type &exterior,
			const std::vector<ring_type> &interiors = std::vector<ring_type>())
	{
		return PolygonOnSphere::create(exterior, interiors.begin(), interiors.end());
	}

	std::vector<ring_type>
	rings_of(
			const PolygonOnSphere &polygon_on_sphere)
	{
		std::vector<ring_type> rings{ ring_type(
				polygon_on_sphere.exterior_ring_vertex_begin(),
				polygon_on_sphere.exterior_ring_vertex_end()) };
		for (unsigned int i = 0; i < polygon_on_sphere.number_of_interior_rings(); ++i)
		{
			rings.push_back(ring_type(
					polygon_on_sphere.interior_ring_vertex_begin(i),
					polygon_on_sphere.interior_ring_vertex_end(i)));
		}
		return rings;
	}

	//! A geometry's points, in GeometryUtils::get_geometry_points order.
	ring_type
	points_of(
			const GeometryOnSphere &geometry)
	{
		ring_type points;
		GPlatesAppLogic::GeometryUtils::get_geometry_points(geometry, points);
		return points;
	}

	//! Whether two point sequences are the same stored positions, coordinate for coordinate.
	bool
	same_points(
			const ring_type &a,
			const ring_type &b)
	{
		if (a.size() != b.size())
		{
			return false;
		}
		for (unsigned int i = 0; i < a.size(); ++i)
		{
			if (!PolygonOverlayOracle::same_position(a[i], b[i]))
			{
				return false;
			}
		}
		return true;
	}

	double
	total_area(
			const PolygonPartitioner::partitioned_polygon_seq_type &polygons)
	{
		double area = 0.0;
		for (const PolygonOnSphere::non_null_ptr_to_const_type &p : polygons)
		{
			area += p->get_area().dval();
		}
		return area;
	}

	bool
	contains(
			const PolygonPartitioner::partitioned_polygon_seq_type &polygons,
			const PointOnSphere &point)
	{
		for (const PolygonOnSphere::non_null_ptr_to_const_type &p : polygons)
		{
			if (p->is_point_in_polygon(point, PolygonOnSphere::LOW_SPEED_NO_SETUP_NO_MEMORY_USAGE))
			{
				return true;
			}
		}
		return false;
	}


	/**
	 * Partition @a subject by @a partitioner (lists of polygons), and check that every other form
	 * gives the same: no list (the result-only path), one list, and the geometry-level entry (the
	 * same pieces, point for point).
	 */
	PolygonPartitioner::Result
	partition(
			const PolygonPartitioner &partitioner,
			const PolygonOnSphere::non_null_ptr_to_const_type &subject,
			PolygonPartitioner::partitioned_polygon_seq_type &inside,
			PolygonPartitioner::partitioned_polygon_seq_type &outside)
	{
		const PolygonPartitioner::Result result =
				partitioner.partition_polygon(subject, inside, outside);
		EXPECT_EQ(partitioner.partition_polygon(subject), result) << "no list";

		PolygonPartitioner::partitioned_polygon_seq_type inside_only, outside_only;
		EXPECT_EQ(partitioner.partition_polygon(subject, inside_only, boost::none), result);
		EXPECT_EQ(partitioner.partition_polygon(subject, boost::none, outside_only), result);
		EXPECT_EQ(inside_only.size(), inside.size()) << "the inside list only";
		EXPECT_EQ(outside_only.size(), outside.size()) << "the outside list only";

		PolygonPartitioner::partitioned_geometry_seq_type geometries_inside;
		PolygonPartitioner::partitioned_geometry_seq_type geometries_outside;
		EXPECT_EQ(
				partitioner.partition_geometry(subject, geometries_inside, geometries_outside),
				result);
		EXPECT_EQ(partitioner.partition_geometry(subject), result) << "no list (geometry)";
		const auto compare = [](
				const PolygonPartitioner::partitioned_polygon_seq_type &polygons,
				const PolygonPartitioner::partitioned_geometry_seq_type &geometries)
		{
			ASSERT_EQ(geometries.size(), polygons.size());
			auto polygon_iter = polygons.begin();
			for (const PolygonPartitioner::PartitionedGeometry &geometry : geometries)
			{
				EXPECT_TRUE(same_points(points_of(*geometry.geometry), points_of(**polygon_iter)));
				EXPECT_FALSE(geometry.provenance);
				++polygon_iter;
			}
		};
		compare(inside, geometries_inside);
		compare(outside, geometries_outside);
		return result;
	}


	/**
	 * A band along a helix about the z axis whose interior reaches past its outline's bounding
	 * circle: the antipode of its boundary centroid is inside it (as in PointInPolygonTest's
	 * create_helix_band, which says more).
	 */
	PolygonOnSphere::non_null_ptr_to_const_type
	create_helix_band()
	{
		const double width = 0.065;
		const double sweep = 576 * GPlatesMaths::PI / 180;
		const auto point_at = [](double colatitude, double longitude)
		{
			return PointOnSphere(GPlatesMaths::UnitVector3D(
					std::sin(colatitude) * std::cos(longitude),
					std::sin(colatitude) * std::sin(longitude),
					std::cos(colatitude)));
		};
		const auto centre_colatitude = [](double fraction) { return 0.779 + 1.579 * fraction; };
		const unsigned int num_points_per_side = 54;
		ring_type points;
		for (unsigned int k = 0; k < num_points_per_side; ++k)
		{
			const double fraction = double(k) / (num_points_per_side - 1);
			points.push_back(point_at(centre_colatitude(fraction) + width / 2, sweep * fraction));
		}
		for (unsigned int k = num_points_per_side; k-- > 0; )
		{
			const double fraction = double(k) / (num_points_per_side - 1);
			points.push_back(point_at(centre_colatitude(fraction) - width / 2, sweep * fraction));
		}
		return PolygonOnSphere::create(points);
	}
}


// Doc 5.1's first no-intersection bug: a partitioning polygon wholly inside the input put the whole
// input outside. Now the inside is the partitioner's region and the outside the input with a hole.
TEST(PolygonPartitionerTest, PartitionerInsideSubject)
{
	const auto P = polygon(box(-10, 10, 0, 20));
	const auto Q = polygon(box(-3, 3, 5, 10));
	const PolygonPartitioner::non_null_ptr_type partitioner = PolygonPartitioner::create(Q);

	PolygonPartitioner::partitioned_polygon_seq_type inside, outside;
	EXPECT_EQ(
			partition(*partitioner, P, inside, outside),
			PolygonPartitioner::GEOMETRY_INTERSECTING);
	ASSERT_EQ(inside.size(), 1u);
	EXPECT_EQ(inside.front()->number_of_interior_rings(), 0u);
	EXPECT_NEAR(total_area(inside), Q->get_area().dval(), AREA_TOLERANCE);
	ASSERT_EQ(outside.size(), 1u);
	EXPECT_EQ(outside.front()->number_of_interior_rings(), 1u);
	EXPECT_NEAR(total_area(outside), P->get_area().dval() - Q->get_area().dval(), AREA_TOLERANCE);
	EXPECT_TRUE(contains(inside, at(0, 7)));
	EXPECT_FALSE(contains(outside, at(0, 7)));
	EXPECT_TRUE(contains(outside, at(0, 15)));
}


// Doc 5.1's second bug: an input crossing only a hole of the partitioning polygon was decided
// wholesale by one vertex. Now the hole's outline cuts it, polygon and polyline.
TEST(PolygonPartitionerTest, SubjectAcrossPartitionerHole)
{
	const auto Q = polygon(box(-15, 15, 0, 30), { box(-5, 5, 10, 20) });
	const PolygonPartitioner::non_null_ptr_type partitioner = PolygonPartitioner::create(Q);

	const auto P = polygon(box(-2, 2, 15, 25));
	PolygonPartitioner::partitioned_polygon_seq_type inside, outside;
	EXPECT_EQ(
			partition(*partitioner, P, inside, outside),
			PolygonPartitioner::GEOMETRY_INTERSECTING);
	ASSERT_EQ(inside.size(), 1u);
	ASSERT_EQ(outside.size(), 1u);
	EXPECT_TRUE(contains(inside, at(0, 22)));
	EXPECT_TRUE(contains(outside, at(0, 17)));
	EXPECT_NEAR(total_area(inside) + total_area(outside), P->get_area().dval(), AREA_TOLERANCE);

	const auto L = PolylineOnSphere::create(ring_type{ at(0, 15), at(0, 25) });
	PolygonPartitioner::partitioned_polyline_seq_type inside_lines, outside_lines;
	EXPECT_EQ(
			partitioner->partition_polyline(L, inside_lines, outside_lines),
			PolygonPartitioner::GEOMETRY_INTERSECTING);
	EXPECT_EQ(partitioner->partition_polyline(L), PolygonPartitioner::GEOMETRY_INTERSECTING);
	ASSERT_EQ(inside_lines.size(), 1u);
	ASSERT_EQ(outside_lines.size(), 1u);
	EXPECT_EQ(*inside_lines.front()->vertex_begin(), at(0, 20));
	EXPECT_EQ(*outside_lines.front()->vertex_begin(), at(0, 15));
}


// Doc 5.1's third bug: an input with a hole was wholly outside a partitioner crossing only that
// hole's outline. Now the partitioner cuts the input's ring around the hole.
TEST(PolygonPartitionerTest, PartitionerAcrossSubjectHole)
{
	const auto P = polygon(box(-15, 15, 0, 30), { box(-5, 5, 10, 20) });
	const auto Q = polygon(box(-2, 2, 15, 25));
	const PolygonPartitioner::non_null_ptr_type partitioner = PolygonPartitioner::create(Q);

	PolygonPartitioner::partitioned_polygon_seq_type inside, outside;
	EXPECT_EQ(
			partition(*partitioner, P, inside, outside),
			PolygonPartitioner::GEOMETRY_INTERSECTING);
	ASSERT_EQ(inside.size(), 1u);
	EXPECT_TRUE(contains(inside, at(0, 22)));
	EXPECT_FALSE(contains(inside, at(0, 17)));
	ASSERT_EQ(outside.size(), 1u);
	EXPECT_EQ(outside.front()->number_of_interior_rings(), 1u);  // the hole and the cut, merged
	EXPECT_FALSE(contains(outside, at(0, 17)));
	EXPECT_FALSE(contains(outside, at(0, 22)));
	EXPECT_TRUE(contains(outside, at(10, 22)));
	EXPECT_NEAR(total_area(inside) + total_area(outside), P->get_area().dval(), AREA_TOLERANCE);
}


// Doc 5.1's fourth bug: a piece along the partitioner's boundary was classified by the
// partitioner's orientation. Now a polygon beside the partitioner, sharing an edge, is outside, one
// inside it sharing an edge is inside, and a polyline along its edge is inside (section 5.2), for
// both orientations.
TEST(PolygonPartitionerTest, AlongBoundaryBothOrientations)
{
	for (bool reversed : { false, true })
	{
		SCOPED_TRACE(reversed);
		const auto Q = polygon(box(-10, 10, 0, 10, reversed));
		const PolygonPartitioner::non_null_ptr_type partitioner = PolygonPartitioner::create(Q);

		const auto beside = polygon(box(-10, 10, 10, 20));
		PolygonPartitioner::partitioned_polygon_seq_type inside, outside;
		EXPECT_EQ(
				partition(*partitioner, beside, inside, outside),
				PolygonPartitioner::GEOMETRY_OUTSIDE);
		EXPECT_TRUE(inside.empty());
		ASSERT_EQ(outside.size(), 1u);
		EXPECT_EQ(outside.front().get(), beside.get());  // the input itself

		const auto within = polygon(box(-3, 3, 5, 10));
		inside.clear();
		outside.clear();
		EXPECT_EQ(
				partition(*partitioner, within, inside, outside),
				PolygonPartitioner::GEOMETRY_INSIDE);
		ASSERT_EQ(inside.size(), 1u);
		EXPECT_EQ(inside.front().get(), within.get());
		EXPECT_TRUE(outside.empty());

		const auto along = PolylineOnSphere::create(ring_type{ at(-5, 10), at(5, 10) });
		PolygonPartitioner::partitioned_polyline_seq_type inside_lines, outside_lines;
		EXPECT_EQ(
				partitioner->partition_polyline(along, inside_lines, outside_lines),
				PolygonPartitioner::GEOMETRY_INSIDE);
		ASSERT_EQ(inside_lines.size(), 1u);
		EXPECT_EQ(inside_lines.front().get(), along.get());
		EXPECT_TRUE(outside_lines.empty());
	}
}


// The result contract (section 5.1): a geometry that only touches the
// partitioner, from either side, or is identical to it, comes back itself, inside or outside.
TEST(PolygonPartitionerTest, TouchesComeBackWhole)
{
	const auto Q = polygon(box(-10, 10, 0, 10));
	const PolygonPartitioner::non_null_ptr_type partitioner = PolygonPartitioner::create(Q);

	struct Case
	{
		const char *name;
		PolygonOnSphere::non_null_ptr_to_const_type subject;
		PolygonPartitioner::Result result;
	};
	const Case cases[] = {
		// A triangle whose apex is on Q's edge, from outside and from inside.
		{ "touch from outside", polygon(ring_type{ at(0, 10), at(-5, 15), at(5, 15) }),
				PolygonPartitioner::GEOMETRY_OUTSIDE },
		{ "touch from inside", polygon(ring_type{ at(0, 10), at(5, 5), at(-5, 5) }),
				PolygonPartitioner::GEOMETRY_INSIDE },
		// A triangle whose apex is Q's corner, from outside.
		{ "corner from outside", polygon(ring_type{ at(10, 10), at(15, 12), at(12, 15) }),
				PolygonPartitioner::GEOMETRY_OUTSIDE },
		{ "identical", polygon(box(-10, 10, 0, 10)), PolygonPartitioner::GEOMETRY_INSIDE },
		{ "identical, reversed", polygon(box(-10, 10, 0, 10, true)),
				PolygonPartitioner::GEOMETRY_INSIDE },
	};
	for (const Case &c : cases)
	{
		SCOPED_TRACE(c.name);
		PolygonPartitioner::partitioned_polygon_seq_type inside, outside;
		EXPECT_EQ(partition(*partitioner, c.subject, inside, outside), c.result);
		const bool is_inside = (c.result == PolygonPartitioner::GEOMETRY_INSIDE);
		ASSERT_EQ((is_inside ? inside : outside).size(), 1u);
		EXPECT_EQ((is_inside ? inside : outside).front().get(), c.subject.get());
		EXPECT_TRUE((is_inside ? outside : inside).empty());
	}

	// A polyline touching from outside (its vertex on Q's edge) is not split there.
	const auto L = PolylineOnSphere::create(ring_type{ at(-5, 15), at(0, 10), at(5, 15) });
	PolygonPartitioner::partitioned_polyline_seq_type inside_lines, outside_lines;
	EXPECT_EQ(
			partitioner->partition_polyline(L, inside_lines, outside_lines),
			PolygonPartitioner::GEOMETRY_OUTSIDE);
	EXPECT_TRUE(inside_lines.empty());
	ASSERT_EQ(outside_lines.size(), 1u);
	EXPECT_EQ(outside_lines.front().get(), L.get());

	// A multi-point and a point wholly on one side come back themselves too.
	const auto M = MultiPointOnSphere::create(ring_type{ at(0, 5), at(5, 5) });
	const GeometryOnSphere::non_null_ptr_to_const_type point =
			GPlatesMaths::PointGeometryOnSphere::create(at(0, 20));
	for (const GeometryOnSphere::non_null_ptr_to_const_type &geometry :
			{ GeometryOnSphere::non_null_ptr_to_const_type(M), point })
	{
		PolygonPartitioner::partitioned_geometry_seq_type inside, outside;
		const PolygonPartitioner::Result result =
				partitioner->partition_geometry(geometry, inside, outside);
		const bool is_inside = (geometry.get() == M.get());
		EXPECT_EQ(
				result,
				is_inside
						? PolygonPartitioner::GEOMETRY_INSIDE
						: PolygonPartitioner::GEOMETRY_OUTSIDE);
		ASSERT_EQ((is_inside ? inside : outside).size(), 1u);
		EXPECT_EQ((is_inside ? inside : outside).front().geometry.get(), geometry.get());
		EXPECT_TRUE((is_inside ? outside : inside).empty());
	}
}


// The result-only path (section 5.1), GLReconstructedStaticPolygonMeshes' call:
// far apart (the bounding circles alone), crossing, a partitioner inside the input (which that
// caller used to work round itself) and an input inside the partitioner. The other tests compare it
// with the lists' result on every input.
TEST(PolygonPartitionerTest, ResultOnlyPath)
{
	const auto Q = polygon(box(-10, 10, 0, 10));
	const PolygonPartitioner::non_null_ptr_type partitioner = PolygonPartitioner::create(Q);

	EXPECT_EQ(
			partitioner->partition_polygon(polygon(box(40, 50, 100, 110))),
			PolygonPartitioner::GEOMETRY_OUTSIDE);
	EXPECT_EQ(
			partitioner->partition_polygon(polygon(box(-5, 5, 5, 15))),
			PolygonPartitioner::GEOMETRY_INTERSECTING);
	EXPECT_EQ(
			partitioner->partition_polygon(polygon(box(-20, 20, -10, 20))),
			PolygonPartitioner::GEOMETRY_INTERSECTING);
	EXPECT_EQ(
			partitioner->partition_polygon(polygon(box(-5, 5, 2, 8))),
			PolygonPartitioner::GEOMETRY_INSIDE);
	EXPECT_EQ(
			partitioner->partition_geometry(polygon(box(-5, 5, 2, 8))),
			PolygonPartitioner::GEOMETRY_INSIDE);
}


// The bounding-circle shortcut (section 5.1) for a polygon whose
// interior reaches past its outline's bounding circle: a small box in that part of its interior has
// a bounding circle disjoint from its own, yet the two overlap, whichever partitions the other.
TEST(PolygonPartitionerTest, InteriorBeyondBoundingCircle)
{
	const auto band = create_helix_band();
	const GPlatesMaths::LatLonPoint centre =
			GPlatesMaths::make_lat_lon_point(PointOnSphere(-band->get_boundary_centroid()));
	const auto small = polygon(box(
			centre.latitude() - 0.5,
			centre.latitude() + 0.5,
			centre.longitude() - 0.5,
			centre.longitude() + 0.5));
	ASSERT_FALSE(GPlatesMaths::intersect(
			band->get_bounding_small_circle(),
			small->get_bounding_small_circle()));
	ASSERT_FALSE(band->is_outside_bounding_small_circle_outside_polygon(
			band->get_bounding_small_circle()));

	// The box partitions the band: the band surrounds it.
	PolygonPartitioner::partitioned_polygon_seq_type inside, outside;
	EXPECT_EQ(
			partition(*PolygonPartitioner::create(small), band, inside, outside),
			PolygonPartitioner::GEOMETRY_INTERSECTING);
	ASSERT_EQ(inside.size(), 1u);
	EXPECT_NEAR(total_area(inside), small->get_area().dval(), AREA_TOLERANCE);

	// The band partitions the box: the box is inside it.
	inside.clear();
	outside.clear();
	EXPECT_EQ(
			partition(*PolygonPartitioner::create(band), small, inside, outside),
			PolygonPartitioner::GEOMETRY_INSIDE);
	ASSERT_EQ(inside.size(), 1u);
	EXPECT_EQ(inside.front().get(), small.get());
}


// The other geometry types through the geometry-level entry, with provenance: a multi-point split
// into one multi-point each side, each point's subject location its index in the input.
TEST(PolygonPartitionerTest, MultiPointProvenance)
{
	const auto Q = polygon(box(-10, 10, 0, 10));
	const PolygonPartitioner::non_null_ptr_type partitioner = PolygonPartitioner::create(Q);
	const ring_type points{ at(0, 5), at(0, 20), at(5, 5), at(0, -5) };
	const auto M = MultiPointOnSphere::create(points);

	PolygonPartitioner::partitioned_geometry_seq_type inside, outside;
	EXPECT_EQ(
			partitioner->partition_geometry(
					PolygonPartitioner::PartitionedGeometry(M),
					inside,
					outside,
					PolygonPartitioner::CascadeProvenance()),
			PolygonPartitioner::GEOMETRY_INTERSECTING);
	ASSERT_EQ(inside.size(), 1u);
	ASSERT_EQ(outside.size(), 1u);
	for (const PolygonPartitioner::PartitionedGeometry *partitioned :
			{ &inside.front(), &outside.front() })
	{
		ASSERT_TRUE(partitioned->provenance);
		const ring_type partitioned_points = points_of(*partitioned->geometry);
		ASSERT_EQ(partitioned->provenance->size(), partitioned_points.size());
		for (unsigned int i = 0; i < partitioned_points.size(); ++i)
		{
			const Overlay::Provenance &provenance = (*partitioned->provenance)[i];
			ASSERT_TRUE(provenance.subject);
			EXPECT_EQ(points.at(provenance.subject->index), partitioned_points[i]);
		}
	}
}


// A piece keeping two holes, with provenance: each hole's points named by their ring and index
// (the table's interiors in the polygon's order), every point resolving.
TEST(PolygonPartitionerTest, TwoHolesProvenance)
{
	const auto P = polygon(box(-10, 10, 0, 30), { box(-3, 3, 3, 6), box(-3, 3, 8, 12) });
	const auto Q = polygon(box(-20, 20, 15, 40));
	const PolygonPartitioner::non_null_ptr_type partitioner = PolygonPartitioner::create(Q);

	PolygonPartitioner::partitioned_geometry_seq_type inside, outside;
	EXPECT_EQ(
			partitioner->partition_geometry(
					PolygonPartitioner::PartitionedGeometry(P),
					inside,
					outside,
					PolygonPartitioner::CascadeProvenance()),
			PolygonPartitioner::GEOMETRY_INTERSECTING);
	ASSERT_EQ(outside.size(), 1u);
	const auto piece = GPlatesUtils::dynamic_pointer_cast<const PolygonOnSphere>(
			outside.front().geometry);
	ASSERT_EQ(piece->number_of_interior_rings(), 2u);
	ASSERT_TRUE(outside.front().provenance);

	const std::vector<ring_type> subject_rings = rings_of(*P);
	const ring_type points = points_of(*piece);
	EXPECT_EQ(
			PolygonOverlayOracle::count_provenance_failures(
					subject_rings,
					{ rings_of(*Q) },
					points,
					*outside.front().provenance,
					2 * H),
			0u);
	unsigned int num_hole_vertices = 0;
	for (unsigned int i = 0; i < points.size(); ++i)
	{
		const Overlay::Provenance &provenance = (*outside.front().provenance)[i];
		ASSERT_TRUE(provenance.subject);
		for (unsigned int r = 1; r < subject_rings.size(); ++r)
		{
			for (unsigned int j = 0; j < subject_rings[r].size(); ++j)
			{
				if (PolygonOverlayOracle::same_position(subject_rings[r][j], points[i]))
				{
					EXPECT_EQ(provenance.subject->ring, r);
					EXPECT_EQ(provenance.subject->index, j);
					++num_hole_vertices;
				}
			}
		}
	}
	EXPECT_EQ(num_hole_vertices, 8u);
}


namespace
{
	//! A feature with a reconstruction plate ID and a polygon (no rotations: present day).
	void
	add_plate(
			const GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesModel::integer_plate_id_type plate_id,
			const PolygonOnSphere::non_null_ptr_to_const_type &boundary)
	{
		const GPlatesModel::FeatureHandle::weak_ref feature = GPlatesModel::FeatureHandle::create(
				collection,
				GPlatesModel::FeatureType::create_gpml("UnclassifiedFeature"));
		feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
						GPlatesModel::PropertyName::create_gpml("reconstructionPlateId"),
						GPlatesPropertyValues::GpmlPlateId::create(plate_id)));
		feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
						GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"),
						GPlatesAppLogic::GeometryUtils::create_geometry_property_value(boundary)));
	}


	/**
	 * Three plates as a cookie cutter, in order (plate IDs sorted highest first): plate A, a far
	 * plate, then plate B sharing A's eastern edge.
	 */
	class ThreePlates
	{
	public:
		ThreePlates() :
			plate_a(polygon(box(-10, 10, 0, 10))),
			plate_far(polygon(box(40, 50, 100, 110))),
			plate_b(polygon(box(-10, 10, 10, 20))),
			d_plates(GPlatesModel::FeatureCollectionHandle::create()),
			d_rotations(GPlatesModel::FeatureCollectionHandle::create())
		{
			add_plate(d_plates->reference(), 3, plate_a);
			add_plate(d_plates->reference(), 2, plate_far);
			add_plate(d_plates->reference(), 1, plate_b);
		}

		std::vector<std::vector<ring_type>>
		clip_rings() const
		{
			return { rings_of(*plate_a), rings_of(*plate_far), rings_of(*plate_b) };
		}

		//! The cookie cutter of the three plates, or of none, constructed with its defaults.
		std::unique_ptr<CookieCutter>
		create_cookie_cutter(
				bool with_plates = true) const
		{
			return std::unique_ptr<CookieCutter>(new CookieCutter(
					0.0,
					d_registry,
					std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>(
							1,
							(with_plates ? d_plates : d_rotations)->reference()),
					get_reconstruction_tree_creator()));
		}

		//! The cookie cutter of the three plates with the sliver filter at @a sliver_min_width.
		std::unique_ptr<CookieCutter>
		create_cookie_cutter_with_sliver_min_width(
				boost::optional<double> sliver_min_width) const
		{
			return std::unique_ptr<CookieCutter>(new CookieCutter(
					0.0,
					d_registry,
					std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>(
							1,
							d_plates->reference()),
					get_reconstruction_tree_creator(),
					true/*group_networks_then_boundaries_then_static_polygons*/,
					CookieCutter::SORT_BY_PLATE_ID,
					PolygonOnSphere::ADAPTIVE,
					sliver_min_width));
		}

		GPlatesAppLogic::ReconstructionTreeCreator
		get_reconstruction_tree_creator() const
		{
			return GPlatesAppLogic::create_cached_reconstruction_tree_creator(
					std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>(
							1,
							d_rotations->reference()));
		}

		const PolygonOnSphere::non_null_ptr_to_const_type plate_a;
		const PolygonOnSphere::non_null_ptr_to_const_type plate_far;
		const PolygonOnSphere::non_null_ptr_to_const_type plate_b;

	private:
		GPlatesModel::FeatureCollectionHandle::non_null_ptr_type d_plates;
		GPlatesModel::FeatureCollectionHandle::non_null_ptr_type d_rotations;
		const GPlatesAppLogic::ReconstructMethodRegistry d_registry;
	};


	//! A partitioned geometry, and its partitioning plate's index in the cascade (outside: none).
	typedef std::pair<const CookieCutter::PartitionedGeometry *, boost::optional<unsigned int>>
			piece_type;

	/**
	 * The pieces of a cookie cutter's partition, by partition then the outside ones last, with
	 * the partitioning plate's index in the cascade for each (outside: none).
	 */
	std::vector<piece_type>
	get_pieces(
			const CookieCutter::partition_seq_type &inside,
			const CookieCutter::partitioned_geometry_seq_type &outside,
			const ThreePlates &plates)
	{
		std::vector<piece_type> pieces;
		for (const CookieCutter::Partition &partition : inside)
		{
			const PolygonOnSphere::non_null_ptr_to_const_type boundary =
					*GPlatesAppLogic::ReconstructionGeometryUtils::get_boundary_polygon(
							partition.reconstruction_geometry);
			unsigned int index = 0;
			for (const auto &plate : { plates.plate_a, plates.plate_far, plates.plate_b })
			{
				if (same_points(points_of(*plate), points_of(*boundary)))
				{
					break;
				}
				++index;
			}
			for (const CookieCutter::PartitionedGeometry &geometry :
					partition.partitioned_geometries)
			{
				pieces.push_back(std::make_pair(&geometry, boost::optional<unsigned int>(index)));
			}
		}
		for (const CookieCutter::PartitionedGeometry &geometry : outside)
		{
			pieces.push_back(std::make_pair(&geometry, boost::none));
		}
		return pieces;
	}
}


// The cascade (sections 5.1 and 5.4): two plates sharing an edge, and an input
// polygon with a hole across that edge, partly outside both. Each plate's piece is a polygon (the
// hole cut open by the shared edge), the rest one polygon outside; the pieces cover the input; and
// with provenance every point's locations resolve to it, on the original input and on the plate
// that cut it, through the second plate's cut of the first's outside piece. A far plate between
// them in the cascade returns that piece whole, which must keep its table.
TEST(PolygonPartitionerTest, CookieCutterCascade)
{
	const ThreePlates plates;
	const std::unique_ptr<CookieCutter> cookie_cutter = plates.create_cookie_cutter();
	ASSERT_TRUE(cookie_cutter->has_partitioning_polygons());

	// Each ring starts at its bottom-right corner, so that its closing segment is its bottom edge,
	// across both plates: composing the second plate's cut needs the wrap-around (vertex 0 ends
	// the last segment), which the original input gives.
	const auto starting_bottom_right = [](ring_type ring)
	{
		std::rotate(ring.begin(), ring.begin() + 1, ring.end());
		return ring;
	};
	const auto P = polygon(
			starting_bottom_right(box(-5, 5, 5, 25)),
			{ starting_bottom_right(box(-2, 2, 8, 12)) });
	const std::vector<ring_type> subject_rings = rings_of(*P);
	const std::vector<std::vector<ring_type>> clip_rings = plates.clip_rings();

	for (bool want_provenance : { false, true })
	{
		SCOPED_TRACE(want_provenance);
		CookieCutter::partition_seq_type inside;
		CookieCutter::partitioned_geometry_seq_type outside;
		EXPECT_TRUE(cookie_cutter->partition_geometry(P, inside, outside, want_provenance));

		ASSERT_EQ(inside.size(), 2u);
		for (const CookieCutter::Partition &partition : inside)
		{
			ASSERT_EQ(partition.partitioned_geometries.size(), 1u);
		}
		ASSERT_EQ(outside.size(), 1u);
		const auto pieces = get_pieces(inside, outside, plates);
		ASSERT_EQ(pieces[0].second, boost::optional<unsigned int>(0u));
		ASSERT_EQ(pieces[1].second, boost::optional<unsigned int>(2u));

		// Plate A's piece, plate B's, then the outside: points in each, not in the hole.
		const PointOnSphere in_each[] = { at(3, 7), at(3, 15), at(0, 22) };
		double area = 0.0;
		for (unsigned int k = 0; k < pieces.size(); ++k)
		{
			SCOPED_TRACE(k);
			const CookieCutter::PartitionedGeometry &partitioned = *pieces[k].first;
			const PolygonOnSphere::non_null_ptr_to_const_type piece =
					GPlatesUtils::dynamic_pointer_cast<const PolygonOnSphere>(partitioned.geometry);
			EXPECT_EQ(piece->number_of_interior_rings(), 0u);  // the hole is cut open
			EXPECT_TRUE(piece->is_point_in_polygon(in_each[k]));
			EXPECT_FALSE(piece->is_point_in_polygon(at(0, 10)));  // the hole
			area += piece->get_area().dval();

			EXPECT_EQ(static_cast<bool>(partitioned.provenance), want_provenance);
			if (!want_provenance)
			{
				continue;
			}
			const ring_type points = points_of(*piece);
			EXPECT_EQ(
					PolygonOverlayOracle::count_provenance_failures(
							subject_rings,
							clip_rings,
							points,
							*partitioned.provenance,
							3 * H),
					0u);
			// A point beside an edge along the input (and not along the plate that cut the piece,
			// which for each piece here is also along every earlier plate's cut) keeps its
			// location on the input, through the cascade (the input's closing segments included).
			EXPECT_EQ(
					PolygonOverlayOracle::count_missing_subject_locations(
							*P,
							(k == 0) ? *plates.plate_a : *plates.plate_b,
							points,
							*partitioned.provenance),
					0u);

			// Every original vertex of the input in a piece is named as that vertex, and every
			// point of plate B's piece without a location on the input is on plate B (index 2).
			for (unsigned int i = 0; i < points.size(); ++i)
			{
				const Overlay::Provenance &provenance = (*partitioned.provenance)[i];
				for (unsigned int r = 0; r < subject_rings.size(); ++r)
				{
					for (unsigned int j = 0; j < subject_rings[r].size(); ++j)
					{
						if (PolygonOverlayOracle::same_position(subject_rings[r][j], points[i]))
						{
							ASSERT_TRUE(provenance.subject);
							EXPECT_EQ(provenance.subject->ring, r);
							EXPECT_EQ(provenance.subject->index, j);
							EXPECT_FALSE(provenance.subject->ratio);
						}
					}
				}
				if (k == 1 && !provenance.subject)
				{
					EXPECT_EQ(provenance.clip_index, 2u);
				}
			}
		}
		EXPECT_NEAR(area, P->get_area().dval(), AREA_TOLERANCE);
	}
}


// The other geometry types through the cascade with provenance (sections 5.1, 5.4): a multi-point
// split over the plates (each point's location its index in the input, through the second cut), a
// polyline cut by both plates (every point on it), a point (a table of one), and a cookie cutter
// with no plates (the input outside, with the identity table).
TEST(PolygonPartitionerTest, CookieCutterOtherGeometryTypes)
{
	const ThreePlates plates;
	const std::unique_ptr<CookieCutter> cookie_cutter = plates.create_cookie_cutter();

	// Points in plate A, plate B, the far plate, outside them all, and plate A again.
	const ring_type multi_points{ at(3, 3), at(3, 15), at(45, 105), at(0, 40), at(-3, 7) };
	{
		SCOPED_TRACE("multi-point");
		CookieCutter::partition_seq_type inside;
		CookieCutter::partitioned_geometry_seq_type outside;
		EXPECT_TRUE(cookie_cutter->partition_geometry(
				MultiPointOnSphere::create(multi_points),
				inside,
				outside,
				true/*want_provenance*/));
		const auto pieces = get_pieces(inside, outside, plates);
		ASSERT_EQ(pieces.size(), 4u);
		// By plate in the cascade's order, then outside: the input indices of each piece's points.
		const std::vector<std::pair<boost::optional<unsigned int>, std::vector<unsigned int>>>
				expected{ { 0u, { 0, 4 } }, { 1u, { 2 } }, { 2u, { 1 } }, { boost::none, { 3 } } };
		for (unsigned int k = 0; k < pieces.size(); ++k)
		{
			SCOPED_TRACE(k);
			EXPECT_EQ(pieces[k].second, expected[k].first);
			ASSERT_TRUE(pieces[k].first->provenance);
			const ring_type points = points_of(*pieces[k].first->geometry);
			const Overlay::provenance_seq_type &table = *pieces[k].first->provenance;
			ASSERT_EQ(table.size(), points.size());
			ASSERT_EQ(table.size(), expected[k].second.size());
			for (unsigned int i = 0; i < table.size(); ++i)
			{
				ASSERT_TRUE(table[i].subject);
				EXPECT_EQ(table[i].subject->index, expected[k].second[i]);
				EXPECT_EQ(points[i], multi_points[expected[k].second[i]]);
			}
		}
	}

	{
		SCOPED_TRACE("polyline");
		const ring_type line{ at(1, -5), at(1, 5), at(1, 15), at(1, 25) };
		CookieCutter::partition_seq_type inside;
		CookieCutter::partitioned_geometry_seq_type outside;
		EXPECT_TRUE(cookie_cutter->partition_geometry(
				PolylineOnSphere::create(line),
				inside,
				outside,
				true/*want_provenance*/));
		const auto pieces = get_pieces(inside, outside, plates);
		ASSERT_EQ(pieces.size(), 4u);  // in A, in B, and outside at each end
		for (const auto &piece : pieces)
		{
			ASSERT_TRUE(piece.first->provenance);
			const ring_type points = points_of(*piece.first->geometry);
			EXPECT_EQ(
					PolygonOverlayOracle::count_provenance_failures(
							{ line },
							plates.clip_rings(),
							points,
							*piece.first->provenance,
							3 * H),
					0u);
			for (const Overlay::Provenance &provenance : *piece.first->provenance)
			{
				EXPECT_TRUE(provenance.subject);  // every point is on the polyline
			}
		}
	}

	{
		SCOPED_TRACE("point");
		CookieCutter::partition_seq_type inside;
		CookieCutter::partitioned_geometry_seq_type outside;
		EXPECT_TRUE(cookie_cutter->partition_geometry(
				GPlatesMaths::PointGeometryOnSphere::create(at(0, 15)),
				inside,
				outside,
				true/*want_provenance*/));
		const auto pieces = get_pieces(inside, outside, plates);
		ASSERT_EQ(pieces.size(), 1u);
		EXPECT_EQ(pieces[0].second, boost::optional<unsigned int>(2u));
		ASSERT_TRUE(pieces[0].first->provenance);
		ASSERT_EQ(pieces[0].first->provenance->size(), 1u);
		ASSERT_TRUE(pieces[0].first->provenance->front().subject);
		EXPECT_EQ(pieces[0].first->provenance->front().subject->index, 0u);
	}

	{
		SCOPED_TRACE("no plates");
		const std::unique_ptr<CookieCutter> empty = plates.create_cookie_cutter(false);
		ASSERT_FALSE(empty->has_partitioning_polygons());
		const auto P = polygon(box(-5, 5, 5, 25), { box(-2, 2, 8, 12) });
		CookieCutter::partition_seq_type inside;
		CookieCutter::partitioned_geometry_seq_type outside;
		EXPECT_FALSE(empty->partition_geometry(P, inside, outside, true/*want_provenance*/));
		EXPECT_TRUE(inside.empty());
		ASSERT_EQ(outside.size(), 1u);
		EXPECT_EQ(outside.front().geometry.get(), P.get());
		ASSERT_TRUE(outside.front().provenance);
		const Overlay::provenance_seq_type &table = *outside.front().provenance;
		ASSERT_EQ(table.size(), 8u);
		for (unsigned int i = 0; i < table.size(); ++i)
		{
			ASSERT_TRUE(table[i].subject);
			EXPECT_EQ(table[i].subject->ring, i / 4);
			EXPECT_EQ(table[i].subject->index, i % 4);
			EXPECT_FALSE(table[i].subject->ratio);
		}
	}
}


namespace
{
	//! @a num_h thicknesses, in degrees (a distance along a meridian, or along the equator).
	double
	degrees_of_h(
			double num_h)
	{
		return GPlatesMaths::convert_rad_to_deg(num_h * H);
	}

	//! A partitioner by @a partitioning_polygon with the sliver filter at @a sliver_min_width.
	PolygonPartitioner::non_null_ptr_type
	filtering(
			const PolygonOnSphere::non_null_ptr_to_const_type &partitioning_polygon,
			boost::optional<double> sliver_min_width)
	{
		return PolygonPartitioner::create(
				partitioning_polygon, PolygonOnSphere::ADAPTIVE, sliver_min_width);
	}

	//! Extract an overlay's two outputs with the sliver filter at @a sliver_min_width.
	Overlay::Diagnostics
	extract_filtered(
			const PolygonOnSphere &subject,
			const PolygonOnSphere &clip,
			boost::optional<double> sliver_min_width,
			std::vector<Overlay::OutputPolygon> &intersection,
			std::vector<Overlay::OutputPolygon> &difference)
	{
		Overlay::Options options;
		options.sliver_min_width = sliver_min_width;
		Overlay::Overlay overlay(subject, clip, options);
		EXPECT_EQ(overlay.classify(), Overlay::RESULT_INTERSECTING);
		overlay.extract(Overlay::INTERSECTION, intersection);
		overlay.extract(Overlay::DIFFERENCE, difference);
		return overlay.get_diagnostics();
	}
}


// The sliver filter (section 5.5), off by default. A clip whose western part
// covers the subject but a strip 5 h wide along its eastern edge (both meridians): the strip is
// the difference, and holds a hole of the subject a third as wide. A filter of 10 h drops the strip
// with its hole, so the subject is whole inside; one of h / 2, or none, keeps both (the hole, a
// square, has a mean width of half its side, about 0.8 h).
TEST(PolygonPartitionerTest, SliverFilterExteriorWithItsHole)
{
	const double w = degrees_of_h(5);
	const auto P = polygon(
			box(-10, 10, 0, 20),
			{ box(-w / 6, w / 6, 20 - 2 * w / 3, 20 - w / 3) });
	const auto Q = polygon(box(-20, 20, -10, 20 - w));

	const std::vector<boost::optional<double>> widths{ boost::none, H / 2 };
	for (const boost::optional<double> &width : widths)
	{
		SCOPED_TRACE(width);
		PolygonPartitioner::partitioned_polygon_seq_type inside, outside;
		EXPECT_EQ(
				partition(*filtering(Q, width), P, inside, outside),
				PolygonPartitioner::GEOMETRY_INTERSECTING);
		ASSERT_EQ(outside.size(), 1u);
		EXPECT_EQ(outside.front()->number_of_interior_rings(), 1u);
	}

	std::vector<Overlay::OutputPolygon> intersection, difference;
	const Overlay::Diagnostics diagnostics =
			extract_filtered(*P, *Q, 10 * H, intersection, difference);
	EXPECT_EQ(intersection.size(), 1u);
	EXPECT_TRUE(difference.empty());
	EXPECT_EQ(diagnostics.sliver_exteriors_dropped, 1u);
	EXPECT_EQ(diagnostics.sliver_holes_dropped, 0u);  // dropped with its exterior, not counted
	EXPECT_GT(diagnostics.sliver_area_dropped, 0.0);

	PolygonPartitioner::partitioned_polygon_seq_type inside, outside;
	EXPECT_EQ(
			partition(*filtering(Q, 10 * H), P, inside, outside),
			PolygonPartitioner::GEOMETRY_INSIDE);
	ASSERT_EQ(inside.size(), 1u);
	EXPECT_EQ(inside.front().get(), P.get());
	EXPECT_TRUE(outside.empty());
}


// A hole below the width is dropped alone, its polygon filling it: a clip over the subject with a
// slot 5 h wide inside it. The intersection is the subject less the slot (a sliver hole), the
// difference the slot itself (a sliver exterior); at 10 h the hole goes and the polygon stays.
TEST(PolygonPartitionerTest, SliverFilterHoleAlone)
{
	const double w = degrees_of_h(5);
	const auto P = polygon(box(-10, 10, 0, 20));
	const auto Q = polygon(box(-20, 20, -10, 30), { box(-5, 5, 10, 10 + w) });

	std::vector<Overlay::OutputPolygon> intersection, difference;
	Overlay::Diagnostics diagnostics =
			extract_filtered(*P, *Q, boost::none, intersection, difference);
	ASSERT_EQ(intersection.size(), 1u);
	EXPECT_EQ(intersection.front().interiors.size(), 1u);
	EXPECT_EQ(difference.size(), 1u);
	EXPECT_EQ(diagnostics.sliver_exteriors_dropped + diagnostics.sliver_holes_dropped, 0u);

	intersection.clear();
	difference.clear();
	diagnostics = extract_filtered(*P, *Q, 10 * H, intersection, difference);
	ASSERT_EQ(intersection.size(), 1u);
	EXPECT_TRUE(intersection.front().interiors.empty());
	EXPECT_TRUE(difference.empty());
	EXPECT_EQ(diagnostics.sliver_holes_dropped, 1u);
	EXPECT_EQ(diagnostics.sliver_exteriors_dropped, 1u);
}


// A hole dropped is filled with all that lies inside it: a clip with a hole, inside the subject, so
// that the difference holds the clip's exterior as a hole and the clip's hole as an island inside
// it. The clip's exterior has ten spikes, so its mean width (about 0.009 radians) is under the
// width of 0.015, and the island's (a square, about 0.017) is not. The filled hole covers the
// island, which is dropped with it rather than kept as a polygon overlapping it.
TEST(PolygonPartitionerTest, SliverFilterHoleWithIsland)
{
	ring_type spiked{ at(-1.3, -1.3), at(-1.3, 1.3), at(1.3, 1.3) };
	for (int i = 9; i >= 0; --i)
	{
		const double lon = -1.2 + 0.25 * i;
		spiked.push_back(at(1.3, lon + 0.1));
		spiked.push_back(at(2.3, lon + 0.05));
		spiked.push_back(at(1.3, lon));
	}
	spiked.push_back(at(1.3, -1.3));
	const ring_type island = box(-1, 1, -1, 1);
	const auto P = polygon(box(-10, 10, -10, 10));
	const auto Q = polygon(spiked, { island });
	const double width = 0.015;
	ASSERT_LT(Overlay::get_mean_width(spiked), width);
	ASSERT_GT(Overlay::get_mean_width(island), width);

	std::vector<Overlay::OutputPolygon> intersection, difference;
	Overlay::Diagnostics diagnostics =
			extract_filtered(*P, *Q, boost::none, intersection, difference);
	ASSERT_EQ(intersection.size(), 1u);
	EXPECT_EQ(intersection.front().interiors.size(), 1u);
	ASSERT_EQ(difference.size(), 2u);  // the subject less the clip's exterior, and the island

	intersection.clear();
	difference.clear();
	diagnostics = extract_filtered(*P, *Q, width, intersection, difference);
	EXPECT_TRUE(intersection.empty());
	ASSERT_EQ(difference.size(), 1u);
	EXPECT_TRUE(difference.front().interiors.empty());
	EXPECT_EQ(diagnostics.sliver_exteriors_dropped, 1u);  // the intersection's, with its hole
	EXPECT_EQ(diagnostics.sliver_holes_dropped, 1u);      // the difference's, with its island

	PolygonPartitioner::partitioned_polygon_seq_type inside, outside;
	EXPECT_EQ(
			partition(*filtering(Q, width), P, inside, outside),
			PolygonPartitioner::GEOMETRY_OUTSIDE);
	ASSERT_EQ(outside.size(), 1u);
	EXPECT_EQ(outside.front().get(), P.get());
	EXPECT_TRUE(inside.empty());
}


// A subject thinner than the width, cut across: the filter drops both halves, and the subject is
// whole on the side that lost the larger area (section 5.1).
TEST(PolygonPartitionerTest, SliverFilterEmptiesBoth)
{
	const double w = degrees_of_h(5);
	const auto P = polygon(box(-5, 5, 0, w));
	const auto lower_six_degrees = polygon(box(-20, 1, -10, 10));
	const auto lower_four_degrees = polygon(box(-20, -1, -10, 10));

	for (const auto &Q : { lower_six_degrees, lower_four_degrees })
	{
		SCOPED_TRACE(Q == lower_six_degrees ? "six" : "four");
		std::vector<Overlay::OutputPolygon> intersection, difference;
		const Overlay::Diagnostics diagnostics =
				extract_filtered(*P, *Q, 10 * H, intersection, difference);
		EXPECT_TRUE(intersection.empty());
		EXPECT_TRUE(difference.empty());
		EXPECT_EQ(diagnostics.sliver_exteriors_dropped, 2u);

		PolygonPartitioner::partitioned_polygon_seq_type inside, outside;
		const PolygonPartitioner::Result expected = (Q == lower_six_degrees)
				? PolygonPartitioner::GEOMETRY_INSIDE
				: PolygonPartitioner::GEOMETRY_OUTSIDE;
		EXPECT_EQ(
				partition(*filtering(Q, 10 * H), P, inside, outside),
				expected);
		const PolygonPartitioner::partitioned_polygon_seq_type &whole =
				(expected == PolygonPartitioner::GEOMETRY_INSIDE) ? inside : outside;
		ASSERT_EQ(whole.size(), 1u);
		EXPECT_EQ(whole.front().get(), P.get());
		EXPECT_EQ(inside.size() + outside.size(), 1u);
	}
}


// The cascade passes the cookie cutter's width to every plate, and the filter is on by default
// at 10 h (section 5.5): a subject reaching 5 h into plate A from plate B is
// whole in plate B by default, and cut in two with the filter off.
TEST(PolygonPartitionerTest, SliverFilterCookieCutter)
{
	EXPECT_DOUBLE_EQ(CookieCutter::get_default_sliver_min_width(), 10 * H);

	const ThreePlates plates;
	const auto P = polygon(box(-5, 5, 10 - degrees_of_h(5), 15));
	for (const bool filter : { true, false })
	{
		SCOPED_TRACE(filter);
		const std::unique_ptr<CookieCutter> cookie_cutter = filter
				? plates.create_cookie_cutter()
				: plates.create_cookie_cutter_with_sliver_min_width(boost::none);
		CookieCutter::partition_seq_type inside;
		CookieCutter::partitioned_geometry_seq_type outside;
		EXPECT_TRUE(cookie_cutter->partition_geometry(P, inside, outside, true/*want_provenance*/));
		EXPECT_TRUE(outside.empty());
		const auto pieces = get_pieces(inside, outside, plates);
		if (filter)
		{
			ASSERT_EQ(pieces.size(), 1u);
			EXPECT_EQ(pieces[0].second, boost::optional<unsigned int>(2u));
			EXPECT_EQ(pieces[0].first->geometry.get(), P.get());
		}
		else
		{
			ASSERT_EQ(pieces.size(), 2u);
			EXPECT_EQ(pieces[0].second, boost::optional<unsigned int>(0u));
			EXPECT_EQ(pieces[1].second, boost::optional<unsigned int>(2u));
		}
	}
}


// The filter's measure (PolygonOverlay::get_mean_width): exact for short edges. A disc of radius
// 10 h drawn with 400 edges of 0.16 h each, all zero length to GreatCircleArc (so zero area to
// SphericalArea, and a sliver at any width if measured by it), has a mean width of its radius. A
// ring is measured by its smaller side, whichever way it runs.
TEST(PolygonPartitionerTest, SliverFilterMeanWidth)
{
	const double radius = 10 * H;
	const GPlatesMaths::UnitVector3D centre = at(20, 30).position_vector();
	const GPlatesMaths::UnitVector3D east = GPlatesMaths::generate_perpendicular(centre);
	const GPlatesMaths::UnitVector3D north = GPlatesMaths::cross(centre, east).get_normalisation();
	ring_type disc;
	for (unsigned int i = 0; i < 400; ++i)
	{
		const double angle = 2 * GPlatesMaths::PI * i / 400;
		disc.push_back(PointOnSphere((
				GPlatesMaths::Vector3D(centre) +
						std::tan(radius) * std::cos(angle) * GPlatesMaths::Vector3D(east) +
						std::tan(radius) * std::sin(angle) * GPlatesMaths::Vector3D(north))
				.get_normalisation()));
	}
	double area = 0.0;
	EXPECT_NEAR(Overlay::get_mean_width(disc, &area), radius, 1e-3 * radius);
	EXPECT_NEAR(area, GPlatesMaths::PI * radius * radius, 1e-3 * area);

	// A box over most of a hemisphere and its complement, run both ways: the same smaller side.
	ring_type large = box(-80, 80, 0, 170);
	const double width = Overlay::get_mean_width(large);
	std::reverse(large.begin(), large.end());
	EXPECT_NEAR(Overlay::get_mean_width(large), width, 1e-12);
	const PolygonOnSphere::non_null_ptr_to_const_type large_polygon = polygon(large);
	EXPECT_NEAR(
			2 * large_polygon->get_area().dval() / large_polygon->get_arc_length().dval(),
			width,
			1e-9);

	// A ring on one great circle: a hemisphere, of mean width 2 (an area of 2 pi over a perimeter
	// of 2 pi), whichever way it runs. A fan from the vertices' mean direction, in the ring's
	// plane, measured the 36-point equator ring 0. The meridians 0 and 180 too.
	ring_type equator, meridians;
	for (unsigned int i = 0; i < 36; ++i)
	{
		const double angle = 2 * GPlatesMaths::PI * i / 36;
		equator.push_back(at(0, 10.0 * i));
		meridians.push_back(PointOnSphere(
				GPlatesMaths::UnitVector3D(std::cos(angle), 0, std::sin(angle))));
	}
	for (ring_type ring : { equator, meridians })
	{
		for (const bool reversed : { false, true })
		{
			SCOPED_TRACE(reversed);
			if (reversed)
			{
				std::reverse(ring.begin(), ring.end());
			}
			EXPECT_NEAR(Overlay::get_mean_width(ring, &area), 2.0, 1e-12);
			EXPECT_NEAR(area, 2 * GPlatesMaths::PI, 1e-12);
		}
	}
}


// The filter on a ring on one great circle: the northern hemisphere, bounded by a 36-point equator
// ring, partitioned by a 10-degree box inside it. Its exterior once measured 0, so the filter
// dropped the difference and reported the hemisphere wholly inside the box.
TEST(PolygonPartitionerTest, SliverFilterGreatCircleRing)
{
	ring_type equator;
	for (unsigned int i = 0; i < 36; ++i)
	{
		equator.push_back(at(0, 10.0 * i));
	}
	const auto P = polygon(equator);
	const auto Q = polygon(box(10, 20, 10, 20));

	const std::vector<boost::optional<double>> widths{ boost::none, H, 10 * H };
	for (const boost::optional<double> &width : widths)
	{
		SCOPED_TRACE(width);
		PolygonPartitioner::partitioned_polygon_seq_type inside, outside;
		EXPECT_EQ(
				partition(*filtering(Q, width), P, inside, outside),
				PolygonPartitioner::GEOMETRY_INTERSECTING);
		ASSERT_EQ(inside.size(), 1u);
		EXPECT_NEAR(inside.front()->get_area().dval(), Q->get_area().dval(), 1e-9);
		ASSERT_EQ(outside.size(), 1u);
		EXPECT_EQ(outside.front()->number_of_interior_rings(), 1u);
		EXPECT_NEAR(
				outside.front()->get_area().dval(),
				2 * GPlatesMaths::PI - Q->get_area().dval(),
				1e-9);
	}
}


// The cookie cutter's options change nothing a cut gives (section 5.1): a plate with a hole and a
// plate filling the hole (a network and its rigid block), and a subject from outside both across
// the outer plate into the inner one, with the polygons grouped or not, sorted by plate ID or by
// area, at every point-in-polygon speed. Each plate's area, and what is outside both, are the
// same every time, and add up to the subject's.
TEST(PolygonPartitionerTest, CookieCutterOptions)
{
	GPlatesModel::FeatureCollectionHandle::non_null_ptr_type plates =
			GPlatesModel::FeatureCollectionHandle::create();
	add_plate(plates->reference(), 7, polygon(box(-20, 20, -20, 20), { box(-5, 5, -5, 5) }));
	add_plate(plates->reference(), 8, polygon(box(-5, 5, -5, 5)));
	GPlatesModel::FeatureCollectionHandle::non_null_ptr_type rotations =
			GPlatesModel::FeatureCollectionHandle::create();
	const GPlatesAppLogic::ReconstructMethodRegistry registry;
	const auto P = polygon(box(-2, 2, -30, 2));

	boost::optional<std::vector<double>> first_areas;
	for (const bool grouped : { true, false })
	{
		for (const CookieCutter::SortPlates sort :
				{ CookieCutter::SORT_BY_PLATE_ID, CookieCutter::SORT_BY_PLATE_AREA })
		{
			for (const PolygonOnSphere::PointInPolygonSpeedAndMemory speed : {
					PolygonOnSphere::ADAPTIVE,
					PolygonOnSphere::LOW_SPEED_NO_SETUP_NO_MEMORY_USAGE,
					PolygonOnSphere::MEDIUM_SPEED_MEDIUM_SETUP_MEDIUM_MEMORY_USAGE,
					PolygonOnSphere::HIGH_SPEED_HIGH_SETUP_HIGH_MEMORY_USAGE })
			{
				SCOPED_TRACE(testing::Message()
						<< "grouped " << grouped << ", sort " << sort << ", speed " << speed);
				const CookieCutter cookie_cutter(
						0.0,
						registry,
						std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>(
								1,
								plates->reference()),
						GPlatesAppLogic::create_cached_reconstruction_tree_creator(
								std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>(
										1,
										rotations->reference())),
						grouped,
						sort,
						speed);
				CookieCutter::partition_seq_type inside;
				CookieCutter::partitioned_geometry_seq_type outside;
				EXPECT_TRUE(cookie_cutter.partition_geometry(P, inside, outside));

				// Areas in the outer plate, the inner plate, and outside both.
				std::vector<double> areas(3, 0.0);
				const auto area_of = [](const CookieCutter::PartitionedGeometry &piece)
				{
					const PolygonOnSphere *polygon_piece =
							dynamic_cast<const PolygonOnSphere *>(piece.geometry.get());
					return polygon_piece ? polygon_piece->get_area().dval() : 0.0;
				};
				for (const CookieCutter::Partition &partition : inside)
				{
					const PolygonOnSphere::non_null_ptr_to_const_type boundary =
							*GPlatesAppLogic::ReconstructionGeometryUtils::get_boundary_polygon(
									partition.reconstruction_geometry);
					const bool outer = boundary->number_of_interior_rings() == 1;
					for (const CookieCutter::PartitionedGeometry &piece :
							partition.partitioned_geometries)
					{
						areas[outer ? 0 : 1] += area_of(piece);
					}
				}
				for (const CookieCutter::PartitionedGeometry &piece : outside)
				{
					areas[2] += area_of(piece);
				}
				EXPECT_GT(areas[0], 0.0);
				EXPECT_GT(areas[1], 0.0);
				EXPECT_GT(areas[2], 0.0);
				EXPECT_NEAR(areas[0] + areas[1] + areas[2], P->get_area().dval(), 1e-12);
				if (!first_areas)
				{
					first_areas = areas;
				}
				for (unsigned int i = 0; i < 3; ++i)
				{
					EXPECT_NEAR(areas[i], (*first_areas)[i], 1e-14);
				}
			}
		}
	}
}


namespace
{
	/**
	 * Each point of a grid over the box @a lat_0 .. @a lat_1, @a lon_0 .. @a lon_1 (degrees),
	 * farther than 20 h from both inputs' outlines, is in no more than one inside piece and one
	 * outside piece, inside a piece exactly when it is in both inputs, and outside one exactly when
	 * it is in the subject only (each read by parity, the pieces as their union).
	 */
	void
	check_pieces_sample_right(
			const PolygonOnSphere &subject,
			const PolygonOnSphere &partitioning_polygon,
			const PolygonPartitioner::partitioned_polygon_seq_type &inside,
			const PolygonPartitioner::partitioned_polygon_seq_type &outside,
			double lat_0,
			double lat_1,
			double lon_0,
			double lon_1)
	{
		const GPlatesMaths::AngularExtent clearance =
				GPlatesMaths::AngularExtent::create_from_angle(20 * H);
		const unsigned int n = 120;
		unsigned int num_samples = 0;
		for (unsigned int i = 0; i <= n; ++i)
		{
			for (unsigned int j = 0; j <= n; ++j)
			{
				const PointOnSphere p = at(
						lat_0 + (lat_1 - lat_0) * i / n,
						lon_0 + (lon_1 - lon_0) * j / n);
				if (GPlatesMaths::minimum_distance(p, subject, false, clearance) ||
					GPlatesMaths::minimum_distance(p, partitioning_polygon, false, clearance))
				{
					continue;
				}
				++num_samples;
				const auto count = [&p](
						const PolygonPartitioner::partitioned_polygon_seq_type &pieces)
				{
					unsigned int num_containing = 0;
					for (const PolygonOnSphere::non_null_ptr_to_const_type &piece : pieces)
					{
						num_containing += piece->is_point_in_polygon(
								p, PolygonOnSphere::ADAPTIVE, false) ? 1 : 0;
					}
					return num_containing;
				};
				const bool in_subject =
						subject.is_point_in_polygon(p, PolygonOnSphere::ADAPTIVE, false);
				const bool in_partitioning = partitioning_polygon.is_point_in_polygon(
						p, PolygonOnSphere::ADAPTIVE, false);
				const unsigned int num_inside = count(inside);
				const unsigned int num_outside = count(outside);
				EXPECT_LE(num_inside, 1u) << i << " " << j;
				EXPECT_LE(num_outside, 1u) << i << " " << j;
				EXPECT_EQ(num_inside == 1, in_subject && in_partitioning) << i << " " << j;
				EXPECT_EQ(num_outside == 1, in_subject && !in_partitioning) << i << " " << j;
			}
		}
		EXPECT_GT(num_samples, n * n / 2);
	}
}


// Inputs whose rings cross (section 5.2), through the partitioner: a plate whose hole crosses its
// exterior, and rings meeting only at shared vertices (a square and an
// interior ring through three of its corners, its first edge the square's diagonal), each as the
// partitioning polygon and as the subject, with no filter and with the cascade's. The pieces read
// as their union (as a feature's polygons are) must be the cut, with no point in two of them.
TEST(PolygonPartitionerTest, CrossingRings)
{
	struct Case
	{
		const char *name;
		PolygonOnSphere::non_null_ptr_to_const_type crossing;
		PolygonOnSphere::non_null_ptr_to_const_type box;
		double lat_0, lat_1, lon_0, lon_1;  // the sampled box
	};
	const Case cases[] = {
		{
			"a hole across its exterior",
			polygon(box(0, 10, 0, 10), { box(4, 6, 8, 12) }),
			polygon(box(-5, 15, 5, 20)),
			-6, 16, -1, 21
		},
		{
			"rings meeting only at shared vertices",
			polygon(box(0, 1, 0, 1), { ring_type{ at(0, 1), at(1, 0), at(0, 0), at(-1, 1) } }),
			polygon(box(-0.5, 2, -1, 2)),
			-1.5, 2.5, -1.5, 2.5
		},
		{
			// A pinwheel: each ring has vertices on the other's edges, and they overlap
			// in the unit square with no vertex of either inside the other. The box takes the tip
			// off one blade.
			"a pinwheel",
			polygon(
					ring_type{ at(0, 0), at(0, 1.5), at(1, 1), at(1, -0.5) },
					{ ring_type{ at(0, 1), at(1.5, 1), at(1, 0), at(-0.5, 0) } }),
			polygon(box(-1, 2, -1, 1.25)),
			-1, 2, -1, 2
		},
	};
	for (const Case &c : cases)
	{
		SCOPED_TRACE(c.name);
		for (const bool crossing_partitions : { true, false })
		{
			SCOPED_TRACE(crossing_partitions ? "it partitions" : "it is the subject");
			const PolygonOnSphere::non_null_ptr_to_const_type &partitioning =
					crossing_partitions ? c.crossing : c.box;
			const PolygonOnSphere::non_null_ptr_to_const_type &subject =
					crossing_partitions ? c.box : c.crossing;
			const boost::optional<double> widths[] = { boost::none, 10 * H };
			for (const boost::optional<double> &width : widths)
			{
				SCOPED_TRACE(width);
				PolygonPartitioner::partitioned_polygon_seq_type inside, outside;
				EXPECT_EQ(
						partition(*filtering(partitioning, width), subject, inside, outside),
						PolygonPartitioner::GEOMETRY_INTERSECTING);
				check_pieces_sample_right(
						*subject,
						*partitioning,
						inside,
						outside,
						c.lat_0,
						c.lat_1,
						c.lon_0,
						c.lon_1);
			}
		}
	}
}


// A ring that crosses itself is not a sliver (section 5.5): a bow-tie's signed area is zero, its
// lobes wound opposite ways, so its mean width measures below every width, and the filter would
// drop it whole (three quarters of its height inside the plate, it would come back wholly
// outside). At every width it is cut, one piece inside (the upper three quarters, crossing
// itself) and two outside (the lower corners).
TEST(PolygonPartitionerTest, SliverFilterSelfCrossingRing)
{
	const auto bow_tie = polygon(ring_type{ at(0, 0), at(2, 2), at(0, 2), at(2, 0) });
	const auto plate = polygon(box(0.5, 3, -1, 3));
	const boost::optional<double> widths[] = { boost::none, H, 10 * H };
	for (const boost::optional<double> &width : widths)
	{
		SCOPED_TRACE(width);
		PolygonPartitioner::partitioned_polygon_seq_type inside, outside;
		EXPECT_EQ(
				partition(*filtering(plate, width), bow_tie, inside, outside),
				PolygonPartitioner::GEOMETRY_INTERSECTING);
		EXPECT_EQ(inside.size(), 1u);
		EXPECT_EQ(outside.size(), 2u);
		check_pieces_sample_right(*bow_tie, *plate, inside, outside, -0.5, 2.5, -0.5, 2.5);
	}

	// With a vertex at its crossing, on one diagonal and on both (the edge test alone misses them,
	// so the filter would drop the bow-tie whole); and the bow-tie as the plate, whose
	// outside piece has it for a hole that measures as a sliver.
	const auto vertex_on_one =
			polygon(ring_type{ at(0, 0), at(1, 1), at(2, 2), at(0, 2), at(2, 0) });
	const auto vertex_on_both =
			polygon(ring_type{ at(0, 0), at(1, 1), at(2, 2), at(0, 2), at(1, 1), at(2, 0) });
	for (const auto &subject : { vertex_on_one, vertex_on_both })
	{
		SCOPED_TRACE(subject == vertex_on_one ? "a vertex on one diagonal" : "on both");
		for (const boost::optional<double> &width : widths)
		{
			SCOPED_TRACE(width);
			PolygonPartitioner::partitioned_polygon_seq_type inside, outside;
			EXPECT_EQ(
					partition(*filtering(plate, width), subject, inside, outside),
					PolygonPartitioner::GEOMETRY_INTERSECTING);
			check_pieces_sample_right(*subject, *plate, inside, outside, -0.5, 2.5, -0.5, 2.5);
		}
	}
	const auto box_around = polygon(box(-1, 3, -1, 3));
	for (const boost::optional<double> &width : widths)
	{
		SCOPED_TRACE(width);
		SCOPED_TRACE("the bow-tie as the plate");
		PolygonPartitioner::partitioned_polygon_seq_type inside, outside;
		EXPECT_EQ(
				partition(*filtering(bow_tie, width), box_around, inside, outside),
				PolygonPartitioner::GEOMETRY_INTERSECTING);
		check_pieces_sample_right(*box_around, *bow_tie, inside, outside, -1.5, 3.5, -1.5, 3.5);
	}
}
