/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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

#include <ostream>
#include "ReconstructionTreeEdge.h"


void
GPlatesModel::output_for_debugging(
		std::ostream &os,
		const ReconstructionTreeEdge &edge)
{
	os << "Edge: moving " << edge.moving_plate() << ", fixed " << edge.fixed_plate();
	if (edge.pole_type() == ReconstructionTreeEdge::PoleTypes::ORIGINAL) {
		os << ", original, edge memory location ";
	} else {
		os << ", reversed, edge memory location ";
	}
	os << (&edge) << ",\n finite rotation " << edge.composed_absolute_rotation() << std::endl;
}


GPlatesModel::ReconstructionTreeEdge::~ReconstructionTreeEdge()
{
	// Unsubscribe all the edge's children, so they aren't left with dangling pointers.
	edge_collection_type::iterator it;
	edge_collection_type::iterator it_begin = d_children_in_built_tree.begin();
	edge_collection_type::iterator it_end = d_children_in_built_tree.end();

	for (it = it_begin; it != it_end ; it++)
	{
		(*it)->set_parent_edge(NULL);
	}
}

