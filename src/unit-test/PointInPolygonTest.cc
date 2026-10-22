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

#include "maths/AngularExtent.h"
#include "maths/CubeQuadTreePartition.h"
#include "maths/GeometryDistance.h"
#include "maths/MathsUtils.h"
#include "maths/PointInPolygon.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/SmallCircleBounds.h"
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
	 * The point at colatitude @a colatitude and longitude @a longitude (radians) about the z axis.
	 */
	GPlatesMaths::PointOnSphere
	point_at(
			double colatitude,
			double longitude)
	{
		return GPlatesMaths::PointOnSphere(GPlatesMaths::UnitVector3D(
				std::sin(colatitude) * std::cos(longitude),
				std::sin(colatitude) * std::sin(longitude),
				std::cos(colatitude)));
	}


	/**
	 * A band 0.065 radians wide along a helix about the z axis: its centre line sweeps 576 degrees
	 * of longitude while its colatitude ramps from 0.779 to 2.358 radians, so successive turns are
	 * about 1 radian apart. The antipode of its boundary centroid is inside the band (about 0.1
	 * radians from the outline), which is what the interior rule fixes.
	 */
	const double HELIX_BAND_WIDTH = 0.065;
	const double HELIX_BAND_SWEEP = 576 * GPlatesMaths::PI / 180;
	const double HELIX_BAND_START_COLATITUDE = 0.779;
	const double HELIX_BAND_RAMP = 1.579;

	double
	helix_band_centre_colatitude(
			double fraction)
	{
		return HELIX_BAND_START_COLATITUDE + HELIX_BAND_RAMP * fraction;
	}

	GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type
	create_helix_band()
	{
		const unsigned int num_points_per_side = 54;
		std::vector<GPlatesMaths::PointOnSphere> points;
		for (unsigned int k = 0; k < num_points_per_side; ++k)
		{
			const double fraction = double(k) / (num_points_per_side - 1);
			points.push_back(point_at(
					helix_band_centre_colatitude(fraction) + HELIX_BAND_WIDTH / 2,
					HELIX_BAND_SWEEP * fraction));
		}
		for (unsigned int k = num_points_per_side; k-- > 0; )
		{
			const double fraction = double(k) / (num_points_per_side - 1);
			points.push_back(point_at(
					helix_band_centre_colatitude(fraction) - HELIX_BAND_WIDTH / 2,
					HELIX_BAND_SWEEP * fraction));
		}
		return GPlatesMaths::PolygonOnSphere::create(points);
	}


	/**
	 * The fraction of @a num_points random points inside @a polygon, times the area of the sphere.
	 */
	double
	monte_carlo_area(
			std::mt19937 &rng,
			const GPlatesMaths::PolygonOnSphere &polygon,
			unsigned int num_points)
	{
		unsigned int num_inside = 0;
		for (unsigned int n = 0; n < num_points; ++n)
		{
			if (polygon.is_point_in_polygon(
					GPlatesMaths::PointOnSphere(random_unit_vector(rng)),
					GPlatesMaths::PolygonOnSphere::HIGH_SPEED_HIGH_SETUP_HIGH_MEMORY_USAGE))
			{
				++num_inside;
			}
		}
		return 4 * GPlatesMaths::PI * num_inside / num_points;
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
		// At, and near, the antipodal centroid and the centroid.
		for (const GPlatesMaths::UnitVector3D &centre : { -centroid, centroid })
		{
			points.push_back(GPlatesMaths::PointOnSphere(centre));
			points.push_back(moved_away_from(centre, generate_perpendicular(centre), 1e-7));
			points.push_back(moved_away_from(centre, generate_perpendicular(centre), 1e-5));
		}

		for (const GPlatesMaths::PointOnSphere &point : points)
		{
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


//
// A point within about 1e-6 radians of the antipodal centroid, or the centroid, has the same status
// as that point (outside, or the centroid's), at every speed.
//
// The crossing arc from the antipodal centroid is then too short, or too nearly antipodal, for its
// plane to be defined. There was a bug in pyGPlates 1.0 (and GPlates 2.5) where such a point could
// be inside a tiny polygon, without set-up only (the first three queries of a polygon).
//
TEST(PointInPolygonTest, near_antipodal_centroid_and_centroid)
{
	// Cases that failed: a bow-tie (self-intersecting, its edges' planes pass through the centroid)
	// at the antipodal centroid, and a tiny polygon (about 1e-6 radians) and a sliver (1e-6 radians
	// wide), each at the antipode of a vertex.
	const double cases[3][5][3] = {
		{
			{ 0.31632634991861924, -0.742914659942909, 0.5899283417408213 },
			{ 0.9654266112134291, -0.05465835848510381, 0.2548802114889822 },
			{ 0.9600187829640893, -0.2743411893103646, 0.05568526020342974 },
			{ 0.3217341781679591, -0.5232318291176482, 0.7891232930263739 },
			{ -0.7409205817415255, 0.4610391258804732, -0.4883437477424776 }  // the test point
		},
		{
			{ 0.46331008067020724, 0.5714775076315026, -0.6773161945654718 },
			{ 0.4633102924993963, 0.5714770455155942, -0.6773164395708191 },
			{ 0.46331017983531914, 0.5714770689359648, -0.6773164968766986 },
			{ 0.46331018144512975, 0.5714778365850115, -0.677315848080787 },
			{ -0.4633102924993963, -0.5714770455155942, 0.6773164395708191 }  // the test point
		},
		{
			{ -0.5170680360101083, -0.2608430351399412, -0.815230983927643 },
			{ -0.4540345429794015, -0.5160869754809512, -0.7262966800973347 },
			{ -0.3560274787820443, -0.73157574133981, -0.5814132515132491 },
			{ 0, 0, 0 },  // (only three vertices)
			{ 0.4540345429794015, 0.5160869754809512, 0.7262966800973347 }  // the test point
		}
	};
	for (unsigned int c = 0; c < 3; ++c)
	{
		SCOPED_TRACE(testing::Message() << "case " << c);
		std::vector<GPlatesMaths::PointOnSphere> vertices;
		const unsigned int num_vertices = (c == 2) ? 3 : 4;
		for (unsigned int v = 0; v < num_vertices; ++v)
		{
			vertices.push_back(GPlatesMaths::PointOnSphere(
					GPlatesMaths::UnitVector3D(cases[c][v][0], cases[c][v][1], cases[c][v][2])));
		}
		const AtEachSpeed at_each_speed(GPlatesMaths::PolygonOnSphere::create(vertices));
		const GPlatesMaths::PointOnSphere point(
				GPlatesMaths::UnitVector3D(cases[c][4][0], cases[c][4][1], cases[c][4][2]));
		at_each_speed.expect(point, true, false);
		at_each_speed.expect(point, false, false);
	}

	// Random tiny polygons, star-shaped about their centre, with their centroid close to it.
	std::mt19937 rng(3);
	for (unsigned int n = 0; n < 40; ++n)
	{
		SCOPED_TRACE(n);
		const GPlatesMaths::UnitVector3D centre = random_unit_vector(rng);
		const double radius = (n % 2 == 0) ? 1e-5 : 1e-3;
		const AtEachSpeed at_each_speed(
				create_random_ring(rng, centre, radius, 3 + (n % 30), 1.0 / 3));
		const GPlatesMaths::UnitVector3D centroid = at_each_speed.polygon->get_boundary_centroid();
		for (double distance : { 0.0, 1e-9, 1e-7, 9e-7 })
		{
			SCOPED_TRACE(distance);
			const GPlatesMaths::UnitVector3D direction = generate_perpendicular(centroid);
			for (bool use_point_on_polygon_threshold : { true, false })
			{
				SCOPED_TRACE(use_point_on_polygon_threshold);
				at_each_speed.expect(
						moved_away_from(-centroid, direction, distance),
						use_point_on_polygon_threshold,
						false);
				at_each_speed.expect(
						moved_away_from(centroid, direction, distance),
						use_point_on_polygon_threshold,
						true);
			}
		}
	}
}


//
// Every speed, with and without the point-on-polygon threshold, gives the same answer for points
// just over 1e-6 radians from the centroid of a polygon whose edges pass through its centroid.
//
// With more than 16 segments the tree of lunes is built about the centroid, and an edge passing
// round a lune's apex (the centroid) without entering the lune must still be in it, for the
// on-outline test. There was a bug in pyGPlates 1.0 (and GPlates 2.5) where it was not.
//
TEST(PointInPolygonTest, edges_through_centroid)
{
	// A self-intersecting polygon: two lobes of 10 points, whose crossing edges pass through its
	// centre. (Their orientation matters: the tree's lunes depend on it. This one omitted edges.)
	const GPlatesMaths::UnitVector3D centre(1, 0, 0);
	const GPlatesMaths::Vector3D reference_direction(0.3, 0.5, 0.8);
	const GPlatesMaths::UnitVector3D u = cross(centre, reference_direction).get_normalisation();
	const GPlatesMaths::Vector3D v = cross(centre, u);
	std::vector<GPlatesMaths::PointOnSphere> points;
	for (double lobe_azimuth : { 0.0, GPlatesMaths::PI })
	{
		for (unsigned int k = 0; k < 10; ++k)
		{
			// The second lobe in reverse, so that the ring crosses itself at the centre.
			const double azimuth = lobe_azimuth +
					((lobe_azimuth == 0.0) ? -0.25 + 0.5 * k / 9 : 0.25 - 0.5 * k / 9);
			points.push_back(GPlatesMaths::PointOnSphere((
					std::cos(0.3) * GPlatesMaths::Vector3D(centre) +
					std::sin(0.3) *
						(std::cos(azimuth) * GPlatesMaths::Vector3D(u) + std::sin(azimuth) * v))
								.get_normalisation()));
		}
	}
	const AtEachSpeed at_each_speed(GPlatesMaths::PolygonOnSphere::create(points));
	const GPlatesMaths::UnitVector3D centroid = at_each_speed.polygon->get_boundary_centroid();

	const GPlatesMaths::UnitVector3D centroid_u =
			cross(centroid, reference_direction).get_normalisation();
	const GPlatesMaths::Vector3D centroid_v = cross(centroid, centroid_u);
	for (double distance : { 1.05e-6, 1.1e-6, 1.2e-6, 1.3e-6, 2e-6 })
	{
		for (unsigned int k = 0; k < 48; ++k)
		{
			const double azimuth = 2 * GPlatesMaths::PI * k / 48 + 0.1;
			const GPlatesMaths::PointOnSphere point((
					GPlatesMaths::Vector3D(centroid) +
					distance * (std::cos(azimuth) * GPlatesMaths::Vector3D(centroid_u) +
						std::sin(azimuth) * centroid_v)).get_normalisation());
			SCOPED_TRACE(testing::Message() << "distance " << distance << ", azimuth " << azimuth);
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


//
// A ring's interior is its smaller side, even when the ring wraps round the globe so that the
// antipode of its boundary centroid is inside it, at every speed. The area is that side's, and the
// outside point (from which crossings are counted) is outside.
//
// There was a bug in pyGPlates 1.0 (and GPlates 2.5) where the antipode of the boundary centroid was
// assumed to be outside, so this band was inside out: every point-in-polygon answer, its area, and
// its filled drawing on the globe.
//
TEST(PointInPolygonTest, wrap_around_band)
{
	const AtEachSpeed at_each_speed(create_helix_band());
	const GPlatesMaths::PolygonOnSphere &polygon = *at_each_speed.polygon;

	EXPECT_FALSE(polygon.get_interior_reference().is_every_ring_in_boundary_centroid_hemisphere());
	EXPECT_FALSE(GPlatesMaths::PointInPolygon::is_point_in_polygon(
			GPlatesMaths::PointOnSphere(polygon.get_outside_point()), polygon, false));

	// The antipode of the boundary centroid is inside.
	at_each_speed.expect(GPlatesMaths::PointOnSphere(-polygon.get_boundary_centroid()), false, true);

	// Along the band's centre line (but not at its ends, which are on the outline), inside. Between
	// turns (about 1 radian apart), or beyond its ends, outside.
	for (unsigned int k = 0; k < 40; ++k)
	{
		const double fraction = (k + 0.5) / 40;
		const double colatitude = helix_band_centre_colatitude(fraction);
		const double longitude = HELIX_BAND_SWEEP * fraction;
		SCOPED_TRACE(fraction);
		for (bool use_point_on_polygon_threshold : { true, false })
		{
			at_each_speed.expect(point_at(colatitude, longitude), use_point_on_polygon_threshold, true);
			at_each_speed.expect(
					point_at(colatitude - 0.3, longitude), use_point_on_polygon_threshold, false);
			at_each_speed.expect(
					point_at(colatitude + 0.3, longitude), use_point_on_polygon_threshold, false);
		}
	}

	// About the length of the centre line times the width (0.59 steradians), the side with the band.
	const double area = polygon.get_area().dval();
	EXPECT_GT(area, 0.55);
	EXPECT_LT(area, 0.62);
	std::mt19937 rng(4);
	EXPECT_NEAR(monte_carlo_area(rng, polygon, 100000), area, 0.04);

	// Its points run along the band's lower edge (in longitude) and back along its upper edge, which
	// is counter-clockwise seen from outside the sphere (the band is to the left).
	EXPECT_GT(polygon.get_signed_area().dval(), 0);
	EXPECT_EQ(polygon.get_orientation(), GPlatesMaths::PolygonOrientation::COUNTERCLOCKWISE);
}


//
// A ring that halves the sphere (four points on the equator) has the side to its left as its
// interior, so reversing it swaps its interior and exterior.
//
// Its boundary centroid is degenerate (the first vertex stands in for it), so there was a bug in
// pyGPlates 1.0 (and GPlates 2.5) where crossings were counted from the antipode of a vertex, on the
// outline.
//
TEST(PointInPolygonTest, ring_halving_the_sphere)
{
	for (bool reversed : { false, true })
	{
		SCOPED_TRACE(reversed);
		std::vector<GPlatesMaths::PointOnSphere> points;
		for (unsigned int k = 0; k < 4; ++k)
		{
			points.push_back(point_at(GPlatesMaths::PI / 2, (reversed ? -1 : 1) * GPlatesMaths::PI / 2 * k));
		}
		const AtEachSpeed at_each_speed(GPlatesMaths::PolygonOnSphere::create(points));

		// Counter-clockwise seen from above the north pole, so the northern hemisphere is to its left.
		for (double colatitude : { 0.0, 0.3, 1.0, 1.5 })
		{
			for (double longitude : { 0.2, 2.0, 4.0 })
			{
				SCOPED_TRACE(testing::Message() << colatitude << " " << longitude);
				at_each_speed.expect(point_at(colatitude, longitude), true, !reversed);
				at_each_speed.expect(
						point_at(GPlatesMaths::PI - colatitude, longitude), true, reversed);
			}
		}

		EXPECT_NEAR(at_each_speed.polygon->get_signed_area().dval(), 2 * GPlatesMaths::PI, 1e-9);
	}
}


//
// An exterior ring and an interior ring on opposite sides of the globe: the polygon is both rings'
// interiors (by parity), and the antipode of the boundary centroid is inside the interior ring.
//
TEST(PointInPolygonTest, antipodal_rings)
{
	std::mt19937 rng(5);
	const GPlatesMaths::UnitVector3D centre(0.3, -0.4, 0.5 * std::sqrt(3.0));
	const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type exterior =
			create_random_ring(rng, centre, 0.4, 12, 0.2);
	const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type interior =
			create_random_ring(rng, -centre, 0.4, 9, 0.2);
	const AtEachSpeed at_each_speed(GPlatesMaths::PolygonOnSphere::create(
			std::vector<GPlatesMaths::PointOnSphere>(
					exterior->exterior_ring_vertex_begin(), exterior->exterior_ring_vertex_end()),
			std::vector< std::vector<GPlatesMaths::PointOnSphere> >(1,
					std::vector<GPlatesMaths::PointOnSphere>(
							interior->exterior_ring_vertex_begin(),
							interior->exterior_ring_vertex_end()))));

	for (bool use_point_on_polygon_threshold : { true, false })
	{
		SCOPED_TRACE(use_point_on_polygon_threshold);
		at_each_speed.expect(GPlatesMaths::PointOnSphere(centre), use_point_on_polygon_threshold, true);
		at_each_speed.expect(GPlatesMaths::PointOnSphere(-centre), use_point_on_polygon_threshold, true);
		at_each_speed.expect(
				GPlatesMaths::PointOnSphere(generate_perpendicular(centre)),
				use_point_on_polygon_threshold,
				false);
	}
}


//
// A polygon whose interior covers the whole sphere except a gap of a few thousandths of a radian:
// the 20 faces of an icosahedron, one vertex's corners cut off. Every candidate for the outside
// point at least 0.01 radians from the outline is inside, so the outside point comes from the
// second pass: the cut vertex, in the gap about 4e-3 radians from the outline. Before the second
// pass the outside point fell back to the antipode of the boundary centroid, which (by symmetry
// about the cut vertex) is on the axis through it, so either an icosahedron vertex on the outline
// or in the gap.
//
TEST(PointInPolygonTest, outside_point_close_to_outline)
{
	// The icosahedron's vertices, as the candidates for the outside point are made.
	const double golden_ratio = (1 + std::sqrt(5.0)) / 2;
	std::vector<GPlatesMaths::UnitVector3D> vertices;
	for (const double first_sign : { -1.0, 1.0 })
	{
		for (const double second_sign : { -1.0, 1.0 })
		{
			vertices.push_back(GPlatesMaths::Vector3D(0, first_sign, second_sign * golden_ratio)
					.get_normalisation());
			vertices.push_back(GPlatesMaths::Vector3D(first_sign, second_sign * golden_ratio, 0)
					.get_normalisation());
			vertices.push_back(GPlatesMaths::Vector3D(second_sign * golden_ratio, 0, first_sign)
					.get_normalisation());
		}
	}
	const GPlatesMaths::UnitVector3D &cut_vertex = vertices[0];

	// Each face is three mutually adjacent vertices. Where a face has the cut vertex, it is replaced
	// by a point this far along each of its two edges from it (the same point in both faces that
	// share the edge), leaving a pentagonal gap round the cut vertex.
	const double cut_distance = 5e-3;
	const auto are_adjacent = [](
			const GPlatesMaths::UnitVector3D &vertex1,
			const GPlatesMaths::UnitVector3D &vertex2)
	{
		return std::fabs(dot(vertex1, vertex2).dval() - 1 / std::sqrt(5.0)) < 1e-6;
	};
	std::vector< std::vector<GPlatesMaths::PointOnSphere> > faces;
	std::vector<GPlatesMaths::PointOnSphere> face_centres;
	std::vector<GPlatesMaths::PointOnSphere> cut_face_points;
	for (unsigned int i = 0; i < vertices.size(); ++i)
	{
		for (unsigned int j = i + 1; j < vertices.size(); ++j)
		{
			for (unsigned int k = j + 1; k < vertices.size(); ++k)
			{
				if (!are_adjacent(vertices[i], vertices[j]) ||
					!are_adjacent(vertices[j], vertices[k]) ||
					!are_adjacent(vertices[i], vertices[k]))
				{
					continue;
				}

				const GPlatesMaths::UnitVector3D face_centre =
						(GPlatesMaths::Vector3D(vertices[i]) +
							GPlatesMaths::Vector3D(vertices[j]) +
							GPlatesMaths::Vector3D(vertices[k])).get_normalisation();
				face_centres.push_back(GPlatesMaths::PointOnSphere(face_centre));

				if (i != 0)
				{
					faces.push_back({
							GPlatesMaths::PointOnSphere(vertices[i]),
							GPlatesMaths::PointOnSphere(vertices[j]),
							GPlatesMaths::PointOnSphere(vertices[k]) });
					continue;
				}

				// A face with the cut vertex (always vertex i, the first).
				faces.push_back({
						moved_away_from(cut_vertex, vertices[j], -cut_distance),
						GPlatesMaths::PointOnSphere(vertices[j]),
						GPlatesMaths::PointOnSphere(vertices[k]),
						moved_away_from(cut_vertex, vertices[k], -cut_distance) });
				// Inside this face, just past the cut.
				cut_face_points.push_back(moved_away_from(cut_vertex, face_centre, -3 * cut_distance));
			}
		}
	}
	ASSERT_EQ(faces.size(), 20u);
	ASSERT_EQ(cut_face_points.size(), 5u);

	// The exterior ring is a face away from the cut vertex (the last), the others interior rings.
	const std::vector<GPlatesMaths::PointOnSphere> exterior_ring = faces.back();
	faces.pop_back();
	const AtEachSpeed at_each_speed(GPlatesMaths::PolygonOnSphere::create(exterior_ring, faces));
	const GPlatesMaths::PolygonOnSphere &polygon = *at_each_speed.polygon;

	// The cut vertex is closer to the outline than the first pass accepts, but not too close for the
	// second. Its distance to the outline is its distance to the great circle of any cut (the first
	// faces have the cut vertex, and each cut joins a face's last point to its first).
	const double cut_vertex_distance = std::asin(std::fabs(dot(
			cut_vertex,
			cross(faces[0].back().position_vector(), faces[0].front().position_vector())
					.get_normalisation()).dval()));
	ASSERT_GT(cut_vertex_distance, 1e-5);
	ASSERT_LT(cut_vertex_distance, 1e-2);

	// The outside point is the cut vertex (or the boundary centroid or its antipode, if it is on the
	// axis through the cut vertex, within rounding).
	const GPlatesMaths::UnitVector3D &outside_point = polygon.get_outside_point();
	EXPECT_GT(dot(outside_point, cut_vertex).dval(), 0);
	EXPECT_LT(cross(outside_point, cut_vertex).magnitude().dval(), 1e-9);

	for (bool use_point_on_polygon_threshold : { true, false })
	{
		SCOPED_TRACE(use_point_on_polygon_threshold);

		// The gap is outside, including the cut vertex (whose crossings arc from the outside point is
		// degenerate, so its crossings are counted from the second reference point).
		at_each_speed.expect(
				GPlatesMaths::PointOnSphere(cut_vertex), use_point_on_polygon_threshold, false);
		for (const GPlatesMaths::PointOnSphere &point : cut_face_points)
		{
			at_each_speed.expect(
					moved_away_from(cut_vertex, point.position_vector(), -cut_distance / 2),
					use_point_on_polygon_threshold,
					false);
		}

		// Everywhere else is inside.
		for (const GPlatesMaths::PointOnSphere &point : cut_face_points)
		{
			at_each_speed.expect(point, use_point_on_polygon_threshold, true);
		}
		for (const GPlatesMaths::PointOnSphere &point : face_centres)
		{
			at_each_speed.expect(point, use_point_on_polygon_threshold, true);
		}
	}

	// The antipode of the outside point is an icosahedron vertex, on the outline (and its crossings
	// arc is degenerate too).
	at_each_speed.expect(GPlatesMaths::PointOnSphere(-cut_vertex), true, true);
}


//
// Without the point-on-polygon threshold, a point within about 1e-6 radians of the boundary
// centroid has its own status, even when the centroid is on the outline (as at a bow-tie's
// crossing), at every speed.
//
// The crossing arc from the antipode of the centroid is then nearly antipodal, so its crossings are
// counted from a second reference point instead. There was a bug in pyGPlates 1.0 (and GPlates 2.5),
// and in the fix for it, where such a point took the centroid's status.
//
TEST(PointInPolygonTest, near_centroid_on_outline)
{
	// A symmetric bow-tie: two lobes, along +/- u from the centre, whose edges cross at the centre
	// (its boundary centroid).
	const GPlatesMaths::UnitVector3D centre(0, 0, 1);
	const GPlatesMaths::Vector3D u(1, 0, 0);
	const GPlatesMaths::Vector3D v(0, 1, 0);
	std::vector<GPlatesMaths::PointOnSphere> points;
	for (double azimuth : { 0.25, GPlatesMaths::PI + 0.25, GPlatesMaths::PI - 0.25, -0.25 })
	{
		points.push_back(GPlatesMaths::PointOnSphere((
				std::cos(0.3) * GPlatesMaths::Vector3D(centre) +
				std::sin(0.3) * (std::cos(azimuth) * u + std::sin(azimuth) * v)).get_normalisation()));
	}
	const AtEachSpeed at_each_speed(GPlatesMaths::PolygonOnSphere::create(points));
	ASSERT_LT(
			cross(at_each_speed.polygon->get_boundary_centroid(), centre).magnitude().dval(),
			1e-12);

	for (double distance : { 1e-8, 1e-7, 5e-7 })
	{
		SCOPED_TRACE(distance);
		// Into a lobe, inside.
		for (const GPlatesMaths::Vector3D &direction : { u, -u })
		{
			at_each_speed.expect(
					GPlatesMaths::PointOnSphere(
							(GPlatesMaths::Vector3D(centre) + distance * direction).get_normalisation()),
					false,
					true);
		}
		// Between the lobes, outside.
		for (const GPlatesMaths::Vector3D &direction : { v, -v })
		{
			at_each_speed.expect(
					GPlatesMaths::PointOnSphere(
							(GPlatesMaths::Vector3D(centre) + distance * direction).get_normalisation()),
					false,
					false);
		}
	}
}


//
// Code that skips a polygon because something lies outside a small circle bounding its outline
// must not skip a solid or filled polygon whose interior reaches past that circle, as a band that
// wraps round the globe does (the region round the antipode of its boundary centroid, outside the
// circle, is inside the band).
//
TEST(PointInPolygonTest, interior_beyond_bounding_circle)
{
	const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon = create_helix_band();
	const GPlatesMaths::BoundingSmallCircle &bounding_small_circle =
			polygon->get_bounding_small_circle();
	const GPlatesMaths::PointOnSphere point(-polygon->get_boundary_centroid());
	ASSERT_EQ(
			bounding_small_circle.test(point.position_vector()),
			GPlatesMaths::BoundingSmallCircle::OUTSIDE_BOUNDS);

	EXPECT_FALSE(polygon->is_outside_bounding_small_circle_outside_polygon(bounding_small_circle));

	// The point is inside the solid polygon, so at distance zero, even with a threshold smaller than
	// its distance to the outline's bounding circle. There was a bug in pyGPlates 1.0 (and GPlates
	// 2.5) where the threshold was exceeded (but then the point was also wrongly outside).
	const GPlatesMaths::AngularExtent threshold = GPlatesMaths::AngularExtent::create_from_angle(1e-3);
	const boost::optional<GPlatesMaths::AngularDistance> distance = minimum_distance(
			point,
			*polygon,
			true/*polygon_interior_is_solid*/,
			threshold);
	ASSERT_TRUE(distance);
	EXPECT_EQ(distance->get_cosine().dval(), 1.0);

	// The polygon's outline is inside its own bounding circle, but its fill reaches past it.
	EXPECT_EQ(
			bounding_small_circle.test_filled_polygon(*polygon),
			GPlatesMaths::BoundingSmallCircle::INTERSECTING_BOUNDS);
	const GPlatesMaths::InnerOuterBoundingSmallCircle inner_outer_bounding_small_circle(
			bounding_small_circle.get_centre(),
			bounding_small_circle.get_angular_extent(),
			bounding_small_circle.get_angular_extent());
	EXPECT_EQ(
			inner_outer_bounding_small_circle.test_filled_polygon(*polygon),
			GPlatesMaths::InnerOuterBoundingSmallCircle::INTERSECTING_BOUNDS);

	// So a spatial partition puts it at the root (not in a quad tree node about its circle), where
	// every tile sees its fill.
	GPlatesMaths::CubeQuadTreeLocation location;
	GPlatesMaths::CubeQuadTreePartition<int>::create(6)->add(0, *polygon, &location);
	EXPECT_FALSE(location.get_node_location());

	// An ordinary polygon's interior doesn't reach past its circle.
	std::mt19937 rng(6);
	const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type ordinary_polygon =
			create_random_ring(rng, GPlatesMaths::UnitVector3D(0, 0, 1), 0.5, 20, 0.2);
	EXPECT_TRUE(ordinary_polygon->is_outside_bounding_small_circle_outside_polygon(
			ordinary_polygon->get_bounding_small_circle()));
	EXPECT_EQ(
			ordinary_polygon->get_bounding_small_circle().test_filled_polygon(*ordinary_polygon),
			GPlatesMaths::BoundingSmallCircle::INSIDE_BOUNDS);
	GPlatesMaths::CubeQuadTreeLocation ordinary_location;
	GPlatesMaths::CubeQuadTreePartition<int>::create(6)->add(0, *ordinary_polygon, &ordinary_location);
	EXPECT_TRUE(ordinary_location.get_node_location());
}
