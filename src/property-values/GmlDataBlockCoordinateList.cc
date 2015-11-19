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

#include <algorithm>

#include "GmlDataBlockCoordinateList.h"

#include "maths/Real.h"


namespace
{
	bool
	double_eq(
			double d1,
			double d2)
	{
		return GPlatesMaths::Real(d1) == GPlatesMaths::Real(d2);
	}
}


bool
GPlatesPropertyValues::GmlDataBlockCoordinateList::operator==(
		const GmlDataBlockCoordinateList &other) const
{
	if (d_value_object_type == other.d_value_object_type &&
			d_value_object_xml_attributes == other.d_value_object_xml_attributes)
	{
		if (d_coordinates.size() == other.d_coordinates.size())
		{
			return std::equal(
					d_coordinates.begin(),
					d_coordinates.end(),
					other.d_coordinates.begin(),
					&double_eq);
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}
