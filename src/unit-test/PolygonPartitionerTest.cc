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
#include <vector>
#include <gtest/gtest.h>

#include "app-logic/GeometryCookieCutter.h"
#include "app-logic/GeometryUtils.h"
#include "app-logic/ReconstructMethodRegistry.h"
#include "app-logic/ReconstructionTreeCreator.h"

#include "maths/GeometryIntersect.h"
#include "maths/LatLonPoint.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolygonPartitioner.h"
#include "maths/PolylineOnSphere.h"

#include "model/FeatureCollectionHandle.h"
#include "model/FeatureHandle.h"
#include "model/FeatureType.h"
#include "model/PropertyName.h"
#include "model/TopLevelPropertyInline.h"

#include "property-values/GpmlPlateId.h"

#include "unit-test/PolygonOverlayOracle.h"


// The partition layer (phase4b-design.md section 8): PolygonPartitioner on the overlay, its result
// contract (section 5.3), its result-only path (10.2), and the cascade of GeometryCookieCutter with
// provenance (7.2, 8.2). Geometries are boxes in degrees, edges along meridians and great circles.
namespace
{
	using GPlatesMaths::GeometryOnSphere;
	using GPlatesMaths::PointOnSphere;
	using GPlatesMaths::PolygonOnSphere;
	using GPlatesMaths::PolygonPartitioner;
	using GPlatesMaths::PolylineOnSphere;
	namespace Overlay = GPlatesMaths::PolygonOverlay;

	typedef std::vector<PointOnSphere> ring_type;

	//! The thickness, as a sine (a distance in radians, for these small distances).
	const double H = GPlatesMaths::GeometryIntersect::Intersection::get_on_segment_start_threshold_sine();

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
	 * Partition @a subject by @a partitioner (lists of polygons), and check the result-only path
	 * agrees and that the geometry-level entry gives the same pieces.
	 */
	PolygonPartitioner::Result
	partition(
			const PolygonPartitioner &partitioner,
			const PolygonOnSphere::non_null_ptr_to_const_type &subject,
			PolygonPartitioner::partitioned_polygon_seq_type &inside,
			PolygonPartitioner::partitioned_polygon_seq_type &outside)
	{
		const PolygonPartitioner::Result result = partitioner.partition_polygon(subject, inside, outside);
		EXPECT_EQ(partitioner.partition_polygon(subject), result) << "the result-only path";

		PolygonPartitioner::partitioned_geometry_seq_type geometries_inside;
		PolygonPartitioner::partitioned_geometry_seq_type geometries_outside;
		EXPECT_EQ(
				partitioner.partition_geometry(subject, geometries_inside, geometries_outside),
				result);
		EXPECT_EQ(geometries_inside.size(), inside.size());
		EXPECT_EQ(geometries_outside.size(), outside.size());
		for (const PolygonPartitioner::PartitionedGeometry &geometry : geometries_inside)
		{
			EXPECT_FALSE(geometry.provenance);
		}
		return result;
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
	EXPECT_EQ(partition(*partitioner, P, inside, outside), PolygonPartitioner::GEOMETRY_INTERSECTING);
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
	EXPECT_EQ(partition(*partitioner, P, inside, outside), PolygonPartitioner::GEOMETRY_INTERSECTING);
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
	EXPECT_EQ(partition(*partitioner, P, inside, outside), PolygonPartitioner::GEOMETRY_INTERSECTING);
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


// Doc 5.1's fourth bug: a piece along the partitioner's boundary was classified by the partitioner's
// orientation. Now a polygon beside the partitioner, sharing an edge, is outside, one inside it
// sharing an edge is inside, and a polyline along its edge is inside (Q2), for both orientations.
TEST(PolygonPartitionerTest, AlongBoundaryBothOrientations)
{
	for (bool reversed : { false, true })
	{
		SCOPED_TRACE(reversed);
		const auto Q = polygon(box(-10, 10, 0, 10, reversed));
		const PolygonPartitioner::non_null_ptr_type partitioner = PolygonPartitioner::create(Q);

		const auto beside = polygon(box(-10, 10, 10, 20));
		PolygonPartitioner::partitioned_polygon_seq_type inside, outside;
		EXPECT_EQ(partition(*partitioner, beside, inside, outside), PolygonPartitioner::GEOMETRY_OUTSIDE);
		EXPECT_TRUE(inside.empty());
		ASSERT_EQ(outside.size(), 1u);
		EXPECT_EQ(outside.front().get(), beside.get());  // the input itself

		const auto within = polygon(box(-3, 3, 5, 10));
		inside.clear();
		outside.clear();
		EXPECT_EQ(partition(*partitioner, within, inside, outside), PolygonPartitioner::GEOMETRY_INSIDE);
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


// The result contract (phase4b-design.md section 5.3): a geometry that only touches the partitioner,
// from either side, or is identical to it, comes back itself, inside or outside.
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
		{ "identical, reversed", polygon(box(-10, 10, 0, 10, true)), PolygonPartitioner::GEOMETRY_INSIDE },
	};
	for (const Case &c : cases)
	{
		SCOPED_TRACE(c.name);
		PolygonPartitioner::partitioned_polygon_seq_type inside, outside;
		EXPECT_EQ(partition(*partitioner, c.subject, inside, outside), c.result);
		PolygonPartitioner::partitioned_polygon_seq_type &whole =
				(c.result == PolygonPartitioner::GEOMETRY_INSIDE) ? inside : outside;
		ASSERT_EQ(whole.size(), 1u);
		EXPECT_EQ(whole.front().get(), c.subject.get());
		EXPECT_TRUE(((c.result == PolygonPartitioner::GEOMETRY_INSIDE) ? outside : inside).empty());
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
}


// The result-only path (phase4b-design.md section 10.2), the one GLReconstructedStaticPolygonMeshes
// uses: far apart (the bounding circles alone), crossing, and a partitioner inside the input
// (which that caller used to work round itself).
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


// The other geometry types through the geometry-level entry, with provenance: a multi-point split
// into one multi-point each side, each point's subject location its index in the input.
TEST(PolygonPartitionerTest, MultiPointProvenance)
{
	const auto Q = polygon(box(-10, 10, 0, 10));
	const PolygonPartitioner::non_null_ptr_type partitioner = PolygonPartitioner::create(Q);
	const ring_type points{ at(0, 5), at(0, 20), at(5, 5), at(0, -5) };
	const auto M = GPlatesMaths::MultiPointOnSphere::create(points);

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
	for (const PolygonPartitioner::PartitionedGeometry *partitioned : { &inside.front(), &outside.front() })
	{
		ASSERT_TRUE(partitioned->provenance);
		std::vector<PointOnSphere> partitioned_points;
		GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point =
				GPlatesUtils::dynamic_pointer_cast<const GPlatesMaths::MultiPointOnSphere>(
						partitioned->geometry);
		partitioned_points.assign(multi_point->begin(), multi_point->end());
		ASSERT_EQ(partitioned->provenance->size(), partitioned_points.size());
		for (unsigned int i = 0; i < partitioned_points.size(); ++i)
		{
			const Overlay::Provenance &provenance = (*partitioned->provenance)[i];
			ASSERT_TRUE(provenance.subject);
			EXPECT_EQ(points.at(provenance.subject->index), partitioned_points[i]);
		}
	}
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
}


// The cascade (phase4b-design.md sections 7.2 and 8.2): two plates sharing an edge, and an input
// polygon with a hole across that edge, partly outside both. Each plate's piece is a polygon (the
// hole cut open by the shared edge), the rest one polygon outside; the pieces cover the input; and
// with provenance every point's locations resolve to it, on the original input and on the plate that
// cut it, through the second plate's cut of the first's outside piece. A far plate between them in
// the cascade returns that piece whole, which must keep its table.
TEST(PolygonPartitionerTest, CookieCutterCascade)
{
	const auto plate_a = polygon(box(-10, 10, 0, 10));
	const auto plate_b = polygon(box(-10, 10, 10, 20));
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

	GPlatesModel::FeatureCollectionHandle::non_null_ptr_type plates =
			GPlatesModel::FeatureCollectionHandle::create();
	const auto plate_far = polygon(box(40, 50, 100, 110));
	add_plate(plates->reference(), 3, plate_a);  // sorted by plate ID, highest first: index 0
	add_plate(plates->reference(), 2, plate_far);
	add_plate(plates->reference(), 1, plate_b);
	GPlatesModel::FeatureCollectionHandle::non_null_ptr_type rotations =
			GPlatesModel::FeatureCollectionHandle::create();

	const GPlatesAppLogic::ReconstructMethodRegistry registry;
	const GPlatesAppLogic::GeometryCookieCutter cookie_cutter(
			0.0,
			registry,
			std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>(1, plates->reference()),
			GPlatesAppLogic::create_cached_reconstruction_tree_creator(
					std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>(1, rotations->reference())));
	ASSERT_TRUE(cookie_cutter.has_partitioning_polygons());

	const std::vector<ring_type> subject_rings = rings_of(*P);
	const std::vector<std::vector<ring_type>> clip_rings{
			rings_of(*plate_a), rings_of(*plate_far), rings_of(*plate_b) };

	for (bool want_provenance : { false, true })
	{
		SCOPED_TRACE(want_provenance);
		GPlatesAppLogic::GeometryCookieCutter::partition_seq_type inside;
		GPlatesAppLogic::GeometryCookieCutter::partitioned_geometry_seq_type outside;
		EXPECT_TRUE(cookie_cutter.partition_geometry(P, inside, outside, want_provenance));

		ASSERT_EQ(inside.size(), 2u);
		std::vector<const GPlatesAppLogic::GeometryCookieCutter::PartitionedGeometry *> pieces;
		double area = 0.0;
		for (const GPlatesAppLogic::GeometryCookieCutter::Partition &partition : inside)
		{
			ASSERT_EQ(partition.partitioned_geometries.size(), 1u);
			pieces.push_back(&partition.partitioned_geometries.front());
		}
		ASSERT_EQ(outside.size(), 1u);
		pieces.push_back(&outside.front());

		// Plate A's piece, plate B's, then the outside: points in each, not in the hole.
		const PointOnSphere in_each[] = { at(3, 7), at(3, 15), at(0, 22) };
		for (unsigned int k = 0; k < pieces.size(); ++k)
		{
			SCOPED_TRACE(k);
			const PolygonOnSphere::non_null_ptr_to_const_type piece =
					GPlatesUtils::dynamic_pointer_cast<const PolygonOnSphere>(pieces[k]->geometry);
			EXPECT_EQ(piece->number_of_interior_rings(), 0u);  // the hole is cut open
			EXPECT_TRUE(piece->is_point_in_polygon(in_each[k]));
			EXPECT_FALSE(piece->is_point_in_polygon(at(0, 10)));  // the hole
			area += piece->get_area().dval();

			EXPECT_EQ(static_cast<bool>(pieces[k]->provenance), want_provenance);
			if (!want_provenance)
			{
				continue;
			}
			const std::vector<ring_type> rings = rings_of(*piece);
			std::vector<PointOnSphere> points;
			for (const ring_type &ring : rings)
			{
				points.insert(points.end(), ring.begin(), ring.end());
			}
			EXPECT_EQ(
					PolygonOverlayOracle::count_provenance_failures(
							subject_rings,
							clip_rings,
							points,
							*pieces[k]->provenance,
							3 * H),
					0u);
			// A point beside an edge along the input (and not along the plate that cut the piece,
			// which for each piece here is also along every earlier plate's cut) keeps its
			// location on the input, through the cascade (the input's closing segments included).
			EXPECT_EQ(
					PolygonOverlayOracle::count_missing_subject_locations(
							*P,
							(k == 0) ? *plate_a : *plate_b,
							points,
							*pieces[k]->provenance),
					0u);

			// Every original vertex of the input in a piece is named as that vertex, and every
			// point of plate B's piece on plate A's edge (the outside piece's cut) has a location
			// on the input or on plate B.
			for (unsigned int i = 0; i < points.size(); ++i)
			{
				const Overlay::Provenance &provenance = (*pieces[k]->provenance)[i];
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
