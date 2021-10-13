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

#include <boost/mpl/if.hpp>
#include <boost/type_traits/is_const.hpp>

#include "CubeCoordinateFrame.h"
#include "CubeQuadTreeLocation.h"
#include "CubeQuadTreePartition.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


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
		 *      CubeQuadTreePartition<DstElementType> &dst_spatial_partition,
		 *      const SrcElementType &src_root_element);
		 *
		 *   void
		 *   mirror_node_element_function(
		 *      CubeQuadTreePartition<DstElementType> &dst_spatial_partition,
		 *      typename CubeQuadTreePartition<DstElementType>::node_reference_type dst_node,
		 *      const SrcElementType &src_element);
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
				typedef CubeQuadTreePartition<DstElementType> dst_spatial_partition_type;
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
			find_intersecting_nodes(d_parent_intersecting_nodes);
		}
	}
}

#endif // GPLATES_MATHS_CUBEQUADTREEPARTITIONUTILS_H
