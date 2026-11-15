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
#include <limits>
#include <random>
#include <string>
#include <utility>
#include <vector>
#include <boost/optional.hpp>
#include <gtest/gtest.h>

#include "maths/CircleSweep.h"
#include "maths/MathsUtils.h"
#include "maths/UnitVector3D.h"
#include "maths/Vector3D.h"


//
// Tests of CircleSweep, which pairs circles on the sphere (the polygon overlay's rings, by their
// bounding circles) in the order of a sweep along an axis.
//
// The overlay's results depend on the order of the pairs, so CircleSweep must visit exactly the
// pairs that the sweep along one axis always visited, in the same order, less those whose circles
// do not meet. The reference here is that sweep as it was, enumerating every pair whose intervals
// overlap; CircleSweep finds the pairs that meet otherwise (by a tree of the circles), so these
// tests check that the two agree, on circles of many kinds.
//

namespace
{
	using GPlatesMaths::CircleSweep;
	using GPlatesMaths::UnitVector3D;

	typedef std::vector<std::pair<unsigned int, unsigned int>> pair_seq_type;


	double
	angle_between(
			const UnitVector3D &a,
			const UnitVector3D &b)
	{
		return std::atan2(cross(a, b).magnitude().dval(), dot(a, b).dval());
	}


	/**
	 * The sweep as it was: the intervals of angle from whichever of x, y and z gives the fewest
	 * overlapping pairs, and every pair whose intervals overlap, in order of their beginnings.
	 */
	pair_seq_type
	reference_sweep_pairs(
			const std::vector<CircleSweep::Circle> &circles)
	{
		struct Interval
		{
			double begin;
			double end;
			unsigned int index;
		};
		const auto end_of_overlaps = [](const std::vector<Interval> &intervals, unsigned int i)
		{
			const double end = intervals[i].end;
			return static_cast<unsigned int>(std::upper_bound(
					intervals.begin() + i + 1,
					intervals.end(),
					end,
					[](double value, const Interval &interval)
					{
						return value < interval.begin;
					}) -
				intervals.begin());
		};

		std::vector<Interval> sweep_intervals;
		boost::optional<unsigned long long> fewest_pairs;
		for (const UnitVector3D &axis :
				{ UnitVector3D::xBasis(), UnitVector3D::yBasis(), UnitVector3D::zBasis() })
		{
			std::vector<Interval> intervals;
			for (unsigned int c = 0; c < circles.size(); ++c)
			{
				const double centre = angle_between(axis, circles[c].centre);
				intervals.push_back(Interval{
						centre - circles[c].radius,
						centre + circles[c].radius,
						c });
			}
			std::stable_sort(
					intervals.begin(),
					intervals.end(),
					[](const Interval &a, const Interval &b)
					{
						return a.begin < b.begin;
					});
			unsigned long long num_pairs = 0;
			for (unsigned int i = 0; i < intervals.size(); ++i)
			{
				num_pairs += end_of_overlaps(intervals, i) - (i + 1);
			}
			if (!fewest_pairs || num_pairs < *fewest_pairs)
			{
				fewest_pairs = num_pairs;
				sweep_intervals.swap(intervals);
			}
		}

		pair_seq_type pairs;
		for (unsigned int i = 0; i < sweep_intervals.size(); ++i)
		{
			const unsigned int end = end_of_overlaps(sweep_intervals, i);
			for (unsigned int j = i + 1; j < end; ++j)
			{
				pairs.emplace_back(sweep_intervals[i].index, sweep_intervals[j].index);
			}
		}
		return pairs;
	}


	//! Whether two circles meet within CircleSweep's slack (half added to each radius, as it does).
	bool
	do_circles_meet(
			const CircleSweep::Circle &a,
			const CircleSweep::Circle &b)
	{
		const double half_slack = 0.5 * CircleSweep::get_meeting_slack();
		const double reach = (a.radius + half_slack) + (b.radius + half_slack);
		return reach >= GPlatesMaths::PI || angle_between(a.centre, b.centre) <= reach;
	}


	pair_seq_type
	circle_sweep_pairs(
			const std::vector<CircleSweep::Circle> &circles,
			unsigned int max_sweep_pairs_per_circle =
					CircleSweep::DEFAULT_MAX_SWEEP_PAIRS_PER_CIRCLE)
	{
		pair_seq_type pairs;
		CircleSweep(circles, max_sweep_pairs_per_circle).visit_pairs(
				[&](unsigned int a, unsigned int b)
				{
					pairs.emplace_back(a, b);
				});
		return pairs;
	}


	/**
	 * Checks that CircleSweep visits the reference sweep's pairs whose circles meet, in its order,
	 * whether it finds them by the tree or by the sweep, and returns the number of pairs the
	 * reference visited.
	 */
	std::size_t
	check_against_reference(
			const std::vector<CircleSweep::Circle> &circles)
	{
		const pair_seq_type reference = reference_sweep_pairs(circles);
		pair_seq_type expected;
		for (const std::pair<unsigned int, unsigned int> &pair : reference)
		{
			if (do_circles_meet(circles[pair.first], circles[pair.second]))
			{
				expected.push_back(pair);
			}
		}
		EXPECT_EQ(circle_sweep_pairs(circles), expected) << "the default";
		EXPECT_EQ(circle_sweep_pairs(circles, 0), expected) << "by the tree";
		EXPECT_EQ(circle_sweep_pairs(circles, std::numeric_limits<unsigned int>::max()), expected)
				<< "by the sweep";
		return reference.size();
	}


	//! Uniform in [0, 1), from the engine's output alone (a distribution's is not portable).
	double
	uniform(
			std::mt19937 &engine)
	{
		return engine() / 4294967296.0;
	}


	UnitVector3D
	random_direction(
			std::mt19937 &engine)
	{
		const double z = 2 * uniform(engine) - 1;
		const double longitude = 2 * GPlatesMaths::PI * uniform(engine);
		const double r = std::sqrt((std::max)(0.0, 1 - z * z));
		return UnitVector3D(r * std::cos(longitude), r * std::sin(longitude), z);
	}


	//! A direction at latitude and longitude in degrees.
	UnitVector3D
	lat_lon(
			double lat,
			double lon)
	{
		const double phi = GPlatesMaths::convert_deg_to_rad(lat);
		const double lambda = GPlatesMaths::convert_deg_to_rad(lon);
		return UnitVector3D(
				std::cos(phi) * std::cos(lambda),
				std::cos(phi) * std::sin(lambda),
				std::sin(phi));
	}
}


TEST(CircleSweepTest, FewCircles)
{
	EXPECT_TRUE(circle_sweep_pairs({}).empty());
	EXPECT_TRUE(circle_sweep_pairs({ { lat_lon(0, 0), 0.1 } }).empty());

	// Two that do not meet (10 degrees apart, radii of about 2.9), and two that do (5 apart),
	// either way round.
	check_against_reference({ { lat_lon(0, 0), 0.05 }, { lat_lon(0, 10), 0.05 } });
	check_against_reference({ { lat_lon(0, 0), 0.05 }, { lat_lon(0, 5), 0.05 } });
	check_against_reference({ { lat_lon(0, 5), 0.05 }, { lat_lon(0, 0), 0.05 } });
	EXPECT_TRUE(circle_sweep_pairs({ { lat_lon(0, 0), 0.05 }, { lat_lon(0, 10), 0.05 } }).empty());
	EXPECT_EQ(
			circle_sweep_pairs({ { lat_lon(0, 0), 0.05 }, { lat_lon(0, 5), 0.05 } }),
			pair_seq_type({ { 0, 1 } }));
}


TEST(CircleSweepTest, RandomCircles)
{
	std::mt19937 engine(2026);
	for (unsigned int round = 0; round < 6; ++round)
	{
		SCOPED_TRACE("round " + std::to_string(round));

		// Radii from tiny to a third of a radian, so that some meet many others.
		std::vector<CircleSweep::Circle> circles;
		const unsigned int num_circles = 100 + 150 * round;
		for (unsigned int c = 0; c < num_circles; ++c)
		{
			const double radius = 0.3 * std::pow(uniform(engine), 3.0);
			circles.push_back({ random_direction(engine), radius });
		}
		check_against_reference(circles);
	}
}


TEST(CircleSweepTest, ClusteredCircles)
{
	// Many small circles within a few degrees, crowding together: most meet some others.
	std::mt19937 engine(7);
	std::vector<CircleSweep::Circle> circles;
	for (unsigned int c = 0; c < 400; ++c)
	{
		const double lat = 30 + 5 * uniform(engine);
		const double lon = -100 + 5 * uniform(engine);
		const double radius = GPlatesMaths::convert_deg_to_rad(0.05 + 0.3 * uniform(engine));
		circles.push_back({ lat_lon(lat, lon), radius });
	}
	check_against_reference(circles);
}


TEST(CircleSweepTest, HugeCircles)
{
	// An exterior ring's circle beside its holes', circles past a hemisphere and past the whole
	// sphere (a radius above pi), and a circle of no radius.
	std::mt19937 engine(11);
	std::vector<CircleSweep::Circle> circles;
	for (unsigned int c = 0; c < 300; ++c)
	{
		circles.push_back({ random_direction(engine), 0.02 * uniform(engine) });
	}
	circles.push_back({ lat_lon(10, 20), 1.0 });
	circles.push_back({ lat_lon(-30, 100), 2.0 });
	circles.push_back({ lat_lon(80, -60), 3.0 });
	circles.push_back({ lat_lon(0, 0), 3.5 });
	circles.push_back({ lat_lon(5, 5), 0.0 });
	check_against_reference(circles);
}


TEST(CircleSweepTest, CoincidentAndAxisCircles)
{
	// The same circle many times (their intervals tie), circles centred on the axes and their
	// antipodes, and circles a hair from them.
	std::vector<CircleSweep::Circle> circles;
	for (unsigned int c = 0; c < 30; ++c)
	{
		circles.push_back({ lat_lon(20, 30), 0.01 });
	}
	for (const UnitVector3D &axis :
			{ UnitVector3D::xBasis(), UnitVector3D::yBasis(), UnitVector3D::zBasis() })
	{
		for (double sign : { 1.0, -1.0 })
		{
			const double x = sign * axis.x().dval();
			const double y = sign * axis.y().dval();
			const double z = sign * axis.z().dval();
			circles.push_back({ UnitVector3D(x, y, z), 0.01 });
			circles.push_back({
					(GPlatesMaths::Vector3D(x, y, z) + GPlatesMaths::Vector3D(1e-9, 2e-9, -1e-9))
							.get_normalisation(),
					0.005 });
		}
	}
	check_against_reference(circles);
}


TEST(CircleSweepTest, GridOfCirclesApart)
{
	// Small circles on a grid, none meeting: the sweep along one axis paired each with every
	// other in its band (the reason CircleSweep finds the pairs otherwise); now there are none.
	std::vector<CircleSweep::Circle> circles;
	for (unsigned int i = 0; i < 60; ++i)
	{
		for (unsigned int j = 0; j < 120; ++j)
		{
			circles.push_back({
					lat_lon(-30 + 1.0 * i, -60 + 1.0 * j),
					GPlatesMaths::convert_deg_to_rad(0.2) });
		}
	}
	const std::size_t num_reference_pairs = check_against_reference(circles);
	EXPECT_GT(num_reference_pairs, 50000u);
	EXPECT_TRUE(circle_sweep_pairs(circles).empty());
	EXPECT_TRUE(circle_sweep_pairs(circles, 0).empty());
}


TEST(CircleSweepTest, OppositeCircles)
{
	// Centres that cancel: a node's mean direction falls back to one of its circles' centres (in
	// the second case, by the tree: the first has no pair on the sweep's axis, so builds no tree).
	check_against_reference({ { UnitVector3D(1, 0, 0), 0.1 }, { UnitVector3D(-1, 0, 0), 0.1 } });
	check_against_reference({
			{ UnitVector3D(1, 0, 0), 2.0 },
			{ UnitVector3D(-1, 0, 0), 2.0 },
			{ UnitVector3D(0, 1, 0), 0.1 },
			{ UnitVector3D(0, -1, 0), 0.1 },
			{ UnitVector3D(0, 0, 1), 1.6 },
			{ UnitVector3D(0, 0, -1), 1.6 } });
}


TEST(CircleSweepTest, CirclesAtTheMeetingBoundary)
{
	// Pairs of circles whose centres are their radii and the slack apart, give or take a hair
	// either way, at reaches from tiny to near pi: where the quick test of the cosines and the
	// tree's margins could decide wrongly. Spread over the sphere, so that the tree has levels.
	std::mt19937 engine(13);
	std::vector<CircleSweep::Circle> circles;
	for (double radius : { 1e-6, 1e-3, 0.3, 1.5 })
	{
		for (double offset :
				{ -1e-8, -1e-9, -1e-10, -1e-11, -1e-12, 0.0, 1e-12, 1e-11, 1e-10, 1e-9, 1e-8 })
		{
			const UnitVector3D a = random_direction(engine);
			const UnitVector3D perpendicular =
					cross(a, random_direction(engine)).get_normalisation();
			const double angle = 2 * radius + CircleSweep::get_meeting_slack() + offset;
			const UnitVector3D b = (
					std::cos(angle) * GPlatesMaths::Vector3D(a) +
					std::sin(angle) * GPlatesMaths::Vector3D(perpendicular)).get_normalisation();
			circles.push_back({ a, radius });
			circles.push_back({ b, radius });
		}
	}
	check_against_reference(circles);
}


TEST(CircleSweepTest, DenseGridOfCirclesApart)
{
	// Dense enough that the default finds the pairs by the tree (the sweep has more pairs per
	// circle than the default allows), none meeting.
	std::vector<CircleSweep::Circle> circles;
	for (unsigned int i = 0; i < 150; ++i)
	{
		for (unsigned int j = 0; j < 150; ++j)
		{
			circles.push_back({
					lat_lon(-30 + 0.4 * i, -60 + 0.4 * j),
					GPlatesMaths::convert_deg_to_rad(0.15) });
		}
	}
	const std::size_t num_reference_pairs = check_against_reference(circles);
	EXPECT_GT(
			num_reference_pairs,
			std::size_t(CircleSweep::DEFAULT_MAX_SWEEP_PAIRS_PER_CIRCLE) * circles.size());
	EXPECT_TRUE(circle_sweep_pairs(circles).empty());
}
