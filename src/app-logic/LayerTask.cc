/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#include "LayerTask.h"

#include "ReconstructUtils.h"


boost::optional<GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type>
GPlatesAppLogic::LayerTask::extract_reconstruction_tree(
		const input_data_type &input_data,
		const ReconstructionTree::non_null_ptr_to_const_type &default_reconstruction_tree)
{
	std::vector<ReconstructionTree::non_null_ptr_to_const_type> reconstruction_trees;
	extract_input_channel_data(
			reconstruction_trees,
			get_reconstruction_tree_channel_name(),
			input_data);

	// If there's no reconstruction tree in the channel then return
	// the default reconstruction tree.
	if (reconstruction_trees.empty())
	{
		return default_reconstruction_tree;
	}

	if (reconstruction_trees.size() > 1)
	{
		// Expecting a single reconstruction tree.
		return boost::none;
	}

	// Return the sole reconstruction tree in the channel.
	return reconstruction_trees.front();
}
