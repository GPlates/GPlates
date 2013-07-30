/* $Id$ */

/**
 * \file 
 * Contains the implementation of the class TopLevelProperty.
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

#include <typeinfo>

#include "TopLevelProperty.h"


bool
GPlatesModel::TopLevelProperty::operator==(
		const TopLevelProperty &other) const
{
	// Both objects must have the same type before testing for equality.
	// This also means derived classes need no type-checking.
	if (typeid(*this) != typeid(other))
	{
		return false;
	}

	// Compare the derived type objects.
	return equality(other);
}


bool
GPlatesModel::TopLevelProperty::equality(
		const TopLevelProperty &other) const
{
	return d_property_name == other.d_property_name &&
			d_xml_attributes == other.d_xml_attributes;
}


std::ostream &
GPlatesModel::operator<<(
		std::ostream &os,
		const TopLevelProperty &top_level_prop)
{
	return top_level_prop.print_to(os);
}

