/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
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

#include "GpmlTopologicalPolygon.h"


const GPlatesPropertyValues::GpmlTopologicalPolygon::non_null_ptr_type
GPlatesPropertyValues::GpmlTopologicalPolygon::deep_clone() const
{
	GpmlTopologicalPolygon::non_null_ptr_type dup = clone();

	// Now we need to clear the topological-section vector in the duplicate, before we
	// push-back the cloned sections.
	dup->d_sections.clear();
	std::vector<GpmlTopologicalSection::non_null_ptr_type>::const_iterator iter,
			end = d_sections.end();
	for (iter = d_sections.begin(); iter != end; ++iter) {
		GpmlTopologicalSection::non_null_ptr_type cloned_section =
				(*iter)->deep_clone_as_topo_section();
		dup->d_sections.push_back(cloned_section);
	}

	return dup;
}
