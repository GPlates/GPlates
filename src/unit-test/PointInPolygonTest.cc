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
#include <random>
#include <vector>
#include <gtest/gtest.h>

#include "maths/MathsUtils.h"
#include "maths/PointInPolygon.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/UnitVector3D.h"
#include "maths/Vector3D.h"


namespace
{
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


	GPlatesMaths::UnitVector3D
	random_unit_vector(
			std::mt19937 &rng)
	{
		while (true)
		{
			const GPlatesMaths::Vector3D v(
					random_signed_unit(rng),
					random_signed_unit(rng),
					random_signed_unit(rng));
			const double mag_sqrd = v.magSqrd().dval();
			if (mag_sqrd > 0.01 && mag_sqrd <= 1)
			{
				return v.get_normalisation();
			}
		}
	}


	/**
	 * @a point moved @a angle radians further from @a centre, along the great circle through both.
	 */
	GPlatesMaths::PointOnSphere
	moved_away_from(
			const GPlatesMaths::UnitVector3D &point,
			const GPlatesMaths::UnitVector3D &centre,
			double angle)
	{
		const GPlatesMaths::UnitVector3D axis = cross(centre, point).get_normalisation();
		const GPlatesMaths::Vector3D tangent = cross(axis, point);
		return GPlatesMaths::PointOnSphere(
				(std::cos(angle) * GPlatesMaths::Vector3D(point) + std::sin(angle) * tangent)
						.get_normalisation());
	}


	/**
	 * A ring of @a num_points points about @a radius radians from @a centre, give or take
	 * @a jitter times the radius.
	 */
	GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type
	create_random_ring(
			std::mt19937 &rng,
			const GPlatesMaths::UnitVector3D &centre,
			double radius,
			unsigned int num_points,
			double jitter)
	{
		const GPlatesMaths::UnitVector3D u = generate_perpendicular(centre);
		const GPlatesMaths::Vector3D v = cross(centre, u);
		std::vector<GPlatesMaths::PointOnSphere> points;
		for (unsigned int k = 0; k < num_points; ++k)
		{
			const double azimuth = 2 * GPlatesMaths::PI * k / num_points;
			const double r = radius * (1 + jitter * random_signed_unit(rng));
			points.push_back(GPlatesMaths::PointOnSphere((
					std::cos(r) * GPlatesMaths::Vector3D(centre) +
					std::sin(r) *
						(std::cos(azimuth) * GPlatesMaths::Vector3D(u) + std::sin(azimuth) * v))
								.get_normalisation()));
		}
		return GPlatesMaths::PolygonOnSphere::create(points);
	}


	/**
	 * Point-in-polygon at each speed: no set-up, bounds only, and bounds with a tree of lunes
	 * (for more than 16 segments).
	 */
	struct AtEachSpeed
	{
		explicit
		AtEachSpeed(
				const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type &polygon_) :
			polygon(polygon_),
			medium(polygon_, false/*build_ologn_hint*/),
			high(polygon_, true/*build_ologn_hint*/)
		{  }

		void
		expect(
				const GPlatesMaths::PointOnSphere &point,
				bool use_point_on_polygon_threshold,
				bool expected) const
		{
			EXPECT_EQ(
					GPlatesMaths::PointInPolygon::is_point_in_polygon(
							point,
							*polygon,
							use_point_on_polygon_threshold),
					expected) << "no set-up";
			EXPECT_EQ(medium.is_point_in_polygon(point, use_point_on_polygon_threshold), expected)
					<< "bounds";
			EXPECT_EQ(high.is_point_in_polygon(point, use_point_on_polygon_threshold), expected)
					<< "tree";
		}

		GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon;
		GPlatesMaths::PointInPolygon::Polygon medium;
		GPlatesMaths::PointInPolygon::Polygon high;
	};
}


//
// A point within the point-on-polygon threshold (about 1.4e-6 radians) of the outline, an edge or a
// vertex, is inside at every speed (and outside without the threshold, if outside the polygon).
//
// There was a bug in pyGPlates 1.0 (and GPlates 2.5) where a vertex itself could be outside, and a
// point 1e-6 to 1.4e-6 radians outside a corner (or up to 1e-5 beyond a sharp one) was inside only
// without set-up (the bounds were widened by about 1e-6 radians, less than the threshold).
//
TEST(PointInPolygonTest, point_on_outline_at_every_speed)
{
	std::mt19937 rng(1);
	const GPlatesMaths::UnitVector3D centre(1, 0, 0);
	// 4 segments (bounds only), and 24 segments (a tree of lunes).
	for (unsigned int num_points : { 4, 24 })
	{
		SCOPED_TRACE(num_points);
		const AtEachSpeed at_each_speed(create_random_ring(rng, centre, 0.2, num_points, 0.0));
		const GPlatesMaths::UnitVector3D centroid = at_each_speed.polygon->get_boundary_centroid();
		const GPlatesMaths::UnitVector3D &vertex =
				at_each_speed.polygon->first_exterior_ring_vertex().position_vector();

		at_each_speed.expect(GPlatesMaths::PointOnSphere(vertex), true, true);

		// Outside a corner (the regular ring is convex, and its centroid is its centre).
		for (double distance : { 1e-7, 1e-6, 1.3e-6 })
		{
			SCOPED_TRACE(distance);
			at_each_speed.expect(moved_away_from(vertex, centroid, distance), true, true);
			at_each_speed.expect(moved_away_from(vertex, centroid, distance), false, false);
		}
		for (double distance : { 2e-6, 1e-5 })
		{
			SCOPED_TRACE(distance);
			at_each_speed.expect(moved_away_from(vertex, centroid, distance), true, false);
		}
		// Inside a corner.
		at_each_speed.expect(moved_away_from(vertex, centroid, -1e-7), false, true);
	}
}


//
// Every speed, with and without the point-on-polygon threshold, gives the same answer for random
// polygons and points (anywhere, and near the outline).
//
TEST(PointInPolygonTest, speeds_agree)
{
	std::mt19937 rng(2);
	for (unsigned int n = 0; n < 200; ++n)
	{
		SCOPED_TRACE(n);
		const GPlatesMaths::UnitVector3D centre = random_unit_vector(rng);
		const double radius = 0.01 + 1.4 * (random_signed_unit(rng) + 1) / 2;
		const unsigned int num_points = 4 + (rng() % 60);
		const AtEachSpeed at_each_speed(
				create_random_ring(rng, centre, radius, num_points, 1.0 / 3));
		const GPlatesMaths::UnitVector3D centroid = at_each_speed.polygon->get_boundary_centroid();

		std::vector<GPlatesMaths::PointOnSphere> points;
		for (unsigned int p = 0; p < 20; ++p)
		{
			points.push_back(GPlatesMaths::PointOnSphere(random_unit_vector(rng)));
		}
		// Near a vertex or an edge, from 1e-8 to 1e-5 radians either side.
		for (unsigned int p = 0; p < 20; ++p)
		{
			const GPlatesMaths::GreatCircleArc &edge =
					*std::next(at_each_speed.polygon->exterior_ring_begin(), rng() % num_points);
			const GPlatesMaths::UnitVector3D on_outline = (p % 2 == 0)
					? edge.start_point().position_vector()
					: (GPlatesMaths::Vector3D(edge.start_point().position_vector()) +
						GPlatesMaths::Vector3D(edge.end_point().position_vector()))
							.get_normalisation();
			const double distance = std::pow(10.0, -8 + 3 * (random_signed_unit(rng) + 1) / 2);
			points.push_back(
					moved_away_from(on_outline, centroid, (rng() % 2 == 0) ? distance : -distance));
		}

		for (const GPlatesMaths::PointOnSphere &point : points)
		{
			// FIXME: Within 1e-6 radians of the antipodal centroid, or of the centroid, the
			// crossing arc (from the antipodal centroid) is too short or nearly antipodal for its
			// plane to be well defined.
			if (std::fabs(dot(point.position_vector(), centroid).dval()) > std::cos(2e-6))
			{
				continue;
			}
			for (bool use_point_on_polygon_threshold : { true, false })
			{
				SCOPED_TRACE(use_point_on_polygon_threshold);
				at_each_speed.expect(
						point,
						use_point_on_polygon_threshold,
						GPlatesMaths::PointInPolygon::is_point_in_polygon(
								point, *at_each_speed.polygon, use_point_on_polygon_threshold));
			}
		}
	}
}
