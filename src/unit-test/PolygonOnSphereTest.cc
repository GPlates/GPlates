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
#include <random>
#include <string>
#include <tuple>
#include <vector>
#include <gtest/gtest.h>

#include "global/PreconditionViolationError.h"

#include "maths/MathsUtils.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolyGreatCircleArcBoundingTree.h"
#include "maths/UnitVector3D.h"


//
// Tests of PolygonOnSphere's iteration over all its rings, and of the shape of its bounding tree.
//
// The iterator over all rings is random access, and a polygon's bounding tree advances it from
// the first arc for every leaf it builds and every leaf a query reaches, so advancing (and
// differencing) must not walk the rings: they find the ring by a table of the rings' offsets
// (a jump into another ring by bisection). These tests check the jumps and differences
// against a walk by increments, and the tree's shape against the order of merges it has always
// had (its shape decides the order in which GeometryIntersect finds intersections that tie).
//

namespace
{
	using GPlatesMaths::PointOnSphere;
	using GPlatesMaths::PolygonOnSphere;

	typedef std::vector<PointOnSphere> ring_points_type;


	/**
	 * A ring of @a num_points on a small circle, of a degree's radius, around a centre that
	 * depends on @a ring_index (so that no two rings are the same; they need not be nested).
	 */
	ring_points_type
	make_ring(
			unsigned int ring_index,
			unsigned int num_points)
	{
		const double centre_lat = GPlatesMaths::convert_deg_to_rad(-60.0 + 3.0 * (ring_index % 40));
		const unsigned int column = ring_index / 40;
		const double centre_lon = GPlatesMaths::convert_deg_to_rad(-170.0 + 4.0 * column);
		const double radius = GPlatesMaths::convert_deg_to_rad(1.0);

		ring_points_type points;
		for (unsigned int i = 0; i < num_points; ++i)
		{
			const double angle = 2.0 * GPlatesMaths::PI * i / num_points;
			const double lat = centre_lat + radius * std::sin(angle);
			const double lon = centre_lon + radius * std::cos(angle) / std::cos(centre_lat);
			points.push_back(PointOnSphere(GPlatesMaths::UnitVector3D(
					std::cos(lat) * std::cos(lon),
					std::cos(lat) * std::sin(lon),
					std::sin(lat))));
		}
		return points;
	}


	/**
	 * A polygon whose rings have the numbers of points @a ring_sizes (the first is the exterior).
	 */
	PolygonOnSphere::non_null_ptr_to_const_type
	make_polygon(
			const std::vector<unsigned int> &ring_sizes)
	{
		const ring_points_type exterior = make_ring(0, ring_sizes[0]);
		if (ring_sizes.size() == 1)
		{
			return PolygonOnSphere::create(exterior);
		}

		std::vector<ring_points_type> interiors;
		for (unsigned int r = 1; r < ring_sizes.size(); ++r)
		{
			interiors.push_back(make_ring(r, ring_sizes[r]));
		}
		return PolygonOnSphere::create(exterior, interiors.begin(), interiors.end());
	}


	/**
	 * The polygon's segments by walking each ring in turn: the exterior's, then the interiors'.
	 */
	std::vector<const GPlatesMaths::GreatCircleArc *>
	segments_ring_by_ring(
			const PolygonOnSphere &polygon)
	{
		std::vector<const GPlatesMaths::GreatCircleArc *> segments;
		for (auto iter = polygon.exterior_ring_begin(); iter != polygon.exterior_ring_end(); ++iter)
		{
			segments.push_back(&*iter);
		}
		for (unsigned int r = 0; r < polygon.number_of_interior_rings(); ++r)
		{
			const auto end = polygon.interior_ring_end(r);
			for (auto iter = polygon.interior_ring_begin(r); iter != end; ++iter)
			{
				segments.push_back(&*iter);
			}
		}
		return segments;
	}


	void
	check_iterator_against_walk(
			const std::vector<unsigned int> &ring_sizes)
	{
		const PolygonOnSphere::non_null_ptr_to_const_type polygon = make_polygon(ring_sizes);
		const std::vector<const GPlatesMaths::GreatCircleArc *> segments =
				segments_ring_by_ring(*polygon);
		const int num_segments = segments.size();
		ASSERT_EQ(polygon->number_of_segments(), segments.size());

		// The iterator at each position, by increments from the beginning (and the end last).
		std::vector<PolygonOnSphere::const_iterator> walk;
		PolygonOnSphere::const_iterator iter = polygon->begin();
		for (int i = 0; i < num_segments; ++i, ++iter)
		{
			ASSERT_EQ(&*iter, segments[i]) << "segment " << i;
			walk.push_back(iter);
		}
		ASSERT_TRUE(iter == polygon->end());
		walk.push_back(iter);

		// Decrements from the end give the same positions.
		for (int i = num_segments - 1; i >= 0; --i)
		{
			--iter;
			ASSERT_TRUE(iter == walk[i]) << "segment " << i;
		}

		// Every jump, forward and backward, from every position (the end included) to every other.
		for (int from = 0; from <= num_segments; ++from)
		{
			for (int to = 0; to <= num_segments; ++to)
			{
				SCOPED_TRACE("from " + std::to_string(from) + " to " + std::to_string(to));

				PolygonOnSphere::const_iterator jumped = walk[from];
				jumped += to - from;
				ASSERT_TRUE(jumped == walk[to]);
				if (to < num_segments)
				{
					ASSERT_EQ(&*jumped, segments[to]);
				}

				ASSERT_EQ(walk[to] - walk[from], to - from);
			}

			if (from < num_segments)
			{
				ASSERT_TRUE(polygon->segment_iterator(from) == walk[from]);
				ASSERT_EQ(&polygon->get_segment(from), segments[from]);
			}
		}
		ASSERT_TRUE(polygon->segment_iterator(num_segments) == polygon->end());
	}
}


TEST(PolygonOnSphereTest, IteratorJumpsMatchWalk)
{
	// Rings of three points beside larger ones, so that some rings are shorter than most jumps.
	check_iterator_against_walk({ 5, 3, 7, 3, 3, 12, 4, 3 });
}


TEST(PolygonOnSphereTest, IteratorJumpsMatchWalkWithoutInteriorRings)
{
	check_iterator_against_walk({ 6 });
}


TEST(PolygonOnSphereTest, IteratorJumpsMatchWalkWithOneInteriorRing)
{
	check_iterator_against_walk({ 3, 4 });
}


TEST(PolygonOnSphereTest, IteratorRefusesToMoveBeyondEnds)
{
	const PolygonOnSphere::non_null_ptr_to_const_type polygon = make_polygon({ 4, 3, 5 });
	const int num_segments = polygon->number_of_segments();
	ASSERT_EQ(num_segments, 12);

	for (int from = 0; from <= num_segments; ++from)
	{
		SCOPED_TRACE("from " + std::to_string(from));

		PolygonOnSphere::const_iterator iter = polygon->begin();
		iter += from;
		EXPECT_THROW(iter += num_segments - from + 1, GPlatesGlobal::PreconditionViolationError);

		iter = polygon->begin();
		iter += from;
		EXPECT_THROW(iter += -(from + 1), GPlatesGlobal::PreconditionViolationError);
	}

	EXPECT_THROW(
			polygon->segment_iterator(num_segments + 1),
			GPlatesGlobal::PreconditionViolationError);
	EXPECT_THROW(polygon->get_segment(num_segments), GPlatesGlobal::PreconditionViolationError);
}


namespace
{
	//! A node of a bounding tree: its first arc, its number of arcs, and whether it is a leaf.
	typedef std::tuple<unsigned int, unsigned int, bool> node_shape_type;


	/**
	 * The bounding tree's shape by the order of merges it has always had, for comparison:
	 * each ring's arcs halved (the first half the smaller) until a node has at most
	 * @a max_arcs_per_leaf, and then, repeatedly, the first ring sub-tree (in arc order) with the
	 * fewest arcs merged with whichever neighbour has fewer (the right one on a tie), by a linear
	 * search, as before the merges were made by a heap.
	 */
	class ReferenceTree
	{
	public:

		ReferenceTree(
				const std::vector<unsigned int> &ring_sizes,
				unsigned int max_arcs_per_leaf) :
			d_max_arcs_per_leaf(max_arcs_per_leaf)
		{
			std::vector<int> sub_trees;
			unsigned int begin = 0;
			for (unsigned int ring_size : ring_sizes)
			{
				sub_trees.push_back(create_node(begin, ring_size));
				begin += ring_size;
			}

			const auto fewer_arcs = [this](int left, int right)
			{
				return d_nodes[left].count < d_nodes[right].count;
			};
			while (sub_trees.size() >= 2)
			{
				const auto none = sub_trees.end();
				const auto min_iter = std::min_element(sub_trees.begin(), none, fewer_arcs);
				const auto left_iter = (min_iter != sub_trees.begin()) ? min_iter - 1 : none;
				const auto right_iter = (min_iter + 1 != none) ? min_iter + 1 : none;

				std::vector<int>::iterator other_iter;
				if (left_iter != sub_trees.end() && right_iter != sub_trees.end())
				{
					other_iter = fewer_arcs(*left_iter, *right_iter) ? left_iter : right_iter;
				}
				else
				{
					other_iter = (left_iter != sub_trees.end()) ? left_iter : right_iter;
				}

				const int merged = (other_iter == left_iter)
						? add_node(*left_iter, *min_iter)
						: add_node(*min_iter, *right_iter);
				*min_iter = merged;
				sub_trees.erase(other_iter);
			}
			d_root = sub_trees.front();
		}

		std::vector<node_shape_type>
		get_preorder() const
		{
			std::vector<node_shape_type> preorder;
			add_preorder(d_root, preorder);
			return preorder;
		}

	private:

		struct Node
		{
			unsigned int begin;
			unsigned int count;
			int children[2];
		};

		unsigned int d_max_arcs_per_leaf;
		std::vector<Node> d_nodes;
		int d_root;

		int
		create_node(
				unsigned int begin,
				unsigned int count)
		{
			if (count > d_max_arcs_per_leaf)
			{
				const unsigned int first_count = count / 2;
				const int first = create_node(begin, first_count);
				const int second = create_node(begin + first_count, count - first_count);
				return add_node(first, second);
			}
			d_nodes.push_back(Node{ begin, count, { -1, -1 } });
			return d_nodes.size() - 1;
		}

		int
		add_node(
				int first,
				int second)
		{
			const Node node{
					d_nodes[first].begin,
					d_nodes[first].count + d_nodes[second].count,
					{ first, second } };
			d_nodes.push_back(node);
			return d_nodes.size() - 1;
		}

		void
		add_preorder(
				int node_index,
				std::vector<node_shape_type> &preorder) const
		{
			const Node &node = d_nodes[node_index];
			const bool is_leaf = node.children[0] < 0;
			preorder.push_back(node_shape_type(node.begin, node.count, is_leaf));
			if (!is_leaf)
			{
				add_preorder(node.children[0], preorder);
				add_preorder(node.children[1], preorder);
			}
		}
	};


	void
	add_tree_preorder(
			const PolygonOnSphere &polygon,
			const PolygonOnSphere::bounding_tree_type &tree,
			const PolygonOnSphere::bounding_tree_type::node_type &node,
			std::vector<node_shape_type> &preorder)
	{
		preorder.push_back(node_shape_type(
				node.get_bounded_great_circle_arcs_begin_index(),
				node.get_num_bounded_great_circle_arcs(),
				node.is_leaf_node()));

		// A node's arcs are found by jumps from the polygon's first arc.
		ASSERT_TRUE(node.get_bounded_great_circle_arcs_begin() ==
				polygon.segment_iterator(node.get_bounded_great_circle_arcs_begin_index()));
		ASSERT_TRUE(node.get_bounded_great_circle_arcs_end() ==
				polygon.segment_iterator(
						node.get_bounded_great_circle_arcs_begin_index() +
							node.get_num_bounded_great_circle_arcs()));

		if (node.is_internal_node())
		{
			add_tree_preorder(polygon, tree, tree.get_child_node(node, 0), preorder);
			add_tree_preorder(polygon, tree, tree.get_child_node(node, 1), preorder);
		}
	}


	void
	check_tree_shape(
			const std::vector<unsigned int> &ring_sizes)
	{
		const PolygonOnSphere::non_null_ptr_to_const_type polygon = make_polygon(ring_sizes);
		const PolygonOnSphere::bounding_tree_type &tree = polygon->get_bounding_tree();

		std::vector<node_shape_type> preorder;
		add_tree_preorder(*polygon, tree, tree.get_root_node(), preorder);

		typedef PolygonOnSphere::bounding_tree_type tree_type;
		const ReferenceTree reference(
				ring_sizes,
				tree_type::DEFAULT_MAX_NUM_NODE_GREAT_CIRCLE_ARCS_PER_LEAF_NODE);
		ASSERT_EQ(preorder, reference.get_preorder());
	}
}


TEST(PolygonOnSphereTest, BoundingTreeShapeKeepsMergeOrder)
{
	{
		SCOPED_TRACE("one ring");
		check_tree_shape({ 37 });
	}
	{
		SCOPED_TRACE("two rings");
		check_tree_shape({ 20, 9 });
	}
	{
		// Ties for the fewest arcs (the first is taken).
		SCOPED_TRACE("equal rings");
		check_tree_shape(std::vector<unsigned int>(50, 4));
	}
	std::vector<unsigned int> increasing_ring_sizes;
	for (unsigned int size = 3; size <= 40; ++size)
	{
		increasing_ring_sizes.push_back(size);
	}
	{
		SCOPED_TRACE("increasing rings");
		check_tree_shape(increasing_ring_sizes);
	}
	{
		SCOPED_TRACE("decreasing rings");
		check_tree_shape(std::vector<unsigned int>(
				increasing_ring_sizes.rbegin(),
				increasing_ring_sizes.rend()));
	}
	{
		// Each small ring between two equal neighbours (it joins the right one).
		SCOPED_TRACE("alternating rings");
		std::vector<unsigned int> ring_sizes;
		for (unsigned int r = 0; r < 41; ++r)
		{
			ring_sizes.push_back(r % 2 ? 3 : 20);
		}
		check_tree_shape(ring_sizes);
	}
	{
		// The engine's output is fixed by the standard (a distribution's is not), so the sizes are
		// the same on every platform.
		SCOPED_TRACE("random rings");
		std::mt19937 engine(2026);
		std::vector<unsigned int> ring_sizes;
		for (unsigned int r = 0; r < 600; ++r)
		{
			ring_sizes.push_back(3 + engine() % 30);
		}
		check_tree_shape(ring_sizes);
	}
}
