/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2008, 2010 The University of Sydney, Australia
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

#include <iostream>

#include "GmlLineString.h"
#include "maths/PolylineOnSphere.h"


const GPlatesPropertyValues::GmlLineString::non_null_ptr_type
GPlatesPropertyValues::GmlLineString::create(
		const internal_polyline_type &polyline_)
{
	// Because PolylineOnSphere can only ever be handled via a non_null_ptr_to_const_type,
	// there is no way a PolylineOnSphere instance can be changed.  Hence, it is safe to store
	// a pointer to the instance which was passed into this 'create' function.
	GmlLineString::non_null_ptr_type line_string_ptr(new GmlLineString(polyline_));
	return line_string_ptr;
}


std::ostream &
GPlatesPropertyValues::GmlLineString::print_to(
		std::ostream &os) const
{
	// FIXME: Implement properly when actually needed for debugging.
	return os << "{ GmlLineString }";
}

