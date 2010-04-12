/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2009, 2010 The University of Sydney, Australia
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
#include <typeinfo>

#include "GmlTimeInstant.h"


std::ostream &
GPlatesPropertyValues::GmlTimeInstant::print_to(
		std::ostream &os) const
{
	return os << d_time_position;
}


bool
GPlatesPropertyValues::GmlTimeInstant::directly_modifiable_fields_equal(
		const PropertyValue &other) const
{
	try
	{
		const GmlTimeInstant &other_casted =
			dynamic_cast<const GmlTimeInstant &>(other);
		return d_time_position_xml_attributes == other_casted.d_time_position_xml_attributes;
	}
	catch (const std::bad_cast &)
	{
		// Should never get here, but doesn't hurt to check.
		return false;
	}
}

