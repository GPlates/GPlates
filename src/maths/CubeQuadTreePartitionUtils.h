/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_MATHS_CUBEQUADTREEPARTITIONUTILS_H
#define GPLATES_MATHS_CUBEQUADTREEPARTITIONUTILS_H

#include <utility>  // std::pair
#include <boost/mpl/if.hpp>
#include <boost/optional.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/utility/in_place_factory.hpp>

#include "CubeCoordinateFrame.h"
#include "CubeQuadTreeLocation.h"
#include "CubeQuadTreePartition.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "utils/IntrusiveSinglyLinkedList.h"


namespace GPlatesMaths
{
	namespace CubeQuadTreePartitionUtils
	{
		/**
		 * Mirrors @a src_spatial_partition into @a dst_spatial_partition such that for each
		 * quad tree node in the former a corresponding quad tree node in the latter is created
		 * if it does not already exist.
		 *
		 * Also the caller can choose what to do with each element using the functions
		 * @a mirror_root_element_function and @a mirror_node_element_function which
		 * specify what to do when mirroring an element in the root of the source
		 * spatial partition and when mirroring an element in a quad tree node of the source
		 * spatial partition respectively.
		 *
		 * The signatures are:
		 *
		 *   void
		 *   mirror_root_element_function(
		 *       CubeQuadTreePartition<DstElementType> &dst_spatial_partition,
		 *       const SrcElementType &src_root_element);
		 *
		 *   void
		 *   mirror_node_element_function(
		 *       CubeQuadTreePartition<DstElementType> &dst_spatial_partition,
		 *       typename CubeQuadTreePartition<DstElementType>::node_reference_type dst_node,
		 *       const SrcElementType &src_element);
		 *
		 */
		template <typename SrcElementType, typename DstElementType,
				typename MirrorRootElementFunctionType, typename MirrorNodeElementFunctionType>
		void
		mirror(
				CubeQuadTreePartition<DstElementType> &dst_spatial_partition,
				const CubeQuadTreePartition<SrcElementType> &src_spatial_partition,
				const MirrorRootElementFunctionType &mirror_root_element_function,
				const MirrorNodeElementFunctionType &mirror_node_element_function);


		/**
		 * A form of mirroring that merges the elements of @a src_spatial_partition into
		 * @a dst_spatial_partition by adding them to @a dst_spatial_partition.
		 */
		template <typename ElementType>
		void
		merge(
				CubeQuadTreePartition<ElementType> &dst_spatial_partition,
				const CubeQuadTreePartition<ElementType> &src_spatial_partition);


		/**
		 * Visits those pairs of elements in @a spatial_partition that potentially intersect each other.
		 *
		 * The function signature is:
		 *
		 *   void
		 *   visit_element_pair_function(
		 *       ElementType &element1,
		 *       ElementType &element1);
		 */
		template <typename ElementType, typename VisitElementPairFunctionType>
		void
		visit_potentially_intersecting_elements(
				CubeQuadTreePartition<ElementType> &spatial_partition,
				const VisitElementPairFunctionType &visit_element_pair_function);


		// Forward declaration.
		template <typename ElementType, class CubeQuadTreePartitionType>
		class CubeQuadTreeIntersectingNodes;

		/**
		 * A utility class to use during traversal of a spatial partition to determine those
		 * 'loose' nodes of another spatial partition that intersect it.
		 *
		 * This class provides an efficient, hierarchical way to accumulate intersections with
		 * a spatial partition during traversal of another spatial partition.
		 *
		 * An example usage of this class is determining those polygons in one group that overlap
		 * geometries in another group.
		 *
		 * The template parameter 'ElementType' is the element type of the spatial partition
		 * that we're looking for intersecting nodes.
		 */
		template <typename ElementType, class CubeQuadTreePartitionType = const CubeQuadTreePartition<ElementType> >
		class CubeQuadTreePartitionIntersectingNodes
		{
		public:
			//! Typedef for the spatial partition we are traversing.
			typedef CubeQuadTreePartitionType cube_quad_tree_partition_type;

			//! Typedef for the node reference type.
			typedef typename boost::mpl::if_<
					boost::is_const<CubeQuadTreePartitionType>,
								typename cube_quad_tree_partition_type::const_node_reference_type,
								typename cube_quad_tree_partition_type::node_reference_type>::type
										cube_quad_tree_partition_node_reference_type;


			/**
			 * Contains node references of intersecting nodes of the spatial partition.
			 */
			template <int max_num_nodes>
			class IntersectingNodesType
			{
			public:
				//! The maximum number of intersecting nodes.
				static const unsigned int MAX_NUM_NODES = max_num_nodes;

				//! Returns the number of intersecting nodes.
				unsigned int
				get_num_nodes() const
				{
					return d_num_nodes;
				}

				//! Returns the node reference of the specified intersecting node.
				cube_quad_tree_partition_node_reference_type
				get_node(
						unsigned int node_index) const
				{
					return d_node_references[node_index];
				}

				//! Returns the node location of the specified intersecting node.
				const CubeQuadTreeLocation &
				get_node_location(
						unsigned int node_index) const
				{
					return d_node_locations[node_index];
				}

			private:
				unsigned int d_num_nodes;
				cube_quad_tree_partition_node_reference_type d_node_references[MAX_NUM_NODES];
				CubeQuadTreeLocation d_node_locations[MAX_NUM_NODES];

				IntersectingNodesType() :
					d_num_nodes(0)
				{  }

				// Make friend so can initialise private data.
				friend class CubeQuadTreePartitionIntersectingNodes<ElementType, CubeQuadTreePartitionType>;
				friend class CubeQuadTreeIntersectingNodes<ElementType, CubeQuadTreePartitionType>;
			};

			/**
			 * Typedef for a sequence of intersecting nodes at the current traversal depth.
			 */
			typedef IntersectingNodesType<9> intersecting_nodes_type;


			/**
			 * Constructor for the root node of a spatial partition (ie, of a face of the cube).
			 *
			 * @a spatial_partition is the spatial partition that we track intersections with
			 * as the client traverses another another spatial partition - the client traverses by
			 * instantiating @a CubeQuadTreePartitionIntersectingNodes objects as they traverse.
			 */
			CubeQuadTreePartitionIntersectingNodes(
					cube_quad_tree_partition_type &spatial_partition,
					CubeCoordinateFrame::CubeFaceType cube_face);


			/**
			 * Constructor for a child node of the specified parent quad tree node.
			 * 
			 * This scenario is:
			 * - spatial partition versus spatial partition.
			 */
			CubeQuadTreePartitionIntersectingNodes(
					const CubeQuadTreePartitionIntersectingNodes &parent,
					unsigned int child_x_offset,
					unsigned int child_y_offset);


			/**
			 * Returns those nodes of the spatial partition, at the current traversal depth,
			 * that intersect the spatial partition at the current depth.
			 *
			 * At most nine nodes can intersect at this depth.
			 */
			const intersecting_nodes_type &
			get_intersecting_nodes() const
			{
				return d_intersecting_nodes;
			}


			/**
			 * Returns the location, in the spatial partition, of this node.
			 */
			const CubeQuadTreeLocation &
			get_node_location() const
			{
				return d_node_location;
			}

		protected:
			CubeQuadTreeLocation d_node_location;
			intersecting_nodes_type d_intersecting_nodes;


			//! Constructor for derived class.
			CubeQuadTreePartitionIntersectingNodes(
					const CubeQuadTreeLocation &node_location) :
				d_node_location(node_location)
			{  }

			/**
			 * Find those child nodes of the parent intersecting nodes that
			 * intersect 'this' child.
			 */
			template <int max_num_parent_nodes>
			void
			find_intersecting_nodes(
					const IntersectingNodesType<max_num_parent_nodes> &parent_intersecting_nodes);
		};


		/**
		 * A utility class to use during traversal of a regular cube quad tree (not a spatial
		 * partition, eg, a multi-resolution raster) to determine those 'loose' nodes of a spatial
		 * partition that intersect it.
		 *
		 * This class provides an efficient, hierarchical way to accumulate intersections with
		 * a spatial partition during traversal of a regular cube quad tree.
		 *
		 * An example usage of this class is determining those polygons that overlap each
		 * quad tree tile of a multi-resolution raster.
		 *
		 * The template parameter 'ElementType' is the element type of the spatial partition
		 * that we're looking for intersecting nodes.
		 */
		template <typename ElementType, class CubeQuadTreePartitionType = const CubeQuadTreePartition<ElementType> >
		class CubeQuadTreeIntersectingNodes :
				public CubeQuadTreePartitionIntersectingNodes<ElementType, CubeQuadTreePartitionType>
		{
		public:
			//! Typedef for the cube quad tree partition.
			typedef typename CubeQuadTreePartitionIntersectingNodes<ElementType, CubeQuadTreePartitionType>
					::cube_quad_tree_partition_type cube_quad_tree_partition_type;

			//! Typedef for the node reference type.
			typedef typename CubeQuadTreePartitionIntersectingNodes<ElementType, CubeQuadTreePartitionType>
					::cube_quad_tree_partition_node_reference_type cube_quad_tree_partition_node_reference_type;

			/**
			 * Typedef for a sequence of intersecting nodes at the parent traversal depth.
			 *
			 * For a regular cube quad tree (not a spatial partition) the maximum number of
			 * parent nodes that can possibly intersect a child node is four instead of nine.
			 */
			typedef typename CubeQuadTreePartitionIntersectingNodes<ElementType, CubeQuadTreePartitionType>
					::template IntersectingNodesType<4>
							parent_intersecting_nodes_type;


			/**
			 * Constructor for the root node of a cube quad tree (ie, of a face of the cube).
			 *
			 * @a spatial_partition is the spatial partition that we track intersections with
			 * as the client traverses a cube quad tree - the client traverses by instantiating
			 * @a CubeQuadTreeIntersectingNodes objects as they traverse.
			 */
			CubeQuadTreeIntersectingNodes(
					cube_quad_tree_partition_type &spatial_partition,
					CubeCoordinateFrame::CubeFaceType cube_face);


			/**
			 * Constructor for a child node of the specified parent quad tree node.
			 * 
			 * This scenario is:
			 * - spatial partition versus regular cube quad tree.
			 */
			CubeQuadTreeIntersectingNodes(
					const CubeQuadTreeIntersectingNodes &parent,
					unsigned int child_x_offset,
					unsigned int child_y_offset);


			/**
			 * Returns those parent nodes that intersect this child.
			 *
			 * This is only needed for a regular cube quad tree (and not a spatial partition)
			 * because the maximum number of possible intersecting parents is reduced from nine
			 * to four for a regular cube quad tree. Hence this method is not in the
			 * @a CubeQuadTreePartitionIntersectingNodes class.
			 */
			const parent_intersecting_nodes_type &
			get_parent_intersecting_nodes() const
			{
				return d_parent_intersecting_nodes;
			}

		private:
			using CubeQuadTreePartitionIntersectingNodes<ElementType, CubeQuadTreePartitionType>::d_node_location;
			using CubeQuadTreePartitionIntersectingNodes<ElementType, CubeQuadTreePartitionType>::d_intersecting_nodes;

			parent_intersecting_nodes_type d_parent_intersecting_nodes;
		};


		////////////////////////
		////////////////////////
		//// Implementation ////
		////////////////////////
		////////////////////////


		namespace Implementation
		{
			template <typename ElementType>
			void
			merge_root_element(
					CubeQuadTreePartition<ElementType> &dst_spatial_partition,
					const ElementType &src_root_element)
			{
				// Add the source root element to the root of the destination spatial partition.
				dst_spatial_partition.add_unpartitioned(src_root_element);
			}

			template <typename ElementType>
			void
			merge_node_element(
					CubeQuadTreePartition<ElementType> &dst_spatial_partition,
					typename CubeQuadTreePartition<ElementType>::node_reference_type dst_node,
					const ElementType &src_element)
			{
				// Add source element to the destination quad tree node.
				dst_spatial_partition.add(src_element, dst_node);
			}
		}


		template <typename ElementType>
		void
		merge(
				CubeQuadTreePartition<ElementType> &dst_spatial_partition,
				const CubeQuadTreePartition<ElementType> &src_spatial_partition)
		{
			mirror(dst_spatial_partition, src_spatial_partition,
					&Implementation::merge_root_element<ElementType>,
					&Implementation::merge_node_element<ElementType>);
		}


		namespace Implementation
		{
			template <typename SrcElementType, typename DstElementType, typename MirrorNodeElementFunctionType>
			void
			mirror_quad_tree(
					CubeQuadTreePartition<DstElementType> &dst_spatial_partition,
					const CubeQuadTreePartition<SrcElementType> &src_spatial_partition,
					typename CubeQuadTreePartition<DstElementType>::node_reference_type dst_node,
					typename CubeQuadTreePartition<SrcElementType>::const_node_reference_type src_node,
					const MirrorNodeElementFunctionType &mirror_node_element_function)
			{
				//typedef CubeQuadTreePartition<DstElementType> dst_spatial_partition_type;
				typedef CubeQuadTreePartition<SrcElementType> src_spatial_partition_type;
				typedef typename CubeQuadTreePartition<DstElementType>::node_reference_type dst_node_reference_type;
				typedef typename CubeQuadTreePartition<SrcElementType>::const_node_reference_type src_node_reference_type;

				// Iterate over the elements in the current node.
				typename src_spatial_partition_type::element_const_iterator src_elements_iter = src_node.begin();
				typename src_spatial_partition_type::element_const_iterator src_elements_end = src_node.end();
				for ( ; src_elements_iter != src_elements_end; ++src_elements_iter)
				{
					const SrcElementType &src_element = *src_elements_iter;

					// Mirror source element to the destination quad tree node.
					mirror_node_element_function(dst_spatial_partition, dst_node, src_element);
				}

				// Iterate over the child nodes of the source spatial partition.
				for (unsigned int child_y_offset = 0; child_y_offset < 2; ++child_y_offset)
				{
					for (unsigned int child_x_offset = 0; child_x_offset < 2; ++child_x_offset)
					{
						// See if there is the current child node in the source spatial partition.
						const src_node_reference_type src_child_node =
								src_spatial_partition.get_child_node(
										src_node, child_x_offset, child_y_offset);
						if (!src_child_node)
						{
							continue;
						}

						// Create a new child node in the destination spatial partition.
						const dst_node_reference_type dst_child_node =
								dst_spatial_partition.get_or_create_child_node(
										dst_node, child_x_offset, child_y_offset);

						mirror_quad_tree(
								dst_spatial_partition,
								src_spatial_partition,
								dst_child_node,
								src_child_node,
								mirror_node_element_function);
					}
				}
			}
		}


		template <typename SrcElementType, typename DstElementType,
				typename MirrorRootElementFunctionType, typename MirrorNodeElementFunctionType>
		void
		mirror(
				CubeQuadTreePartition<DstElementType> &dst_spatial_partition,
				const CubeQuadTreePartition<SrcElementType> &src_spatial_partition,
				const MirrorRootElementFunctionType &mirror_root_element_function,
				const MirrorNodeElementFunctionType &mirror_node_element_function)
		{
			typedef CubeQuadTreePartition<DstElementType> dst_spatial_partition_type;
			typedef CubeQuadTreePartition<SrcElementType> src_spatial_partition_type;

			// Mirror the root elements from the source spatial partition to the destination one.
			typename src_spatial_partition_type::element_const_iterator src_root_elements_iter =
					src_spatial_partition.begin_root_elements();
			const typename src_spatial_partition_type::element_const_iterator src_root_elements_end =
					src_spatial_partition.end_root_elements();
			for ( ; src_root_elements_iter != src_root_elements_end; ++src_root_elements_iter)
			{
				const SrcElementType &src_root_element = *src_root_elements_iter;

				// Mirror the source root element to the root of the destination spatial partition.
				mirror_root_element_function(dst_spatial_partition, src_root_element);
			}

			// Iterate over the faces of the cube and then traverse the quad tree of each face.
			for (unsigned int face = 0; face < 6; ++face)
			{
				GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face =
						static_cast<GPlatesMaths::CubeCoordinateFrame::CubeFaceType>(face);

				// See if there is the current quad tree root node in the source spatial partition.
				const typename src_spatial_partition_type::const_node_reference_type src_quad_tree_root_node =
						src_spatial_partition.get_quad_tree_root_node(cube_face);
				if (!src_quad_tree_root_node)
				{
					continue;
				}

				// Get the current quad tree root node in the destination spatial partition.
				const typename dst_spatial_partition_type::node_reference_type dst_quad_tree_root_node =
						dst_spatial_partition.get_or_create_quad_tree_root_node(cube_face);

				Implementation::mirror_quad_tree(
						dst_spatial_partition,
						src_spatial_partition,
						dst_quad_tree_root_node,
						src_quad_tree_root_node,
						mirror_node_element_function);
			}
		}


		namespace Implementation
		{
			template <typename ElementType>
			using element_iterator = typename CubeQuadTreePartition<ElementType>::element_iterator;

			template <typename ElementType>
			using element_range_type = std::pair<
					element_iterator<ElementType>,
					element_iterator<ElementType>>;

			template <typename ElementType>
			using node_reference_type = typename CubeQuadTreePartition<ElementType>::node_reference_type;

			template <typename ElementType>
			using neighbour_nodes_type = CubeQuadTreePartitionIntersectingNodes<
					ElementType,
					/*non-const*/ CubeQuadTreePartition<ElementType>>;

			//! A linked list node that references a list of elements (either the root elements or elements in a node).
			template <typename ElementType>
			struct ElementRangeListNode :
					public GPlatesUtils::IntrusiveSinglyLinkedList<ElementRangeListNode<ElementType>>::Node
			{
				explicit
				ElementRangeListNode(
						const element_range_type<ElementType> &element_range_) :
					element_range(element_range_)
				{  }

				ElementRangeListNode(
						const element_iterator<ElementType> &element_range_begin_,
						const element_iterator<ElementType> &element_range_end_) :
					element_range(element_range_begin_, element_range_end_)
				{  }

				element_range_type<ElementType> element_range;
			};

			/**
			 * We use our own intrusive singly linked list (instead of boost::slist) since it supports
			 * tail-sharing (where multiple lists can share their tail ends).
			 */
			template <typename ElementType>
			using element_range_list_type = GPlatesUtils::IntrusiveSinglyLinkedList<ElementRangeListNode<ElementType>>;

			template <typename ElementType>
			using element_range_list_const_iterator_type = typename element_range_list_type<ElementType>::const_iterator;

			template <typename ElementType, typename VisitElementPairFunctionType>
			void
			visit_potentially_intersecting_element_range(
					const element_range_type<ElementType> &element_range,
					const element_range_list_type<ElementType> &neighbour_element_range_list,
					const element_range_list_const_iterator_type<ElementType> &sibling_ancestor_neighbour_boundary_element_range_iterator,
					const VisitElementPairFunctionType &visit_element_pair_function)
			{
				// Iterate over the elements in the current element range
				// (which is either the root elements or the elements of a quad tree node).
				for (auto element_iter = element_range.first; element_iter != element_range.second; ++element_iter)
				{
					auto &element = *element_iter;

					// Iterate over prior elements (to the current element) in the current element range.
					// By only visiting the *prior* elements we avoid visiting each pair of elements twice.
					for (auto prev_element_iter = element_range.first; prev_element_iter != element_iter; ++prev_element_iter)
					{
						auto &prev_element = *prev_element_iter;

						// Visit the pair of previous and current elements (in the current element range).
						visit_element_pair_function(prev_element, element);
					}

					// Next visit the sibling neighbour element ranges (at the same quad tree depth as elements in the current range).
					//
					// The sibling part of the list is the first part.
					for (auto sibling_neighbour_element_range_iter = neighbour_element_range_list.begin();
						sibling_neighbour_element_range_iter != sibling_ancestor_neighbour_boundary_element_range_iterator;
						++sibling_neighbour_element_range_iter)
					{
						const auto &sibling_neighbour_element_range = sibling_neighbour_element_range_iter->element_range;

						// Iterate over the elements in the sibling neighbour element range.
						for (auto sibling_neighbour_element_iter = sibling_neighbour_element_range.first;
							sibling_neighbour_element_iter != sibling_neighbour_element_range.second;
							++sibling_neighbour_element_iter)
						{
							auto &sibling_neighbour_element = *sibling_neighbour_element_iter;

							// With *sibling* neighbour elements we end up visiting each element pair twice
							// (ie, 'visit_element_pair_function(sibling_neighbour_element, element)' and
							// 'visit_element_pair_function(element, sibling_neighbour_element)').
							// This is because we're visiting the pair now where we visit 'element' and
							// its neighbour 'sibling_neighbour_element' but we also will visit
							// 'sibling_neighbour_element' and its neighbour 'element.
							// So in order to only visit the pair once we need an arbitrary condition
							// that excludes one ordering of the pair. In our case it's easiest just
							// to compare the memory addresses of the two elements.
							if (&sibling_neighbour_element < &element)
							{
								// Visit the pair of sibling neighbour and current elements.
								visit_element_pair_function(sibling_neighbour_element, element);
							}
						}
					}

					// Next visit all ancestor neighbour element ranges (higher in the quad tree, closer to root, than the
					// elements in the current range). These neighbour elements are the root elements
					// (which potentially intersect all elements) and elements of parent quad tree nodes up to the root
					// (which also potentially intersect elements in the current range).
					//
					// The ancestor part of the list is the last part.
					for (auto ancestor_neighbour_element_range_iter = sibling_ancestor_neighbour_boundary_element_range_iterator;
						ancestor_neighbour_element_range_iter != neighbour_element_range_list.end();
						++ancestor_neighbour_element_range_iter)
					{
						const auto &ancestor_neighbour_element_range = ancestor_neighbour_element_range_iter->element_range;

						// Iterate over the elements in the ancestor neighbour element range.
						for (auto ancestor_neighbour_element_iter = ancestor_neighbour_element_range.first;
							ancestor_neighbour_element_iter != ancestor_neighbour_element_range.second;
							++ancestor_neighbour_element_iter)
						{
							auto &ancestor_neighbour_element = *ancestor_neighbour_element_iter;

							// Visit the pair of ancestor neighbour and current elements.
							//
							// Note that, unlike a sibling neighbour, we always visit an ancestor neighbour.
							// This is because we never end up visiting the swapped version of these elements
							// (ie, 'visit_element_pair_function(element, ancestor_neighbour_element)' because
							// when we visited the ancestor element it did not visit any descendant elements.
							visit_element_pair_function(ancestor_neighbour_element, element);
						}
					}
				}
			}

			template <typename ElementType, typename VisitElementPairFunctionType>
			void
			visit_potentially_intersecting_elements_quad_tree(
					CubeQuadTreePartition<ElementType> &spatial_partition,
					const element_range_list_type<ElementType> &ancestor_neighbour_element_range_list,
					const node_reference_type<ElementType> &node_reference,
					const neighbour_nodes_type<ElementType> &sibling_neighbour_nodes,
					const VisitElementPairFunctionType &visit_element_pair_function)
			{
				// Tail share with the ancestor neighbour element-range list since we want to traverse the nodes
				// in that list as well as new sibling neighbour nodes that we'll add now.
				element_range_list_type<ElementType> neighbour_element_range_list(ancestor_neighbour_element_range_list);

				// The current beginning of the neighbour element range list is the boundary between ancestor and
				// sibling neighbours. We'll soon be adding the sibling neighbours.
				const element_range_list_const_iterator_type<ElementType>
						sibling_ancestor_neighbour_boundary_element_range_iterator = neighbour_element_range_list.begin();

				// Construct linked list nodes on the runtime stack as it simplifies memory management.
				// When the stack unwinds, the list(s) referencing these nodes, as well as the nodes themselves,
				// will disappear together (leaving any lists higher up in the stack still intact) - this happens
				// because this list implementation supports tail-sharing.
				// Note that we use boost::optional since 'ElementRangeListNode' does not have a default constructor.
				boost::optional<ElementRangeListNode<ElementType>> sibling_neighbour_element_range_list_nodes[
						neighbour_nodes_type<ElementType>::intersecting_nodes_type::MAX_NUM_NODES];

				// Now add those neighbour nodes that exist (not all areas of the spatial partition will be populated).
				const unsigned int num_sibling_neighbour_nodes = sibling_neighbour_nodes.get_intersecting_nodes().get_num_nodes();
				for (unsigned int sibling_neighbour_node_index = 0;
					sibling_neighbour_node_index < num_sibling_neighbour_nodes;
					++sibling_neighbour_node_index)
				{
					const auto &sibling_neighbour_node_reference =
							sibling_neighbour_nodes.get_intersecting_nodes().get_node(sibling_neighbour_node_index);
					// Skip the sibling neighbour node if it's also the current node.
					if (sibling_neighbour_node_reference == node_reference)
					{
						continue;
					}

					// Only need to add neighbour nodes that actually contain elements.
					// NOTE: We still recurse into child nodes though - an empty internal node does not mean the child nodes are necessarily empty.
					if (sibling_neighbour_node_reference.empty())
					{
						continue;
					}

					sibling_neighbour_element_range_list_nodes[sibling_neighbour_node_index] =
							boost::in_place(sibling_neighbour_node_reference.begin(), sibling_neighbour_node_reference.end());

					// Add to the list of element ranges that neighbour the current node.
					neighbour_element_range_list.push_front(
							&sibling_neighbour_element_range_list_nodes[sibling_neighbour_node_index].get());
				}

				// The elements in the current node.
				const element_range_type<ElementType> element_range(node_reference.begin(), node_reference.end());

				// Visit the elements in the current node.
				//
				// Their neighbours are the same elements in the current node as well as elements
				// gathered in the neighbour list so far.
				visit_potentially_intersecting_element_range(
						element_range,
						neighbour_element_range_list,
						sibling_ancestor_neighbour_boundary_element_range_iterator,
						visit_element_pair_function);

				// Add the range of elements in the current node to the list of neighbour element ranges.
				// This is so any child nodes will consider the current node as a neighbour
				// (albeit an overlapping neighbour).
				ElementRangeListNode<ElementType> element_range_list_node(element_range);
				neighbour_element_range_list.push_front(&element_range_list_node);

				// Iterate over the child nodes.
				for (unsigned int child_y_offset = 0; child_y_offset < 2; ++child_y_offset)
				{
					for (unsigned int child_x_offset = 0; child_x_offset < 2; ++child_x_offset)
					{
						// See if there is the current child node in the source spatial partition.
						const auto child_node = spatial_partition.get_child_node(node_reference, child_x_offset, child_y_offset);
						if (child_node)
						{
							const neighbour_nodes_type<ElementType> child_neighbour_nodes(
									sibling_neighbour_nodes, child_x_offset, child_y_offset);

							visit_potentially_intersecting_elements_quad_tree(
									spatial_partition,
									neighbour_element_range_list,
									child_node,
									child_neighbour_nodes,
									visit_element_pair_function);
						}
					}
				}
			}
		}


		template <typename ElementType, typename VisitElementPairFunctionType>
		void
		visit_potentially_intersecting_elements(
				CubeQuadTreePartition<ElementType> &spatial_partition,
				const VisitElementPairFunctionType &visit_element_pair_function)
		{
			Implementation::element_range_list_type<ElementType> root_neighbour_element_range_list;

			// Elements in the root of the spatial partition (not in any cube-face quadtrees).
			const Implementation::element_range_type<ElementType> root_element_range(
					spatial_partition.begin_root_elements(),
					spatial_partition.end_root_elements());

			// Visit the root elements.
			//
			// Their neighbours are the same root elements, but there are no sibling/ancestor elements
			// gathered yet into the list (so it is empty).
			Implementation::visit_potentially_intersecting_element_range(
					root_element_range,
					root_neighbour_element_range_list,  // empty
					// There are no sibling or ancestor neighbour element ranges to the root elements...
					root_neighbour_element_range_list.end(),
					visit_element_pair_function);

			// Add the range of root elements to the list of neighbour element ranges.
			// This is so the cube-face quadtrees will consider the root elements as neighbours
			// (albeit overlapping neighbours).
			Implementation::ElementRangeListNode<ElementType> root_neighbour_element_range_list_node(root_element_range);
			root_neighbour_element_range_list.push_front(&root_neighbour_element_range_list_node);

			// Iterate over the faces of the cube and then traverse the quad tree of each face.
			for (unsigned int face = 0; face < 6; ++face)
			{
				const GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face =
						static_cast<GPlatesMaths::CubeCoordinateFrame::CubeFaceType>(face);

				const Implementation::neighbour_nodes_type<ElementType> sibling_neighbour_nodes(spatial_partition, cube_face);

				// See if there is the current quad tree root node in the spatial partition.
				const auto node_reference = spatial_partition.get_quad_tree_root_node(cube_face);
				if (node_reference)
				{
					Implementation::visit_potentially_intersecting_elements_quad_tree(
							spatial_partition,
							root_neighbour_element_range_list,
							node_reference,
							sibling_neighbour_nodes,
							visit_element_pair_function);
				}
			}
		}


		template <typename ElementType, class CubeQuadTreePartitionType>
		CubeQuadTreePartitionIntersectingNodes<ElementType, CubeQuadTreePartitionType>::CubeQuadTreePartitionIntersectingNodes(
				CubeQuadTreePartitionType &spatial_partition,
				CubeCoordinateFrame::CubeFaceType cube_face) :
			d_node_location(cube_face)
		{
			const CubeCoordinateFrame::CubeFaceType opposite_cube_face = get_cube_face_opposite(cube_face);

			// Iterate over the cube faces.
			for (unsigned int neighbour_cube_face_index = 0; neighbour_cube_face_index < 6; ++neighbour_cube_face_index)
			{
				const CubeCoordinateFrame::CubeFaceType neighbour_cube_face =
						static_cast<CubeCoordinateFrame::CubeFaceType>(neighbour_cube_face_index);

				// The face opposite 'cube_face' cannot intersect it so skip it.
				if (neighbour_cube_face == opposite_cube_face)
				{
					continue;
				}

				cube_quad_tree_partition_node_reference_type neighbour_root_node =
						spatial_partition.get_quad_tree_root_node(neighbour_cube_face);
				if (neighbour_root_node)
				{
					d_intersecting_nodes.d_node_references[d_intersecting_nodes.d_num_nodes] = neighbour_root_node;
					d_intersecting_nodes.d_node_locations[d_intersecting_nodes.d_num_nodes] = CubeQuadTreeLocation(neighbour_cube_face);
					++d_intersecting_nodes.d_num_nodes;
				}
			}
		}


		template <typename ElementType, class CubeQuadTreePartitionType>
		CubeQuadTreePartitionIntersectingNodes<ElementType, CubeQuadTreePartitionType>::CubeQuadTreePartitionIntersectingNodes(
				const CubeQuadTreePartitionIntersectingNodes &parent,
				unsigned int child_x_offset,
				unsigned int child_y_offset) :
			d_node_location(
					parent.d_node_location, child_x_offset, child_y_offset)
		{
			find_intersecting_nodes(parent.d_intersecting_nodes);
		}


		template <typename ElementType, class CubeQuadTreePartitionType>
		template <int max_num_parent_nodes>
		void
		CubeQuadTreePartitionIntersectingNodes<ElementType, CubeQuadTreePartitionType>::find_intersecting_nodes(
				const IntersectingNodesType<max_num_parent_nodes> &parent_intersecting_nodes)
		{
			const unsigned int num_parent_nodes = parent_intersecting_nodes.get_num_nodes();
			for (unsigned int parent_node_index = 0; parent_node_index < num_parent_nodes; ++parent_node_index)
			{
				cube_quad_tree_partition_node_reference_type
						parent_intersecting_node_reference =
								parent_intersecting_nodes.get_node(parent_node_index);

				const CubeQuadTreeLocation &parent_intersecting_node_location =
						parent_intersecting_nodes.get_node_location(parent_node_index);

				// Iterate over the four child nodes of the current parent node.
				for (unsigned int child_y_offset = 0; child_y_offset < 2; ++child_y_offset)
				{
					for (unsigned int child_x_offset = 0; child_x_offset < 2; ++child_x_offset)
					{
						cube_quad_tree_partition_node_reference_type
								intersecting_node_reference =
										parent_intersecting_node_reference.get_child_node(
												child_x_offset, child_y_offset);
						if (!intersecting_node_reference)
						{
							continue;
						}

						const CubeQuadTreeLocation intersecting_node_location(
								parent_intersecting_node_location,
								child_x_offset,
								child_y_offset);

						// If the child node intersects us then it's an intersecting node.
						if (do_same_depth_nodes_intersect(
								intersecting_node_location, d_node_location))
						{
							// It should only be possible to have at most nine neighbours
							// out of the 36 (9 * 4) that we are testing against.
							GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
									d_intersecting_nodes.d_num_nodes < intersecting_nodes_type::MAX_NUM_NODES,
									GPLATES_ASSERTION_SOURCE);

							d_intersecting_nodes.d_node_references[d_intersecting_nodes.d_num_nodes] = intersecting_node_reference;
							d_intersecting_nodes.d_node_locations[d_intersecting_nodes.d_num_nodes] = intersecting_node_location;
							++d_intersecting_nodes.d_num_nodes;
						}
					}
				}
			}
		}


		template <typename ElementType, class CubeQuadTreePartitionType>
		CubeQuadTreeIntersectingNodes<ElementType, CubeQuadTreePartitionType>::CubeQuadTreeIntersectingNodes(
				cube_quad_tree_partition_type &spatial_partition,
				CubeCoordinateFrame::CubeFaceType cube_face) :
			CubeQuadTreePartitionIntersectingNodes<ElementType, CubeQuadTreePartitionType>(
					spatial_partition,
					cube_face)
		{
		}


		template <typename ElementType, class CubeQuadTreePartitionType>
		CubeQuadTreeIntersectingNodes<ElementType, CubeQuadTreePartitionType>::CubeQuadTreeIntersectingNodes(
				const CubeQuadTreeIntersectingNodes &parent,
				unsigned int child_x_offset,
				unsigned int child_y_offset) :
			CubeQuadTreePartitionIntersectingNodes<ElementType, CubeQuadTreePartitionType>(
					CubeQuadTreeLocation(parent.d_node_location, child_x_offset, child_y_offset))
		{
			// First reduce the set of parent nodes from nine to four.
			// Since we're traversing a regular (not loose) cube quad tree (instead of a loose
			// spatial partition) it turns out that only four of the (up to) nine parent nodes
			// can actually intersect 'this' child.
			const unsigned int num_parent_nodes = parent.d_intersecting_nodes.get_num_nodes();
			for (unsigned int parent_node_index = 0; parent_node_index < num_parent_nodes; ++parent_node_index)
			{
				cube_quad_tree_partition_node_reference_type
						parent_intersecting_node_reference =
								parent.d_intersecting_nodes.get_node(parent_node_index);

				const CubeQuadTreeLocation &parent_intersecting_node_location =
						parent.d_intersecting_nodes.get_node_location(parent_node_index);

				// If the parent node intersects us then it's an intersecting node.
				if (intersect_loose_quad_tree_node_with_regular_quad_tree_node_at_parent_child_depths(
						parent_intersecting_node_location,
						d_node_location))
				{
					// It should only be possible to have at most four parent neighbours
					// out of the nine that we are testing against.
					GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
							d_parent_intersecting_nodes.d_num_nodes < parent_intersecting_nodes_type::MAX_NUM_NODES,
							GPLATES_ASSERTION_SOURCE);

					d_parent_intersecting_nodes.d_node_references[d_parent_intersecting_nodes.d_num_nodes] =
							parent_intersecting_node_reference;
					d_parent_intersecting_nodes.d_node_locations[d_parent_intersecting_nodes.d_num_nodes] =
							parent_intersecting_node_location;
					++d_parent_intersecting_nodes.d_num_nodes;
				}
			}

			// Now find the child intersecting nodes using the reduced set of parent intersecting nodes.
			this->find_intersecting_nodes(d_parent_intersecting_nodes);
		}
	}
}

#endif // GPLATES_MATHS_CUBEQUADTREEPARTITIONUTILS_H
