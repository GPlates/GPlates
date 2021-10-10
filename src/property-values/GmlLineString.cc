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

#include "GmlLineString.h"
#include "maths/PolylineOnSphere.h"


const GPlatesPropertyValues::GmlLineString::non_null_ptr_type
GPlatesPropertyValues::GmlLineString::create(
		const internal_polyline_type &polyline_)
{
	// Because PolylineOnSphere is mutable, we must clone the polyline we are given,
	// otherwise it might be possible to change the polyline without our knowledge.
	GmlLineString::non_null_ptr_type line_string_ptr(*(new GmlLineString(
			polyline_->clone_on_heap())));
	return line_string_ptr;
}
