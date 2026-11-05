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
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include <QString>

#include "app-logic/AssignPlateIds.h"
#include "app-logic/GeometryCookieCutter.h"
#include "app-logic/GeometryUtils.h"
#include "app-logic/PartitionFeatureUtils.h"
#include "app-logic/ReconstructMethodRegistry.h"
#include "app-logic/ReconstructionTreeCreator.h"

#include "maths/GeometryDistance.h"
#include "maths/LatLonPoint.h"
#include "maths/MathsUtils.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolygonOverlay.h"
#include "maths/PolygonPartitioner.h"
#include "maths/PolylineOnSphere.h"

#include "model/FeatureCollectionHandle.h"
#include "model/FeatureHandle.h"
#include "model/FeatureType.h"
#include "model/PropertyName.h"
#include "model/TopLevelPropertyInline.h"

#include "property-values/GmlDataBlock.h"
#include "property-values/GmlDataBlockCoordinateList.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/ValueObjectType.h"

#include "model/PropertyValueFinder.h"


// Assign Plate IDs' partitioning of a feature (phase4b-design.md section 8.3): a scalar coverage
// carried to the pieces from each point's provenance (section 7.3), and "most overlapping" by area
// for polygons. Geometries are boxes in degrees, edges along meridians and great circles.
namespace
{
	using GPlatesMaths::PointOnSphere;
	using GPlatesMaths::PolygonOnSphere;
	namespace PartitionFeatureUtils = GPlatesAppLogic::PartitionFeatureUtils;

	using GPlatesPropertyValues::GmlDataBlockCoordinateList;
	using GPlatesPropertyValues::ValueObjectType;

	typedef std::vector<PointOnSphere> ring_type;
	typedef PartitionFeatureUtils::PartitionedFeature::PartitionedGeometry PartitionedGeometry;


	PointOnSphere
	at(
			double lat,
			double lon)
	{
		return GPlatesMaths::make_point_on_sphere(GPlatesMaths::LatLonPoint(lat, lon));
	}

	ring_type
	box(
			double lat_0,
			double lat_1,
			double lon_0,
			double lon_1)
	{
		return ring_type{ at(lat_0, lon_0), at(lat_0, lon_1), at(lat_1, lon_1), at(lat_1, lon_0) };
	}

	PolygonOnSphere::non_null_ptr_to_const_type
	polygon(
			const ring_type &exterior,
			const std::vector<ring_type> &interiors = std::vector<ring_type>())
	{
		return PolygonOnSphere::create(exterior, interiors.begin(), interiors.end());
	}

	bool
	same_position(
			const PointOnSphere &a,
			const PointOnSphere &b)
	{
		return a.position_vector().x() == b.position_vector().x() &&
				a.position_vector().y() == b.position_vector().y() &&
				a.position_vector().z() == b.position_vector().z();
	}

	double
	angle(
			const PointOnSphere &a,
			const PointOnSphere &b)
	{
		const GPlatesMaths::UnitVector3D &u = a.position_vector();
		const GPlatesMaths::UnitVector3D &v = b.position_vector();
		return std::atan2(cross(u, v).magnitude().dval(), dot(u, v).dval());
	}


	/**
	 * A feature collection of partitioning plates (identity rotations: present day), and the cookie
	 * cutter of them.
	 */
	class Plates
	{
	public:
		explicit
		Plates(
				const std::vector<PolygonOnSphere::non_null_ptr_to_const_type> &boundaries) :
			d_plates(GPlatesModel::FeatureCollectionHandle::create()),
			d_rotations(GPlatesModel::FeatureCollectionHandle::create())
		{
			GPlatesModel::integer_plate_id_type plate_id = 100;
			for (const PolygonOnSphere::non_null_ptr_to_const_type &boundary : boundaries)
			{
				const GPlatesModel::FeatureHandle::weak_ref feature =
						GPlatesModel::FeatureHandle::create(
								d_plates->reference(),
								GPlatesModel::FeatureType::create_gpml("UnclassifiedFeature"));
				feature->add(
						GPlatesModel::TopLevelPropertyInline::create(
								GPlatesModel::PropertyName::create_gpml("reconstructionPlateId"),
								GPlatesPropertyValues::GpmlPlateId::create(plate_id++)));
				feature->add(
						GPlatesModel::TopLevelPropertyInline::create(
								GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"),
								GPlatesAppLogic::GeometryUtils::create_geometry_property_value(
										boundary)));
			}
			d_cookie_cutter.reset(new GPlatesAppLogic::GeometryCookieCutter(
					0.0,
					d_registry,
					std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>(
							1,
							d_plates->reference()),
					GPlatesAppLogic::create_cached_reconstruction_tree_creator(
							std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>(
									1,
									d_rotations->reference()))));
		}

		const GPlatesAppLogic::GeometryCookieCutter &
		get_cookie_cutter() const
		{
			return *d_cookie_cutter;
		}

	private:
		GPlatesModel::FeatureCollectionHandle::non_null_ptr_type d_plates;
		GPlatesModel::FeatureCollectionHandle::non_null_ptr_type d_rotations;
		const GPlatesAppLogic::ReconstructMethodRegistry d_registry;
		std::unique_ptr<GPlatesAppLogic::GeometryCookieCutter> d_cookie_cutter;
	};


	/**
	 * A feature with a geometry and a scalar coverage of it (one scalar per point for each scalar
	 * type, 'elements'), in its own collection.
	 */
	class CoverageFeature
	{
	public:
		CoverageFeature(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
				const std::vector<double> &values) :
			CoverageFeature(geometry, std::vector<std::vector<double>>{ values })
		{  }

		CoverageFeature(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
				const std::vector<std::vector<double>> &elements) :
			d_collection(GPlatesModel::FeatureCollectionHandle::create())
		{
			std::vector<GmlDataBlockCoordinateList::non_null_ptr_type> tuple;
			for (unsigned int e = 0; e < elements.size(); ++e)
			{
				tuple.push_back(
						GmlDataBlockCoordinateList::create(
								ValueObjectType::create_gpml(QString("TestScalar%1").arg(e)),
								GmlDataBlockCoordinateList::xml_attributes_type(),
								elements[e]));
			}
			d_feature = GPlatesModel::FeatureHandle::create(
					d_collection->reference(),
					GPlatesModel::FeatureType::create_gpml("UnclassifiedFeature"));
			d_feature->add(
					GPlatesModel::TopLevelPropertyInline::create(
							GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"),
							GPlatesAppLogic::GeometryUtils::create_geometry_property_value(
									geometry)));
			d_feature->add(
					GPlatesModel::TopLevelPropertyInline::create(
							GPlatesModel::PropertyName::create_gpml("unclassifiedGeometryCoverage"),
							GPlatesPropertyValues::GmlDataBlock::create(tuple)));
		}

		const GPlatesModel::FeatureHandle::weak_ref &
		get_feature() const
		{
			return d_feature;
		}

	private:
		GPlatesModel::FeatureCollectionHandle::non_null_ptr_type d_collection;
		GPlatesModel::FeatureHandle::weak_ref d_feature;
	};


	//! A partitioned geometry's points (all, or its exterior ring's) with its coverage's values
	//! (the first scalar type's, and every type's in 'elements').
	class Piece
	{
	public:
		ring_type points;
		std::vector<double> values;
		std::vector<std::vector<double>> elements;
		std::vector<unsigned int> ring_sizes;
	};

	//! The pieces of a feature's one geometry property: inside each partition, then outside.
	std::vector<Piece>
	partition(
			const GPlatesModel::FeatureHandle::weak_ref &feature,
			const GPlatesAppLogic::GeometryCookieCutter &cookie_cutter,
			bool exterior_ring_only = false)
	{
		const boost::shared_ptr<const PartitionFeatureUtils::PartitionedFeature> partitioned =
				PartitionFeatureUtils::partition_feature(feature, cookie_cutter, false);
		std::vector<Piece> pieces;
		if (!partitioned || partitioned->partitioned_geometry_properties.size() != 1)
		{
			ADD_FAILURE() << "expected one geometry property";
			return pieces;
		}
		const PartitionFeatureUtils::PartitionedFeature::GeometryProperty &property =
				partitioned->partitioned_geometry_properties.begin()->second;
		std::vector<const PartitionedGeometry *> geometries;
		for (const PartitionFeatureUtils::PartitionedFeature::Partition &partition :
				property.partitioned_inside_geometries)
		{
			for (const auto &geometry : partition.partitioned_geometries)
			{
				geometries.push_back(&geometry);
			}
		}
		for (const auto &geometry : property.partitioned_outside_geometries)
		{
			geometries.push_back(&geometry);
		}

		for (const PartitionedGeometry *geometry : geometries)
		{
			Piece piece;
			GPlatesAppLogic::GeometryUtils::get_geometry_points(
					*geometry->geometry_domain,
					piece.points);
			if (const PolygonOnSphere *p =
					dynamic_cast<const PolygonOnSphere *>(geometry->geometry_domain.get()))
			{
				piece.ring_sizes.push_back(p->number_of_vertices_in_exterior_ring());
				const unsigned int num_interior_rings =
						exterior_ring_only ? 0 : p->number_of_interior_rings();
				for (unsigned int i = 0; i < num_interior_rings; ++i)
				{
					piece.ring_sizes.push_back(p->number_of_vertices_in_interior_ring(i));
				}
				if (exterior_ring_only)
				{
					piece.points.erase(
							piece.points.begin() + p->number_of_vertices_in_exterior_ring(),
							piece.points.end());
				}
			}
			if (!geometry->geometry_range || geometry->geometry_range->empty())
			{
				ADD_FAILURE() << "expected a coverage";
				continue;
			}
			for (const auto &element : *geometry->geometry_range)
			{
				const auto &coordinates = element->get_coordinates();
				piece.elements.push_back(std::vector<double>(coordinates.begin(), coordinates.end()));
			}
			piece.values = piece.elements.front();
			pieces.push_back(piece);
		}
		return pieces;
	}


	/**
	 * Check design 7.3 step 2 on every ring of a piece, independently of how the code finds it:
	 * the points on the coverage's domain outline (@a domain: the exterior ring alone for an
	 * exterior-ring-only coverage) have their values from the original; each run of the others,
	 * cyclically round the ring, must be the arc-length interpolation between its valued ends. A ring
	 * with no point on the outline is not checked here. Returns whether a ring started with a run
	 * (the case review 6 1.1 found wrong).
	 */
	bool
	check_runs(
			const Piece &piece,
			const PolygonOnSphere &domain)
	{
		bool any_ring_starts_in_a_run = false;
		unsigned int ring_start = 0;
		for (unsigned int ring_size : piece.ring_sizes)
		{
			std::vector<bool> valued(ring_size);
			bool any_valued = false;
			for (unsigned int k = 0; k < ring_size; ++k)
			{
				const GPlatesMaths::AngularDistance distance = *GPlatesMaths::minimum_distance(
						*piece.points[ring_start + k].get_geometry_on_sphere(),
						domain,
						false/*geometry1_interior_is_solid*/,
						false/*geometry2_interior_is_solid*/);
				valued[k] = distance.calculate_angle().dval() < 1e-6;
				any_valued = any_valued || valued[k];
			}
			if (any_valued)
			{
				any_ring_starts_in_a_run = any_ring_starts_in_a_run || !valued[0];
				unsigned int first = 0;
				while (!valued[first])
				{
					++first;
				}
				// Each run from a valued point 'a' to the next valued point 'b', cyclically.
				unsigned int a = first;
				for (unsigned int step = 1; step <= ring_size; ++step)
				{
					const unsigned int b = (first + step) % ring_size;
					if (!valued[b])
					{
						continue;
					}
					std::vector<double> along;  // arc length from 'a' to each point up to 'b'
					double total = 0.0;
					for (unsigned int k = a; k != b; k = (k + 1) % ring_size)
					{
						total += angle(
								piece.points[ring_start + k],
								piece.points[ring_start + (k + 1) % ring_size]);
						along.push_back(total);
					}
					unsigned int r = 0;
					for (unsigned int k = (a + 1) % ring_size; k != b; k = (k + 1) % ring_size, ++r)
					{
						const double ratio = (total > 0) ? along[r] / total : 0.0;
						const double start = piece.values[ring_start + a];
						const double end = piece.values[ring_start + b];
						const double expected = start + ratio * (end - start);
						EXPECT_NEAR(piece.values[ring_start + k], expected, 1e-9)
								<< "point " << k << " of a ring of " << ring_size;
					}
					a = b;
				}
			}
			ring_start += ring_size;
		}
		return any_ring_starts_in_a_run;
	}


	//! The input of the coverage tests: a box with a hole, its exterior ring starting at its
	//! top-right corner (so that its closing segment is its right edge, which the plate's
	//! lower edge crosses).
	class CutInput
	{
	public:
		CutInput() :
			exterior{ at(10, 20), at(10, 0), at(-10, 0), at(-10, 20) },
			hole(box(-2, 2, 13, 17))
		{  }

		//! The exterior's values are its vertices' latitudes, so that along its meridian edges a
		//! point's interpolated value is its latitude; the hole's are 100 to 103.
		std::vector<double>
		values(
				bool with_hole = true) const
		{
			std::vector<double> v;
			for (const PointOnSphere &point : exterior)
			{
				v.push_back(GPlatesMaths::make_lat_lon_point(point).latitude());
			}
			for (unsigned int k = 0; with_hole && k < hole.size(); ++k)
			{
				v.push_back(100 + k);
			}
			return v;
		}

		const ring_type exterior;
		const ring_type hole;
	};
}


// A polygon with a hole and a coverage, cut by a plate whose corner lies inside it (design 7.3):
// original vertices keep their values exactly (the hole's too), a point where the plate crosses an
// edge interpolates along that edge (including the ring's closing segment, which once read past
// the end), and the plate's corner, a cut point, is interpolated by arc length between its ring
// neighbours (requirements Q1).
TEST(PartitionFeatureUtilsTest, CoverageAcrossCut)
{
	const CutInput input;
	const auto P = polygon(input.exterior, { input.hole });
	const Plates plates({ polygon(box(-5, 30, 10, 30)) });
	const CoverageFeature feature(P, input.values());
	const PointOnSphere plate_corner = at(-5, 10);

	const std::vector<Piece> pieces = partition(feature.get_feature(), plates.get_cookie_cutter());
	ASSERT_EQ(pieces.size(), 2u);  // inside the plate, then outside it
	unsigned int num_on_closing_segment = 0;
	unsigned int num_corners = 0;
	for (unsigned int k = 0; k < pieces.size(); ++k)
	{
		SCOPED_TRACE(k);
		const Piece &piece = pieces[k];
		ASSERT_EQ(piece.values.size(), piece.points.size());
		for (unsigned int i = 0; i < piece.points.size(); ++i)
		{
			const PointOnSphere &point = piece.points[i];
			const GPlatesMaths::LatLonPoint lat_lon = GPlatesMaths::make_lat_lon_point(point);
			bool is_original = false;
			for (unsigned int j = 0; j < 4; ++j)
			{
				if (same_position(point, input.exterior[j]))
				{
					EXPECT_EQ(piece.values[i], input.values()[j]);
					is_original = true;
				}
				if (same_position(point, input.hole[j]))
				{
					EXPECT_EQ(piece.values[i], 100 + j);
					is_original = true;
				}
			}
			if (is_original)
			{
				continue;
			}
			if (std::fabs(lat_lon.longitude() - 20) < 1e-6)
			{
				// On the right edge, the ring's closing segment: the value is the latitude.
				EXPECT_NEAR(piece.values[i], lat_lon.latitude(), 1e-9);
				++num_on_closing_segment;
			}
			else if (std::fabs(lat_lon.longitude() - 10) < 1e-6 && lat_lon.latitude() > 5)
			{
				// On the top edge (a great circle, so north of latitude 10 here), both ends 10.
				EXPECT_NEAR(piece.values[i], 10.0, 1e-9);
			}
			else if (same_position(point, plate_corner))
			{
				// Between its two ring neighbours (both crossings), by arc length.
				ASSERT_EQ(piece.ring_sizes.size() > 0, true);
				const unsigned int n = piece.ring_sizes[0];
				ASSERT_LT(i, n);  // on the exterior ring
				const unsigned int before = (i + n - 1) % n;
				const unsigned int after = (i + 1) % n;
				const double to_before = angle(point, piece.points[before]);
				const double to_after = angle(point, piece.points[after]);
				const double expected = piece.values[before] +
						to_before / (to_before + to_after) *
							(piece.values[after] - piece.values[before]);
				EXPECT_NEAR(piece.values[i], expected, 1e-9);
				++num_corners;
			}
			else
			{
				ADD_FAILURE() << "an unexpected point at " << lat_lon.latitude() << ", "
						<< lat_lon.longitude();
			}
		}
	}
	EXPECT_EQ(num_on_closing_segment, 2u);  // the crossing, in both pieces
	EXPECT_EQ(num_corners, 2u);
	for (const Piece &piece : pieces)
	{
		check_runs(piece, *P);
	}
}


// An exterior-ring-only coverage (an old file's: scalars for the exterior ring alone, design 7.3):
// a piece's coverage covers its exterior ring only, and where the cut opens the hole into a piece's
// exterior ring, the hole's points (which have no values) are interpolated along the ring.
TEST(PartitionFeatureUtilsTest, ExteriorRingOnlyCoverage)
{
	const CutInput input;
	const auto P = polygon(input.exterior, { input.hole });
	// The plate's left edge crosses the hole, so that its pieces join the exterior rings.
	const Plates plates({ polygon(box(-5, 30, 15, 30)) });
	const CoverageFeature feature(P, input.values(false/*with_hole*/));

	const std::vector<Piece> pieces =
			partition(
				feature.get_feature(),
				plates.get_cookie_cutter(),
				true/*exterior_ring_only*/);
	ASSERT_EQ(pieces.size(), 2u);
	unsigned int num_hole_vertices = 0;
	for (const Piece &piece : pieces)
	{
		ASSERT_EQ(piece.values.size(), piece.points.size());  // the exterior ring's points only
		ASSERT_EQ(piece.ring_sizes.size(), 1u);
		for (unsigned int i = 0; i < piece.points.size(); ++i)
		{
			EXPECT_TRUE(std::isfinite(piece.values[i]));
			EXPECT_GE(piece.values[i], -10.0);
			EXPECT_LE(piece.values[i], 10.0);
			for (unsigned int j = 0; j < 4; ++j)
			{
				if (same_position(piece.points[i], input.exterior[j]))
				{
					EXPECT_EQ(piece.values[i], input.values(false)[j]);
				}
				num_hole_vertices += same_position(piece.points[i], input.hole[j]) ? 1 : 0;
			}
		}
	}
	EXPECT_EQ(num_hole_vertices, 4u);  // each hole vertex now in an exterior ring, interpolated
	// The hole's points have no values (the domain is the exterior ring alone): a run of four.
	for (const Piece &piece : pieces)
	{
		check_runs(piece, *polygon(input.exterior));
	}

	// A plate that keeps the hole whole, inside it: that piece still has the hole, and its coverage
	// is its exterior ring's alone (as the original's was).
	const Plates plates_keeping_hole({ polygon(box(-5, 30, 10, 30)) });
	const boost::shared_ptr<const PartitionFeatureUtils::PartitionedFeature> partitioned =
			PartitionFeatureUtils::partition_feature(
					feature.get_feature(),
					plates_keeping_hole.get_cookie_cutter(),
					false);
	ASSERT_TRUE(partitioned);
	const auto &property = partitioned->partitioned_geometry_properties.begin()->second;
	ASSERT_EQ(property.partitioned_inside_geometries.size(), 1u);
	const auto &with_hole = property.partitioned_inside_geometries.front().partitioned_geometries;
	ASSERT_EQ(with_hole.size(), 1u);
	const auto piece = GPlatesUtils::dynamic_pointer_cast<const PolygonOnSphere>(
			with_hole.front().geometry_domain);
	ASSERT_EQ(piece->number_of_interior_rings(), 1u);
	ASSERT_TRUE(with_hole.front().geometry_range);
	EXPECT_EQ(
			with_hole.front().geometry_range->front()->get_coordinates().size(),
			piece->number_of_vertices_in_exterior_ring());
}


// An output ring with no point of the input's (the plate wholly inside the input: the piece inside
// it is the plate's own outline): each point takes the value at the nearest point of the input's
// outline (design 7.3 step 3), here its left edge, the meridian at longitude 0 (where the value is
// the latitude of the point's projection onto it).
TEST(PartitionFeatureUtilsTest, RingWithNoValuedPoint)
{
	const CutInput input;
	const auto P = polygon(input.exterior);
	const ring_type plate = box(-2, 2, 2, 4);
	const Plates plates({ polygon(plate) });
	const CoverageFeature feature(P, input.values(false/*with_hole*/));

	const std::vector<Piece> pieces = partition(feature.get_feature(), plates.get_cookie_cutter());
	ASSERT_EQ(pieces.size(), 2u);
	const Piece &inside = pieces[0];
	ASSERT_EQ(inside.points.size(), 4u);
	for (unsigned int i = 0; i < inside.points.size(); ++i)
	{
		const GPlatesMaths::LatLonPoint lat_lon =
				GPlatesMaths::make_lat_lon_point(inside.points[i]);
		const double projected_latitude = GPlatesMaths::convert_rad_to_deg(std::atan(
				std::tan(GPlatesMaths::convert_deg_to_rad(lat_lon.latitude())) /
				std::cos(GPlatesMaths::convert_deg_to_rad(lat_lon.longitude()))));
		EXPECT_NEAR(inside.values[i], projected_latitude, 1e-9);
	}
}


// "Most overlapping" (design 8.3): area for polygons, so a fat piece beats a thin one with longer
// edges (whose cuts would inflate arc length), and polygons decide a feature that also has a longer
// polyline.
TEST(PartitionFeatureUtilsTest, SizeMetricByArea)
{
	PartitionFeatureUtils::GeometrySizeMetric thin, fat;
	thin.accumulate(*polygon(box(0, 0.5, 0, 40)));
	fat.accumulate(*polygon(box(0, 10, 0, 10)));
	ASSERT_GT(
			polygon(box(0, 0.5, 0, 40))->get_arc_length().dval(),
			polygon(box(0, 10, 0, 10))->get_arc_length().dval());
	EXPECT_TRUE(thin < fat);
	EXPECT_FALSE(fat < thin);

	PartitionFeatureUtils::GeometrySizeMetric with_polygon, polyline_only;
	with_polygon.accumulate(*polygon(box(0, 1, 0, 1)));
	polyline_only.accumulate(
			*GPlatesMaths::PolylineOnSphere::create(ring_type{ at(0, 0), at(0, 90) }));
	EXPECT_TRUE(polyline_only < with_polygon);

	// The same length, more points (a vertex in the middle): the number of points decides.
	PartitionFeatureUtils::GeometrySizeMetric fewer_points, more_points;
	fewer_points.accumulate(
			*GPlatesMaths::MultiPointOnSphere::create(ring_type{ at(0, 0), at(0, 1) }));
	more_points.accumulate(
			*GPlatesMaths::MultiPointOnSphere::create(ring_type{ at(0, 0), at(0, 1), at(0, 2) }));
	EXPECT_TRUE(fewer_points < more_points);
	EXPECT_FALSE(more_points < fewer_points);

	// Without polygons, arc length, then the number of points.
	PartitionFeatureUtils::GeometrySizeMetric shorter, longer;
	shorter.accumulate(*GPlatesMaths::PolylineOnSphere::create(ring_type{ at(0, 0), at(0, 10) }));
	longer.accumulate(*GPlatesMaths::PolylineOnSphere::create(ring_type{ at(0, 0), at(0, 20) }));
	EXPECT_TRUE(shorter < longer);
}


// A piece cut a second time (design 7.2 and 7.3): the second plate's ring starts on the first
// plate's cut edge, whose points have no value of their own, so the run of cut points wraps past
// the ring's start. Every run of every ring, in both orders of the plates, must be the arc-length
// interpolation between its valued ends (review 6 1.1: the points before the ring's first valued
// point once all took that point's value).
TEST(PartitionFeatureUtilsTest, RunAcrossRingStart)
{
	const CutInput input;
	const auto P = polygon(input.exterior);
	const auto plate_a = polygon(box(-5, 30, -10, 10));
	const auto plate_b = polygon(box(0, 5, 5, 30));
	const CoverageFeature feature(P, input.values(false/*with_hole*/));
	for (bool a_first : { false, true })
	{
		SCOPED_TRACE(a_first);
		// Plates sort by plate ID, highest first: the later one here.
		const Plates plates(a_first
				? std::vector<PolygonOnSphere::non_null_ptr_to_const_type>{ plate_b, plate_a }
				: std::vector<PolygonOnSphere::non_null_ptr_to_const_type>{ plate_a, plate_b });
		const std::vector<Piece> pieces =
				partition(feature.get_feature(), plates.get_cookie_cutter());
		ASSERT_GE(pieces.size(), 3u);  // inside each plate, then outside both (in two here)
		bool any_ring_starts_in_a_run = false;
		for (const Piece &piece : pieces)
		{
			any_ring_starts_in_a_run = check_runs(piece, *P) || any_ring_starts_in_a_run;
		}
		EXPECT_TRUE(any_ring_starts_in_a_run);  // the case this test is for
	}
}


// More than one scalar type (a coverage tuple): each is carried independently; here the second is
// the first negated, so every piece's second must be its first negated.
TEST(PartitionFeatureUtilsTest, TwoScalarTypes)
{
	const CutInput input;
	const auto P = polygon(input.exterior, { input.hole });
	const Plates plates({ polygon(box(-5, 30, 10, 30)) });
	std::vector<double> negated = input.values();
	for (double &value : negated)
	{
		value = -value;
	}
	const CoverageFeature feature(P, std::vector<std::vector<double>>{ input.values(), negated });

	const std::vector<Piece> pieces = partition(feature.get_feature(), plates.get_cookie_cutter());
	ASSERT_EQ(pieces.size(), 2u);
	for (const Piece &piece : pieces)
	{
		ASSERT_EQ(piece.elements.size(), 2u);
		ASSERT_EQ(piece.elements[1].size(), piece.points.size());
		for (unsigned int i = 0; i < piece.points.size(); ++i)
		{
			EXPECT_EQ(piece.elements[1][i], -piece.elements[0][i]);
		}
	}
}


// The nearest outline of a ring with no valued point can be a hole's (design 7.3 step 3): a plate
// wholly inside the input, just beside its hole, takes the hole's values (100 to 103), not the
// exterior's (within 10 of zero).
TEST(PartitionFeatureUtilsTest, NearestOutlineOnHole)
{
	const CutInput input;
	const auto P = polygon(input.exterior, { input.hole });
	const Plates plates({ polygon(box(-1, 1, 11, 12)) });
	const CoverageFeature feature(P, input.values());

	const std::vector<Piece> pieces = partition(feature.get_feature(), plates.get_cookie_cutter());
	ASSERT_EQ(pieces.size(), 2u);
	ASSERT_EQ(pieces[0].points.size(), 4u);  // the plate's outline
	for (double value : pieces[0].values)
	{
		EXPECT_GE(value, 100.0);
		EXPECT_LE(value, 103.0);
	}
}


// The nearest outline value is clamped to the segment (design 7.3 step 3): beside an L-shaped
// input's reflex vertex, every point of a plate inside it is nearest to that vertex, beyond both
// segments' ends, so takes the vertex's value exactly (unclamped, the ratio runs past the end).
TEST(PartitionFeatureUtilsTest, NearestOutlineClamp)
{
	const ring_type l_shape{ at(10, 10), at(10, 0), at(-10, 0), at(-10, 20), at(0, 20), at(0, 10) };
	const std::vector<double> values{ 0, 0, 0, 0, 0, 1000 };  // 1000 at the reflex vertex (0, 10)
	const Plates plates({ polygon(box(-3, -1, 7, 9)) });
	const CoverageFeature feature(polygon(l_shape), values);

	const std::vector<Piece> pieces = partition(feature.get_feature(), plates.get_cookie_cutter());
	ASSERT_EQ(pieces.size(), 2u);
	ASSERT_EQ(pieces[0].points.size(), 4u);
	for (double value : pieces[0].values)
	{
		EXPECT_NEAR(value, 1000.0, 1e-9);
	}
}


// The other geometry types with a coverage (design 7.3): a polyline cut at a plate's edge (its
// crossing interpolated along its segment), a multi-point split by the plate (each point its own
// value), a point, and a polygon outside every plate, returned whole with its values unchanged.
TEST(PartitionFeatureUtilsTest, OtherGeometryTypes)
{
	const Plates plates({ polygon(box(-10, 10, 0, 10)) });

	{
		SCOPED_TRACE("polyline");
		// Along the equator, so that a value equal to the longitude interpolates exactly.
		const CoverageFeature feature(
				GPlatesMaths::PolylineOnSphere::create(ring_type{ at(0, -5), at(0, 5) }),
				std::vector<double>{ -5, 5 });
		const std::vector<Piece> pieces =
				partition(feature.get_feature(), plates.get_cookie_cutter());
		ASSERT_EQ(pieces.size(), 2u);  // inside, then outside
		for (const Piece &piece : pieces)
		{
			ASSERT_EQ(piece.values.size(), piece.points.size());
			for (unsigned int i = 0; i < piece.points.size(); ++i)
			{
				EXPECT_NEAR(
						piece.values[i],
						GPlatesMaths::make_lat_lon_point(piece.points[i]).longitude(),
						1e-9);
			}
		}
	}

	{
		SCOPED_TRACE("multi-point");
		const CoverageFeature feature(
				GPlatesMaths::MultiPointOnSphere::create(ring_type{ at(0, 5), at(0, -5), at(5, 5) }),
				std::vector<double>{ 1, 2, 3 });
		const std::vector<Piece> pieces =
				partition(feature.get_feature(), plates.get_cookie_cutter());
		ASSERT_EQ(pieces.size(), 2u);
		EXPECT_EQ(pieces[0].values, (std::vector<double>{ 1, 3 }));
		EXPECT_EQ(pieces[1].values, (std::vector<double>{ 2 }));
	}

	{
		SCOPED_TRACE("point");
		const CoverageFeature feature(
				GPlatesMaths::PointGeometryOnSphere::create(at(0, 5)),
				std::vector<double>{ 7 });
		const std::vector<Piece> pieces =
				partition(feature.get_feature(), plates.get_cookie_cutter());
		ASSERT_EQ(pieces.size(), 1u);
		EXPECT_EQ(pieces[0].values, (std::vector<double>{ 7 }));
	}

	{
		SCOPED_TRACE("whole");
		const CutInput input;
		const auto far_away = polygon(box(40, 50, 100, 110), { box(44, 46, 104, 106) });
		const CoverageFeature feature(far_away, input.values());
		const std::vector<Piece> pieces =
				partition(feature.get_feature(), plates.get_cookie_cutter());
		ASSERT_EQ(pieces.size(), 1u);
		EXPECT_EQ(pieces[0].values, input.values());
	}
}


namespace
{
	//! The rings of a case in the prototype's export format ('ring P n' or 'ring Q n', then n
	//! lines of 'x y z'); everything else in the file is skipped.
	std::vector<std::vector<ring_type>>
	read_case_rings(
			const char *filename)
	{
		std::vector<std::vector<ring_type>> rings(2);  // P's, then Q's
		std::ifstream file(filename);
		std::string word;
		while (file >> word)
		{
			if (word != "ring")
			{
				continue;
			}
			std::string which;
			unsigned int n = 0;
			file >> which >> n;
			ring_type ring;
			for (unsigned int k = 0; k < n; ++k)
			{
				double x, y, z;
				file >> x >> y >> z;
				ring.push_back(PointOnSphere(GPlatesMaths::UnitVector3D(x, y, z)));
			}
			rings[which == "P" ? 0 : 1].push_back(ring);
		}
		return rings;
	}
}


// Assign Plate IDs on a feature whose cut is inconsistent (design 9.2, the developer's answer to
// question 3): the overlay throws for this input (a dense shared-edge case, 'shared#1434 seed 12',
// one of the two the hunt left failing for step 15), with or without lists; Assign Plate IDs leaves
// the feature as it was and reports it.
TEST(PartitionFeatureUtilsTest, AssignPlateIdsSkipsInconsistentFeature)
{
	const std::vector<std::vector<ring_type>> rings =
			read_case_rings(GPLATES_UNIT_TEST_DATA_DIR "/polygon-overlay/inconsistent.txt");
	ASSERT_EQ(rings[0].size(), 2u);
	ASSERT_EQ(rings[1].size(), 2u);
	const auto P = polygon(rings[0][0], { rings[0][1] });
	const auto Q = polygon(rings[1][0], { rings[1][1] });

	// The partitioner raises with or without lists (review 5 1.1: the same contract either way).
	const GPlatesMaths::PolygonPartitioner::non_null_ptr_type partitioner =
			GPlatesMaths::PolygonPartitioner::create(Q);
	EXPECT_THROW(
			partitioner->partition_polygon(P),
			GPlatesMaths::PolygonOverlay::InconsistentOverlayException);
	GPlatesMaths::PolygonPartitioner::partitioned_polygon_seq_type inside, outside;
	EXPECT_THROW(
			partitioner->partition_polygon(P, inside, outside),
			GPlatesMaths::PolygonOverlay::InconsistentOverlayException);

	// Plate 100 is Q; the feature to assign has P and plate ID 5.
	GPlatesModel::FeatureCollectionHandle::non_null_ptr_type plates =
			GPlatesModel::FeatureCollectionHandle::create();
	{
		const GPlatesModel::FeatureHandle::weak_ref plate = GPlatesModel::FeatureHandle::create(
				plates->reference(),
				GPlatesModel::FeatureType::create_gpml("UnclassifiedFeature"));
		plate->add(
				GPlatesModel::TopLevelPropertyInline::create(
						GPlatesModel::PropertyName::create_gpml("reconstructionPlateId"),
						GPlatesPropertyValues::GpmlPlateId::create(100)));
		plate->add(
				GPlatesModel::TopLevelPropertyInline::create(
						GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"),
						GPlatesAppLogic::GeometryUtils::create_geometry_property_value(Q)));
	}
	GPlatesModel::FeatureCollectionHandle::non_null_ptr_type features =
			GPlatesModel::FeatureCollectionHandle::create();
	const GPlatesModel::FeatureHandle::weak_ref feature = GPlatesModel::FeatureHandle::create(
			features->reference(),
			GPlatesModel::FeatureType::create_gpml("UnclassifiedFeature"));
	feature->add(
			GPlatesModel::TopLevelPropertyInline::create(
					GPlatesModel::PropertyName::create_gpml("reconstructionPlateId"),
					GPlatesPropertyValues::GpmlPlateId::create(5)));
	feature->add(
			GPlatesModel::TopLevelPropertyInline::create(
					GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"),
					GPlatesAppLogic::GeometryUtils::create_geometry_property_value(P)));
	const unsigned int num_properties = std::distance(feature->begin(), feature->end());

	GPlatesModel::FeatureCollectionHandle::non_null_ptr_type rotations =
			GPlatesModel::FeatureCollectionHandle::create();
	const GPlatesAppLogic::AssignPlateIds::non_null_ptr_type assigner =
			GPlatesAppLogic::AssignPlateIds::create(
					GPlatesAppLogic::AssignPlateIds::ASSIGN_FEATURE_TO_MOST_OVERLAPPING_PLATE,
					std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>(
							1,
							plates->reference()),
					GPlatesAppLogic::create_cached_reconstruction_tree_creator(
							std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>(
									1,
									rotations->reference())),
					0.0);
	ASSERT_TRUE(assigner->has_partitioning_polygons());
	assigner->assign_reconstruction_plate_ids(features->reference());

	ASSERT_EQ(assigner->get_features_not_partitioned().size(), 1u);
	EXPECT_TRUE(assigner->get_features_not_partitioned().front() == feature);
	EXPECT_EQ(std::distance(feature->begin(), feature->end()), int(num_properties));
	EXPECT_EQ(std::distance(features->begin(), features->end()), 1);
	const boost::optional<GPlatesPropertyValues::GpmlPlateId::non_null_ptr_to_const_type> plate_id =
			GPlatesModel::get_property_value<GPlatesPropertyValues::GpmlPlateId>(
					feature,
					GPlatesModel::PropertyName::create_gpml("reconstructionPlateId"));
	ASSERT_TRUE(plate_id);
	EXPECT_EQ(plate_id.get()->get_value(), 5u);
	EXPECT_FALSE(GPlatesAppLogic::AssignPlateIds::get_feature_description(feature).isEmpty());
}
