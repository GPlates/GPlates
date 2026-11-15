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
#include <utility>
#include <vector>
#include <boost/optional.hpp>

#include "CircleSweep.h"

#include "MathsUtils.h"
#include "UnitVector3D.h"
#include "Vector3D.h"


namespace
{
	using GPlatesMaths::CircleSweep;
	using GPlatesMaths::UnitVector3D;


	//! Circles per leaf of the tree of circles.
	const unsigned int MAX_CIRCLES_PER_LEAF = 8;

	//! Added to a node's radius, so that it encloses its circles whatever the angles' rounding.
	const double NODE_RADIUS_MARGIN = 1e-12;

	/**
	 * How far, as a cosine, a dot product must be from the cosine of the reach for the quick test
	 * to decide: far above the rounding of either, and of a unit vector's length (which
	 * UnitVector3D checks only to real_t's tolerance), so that only the exact test decides the
	 * pairs near the boundary.
	 */
	const double QUICK_TEST_MARGIN = 1e-9;


	/**
	 * The angle between two unit vectors, accurate near zero (the polygon overlay's formula, so
	 * that the intervals, and so the order, are the sweep's as it always was).
	 */
	double
	angle_between(
			const UnitVector3D &a,
			const UnitVector3D &b)
	{
		return std::atan2(cross(a, b).magnitude().dval(), dot(a, b).dval());
	}


	/**
	 * A circle as the meeting test needs it: half the slack added to its radius (so that two such
	 * radii sum to the reach), and that angle's cosine and sine.
	 */
	class Reach
	{
	public:
		explicit
		Reach(
				double radius) :
			half_reach(radius + 0.5 * CircleSweep::get_meeting_slack()),
			cos_half_reach(std::cos(half_reach)),
			sin_half_reach(std::sin(half_reach))
		{  }

		double half_reach;
		double cos_half_reach;
		double sin_half_reach;
	};


	/**
	 * Whether two circles meet, within the slack: the angle between their centres is at most the
	 * sum of their radii and the slack. Decided by the cosines where they are clearly apart either
	 * way, and otherwise by the angle itself, so that the answer is always the angle's.
	 *
	 * The slack is an angle, and the cosines only a quicker way to the same answer: a slack on the
	 * cosine would be a wider angle for small circles than for the nodes that enclose them, so
	 * the tree could prune two nodes while two of their circles meet.
	 *
	 * The answer is the same, bit for bit, with the two circles exchanged (no operation here is
	 * contracted: src/CMakeLists.txt). The sweep asks with the circle of lower rank first and the
	 * tree either way round, and the two must give the same pairs.
	 */
	bool
	do_circles_meet(
			const UnitVector3D &centre_a,
			const Reach &reach_a,
			const UnitVector3D &centre_b,
			const Reach &reach_b)
	{
		const double reach = reach_a.half_reach + reach_b.half_reach;
		if (reach >= GPlatesMaths::PI)
		{
			return true;
		}
		const double cos_reach = reach_a.cos_half_reach * reach_b.cos_half_reach -
				reach_a.sin_half_reach * reach_b.sin_half_reach;
		const double cos_angle = dot(centre_a, centre_b).dval();
		if (cos_angle > cos_reach + QUICK_TEST_MARGIN)
		{
			return true;
		}
		if (cos_angle < cos_reach - QUICK_TEST_MARGIN)
		{
			return false;
		}
		return angle_between(centre_a, centre_b) <= reach;
	}


	//! A circle's interval of angle from an axis.
	class Interval
	{
	public:
		double begin;
		double end;
		unsigned int index;
	};


	//! The circles' intervals on @a axis, in order of their beginnings (stably).
	std::vector<Interval>
	get_intervals(
			const UnitVector3D &axis,
			const std::vector<CircleSweep::Circle> &circles)
	{
		std::vector<Interval> intervals;
		intervals.reserve(circles.size());
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
		return intervals;
	}


	//! One past the last interval, in order of their beginnings, that begins in interval @a i.
	unsigned int
	end_of_overlaps(
			const std::vector<Interval> &intervals,
			unsigned int i)
	{
		const double end = intervals[i].end;
		return std::upper_bound(
				intervals.begin() + i + 1,
				intervals.end(),
				end,
				[](double value, const Interval &interval)
				{
					return value < interval.begin;
				}) -
			intervals.begin();
	}


	/**
	 * The intervals of the axis of fewest overlapping pairs (the first of x, y and z on a tie),
	 * in order of their beginnings, and that number of pairs.
	 */
	std::pair<std::vector<Interval>, unsigned long long>
	get_sweep_intervals(
			const std::vector<CircleSweep::Circle> &circles)
	{
		std::vector<Interval> sweep_intervals;
		boost::optional<unsigned long long> fewest_pairs;
		for (const UnitVector3D &axis :
				{ UnitVector3D::xBasis(), UnitVector3D::yBasis(), UnitVector3D::zBasis() })
		{
			std::vector<Interval> intervals = get_intervals(axis, circles);
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
		return std::make_pair(std::move(sweep_intervals), fewest_pairs.get());
	}


	/**
	 * A tree of circles, each node a circle enclosing its children's, built top down by splitting
	 * the circles at the median of their centres along the axis of their widest spread.
	 */
	class CircleTree
	{
	public:

		class Node
		{
		public:
			UnitVector3D centre;
			double radius;
			Reach reach;

			//! The sum of the node's circles' centres (for its parent's centre).
			double sum[3];

			//! The node's circles: @a d_order[begin, end).
			unsigned int begin;
			unsigned int end;

			//! Whether the node has no children, else its two children's indices.
			bool leaf;
			unsigned int children[2];
		};


		CircleTree(
				const std::vector<CircleSweep::Circle> &circles,
				const std::vector<Reach> &reaches) :
			d_circles(circles),
			d_reaches(reaches)
		{
			d_order.resize(circles.size());
			for (unsigned int c = 0; c < circles.size(); ++c)
			{
				d_order[c] = c;
			}
			build(0, circles.size());
		}


		/**
		 * Calls @a visit(a, b) for each pair of circles that meet, within the slack, once (in no
		 * particular order).
		 */
		template <typename VisitType>
		void
		visit_meeting_pairs(
				VisitType visit) const
		{
			// The root is the last node built.
			std::vector<std::pair<unsigned int, unsigned int>> node_pairs;
			node_pairs.emplace_back(d_nodes.size() - 1, d_nodes.size() - 1);
			while (!node_pairs.empty())
			{
				const unsigned int a = node_pairs.back().first;
				const unsigned int b = node_pairs.back().second;
				node_pairs.pop_back();
				const Node &node_a = d_nodes[a];
				const Node &node_b = d_nodes[b];

				if (a == b)
				{
					// A node with itself: its circles' pairs.
					if (node_a.leaf)
					{
						for (unsigned int i = node_a.begin; i < node_a.end; ++i)
						{
							for (unsigned int j = i + 1; j < node_a.end; ++j)
							{
								visit_if_meeting(d_order[i], d_order[j], visit);
							}
						}
					}
					else
					{
						node_pairs.emplace_back(node_a.children[0], node_a.children[0]);
						node_pairs.emplace_back(node_a.children[1], node_a.children[1]);
						node_pairs.emplace_back(node_a.children[0], node_a.children[1]);
					}
					continue;
				}

				// Two nodes: none of their circles meet if the nodes' circles do not.
				if (!do_circles_meet(node_a.centre, node_a.reach, node_b.centre, node_b.reach))
				{
					continue;
				}
				if (node_a.leaf && node_b.leaf)
				{
					for (unsigned int i = node_a.begin; i < node_a.end; ++i)
					{
						for (unsigned int j = node_b.begin; j < node_b.end; ++j)
						{
							visit_if_meeting(d_order[i], d_order[j], visit);
						}
					}
				}
				else if (node_b.leaf || (!node_a.leaf && node_a.radius > node_b.radius))
				{
					// Down the larger node (or the one that is not a leaf).
					node_pairs.emplace_back(node_a.children[0], b);
					node_pairs.emplace_back(node_a.children[1], b);
				}
				else
				{
					node_pairs.emplace_back(a, node_b.children[0]);
					node_pairs.emplace_back(a, node_b.children[1]);
				}
			}
		}

	private:

		const std::vector<CircleSweep::Circle> &d_circles;
		const std::vector<Reach> &d_reaches;

		//! The circles in the order of the leaves.
		std::vector<unsigned int> d_order;

		std::vector<Node> d_nodes;


		template <typename VisitType>
		void
		visit_if_meeting(
				unsigned int a,
				unsigned int b,
				VisitType &visit) const
		{
			if (do_circles_meet(
					d_circles[a].centre,
					d_reaches[a],
					d_circles[b].centre,
					d_reaches[b]))
			{
				visit(a, b);
			}
		}


		//! The circles @a d_order[begin, end) as a node and its subtree; returns the node's index.
		unsigned int
		build(
				unsigned int begin,
				unsigned int end)
		{
			if (end - begin <= MAX_CIRCLES_PER_LEAF)
			{
				d_nodes.push_back(make_leaf(begin, end));
				return d_nodes.size() - 1;
			}

			// Split at the median of the centres along the axis of their widest spread.
			double low[3] = { 2, 2, 2 };
			double high[3] = { -2, -2, -2 };
			for (unsigned int i = begin; i < end; ++i)
			{
				const UnitVector3D &centre = d_circles[d_order[i]].centre;
				const double coordinates[3] = {
						centre.x().dval(), centre.y().dval(), centre.z().dval() };
				for (unsigned int k = 0; k < 3; ++k)
				{
					low[k] = (std::min)(low[k], coordinates[k]);
					high[k] = (std::max)(high[k], coordinates[k]);
				}
			}
			unsigned int axis = 0;
			for (unsigned int k = 1; k < 3; ++k)
			{
				if (high[k] - low[k] > high[axis] - low[axis])
				{
					axis = k;
				}
			}
			const auto coordinate = [&](unsigned int c)
			{
				const UnitVector3D &centre = d_circles[c].centre;
				return (axis == 0) ? centre.x().dval() :
						(axis == 1) ? centre.y().dval() : centre.z().dval();
			};
			const unsigned int middle = begin + (end - begin) / 2;
			std::nth_element(
					d_order.begin() + begin,
					d_order.begin() + middle,
					d_order.begin() + end,
					[&](unsigned int a, unsigned int b)
					{
						return coordinate(a) < coordinate(b);
					});

			const unsigned int first = build(begin, middle);
			const unsigned int second = build(middle, end);
			d_nodes.push_back(make_parent(first, second));
			return d_nodes.size() - 1;
		}


		//! A leaf enclosing the circles @a d_order[begin, end).
		Node
		make_leaf(
				unsigned int begin,
				unsigned int end) const
		{
			double sum[3] = { 0, 0, 0 };
			for (unsigned int i = begin; i < end; ++i)
			{
				const UnitVector3D &centre = d_circles[d_order[i]].centre;
				sum[0] += centre.x().dval();
				sum[1] += centre.y().dval();
				sum[2] += centre.z().dval();
			}
			const UnitVector3D centre =
					get_mean_direction(sum, end - begin, d_circles[d_order[begin]].centre);

			double radius = 0;
			for (unsigned int i = begin; i < end; ++i)
			{
				const CircleSweep::Circle &circle = d_circles[d_order[i]];
				radius = (std::max)(radius, angle_between(centre, circle.centre) + circle.radius);
			}
			return make_node(centre, radius, sum, begin, end, true, 0, 0);
		}


		//! A node enclosing its two children's circles.
		Node
		make_parent(
				unsigned int first,
				unsigned int second) const
		{
			const Node &a = d_nodes[first];
			const Node &b = d_nodes[second];
			const double sum[3] = { a.sum[0] + b.sum[0], a.sum[1] + b.sum[1], a.sum[2] + b.sum[2] };
			const UnitVector3D centre = get_mean_direction(sum, b.end - a.begin, a.centre);
			const double radius = (std::max)(
					angle_between(centre, a.centre) + a.radius,
					angle_between(centre, b.centre) + b.radius);
			return make_node(centre, radius, sum, a.begin, b.end, false, first, second);
		}


		static
		Node
		make_node(
				const UnitVector3D &centre,
				double radius,
				const double sum[3],
				unsigned int begin,
				unsigned int end,
				bool leaf,
				unsigned int first_child,
				unsigned int second_child)
		{
			radius = (std::min)(radius + NODE_RADIUS_MARGIN, GPlatesMaths::PI);
			return Node{
					centre,
					radius,
					Reach(radius),
					{ sum[0], sum[1], sum[2] },
					begin,
					end,
					leaf,
					{ first_child, second_child } };
		}


		//! The direction of @a sum (of @a count unit vectors), or @a fallback if they cancel.
		static
		UnitVector3D
		get_mean_direction(
				const double sum[3],
				unsigned int count,
				const UnitVector3D &fallback)
		{
			const double length = std::sqrt(sum[0] * sum[0] + sum[1] * sum[1] + sum[2] * sum[2]);
			return (length > 1e-6 * count)
					? UnitVector3D(sum[0] / length, sum[1] / length, sum[2] / length)
					: fallback;
		}
	};
}


GPlatesMaths::CircleSweep::CircleSweep(
		const std::vector<Circle> &circles,
		unsigned int max_sweep_pairs_per_circle)
{
	if (circles.size() < 2)
	{
		return;
	}

	// The sweep's order: the circles' ranks by the beginnings of their intervals.
	const std::pair<std::vector<Interval>, unsigned long long> sweep = get_sweep_intervals(circles);
	const std::vector<Interval> &intervals = sweep.first;

	std::vector<Reach> reaches;
	reaches.reserve(circles.size());
	for (const Circle &circle : circles)
	{
		reaches.push_back(Reach(circle.radius));
	}

	// Few pairs: the sweep's own, which are in order, less those that do not meet.
	const unsigned long long max_sweep_pairs =
			static_cast<unsigned long long>(max_sweep_pairs_per_circle) * circles.size();
	if (sweep.second <= max_sweep_pairs)
	{
		for (unsigned int i = 0; i < intervals.size(); ++i)
		{
			const unsigned int a = intervals[i].index;
			const unsigned int end = end_of_overlaps(intervals, i);
			for (unsigned int j = i + 1; j < end; ++j)
			{
				const unsigned int b = intervals[j].index;
				if (do_circles_meet(circles[a].centre, reaches[a], circles[b].centre, reaches[b]))
				{
					d_pairs.emplace_back(a, b);
				}
			}
		}
		return;
	}

	// Many: the pairs that meet, from the tree, as ranks (the first the lower), each kept only if
	// the sweep would visit it: the higher's interval begins within the lower's. (Circles that
	// meet overlap on every axis, but a pair that meets only within the slack can miss on the
	// sweep's axis by as much: this keeps exactly the sweep's pairs, as the sweep's path does.)
	std::vector<unsigned int> rank(circles.size());
	for (unsigned int r = 0; r < intervals.size(); ++r)
	{
		rank[intervals[r].index] = r;
	}
	std::vector<std::pair<unsigned int, unsigned int>> ranked_pairs;
	CircleTree(circles, reaches).visit_meeting_pairs(
			[&](unsigned int a, unsigned int b)
			{
				unsigned int first = rank[a];
				unsigned int second = rank[b];
				if (first > second)
				{
					std::swap(first, second);
				}
				if (!(intervals[first].end < intervals[second].begin))
				{
					ranked_pairs.emplace_back(first, second);
				}
			});

	// In the sweep's order, as the circles' indices (in place: where most circles meet, the pairs
	// are many, and one copy of them is enough).
	std::sort(ranked_pairs.begin(), ranked_pairs.end());
	for (std::pair<unsigned int, unsigned int> &pair : ranked_pairs)
	{
		pair = std::make_pair(intervals[pair.first].index, intervals[pair.second].index);
	}
	d_pairs.swap(ranked_pairs);
}
