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
	// Random polylines and points, clustered so that many segments are nearly as close as the closest
	// (and with enough segments for the bounding-tree search, whose leaves bound up to 8 segments).
	std::mt19937 rng(1234);
	std::uniform_real_distribution<double> uniform(-1.0, 1.0);

	for (unsigned int n = 0; n < 200; ++n)
	{
		SCOPED_TRACE(testing::Message() << "case " << n);

		const double centre_lat = 1.4 * uniform(rng);
		const double centre_lon = 3.1 * uniform(rng);
		std::vector<GPlatesMaths::PointOnSphere> polyline_points;
		double lat = centre_lat;
		double lon = centre_lon;
		for (unsigned int p = 0; p < 30; ++p)
		{
			lat += 0.02 * uniform(rng);
			lon += 0.02 * uniform(rng);
			polyline_points.push_back(point_from_lat_lon_radians(lat, lon));
		}
		const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline =
				GPlatesMaths::PolylineOnSphere::create(polyline_points);

		const GPlatesMaths::PointOnSphere point = point_from_lat_lon_radians(
				centre_lat + 0.1 * uniform(rng),
				centre_lon + 0.1 * uniform(rng));

		const boost::optional<GPlatesMaths::AngularDistance> distance =
				GPlatesMaths::minimum_distance(point, *polyline);
		ASSERT_TRUE(distance);
		const GPlatesMaths::AngularExtent threshold(distance.get());
		const boost::optional<GPlatesMaths::AngularDistance> thresholded_distance =
				GPlatesMaths::minimum_distance(point, *polyline, threshold);
		ASSERT_TRUE(thresholded_distance);
		EXPECT_EQ(thresholded_distance->get_cosine().dval(), distance->get_cosine().dval());
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
