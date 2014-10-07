/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_MATHS_POLYGREATCIRCLEARCBOUNDINGTREE_H
#define GPLATES_MATHS_POLYGREATCIRCLEARCBOUNDINGTREE_H

#include <iterator>  // std::distance, std::advance
#include <utility>
#include <boost/iterator/iterator_traits.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>

#include "Centroid.h"
#include "GeometryOnSphere.h"
#include "PolygonOnSphere.h"
#include "PolylineOnSphere.h"
#include "SmallCircleBounds.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


namespace GPlatesMaths
{
	/**
	 * A small circle bounding tree over a linear sequence of great circle arcs.
	 *
	 * This is used by both @a PolylineOnSphere and @a PolygonOnSphere to allows clients of
	 * those classes to improve performance of queries such as intersection testing.
	 *
	 * @a GreatCircleArcConstIteratorType should be iterable over @a GreatCircleArc.
	 * Typically this is 'PolylineOnSphere::const_iterator' or 'PolygonOnSphere::const_iterator'.
	 *
	 * Note that GreatCircleArcConstIteratorType should ideally be a random-access iterator in all
	 * cases, otherwise construction and querying will be slow (due to inability of std::advance()
	 * to efficiently skip over large numbers of great circle arcs).
	 *
	 * If @a RequireRandomAccessIterator is true (the default) then @a GreatCircleArcConstIteratorType
	 * must be a random-access iterator otherwise the code will not compile.
	 * This is really to catch cases where a non-random-access iterator is used and force the user
	 * to explicitly set @a RequireRandomAccessIterator to false as an acknowledgement that the
	 * code will run slowly.
	 */
	template <
			typename GreatCircleArcConstIteratorType,
			bool RequireRandomAccessIterator = true>
	class PolyGreatCircleArcBoundingTree :
			private boost::noncopyable
	{
	private:
		/**
		 * A binary tree node containing implementation details not needed by the client.
		 */
		struct NodeImpl
		{
			NodeImpl(
					const BoundingSmallCircle &bounding_small_circle_,
					unsigned int bounded_great_circle_arcs_begin_index_,
					unsigned int num_bounded_great_circle_arcs_) :
				bounding_small_circle(bounding_small_circle_),
				bounded_great_circle_arcs_begin_index(bounded_great_circle_arcs_begin_index_),
				num_bounded_great_circle_arcs(num_bounded_great_circle_arcs_)
			{
				// Default is no children.
				child_node_indices[0] = child_node_indices[1] = INVALID_NODE_INDEX;
			}

			NodeImpl(
					const BoundingSmallCircle &bounding_small_circle_,
					unsigned int bounded_great_circle_arcs_begin_index_,
					unsigned int num_bounded_great_circle_arcs_,
					unsigned int first_child_node_index,
					unsigned int second_child_node_index) :
				bounding_small_circle(bounding_small_circle_),
				bounded_great_circle_arcs_begin_index(bounded_great_circle_arcs_begin_index_),
				num_bounded_great_circle_arcs(num_bounded_great_circle_arcs_)
			{
				child_node_indices[0] = first_child_node_index;
				child_node_indices[1] = second_child_node_index;
			}

			//! The small circle that bounds the current node (and the great circle arcs within).
			BoundingSmallCircle bounding_small_circle;

			//! The index of the first great circle arc bounded by this node.
			unsigned int bounded_great_circle_arcs_begin_index;

			//! The number of great circle arcs bounded by this node.
			unsigned int num_bounded_great_circle_arcs;

			/**
			 * The two child node indices (an invalid index means no child).
			 *
			 * NOTE: Either both children exist or neither.
			 * So if either child index is invalid then 'this' node is a leaf node.
			 */
			int child_node_indices[2];
		};

		typedef std::vector<NodeImpl> node_impl_seq_type;

	public:

		typedef GreatCircleArcConstIteratorType great_circle_arc_const_iterator_type;

		/**
		 * The default value for the maximum number of great circles arcs to bound at leaf nodes.
		 */
		static const unsigned int DEFAULT_MAX_NUM_NODE_GREAT_CIRCLE_ARCS_PER_LEAF_NODE = 8;


		/**
		 * A node of the binary bounding tree.
		 */
		class Node
		{
		public:
			/**
			 * Returns the small circle that bounds the current node (and the great circle arcs within).
			 */
			const BoundingSmallCircle &
			get_bounding_small_circle() const
			{
				return d_node_impl->bounding_small_circle;
			}

			/**
			 * Returns the begin iterator over the contiguous sequence of great circle arcs bounded by this node.
			 */
			great_circle_arc_const_iterator_type
			get_bounded_great_circle_arcs_begin() const
			{
				GreatCircleArcConstIteratorType bounded_great_circle_arcs_begin = d_begin_great_circle_arcs;
				std::advance(bounded_great_circle_arcs_begin, get_bounded_great_circle_arcs_begin_index());

				return bounded_great_circle_arcs_begin;
			}

			/**
			 * Returns the end iterator over the contiguous sequence of great circle arcs bounded by this node.
			 */
			great_circle_arc_const_iterator_type
			get_bounded_great_circle_arcs_end() const
			{
				GreatCircleArcConstIteratorType bounded_great_circle_arcs_end = d_begin_great_circle_arcs;
				std::advance(bounded_great_circle_arcs_end,
						get_bounded_great_circle_arcs_begin_index() + get_num_bounded_great_circle_arcs());

				return bounded_great_circle_arcs_end;
			}

			/**
			 * Returns the index of the first great circle arc bounded by this node.
			 *
			 * This is the index into the PolylineOnSphere or PolygonOnSphere or sequence of
			 * great circle arcs passed into PolyGreatCircleArcBoundingTree constructor.
			 */
			unsigned int
			get_bounded_great_circle_arcs_begin_index() const
			{
				return d_node_impl->bounded_great_circle_arcs_begin_index;
			}

			/**
			 * Returns the number of great circle arcs bounded by this node.
			 */
			unsigned int
			get_num_bounded_great_circle_arcs() const
			{
				return d_node_impl->num_bounded_great_circle_arcs;
			}

			/**
			 * Returns true if this node has no children.
			 *
			 * If false is returned then 'this' node has two children.
			 */
			bool
			is_leaf_node() const
			{
				// If either child index is invalid then 'this' node is a leaf node.
				return d_node_impl->child_node_indices[0] == INVALID_NODE_INDEX;
			}

			/**
			 * Returns true if this node has children.
			 *
			 * If true is returned then 'this' node has two children.
			 */
			bool
			is_internal_node() const
			{
				return !is_leaf_node();
			}

		private:
			const NodeImpl *d_node_impl;

			//! Iterator to beginning of *entire* sequence of GCAs (not just the ones bounded by this node).
			great_circle_arc_const_iterator_type d_begin_great_circle_arcs;


			Node(
					const NodeImpl &node_impl,
					great_circle_arc_const_iterator_type begin_great_circle_arcs) :
				d_node_impl(&node_impl),
				d_begin_great_circle_arcs(begin_great_circle_arcs)
			{
			}

			// Make a friend so can construct and access node implementation.
			friend class PolyGreatCircleArcBoundingTree<
					GreatCircleArcConstIteratorType,
					RequireRandomAccessIterator>;
		};

		//! Typedef for bounding tree node.
		typedef Node node_type;


		/**
		 * Constructs a binary bounding tree over the great circle arcs of a polyline.
		 *
		 * @param max_num_node_great_circle_arcs_per_leaf_node the maximum number of great circles arcs
		 * to bound at each leaf node - ensures that each leaf node will bound at least this number
		 * of great circle arcs.
		 *
		 * If @a keep_shared_reference_to_polyline is true then a shared pointer to @a polyline
		 * is kept internally in order to ensure the sequence of great circle arcs (inside the polyline)
		 * remain alive for the lifetime of the newly constructed bounding tree.
		 * This is set to false by PolylineOnSphere itself since it has a shared pointer to us
		 * (otherwise we'd get a memory island and hence a memory leak).
		 */
		PolyGreatCircleArcBoundingTree(
				const PolylineOnSphere::non_null_ptr_to_const_type &polyline,
				bool keep_shared_reference_to_polyline = true,
				unsigned int max_num_node_great_circle_arcs_per_leaf_node =
						DEFAULT_MAX_NUM_NODE_GREAT_CIRCLE_ARCS_PER_LEAF_NODE);


		/**
		 * Constructs a binary bounding tree over the great circle arcs of a polygon.
		 *
		 * @param max_num_node_great_circle_arcs_per_leaf_node the maximum number of great circles arcs
		 * to bound at each leaf node - ensures that each leaf node will bound at least this number
		 * of great circle arcs.
		 *
		 * If @a keep_shared_reference_to_polygon is true then a shared pointer to @a polygon
		 * is kept internally in order to ensure the sequence of great circle arcs (inside the polygon)
		 * remain alive for the lifetime of the newly constructed bounding tree.
		 * This is set to false by PolygonOnSphere itself since it has a shared pointer to us
		 * (otherwise we'd get a memory island and hence a memory leak).
		 */
		PolyGreatCircleArcBoundingTree(
				const PolygonOnSphere::non_null_ptr_to_const_type &polygon,
				bool keep_shared_reference_to_polygon = true,
				unsigned int max_num_node_great_circle_arcs_per_leaf_node =
						DEFAULT_MAX_NUM_NODE_GREAT_CIRCLE_ARCS_PER_LEAF_NODE);


		/**
		 * Constructs a binary bounding tree over the specified iteration sequence of great circle arcs.
		 *
		 * @param max_num_node_great_circle_arcs_per_leaf_node the maximum number of great circles arcs
		 * to bound at each leaf node - ensures that each leaf node will bound at least this number
		 * of great circle arcs.
		 *
		 * NOTE: It is the caller's responsibility to ensure the sequence of great circle arcs
		 * (in the iterable range) remain alive for the lifetime of the newly constructed bounding tree.
		 */
		PolyGreatCircleArcBoundingTree(
				GreatCircleArcConstIteratorType begin_great_circle_arcs,
				GreatCircleArcConstIteratorType end_great_circle_arcs,
				unsigned int max_num_node_great_circle_arcs_per_leaf_node =
						DEFAULT_MAX_NUM_NODE_GREAT_CIRCLE_ARCS_PER_LEAF_NODE);


		/**
		 * Returns the root node of the binary bounding tree.
		 */
		node_type
		get_root_node() const;


		/**
		 * Returns the specified child node of the specified parent node.
		 *
		 * NOTE: You should check node_type::is_internal_node() before calling this to ensure @a parent_node has child nodes.
		 *
		 * @a child_offset should be either 0 or 1.
		 */
		node_type
		get_child_node(
				const node_type &parent_node,
				unsigned int child_offset) const;

	private:

		/**
		 * Index used to test if a node has children or not.
		 */
		static const int INVALID_NODE_INDEX = -1;

		node_impl_seq_type d_nodes;
		unsigned int d_root_node_index;
		great_circle_arc_const_iterator_type d_begin_great_circle_arcs;

		/**
		 * A reference to ensure the owner of the great circle arcs stays alive because we are
		 * storing iterators into its internal structures.
		 *
		 * This is optional because the polyline/polygon itself might be caching *us* in
		 * which case we would have circular shared pointers causing a memory leak.
		 */
		boost::optional<GeometryOnSphere::non_null_ptr_to_const_type> d_geometry_shared_pointer;


		void
		initialise(
				GreatCircleArcConstIteratorType begin_great_circle_arcs,
				GreatCircleArcConstIteratorType end_great_circle_arcs,
				unsigned int max_num_node_great_circle_arcs_per_leaf_node);

		unsigned int
		create_node(
				unsigned int begin_node_great_circle_arcs_index,
				unsigned int num_node_great_circle_arcs,
				unsigned int max_num_node_great_circle_arcs_per_leaf_node);
	};


	////////////////////
	// Implementation //
	////////////////////


	template <typename GreatCircleArcConstIteratorType, bool RequireRandomAccessIterator>
	PolyGreatCircleArcBoundingTree<GreatCircleArcConstIteratorType, RequireRandomAccessIterator>::PolyGreatCircleArcBoundingTree(
			const PolylineOnSphere::non_null_ptr_to_const_type &polyline,
			bool keep_shared_reference_to_polyline,
			unsigned int max_num_node_great_circle_arcs_per_leaf_node)
	{
		initialise(polyline->begin(), polyline->end(), max_num_node_great_circle_arcs_per_leaf_node);

		if (keep_shared_reference_to_polyline)
		{
			d_geometry_shared_pointer = GeometryOnSphere::non_null_ptr_to_const_type(polyline);
		}
	}


	template <typename GreatCircleArcConstIteratorType, bool RequireRandomAccessIterator>
	PolyGreatCircleArcBoundingTree<GreatCircleArcConstIteratorType, RequireRandomAccessIterator>::PolyGreatCircleArcBoundingTree(
			const PolygonOnSphere::non_null_ptr_to_const_type &polygon,
			bool keep_shared_reference_to_polygon,
			unsigned int max_num_node_great_circle_arcs_per_leaf_node)
	{
		initialise(polygon->begin(), polygon->end(), max_num_node_great_circle_arcs_per_leaf_node);

		if (keep_shared_reference_to_polygon)
		{
			d_geometry_shared_pointer = GeometryOnSphere::non_null_ptr_to_const_type(polygon);
		}
	}


	template <typename GreatCircleArcConstIteratorType, bool RequireRandomAccessIterator>
	PolyGreatCircleArcBoundingTree<GreatCircleArcConstIteratorType, RequireRandomAccessIterator>::PolyGreatCircleArcBoundingTree(
			GreatCircleArcConstIteratorType begin_great_circle_arcs,
			GreatCircleArcConstIteratorType end_great_circle_arcs,
			unsigned int max_num_node_great_circle_arcs_per_leaf_node)
	{
		initialise(begin_great_circle_arcs, end_great_circle_arcs, max_num_node_great_circle_arcs_per_leaf_node);
	}


	template <typename GreatCircleArcConstIteratorType, bool RequireRandomAccessIterator>
	void
	PolyGreatCircleArcBoundingTree<GreatCircleArcConstIteratorType, RequireRandomAccessIterator>::initialise(
			GreatCircleArcConstIteratorType begin_great_circle_arcs,
			GreatCircleArcConstIteratorType end_great_circle_arcs,
			unsigned int max_num_node_great_circle_arcs_per_leaf_node)
	{
		// Fail to compile if caller is supposed to be providing a random-access iterator but doesn't.
		BOOST_STATIC_ASSERT(
				!RequireRandomAccessIterator ||
					(boost::is_same<
							boost::iterator_category<GreatCircleArcConstIteratorType>::type,
							std::random_access_iterator_tag>::value));

		d_root_node_index = 0;
		d_begin_great_circle_arcs = begin_great_circle_arcs;

		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				begin_great_circle_arcs != end_great_circle_arcs &&
					max_num_node_great_circle_arcs_per_leaf_node != 0,
				GPLATES_ASSERTION_SOURCE);

		const unsigned int num_great_circle_arcs =
				std::distance(begin_great_circle_arcs, end_great_circle_arcs);

		// Start a recursion to build the binary tree.
		d_root_node_index =
				create_node(
						0/*begin_node_great_circle_arcs_index*/,
						num_great_circle_arcs,
						max_num_node_great_circle_arcs_per_leaf_node);
	}


	template <typename GreatCircleArcConstIteratorType, bool RequireRandomAccessIterator>
	unsigned int
	PolyGreatCircleArcBoundingTree<GreatCircleArcConstIteratorType, RequireRandomAccessIterator>::create_node(
			unsigned int begin_node_great_circle_arcs_index,
			unsigned int num_node_great_circle_arcs,
			unsigned int max_num_node_great_circle_arcs_per_leaf_node)
	{
		// If the number of great circle arcs exceeds a limit then create two child nodes.
		if (num_node_great_circle_arcs > max_num_node_great_circle_arcs_per_leaf_node)
		{
			// Currently just divide the arcs equally between both child nodes.
			// This assumes a fairly uniform spacing of arcs which is not always the case so
			// a better algorithm could be used here if it makes a noticeable difference.

			// Create the first child node.
			const unsigned int first_child_begin_node_great_circle_arcs_index =
					begin_node_great_circle_arcs_index;
			const unsigned int first_child_node_num_node_great_circle_arcs =
					num_node_great_circle_arcs >> 1;
			const unsigned int first_child_node_index = create_node(
					first_child_begin_node_great_circle_arcs_index,
					first_child_node_num_node_great_circle_arcs,
					max_num_node_great_circle_arcs_per_leaf_node);

			// Create the second child node.
			const unsigned int second_child_begin_node_great_circle_arcs_index =
					first_child_begin_node_great_circle_arcs_index + first_child_node_num_node_great_circle_arcs;
			const unsigned int second_child_node_num_node_great_circle_arcs =
					num_node_great_circle_arcs - first_child_node_num_node_great_circle_arcs;
			const unsigned int second_child_node_index = create_node(
					second_child_begin_node_great_circle_arcs_index,
					second_child_node_num_node_great_circle_arcs,
					max_num_node_great_circle_arcs_per_leaf_node);

			// NOTE: This must be done after creating the child nodes since the 'd_nodes'
			// vector can get reallocated as nodes are created and added.
			const NodeImpl &first_child_node_impl = d_nodes[first_child_node_index];
			const NodeImpl &second_child_node_impl = d_nodes[second_child_node_index];

			const unsigned int node_index = d_nodes.size();

			// Create the interior node that bounds both the first and second child nodes.
			d_nodes.push_back(
					NodeImpl(
							create_optimal_bounding_small_circle(
									first_child_node_impl.bounding_small_circle,
									second_child_node_impl.bounding_small_circle),
							begin_node_great_circle_arcs_index,
							num_node_great_circle_arcs,
							first_child_node_index,
							second_child_node_index));

			return node_index;
		}

		// Create a leaf node.

		GreatCircleArcConstIteratorType begin_node_great_circle_arcs = d_begin_great_circle_arcs;
		std::advance(begin_node_great_circle_arcs, begin_node_great_circle_arcs_index);

		GreatCircleArcConstIteratorType end_node_great_circle_arcs = d_begin_great_circle_arcs;
		std::advance(end_node_great_circle_arcs, begin_node_great_circle_arcs_index + num_node_great_circle_arcs);

		// Use the centroid of the node edges as the centre of the node's bounding small circle.
		BoundingSmallCircleBuilder bounding_small_circle_builder(
				Centroid::calculate_outline_centroid(begin_node_great_circle_arcs, end_node_great_circle_arcs));

		// Add the edges (great circle arcs) for the current node.
		bounding_small_circle_builder.add(begin_node_great_circle_arcs, end_node_great_circle_arcs);

		const unsigned int node_index = d_nodes.size();

		d_nodes.push_back(
				NodeImpl(
						bounding_small_circle_builder.get_bounding_small_circle(),
						begin_node_great_circle_arcs_index,
						num_node_great_circle_arcs));

		return node_index;
	}


	template <typename GreatCircleArcConstIteratorType, bool RequireRandomAccessIterator>
	typename PolyGreatCircleArcBoundingTree<GreatCircleArcConstIteratorType, RequireRandomAccessIterator>::node_type
	PolyGreatCircleArcBoundingTree<GreatCircleArcConstIteratorType, RequireRandomAccessIterator>::get_root_node() const
	{
		const NodeImpl &root_node_impl = d_nodes[d_root_node_index];

		return node_type(root_node_impl, d_begin_great_circle_arcs);
	}


	template <typename GreatCircleArcConstIteratorType, bool RequireRandomAccessIterator>
	typename PolyGreatCircleArcBoundingTree<GreatCircleArcConstIteratorType, RequireRandomAccessIterator>::node_type
	PolyGreatCircleArcBoundingTree<GreatCircleArcConstIteratorType, RequireRandomAccessIterator>::get_child_node(
			const node_type &parent_node,
			unsigned int child_offset) const
	{
		//GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
		//		child_offset < 2 && parent_node.is_internal_node(),
		//		GPLATES_ASSERTION_SOURCE);

		const int child_node_index = parent_node.d_node_impl->child_node_indices[child_offset];
		const NodeImpl &child_node_impl = d_nodes[child_node_index];

		return node_type(child_node_impl, d_begin_great_circle_arcs);
	}
}

#endif // GPLATES_MATHS_POLYGREATCIRCLEARCBOUNDINGTREE_H
