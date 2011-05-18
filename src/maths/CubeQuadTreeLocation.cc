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

#include "CubeQuadTreeLocation.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


GPlatesMaths::CubeQuadTreeLocation::CubeQuadTreeLocation(
		const CubeQuadTreeLocation &parent_location,
		unsigned int child_x_offset,
		unsigned int child_y_offset)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			parent_location.d_node_location,
			GPLATES_ASSERTION_SOURCE);
	const NodeLocation &parent_node_location = parent_location.d_node_location.get();

	d_node_location = NodeLocation(
			parent_node_location.cube_face,
			parent_node_location.quad_tree_depth + 1,
			(parent_node_location.x_node_offset << 1) + child_x_offset,
			(parent_node_location.y_node_offset << 1) + child_y_offset);
}


bool
GPlatesMaths::do_same_depth_nodes_intersect(
		const CubeQuadTreeLocation &location_1,
		const CubeQuadTreeLocation &location_2)
{
	const boost::optional<CubeQuadTreeLocation::NodeLocation> &location_1_opt = location_1.get_node_location();
	const boost::optional<CubeQuadTreeLocation::NodeLocation> &location_2_opt = location_2.get_node_location();

	// Ensure some preconditions.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			location_1_opt && location_2_opt &&
					location_1_opt->quad_tree_depth == location_2_opt->quad_tree_depth,
			GPLATES_ASSERTION_SOURCE);

	const CubeQuadTreeLocation::NodeLocation &node_location_1 = location_1_opt.get();
	const CubeQuadTreeLocation::NodeLocation &node_location_2 = location_2_opt.get();

	const int x_node_offset_1 = node_location_1.x_node_offset;
	const int y_node_offset_1 = node_location_1.y_node_offset;
	int x_node_offset_2, y_node_offset_2;
	if (node_location_1.cube_face == node_location_2.cube_face)
	{
		x_node_offset_2 = node_location_2.x_node_offset;
		y_node_offset_2 = node_location_2.y_node_offset;
	}
	else
	{
		// The nodes are on different cube faces so we need to transform the offsets of one of the
		// nodes so we can compare with the offsets of the other node.
		CubeCoordinateFrame::get_cube_quad_tree_node_location_relative_to_cube_face(
				x_node_offset_2,
				y_node_offset_2,
				node_location_1.cube_face/*transform_to_cube_face*/,
				node_location_2.cube_face/*transform_from_cube_face*/,
				node_location_2.quad_tree_depth,
				node_location_2.x_node_offset,
				node_location_2.y_node_offset);
	}

	// The two nodes only intersect if they refer to the same location or are neighbours -
	// which means the absolute difference in their positions is zero or one.

	const int x_diff = x_node_offset_1 - x_node_offset_2;
	const int x_diff_2 = x_diff * x_diff;
	if (x_diff_2 > 1)
	{
		return false;
	}

	const int y_diff = y_node_offset_1 - y_node_offset_2;
	const int y_diff_2 = y_diff * y_diff;
	if (y_diff_2 > 1)
	{
		return false;
	}

	return true;
}


bool
GPlatesMaths::intersect_loose_quad_tree_node_with_regular_quad_tree_node_at_parent_child_depths(
		const CubeQuadTreeLocation &loose_quad_tree_location_at_parent_depth,
		const CubeQuadTreeLocation &regular_quad_tree_location_at_child_depth)
{
	// If either location represents the entire cube then return false because it means
	// the locations must intersect.
	const boost::optional<CubeQuadTreeLocation::NodeLocation> &loose_node_location_at_parent_depth_opt =
			loose_quad_tree_location_at_parent_depth.get_node_location();
	const boost::optional<CubeQuadTreeLocation::NodeLocation> &regular_node_location_at_child_depth_opt =
			regular_quad_tree_location_at_child_depth.get_node_location();

	// Ensure some preconditions.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			loose_node_location_at_parent_depth_opt && regular_node_location_at_child_depth_opt &&
					loose_node_location_at_parent_depth_opt->quad_tree_depth + 1 ==
						regular_node_location_at_child_depth_opt->quad_tree_depth,
			GPLATES_ASSERTION_SOURCE);

	const CubeQuadTreeLocation::NodeLocation &loose_node_location_at_parent_depth =
			loose_node_location_at_parent_depth_opt.get();
	const CubeQuadTreeLocation::NodeLocation &regular_node_location_at_child_depth =
			regular_node_location_at_child_depth_opt.get();

	// If the nodes are on different cube faces then we need to transform the offsets of one of the
	// nodes so we can compare with the offsets of the other node.
	const int x_node_offset_1 = loose_node_location_at_parent_depth.x_node_offset;
	const int y_node_offset_1 = loose_node_location_at_parent_depth.y_node_offset;
	int x_node_offset_2, y_node_offset_2;
	if (loose_node_location_at_parent_depth.cube_face == regular_node_location_at_child_depth.cube_face)
	{
		x_node_offset_2 = regular_node_location_at_child_depth.x_node_offset;
		y_node_offset_2 = regular_node_location_at_child_depth.y_node_offset;
	}
	else
	{
		// The nodes are on different cube faces so we need to transform the offsets of one of the
		// nodes so we can compare with the offsets of the other node.
		CubeCoordinateFrame::get_cube_quad_tree_node_location_relative_to_cube_face(
				x_node_offset_2,
				y_node_offset_2,
				loose_node_location_at_parent_depth.cube_face/*transform_to_cube_face*/,
				regular_node_location_at_child_depth.cube_face/*transform_from_cube_face*/,
				regular_node_location_at_child_depth.quad_tree_depth,
				regular_node_location_at_child_depth.x_node_offset,
				regular_node_location_at_child_depth.y_node_offset);
	}

	//
	// Because the node offsets at the parent depth are offset by half the width of a child node
	// we can convert both parent and child offsets to grandchild offsets (the '+ 1' below)
	// and then the offset become one instead of a half. Also the comparison of the child node
	// offset against the loose bounds of the parent node become symmetrical and we only need
	// a single comparison. The threshold offset then becomes +/- 5.
	//

	const int x_diff = (x_node_offset_1 << 2) + 1 - (x_node_offset_2 << 1);
	const int x_diff_2 = x_diff * x_diff; // The equivalent of abs()
	if (x_diff_2 >= 25) // Same as "x_diff <= -5 || x_diff >= 5"
	{
		return false;
	}

	const int y_diff = (y_node_offset_1 << 2) + 1 - (y_node_offset_2 << 1);
	const int y_diff_2 = y_diff * y_diff;
	if (y_diff_2 >= 25)
	{
		return false;
	}

	return true;
}


bool
GPlatesMaths::intersect_loose_cube_quad_tree_location_with_regular_cube_quad_tree_location(
		const CubeQuadTreeLocation &loose_quad_tree_location,
		const CubeQuadTreeLocation &regular_quad_tree_location)
{
	// If either location represents the entire cube then return true because it means
	// the locations must intersect.
	const boost::optional<CubeQuadTreeLocation::NodeLocation> &loose_quad_tree_node_location_opt =
			loose_quad_tree_location.get_node_location();
	if (!loose_quad_tree_node_location_opt)
	{
		return true;
	}
	const boost::optional<CubeQuadTreeLocation::NodeLocation> &regular_quad_tree_node_location_opt =
			regular_quad_tree_location.get_node_location();
	if (!regular_quad_tree_node_location_opt)
	{
		return true;
	}

	const CubeQuadTreeLocation::NodeLocation &loose_quad_tree_node_location = loose_quad_tree_node_location_opt.get();
	const CubeQuadTreeLocation::NodeLocation &regular_quad_tree_node_location = regular_quad_tree_node_location_opt.get();

	// If the nodes are on different cube faces then we need to transform the offsets of one of the
	// nodes so we can compare with the offsets of the other node.
	const int x_node_offset_1 = loose_quad_tree_node_location.x_node_offset;
	const int y_node_offset_1 = loose_quad_tree_node_location.y_node_offset;
	int x_node_offset_2, y_node_offset_2;
	if (loose_quad_tree_node_location.cube_face == regular_quad_tree_node_location.cube_face)
	{
		x_node_offset_2 = regular_quad_tree_node_location.x_node_offset;
		y_node_offset_2 = regular_quad_tree_node_location.y_node_offset;
	}
	else
	{
		// The nodes are on different cube faces so we need to transform the offsets of one of the
		// nodes so we can compare with the offsets of the other node.
		CubeCoordinateFrame::get_cube_quad_tree_node_location_relative_to_cube_face(
				x_node_offset_2,
				y_node_offset_2,
				loose_quad_tree_node_location.cube_face/*transform_to_cube_face*/,
				regular_quad_tree_node_location.cube_face/*transform_from_cube_face*/,
				regular_quad_tree_node_location.quad_tree_depth,
				regular_quad_tree_node_location.x_node_offset,
				regular_quad_tree_node_location.y_node_offset);
	}

	const int diff_quad_tree_depth =
			loose_quad_tree_node_location.quad_tree_depth - regular_quad_tree_node_location.quad_tree_depth;

	if (diff_quad_tree_depth >= 0)
	{
		//
		// The logic here is similar to that in 'intersect_loose_quad_tree_node_with_regular_quad_tree_node_at_parent_child_depths'
		// except here it's arbitrary depth difference (instead of a difference of one).
		//

		const int lsh_diff = (1 << diff_quad_tree_depth);
		const int diff_plus_one = 1 + diff_quad_tree_depth;
		const int threshold = lsh_diff + 1;
		const int threshold_2 = threshold * threshold;

		const int x_diff = (x_node_offset_2 << diff_plus_one) + lsh_diff - 1 - (x_node_offset_1 << 1);
		const int x_diff_2 = x_diff * x_diff; // The equivalent of abs()
		if (x_diff_2 > threshold_2) // Same as "x_diff < -threshold || x_diff > threshold"
		{
			return false;
		}

		const int y_diff = (y_node_offset_2 << diff_plus_one) + lsh_diff - 1 - (y_node_offset_1 << 1);
		const int y_diff_2 = y_diff * y_diff; // The equivalent of abs()
		if (y_diff_2 > threshold_2) // Same as "y_diff < -threshold || y_diff > threshold"
		{
			return false;
		}
	}
	else
	{
		//
		// The logic here is similar to that in 'intersect_loose_quad_tree_node_with_regular_quad_tree_node_at_parent_child_depths'
		// except here it's arbitrary depth difference (instead of a difference of one).
		//

		const int diff = -diff_quad_tree_depth;
		const int lsh_diff = (1 << diff);
		const int diff_plus_one = 1 + diff;
		const int threshold = (1 << diff_plus_one) + 1;
		const int threshold_2 = threshold * threshold;

		const int x_diff = (x_node_offset_1 << diff_plus_one) + lsh_diff - 1 - (x_node_offset_2 << 1);
		const int x_diff_2 = x_diff * x_diff; // The equivalent of abs()
		if (x_diff_2 >= threshold_2) // Same as "x_diff <= -threshold || x_diff >= threshold"
		{
			return false;
		}

		const int y_diff = (y_node_offset_1 << diff_plus_one) + lsh_diff - 1 - (y_node_offset_2 << 1);
		const int y_diff_2 = y_diff * y_diff; // The equivalent of abs()
		if (y_diff_2 >= threshold_2) // Same as "y_diff <= -threshold || y_diff >= threshold"
		{
			return false;
		}
	}

	// Intersection detected.
	return true;
}


#if 0
bool
GPlatesMaths::intersect_loose_cube_quad_tree_location_with_loose_cube_quad_tree_location(
		const CubeQuadTreeLocation &loose_quad_tree_location_1,
		const CubeQuadTreeLocation &loose_quad_tree_location_2)
{
	return true;
}
#endif
