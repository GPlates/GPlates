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
#include <random>
#include <vector>
#include <boost/optional.hpp>
#include <boost/tuple/tuple.hpp>
#include <gtest/gtest.h>

#include "maths/AngularDistance.h"
#include "maths/AngularExtent.h"
#include "maths/GeometryDistance.h"
#include "maths/GeometryOnSphere.h"
#include "maths/GreatCircleArc.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/UnitVector3D.h"


namespace
{
	GPlatesMaths::PointOnSphere
	point_from_lat_lon_radians(
			double lat,
			double lon)
	{
		return GPlatesMaths::PointOnSphere(
				GPlatesMaths::UnitVector3D(
						std::cos(lat) * std::cos(lon),
						std::cos(lat) * std::sin(lon),
						std::sin(lat)));
	}


	/**
	 * A random number in [-1, 1].
	 *
	 * Drawn directly from the generator, whose sequence the C++ standard defines, and not with
	 * std::uniform_real_distribution, whose output differs between standard libraries (so CI would
	 * test different cases on each platform, and a failure there might not reproduce locally).
	 */
	double
	random_signed_unit(
			std::mt19937 &rng)
	{
		return 2.0 * (rng() / 4294967295.0) - 1.0;
	}


	/**
	 * A random geometry of type @a geometry_type (0 point, 1 multi-point, 2 polyline, 3 polygon) as
	 * a random walk from (@a lat, @a lon) radians, in steps of up to @a step radians.
	 *
	 * The polylines and polygons have enough segments for the bounding-tree search (a leaf
	 * bounds up to 8).
	 */
	GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
	create_random_geometry(
			std::mt19937 &rng,
			unsigned int geometry_type,
			double lat,
			double lon,
			double step)
	{
		const unsigned int num_points[4] = { 1, 10, 30, 30 };
		std::vector<GPlatesMaths::PointOnSphere> points;
		for (unsigned int p = 0; p < num_points[geometry_type]; ++p)
		{
			lat += step * random_signed_unit(rng);
			lon += step * random_signed_unit(rng);
			points.push_back(point_from_lat_lon_radians(lat, lon));
		}

		switch (geometry_type)
		{
		case 0:
			return points.front().get_geometry_on_sphere();
		case 1:
			return GPlatesMaths::MultiPointOnSphere::create(points);
		case 2:
			return GPlatesMaths::PolylineOnSphere::create(points);
		default:
			return GPlatesMaths::PolygonOnSphere::create(points);
		}
	}
}


//
// The minimum distance threshold is not exceeded by a distance equal to it.
//
// So a distance passed back as the threshold must be returned again, unchanged. This is the C++
// contract (in pyGPlates the wrapper applies the caller's threshold to the distance in radians).
//

TEST(GeometryDistanceTest, point_to_arc_threshold_equal_to_distance)
{
	// An arc along the equator, and points inside its lune at distances from very small (where many
	// sines give the same cosine) to large.
	const GPlatesMaths::GreatCircleArc arc = GPlatesMaths::GreatCircleArc::create(
			point_from_lat_lon_radians(0, -0.2),
			point_from_lat_lon_radians(0, 0.2));

	for (const double lat : { 1e-12, 1e-9, 3e-9, 1e-8, 1e-7, 1e-4, 1e-2, 0.3, 1.0, 1.5 })
	{
		for (const double lon : { -0.15, 0.0, 0.07 })
		{
			SCOPED_TRACE(testing::Message() << "lat " << lat << " lon " << lon);
			const GPlatesMaths::PointOnSphere point = point_from_lat_lon_radians(lat, lon);

			const boost::optional<GPlatesMaths::AngularDistance> distance =
					GPlatesMaths::minimum_distance(point, arc);
			ASSERT_TRUE(distance);

			const GPlatesMaths::AngularExtent threshold(distance.get());
			const boost::optional<GPlatesMaths::AngularDistance> thresholded_distance =
					GPlatesMaths::minimum_distance(point, arc, threshold);
			ASSERT_TRUE(thresholded_distance);
			EXPECT_EQ(thresholded_distance->get_cosine().dval(), distance->get_cosine().dval());
		}
	}
}


TEST(GeometryDistanceTest, geometry_threshold_equal_to_distance)
{
	// Random geometries of every pairing of types, with solid flags, clustered so that many segments
	// are nearly as close as the closest. In half the cases the second geometry is tiny and near the
	// first's antipode (with the first also tiny), so the distance is close to PI.
	//
	// Solid ones near the antipode test point-in-polygon near a tiny polygon's antipode: there was
	// a bug in pyGPlates 1.0 where a point there could be inside for the first three queries of a
	// polygon, so the second call (with the threshold) could disagree with the first.
	std::mt19937 rng(1234);

	for (unsigned int n = 0; n < 640; ++n)
	{
		const unsigned int geometry1_type = n % 4;
		const unsigned int geometry2_type = (n / 4) % 4;
		const bool near_antipodal = (n / 16) % 2 == 1;
		const bool geometry1_is_solid = (n / 32) % 2 == 1;
		const bool geometry2_is_solid = (n / 64) % 2 == 1;
		SCOPED_TRACE(testing::Message() << "case " << n
				<< ": types " << geometry1_type << " " << geometry2_type
				<< (near_antipodal ? ", near antipodal" : "")
				<< ", solid " << geometry1_is_solid << " " << geometry2_is_solid);

		const double centre_lat = 1.4 * random_signed_unit(rng);
		const double centre_lon = 3.1 * random_signed_unit(rng);
		const double step = near_antipodal ? 1e-7 : 0.02;
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry1 =
				create_random_geometry(rng, geometry1_type, centre_lat, centre_lon, step);
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry2 = near_antipodal
				? create_random_geometry(
						rng, geometry2_type, -centre_lat, centre_lon + GPlatesMaths::PI, step)
				: create_random_geometry(
						rng,
						geometry2_type,
						centre_lat + 0.1 * random_signed_unit(rng),
						centre_lon + 0.1 * random_signed_unit(rng),
						step);

		const boost::optional<GPlatesMaths::AngularDistance> distance =
				GPlatesMaths::minimum_distance(
						*geometry1, *geometry2, geometry1_is_solid, geometry2_is_solid);
		ASSERT_TRUE(distance);
		const GPlatesMaths::AngularExtent threshold(distance.get());
		const boost::optional<GPlatesMaths::AngularDistance> thresholded_distance =
				GPlatesMaths::minimum_distance(
						*geometry1, *geometry2, geometry1_is_solid, geometry2_is_solid, threshold);
		ASSERT_TRUE(thresholded_distance) << "distance cosine " << distance->get_cosine().dval();
		EXPECT_EQ(thresholded_distance->get_cosine().dval(), distance->get_cosine().dval());
	}
}


TEST(GeometryDistanceTest, exact_antipodes_within_threshold_of_pi)
{
	// A point and its exact antipode (coordinates negated) are PI apart, so a threshold of PI must
	// not be exceeded. Their dot product can round to just below -1 (a cosine no threshold can
	// have), which used to exceed it.
	std::mt19937 rng(5678);

	for (unsigned int n = 0; n < 1000; ++n)
	{
		SCOPED_TRACE(testing::Message() << "case " << n);
		const GPlatesMaths::PointOnSphere point = point_from_lat_lon_radians(
				1.5 * random_signed_unit(rng),
				3.1 * random_signed_unit(rng));
		const GPlatesMaths::UnitVector3D &position = point.position_vector();
		const GPlatesMaths::PointOnSphere antipode(
				GPlatesMaths::UnitVector3D(-position.x(), -position.y(), -position.z()));

		const boost::optional<GPlatesMaths::AngularDistance> distance =
				GPlatesMaths::minimum_distance(point, antipode, GPlatesMaths::AngularExtent::PI);
		ASSERT_TRUE(distance);
		// The dot product rounds to either side of -1 (by an ULP or two); below -1 it is clamped.
		EXPECT_GE(distance->get_cosine().dval(), -1.0);
		EXPECT_NEAR(distance->get_cosine().dval(), -1.0, 1e-15);
	}
}


TEST(GeometryDistanceTest, antipodal_geometries)
{
	// Geometries exactly pi apart: the closest positions and indices are set, with or without a
	// threshold of pi, and a threshold of pi is not exceeded.
	const GPlatesMaths::PointOnSphere north(GPlatesMaths::UnitVector3D(0, 0, 1));
	const GPlatesMaths::PointOnSphere south(GPlatesMaths::UnitVector3D(0, 0, -1));
	const std::vector<GPlatesMaths::PointOnSphere> south_points(2, south);
	const GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multipoint =
			GPlatesMaths::MultiPointOnSphere::create(south_points);
	const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline =
			GPlatesMaths::PolylineOnSphere::create(south_points);

	for (const GPlatesMaths::GeometryOnSphere *geometry :
			{ static_cast<const GPlatesMaths::GeometryOnSphere *>(multipoint.get()),
				static_cast<const GPlatesMaths::GeometryOnSphere *>(polyline.get()) })
	{
		for (const bool use_threshold : { false, true })
		{
			SCOPED_TRACE(testing::Message()
					<< (geometry == multipoint.get() ? "multipoint" : "polyline")
					<< (use_threshold ? " with threshold of pi" : " without threshold"));

			GPlatesMaths::UnitVector3D closest_position1 = GPlatesMaths::UnitVector3D::xBasis();
			GPlatesMaths::UnitVector3D closest_position2 = GPlatesMaths::UnitVector3D::xBasis();
			unsigned int closest_index1 = 99;
			unsigned int closest_index2 = 99;
			boost::optional<const GPlatesMaths::AngularExtent &> threshold;
			if (use_threshold)
			{
				threshold = GPlatesMaths::AngularExtent::PI;
			}

			const boost::optional<GPlatesMaths::AngularDistance> distance =
					GPlatesMaths::minimum_distance(
							*north.get_geometry_on_sphere(),
							*geometry,
							false/*geometry1_interior_is_solid*/,
							false/*geometry2_interior_is_solid*/,
							threshold,
							boost::make_tuple(
									boost::ref(closest_position1), boost::ref(closest_position2)),
							boost::make_tuple(
									boost::ref(closest_index1), boost::ref(closest_index2)));
			ASSERT_TRUE(distance);
			EXPECT_EQ(distance->get_cosine().dval(), -1.0);
			EXPECT_TRUE(closest_position1 == north.position_vector());
			EXPECT_TRUE(closest_position2 == south.position_vector());
			EXPECT_EQ(closest_index1, 0u);
			EXPECT_EQ(closest_index2, 0u);
		}
	}
}


TEST(GeometryDistanceTest, inside_solid_polygon_near_its_outline)
{
	// A polyline, and a polygon, inside a solid polygon and about 1e-7 radians from its outline
	// (within epsilon, but not intersecting). Being inside the solid interior, their distance is
	// exactly zero, which used to be skipped for outlines within epsilon of each other.
	const std::vector<GPlatesMaths::PointOnSphere> triangle_points = {
			point_from_lat_lon_radians(0, -0.1),
			point_from_lat_lon_radians(0, 0.1),
			point_from_lat_lon_radians(0.1, 0) };
	const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type triangle =
			GPlatesMaths::PolygonOnSphere::create(triangle_points);

	const std::vector<GPlatesMaths::PointOnSphere> inner_points = {
			point_from_lat_lon_radians(1e-7, -0.05),
			point_from_lat_lon_radians(1e-7, 0.05),
			point_from_lat_lon_radians(0.01, 0) };
	const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type inner_polyline =
			GPlatesMaths::PolylineOnSphere::create(inner_points);
	const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type inner_polygon =
			GPlatesMaths::PolygonOnSphere::create(inner_points);

	// Not intersecting, but within epsilon (when the interior is not solid).
	const boost::optional<GPlatesMaths::AngularDistance> outline_distance =
			GPlatesMaths::minimum_distance(*inner_polyline, *triangle);
	ASSERT_TRUE(outline_distance);
	EXPECT_LT(outline_distance->get_cosine().dval(), 1.0);
	EXPECT_TRUE(
			outline_distance.get() == GPlatesMaths::AngularDistance::ZERO/*epsilon comparison*/);

	const boost::optional<GPlatesMaths::AngularDistance> polyline_distance =
			GPlatesMaths::minimum_distance(
					*inner_polyline, *triangle, true/*polygon_interior_is_solid*/);
	ASSERT_TRUE(polyline_distance);
	EXPECT_EQ(polyline_distance->get_cosine().dval(), 1.0);

	const boost::optional<GPlatesMaths::AngularDistance> polygon_distance =
			GPlatesMaths::minimum_distance(
					*triangle, *inner_polygon, true/*polygon1_interior_is_solid*/);
	ASSERT_TRUE(polygon_distance);
	EXPECT_EQ(polygon_distance->get_cosine().dval(), 1.0);

	const boost::optional<GPlatesMaths::AngularDistance> reversed_polygon_distance =
			GPlatesMaths::minimum_distance(
					*inner_polygon,
					*triangle,
					false/*polygon1_interior_is_solid*/,
					true/*polygon2_interior_is_solid*/);
	ASSERT_TRUE(reversed_polygon_distance);
	EXPECT_EQ(reversed_polygon_distance->get_cosine().dval(), 1.0);
}
