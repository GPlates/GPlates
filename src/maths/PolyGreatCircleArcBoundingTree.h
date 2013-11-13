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
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

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
	 */
	template <typename GreatCircleArcConstIteratorType>
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
					unsigned int num_bounded_great_circle_arcs_) :
				bounding_small_circle(bounding_small_circle_),
				num_bounded_great_circle_arcs(num_bounded_great_circle_arcs_)
			{
				// Default is no children.
				child_node_indices[0] = child_node_indices[1] = INVALID_NODE_INDEX;
			}

			NodeImpl(
					const BoundingSmallCircle &bounding_small_circle_,
					unsigned int num_bounded_great_circle_arcs_,
					unsigned int first_child_node_index,
					unsigned int second_child_node_index) :
				bounding_small_circle(bounding_small_circle_),
				num_bounded_great_circle_arcs(num_bounded_great_circle_arcs_)
			{
				child_node_indices[0] = first_child_node_index;
				child_node_indices[1] = second_child_node_index;
			}

			//! The small circle that bounds the current node (and the great circle arcs within).
			BoundingSmallCircle bounding_small_circle;

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
			const great_circle_arc_const_iterator_type &
			get_bounded_great_circle_arcs_begin() const
			{
				return d_bounded_great_circle_arcs_begin;
			}

			/**
			 * Returns the end iterator over the contiguous sequence of great circle arcs bounded by this node.
			 */
			const great_circle_arc_const_iterator_type &
			get_bounded_great_circle_arcs_end() const
			{
				return d_bounded_great_circle_arcs_end;
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

			// NOTE: We save memory if we place the iterators (each iterator is size 8) here
			// rather than in NodeImpl.
			great_circle_arc_const_iterator_type d_bounded_great_circle_arcs_begin;
			great_circle_arc_const_iterator_type d_bounded_great_circle_arcs_end;


			Node(
					const NodeImpl &node_impl,
					const great_circle_arc_const_iterator_type &bounded_great_circle_arcs_begin,
					const great_circle_arc_const_iterator_type &bounded_great_circle_arcs_end) :
				d_node_impl(&node_impl),
				d_bounded_great_circle_arcs_begin(bounded_great_circle_arcs_begin),
				d_bounded_great_circle_arcs_end(bounded_great_circle_arcs_end)
			{
			}

			// Make a friend so can construct and access node implementation.
			friend class PolyGreatCircleArcBoundingTree<GreatCircleArcConstIteratorType>;
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
		 * NOTE: You should check node_type::is_leaf_node() before calling this to ensure @a parent_node has child nodes.
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
				GreatCircleArcConstIteratorType begin_node_great_circle_arcs,
				unsigned int num_node_great_circle_arcs,
				unsigned int max_num_node_great_circle_arcs_per_leaf_node);
	};


	////////////////////
	// Implementation //
	////////////////////


	template <typename GreatCircleArcConstIteratorType>
	PolyGreatCircleArcBoundingTree<GreatCircleArcConstIteratorType>::PolyGreatCircleArcBoundingTree(
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


	template <typename GreatCircleArcConstIteratorType>
	PolyGreatCircleArcBoundingTree<GreatCircleArcConstIteratorType>::PolyGreatCircleArcBoundingTree(
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


	template <typename GreatCircleArcConstIteratorType>
	PolyGreatCircleArcBoundingTree<GreatCircleArcConstIteratorType>::PolyGreatCircleArcBoundingTree(
			GreatCircleArcConstIteratorType begin_great_circle_arcs,
			GreatCircleArcConstIteratorType end_great_circle_arcs,
			unsigned int max_num_node_great_circle_arcs_per_leaf_node)
	{
		initialise(begin_great_circle_arcs, end_great_circle_arcs, max_num_node_great_circle_arcs_per_leaf_node);
	}


	template <typename GreatCircleArcConstIteratorType>
	void
	PolyGreatCircleArcBoundingTree<GreatCircleArcConstIteratorType>::initialise(
			GreatCircleArcConstIteratorType begin_great_circle_arcs,
			GreatCircleArcConstIteratorType end_great_circle_arcs,
			unsigned int max_num_node_great_circle_arcs_per_leaf_node)
	{
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
						d_begin_great_circle_arcs,
						num_great_circle_arcs,
						max_num_node_great_circle_arcs_per_leaf_node);
	}


	template <typename GreatCircleArcConstIteratorType>
	unsigned int
	PolyGreatCircleArcBoundingTree<GreatCircleArcConstIteratorType>::create_node(
			GreatCircleArcConstIteratorType begin_node_great_circle_arcs,
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
			GreatCircleArcConstIteratorType first_child_node_great_circle_arcs_iter =
					begin_node_great_circle_arcs;
			const unsigned int first_child_node_num_node_great_circle_arcs =
					num_node_great_circle_arcs >> 1;
			const unsigned int first_child_node_index = create_node(
					first_child_node_great_circle_arcs_iter,
					first_child_node_num_node_great_circle_arcs,
					max_num_node_great_circle_arcs_per_leaf_node);

			// Create the second child node.
			GreatCircleArcConstIteratorType second_child_node_great_circle_arcs_iter =
					first_child_node_great_circle_arcs_iter;
			std::advance(second_child_node_great_circle_arcs_iter, first_child_node_num_node_great_circle_arcs);
			const unsigned int second_child_node_num_node_great_circle_arcs =
					num_node_great_circle_arcs - first_child_node_num_node_great_circle_arcs;
			const unsigned int second_child_node_index = create_node(
					second_child_node_great_circle_arcs_iter,
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
							num_node_great_circle_arcs,
							first_child_node_index,
							second_child_node_index));

			return node_index;
		}

		// Create a leaf node.

		GreatCircleArcConstIteratorType end_node_great_circle_arcs = begin_node_great_circle_arcs;
		std::advance(end_node_great_circle_arcs, num_node_great_circle_arcs);

		// Use the centroid of the node edges as the centre of the node's bounding small circle.
		BoundingSmallCircleBuilder bounding_small_circle_builder(
				Centroid::calculate_edges_centroid(begin_node_great_circle_arcs, end_node_great_circle_arcs));

		// Add the edges (great circle arcs) for the current node.
		bounding_small_circle_builder.add(begin_node_great_circle_arcs, end_node_great_circle_arcs);

		const unsigned int node_index = d_nodes.size();

		d_nodes.push_back(
				NodeImpl(
						bounding_small_circle_builder.get_bounding_small_circle(),
						num_node_great_circle_arcs));

		return node_index;
	}


	template <typename GreatCircleArcConstIteratorType>
	typename PolyGreatCircleArcBoundingTree<GreatCircleArcConstIteratorType>::node_type
	PolyGreatCircleArcBoundingTree<GreatCircleArcConstIteratorType>::get_root_node() const
	{
		const NodeImpl &root_node_impl = d_nodes[d_root_node_index];

		return node_type(
				root_node_impl,
				d_begin_great_circle_arcs,
				// We're expecting a random access iterator since much faster - will fail to compile otherwise...
				d_begin_great_circle_arcs + root_node_impl.num_bounded_great_circle_arcs);
	}


	template <typename GreatCircleArcConstIteratorType>
	typename PolyGreatCircleArcBoundingTree<GreatCircleArcConstIteratorType>::node_type
	PolyGreatCircleArcBoundingTree<GreatCircleArcConstIteratorType>::get_child_node(
			const node_type &parent_node,
			unsigned int child_offset) const
	{
		//GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
		//		child_offset < 2 && !parent_node.is_leaf_node(),
		//		GPLATES_ASSERTION_SOURCE);

		if (child_offset == 0)
		{
			const int first_child_node_index = parent_node.d_node_impl->child_node_indices[0];
			const NodeImpl &first_child_node_impl = d_nodes[first_child_node_index];

			return node_type(
					first_child_node_impl,
					parent_node.d_bounded_great_circle_arcs_begin,
					// We're expecting a random access iterator since much faster - will fail to compile otherwise...
					parent_node.d_bounded_great_circle_arcs_begin +
							first_child_node_impl.num_bounded_great_circle_arcs);
		}
		else // child_offset == 1
		{
			const int first_child_node_index = parent_node.d_node_impl->child_node_indices[0];
			const NodeImpl &first_child_node_impl = d_nodes[first_child_node_index];
			const int second_child_node_index = parent_node.d_node_impl->child_node_indices[1];
			const NodeImpl &second_child_node_impl = d_nodes[second_child_node_index];

			return node_type(
					second_child_node_impl,
					// We're expecting a random access iterator since much faster - will fail to compile otherwise...
					parent_node.d_bounded_great_circle_arcs_begin +
							first_child_node_impl.num_bounded_great_circle_arcs,
					// We're expecting a random access iterator since much faster - will fail to compile otherwise...
					parent_node.d_bounded_great_circle_arcs_begin +
							parent_node.d_node_impl->num_bounded_great_circle_arcs);
		}
	}
}

#endif // GPLATES_MATHS_POLYGREATCIRCLEARCBOUNDINGTREE_H
