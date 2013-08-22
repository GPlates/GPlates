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

#include <iostream>

#include "GpmlKeyValueDictionaryElement.h"


bool
GPlatesPropertyValues::GpmlKeyValueDictionaryElement::operator==(
		const GpmlKeyValueDictionaryElement &other) const
{
	return *d_key == *other.d_key &&
		*d_value == *other.d_value &&
		d_value_type == other.d_value_type;
}


std::ostream &
GPlatesPropertyValues::operator<<(
		std::ostream &os,
		const GpmlKeyValueDictionaryElement &element)
{
	return os << *element.key() << ":" << *element.value();
}

