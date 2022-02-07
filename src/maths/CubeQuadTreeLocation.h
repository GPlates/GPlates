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

#ifndef GPLATES_MATHS_CUBEQUADTREELOCATION_H
#define GPLATES_MATHS_CUBEQUADTREELOCATION_H

#include <boost/optional.hpp>

#include "CubeCoordinateFrame.h"


namespace GPlatesMaths
{
	/**
	 * Specifies the location in a cube quad tree.
	 *
	 * The location can be a specific node of one of the six quad trees (one per cube face) or
	 * it can be the root of the cube (ie, not in any quad tree).
	 *
	 * The root of the cube is used by the @a CubeQuadTreePartition class, for example, to
	 * place objects that don't fit within any of the loose bounds of the cube faces.
	 * For other uses, such as a mult-resolution cube raster, the root of the cube does not
	 * have any meaning and is not used.
	 *
	 * This location is not a reference to a node so it can't be used to dereference an existing
	 * node like @a NodeReference (in @a CubeQuadTreePartition) but it can be used with one of
	 * the @a add overloads (in @a CubeQuadTreePartition, for example) to insert a node without
	 * using a spatial geometry or a parent node.
	 */
	class CubeQuadTreeLocation
	{
	public:
		/**
		 * The location of a node in a quad tree (if applicable, ie, if not the root of the cube).
		 *
		 * @a x_node_offset_ and @a y_node_offset_ are in the range [0, 2^quad_tree_depth).
		 */
		struct NodeLocation
		{
			NodeLocation(
					CubeCoordinateFrame::CubeFaceType cube_face_,
					unsigned int quad_tree_depth_,
					unsigned int x_node_offset_,
					unsigned int y_node_offset_) :
				cube_face(cube_face_),
				quad_tree_depth(quad_tree_depth_),
				x_node_offset(x_node_offset_),
				y_node_offset(y_node_offset_)
			{  }

			CubeCoordinateFrame::CubeFaceType cube_face;
			unsigned int quad_tree_depth;
			unsigned int x_node_offset;
			unsigned int y_node_offset;
		};


		//! Default constructor places location at the root of the cube (not in any quad tree).
		CubeQuadTreeLocation()
		{  }


		/**
		 * This constructor places the location at the root node of the specified *quad tree*.
		 *
		 * Note that this is the root *node* of a quad tree and *not* the root of the cube.
		 *
		 * There are six quad trees (one per cube face).
		 */
		explicit
		CubeQuadTreeLocation(
				CubeCoordinateFrame::CubeFaceType cube_face) :
			d_node_location(NodeLocation(cube_face, 0, 0, 0))
		{  }


		/**
		 * This constructor creates a child node of the specified parent *quad tree node* location.
		 *
		 * @throws PreconditionViolationError if @a parent_location is the root of the cube.
		 */
		CubeQuadTreeLocation(
				const CubeQuadTreeLocation &parent_location,
				unsigned int child_x_offset,
				unsigned int child_y_offset);


		/**
		 * This constructor places the location at a specific node in one of the six quad trees.
		 *
		 * @a x_node_offset and @a y_node_offset are in the range [0, 2^quad_tree_depth).
		 */
		CubeQuadTreeLocation(
				CubeCoordinateFrame::CubeFaceType cube_face,
				unsigned int quad_tree_depth,
				unsigned int x_node_offset,
				unsigned int y_node_offset) :
			d_node_location(NodeLocation(cube_face, quad_tree_depth, x_node_offset, y_node_offset))
		{  }


		/**
		 * This convenience constructor places the location at a specific node in one of the six quad trees.
		 */
		explicit
		CubeQuadTreeLocation(
				const NodeLocation &node_location) :
			d_node_location(node_location)
		{  }


		/**
		 * Returns true if 'this' location refers to the root of the cube (not in any quad tree).
		 */
		bool
		is_root_of_cube() const
		{
			return !d_node_location;
		}


		/**
		 * Returns the current location in a cube quad tree or returns boost::none
		 * if the current location refers to the root of the cube (ie, not in any quad tree).
		 */
		const boost::optional<NodeLocation> &
		get_node_location() const
		{
			return d_node_location;
		}


		/**
		 * Creates a child node of the specified parent *quad tree node* location.
		 *
		 * This is effectively the same as the constructor that creates a child node except
		 * this involves an extra copy (of the return value) and so is slightly more expensive.
		 *
		 * @throws PreconditionViolationError if @a parent_location is the root of the cube.
		 */
		CubeQuadTreeLocation
		get_child_node_location(
				unsigned int child_x_offset,
				unsigned int child_y_offset) const
		{
			return CubeQuadTreeLocation(*this, child_x_offset, child_y_offset);
		}

	private:
		/**
		 * Is true if the location is a node in any of the six quad trees, otherwise
		 * it's false (location is at the root of the cube - not in any quad tree).
		 */
		boost::optional<NodeLocation> d_node_location;
	};


	/**
	 * Returns true if both locations are quad tree nodes, at the same quad tree depth, that intersect.
	 *
	 * The nodes can also be on different cube faces and still intersect.
	 *
	 * NOTE: Either location can refer to a 'loose' node (ie, of a spatial partition) or both can.
	 * Due to the nature of overlap of nodes at the same level they all give the same result.
	 *
	 * @throws PreconditionViolationError if the specified locations are not quad tree nodes
	 * or are not at the same depth.
	 */
	bool
	do_same_depth_nodes_intersect(
			const CubeQuadTreeLocation &location_1,
			const CubeQuadTreeLocation &location_2);


	/**
	 * Returns true if both locations are quad tree nodes that intersect and
	 * @a loose_quad_tree_location_at_parent_depth is at one depth closer
	 * to the root than @a regular_quad_tree_location_at_child_depth.
	 *
	 * NOTE: The node at parent depth is a 'loose' node (ie, spatial partition node) while
	 * the node at child depth is a regular (non-loose) node.
	 *
	 * The nodes can also be on different cube faces and still intersect.
	 *
	 * @throws PreconditionViolationError if the specified locations are not quad tree nodes
	 * and are not at the same depth.
	 */
	bool
	intersect_loose_quad_tree_node_with_regular_quad_tree_node_at_parent_child_depths(
			const CubeQuadTreeLocation &loose_quad_tree_location_at_parent_depth,
			const CubeQuadTreeLocation &regular_quad_tree_location_at_child_depth);


	/**
	 * Returns true if the specified loose cube quad tree location intersects the specified regular one.
	 *
	 * The nodes can be at any quad tree depths.
	 *
	 * The nodes can also be on different cube faces.
	 */
	bool
	intersect_loose_cube_quad_tree_location_with_regular_cube_quad_tree_location(
			const CubeQuadTreeLocation &loose_quad_tree_location,
			const CubeQuadTreeLocation &regular_quad_tree_location);


#if 0
	/**
	 * Returns true if the specified loose cube quad tree locations intersect each other.
	 *
	 * The nodes can be at any quad tree depths.
	 *
	 * The nodes can also be on different cube faces.
	 */
	bool
	intersect_loose_cube_quad_tree_location_with_loose_cube_quad_tree_location(
			const CubeQuadTreeLocation &loose_quad_tree_location_1,
			const CubeQuadTreeLocation &loose_quad_tree_location_2);
#endif
}

#endif // GPLATES_MATHS_CUBEQUADTREELOCATION_H
