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

#include "GmlDataBlock.h"


const GPlatesPropertyValues::GmlDataBlock::non_null_ptr_type
GPlatesPropertyValues::GmlDataBlock::deep_clone() const
{
	GmlDataBlock::non_null_ptr_type dup = clone();

	// Now we need to clear the tuple-list in the duplicate, before we push-back the cloned
	// coordinate lists.
	dup->tuple_list_clear();
	tuple_list_type::const_iterator iter, end = tuple_list_end();
	for (iter = tuple_list_begin(); iter != end; ++iter) {
		// GmlDataBlockCoordinateList doesn't contain any nested property values, so
		// regular 'clone' is fine.
		GmlDataBlockCoordinateList::non_null_ptr_type cloned_coord_list = (*iter)->clone();
		dup->tuple_list_push_back(cloned_coord_list);
	}
	return dup;
}
